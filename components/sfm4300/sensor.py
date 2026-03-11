import esphome.codegen as cg
from esphome.components import i2c, sensor
import esphome.config_validation as cv
from esphome.const import (
    STATE_CLASS_MEASUREMENT,
    UNIT_EMPTY,
)

CODEOWNERS = ["@Protofy-xyz"]
DEPENDENCIES = ["i2c"]

sfm4300_ns = cg.esphome_ns.namespace("sfm4300")
SFM4300Sensor = sfm4300_ns.class_(
    "SFM4300Sensor", sensor.Sensor, cg.PollingComponent, i2c.I2CDevice
)

GasType = sfm4300_ns.enum("GasType")
GAS_TYPES = {
    "O2": GasType.GAS_O2,
    "Air": GasType.GAS_AIR,
    "N2O": GasType.GAS_N2O,
    "CO2": GasType.GAS_CO2,
}

CONF_GAS_TYPE = "gas_type"

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        SFM4300Sensor,
        unit_of_measurement="slm",
        accuracy_decimals=2,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x2A))
    .extend(
        {
            cv.Optional(CONF_GAS_TYPE, default="O2"): cv.enum(GAS_TYPES, upper=False),
        }
    )
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)
    cg.add(var.set_gas_type(config[CONF_GAS_TYPE]))
