from esphome import automation
import esphome.codegen as cg
from esphome.components import runtime_image
from esphome.components.const import CONF_REQUEST_HEADERS
from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent
from esphome.components.image import CONF_TRANSPARENCY, add_metadata
import esphome.config_validation as cv
from esphome.const import CONF_BUFFER_SIZE, CONF_ID, CONF_ON_ERROR, CONF_TYPE, CONF_URL
from esphome.core import ID, Lambda
from esphome.cpp_generator import MockObj, TemplateArgsType
from esphome.types import ConfigType

AUTO_LOAD = ["runtime_image"]
DEPENDENCIES = ["http_request"]
CODEOWNERS = ["@slump1vn"]

CONF_ON_FRAME = "on_frame"

mjpeg_stream_ns = cg.esphome_ns.namespace("mjpeg_stream")

MjpegStream = mjpeg_stream_ns.class_(
    "MjpegStream", cg.Component, runtime_image.RuntimeImage
)

StartAction = mjpeg_stream_ns.class_(
    "MjpegStartAction", automation.Action, cg.Parented.template(MjpegStream)
)
StopAction = mjpeg_stream_ns.class_(
    "MjpegStopAction", automation.Action, cg.Parented.template(MjpegStream)
)

MJPEG_STREAM_SCHEMA = runtime_image.runtime_image_schema(MjpegStream).extend(
    {
        cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
        cv.Required(CONF_URL): cv.url,
        # Largest single JPEG frame the stream may carry, in bytes.
        cv.Optional(CONF_BUFFER_SIZE, default=131072): cv.int_range(4096, 1048576),
        cv.Optional(CONF_REQUEST_HEADERS): cv.All(
            cv.Schema({cv.string: cv.templatable(cv.string)})
        ),
        cv.Optional(CONF_ON_FRAME): automation.validate_automation({}),
        cv.Optional(CONF_ON_ERROR): automation.validate_automation({}),
    }
)

CONFIG_SCHEMA = cv.All(
    MJPEG_STREAM_SCHEMA,
    cv.only_with_framework("esp-idf"),
    runtime_image.validate_runtime_image_settings,
)

START_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MjpegStream),
        cv.Optional(CONF_URL): cv.templatable(cv.url),
    }
)

STOP_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(MjpegStream),
    }
)


@automation.register_action(
    "mjpeg_stream.start", StartAction, START_SCHEMA, synchronous=True
)
@automation.register_action(
    "mjpeg_stream.stop", StopAction, STOP_SCHEMA, synchronous=True
)
async def mjpeg_stream_action_to_code(
    config: ConfigType,
    action_id: ID,
    template_arg: cg.TemplateArguments,
    args: TemplateArgsType,
) -> MockObj:
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    if CONF_URL in config:
        template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
        cg.add(var.set_url(template_))
    return var


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(CONF_ON_FRAME, "add_on_frame_callback"),
    automation.CallbackAutomation(CONF_ON_ERROR, "add_on_error_callback"),
)


async def to_code(config: ConfigType) -> None:
    settings = await runtime_image.process_runtime_image_config(config)
    add_metadata(
        config[CONF_ID],
        settings.width,
        settings.height,
        config[CONF_TYPE],
        config[CONF_TRANSPARENCY],
    )

    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_URL],
        settings.width,
        settings.height,
        settings.format_enum,
        settings.image_type_enum,
        settings.transparent,
        settings.placeholder or cg.nullptr,
        config[CONF_BUFFER_SIZE],
        settings.byte_order_big_endian,
    )
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_HTTP_REQUEST_ID])

    for key, value in config.get(CONF_REQUEST_HEADERS, {}).items():
        if isinstance(value, Lambda):
            template_ = await cg.templatable(value, [], cg.std_string)
            cg.add(var.add_request_header(key, template_))
        else:
            cg.add(var.add_request_header(key, value))

    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)
