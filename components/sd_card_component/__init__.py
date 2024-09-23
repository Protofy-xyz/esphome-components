import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi, sensor
from esphome.const import CONF_ID, CONF_SENSORS

sd_card_ns = cg.esphome_ns.namespace('sd_card_component')
SDCardComponent = sd_card_ns.class_('SDCardComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SDCardComponent),
        cv.Required("cs_pin"): pins.gpio_output_pin_schema,
        cv.Required(CONF_SENSORS): cv.ensure_list(cv.use_id(sensor.Sensor)),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    
    for sensor_id in config[CONF_SENSORS]:
        sens = await cg.get_variable(sensor_id)
        cg.add(var.add_sensor(sens))
