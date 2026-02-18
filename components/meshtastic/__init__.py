import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.components import uart
from esphome.const import CONF_ID, CONF_TRIGGER_ID, CONF_CHANNEL, CONF_MESSAGE
import base64

DEPENDENCIES = ["uart"]
CODEOWNERS = ["@ton"]

CONF_POWER_PIN = "power_pin"
CONF_BOOT_TIMEOUT = "boot_timeout"
CONF_ACK_TIMEOUT = "ack_timeout"
CONF_DESTINATION = "destination"
CONF_ENABLE_ON_BOOT = "enable_on_boot"
CONF_ON_READY = "on_ready"
CONF_ON_MESSAGE = "on_message"
CONF_ON_SEND_SUCCESS = "on_send_success"
CONF_ON_SEND_FAILED = "on_send_failed"
CONF_CONFIGURE = "configure"

meshtastic_ns = cg.esphome_ns.namespace("meshtastic")
MeshtasticComponent = meshtastic_ns.class_(
    "MeshtasticComponent", cg.Component, uart.UARTDevice
)

# Triggers
ReadyTrigger = meshtastic_ns.class_("ReadyTrigger", automation.Trigger.template())
MessageTrigger = meshtastic_ns.class_(
    "MessageTrigger", automation.Trigger.template(cg.std_string)
)
SendSuccessTrigger = meshtastic_ns.class_(
    "SendSuccessTrigger", automation.Trigger.template()
)
SendFailedTrigger = meshtastic_ns.class_(
    "SendFailedTrigger", automation.Trigger.template()
)

# Actions
SendTextAction = meshtastic_ns.class_("SendTextAction", automation.Action)
SendNodeInfoAction = meshtastic_ns.class_("SendNodeInfoAction", automation.Action)
PowerOnAction = meshtastic_ns.class_("PowerOnAction", automation.Action)
PowerOffAction = meshtastic_ns.class_("PowerOffAction", automation.Action)
ApplyConfigAction = meshtastic_ns.class_("ApplyConfigAction", automation.Action)
DumpRadioConfigAction = meshtastic_ns.class_("DumpRadioConfigAction", automation.Action)

# --- Protobuf encoding helpers (used at codegen time) ---

def _encode_varint(value):
    result = bytearray()
    while value > 0x7F:
        result.append((value & 0x7F) | 0x80)
        value >>= 7
    result.append(value & 0x7F)
    return bytes(result)

def _encode_field_varint(field_num, value):
    return _encode_varint((field_num << 3) | 0) + _encode_varint(value)

def _encode_field_bytes(field_num, data):
    return _encode_varint((field_num << 3) | 2) + _encode_varint(len(data)) + data

def _encode_field_bool(field_num, value):
    return _encode_field_varint(field_num, 1 if value else 0)

def _encode_field_string(field_num, value):
    return _encode_field_bytes(field_num, value.encode("utf-8"))

# LoRa region enum
LORA_REGIONS = {
    "UNSET": 0, "US": 1, "EU_433": 2, "EU_868": 3, "CN": 4,
    "JP": 5, "ANZ": 6, "KR": 7, "TW": 8, "RU": 9,
    "IN": 10, "NZ_865": 11, "TH": 12, "LORA_24": 13,
    "UA_433": 14, "UA_868": 15, "MY_433": 16, "MY_919": 17,
    "SG_923": 18,
}

# Serial baud enum
SERIAL_BAUDS = {
    "BAUD_DEFAULT": 0, "BAUD_110": 1, "BAUD_300": 2, "BAUD_600": 3,
    "BAUD_1200": 4, "BAUD_2400": 5, "BAUD_4800": 6, "BAUD_9600": 7,
    "BAUD_19200": 8, "BAUD_38400": 9, "BAUD_57600": 10,
    "BAUD_115200": 11, "BAUD_230400": 12, "BAUD_460800": 13,
    "BAUD_576000": 14, "BAUD_921600": 15,
}

