import re

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import binary_sensor, i2c, sensor
from esphome.const import (
    CONF_ID,
    STATE_CLASS_MEASUREMENT,
    UNIT_DEGREE_PER_SECOND,
    UNIT_METER_PER_SECOND_SQUARED,
)

CODEOWNERS = ["@DenshiNingen"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["binary_sensor"]

lsm6ds_ns = cg.esphome_ns.namespace("lsm6ds")
LSM6DSComponent = lsm6ds_ns.class_("LSM6DSComponent", cg.PollingComponent, i2c.I2CDevice)
AccelRange = lsm6ds_ns.enum("AccelRange")
GyroRange = lsm6ds_ns.enum("GyroRange")

CONF_ACCELERATION_X = "acceleration_x"
CONF_ACCELERATION_Y = "acceleration_y"
CONF_ACCELERATION_Z = "acceleration_z"
CONF_GYRO_X = "gyro_x"
CONF_GYRO_Y = "gyro_y"
CONF_GYRO_Z = "gyro_z"
CONF_ACCEL_RANGE = "accel_range"
CONF_GYRO_RANGE = "gyro_range"
CONF_SHAKE = "shake"
CONF_SHAKE_THRESHOLD = "shake_threshold"
CONF_SHAKE_DURATION = "shake_duration"

ACCEL_RANGES = {
    "2G": AccelRange.ACCEL_RANGE_2G,
    "4G": AccelRange.ACCEL_RANGE_4G,
    "8G": AccelRange.ACCEL_RANGE_8G,
    "16G": AccelRange.ACCEL_RANGE_16G,
}

GYRO_RANGES = {
    "125DPS": GyroRange.GYRO_RANGE_125DPS,
    "250DPS": GyroRange.GYRO_RANGE_250DPS,
    "500DPS": GyroRange.GYRO_RANGE_500DPS,
    "1000DPS": GyroRange.GYRO_RANGE_1000DPS,
    "2000DPS": GyroRange.GYRO_RANGE_2000DPS,
}

G_MS2 = 9.80665
SHAKE_THRESHOLD_RE = re.compile(
    r"^\s*(?P<value>[-+]?(?:\d+(?:\.\d+)?|\.\d+))\s*(?P<unit>m/s\^2|m/s²|m/s\*\*2)\s*$",
    re.IGNORECASE,
)
ACCEL_RANGE_MAX_MS2 = {
    "2G": 2 * G_MS2,
    "4G": 4 * G_MS2,
    "8G": 8 * G_MS2,
    "16G": 16 * G_MS2,
}

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


def _parse_shake_threshold(value):
    if isinstance(value, (int, float)):
        raise cv.Invalid("shake_threshold must include units, e.g. '8 m/s^2'")
    if not isinstance(value, str):
        raise cv.Invalid("shake_threshold must be a string with units, e.g. '8 m/s^2'")

    match = SHAKE_THRESHOLD_RE.match(value)
    if not match:
        raise cv.Invalid("Invalid shake_threshold. Use a value with units, e.g. '8 m/s^2'")

    numeric_value = float(match.group("value"))
    return numeric_value


def _validate_shake(config):
    # shake_threshold is compared against delta acceleration (m/s²); keep it within sensor full-scale.
    threshold = config.get(CONF_SHAKE_THRESHOLD)
    if threshold is None:
        return config
    accel_range_value = config.get(CONF_ACCEL_RANGE, ACCEL_RANGES["2G"])
    # Map enum back to the label we use in docs/schemas.
    accel_range_label = next((label for label, enum_val in ACCEL_RANGES.items() if enum_val == accel_range_value), "2G")
    max_ms2 = ACCEL_RANGE_MAX_MS2.get(accel_range_label, ACCEL_RANGE_MAX_MS2["2G"])

    if threshold <= 0:
        raise cv.Invalid("shake_threshold must be greater than 0 m/s²")
    if threshold > max_ms2:
        raise cv.Invalid(
            f"shake_threshold ({threshold} m/s²) exceeds the accelerometer full-scale (~{max_ms2:.1f} m/s²) for accel_range {accel_range_label}"
        )

    return config


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
            cv.Optional(CONF_ACCEL_RANGE, default="2G"): cv.enum(ACCEL_RANGES, upper=True),
            cv.Optional(CONF_GYRO_RANGE, default="250DPS"): cv.enum(GYRO_RANGES, upper=True),
            cv.Optional(CONF_SHAKE): binary_sensor.binary_sensor_schema(icon="mdi:vibrate"),
            cv.Optional(CONF_SHAKE_THRESHOLD, default="8 m/s^2"): cv.All(
                cv.string_strict, _parse_shake_threshold
            ),
            cv.Optional(CONF_SHAKE_DURATION, default="500ms"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(i2c.i2c_device_schema(0x6A))
)

CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, _validate_shake)


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

    cg.add(var.set_accel_range(config[CONF_ACCEL_RANGE]))
    cg.add(var.set_gyro_range(config[CONF_GYRO_RANGE]))
    if CONF_SHAKE in config:
        shake = await binary_sensor.new_binary_sensor(config[CONF_SHAKE])
        cg.add(var.set_shake_sensor(shake))
    cg.add(var.set_shake_threshold(config[CONF_SHAKE_THRESHOLD]))
    cg.add(var.set_shake_latch_ms(config[CONF_SHAKE_DURATION].total_milliseconds))
