/* Copyright 2017 Jason Williams
 * Copyright 2018 Jack Humbert
 *
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

#include "is31fl3731_imp.h"
#include "i2c_master.h"
#include "wait.h"

// This is a 7-bit address, that gets left-shifted and bit 0
// set to 0 for write, 1 for read (as per I2C protocol)
// The address will vary depending on your wiring:
// 0b1110100 AD <-> GND
// 0b1110111 AD <-> VCC
// 0b1110101 AD <-> SCL
// 0b1110110 AD <-> SDA
#define ISSI_ADDR_DEFAULT 0x74

#define ISSI_REG_CONFIG 0x00
#define ISSI_REG_CONFIG_PICTUREMODE 0x00
#define ISSI_REG_CONFIG_AUTOPLAYMODE 0x08
#define ISSI_REG_CONFIG_AUDIOPLAYMODE 0x18

#define ISSI_CONF_PICTUREMODE 0x00
#define ISSI_CONF_AUTOFRAMEMODE 0x04
#define ISSI_CONF_AUDIOMODE 0x08

#define ISSI_REG_PICTUREFRAME 0x01

#define ISSI_REG_SHUTDOWN 0x0A
#define ISSI_REG_AUDIOSYNC 0x06

#define ISSI_COMMANDREGISTER 0xFD
#define ISSI_BANK_FUNCTIONREG 0x0B  // helpfully called 'page nine'

#ifndef ISSI_TIMEOUT
#    define ISSI_TIMEOUT 100
#endif

#ifndef ISSI_PERSISTENCE
#    define ISSI_PERSISTENCE 0
#endif

// Transfer buffer for TWITransmitData()
static uint8_t s_twi_transfer_buffer[20];

#define DRIVER_C   1

// These buffers match the IS31FL3731 PWM registers 0x24-0xB3.
// Storing them like this is optimal for I2C transfers to the registers.
// We could optimize this and take out the unused registers from these
// buffers and the transfers in IS31FL3731_write_pwm_buffer() but it's
// probably not worth the extra complexity.
static uint8_t s_pwm_buffer[DRIVER_C][144];
static bool    s_pwm_buffer_update_required[DRIVER_C] = {false};

static uint8_t s_led_control_registers[DRIVER_C][18]             = {{0}};
static bool    s_led_control_registers_update_required[DRIVER_C] = {false};

static bool s_available = true;

#define DRIVER_LED_NUM 13
static const is31_led s_is31_leds[DRIVER_LED_NUM] = {
    {0, C1_9,   C3_10,  C4_10},
    {0, C1_10,  C2_10,  C4_11},
    {0, C1_11,  C2_11,  C3_11},
    {0, C1_12,  C2_12,  C3_12},
    {0, C1_13,  C2_13,  C3_13},
    {0, C1_14,  C2_14,  C3_14},
    {0, C1_15,  C2_15,  C3_15},
    {0, C1_16,  C2_16,  C3_16},

    {0, C9_9,   C8_9,   C7_9},
    {0, C9_10,  C8_10,  C7_10},
    {0, C9_11,  C8_11,  C7_11},
    {0, C9_12,  C8_12,  C7_12},
    {0, C9_13,  C8_13,  C7_13},
    //{0, C9_14,  C8_14,  C7_14},
    //{0, C9_15,  C8_15,  C6_14},
    //{0, C9_16,  C7_15,  C6_15},
};

bool IS31FL3731_available(void)
{
    return s_available;
}


// This is the bit pattern in the LED control registers
// (for matrix A, add one to register for matrix B)
//
//  reg -  b7  b6  b5  b4  b3  b2  b1  b0
// 0x00 - R08,R07,R06,R05,R04,R03,R02,R01
// 0x02 - G08,G07,G06,G05,G04,G03,G02,R00
// 0x04 - B08,B07,B06,B05,B04,B03,G01,G00
// 0x06 -  - , - , - , - , - ,B02,B01,B00
// 0x08 -  - , - , - , - , - , - , - , -
// 0x0A - B17,B16,B15, - , - , - , - , -
// 0x0C - G17,G16,B14,B13,B12,B11,B10,B09
// 0x0E - R17,G15,G14,G13,G12,G11,G10,G09
// 0x10 - R16,R15,R14,R13,R12,R11,R10,R09

void IS31FL3731_write_register(uint8_t addr, uint8_t reg, uint8_t data) {
    s_twi_transfer_buffer[0] = reg;
    s_twi_transfer_buffer[1] = data;

#if ISSI_PERSISTENCE > 0
    for (uint8_t i = 0; i < ISSI_PERSISTENCE; i++) {
        if (i2c_transmit(addr << 1, s_twi_transfer_buffer, 2, ISSI_TIMEOUT) == 0) break;
    }
#else
    i2c_transmit(addr << 1, s_twi_transfer_buffer, 2, ISSI_TIMEOUT);
#endif
}

void IS31FL3731_write_pwm_buffer(uint8_t addr, uint8_t *pwm_buffer) {
    // assumes bank is already selected

    // transmit PWM registers in 9 transfers of 16 bytes
    // g_twi_transfer_buffer[] is 20 bytes

    // iterate over the pwm_buffer contents at 16 byte intervals
    for (int i = 0; i < 144; i += 16) {
        // set the first register, e.g. 0x24, 0x34, 0x44, etc.
        s_twi_transfer_buffer[0] = 0x24 + i;
        // copy the data from i to i+15
        // device will auto-increment register for data after the first byte
        // thus this sets registers 0x24-0x33, 0x34-0x43, etc. in one transfer
        for (int j = 0; j < 16; j++) {
            s_twi_transfer_buffer[1 + j] = pwm_buffer[i + j];
        }

#if ISSI_PERSISTENCE > 0
        for (uint8_t i = 0; i < ISSI_PERSISTENCE; i++) {
            if (i2c_transmit(addr << 1, s_twi_transfer_buffer, 17, ISSI_TIMEOUT) == 0) break;
        }
#else
        i2c_transmit(addr << 1, s_twi_transfer_buffer, 17, ISSI_TIMEOUT);
#endif
    }
}

void IS31FL3731_init(uint8_t addr) {
    // In order to avoid the LEDs being driven with garbage data
    // in the LED driver's PWM registers, first enable software shutdown,
    // then set up the mode and other settings, clear the PWM registers,
    // then disable software shutdown.
    s_twi_transfer_buffer[0] = ISSI_COMMANDREGISTER;
    s_twi_transfer_buffer[1] = ISSI_BANK_FUNCTIONREG;

    if (i2c_transmit(addr << 1, s_twi_transfer_buffer, 2, ISSI_TIMEOUT) != I2C_STATUS_SUCCESS) {
        s_available = false;
        return;
    }


    // select "function register" bank
    IS31FL3731_write_register(addr, ISSI_COMMANDREGISTER, ISSI_BANK_FUNCTIONREG);

    // enable software shutdown
    IS31FL3731_write_register(addr, ISSI_REG_SHUTDOWN, 0x00);

    // this delay was copied from other drivers, might not be needed
    wait_ms(10);

    // picture mode
    IS31FL3731_write_register(addr, ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);
    // display frame 0
    IS31FL3731_write_register(addr, ISSI_REG_PICTUREFRAME, 0x00);
    // audio sync off
    IS31FL3731_write_register(addr, ISSI_REG_AUDIOSYNC, 0x00);

    // select bank 0
    IS31FL3731_write_register(addr, ISSI_COMMANDREGISTER, 0);

    // turn off all LEDs in the LED control register
    for (int i = 0x00; i <= 0x11; i++) {
        IS31FL3731_write_register(addr, i, 0x00);
    }

    // turn off all LEDs in the blink control register (not really needed)
    for (int i = 0x12; i <= 0x23; i++) {
        IS31FL3731_write_register(addr, i, 0x00);
    }

    // set PWM on all LEDs to 0
    for (int i = 0x24; i <= 0xB3; i++) {
        IS31FL3731_write_register(addr, i, 0x00);
    }

    // select "function register" bank
    IS31FL3731_write_register(addr, ISSI_COMMANDREGISTER, ISSI_BANK_FUNCTIONREG);

    // disable software shutdown
    IS31FL3731_write_register(addr, ISSI_REG_SHUTDOWN, 0x01);

    // select bank 0 and leave it selected.
    // most usage after initialization is just writing PWM buffers in bank 0
    // as there's not much point in double-buffering
    IS31FL3731_write_register(addr, ISSI_COMMANDREGISTER, 0);
}

void IS31FL3731_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (!IS31FL3731_available()) return;

    if (index >= 0 && index < DRIVER_LED_NUM) {
        is31_led led = s_is31_leds[index];

        // Subtract 0x24 to get the second index of g_pwm_buffer
        s_pwm_buffer[led.driver][led.r - 0x24]   = red;
        s_pwm_buffer[led.driver][led.g - 0x24]   = green;
        s_pwm_buffer[led.driver][led.b - 0x24]   = blue;
        s_pwm_buffer_update_required[led.driver] = true;
    }
}

void IS31FL3731_set_color_all(uint8_t red, uint8_t green, uint8_t blue) {
    for (int i = 0; i < DRIVER_LED_NUM; i++) {
        IS31FL3731_set_color(i, red, green, blue);
    }
}

void IS31FL3731_set_led_control_register(uint8_t index, bool red, bool green, bool blue) {
    if (!IS31FL3731_available()) return;
    is31_led led = s_is31_leds[index];

    uint8_t control_register_r = (led.r - 0x24) / 8;
    uint8_t control_register_g = (led.g - 0x24) / 8;
    uint8_t control_register_b = (led.b - 0x24) / 8;
    uint8_t bit_r              = (led.r - 0x24) % 8;
    uint8_t bit_g              = (led.g - 0x24) % 8;
    uint8_t bit_b              = (led.b - 0x24) % 8;

    if (red) {
        s_led_control_registers[led.driver][control_register_r] |= (1 << bit_r);
    } else {
        s_led_control_registers[led.driver][control_register_r] &= ~(1 << bit_r);
    }
    if (green) {
        s_led_control_registers[led.driver][control_register_g] |= (1 << bit_g);
    } else {
        s_led_control_registers[led.driver][control_register_g] &= ~(1 << bit_g);
    }
    if (blue) {
        s_led_control_registers[led.driver][control_register_b] |= (1 << bit_b);
    } else {
        s_led_control_registers[led.driver][control_register_b] &= ~(1 << bit_b);
    }

    s_led_control_registers_update_required[led.driver] = true;
}

void IS31FL3731_update_pwm_buffers(uint8_t addr, uint8_t index) {
    if (!IS31FL3731_available()) return;
    if (s_pwm_buffer_update_required[index]) {
        IS31FL3731_write_pwm_buffer(addr, s_pwm_buffer[index]);
    }
    s_pwm_buffer_update_required[index] = false;
}

void IS31FL3731_update_led_control_registers(uint8_t addr, uint8_t index) {
    if (!IS31FL3731_available()) return;
    if (s_led_control_registers_update_required[index]) {
        for (int i = 0; i < 18; i++) {
            IS31FL3731_write_register(addr, i, s_led_control_registers[index][i]);
        }
    }
    s_led_control_registers_update_required[index] = false;
}