# Serial mode enum
SERIAL_MODES = {
    "DEFAULT": 0, "SIMPLE": 1, "PROTO": 2, "TEXTMSG": 3,
    "NMEA": 4, "CALTOPO": 5,
}

# Channel role enum
CHANNEL_ROLES = {
    "DISABLED": 0, "PRIMARY": 1, "SECONDARY": 2,
}


def _build_config_bluetooth(cfg):
    """Build Config.BluetoothConfig protobuf bytes."""
    payload = b""
    if "enabled" in cfg:
        payload += _encode_field_bool(1, cfg["enabled"])
    if "mode" in cfg:
        modes = {"RANDOM_PIN": 0, "FIXED_PIN": 1, "NO_PIN": 2}
        payload += _encode_field_varint(2, modes.get(cfg["mode"], 0))
    if "fixed_pin" in cfg:
        payload += _encode_field_varint(3, cfg["fixed_pin"])
    return payload


def _build_config_lora(cfg):
    """Build Config.LoRaConfig protobuf bytes."""
    payload = b""
    if "use_preset" in cfg:
        payload += _encode_field_bool(1, cfg["use_preset"])
    if "region" in cfg:
        payload += _encode_field_varint(7, LORA_REGIONS.get(cfg["region"], 0))
    if "hop_limit" in cfg:
        payload += _encode_field_varint(8, cfg["hop_limit"])
    if "tx_enabled" in cfg:
        payload += _encode_field_bool(9, cfg["tx_enabled"])
    if "tx_power" in cfg:
        payload += _encode_field_varint(10, cfg["tx_power"])
    if "ignore_mqtt" in cfg:
        payload += _encode_field_bool(104, cfg["ignore_mqtt"])
    if "config_ok_to_mqtt" in cfg:
        payload += _encode_field_bool(105, cfg["config_ok_to_mqtt"])
    return payload


def _build_config_network(cfg):
    """Build Config.NetworkConfig protobuf bytes."""
    payload = b""
    if "wifi_enabled" in cfg:
        payload += _encode_field_bool(1, cfg["wifi_enabled"])
    if "wifi_ssid" in cfg:
        payload += _encode_field_string(3, cfg["wifi_ssid"])
    if "wifi_psk" in cfg:
        payload += _encode_field_string(4, cfg["wifi_psk"])
    if "eth_enabled" in cfg:
        payload += _encode_field_bool(6, cfg["eth_enabled"])
    return payload


def _build_module_mqtt(cfg):
    """Build ModuleConfig.MQTTConfig protobuf bytes."""
    payload = b""
    if "enabled" in cfg:
        payload += _encode_field_bool(1, cfg["enabled"])
    if "address" in cfg:
        payload += _encode_field_string(2, cfg["address"])
    if "username" in cfg:
        payload += _encode_field_string(3, cfg["username"])
    if "password" in cfg:
        payload += _encode_field_string(4, cfg["password"])
    if "encryption_enabled" in cfg:
        payload += _encode_field_bool(5, cfg["encryption_enabled"])
    if "root" in cfg:
        payload += _encode_field_string(8, cfg["root"])
    return payload


def _build_module_serial(cfg):
    """Build ModuleConfig.SerialConfig protobuf bytes."""
    payload = b""
    if "enabled" in cfg:
        payload += _encode_field_bool(1, cfg["enabled"])
    if "echo" in cfg:
        payload += _encode_field_bool(2, cfg["echo"])
    if "rxd" in cfg:
        payload += _encode_field_varint(3, cfg["rxd"])
    if "txd" in cfg:
        payload += _encode_field_varint(4, cfg["txd"])
    if "baud" in cfg:
        payload += _encode_field_varint(5, SERIAL_BAUDS.get(cfg["baud"], 0))
    if "mode" in cfg:
        payload += _encode_field_varint(7, SERIAL_MODES.get(cfg["mode"], 0))
    return payload


