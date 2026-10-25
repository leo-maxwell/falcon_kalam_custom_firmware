// Copyright 2026 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

/*******************************************************************************
  88888888888 888      d8b                .d888 d8b 888               d8b
      888     888      Y8P               d88P"  Y8P 888               Y8P
      888     888                        888        888
      888     88888b.  888 .d8888b       888888 888 888  .d88b.       888 .d8888b
      888     888 "88b 888 88K           888    888 888 d8P  Y8b      888 88K
      888     888  888 888 "Y8888b.      888    888 888 88888888      888 "Y8888b.
      888     888  888 888      X88      888    888 888 Y8b.          888      X88
      888     888  888 888  88888P'      888    888 888  "Y8888       888  88888P'
                                                        888                 888
                                                        888                 888
                                                        888                 888
     .d88b.   .d88b.  88888b.   .d88b.  888d888 8888b.  888888 .d88b.   .d88888
    d88P"88b d8P  Y8b 888 "88b d8P  Y8b 888P"      "88b 888   d8P  Y8b d88" 888
    888  888 88888888 888  888 88888888 888    .d888888 888   88888888 888  888
    Y88b 888 Y8b.     888  888 Y8b.     888    888  888 Y88b. Y8b.     Y88b 888
     "Y88888  "Y8888  888  888  "Y8888  888    "Y888888  "Y888 "Y8888   "Y88888
         888
    Y8b d88P
     "Y88P"
*******************************************************************************/

#pragma once

#ifndef WEAR_LEVELING_BACKING_SIZE
#    define WEAR_LEVELING_BACKING_SIZE 2048
#endif // WEAR_LEVELING_BACKING_SIZE

#ifndef WEAR_LEVELING_LOGICAL_SIZE
#    define WEAR_LEVELING_LOGICAL_SIZE 1024
#endif // WEAR_LEVELING_LOGICAL_SIZE

#ifndef DIODE_DIRECTION
#    define DIODE_DIRECTION COL2ROW
#endif // DIODE_DIRECTION

#ifndef RGBLIGHT_LED_COUNT
#    define RGBLIGHT_LED_COUNT 4
#endif // RGBLIGHT_LED_COUNT

#define RGBLIGHT_SLEEP

#ifndef WS2812_DI_PIN
#    define WS2812_DI_PIN A1
#endif // WS2812_DI_PIN

#ifndef DEVICE_VER
#    define DEVICE_VER 0x0001
#endif // DEVICE_VER

#ifndef MANUFACTURER
#    define MANUFACTURER "Falcon"
#endif // MANUFACTURER

#ifndef PRODUCT
#    define PRODUCT "Falcon HS R2"
#endif // PRODUCT

#ifndef PRODUCT_ID
#    define PRODUCT_ID 0x9D89
#endif // PRODUCT_ID

#ifndef VENDOR_ID
#    define VENDOR_ID 0x9DD1
#endif // VENDOR_ID

#ifndef MATRIX_COLS
#    define MATRIX_COLS 15
#endif // MATRIX_COLS

#ifndef MATRIX_ROWS
#    define MATRIX_ROWS 5
#endif // MATRIX_ROWS

#ifndef MATRIX_COL_PINS
#    define MATRIX_COL_PINS { A9, A3, A4, A5, A6, A7, B0, B1, B10, B11, B12, B13, B14, B15, A8 }
#endif // MATRIX_COL_PINS

#ifndef MATRIX_ROW_PINS
#    define MATRIX_ROW_PINS { B3, B4, B5, B6, A2 }
#endif // MATRIX_ROW_PINS

#define RGBLIGHT_EFFECT_ALTERNATING

#define RGBLIGHT_EFFECT_BREATHING

#define RGBLIGHT_EFFECT_CHRISTMAS

#define RGBLIGHT_EFFECT_KNIGHT

#define RGBLIGHT_EFFECT_RAINBOW_MOOD

#define RGBLIGHT_EFFECT_RAINBOW_SWIRL

#define RGBLIGHT_EFFECT_RGB_TEST

#define RGBLIGHT_EFFECT_SNAKE

#define RGBLIGHT_EFFECT_STATIC_GRADIENT

#define RGBLIGHT_EFFECT_TWINKLE
