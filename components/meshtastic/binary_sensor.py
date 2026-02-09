import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor
from . import MeshtasticComponent

DEPENDENCIES = ["meshtastic"]

CONFIG_SCHEMA = binary_sensor.binary_sensor_schema(
    device_class="connectivity",
).extend(
    {
        cv.GenerateID("meshtastic_id"): cv.use_id(MeshtasticComponent),
    }
)


async def to_code(config):
    parent = await cg.get_variable(config["meshtastic_id"])
    var = await binary_sensor.new_binary_sensor(config)
    cg.add(parent.set_ready_sensor(var))