def _build_channel(cfg):
    """Build Channel protobuf bytes."""
    # ChannelSettings sub-message
    settings = b""
    if "psk" in cfg:
        psk_str = cfg["psk"]
        if psk_str.startswith("base64:"):
            psk_bytes = base64.b64decode(psk_str[7:])
        else:
            psk_bytes = psk_str.encode("utf-8")
        settings += _encode_field_bytes(2, psk_bytes)
    if "name" in cfg:
        settings += _encode_field_string(3, cfg["name"])
    if "uplink_enabled" in cfg:
        settings += _encode_field_bool(5, cfg["uplink_enabled"])
    if "downlink_enabled" in cfg:
        settings += _encode_field_bool(6, cfg["downlink_enabled"])

    channel = b""
    index = cfg.get("index", 0)
    channel += _encode_field_varint(1, index)
    channel += _encode_field_bytes(2, settings)
    role = CHANNEL_ROLES.get(cfg.get("role", "PRIMARY"), 1)
    channel += _encode_field_varint(3, role)
    return channel


def _wrap_admin_set_config(config_field_num, config_payload):
    """Wrap a config sub-message into AdminMessage.set_config (field 34)."""
    config_msg = _encode_field_bytes(config_field_num, config_payload)
    return _encode_field_bytes(34, config_msg)


def _wrap_admin_set_module(module_field_num, module_payload):
    """Wrap a module config sub-message into AdminMessage.set_module_config (field 35)."""
    module_msg = _encode_field_bytes(module_field_num, module_payload)
    return _encode_field_bytes(35, module_msg)


def _wrap_admin_set_channel(channel_payload):
    """Wrap a channel into AdminMessage.set_channel (field 33)."""
    return _encode_field_bytes(33, channel_payload)


def _wrap_admin_begin_edit():
    """AdminMessage: begin_edit_settings (field 64) = true."""
    return _encode_field_bool(64, True)


def _wrap_admin_commit_edit():
    """AdminMessage: commit_edit_settings (field 65) = true."""
    return _encode_field_bool(65, True)


def _wrap_admin_reboot(seconds=2):
    """AdminMessage: reboot_seconds (field 97) = seconds."""
    return _encode_field_varint(97, seconds)


# --- YAML schema for configure: block ---

CONFIGURE_BLUETOOTH_SCHEMA = cv.Schema({
    cv.Optional("enabled"): cv.boolean,
    cv.Optional("mode"): cv.one_of("RANDOM_PIN", "FIXED_PIN", "NO_PIN", upper=True),
    cv.Optional("fixed_pin"): cv.uint32_t,
})

CONFIGURE_LORA_SCHEMA = cv.Schema({
    cv.Optional("region"): cv.one_of(*LORA_REGIONS.keys(), upper=True),
    cv.Optional("use_preset"): cv.boolean,
    cv.Optional("tx_enabled"): cv.boolean,
    cv.Optional("tx_power"): cv.int_range(min=0, max=30),
    cv.Optional("hop_limit"): cv.int_range(min=1, max=7),
    cv.Optional("ignore_mqtt"): cv.boolean,
    cv.Optional("config_ok_to_mqtt"): cv.boolean,
})

CONFIGURE_NETWORK_SCHEMA = cv.Schema({
    cv.Optional("wifi_enabled"): cv.boolean,
    cv.Optional("wifi_ssid"): cv.string,
    cv.Optional("wifi_psk"): cv.string,
    cv.Optional("eth_enabled"): cv.boolean,
})

CONFIGURE_MQTT_SCHEMA = cv.Schema({
    cv.Optional("enabled"): cv.boolean,
    cv.Optional("address"): cv.string,
    cv.Optional("username"): cv.string,
    cv.Optional("password"): cv.string,
    cv.Optional("encryption_enabled"): cv.boolean,
    cv.Optional("root"): cv.string,
})

CONFIGURE_SERIAL_SCHEMA = cv.Schema({
    cv.Optional("enabled"): cv.boolean,
    cv.Optional("echo"): cv.boolean,
    cv.Optional("rxd"): cv.uint32_t,
    cv.Optional("txd"): cv.uint32_t,
    cv.Optional("baud"): cv.one_of(*SERIAL_BAUDS.keys(), upper=True),
    cv.Optional("mode"): cv.one_of(*SERIAL_MODES.keys(), upper=True),
})

