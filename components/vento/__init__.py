"""
Vento ESPHome Component — YAML Hash Publisher

Publishes a server-provided YAML hash via MQTT at boot so the Vento server
can detect whether the device firmware matches the current YAML config.

The hash is injected by the server into the YAML before compilation:

  vento:
    yaml_hash: "a1b2c3..."

The server calculates the hash, parses the YAML to generate board cards,
and uses this hash to detect firmware/YAML desync.

Dependencies: mqtt (required for hash publishing)
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ['mqtt']
AUTO_LOAD = []

CONF_YAML_HASH = 'yaml_hash'

vento_ns = cg.esphome_ns.namespace('vento')
VentoComponent = vento_ns.class_('VentoComponent', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(VentoComponent),
    cv.Optional(CONF_YAML_HASH, default=''): cv.string,
}).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    yaml_hash = config.get(CONF_YAML_HASH, '')
    if yaml_hash:
        cg.add(var.set_yaml_hash(yaml_hash))
