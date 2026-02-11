import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, i2c
from esphome.const import (
    CONF_ID,
    DEVICE_CLASS_DISTANCE,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

DEPENDENCIES = ["i2c"]

tfmini_plus_ns = cg.esphome_ns.namespace("tfmini_plus")
TFMiniPlusComponent = tfmini_plus_ns.class_(
    "TFMiniPlusComponent", cg.PollingComponent, i2c.I2CDevice
)

CONF_DISTANCE = "distance"
CONF_STRENGTH = "strength"
CONF_TEMPERATURE = "temperature"
CONF_DISTANCE_UNIT = "distance_unit"
CONF_FRAME_RATE = "frame_rate"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(TFMiniPlusComponent),
            cv.Optional(CONF_DISTANCE): sensor.sensor_schema(
                unit_of_measurement="cm",
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_DISTANCE,
                state_class=STATE_CLASS_MEASUREMENT,
                icon="mdi:ruler",
            ),
            cv.Optional(CONF_STRENGTH): sensor.sensor_schema(
                accuracy_decimals=0,
                state_class=STATE_CLASS_MEASUREMENT,
                icon="mdi:signal",
            ),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_DISTANCE_UNIT, default="cm"): cv.one_of(
                "cm", "mm", lower=True
            ),
            cv.Optional(CONF_FRAME_RATE): cv.int_range(min=0, max=1000),
        }
    )
    .extend(cv.polling_component_schema("500ms"))
    .extend(i2c.i2c_device_schema(0x10))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_DISTANCE in config:
        sens = await sensor.new_sensor(config[CONF_DISTANCE])
        cg.add(var.set_distance_sensor(sens))
        cg.add(sens.set_unit_of_measurement(config[CONF_DISTANCE_UNIT]))

    if CONF_STRENGTH in config:
        sens = await sensor.new_sensor(config[CONF_STRENGTH])
        cg.add(var.set_strength_sensor(sens))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))

    if config[CONF_DISTANCE_UNIT] == "mm":
        cg.add(var.set_distance_unit_mm(True))

    if CONF_FRAME_RATE in config:
        cg.add(var.set_frame_rate(config[CONF_FRAME_RATE]))