CONFIGURE_CHANNEL_SCHEMA = cv.Schema({
    cv.Optional("index", default=0): cv.int_range(min=0, max=7),
    cv.Optional("role", default="PRIMARY"): cv.one_of(*CHANNEL_ROLES.keys(), upper=True),
    cv.Optional("name"): cv.string,
    cv.Optional("psk"): cv.string,
    cv.Optional("uplink_enabled"): cv.boolean,
    cv.Optional("downlink_enabled"): cv.boolean,
})

CONFIGURE_SCHEMA = cv.Schema({
    cv.Optional("apply_on_boot", default=True): cv.boolean,
    cv.Optional("bluetooth"): CONFIGURE_BLUETOOTH_SCHEMA,
    cv.Optional("network"): CONFIGURE_NETWORK_SCHEMA,
    cv.Optional("lora"): CONFIGURE_LORA_SCHEMA,
    cv.Optional("mqtt"): CONFIGURE_MQTT_SCHEMA,
    cv.Optional("serial"): CONFIGURE_SERIAL_SCHEMA,
    cv.Optional("channel"): CONFIGURE_CHANNEL_SCHEMA,
})


CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(MeshtasticComponent),
            cv.Optional(CONF_POWER_PIN): pins.gpio_output_pin_schema,
            cv.Optional(
                CONF_BOOT_TIMEOUT, default="30s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(
                CONF_ACK_TIMEOUT, default="30s"
            ): cv.positive_time_period_milliseconds,
            cv.Optional(CONF_DESTINATION, default=0xFFFFFFFF): cv.hex_uint32_t,
            cv.Optional(CONF_CHANNEL, default=0): cv.uint8_t,
            cv.Optional(CONF_ENABLE_ON_BOOT, default=True): cv.boolean,
            cv.Optional(CONF_CONFIGURE): CONFIGURE_SCHEMA,
            cv.Optional(CONF_ON_READY): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(ReadyTrigger)}
            ),
            cv.Optional(CONF_ON_MESSAGE): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(MessageTrigger)}
            ),
            cv.Optional(CONF_ON_SEND_SUCCESS): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SendSuccessTrigger)}
            ),
            cv.Optional(CONF_ON_SEND_FAILED): automation.validate_automation(
                {cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(SendFailedTrigger)}
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_POWER_PIN in config:
        pin = await cg.gpio_pin_expression(config[CONF_POWER_PIN])
        cg.add(var.set_power_pin(pin))

    cg.add(var.set_boot_timeout(config[CONF_BOOT_TIMEOUT]))
    cg.add(var.set_ack_timeout(config[CONF_ACK_TIMEOUT]))
    cg.add(var.set_default_destination(config[CONF_DESTINATION]))
    cg.add(var.set_default_channel(config[CONF_CHANNEL]))
    cg.add(var.set_enable_on_boot(config[CONF_ENABLE_ON_BOOT]))

    # Build configure admin messages at compile time
    if CONF_CONFIGURE in config:
        conf = config[CONF_CONFIGURE]
        admin_msgs = []

        # begin_edit_settings
        admin_msgs.append(_wrap_admin_begin_edit())

        # Config.NetworkConfig (Config field 4)
        if "network" in conf:
            net = _build_config_network(conf["network"])
            admin_msgs.append(_wrap_admin_set_config(4, net))

        # Config.LoRaConfig (Config field 6)
        if "lora" in conf:
            lora = _build_config_lora(conf["lora"])
            admin_msgs.append(_wrap_admin_set_config(6, lora))

        # Config.BluetoothConfig (Config field 7)
        if "bluetooth" in conf:
            bt = _build_config_bluetooth(conf["bluetooth"])
            admin_msgs.append(_wrap_admin_set_config(7, bt))

        # ModuleConfig.MQTTConfig (ModuleConfig field 1)
        if "mqtt" in conf:
            mqtt_cfg = _build_module_mqtt(conf["mqtt"])
            admin_msgs.append(_wrap_admin_set_module(1, mqtt_cfg))

        # ModuleConfig.SerialConfig (ModuleConfig field 2)
        if "serial" in conf:
            serial_cfg = _build_module_serial(conf["serial"])
            admin_msgs.append(_wrap_admin_set_module(2, serial_cfg))

        # commit_edit_settings
        admin_msgs.append(_wrap_admin_commit_edit())

        # Pass settings admin messages as byte arrays
        for i, msg_bytes in enumerate(admin_msgs):
            arr = [f"0x{b:02X}" for b in msg_bytes]
            arr_str = ", ".join(arr)
            cg.add(
                cg.RawExpression(
                    f"{{ static const uint8_t msg_{i}[] = {{{arr_str}}}; "
                    f"{var}->add_config_admin_msg(msg_{i}, sizeof(msg_{i})); }}"
                )
            )

        # Pass apply_on_boot setting
        cg.add(var.set_configure_on_boot(conf.get("apply_on_boot", True)))

        # Channel config (sent after reboot from settings commit)
        if "channel" in conf:
            ch_bytes = _build_channel(conf["channel"])
            ch_admin = _wrap_admin_set_channel(ch_bytes)
            arr = [f"0x{b:02X}" for b in ch_admin]
            arr_str = ", ".join(arr)
            cg.add(
                cg.RawExpression(
                    f"{{ static const uint8_t ch_msg[] = {{{arr_str}}}; "
                    f"{var}->set_channel_admin_msg(ch_msg, sizeof(ch_msg)); }}"
                )
            )

    for conf in config.get(CONF_ON_READY, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_MESSAGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for conf in config.get(CONF_ON_SEND_SUCCESS, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)

    for conf in config.get(CONF_ON_SEND_FAILED, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)


# --- Actions ---

SEND_TEXT_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
        cv.Required(CONF_MESSAGE): cv.templatable(cv.string),
        cv.Optional(CONF_DESTINATION): cv.templatable(cv.hex_uint32_t),
        cv.Optional(CONF_CHANNEL): cv.templatable(cv.uint8_t),
    }
)

