#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <raw_hid/events.h>
#include <zmk/agent_status.h>
#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define AGENT_TIMEOUT_SECONDS 120

// Underglow LED indices beneath F1-F5 on the left half strip
static const uint8_t slot_led_index[AGENT_USABLE_SLOTS] = {34, 28, 22, 16, 10};

static uint8_t slot_colors[AGENT_SLOT_COUNT][3];
static bool agents_layer_active;

static void agent_repaint(void) {
    for (int i = 0; i < AGENT_USABLE_SLOTS; i++) {
        uint8_t r = slot_colors[i][0];
        uint8_t g = slot_colors[i][1];
        uint8_t b = slot_colors[i][2];
        if (r == 0 && g == 0 && b == 0 && agents_layer_active) {
            // dim violet: this slot key is an armed jump target
            r = 40;
            g = 20;
            b = 80;
        }
        zmk_rgb_underglow_set_agent_pixel(slot_led_index[i], r, g, b);
    }
    zmk_rgb_underglow_agent_commit();
}

static void agent_timeout_handler(struct k_work *work) {
    LOG_INF("Agent status: host silent for %ds, clearing overlay", AGENT_TIMEOUT_SECONDS);
    memset(slot_colors, 0, sizeof(slot_colors));
    agent_repaint();
}

static K_WORK_DELAYABLE_DEFINE(agent_timeout_work, agent_timeout_handler);

static void agent_send_hello_reply(void) {
    static uint8_t reply[3] = {AGENT_PROTO_VERSION, AGENT_CMD_HELLO, AGENT_USABLE_SLOTS};
    raise_raw_hid_sent_event((struct raw_hid_sent_event){.data = reply, .length = sizeof(reply)});
}

static int agent_status_hid_listener(const zmk_event_t *eh) {
    struct raw_hid_received_event *ev = as_raw_hid_received_event(eh);
    if (ev == NULL || ev->length < 2 || ev->data[0] != AGENT_PROTO_VERSION) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    switch (ev->data[1]) {
    case AGENT_CMD_SET_LEDS:
        if (ev->length < 2 + AGENT_SLOT_COUNT * 3) {
            return ZMK_EV_EVENT_BUBBLE;
        }
        for (int s = 0; s < AGENT_SLOT_COUNT; s++) {
            slot_colors[s][0] = ev->data[2 + s * 3];
            slot_colors[s][1] = ev->data[3 + s * 3];
            slot_colors[s][2] = ev->data[4 + s * 3];
        }
        agent_repaint();
        break;
    case AGENT_CMD_HELLO:
        agent_send_hello_reply();
        break;
    default:
        return ZMK_EV_EVENT_BUBBLE;
    }

    k_work_reschedule(&agent_timeout_work, K_SECONDS(AGENT_TIMEOUT_SECONDS));
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(agent_status_hid, agent_status_hid_listener);
ZMK_SUBSCRIPTION(agent_status_hid, raw_hid_received_event);

#if CONFIG_ZMK_AGENT_STATUS_LAYER >= 0

static int agent_status_layer_listener(const zmk_event_t *eh) {
    const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
    if (ev == NULL) {
        return ZMK_EV_EVENT_BUBBLE;
    }
    if (ev->layer == CONFIG_ZMK_AGENT_STATUS_LAYER) {
        agents_layer_active = ev->state;
        agent_repaint();
    }
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(agent_status_layer, agent_status_layer_listener);
ZMK_SUBSCRIPTION(agent_status_layer, zmk_layer_state_changed);

#endif /* CONFIG_ZMK_AGENT_STATUS_LAYER >= 0 */
