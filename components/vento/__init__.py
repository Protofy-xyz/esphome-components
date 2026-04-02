"""
Vento ESPHome Component — Device Self-Registration via MQTT Manifest

This component scans the full ESPHome config (CORE.config) at compile time,
generates a JSON manifest of all subsystems (sensors, switches, lights, etc.),
and publishes it via MQTT with retain=true at boot.

The Vento server listens for manifests on {network}/devices/{name}/manifest
and auto-generates the board UI from the subsystem definitions.

Dependencies: mqtt (required for manifest publishing)
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
import json

DEPENDENCIES = ['mqtt']
AUTO_LOAD = []

# Domain → subsystem type mapping (covers all standard ESPHome domains)
DOMAIN_TYPE_MAP = {
    'switch': 'switch',
    'binary_sensor': 'binary_sensor',
    'sensor': 'sensor',
    'light': 'light',
    'fan': 'fan',
    'cover': 'cover',
    'number': 'number',
    'select': 'select',
    'text_sensor': 'text_sensor',
    'climate': 'climate',
    'lock': 'lock',
    'button': 'button',
}

# Domain → monitors (state endpoints the device publishes to)
DOMAIN_MONITORS = {
    'switch': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'binary_sensor': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'sensor': [{'name': 'state', 'label': 'Value', 'suffix': '/state'}],
    'light': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'text_sensor': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'number': [{'name': 'state', 'label': 'Value', 'suffix': '/state'}],
    'fan': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'cover': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'climate': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'button': [],
    'lock': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
    'select': [{'name': 'state', 'label': 'State', 'suffix': '/state'}],
}

# Domain → actions (command endpoints the device listens on)
DOMAIN_ACTIONS = {
    'switch': [
        {'name': 'on', 'label': 'Turn On', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'ON'}},
        {'name': 'off', 'label': 'Turn Off', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'OFF'}},
    ],
    'light': [
        {'name': 'on', 'label': 'Turn On', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'ON'}},
        {'name': 'off', 'label': 'Turn Off', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'OFF'}},
    ],
    'button': [
        {'name': 'press', 'label': 'Press', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'PRESS'}},
    ],
    'fan': [
        {'name': 'on', 'label': 'Turn On', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'ON'}},
        {'name': 'off', 'label': 'Turn Off', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'OFF'}},
    ],
    'cover': [
        {'name': 'open', 'label': 'Open', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'OPEN'}},
        {'name': 'close', 'label': 'Close', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'CLOSE'}},
        {'name': 'stop', 'label': 'Stop', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'STOP'}},
    ],
    'number': [
        {'name': 'set', 'label': 'Set Value', 'suffix': '/command', 'payload': {'type': 'str', 'value': ''}},
    ],
    'lock': [
        {'name': 'lock', 'label': 'Lock', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'LOCK'}},
        {'name': 'unlock', 'label': 'Unlock', 'suffix': '/command', 'payload': {'type': 'str', 'value': 'UNLOCK'}},
    ],
    'select': [
        {'name': 'set', 'label': 'Set Option', 'suffix': '/command', 'payload': {'type': 'str', 'value': ''}},
    ],
}

# Keys that are not sub-component entries (platform config, automations, etc.)
_NON_COMPONENT_KEYS = {
    'platform', 'id', 'name', 'icon', 'internal', 'disabled_by_default',
    'on_value', 'on_state', 'on_press', 'on_release', 'on_click',
    'on_turn_on', 'on_turn_off', 'on_multi_click', 'on_double_click',
    'on_raw_value', 'on_value_range', 'filters', 'update_interval',
    'entity_category', 'device_class', 'state_class', 'unit_of_measurement',
    'accuracy_decimals', 'expire_after', 'force_update', 'web_server',
}

vento_ns = cg.esphome_ns.namespace('vento')
VentoComponent = vento_ns.class_('VentoComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VentoComponent),
}).extend(cv.COMPONENT_SCHEMA)


def _to_mqtt_id(name_or_id):
    """Convert a component name/id to the MQTT object_id format.

    ESPHome uses the 'id' (with underscores) for MQTT topics, not the 'name'
    (which can have spaces). If given a name with spaces, convert to underscores.
    """
    return str(name_or_id).replace(' ', '_').lower()


def _extract_entries(domain, entries):
    """Extract individual named components from a domain's config entries.

    Returns (mqtt_id, display_name, domain) tuples where:
    - mqtt_id: the ID used in MQTT topics (underscores, lowercase)
    - display_name: human-readable name for labels
    - domain: the ESPHome domain (text_sensor, sensor, switch, etc.)

    Handles two patterns:
    1. Standard: entry has 'name'/'id' at top level → one component per entry
    2. Platform sub-entries: entry has sub-keys that are dicts with 'name'/'id'
       (e.g., text_sensor with platform: mks42d has step_state:, home_state:, etc.)
    """
    results = []
    for entry in entries:
        if not isinstance(entry, dict):
            continue

        # Standard case: entry has an id or name directly
        top_id = entry.get('id')
        top_name = entry.get('name')
        if top_name or top_id:
            mqtt_id = _to_mqtt_id(top_id or top_name)
            display = str(top_name or top_id)
            results.append((mqtt_id, display, domain))
            continue

        # Platform sub-entry case: scan keys for nested dicts with name/id
        found_any = False
        for key, val in entry.items():
            if key in _NON_COMPONENT_KEYS:
                continue
            if isinstance(val, dict) and (val.get('name') or val.get('id')):
                sub_id = val.get('id')
                sub_name = val.get('name')
                mqtt_id = _to_mqtt_id(sub_id or sub_name)
                display = str(sub_name or sub_id)
                results.append((mqtt_id, display, domain))
                found_any = True

        # Fallback: use entry id or domain name
        if not found_any:
            fallback = entry.get('id') or entry.get('platform', domain)
            mqtt_id = _to_mqtt_id(fallback)
            results.append((mqtt_id, str(fallback), domain))

    return results


def _extract_mqtt_actions(full_config):
    """Extract MQTT on_message/on_json_message topics as action subsystems.

    Parses mqtt.on_message and mqtt.on_json_message entries. Each topic that
    matches the device's topic_prefix is converted into an action endpoint.
    Topics are grouped by the path after the topic_prefix.
    """
    mqtt_config = full_config.get('mqtt')
    if not mqtt_config or not isinstance(mqtt_config, dict):
        return []

    topic_prefix = str(mqtt_config.get('topic_prefix', ''))
    if not topic_prefix:
        return []

    actions = []
    seen_topics = set()

    for key in ('on_message', 'on_json_message'):
        msg_entries = mqtt_config.get(key, [])
        if not isinstance(msg_entries, list):
            msg_entries = [msg_entries]

        is_json = key == 'on_json_message'

        for msg in msg_entries:
            if not isinstance(msg, dict):
                continue
            topic = str(msg.get('topic', ''))
            if not topic or not topic.startswith(topic_prefix + '/'):
                continue
            if topic in seen_topics:
                continue
            seen_topics.add(topic)

            # Extract the relative path after topic_prefix
            # e.g., "vento/devices/controler1/motor/motor1/send_home" → "motor/motor1/send_home"
            rel_path = topic[len(topic_prefix) + 1:]
            parts = rel_path.split('/')

            # Derive action name from last path segment
            action_name = parts[-1] if parts else rel_path

            # Derive a subsystem group from the path (e.g., "motor/motor1")
            # Use first two segments as group, or just the first
            if len(parts) >= 2:
                group_name = parts[1]  # e.g., "motor1"
            else:
                group_name = parts[0] if parts else 'mqtt'

            payload_type = 'json' if is_json else 'str'

            actions.append({
                'group': group_name,
                'name': action_name,
                'label': action_name.replace('_', ' ').title(),
                'endpoint': '/' + rel_path,
                'payload_type': payload_type,
            })

    # Group actions by their group name into subsystems
    groups = {}
    for a in actions:
        g = a['group']
        if g not in groups:
            groups[g] = []
        groups[g].append({
            'name': a['name'],
            'label': a['label'],
            'endpoint': a['endpoint'],
            'connectionType': 'mqtt',
            'payload': {'type': a['payload_type'], 'value': ''},
        })

    subsystems = []
    for group_name, group_actions in groups.items():
        subsystems.append({
            'name': group_name,
            'type': 'mqtt_device',
            'monitors': [],
            'actions': group_actions,
        })

    return subsystems


def build_manifest():
    """Build manifest JSON by scanning CORE.config for all known domains."""
    from esphome.core import CORE

    subsystems = []
    full_config = CORE.config

    for domain, type_name in DOMAIN_TYPE_MAP.items():
        if domain not in full_config:
            continue
        entries = full_config[domain]
        if not isinstance(entries, list):
            entries = [entries]

        for mqtt_id, display_name, comp_domain in _extract_entries(domain, entries):
            monitors = []
            for m in DOMAIN_MONITORS.get(comp_domain, []):
                monitors.append({
                    'name': m['name'],
                    'label': m['label'],
                    'endpoint': '/%s/%s%s' % (comp_domain, mqtt_id, m['suffix']),
                    'connectionType': 'mqtt',
                })

            actions = []
            for a in DOMAIN_ACTIONS.get(comp_domain, []):
                action_entry = {
                    'name': a['name'],
                    'label': a['label'],
                    'endpoint': '/%s/%s%s' % (comp_domain, mqtt_id, a['suffix']),
                    'connectionType': 'mqtt',
                }
                if a.get('payload'):
                    action_entry['payload'] = a['payload']
                actions.append(action_entry)

            subsystems.append({
                'name': display_name,
                'type': type_name,
                'monitors': monitors,
                'actions': actions,
            })

    # B: Extract MQTT on_message/on_json_message as action subsystems
    mqtt_subsystems = _extract_mqtt_actions(full_config)
    subsystems.extend(mqtt_subsystems)

    # Device status: ESPHome publishes online/offline on {topic_prefix}/status
    subsystems.insert(0, {
        'name': 'device',
        'type': 'status',
        'monitors': [{
            'name': 'status',
            'label': 'Connection',
            'endpoint': '/status',
            'connectionType': 'mqtt',
        }],
        'actions': [],
    })

    return json.dumps({'version': 1, 'subsystems': subsystems}, separators=(',', ':'))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    manifest_json = build_manifest()
    cg.add(var.set_manifest(manifest_json))