POWER_ON_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)

POWER_OFF_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)


@automation.register_action(
    "meshtastic.send_text", SendTextAction, SEND_TEXT_SCHEMA
)
async def send_text_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    tmpl = await cg.templatable(config[CONF_MESSAGE], args, cg.std_string)
    cg.add(var.set_message(tmpl))
    if CONF_DESTINATION in config:
        tmpl = await cg.templatable(config[CONF_DESTINATION], args, cg.uint32)
        cg.add(var.set_destination(tmpl))
    if CONF_CHANNEL in config:
        tmpl = await cg.templatable(config[CONF_CHANNEL], args, cg.uint8)
        cg.add(var.set_channel(tmpl))
    return var


@automation.register_action("meshtastic.power_on", PowerOnAction, POWER_ON_SCHEMA)
async def power_on_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


@automation.register_action("meshtastic.power_off", PowerOffAction, POWER_OFF_SCHEMA)
async def power_off_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


APPLY_CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)


@automation.register_action(
    "meshtastic.apply_config", ApplyConfigAction, APPLY_CONFIG_SCHEMA
)
async def apply_config_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


DUMP_RADIO_CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)


@automation.register_action(
    "meshtastic.dump_radio_config", DumpRadioConfigAction, DUMP_RADIO_CONFIG_SCHEMA
)
async def dump_radio_config_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var


SEND_NODEINFO_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(MeshtasticComponent),
    }
)


@automation.register_action(
    "meshtastic.send_nodeinfo", SendNodeInfoAction, SEND_NODEINFO_SCHEMA
)
async def send_nodeinfo_to_code(config, action_id, template_arg, args):
    parent = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, parent)
    return var
