import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID
import base64
import os

CODEOWNERS = ["@Protofy-xyz"]
DEPENDENCIES = ["wifi"]

setup_portal_ns = cg.esphome_ns.namespace("setup_portal")
SetupPortalComponent = setup_portal_ns.class_("SetupPortalComponent", cg.Component)

CONF_TITLE = "title"
CONF_PIN = "pin"
CONF_FIELDS = "fields"
CONF_LOGO = "logo"
CONF_ACCENT_COLOR = "accent_color"

MIME_TYPES = {
    ".png": "image/png",
    ".jpg": "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif": "image/gif",
    ".svg": "image/svg+xml",
    ".webp": "image/webp",
    ".ico": "image/x-icon",
}


def validate_logo(value):
    """Accept a file path (converted to data URI) or a raw data URI / URL string."""
    value = cv.string(value)
    if not value:
        return value
    # Already a data URI or URL — pass through
    if value.startswith("data:") or value.startswith("http://") or value.startswith("https://"):
        return value
    # Treat as file path
    value = cv.file_(value)
    return str(value)


def validate_field_id(value):
    """NVS keys are limited to 15 characters."""
    value = cv.valid_name(value)
    if len(value) > 15:
        raise cv.Invalid(f"Field ID '{value}' is too long ({len(value)} chars). NVS keys max 15 characters.")
    return value


FIELD_SCHEMA = cv.Schema(
    {
        cv.Required("id"): validate_field_id,
        cv.Required("label"): cv.string,
        cv.Optional("type", default="text"): cv.one_of("text", "number", "password", "interval", lower=True),
        cv.Optional("default", default=""): cv.string,
        cv.Optional("min"): cv.float_,
        cv.Optional("max"): cv.float_,
    }
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(SetupPortalComponent),
            cv.Optional(CONF_TITLE, default="Device Setup"): cv.string,
            cv.Optional(CONF_PIN, default=""): cv.string,
            cv.Optional(CONF_LOGO, default=""): validate_logo,
            cv.Optional(CONF_ACCENT_COLOR, default="#4a90d9"): cv.string,
            cv.Optional(CONF_FIELDS, default=[]): cv.ensure_list(FIELD_SCHEMA),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


def _logo_to_data_uri(logo_value):
    """Convert a logo value to a data URI string for embedding."""
    if not logo_value:
        return ""
    # Already a data URI or URL
    if logo_value.startswith("data:") or logo_value.startswith("http"):
        return logo_value
    # File path — read and convert to base64 data URI
    path = str(logo_value)
    ext = os.path.splitext(path)[1].lower()
    mime = MIME_TYPES.get(ext, "image/png")
    with open(path, "rb") as f:
        data = base64.b64encode(f.read()).decode("ascii")
    return f"data:{mime};base64,{data}"


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_title(config[CONF_TITLE]))

    if config[CONF_PIN]:
        cg.add(var.set_pin(config[CONF_PIN]))

    logo_uri = _logo_to_data_uri(config[CONF_LOGO])
    if logo_uri:
        cg.add(var.set_logo_url(logo_uri))

    cg.add(var.set_accent_color(config[CONF_ACCENT_COLOR]))

    for field in config[CONF_FIELDS]:
        cg.add(
            var.add_field(
                field["id"],
                field["label"],
                field["type"],
                field.get("default", ""),
                float(field.get("min", 0)),
                float(field.get("max", 0)),
                "min" in field,
                "max" in field,
            )
        )
