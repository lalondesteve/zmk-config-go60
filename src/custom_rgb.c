#include "../config/layers.h"
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/hid_indicators.h>
#include <zmk/rgb_underglow.h>

#define COLOR_BASE_HUE 216
#define COLOR_NUMPAD 150
#define COLOR_CAPS 12

void change_hue_only(uint16_t new_hue) {
  struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
  color.h = new_hue;
  zmk_rgb_underglow_set_hsb(color);
}

static uint16_t previous_hue = 0;

void set_previous_hue(void) {
  struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
  previous_hue = color.h;
}

int rgb_per_layer(const zmk_event_t *eh) {
  const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);

  if (ev) {
    if (ev->layer == LAYER_Numpad) {
      if (ev->state) {
        set_previous_hue();
        change_hue_only(COLOR_NUMPAD);
      } else {
        change_hue_only(previous_hue);
      }
    }
  }
  return ZMK_EV_EVENT_BUBBLE;
}

int caps_lock_listener(const zmk_event_t *eh) {
  if (as_zmk_hid_indicators_changed(eh)) {
    uint8_t indicators = zmk_hid_indicators_get_current_profile();
    bool caps_active = (indicators & BIT(1)) != 0;

    if (caps_active) {
      set_previous_hue();
      change_hue_only(COLOR_CAPS);
    } else {
      change_hue_only(previous_hue);
    }
  }
  return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(rgb_caps_listener, caps_lock_listener);
ZMK_LISTENER(rgb_layer_listener, rgb_per_layer);
ZMK_SUBSCRIPTION(rgb_caps_listener, zmk_hid_indicators_changed);
ZMK_SUBSCRIPTION(rgb_layer_listener, zmk_layer_state_changed);
