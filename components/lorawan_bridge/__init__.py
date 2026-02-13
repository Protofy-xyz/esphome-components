import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import uart
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_MESSAGE

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@Protofy-xyz"]

CONF_POWER_PIN = "power_pin"
CONF_POWER_ON_DELAY = "power_on_delay"
CONF_BOOT_TIMEOUT = "boot_timeout"
CONF_ACK_TIMEOUT = "ack_timeout"
CONF_ENABLE_ON_BOOT = "enable_on_boot"
CONF_ON_READY = "on_ready"
CONF_ON_MESSAGE = "on_message"
CONF_ON_SEND_SUCCESS = "on_send_success"
CONF_ON_SEND_FAILED = "on_send_failed"

lorawan_bridge_ns = cg.esphome_ns.namespace("lorawan_bridge")
LoRaWANBridgeComponent = lorawan_bridge_ns.class_(
    "LoRaWANBridgeComponent", cg.Component, uart.UARTDevice
)

# Triggers
ReadyTrigger = lorawan_bridge_ns.class_("ReadyTrigger", automation.Trigger.template())
MessageTrigger = lorawan_bridge_ns.class_(
    "MessageTrigger", automation.Trigger.template(cg.std_string)
)
SendSuccessTrigger = lorawan_bridge_ns.class_(
    "SendSuccessTrigger", automation.Trigger.template()
)
SendFailedTrigger = lorawan_bridge_ns.class_(
    "SendFailedTrigger", automation.Trigger.template()
)

# Actions
SendTextAction = lorawan_bridge_ns.class_("SendTextAction", automation.Action)
PowerOnAction = lorawan_bridge_ns.class_("PowerOnAction", automation.Action)
PowerOffAction = lorawan_bridge_ns.class_("PowerOffAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LoRaWANBridgeComponent),
            cv.Optional(CONF_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Optional(
                CONF_POWER_ON_DELAY, default="2s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_BOOT_TIMEOUT, default="15s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_ACK_TIMEOUT, default="10s"
            ): cv.positive_time_period_milliseconds,
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

    cg.add(var.set_power_on_delay(config[CONF_POWER_ON_DELAY]))
    cg.add(var.set_boot_timeout(config[CONF_BOOT_TIMEOUT]))
    cg.add(var.set_ack_timeout(config[CONF_ACK_TIMEOUT]))
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
        cv.GenerateID(): cv.use_id(LoRaWANBridgeComponent),
        cv.Required(CONF_MESSAGE): cv.templatable(cv.string),
    }
)

POWER_ON_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(LoRaWANBridgeComponent),
    }
)

POWER_OFF_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(LoRaWANBridgeComponent),
    }
)


@automation.register_action(
    "lorawan_bridge.send_text", SendTextAction, SEND_TEXT_SCHEMA
)
async def send_text_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    tmpl = await cg.templatable(config[CONF_MESSAGE], args, cg.std_string)
    cg.add(var.set_message(tmpl))
    return var


@automation.register_action(
    "lorawan_bridge.power_on", PowerOnAction, POWER_ON_SCHEMA
)
async def power_on_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action(
    "lorawan_bridge.power_off", PowerOffAction, POWER_OFF_SCHEMA
)
async def power_off_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var
