import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_THROTTLE
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
EnableRotationAction = mks42d_ns.class_("EnableRotationAction", automation.Action)
SendLimitRemapAction = mks42d_ns.class_("SendLimitRemapAction", automation.Action)
QueryStallAction = mks42d_ns.class_("QueryStallAction", automation.Action)
UnstallCommandAction = mks42d_ns.class_("UnstallCommandAction", automation.Action)
SetNoLimitReverseAction = mks42d_ns.class_("SetNoLimitReverseAction", automation.Action)
SetDirectionAction = mks42d_ns.class_("SetDirectionAction", automation.Action)
SetMicrostepAction = mks42d_ns.class_("SetMicrostepAction", automation.Action)
SetHoldCurrentAction = mks42d_ns.class_("SetHoldCurrentAction", automation.Action)
SetWorkingCurrentAction = mks42d_ns.class_("SetWorkingCurrentAction", automation.Action)
SetModeAction = mks42d_ns.class_("SetModeAction", automation.Action)
SetHomeParamsAction = mks42d_ns.class_("SetHomeParamsAction", automation.Action)
QueryIOStatusAction = mks42d_ns.class_("QueryIOStatusAction", automation.Action)


CONFIG_SCHEMA = cv.ensure_list(cv.Schema({
    cv.Required(CONF_ID): cv.declare_id(MKS42DComponent),
    cv.Required(CONF_CANBUS_ID): cv.use_id(canbus.CanbusComponent),
    cv.Required(CONF_CAN_ID): cv.uint8_t,
    cv.Optional(CONF_DEBUG_RECEIVED_MESSAGES, default=False): cv.boolean,
    cv.Optional(CONF_THROTTLE, default="60s"): cv.positive_time_period_milliseconds,

}).extend(cv.COMPONENT_SCHEMA))

async def to_code(config):
    for conf in config:
        var = cg.new_Pvariable(conf[CONF_ID])
        await cg.register_component(var, conf)

        cg.add(var.set_can_id(conf[CONF_CAN_ID]))
        cg.add(var.set_canbus_parent(await cg.get_variable(conf[CONF_CANBUS_ID])))
        cg.add(var.set_debug_received_messages(conf[CONF_DEBUG_RECEIVED_MESSAGES]))
        cg.add(var.set_throttle(conf[CONF_THROTTLE]))

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

@automation.register_action(
    "mks42d.enable_rotation", EnableRotationAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("state"): cv.templatable(cv.string_strict)
    })
)
async def enable_rotation_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    state = await cg.templatable(config["state"], args, cg.std_string)
    cg.add(var.set_state(state))
    return var


@automation.register_action(
    "mks42d.send_limit_remap", SendLimitRemapAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("state"): cv.templatable(cv.string_strict)
    })
)
async def send_limit_remap_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    state = await cg.templatable(config["state"], args, cg.std_string)
    cg.add(var.set_state(state))
    return var


@automation.register_action(
    "mks42d.query_stall", QueryStallAction,
    cv.Schema({cv.Required(CONF_ID): cv.use_id(MKS42DComponent)})
)
async def query_stall_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "mks42d.unstall_command", UnstallCommandAction,
    cv.Schema({cv.Required(CONF_ID): cv.use_id(MKS42DComponent)})
)
async def unstall_command_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var


@automation.register_action(
    "mks42d.set_no_limit_reverse", SetNoLimitReverseAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("degrees"): cv.templatable(cv.int_),
        cv.Required("current_ma"): cv.templatable(cv.int_),
    })
)
async def set_no_limit_reverse_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    degrees = await cg.templatable(config["degrees"], args, cg.int_)
    current_ma = await cg.templatable(config["current_ma"], args, cg.int_)
    cg.add(var.set_degrees(degrees))
    cg.add(var.set_current_ma(current_ma))
    return var


@automation.register_action(
    "mks42d.set_direction", SetDirectionAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("dir"): cv.templatable(cv.string_strict)
    })
)
async def set_direction_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    direction = await cg.templatable(config["dir"], args, cg.std_string)
    cg.add(var.set_direction(direction))
    return var


@automation.register_action(
    "mks42d.set_microstep", SetMicrostepAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("microstep"): cv.templatable(cv.int_)
    })
)
async def set_microstep_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    microstep = await cg.templatable(config["microstep"], args, cg.int_)
    cg.add(var.set_microstep(microstep))
    return var


@automation.register_action(
    "mks42d.set_hold_current", SetHoldCurrentAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("percent"): cv.templatable(cv.int_)
    })
)
async def set_hold_current_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    percent = await cg.templatable(config["percent"], args, cg.int_)
    cg.add(var.set_percent(percent))
    return var


@automation.register_action(
    "mks42d.set_working_current", SetWorkingCurrentAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("ma"): cv.templatable(cv.int_)
    })
)
async def set_working_current_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    ma = await cg.templatable(config["ma"], args, cg.int_)
    cg.add(var.set_ma(ma))
    return var


@automation.register_action(
    "mks42d.set_mode", SetModeAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("mode"): cv.templatable(cv.int_)
    })
)
async def set_mode_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    mode = await cg.templatable(config["mode"], args, cg.int_)
    cg.add(var.set_mode(mode))
    return var


@automation.register_action(
    "mks42d.set_home_params", SetHomeParamsAction,
    cv.Schema({
        cv.Required(CONF_ID): cv.use_id(MKS42DComponent),
        cv.Required("hm_trig_level"): cv.templatable(cv.boolean),
        cv.Required("hm_dir"): cv.templatable(cv.string_strict),
        cv.Required("hm_speed"): cv.templatable(cv.int_),
        cv.Required("end_limit"): cv.templatable(cv.boolean),
        cv.Required("sensorless_homing"): cv.templatable(cv.boolean)
    })
)
async def set_home_params_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    cg.add(var.set_hm_trig_level(await cg.templatable(config["hm_trig_level"], args, cg.bool_)))
    cg.add(var.set_hm_dir(await cg.templatable(config["hm_dir"], args, cg.std_string)))
    cg.add(var.set_hm_speed(await cg.templatable(config["hm_speed"], args, cg.int_)))
    cg.add(var.set_end_limit(await cg.templatable(config["end_limit"], args, cg.bool_)))
    cg.add(var.set_sensorless_homing(await cg.templatable(config["sensorless_homing"], args, cg.bool_)))
    return var


@automation.register_action(
    "mks42d.query_io_status", QueryIOStatusAction,
    cv.Schema({cv.Required(CONF_ID): cv.use_id(MKS42DComponent)})
)
async def query_io_status_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
