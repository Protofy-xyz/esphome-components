import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c, sensor
from esphome.const import CONF_ID, STATE_CLASS_MEASUREMENT, UNIT_DEGREE_PER_SECOND, UNIT_METER_PER_SECOND_SQUARED

CODEOWNERS = ["@DenshiNingen"]
DEPENDENCIES = ["i2c"]

lsm6ds_ns = cg.esphome_ns.namespace("lsm6ds")
LSM6DSComponent = lsm6ds_ns.class_("LSM6DSComponent", cg.PollingComponent, i2c.I2CDevice)

CONF_ACCELERATION_X = "acceleration_x"
CONF_ACCELERATION_Y = "acceleration_y"
CONF_ACCELERATION_Z = "acceleration_z"
CONF_GYRO_X = "gyro_x"
CONF_GYRO_Y = "gyro_y"
CONF_GYRO_Z = "gyro_z"

accel_schema = sensor.sensor_schema(
    unit_of_measurement=UNIT_METER_PER_SECOND_SQUARED,
    accuracy_decimals=3,
    state_class=STATE_CLASS_MEASUREMENT,
    icon="mdi:axis-arrow",
)

gyro_schema = sensor.sensor_schema(
    unit_of_measurement=UNIT_DEGREE_PER_SECOND,
    accuracy_decimals=2,
    state_class=STATE_CLASS_MEASUREMENT,
    icon="mdi:rotate-360",
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LSM6DSComponent),
            cv.Optional(CONF_ACCELERATION_X): accel_schema,
            cv.Optional(CONF_ACCELERATION_Y): accel_schema,
            cv.Optional(CONF_ACCELERATION_Z): accel_schema,
            cv.Optional(CONF_GYRO_X): gyro_schema,
            cv.Optional(CONF_GYRO_Y): gyro_schema,
            cv.Optional(CONF_GYRO_Z): gyro_schema,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x6A))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    for axis in ("x", "y", "z"):
        accel_key = f"acceleration_{axis}"
        gyro_key = f"gyro_{axis}"
        if accel_key in config:
            sens = await sensor.new_sensor(config[accel_key])
            cg.add(getattr(var, f"set_accel_{axis}_sensor")(sens))
        if gyro_key in config:
            sens = await sensor.new_sensor(config[gyro_key])
            cg.add(getattr(var, f"set_gyro_{axis}_sensor")(sens))
