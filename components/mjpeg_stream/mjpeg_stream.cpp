#include "mjpeg_stream.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include <algorithm>
#include <cstring>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

static const char *const TAG = "mjpeg_stream";

namespace esphome::mjpeg_stream {

static constexpr size_t NPOS = static_cast<size_t>(-1);
static constexpr uint32_t TASK_STACK_BYTES = 14336;
static constexpr size_t READ_CHUNK_BYTES = 8192;

/// Find the two-byte marker (first, second) in buf[from, len); NPOS if absent.
static size_t find_marker(const uint8_t *buf, size_t from, size_t len, uint8_t first, uint8_t second) {
  size_t i = from;
  while (i + 1 < len) {
    const void *hit = memchr(buf + i, first, len - i - 1);
    if (hit == nullptr) {
      return NPOS;
    }
    i = static_cast<const uint8_t *>(hit) - buf;
    if (buf[i + 1] == second) {
      return i;
    }
    i++;
  }
  return NPOS;
}

void MjpegStream::log_heap_(const char *where) {
#ifdef USE_ESP32
  ESP_LOGI(TAG, "%s: internal free %u (largest %u), PSRAM free %u (largest %u)", where,
           (unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
           (unsigned) heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
#endif
}

void MjpegStream::setup() {
  // Internal RAM is scarce on this board; only log it so low-memory problems are easy to spot.
  this->log_heap_("setup");
}

void MjpegStream::dump_config() {
  ESP_LOGCONFIG(TAG,
                "MJPEG stream:\n"
                "  Frame buffer: %zu bytes",
                this->frame_buffer_size_);
}

void MjpegStream::set_url(const std::string &url) {
  bool restart = false;
  {
    std::lock_guard<std::mutex> guard(this->mutex_);
    restart = this->url_ != url;
    this->url_ = url;
  }
  if (restart && this->active_.load()) {
    // Bump the generation so the task drops the current connection and reconnects.
    this->generation_atomic_.fetch_add(1);
  }
}

void MjpegStream::start() {
  std::vector<http_request::Header> resolved;
  for (auto &header : this->request_headers_) {
    resolved.push_back(http_request::Header{header.first, header.second.value()});
  }
  {
    std::lock_guard<std::mutex> guard(this->mutex_);
    this->resolved_headers_ = std::move(resolved);
  }
  this->generation_atomic_.fetch_add(1);
  this->active_.store(true);

#ifdef USE_ESP32
  if (this->task_ == nullptr) {
    // Core 0 hosts the Wi-Fi stack; keep the main loop (core 1) free for LVGL.
    BaseType_t ok = xTaskCreatePinnedToCore(MjpegStream::task_entry_, "mjpeg", TASK_STACK_BYTES, this, 2,
                                            &this->task_, 0);
    if (ok != pdPASS) {
      ESP_LOGE(TAG, "Could not create the streaming task");
      this->task_ = nullptr;
      this->active_.store(false);
      this->error_flag_.store(true);
      this->enable_loop();
      return;
    }
  }
#endif
  this->enable_loop();
}

void MjpegStream::stop() {
  this->active_.store(false);
  this->generation_atomic_.fetch_add(1);
}

void MjpegStream::loop() {
  if (this->frame_ready_.exchange(false)) {
    this->frame_callback_.call();
  }
  if (this->error_flag_.exchange(false)) {
    this->error_callback_.call();
  }
}

#ifdef USE_ESP32
void MjpegStream::task_entry_(void *arg) {
  static_cast<MjpegStream *>(arg)->task_main_();
  vTaskDelete(nullptr);
}
#endif

bool MjpegStream::should_run_(uint32_t generation) const {
  return this->active_.load() && this->generation_atomic_.load() == generation;
}

void MjpegStream::wait_while_running_(uint32_t generation, uint32_t ms) {
  for (uint32_t waited = 0; waited < ms && this->should_run_(generation); waited += 50) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void MjpegStream::task_main_() {
  RAMAllocator<uint8_t> allocator;
  uint8_t *buf = allocator.allocate(this->frame_buffer_size_);
  if (buf == nullptr) {
    ESP_LOGE(TAG, "Could not allocate the %zu byte frame buffer", this->frame_buffer_size_);
    this->error_flag_.store(true);
    return;
  }

  while (true) {
    if (!this->active_.load()) {
      vTaskDelay(pdMS_TO_TICKS(50));
      continue;
    }

    std::string url;
    std::vector<http_request::Header> headers;
    uint32_t generation;
    {
      std::lock_guard<std::mutex> guard(this->mutex_);
      url = this->url_;
      headers = this->resolved_headers_;
      generation = this->generation_atomic_.load();
    }

    this->stream_once_(buf, url, headers, generation);

    // Give a failed connection a moment before retrying (stop/url changes cut this short).
    this->wait_while_running_(generation, 1500);
  }
}

void MjpegStream::stream_once_(uint8_t *buf, const std::string &url,
                               const std::vector<http_request::Header> &headers, uint32_t generation) {
  this->log_heap_("connecting");
  ESP_LOGI(TAG, "Connecting to %s", url.c_str());
  auto conn = this->parent_->get(url, headers, {"content-type"});
  if (conn == nullptr) {
    ESP_LOGW(TAG, "Connection failed");
    this->error_flag_.store(true);
    return;
  }
  if (conn->status_code != 200) {
    ESP_LOGW(TAG, "HTTP status %d", conn->status_code);
    conn->end();
    this->error_flag_.store(true);
    return;
  }
  ESP_LOGI(TAG, "Streaming (%s)", conn->get_response_header("content-type").c_str());

  size_t len = 0;
  size_t scan_from = 2;
  uint32_t frames = 0, dropped = 0, window_start = millis(), decode_ms = 0;
  size_t window_bytes = 0;

  while (this->should_run_(generation)) {
    size_t space = this->frame_buffer_size_ - len;
    if (space == 0) {
      // No end-of-frame within the buffer: discard it and resynchronise on the next SOI.
      ESP_LOGW(TAG, "Frame larger than the %zu byte buffer; dropping", this->frame_buffer_size_);
      len = 0;
      scan_from = 2;
      dropped++;
      continue;
    }

    int n = conn->read(buf + len, std::min(space, READ_CHUNK_BYTES));
    if (n < 0) {
      ESP_LOGW(TAG, "Read error %d", n);
      this->error_flag_.store(true);
      break;
    }
    if (n == 0) {
      if (conn->is_read_complete()) {
        ESP_LOGW(TAG, "Stream ended by server");
        this->error_flag_.store(true);
        break;
      }
      vTaskDelay(1);
      continue;
    }
    len += n;
    window_bytes += n;

    for (;;) {
      size_t soi = find_marker(buf, 0, len, 0xFF, 0xD8);
      if (soi == NPOS) {
        // Keep a trailing 0xFF: it may be the first half of a marker split across reads.
        if (len > 0 && buf[len - 1] == 0xFF) {
          buf[0] = 0xFF;
          len = 1;
        } else {
          len = 0;
        }
        scan_from = 2;
        break;
      }
      if (soi > 0) {
        memmove(buf, buf + soi, len - soi);
        len -= soi;
        scan_from = 2;
      }
      size_t eoi = find_marker(buf, scan_from, len, 0xFF, 0xD9);
      if (eoi == NPOS) {
        scan_from = len > 1 ? len - 1 : 2;
        break;
      }
      size_t frame_len = eoi + 2;
      uint32_t t0 = millis();
      if (this->decode_frame_(buf, frame_len)) {
        frames++;
      } else {
        dropped++;
      }
      decode_ms += millis() - t0;
      memmove(buf, buf + frame_len, len - frame_len);
      len -= frame_len;
      scan_from = 2;
    }

    uint32_t now = millis();
    if (now - window_start >= 5000) {
      float seconds = (now - window_start) / 1000.0f;
      ESP_LOGI(TAG, "%.1f fps, %u dropped, avg frame %u B, avg decode %u ms", frames / seconds, (unsigned) dropped,
               frames ? (unsigned) (window_bytes / frames) : 0u, frames ? (unsigned) (decode_ms / frames) : 0u);
      frames = 0;
      dropped = 0;
      decode_ms = 0;
      window_bytes = 0;
      window_start = now;
    }
  }

  conn->end();
}

/// Copy one decoded pixel block (RGB565) straight into the image buffer.
int MjpegStream::fast_draw_(JPEGDRAW *draw) {
  auto *self = static_cast<MjpegStream *>(draw->pUser);
  uint8_t *fb = self->buffer_;
  const int width = self->get_buffer_width();
  const int height = self->get_buffer_height();
  if (fb == nullptr || draw->x >= width) {
    return 1;
  }
  const int copy_w = std::min(draw->iWidthUsed > 0 ? draw->iWidthUsed : draw->iWidth, width - draw->x);
  for (int row = 0; row < draw->iHeight; row++) {
    int y = draw->y + row;
    if (y >= height) {
      break;
    }
    memcpy(fb + (static_cast<size_t>(y) * width + draw->x) * 2, draw->pPixels + static_cast<size_t>(row) * draw->iWidth,
           static_cast<size_t>(copy_w) * 2);
  }
  return 1;
}

/// JPEGDEC emits RGB565 blocks that are copied row by row (no per-pixel calls). The generic
/// RuntimeImage decoder is bypassed: it costs ~840 ms per frame and a second ~18 KB JPEGDEC
/// in internal RAM. The stream's frame size must equal the configured image size.
bool MjpegStream::decode_frame_(uint8_t *frame, size_t length) {
  if (!this->fast_jpeg_.openRAM(frame, static_cast<int>(length), MjpegStream::fast_draw_)) {
    ESP_LOGW(TAG, "Not a decodable JPEG (error %d)", this->fast_jpeg_.getLastError());
    return false;
  }
  const int w = this->fast_jpeg_.getWidth();
  const int h = this->fast_jpeg_.getHeight();
  if (this->buffer_ == nullptr && this->resize(w, h) == 0) {
    ESP_LOGW(TAG, "Could not allocate the %dx%d image buffer", w, h);
    this->fast_jpeg_.close();
    return false;
  }
  if (this->get_buffer_width() != w || this->get_buffer_height() != h) {
    ESP_LOGW(TAG, "Frame is %dx%d but the image is configured for %dx%d; set the stream size to match", w, h,
             this->get_buffer_width(), this->get_buffer_height());
    this->fast_jpeg_.close();
    return false;
  }
  this->fast_jpeg_.setUserPointer(this);
  this->fast_jpeg_.setPixelType(this->is_big_endian_ ? RGB565_BIG_ENDIAN : RGB565_LITTLE_ENDIAN);
  bool ok = this->fast_jpeg_.decode(0, 0, 0) != 0;
  this->fast_jpeg_.close();
  if (!ok) {
    ESP_LOGW(TAG, "JPEG decode failed (error %d)", this->fast_jpeg_.getLastError());
    return false;
  }
  // Publish the buffer to LVGL (done once; the buffer is reused for every later frame).
  this->width_ = this->buffer_width_;
  this->height_ = this->buffer_height_;
  this->data_start_ = this->buffer_;
  this->frame_ready_.store(true);
  return true;
}

}  // namespace esphome::mjpeg_stream
