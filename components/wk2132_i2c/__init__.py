import esphome.codegen as cg
from esphome.components import i2c, weikai
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome import pins

CODEOWNERS = ["@DrCoolZic"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["weikai", "weikai_i2c"]
MULTI_CONF = True

CONF_POWER_PIN = "power_pin"
CONF_ENABLE_ON_BOOT = "enable_on_boot"

weikai_i2c_ns = cg.esphome_ns.namespace("weikai_i2c")
WeikaiComponentI2C = weikai_i2c_ns.class_(
    "WeikaiComponentI2C", weikai.WeikaiComponent, i2c.I2CDevice
)

CONFIG_SCHEMA = cv.All(
    weikai.WKBASE_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(WeikaiComponentI2C),
            cv.Optional(CONF_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_ENABLE_ON_BOOT, default=True): cv.boolean,
        }
    ).extend(i2c.i2c_device_schema(0x2C)),
    weikai.check_channel_max_2,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    cg.add(var.set_name(str(config[CONF_ID])))
    await weikai.register_weikai(var, config)
    await i2c.register_i2c_device(var, config)
    if CONF_POWER_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_POWER_PIN])
        cg.add(var.set_power_pin(pin))
    cg.add(var.set_enable_on_boot(config[CONF_ENABLE_ON_BOOT]))
