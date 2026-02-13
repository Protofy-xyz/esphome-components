import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@Protofy-xyz"]

lorawan_ns = cg.esphome_ns.namespace("lorawan")
LoRaWANComponent = lorawan_ns.class_(
    "LoRaWANComponent", cg.Component, uart.UARTDevice
)

CONF_CS_PIN = "cs_pin"
CONF_DIO1_PIN = "dio1_pin"
CONF_RESET_PIN = "reset_pin"
CONF_BUSY_PIN = "busy_pin"
CONF_SCK_PIN = "sck_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_MISO_PIN = "miso_pin"
CONF_DEV_ADDR = "dev_addr"
CONF_NWK_S_KEY = "nwk_s_key"
CONF_APP_S_KEY = "app_s_key"
CONF_BAND = "band"
CONF_TX_POWER = "tx_power"
CONF_PORT = "port"
CONF_DIO2_AS_RF_SWITCH = "dio2_as_rf_switch"
CONF_TCXO_VOLTAGE = "tcxo_voltage"

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LoRaWANComponent),
            cv.Required(CONF_CS_PIN): cv.int_,
            cv.Required(CONF_DIO1_PIN): cv.int_,
            cv.Required(CONF_RESET_PIN): cv.int_,
            cv.Required(CONF_BUSY_PIN): cv.int_,
            cv.Required(CONF_SCK_PIN): cv.int_,
            cv.Required(CONF_MOSI_PIN): cv.int_,
            cv.Required(CONF_MISO_PIN): cv.int_,
            cv.Required(CONF_DEV_ADDR): cv.string,
            cv.Required(CONF_NWK_S_KEY): cv.string,
            cv.Required(CONF_APP_S_KEY): cv.string,
            cv.Optional(CONF_BAND, default="EU868"): cv.one_of(
                "EU868", "US915", "AU915", "AS923", "IN865", "KR920", upper=True
            ),
            cv.Optional(CONF_TX_POWER, default=14): cv.int_range(min=2, max=20),
            cv.Optional(CONF_PORT, default=1): cv.int_range(min=1, max=223),
            cv.Optional(CONF_DIO2_AS_RF_SWITCH, default=False): cv.boolean,
            cv.Optional(CONF_TCXO_VOLTAGE, default=0.0): cv.float_,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "lorawan", require_rx=True, require_tx=True
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    cg.add(var.set_dio1_pin(config[CONF_DIO1_PIN]))
    cg.add(var.set_reset_pin(config[CONF_RESET_PIN]))
    cg.add(var.set_busy_pin(config[CONF_BUSY_PIN]))
    cg.add(var.set_sck_pin(config[CONF_SCK_PIN]))
    cg.add(var.set_mosi_pin(config[CONF_MOSI_PIN]))
    cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))

    cg.add(var.set_dev_addr_str(config[CONF_DEV_ADDR]))
    cg.add(var.set_nwk_s_key_str(config[CONF_NWK_S_KEY]))
    cg.add(var.set_app_s_key_str(config[CONF_APP_S_KEY]))

    cg.add(var.set_band(config[CONF_BAND]))
    cg.add(var.set_tx_power(config[CONF_TX_POWER]))
    cg.add(var.set_port(config[CONF_PORT]))
    cg.add(var.set_dio2_as_rf_switch(config[CONF_DIO2_AS_RF_SWITCH]))
    cg.add(var.set_tcxo_voltage(config[CONF_TCXO_VOLTAGE]))

    cg.add_library("jgromes/RadioLib", "7.1.2")
