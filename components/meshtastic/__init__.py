import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import uart
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_CHANNEL, CONF_MESSAGE

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@ton"]

CONF_POWER_PIN = "power_pin"
CONF_BOOT_TIMEOUT = "boot_timeout"
CONF_ACK_TIMEOUT = "ack_timeout"
CONF_DESTINATION = "destination"
CONF_ENABLE_ON_BOOT = "enable_on_boot"
CONF_ON_READY = "on_ready"
CONF_ON_MESSAGE = "on_message"
CONF_ON_SEND_SUCCESS = "on_send_success"
CONF_ON_SEND_FAILED = "on_send_failed"

meshtastic_ns = cg.esphome_ns.namespace("meshtastic")
MeshtasticComponent = meshtastic_ns.class_(
    "MeshtasticComponent", cg.Component, uart.UARTDevice
)

# Triggers
ReadyTrigger = meshtastic_ns.class_("ReadyTrigger", automation.Trigger.template())
MessageTrigger = meshtastic_ns.class_(
    "MessageTrigger", automation.Trigger.template(cg.std_string)
)
SendSuccessTrigger = meshtastic_ns.class_(
    "SendSuccessTrigger", automation.Trigger.template()
)
SendFailedTrigger = meshtastic_ns.class_(
    "SendFailedTrigger", automation.Trigger.template()
)

# Actions
SendTextAction = meshtastic_ns.class_("SendTextAction", automation.Action)
PowerOnAction = meshtastic_ns.class_("PowerOnAction", automation.Action)
PowerOffAction = meshtastic_ns.class_("PowerOffAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MeshtasticComponent),
            cv.Optional(CONF_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Optional(
                CONF_BOOT_TIMEOUT, default="30s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_ACK_TIMEOUT, default="30s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_DESTINATION, default=0xFFFFFFFF): cv.hex_uint32_t,
            cv.Optional(CONF_CHANNEL, default=0): cv.uint8_t,
            cv.Optional(CONF_ENABLE_ON_BOOT, default=True): cv.boolean,
            cv.Optional(CONF_ON_READY): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ReadyTrigger)}
            ),
            cv.Optional(CONF_ON_MESSAGE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MessageTrigger)}
            ),
            cv.Optional(CONF_ON_SEND_SUCCESS): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SendSuccessTrigger)}
            ),
            cv.Optional(CONF_ON_SEND_FAILED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SendFailedTrigger)}
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_POWER_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_POWER_PIN])
        cg.add(var.set_power_pin(pin))

    cg.add(var.set_boot_timeout(config[CONF_BOOT_TIMEOUT]))
    cg.add(var.set_ack_timeout(config[CONF_ACK_TIMEOUT]))
    cg.add(var.set_default_destination(config[CONF_DESTINATION]))
    cg.add(var.set_default_channel(config[CONF_CHANNEL]))
    cg.add(var.set_enable_on_boot(config[CONF_ENABLE_ON_BOOT]))

    for conf in config.get(CONF_ON_READY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for conf in config.get(CONF_ON_SEND_SUCCESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_SEND_FAILED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


# --- Actions ---

SEND_TEXT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
        cv.Required(CONF_MESSAGE): cv.templatable(cv.string),
        cv.Optional(CONF_DESTINATION): cv.templatable(cv.hex_uint32_t),
        cv.Optional(CONF_CHANNEL): cv.templatable(cv.uint8_t),
    }
)

POWER_ON_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)

POWER_OFF_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)


@automation.register_action(
    "meshtastic.send_text", SendTextAction, SEND_TEXT_SCHEMA
)
async def send_text_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    tmpl = await cg.templatable(config[CONF_MESSAGE], args, cg.std_string)
    cg.add(var.set_message(tmpl))
    if CONF_DESTINATION in config:
        tmpl = await cg.templatable(config[CONF_DESTINATION], args, cg.uint32)
        cg.add(var.set_destination(tmpl))
    if CONF_CHANNEL in config:
        tmpl = await cg.templatable(config[CONF_CHANNEL], args, cg.uint8)
        cg.add(var.set_channel(tmpl))
    return var


@automation.register_action("meshtastic.power_on", PowerOnAction, POWER_ON_SCHEMA)
async def power_on_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action("meshtastic.power_off", PowerOffAction, POWER_OFF_SCHEMA)
async def power_off_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var
