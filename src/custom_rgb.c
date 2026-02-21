#include <stdint.h>
#include <zephyr/kernel.h>
#include <zmk/rgb_underglow.h>

void change_hue_only(uint16_t new_hue) {
  struct zmk_led_hsb color = zmk_rgb_underglow_calc_hue(0);
  color.h = new_hue;
  zmk_rgb_underglow_set_hsb(color);
}
