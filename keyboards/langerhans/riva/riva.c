/* SPDX-License-Identifier: GPL-2.0-or-later */

#include "riva.h"

void matrix_init_kb(void) {
    setPinOutput(GP18);
}

bool led_update_kb(led_t led_state) {
    if (led_update_user(led_state)) {
        writePin(GP18, led_state.caps_lock);
    }
    return true;
}