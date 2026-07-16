#define DT_DRV_COMPAT zmk_behavior_agent_jump

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

#include <raw_hid/events.h>
#include <zmk/agent_status.h>
#include <zmk/behavior.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

static int agent_jump_binding_pressed(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    static uint8_t msg[3];
    msg[0] = AGENT_PROTO_VERSION;
    msg[1] = AGENT_CMD_JUMP;
    msg[2] = (uint8_t)binding->param1;
    LOG_DBG("agent jump slot %d", binding->param1);
    raise_raw_hid_sent_event((struct raw_hid_sent_event){.data = msg, .length = sizeof(msg)});
    return ZMK_BEHAVIOR_OPAQUE;
}

static int agent_jump_binding_released(struct zmk_behavior_binding *binding,
                                       struct zmk_behavior_binding_event event) {
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api behavior_agent_jump_driver_api = {
    .binding_pressed = agent_jump_binding_pressed,
    .binding_released = agent_jump_binding_released,
};

BEHAVIOR_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                        &behavior_agent_jump_driver_api);

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
