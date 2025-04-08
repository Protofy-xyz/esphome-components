import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID
from esphome import automation

CODEOWNERS = ["@TonProtofy"]
DEPENDENCIES = ["i2c"]

pca9632_ns = cg.esphome_ns.namespace("pca9632")
PCA9632Component = pca9632_ns.class_("PCA9632Component", cg.Component, i2c.I2CDevice)

SetPWMAction = pca9632_ns.class_("SetPWMAction", automation.Action)
SetCurrentAction = pca9632_ns.class_("SetCurrentAction", automation.Action)
SetColorAction = pca9632_ns.class_("SetColorAction", automation.Action)
SetGroupModeAction = pca9632_ns.class_("SetGroupModeAction", automation.Action)
SetBlinkingAction = pca9632_ns.class_("SetBlinkingAction", automation.Action)

CONF_CHANNEL = "channel"
CONF_VALUE = "value"
CONF_CURRENT = "current"
CONF_RED = "red"
CONF_GREEN = "green"
CONF_BLUE = "blue"
CONF_WHITE = "white"
CONF_MODE = "mode"
CONF_PERIOD = "period"
CONF_DUTY_CYCLE = "duty_cycle"

CONFIG_SCHEMA = (
    cv.Schema({cv.GenerateID(): cv.declare_id(PCA9632Component)})
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x60))  # default I2C address
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

@automation.register_action(
    "pca9632.set_pwm", SetPWMAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(PCA9632Component),
        cv.Required(CONF_CHANNEL): cv.templatable(cv.uint8_t),
        cv.Required(CONF_VALUE): cv.templatable(cv.uint8_t),
    }),
)
async def set_pwm_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_channel(await cg.templatable(config[CONF_CHANNEL], args, cg.uint8)))
    cg.add(var.set_value(await cg.templatable(config[CONF_VALUE], args, cg.uint8)))
    return var


def validate_current(value):
    # PCA9632 tiene 25mA máx (a veces 24 dependiendo del modelo)
    value = cv.string_strict(value)
    if value.endswith("mA"):
        ma = float(value[:-2])
        raw = round((ma / 25.0) * 255)
        return max(0, min(255, raw))
    return cv.uint8_t(value)
@automation.register_action(
    "pca9632.set_current", SetCurrentAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(PCA9632Component),
        cv.Required(CONF_CURRENT): cv.templatable(validate_current),
    }),
)
async def set_current_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_value(await cg.templatable(config[CONF_CURRENT], args, cg.uint8)))
    return var




@automation.register_action(
    "pca9632.set_color", SetColorAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(PCA9632Component),
        cv.Required(CONF_RED): cv.templatable(cv.uint8_t),
        cv.Required(CONF_GREEN): cv.templatable(cv.uint8_t),
        cv.Required(CONF_BLUE): cv.templatable(cv.uint8_t),
        cv.Required(CONF_WHITE): cv.templatable(cv.uint8_t)
    }),
)
async def set_color_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_r(await cg.templatable(config[CONF_RED], args, cg.uint8)))
    cg.add(var.set_g(await cg.templatable(config[CONF_GREEN], args, cg.uint8)))
    cg.add(var.set_b(await cg.templatable(config[CONF_BLUE], args, cg.uint8)))
    cg.add(var.set_w(await cg.templatable(config[CONF_WHITE], args, cg.uint8)))
    return var


@automation.register_action(
    "pca9632.set_group_control_mode", SetGroupModeAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(PCA9632Component),
        cv.Required(CONF_MODE): cv.templatable(cv.boolean),
    }),
)
async def set_group_mode_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_mode(await cg.templatable(config[CONF_MODE], args, bool)))
    return var

def validate_duty_cycle(value):
    value = cv.percentage(value)
    return round(value * 255)

@automation.register_action(
    "pca9632.set_blinking", SetBlinkingAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(PCA9632Component),
        cv.Required(CONF_PERIOD): cv.templatable(cv.positive_time_period_milliseconds),
        cv.Required(CONF_DUTY_CYCLE): cv.templatable(validate_duty_cycle),
    }),
)

async def set_blinking_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    period = await cg.templatable(config[CONF_PERIOD], args, cg.uint32)
    duty = await cg.templatable(config[CONF_DUTY_CYCLE], args, cg.uint8)
    cg.add(var.set_period(period))
    cg.add(var.set_duty(duty))
    return var