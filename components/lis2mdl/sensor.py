import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, UNIT_MICROTESLA

CODEOWNERS = ["@DenshiNingen"]
DEPENDENCIES = ["i2c"]

lis2mdl_ns = cg.esphome_ns.namespace("lis2mdl")
LIS2MDLComponent = lis2mdl_ns.class_("LIS2MDLComponent", cg.PollingComponent, i2c.I2CDevice)

CONF_FIELD_STRENGTH_X = "field_strength_x"
CONF_FIELD_STRENGTH_Y = "field_strength_y"
CONF_FIELD_STRENGTH_Z = "field_strength_z"

_sensor_schema = sensor.sensor_schema(
    unit_of_measurement=UNIT_MICROTESLA,
    accuracy_decimals=1,
    state_class=STATE_CLASS_MEASUREMENT,
    icon="mdi:magnet",
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LIS2MDLComponent),
            cv.Optional(CONF_FIELD_STRENGTH_X): _sensor_schema,
            cv.Optional(CONF_FIELD_STRENGTH_Y): _sensor_schema,
            cv.Optional(CONF_FIELD_STRENGTH_Z): _sensor_schema,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x1E))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_FIELD_STRENGTH_X in config:
        sens = await sensor.new_sensor(config[CONF_FIELD_STRENGTH_X])
        cg.add(var.set_x_sensor(sens))

    if CONF_FIELD_STRENGTH_Y in config:
        sens = await sensor.new_sensor(config[CONF_FIELD_STRENGTH_Y])
        cg.add(var.set_y_sensor(sens))

    if CONF_FIELD_STRENGTH_Z in config:
        sens = await sensor.new_sensor(config[CONF_FIELD_STRENGTH_Z])
        cg.add(var.set_z_sensor(sens))
