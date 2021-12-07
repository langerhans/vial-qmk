/* Copyright 2021 CapsUnlocked
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

#define CONTROL_CHANNEL 9 // Arbitrary channel for control messages independant of selected output channel

#define VOL_STEP 5 // One encoder step will inc/dec volume by this value

#define MSG_VOL 7
#define MSG_MUTE_0 80
#define MSG_MUTE_1 81
#define MSG_CH_SWTCH 82

extern MidiDevice midi_device;

enum my_keycodes {
  MI_CH_SWTCH = SAFE_RANGE,
};

// Tapdance definitions
enum {
    TD_CH0_MUTE,
    TD_CH1_MUTE,
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT(
                        LT(1, KC_MUTE),
    TD(TD_CH0_MUTE),    TD(TD_CH1_MUTE),    MI_CH_SWTCH,
        KC_MPRV,             KC_MPLY,       KC_MNXT
  ),
  [1] = LAYOUT(
             _______,
    RGB_MOD, KC_UP,   RESET,
    KC_LEFT, KC_DOWN, KC_RGHT
  ),
  [2] = LAYOUT(
             _______,
    _______, _______, _______,
    _______, _______, _______
  ),
  [3] = LAYOUT(
             _______,
    _______, _______, _______,
    _______, _______, _______
  ),
};

// Convert an absolute value into a relativeone that Voicemeeter understands
static uint8_t toBinaryOffset7bit(int8_t value) { return value + 64; }

// Volume up/down on the encoder
bool encoder_update_user(uint8_t index, bool clockwise) {
  if (clockwise) {
    midi_send_cc(&midi_device, midi_config.channel, MSG_VOL, toBinaryOffset7bit(VOL_STEP));
  } else {
    midi_send_cc(&midi_device, midi_config.channel, MSG_VOL, toBinaryOffset7bit(-VOL_STEP));
  }

  return true;
}

// Custom keycodes
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case MI_CH_SWTCH:
            if (record->event.pressed) {
                midi_send_cc(&midi_device, CONTROL_CHANNEL, MSG_CH_SWTCH, 1);
            }
            return false;
    }
    return true;
};

// Channel 0 single/double tap
void dance_ch0(qk_tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        midi_config.channel = 0; // Single tap changes internal channel
    } else {
        midi_send_cc(&midi_device, CONTROL_CHANNEL, MSG_MUTE_0, 1); // Double tap sends mute
    }
}

// Channel 1 single/double tap
void dance_ch1(qk_tap_dance_state_t *state, void *user_data) {
    if (state->count == 1) {
        midi_config.channel = 1; // Single tap changes internal channel
    } else {
        midi_send_cc(&midi_device, CONTROL_CHANNEL, MSG_MUTE_1, 1); // Double tap sends mute
    }
}

qk_tap_dance_action_t tap_dance_actions[] = {
    [TD_CH0_MUTE] = ACTION_TAP_DANCE_FN(dance_ch0),
    [TD_CH1_MUTE] = ACTION_TAP_DANCE_FN(dance_ch1),
};