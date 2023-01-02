/* SPDX-License-Identifier: GPL-2.0-or-later */

#pragma once

#include "config_common.h"

/* key matrix size */
#define MATRIX_ROWS 5
#define MATRIX_COLS 15

#define MATRIX_ROW_PINS {GP25, GP26, GP27, GP28, GP29}
#define MATRIX_COL_PINS {GP14, GP15, GP12, GP0, GP1, GP2, GP3, GP4, GP5, GP6, GP7, GP8, GP9, GP10, GP11}

/* COL2ROW or ROW2COL */
#define DIODE_DIRECTION COL2ROW
