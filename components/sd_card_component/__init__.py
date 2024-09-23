import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi, sensor, time as time_component
from esphome.const import CONF_ID, CONF_SENSORS, CONF_TIME_ID

sd_card_ns = cg.esphome_ns.namespace('sd_card_component')
SDCardComponent = sd_card_ns.class_('SDCardComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SDCardComponent),
        cv.Required("cs_pin"): pins.gpio_output_pin_schema,
        cv.Required(CONF_SENSORS): cv.ensure_list(cv.use_id(sensor.Sensor)),
        cv.Required(CONF_TIME_ID): cv.use_id(time_component.RealTimeClock),  # Add this line
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    time = await cg.get_variable(config[CONF_TIME_ID])  # Get time component
    cg.add(var.set_time(time))  # Set time component
    
    for sensor_id in config[CONF_SENSORS]:
        sens = await cg.get_variable(sensor_id)
        cg.add(var.add_sensor(sens))
