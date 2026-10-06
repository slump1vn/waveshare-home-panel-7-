#pragma once

#include "esphome/components/http_request/http_request.h"
#include "esphome/components/runtime_image/runtime_image.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

#include <JPEGDEC.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#ifdef USE_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace esphome::mjpeg_stream {

/**
 * @brief Keeps a single HTTP(S) connection open and decodes the JPEG frames of a
 * multipart MJPEG stream into a RuntimeImage.
 *
 * Network reads and JPEG decoding run in their own FreeRTOS task so the main loop
 * (LVGL) is never blocked. The main loop only fires the on_frame / on_error callbacks.
 * Frames are split on the JPEG SOI (FFD8) / EOI (FFD9) markers, so no multipart
 * header parsing is needed.
 */
class MjpegStream final : public Component,
                          public runtime_image::RuntimeImage,
                          public Parented<esphome::http_request::HttpRequestComponent> {
 public:
  MjpegStream(const std::string &url, int width, int height, runtime_image::ImageFormat format,
              image::ImageType type, image::Transparency transparency, image::Image *placeholder,
              uint32_t buffer_size, bool is_big_endian = false)
      : RuntimeImage(format, type, transparency, placeholder, is_big_endian, width, height),
        url_(url),
        is_big_endian_(is_big_endian),
        frame_buffer_size_(buffer_size) {}

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }

  /// Set the stream URL (restarts the connection if the stream is running).
  void set_url(const std::string &url);

  template<typename V> void add_request_header(const std::string &header, V value) {
    this->request_headers_.push_back(std::pair<std::string, TemplatableValue<std::string>>(header, value));
  }

  /// Start (or restart) streaming from the current URL.
  void start();
  /// Stop streaming and close the connection.
  void stop();
  bool is_streaming() const { return this->active_.load(); }

  template<typename F> void add_on_frame_callback(F &&callback) { this->frame_callback_.add(std::forward<F>(callback)); }
  template<typename F> void add_on_error_callback(F &&callback) { this->error_callback_.add(std::forward<F>(callback)); }

 protected:
#ifdef USE_ESP32
  static void task_entry_(void *arg);
#endif
  void task_main_();
  void stream_once_(uint8_t *buf, const std::string &url, const std::vector<http_request::Header> &headers,
                    uint32_t generation);
  bool decode_frame_(uint8_t *frame, size_t length);
  static int fast_draw_(JPEGDRAW *draw);
  JPEGDEC fast_jpeg_{};
  bool is_big_endian_;
  void log_heap_(const char *where);
  bool should_run_(uint32_t generation) const;
  void wait_while_running_(uint32_t generation, uint32_t ms);

  std::mutex mutex_;
  std::string url_;
  std::vector<std::pair<std::string, TemplatableValue<std::string>>> request_headers_;
  std::vector<http_request::Header> resolved_headers_;

  std::atomic<bool> active_{false};
  std::atomic<bool> frame_ready_{false};
  std::atomic<bool> error_flag_{false};
  std::atomic<uint32_t> generation_atomic_{0};

  size_t frame_buffer_size_;
  CallbackManager<void()> frame_callback_{};
  CallbackManager<void()> error_callback_{};

#ifdef USE_ESP32
  TaskHandle_t task_{nullptr};
#endif
};

template<typename... Ts> class MjpegStartAction final : public Action<Ts...> {
 public:
  MjpegStartAction(MjpegStream *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, url)
  void play(const Ts &...x) override {
    if (this->url_.has_value()) {
      this->parent_->set_url(this->url_.value(x...));
    }
    this->parent_->start();
  }

 protected:
  MjpegStream *parent_;
};

template<typename... Ts> class MjpegStopAction final : public Action<Ts...> {
 public:
  MjpegStopAction(MjpegStream *parent) : parent_(parent) {}
  void play(const Ts &...x) override { this->parent_->stop(); }

 protected:
  MjpegStream *parent_;
};

}  // namespace esphome::mjpeg_stream
