import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID, CONF_TRIGGER_ID

DEPENDENCIES = ["uart"]

gm77_ns = cg.esphome_ns.namespace("gm77")
GM77Component = gm77_ns.class_("GM77Component", cg.Component, uart.UARTDevice, cg.Controller)

CONF_ON_TAG = "on_tag"
OnTagTrigger = gm77_ns.class_("OnTagTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.All(
    cv.ensure_list(
        cv.Schema(
            {
                cv.GenerateID(): cv.declare_id(GM77Component),
                cv.Required(CONF_UART_ID): cv.use_id(uart.UARTComponent),
                cv.Optional(CONF_ON_TAG): automation.validate_automation(
                    {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnTagTrigger)}
                ),
            }
        ).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)
    )
)

async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)
        await uart.register_uart_device(var, conf)

        for tag_conf in conf.get(CONF_ON_TAG, []):
            trigger = cg.new_Pvariable(tag_conf[CONF_TRIGGER_ID], var)
            await automation.build_automation(trigger, [(cg.std_string, "bytes")], tag_conf)
