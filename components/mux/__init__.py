import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.const import (
    CONF_ID,
    CONF_UPDATE_INTERVAL,
    CONF_NUMBER,
    CONF_INVERTED,
)

CODEOWNERS = ["@TonProtofy"]

MULTI_CONF = True

CONF_SEL0 = "sel0"
CONF_SEL1 = "sel1"
CONF_SEL2 = "sel2"
CONF_RESULT = "result"

mux_ns = cg.esphome_ns.namespace("mux")
MUXComponent = mux_ns.class_("MUXComponent", cg.Component)
MUXGPIOPin = mux_ns.class_("MUXGPIOPin", cg.GPIOPin)

CONFIG_SCHEMA = cv.ensure_list(
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(MUXComponent),
            cv.Required(CONF_SEL0): pins.gpio_output_pin_schema,
            cv.Required(CONF_SEL1): pins.gpio_output_pin_schema,
            cv.Required(CONF_SEL2): pins.gpio_output_pin_schema,
            cv.Required(CONF_RESULT): pins.gpio_input_pin_schema,
            cv.Optional(CONF_UPDATE_INTERVAL, default="100ms"): cv.update_interval,
        }
    ).extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)

        sel0 = await cg.gpio_pin_expression(conf[CONF_SEL0])
        sel1 = await cg.gpio_pin_expression(conf[CONF_SEL1])
        sel2 = await cg.gpio_pin_expression(conf[CONF_SEL2])
        result = await cg.gpio_pin_expression(conf[CONF_RESULT])

        cg.add(var.set_sel_pins(sel0, sel1, sel2))
        cg.add(var.set_result_pin(result))
        cg.add(var.set_update_interval(conf[CONF_UPDATE_INTERVAL]))



MUX_PIN_SCHEMA = pins.gpio_base_schema(
    MUXGPIOPin,
    cv.int_range(min=0, max=7),
    modes=["input"],
    mode_validator=lambda x: x,
    invertable=True,
).extend({cv.Required("mux"): cv.use_id(MUXComponent)})


@pins.PIN_SCHEMA_REGISTRY.register("mux", MUX_PIN_SCHEMA)
async def mux_pin_to_code(config):
    parent = await cg.get_variable(config["mux"])
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_parent(parent))
    cg.add(var.set_channel(config[CONF_NUMBER]))
    cg.add(var.set_inverted(config[CONF_INVERTED]))
    return var
