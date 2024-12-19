import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import sensor,uart, time as time_component
from esphome.const import CONF_ID, CONF_SENSORS, CONF_TIME_ID, CONF_TRIGGER_ID
from esphome.cpp_generator import MockObj

DEPENDENCIES = ["uart"]

gm77_ns = cg.esphome_ns.namespace('gm77')
GM77Component = gm77_ns.class_('GM77Component', cg.Component,uart.UARTDevice,cg.Controller)

CONF_ON_TAG = "on_tag"
OnTagTrigger = gm77_ns.class_('OnTagTrigger', automation.Trigger.template())

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(GM77Component),

        cv.Optional(CONF_ON_TAG): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(OnTagTrigger)
        })
   
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)
FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "GM77", baud_rate=9600, parity="NONE", stop_bits=1, data_bits=8
)

async def to_code(config):
    
    # cg.add_library("plerup/EspSoftwareSerial","8.2.0")
    cg.add_library("bblanchon/ArduinoJson", "6.18.5")
    
    
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    
    for conf in config.get(CONF_ON_TAG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "bytes")], conf)
    


