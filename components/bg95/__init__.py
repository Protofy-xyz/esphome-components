import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import CONF_ID

# DEPENDENCIES = ["uart"]
CODEOWNERS = ["@glmnet"]

bg95_ns = cg.esphome_ns.namespace("bg95")
BG95Component = bg95_ns.class_("BG95Component", cg.Component)

MULTI_CONF = True


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BG95Component),
            cv.Required("tx_pin"): pins.gpio_output_pin_schema,
            cv.Required("rx_pin"): pins.gpio_output_pin_schema,
            cv.Required("enable_pin"): pins.gpio_output_pin_schema,
            cv.Required("on_pin"): pins.gpio_output_pin_schema,
        }
    )
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    tx_pin = await cg.gpio_pin_expression(config["tx_pin"])
    cg.add(var.set_tx_pin(tx_pin))
    rx_pin = await cg.gpio_pin_expression(config["rx_pin"])
    cg.add(var.set_rx_pin(rx_pin))
    enable_pin = await cg.gpio_pin_expression(config["enable_pin"])
    cg.add(var.set_enable_pin(enable_pin))
    on_pin = await cg.gpio_pin_expression(config["on_pin"])
    cg.add(var.set_on_pin(on_pin))