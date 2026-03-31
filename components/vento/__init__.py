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

vento_ns = cg.esphome_ns.namespace('vento')
VentoComponent = vento_ns.class_('VentoComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VentoComponent),
}).extend(cv.COMPONENT_SCHEMA)


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

        for entry in entries:
            name = entry.get('name') or entry.get('id', domain)
            comp_name = str(name) if not isinstance(name, str) else name

            monitors = []
            for m in DOMAIN_MONITORS.get(domain, []):
                monitors.append({
                    'name': m['name'],
                    'label': m['label'],
                    'endpoint': '/%s/%s%s' % (domain, comp_name, m['suffix']),
                    'connectionType': 'mqtt',
                })

            actions = []
            for a in DOMAIN_ACTIONS.get(domain, []):
                action_entry = {
                    'name': a['name'],
                    'label': a['label'],
                    'endpoint': '/%s/%s%s' % (domain, comp_name, a['suffix']),
                    'connectionType': 'mqtt',
                }
                if a.get('payload'):
                    action_entry['payload'] = a['payload']
                actions.append(action_entry)

            subsystems.append({
                'name': comp_name,
                'type': type_name,
                'monitors': monitors,
                'actions': actions,
            })

    return json.dumps({'version': 1, 'subsystems': subsystems}, separators=(',', ':'))


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    manifest_json = build_manifest()
    cg.add(var.set_manifest(manifest_json))
