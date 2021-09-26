/**
 * config.h
 *
    Copyright 2021 astro

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID       0x4E65  //'Ne'
#define PRODUCT_ID      0x4537  //'E7'
#define DEVICE_VER      0x0001
#define MANUFACTURER    Neson Design
#define PRODUCT         E7 Keyboard

/* key matrix size */
#define MATRIX_ROWS 5
#define MATRIX_COLS 16
#define MATRIX_ROW_PINS { E6, B7, F6, F1, F0}
#ifdef V1
#define RGB_DI_PIN          D4
#define MATRIX_COL_PINS { F7, B3, B2, B1, B0, F4, C7, C6, B6, B5, B4, D5, D2, D3, D7, D6}
#else
#define MATRIX_COL_PINS { F7, B0, B3, B1, B2, F4, C7, C6, B6, B5, B4, D7, D2, D3, D6, D4}
#define RGB_DI_PIN          D5
#endif

#define UNUSED_PINS
#define DIODE_DIRECTION COL2ROW

/* Debounce reduces chatter (unintended double-presses) - set 0 if debouncing is not needed */
#define DEBOUNCE 5

/*
 * Feature disable options
 *  These options are also useful to firmware size reduction.
 */

/* disable debug print */
//#define NO_DEBUG

/* disable print */
//#define NO_PRINT


//rgb light setting
//#define RGBLIGHT_LIMIT_VAL 128
#define RGBLED_NUM          38
#define RGBLIGHT_ANIMATIONS
#define RGBLIGHT_HUE_STEP   8
#define RGBLIGHT_SAT_STEP   8
#define RGBLIGHT_VAL_STEP   8

#define DRIVER_1_LED_TOTAL 32
#define DRIVER_ADDR_1 0b1110100
#define DRIVER_COUNT 1
#define DRIVER_LED_TOTAL (DRIVER_1_LED_TOTAL)

#define CAPS_PIN    F5

//#define VIAL_INSECURE
//#define VIAL_KEYBOARD_UID {0xC3, 0xE5, 0x85, 0x77, 0x80, 0x0B, 0x08, 0xCB}