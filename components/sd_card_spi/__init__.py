import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation
from esphome.const import CONF_ID
from esphome.components.esp32 import add_idf_sdkconfig_option, include_builtin_idf_component, require_fatfs

CONF_ROOT_PATH = "root_path"
CONF_PATH = "path"
CONF_DATA = "data"
CONF_APPEND = "append"
CONF_MAX_BYTES = "max_bytes"
CONF_CS_PIN = "cs_pin"
CONF_SCK_PIN = "sck_pin"
CONF_MISO_PIN = "miso_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_FREQUENCY = "frequency"

sd_card_spi_ns = cg.esphome_ns.namespace("sd_card_spi")
SDCardSPIComponent = sd_card_spi_ns.class_(
    "SDCardSPIComponent", cg.Component
)

WriteFileAction = sd_card_spi_ns.class_("WriteFileAction", automation.Action)
DeleteFileAction = sd_card_spi_ns.class_("DeleteFileAction", automation.Action)
ReadFileAction = sd_card_spi_ns.class_("ReadFileAction", automation.Action)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SDCardSPIComponent),
            cv.Optional(CONF_ROOT_PATH, default="/"): cv.string_strict,
            cv.Required(CONF_CS_PIN): cv.int_,
            cv.Required(CONF_SCK_PIN): cv.int_,
            cv.Required(CONF_MISO_PIN): cv.int_,
            cv.Required(CONF_MOSI_PIN): cv.int_,
            cv.Optional(CONF_FREQUENCY, default=4000000): cv.int_range(min=100000, max=25000000),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_root_path(config[CONF_ROOT_PATH]))
    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    cg.add(var.set_sck_pin(config[CONF_SCK_PIN]))
    cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))
    cg.add(var.set_mosi_pin(config[CONF_MOSI_PIN]))
    cg.add(var.set_frequency(config[CONF_FREQUENCY]))

    require_fatfs()
    include_builtin_idf_component("fatfs")
    include_builtin_idf_component("wear_levelling")

    # Enable Long File Name (LFN) support — allows filenames beyond 8.3 format
    add_idf_sdkconfig_option("CONFIG_FATFS_LFN_NONE", False)
    add_idf_sdkconfig_option("CONFIG_FATFS_LFN_HEAP", True)
    add_idf_sdkconfig_option("CONFIG_FATFS_MAX_LFN", 255)


WRITE_FILE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(SDCardSPIComponent),
        cv.Required(CONF_PATH): cv.templatable(cv.string_strict),
        cv.Required(CONF_DATA): cv.templatable(cv.string_strict),
        cv.Optional(CONF_APPEND, default=False): cv.templatable(cv.boolean),
    }
)


@automation.register_action(
    "sd_card_spi.write_file", WriteFileAction, WRITE_FILE_ACTION_SCHEMA
)
async def write_file_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    path = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    data = await cg.templatable(config[CONF_DATA], args, cg.std_string)
    append = await cg.templatable(config[CONF_APPEND], args, cg.bool_)
    cg.add(var.set_path(path))
    cg.add(var.set_data(data))
    cg.add(var.set_append(append))
    return var


DELETE_FILE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(SDCardSPIComponent),
        cv.Required(CONF_PATH): cv.templatable(cv.string_strict),
    }
)


@automation.register_action(
    "sd_card_spi.delete_file", DeleteFileAction, DELETE_FILE_ACTION_SCHEMA
)
async def delete_file_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    path = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    cg.add(var.set_path(path))
    return var


READ_FILE_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(SDCardSPIComponent),
        cv.Required(CONF_PATH): cv.templatable(cv.string_strict),
        cv.Optional(CONF_MAX_BYTES, default=2048): cv.templatable(cv.int_range(min=1, max=32768)),
    }
)


@automation.register_action(
    "sd_card_spi.read_file", ReadFileAction, READ_FILE_ACTION_SCHEMA
)
async def read_file_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    path = await cg.templatable(config[CONF_PATH], args, cg.std_string)
    max_bytes = await cg.templatable(config[CONF_MAX_BYTES], args, cg.size_t)
    cg.add(var.set_path(path))
    cg.add(var.set_max_bytes(max_bytes))
    return var
