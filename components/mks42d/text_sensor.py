import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import ENTITY_CATEGORY_NONE
from . import MKS42DComponent

DEPENDENCIES = ['mks42d']

CONF_MKS42D_ID = "mks42d_id"
CONF_STEP_STATE = 'step_state'
CONF_HOME_STATE = 'home_state'
CONF_IN1_STATE    = 'in1_state'
CONF_IN2_STATE    = 'in2_state'
CONF_OUT1_STATE   = 'out1_state'
CONF_OUT2_STATE   = 'out2_state'
CONF_STALL_STATE  = 'stall_state'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MKS42D_ID): cv.use_id(MKS42DComponent),
    cv.Optional(CONF_STEP_STATE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_NONE,
    ),
    cv.Optional(CONF_HOME_STATE): text_sensor.text_sensor_schema(
        entity_category=ENTITY_CATEGORY_NONE,
    ),
    cv.Optional(CONF_IN1_STATE):    text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_NONE),
    cv.Optional(CONF_IN2_STATE):    text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_NONE),
    cv.Optional(CONF_OUT1_STATE):   text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_NONE),
    cv.Optional(CONF_OUT2_STATE):   text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_NONE),
    cv.Optional(CONF_STALL_STATE):  text_sensor.text_sensor_schema(entity_category=ENTITY_CATEGORY_NONE),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    mks = await cg.get_variable(config[CONF_MKS42D_ID])
    if CONF_STEP_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STEP_STATE])
        cg.add(mks.set_step_state_text_sensor(sens))
    if CONF_HOME_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_HOME_STATE])
        cg.add(mks.set_home_state_text_sensor(sens))
    if CONF_IN1_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_IN1_STATE])
        cg.add(mks.set_in1_state_text_sensor(sens))
    if CONF_IN2_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_IN2_STATE])
        cg.add(mks.set_in2_state_text_sensor(sens))
    if CONF_OUT1_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_OUT1_STATE])
        cg.add(mks.set_out1_state_text_sensor(sens))
    if CONF_OUT2_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_OUT2_STATE])
        cg.add(mks.set_out2_state_text_sensor(sens))
    if CONF_STALL_STATE in config:
        sens = await text_sensor.new_text_sensor(config[CONF_STALL_STATE])
        cg.add(mks.set_stall_state_text_sensor(sens))
