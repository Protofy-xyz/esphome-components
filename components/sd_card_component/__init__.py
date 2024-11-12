import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import spi, sensor, time as time_component
from esphome.const import CONF_ID, CONF_SENSORS, CONF_TIME_ID
from esphome.core import CORE

CONF_CS_PIN = "cs_pin"
CONF_JSON_FILE_NAME = "json_file_name"
CONF_INTERVAL_SECONDS = "interval_seconds"
CONF_PUBLISH_DATA_WHEN_ONLINE = "publish_data_when_online"
CONF_PUBLISH_DATA_TOPIC = "publish_data_topic"

sd_card_ns = cg.esphome_ns.namespace('sd_card_component')
SDCardComponent = sd_card_ns.class_('SDCardComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(SDCardComponent),
        cv.Required(CONF_CS_PIN): pins.gpio_output_pin_schema,
        cv.Required(CONF_SENSORS): cv.ensure_list(cv.use_id(sensor.Sensor)),
        cv.Required(CONF_TIME_ID): cv.use_id(time_component.RealTimeClock),
        cv.Required(CONF_JSON_FILE_NAME): cv.string_strict,
        cv.Optional(CONF_INTERVAL_SECONDS, default=5): cv.positive_time_period_seconds,
        cv.Optional(CONF_PUBLISH_DATA_WHEN_ONLINE, default=False): cv.boolean,
        cv.Optional(CONF_PUBLISH_DATA_TOPIC): cv.string_strict,
    }
).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    cg.add(var.set_cs_pin(cs_pin.get_pin()))

    time = await cg.get_variable(config[CONF_TIME_ID])
    cg.add(var.set_time(time))

    cg.add(var.set_json_file_name(config[CONF_JSON_FILE_NAME]))
    cg.add(var.set_interval_seconds(config[CONF_INTERVAL_SECONDS]))

    cg.add(var.set_publish_data_when_online(config[CONF_PUBLISH_DATA_WHEN_ONLINE]))

    if CONF_PUBLISH_DATA_TOPIC in config:
        cg.add(var.set_publish_data_topic(config[CONF_PUBLISH_DATA_TOPIC]))

    for sensor_id in config[CONF_SENSORS]:
        sens = await cg.get_variable(sensor_id)
        cg.add(var.add_sensor(sens))

    if CORE.using_arduino:
        if CORE.is_esp32:
            cg.add_library("FS", None)
            cg.add_library("SD", None)
