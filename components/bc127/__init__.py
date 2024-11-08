import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import sensor,uart, time as time_component
from esphome.const import CONF_ID, CONF_SENSORS, CONF_TIME_ID, CONF_TRIGGER_ID
from esphome.cpp_generator import MockObj

CONF_RX = "rx"
CONF_TX = "tx"
CONF_BAUDRATE = "baudrate"
# CONF_JSON_FILE_NAME = "json_file_name"
# CONF_INTERVAL_SECONDS = "interval_seconds"
# CONF_PUBLISH_DATA_WHEN_ONLINE = "publish_data_when_online"
# CONF_PUBLISH_DATA_TOPIC = "publish_data_topic"
DEPENDENCIES = ["uart"]
CODEOWNERS = ['@lluis-protofy-xyz']

bc127_ns = cg.esphome_ns.namespace('bc127')
BC127Component = bc127_ns.class_('BC127Component', cg.Component,uart.UARTDevice,cg.Controller)

CONF_ON_BC127_CONNECTED = "on_connected"
BC127OnConnectedTrigger = bc127_ns.class_('BC127ConnectedTrigger', automation.Trigger.template())

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BC127Component),
        # cv.Required(CONF_RX): pins.gpio_input_pin_schema,
        # cv.Required(CONF_TX): pins.gpio_output_pin_schema,
        # cv.Required(CONF_BAUDRATE): cv.int_range(9600, 115200),
        # cv.Required(CONF_SENSORS): cv.ensure_list(cv.use_id(sensor.Sensor)),
        # cv.Required(CONF_TIME_ID): cv.use_id(time_component.RealTimeClock),
        # cv.Required(CONF_JSON_FILE_NAME): cv.string_strict,
        # cv.Optional(CONF_INTERVAL_SECONDS, default=5): cv.positive_time_period_seconds,
        # cv.Optional(CONF_PUBLISH_DATA_WHEN_ONLINE, default=False): cv.boolean,
        # cv.Optional(CONF_PUBLISH_DATA_TOPIC): cv.string_strict,
        
        cv.Optional(CONF_ON_BC127_CONNECTED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BC127OnConnectedTrigger),
    })
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "bc127", baud_rate=9600, parity="NONE", stop_bits=1, data_bits=8
)

async def to_code(config):
    
    # cg.add_library("plerup/EspSoftwareSerial","8.2.0")
    cg.add_library("bblanchon/ArduinoJson", "6.18.5")
    
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    for conf in config.get(CONF_ON_BC127_CONNECTED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_vector.template(cg.uint8), "bytes")], conf)
    # GLOBAL_BLE_CONTROLLER_VAR = MockObj(bc127_ns.controller, "->")

    # rx = await cg.gpio_pin_expression(config[CONF_RX])
    # cg.add(var.set_rx(rx.get_pin()))
    
    # tx = await cg.gpio_pin_expression(config[CONF_TX])
    # cg.add(var.set_tx(tx.get_pin()))
    
    # cg.add(var.set_baudrate(config[CONF_BAUDRATE]))
    
    # cs_pin = await cg.gpio_pin_expression(config[CONF_CS_PIN])
    # cg.add(var.set_cs_pin(cs_pin.get_pin()))

    # time = await cg.get_variable(config[CONF_TIME_ID])
    # cg.add(var.set_time(time))

    # cg.add(var.set_json_file_name(config[CONF_JSON_FILE_NAME]))
    # cg.add(var.set_interval_seconds(config[CONF_INTERVAL_SECONDS]))

    # cg.add(var.set_publish_data_when_online(config[CONF_PUBLISH_DATA_WHEN_ONLINE]))

    # if CONF_PUBLISH_DATA_TOPIC in config:
    #     cg.add(var.set_publish_data_topic(config[CONF_PUBLISH_DATA_TOPIC]))

    # for sensor_id in config[CONF_SENSORS]:
    #     sens = await cg.get_variable(sensor_id)
    #     cg.add(var.add_sensor(sens))