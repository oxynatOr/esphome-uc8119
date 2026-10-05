import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import i2c
from esphome.const import CONF_ID, CONF_RESET_PIN
from esphome import pins

DEPENDENCIES = ["i2c"]
CODEOWNERS = ["@oxynatOr"]

CONF_BUSY_PIN = "busy_pin"
CONF_ENABLE_PIN = "enable_pin"
CONF_GHOST_CLEAR_INTERVAL = "ghost_clear_interval"
CONF_FULL_UPDATE_EVERY = "full_update_every"

uc8119_ns = cg.esphome_ns.namespace("uc8119")
UC8119 = uc8119_ns.class_("UC8119", cg.Component, i2c.I2CDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(UC8119),
            cv.Required(CONF_RESET_PIN): pins.gpio_output_pin_schema,
            cv.Required(CONF_BUSY_PIN): pins.gpio_input_pin_schema,
            cv.Optional(CONF_ENABLE_PIN): pins.gpio_output_pin_schema,
            cv.Optional(CONF_FULL_UPDATE_EVERY, default=10): cv.uint32_t,
            cv.Optional(CONF_GHOST_CLEAR_INTERVAL, default="30min"): cv.positive_time_period_milliseconds,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x50))
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    reset = await cg.gpio_pin_expression(config[CONF_RESET_PIN])
    cg.add(var.set_reset_pin(reset))

    busy = await cg.gpio_pin_expression(config[CONF_BUSY_PIN])
    cg.add(var.set_busy_pin(busy))

    cg.add(var.set_full_update_every(config[CONF_FULL_UPDATE_EVERY]))
    cg.add(var.set_ghost_clear_interval(config[CONF_GHOST_CLEAR_INTERVAL]))

    if CONF_ENABLE_PIN in config:
        en = await cg.gpio_pin_expression(config[CONF_ENABLE_PIN])
        cg.add(var.set_enable_pin(en))
