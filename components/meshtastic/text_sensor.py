import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from . import MeshtasticComponent

DEPENDENCIES = ["meshtastic"]

CONFIG_SCHEMA = text_sensor.text_sensor_schema().extend(
    {
        cv.GenerateID("meshtastic_id"): cv.use_id(MeshtasticComponent),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config["meshtastic_id"])
    var = await text_sensor.new_text_sensor(config)
    cg.add(parent.set_state_sensor(var))
