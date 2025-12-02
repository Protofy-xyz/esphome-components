
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.components.sensor as esp_sensor
from esphome.components import i2c, sensor
from esphome.const import (
    CONF_ID,
    CONF_VOLTAGE,
    CONF_CURRENT,
    CONF_UPDATE_INTERVAL,
    UNIT_VOLT,
    UNIT_AMPERE,
    UNIT_PERCENT,
    ICON_BATTERY,
    DEVICE_CLASS_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
)

CODECODEOWNERS = ["@DenshiNingen"]
DEPENDENCIES = ["i2c"]

CONF_SOC = "soc"
CONF_REMAINING_CAPACITY = "remaining_capacity"
CONF_TEMPERATURE = "temperature"
CONF_FULL_CHARGE_CAPACITY = "full_charge_capacity"
CONF_DESIGN_CAPACITY = "design_capacity"
CONF_STATE_OF_HEALTH = "state_of_health"
CONF_DEVICE_NUMBER = "device_number"

bq27220_ns = cg.esphome_ns.namespace('bq27220')
BQ27220Component = bq27220_ns.class_('BQ27220Component', cg.PollingComponent, i2c.I2CDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BQ27220Component),
    cv.Optional(CONF_UPDATE_INTERVAL, default="60s"): cv.update_interval,
    cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
        unit_of_measurement=UNIT_VOLT,
        icon=ICON_BATTERY,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_CURRENT): sensor.sensor_schema(
        unit_of_measurement="mA",
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_CURRENT,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_SOC): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_REMAINING_CAPACITY): sensor.sensor_schema(
        unit_of_measurement="mAh",
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
        unit_of_measurement="°C",
        icon=ICON_BATTERY,
        accuracy_decimals=2,
        device_class=DEVICE_CLASS_TEMPERATURE,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_FULL_CHARGE_CAPACITY): sensor.sensor_schema(
        unit_of_measurement="mAh",
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DESIGN_CAPACITY): sensor.sensor_schema(
        unit_of_measurement="mAh",
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_STATE_OF_HEALTH): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_DEVICE_NUMBER): sensor.sensor_schema(
        unit_of_measurement="",
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT,
    ),
}).extend(i2c.i2c_device_schema(0x55)).extend(cv.polling_component_schema("60s"))

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    if CONF_VOLTAGE in config:
        sens = await esp_sensor.new_sensor(config[CONF_VOLTAGE])
        cg.add(var.set_voltage_sensor(sens))

    if CONF_CURRENT in config:
        sens = await esp_sensor.new_sensor(config[CONF_CURRENT])
        cg.add(var.set_current_sensor(sens))

    if CONF_SOC in config:
        sens = await esp_sensor.new_sensor(config[CONF_SOC])
        cg.add(var.set_soc_sensor(sens))
        
    if CONF_REMAINING_CAPACITY in config:
        sens = await esp_sensor.new_sensor(config[CONF_REMAINING_CAPACITY])
        cg.add(var.set_remaining_capacity_sensor(sens))

    if CONF_TEMPERATURE in config:
        sens = await esp_sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
    
    if CONF_FULL_CHARGE_CAPACITY in config:
        sens = await esp_sensor.new_sensor(config[CONF_FULL_CHARGE_CAPACITY])
        cg.add(var.set_full_charge_capacity_sensor(sens))

    if CONF_DESIGN_CAPACITY in config:
        sens = await esp_sensor.new_sensor(config[CONF_DESIGN_CAPACITY])
        cg.add(var.set_design_capacity_sensor(sens))
        
    if CONF_STATE_OF_HEALTH in config:
        sens = await esp_sensor.new_sensor(config[CONF_STATE_OF_HEALTH])
        cg.add(var.set_state_of_health_sensor(sens))

    if CONF_DEVICE_NUMBER in config:
        sens = await esp_sensor.new_sensor(config[CONF_DEVICE_NUMBER])
        cg.add(var.set_device_number_sensor(sens))
