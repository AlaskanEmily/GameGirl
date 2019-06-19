/* Copyright (c) 2019 Emily McDonough
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu_length.h"

#include "cpu_dummy.h"
#include "cpu_dummy_meta.h"

#define GG_OPCODE(_1, _2, TIME) TIME,
#define GG_END_OPCODE( _ )
#define GG_PREFIX_CB( _ )

static unsigned char cpu_opcode_times[0x101] = {
#include "cpu.inc"
    0
};

const unsigned char *const gg_cpu_opcode_times = cpu_opcode_times;
const unsigned char *const _gg_cpu_opcode_times = cpu_opcode_times;
