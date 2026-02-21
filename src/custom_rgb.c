#include "../config/layers.h"
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/position_state_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/rgb_underglow.h>

#define COLOR_NUMPAD 150
#define COLOR_CAPS 12

void change_hue_only(uint16_t new_hue) {
  struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
  color.h = new_hue;
  zmk_rgb_underglow_set_hsb(color);
}

static uint16_t base_hue = 0;
static bool numpad_active = false;
static bool caps_active = false;
static struct k_work_delayable capture_work;
static struct k_work_delayable init_work;

void capture_base_hue(void) {
  struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
  base_hue = color.h;
}

void update_base_hue_if_idle(void) {
  bool rgb_on = false;

  if (!caps_active && !numpad_active && zmk_rgb_underglow_get_state(&rgb_on) == 0 && rgb_on) {
    capture_base_hue();
  }
}

void capture_work_handler(struct k_work *work) {
  ARG_UNUSED(work);
  update_base_hue_if_idle();
}

void init_work_handler(struct k_work *work) {
  ARG_UNUSED(work);
  update_base_hue_if_idle();
}

int rgb_per_layer(const zmk_event_t *eh) {
  const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);

  if (ev && ev->layer == LAYER_Numpad) {
    numpad_active = ev->state;

    if (numpad_active) {
      if (!caps_active) {
        capture_base_hue();
        change_hue_only(COLOR_NUMPAD);
      }
    } else {
      if (caps_active) {
        change_hue_only(COLOR_CAPS);
      } else {
        change_hue_only(base_hue);
      }
    }
  }
  return ZMK_EV_EVENT_BUBBLE;
}

int caps_lock_listener(const zmk_event_t *eh) {
  if (as_zmk_hid_indicators_changed(eh)) {
    uint8_t indicators = zmk_hid_indicators_get_current_profile();
    bool new_caps_active = (indicators & BIT(1)) != 0;

    if (new_caps_active && !caps_active) {
      capture_base_hue();
      change_hue_only(COLOR_CAPS);
    } else if (!new_caps_active && caps_active) {
      if (numpad_active) {
        change_hue_only(COLOR_NUMPAD);
      } else {
        change_hue_only(base_hue);
      }
    }

    caps_active = new_caps_active;
  }
  return ZMK_EV_EVENT_BUBBLE;
}

int rgb_key_listener_cb(const zmk_event_t *eh) {
  const struct zmk_position_state_changed *ev = as_zmk_position_state_changed(eh);

  if (ev && ev->state && !caps_active && !numpad_active) {
    k_work_schedule(&capture_work, K_MSEC(30));
  }
  return ZMK_EV_EVENT_BUBBLE;
}

static int custom_rgb_init(const struct device *dev) {
  ARG_UNUSED(dev);

  k_work_init_delayable(&capture_work, capture_work_handler);
  k_work_init_delayable(&init_work, init_work_handler);
  k_work_schedule(&init_work, K_MSEC(500));
  return 0;
}

ZMK_LISTENER(rgb_caps_listener, caps_lock_listener);
ZMK_LISTENER(rgb_layer_listener, rgb_per_layer);
ZMK_LISTENER(rgb_key_listener, rgb_key_listener_cb);
ZMK_SUBSCRIPTION(rgb_caps_listener, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(rgb_layer_listener, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(rgb_key_listener, zmk_position_state_changed);
SYS_INIT(custom_rgb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
