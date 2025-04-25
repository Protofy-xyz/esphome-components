import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ENTITY_CATEGORY_NONE
from . import MKS42DComponent

DEPENDENCIES = ['mks42d']

CONF_STEP_STATE = 'step_state'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_ID): cv.use_id(MKS42DComponent),
    cv.Required(CONF_STEP_STATE): text_sensor.text_sensor_schema(
        # icon="mdi:run",
        entity_category=ENTITY_CATEGORY_NONE,
    ),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    mks = await cg.get_variable(config[CONF_ID])
    sens = await text_sensor.new_text_sensor(config[CONF_STEP_STATE])
    cg.add(mks.set_step_state_text_sensor(sens))