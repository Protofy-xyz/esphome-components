import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins, automation
from esphome.components import sensor,uart, time as time_component
from esphome.const import CONF_ID, CONF_SENSORS, CONF_TIME_ID, CONF_TRIGGER_ID
from esphome.cpp_generator import MockObj

DEPENDENCIES = ["uart"]
CODEOWNERS = ['@lluis-protofy-xyz','@TonProtofy', '@RogerOrRobert', '@ghofraneZirouh']

bc127_ns = cg.esphome_ns.namespace('bc127')
BC127Component = bc127_ns.class_('BC127Component', cg.Component,uart.UARTDevice,cg.Controller)

CONF_ON_BC127_CONNECTED = "on_connected"
BC127OnConnectedTrigger = bc127_ns.class_('BC127ConnectedTrigger', automation.Trigger.template())

CONF_ON_INCOMING_CALL = "on_incoming_call"
IncomingCallTrigger = bc127_ns.class_('IncomingCallTrigger', automation.Trigger.template())

CONF_ON_ENDED_CALL = "on_ended_call"
EndedCallTrigger = bc127_ns.class_('EndedCallTrigger', automation.Trigger.template())

CONF_START_CALL = "start_call"
StartCallAction  = bc127_ns.class_('StartCallAction', automation.Action.template())


CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(BC127Component),
      
        
        cv.Optional(CONF_ON_BC127_CONNECTED): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(BC127OnConnectedTrigger),
        }),     
        cv.Optional(CONF_ON_INCOMING_CALL): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(IncomingCallTrigger)
        }),
        cv.Optional(CONF_ON_ENDED_CALL): automation.validate_automation({
        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(EndedCallTrigger)
        }),
        cv.Optional(CONF_START_CALL): automation.validate_automation({
            cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(StartCallAction),
            cv.Required("number"): cv.string,
        }),
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
    for conf in config.get(CONF_ON_INCOMING_CALL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "bytes")], conf)
    for conf in config.get(CONF_ON_ENDED_CALL, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "bytes")], conf)

    for conf in config.get(CONF_START_CALL, []):
        action = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        number = conf["number"]
        cg.add(action.set_number(number))
        await automation.build_automation(action, [], conf)

