import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_NONE
from . import MKS42DComponent

DEPENDENCIES = ['mks42d']

CONF_STEP_STATE = 'step_state'
CONF_HOME_STATE = 'home_state'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.use_id(MKS42DComponent),
    cv.Optional(CONF_STEP_STATE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_NONE,
    ),
    cv.Optional(CONF_HOME_STATE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_NONE,
    ),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    mks = await cg.get_variable(config[CONF_ID])
    if CONF_STEP_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STEP_STATE])
        cg.add(mks.set_step_state_text_sensor(sens))
    if CONF_HOME_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HOME_STATE])
        cg.add(mks.set_home_state_text_sensor(sens))
