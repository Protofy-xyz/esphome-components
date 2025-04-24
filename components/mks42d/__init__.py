import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.components import canbus
from esphome import automation

CODEOWNERS = ["@TonProtofy"]

AUTO_LOAD = ["text_sensor"]
DEPENDENCIES = ["canbus"]
MULTI_CONF = True

CONF_CANBUS_ID = "can_bus_id"
CONF_CAN_ID = "can_id"
CONF_DEBUG_RECEIVED_MESSAGES = "debug_received_messages"

mks42d_ns = cg.esphome_ns.namespace("mks42d")
MKS42DComponent = mks42d_ns.class_("MKS42DComponent", cg.Component)
SetTargetPositionAction = mks42d_ns.class_("SetTargetPositionAction", automation.Action)
SendHomeAction = mks42d_ns.class_("SendHomeAction", automation.Action)

CONFIG_SCHEMA = cv.ensure_list(cv.Schema({
    cv.Required(CONF_ID): cv.declare_id(MKS42DComponent),
    cv.Required(CONF_CANBUS_ID): cv.use_id(canbus.CanbusComponent),
    cv.Required(CONF_CAN_ID): cv.uint8_t,
    cv.Optional(CONF_DEBUG_RECEIVED_MESSAGES, default=False): cv.boolean
}).extend(cv.COMPONENT_SCHEMA))

async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)

        cg.add(var.set_can_id(conf[CONF_CAN_ID]))
        cg.add(var.set_canbus_parent(await cg.get_variable(conf[CONF_CANBUS_ID])))
        cg.add(var.set_debug_received_messages(conf[CONF_DEBUG_RECEIVED_MESSAGES]))

@automation.register_action(
        "mks42d.set_target_position", SetTargetPositionAction, 
        cv.Schema({
            cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
            cv.Required("target_position"): cv.int_,
            cv.Required("speed"): cv.int_,
            cv.Required("acceleration"): cv.int_,
        })
)
async def set_target_position_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    target_position = await cg.templatable(config["target_position"], args, cg.int_)
    speed = await cg.templatable(config["speed"], args, cg.int_)
    acceleration = await cg.templatable(config["acceleration"], args, cg.int_)
    cg.add(var.set_target_position(target_position))
    cg.add(var.set_speed(speed))
    cg.add(var.set_acceleration(acceleration))
    return var

@automation.register_action(
    "mks42d.send_home", SendHomeAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent)
    })
)
async def send_home_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
