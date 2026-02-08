// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

extern int recoil_v_global, recoil_h_global;
extern int lock_power_global, start_delay_global, sticky_power_global, anti_dz_global;
extern bool sticky_aim_global, rapid_fire_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0;

#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b) (ABS(a) > ABS(b) ? (a) : (b))
#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;
    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;

    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- DISPARO SEGURADO ---
    if (out->r2_state > 40) fire_duration++; else fire_duration = 0;

    if (fire_duration > (uint32_t)start_delay_global) {
        float mod = (fire_duration > 45) ? (lock_power_global / 100.0f) : 1.25f;
        ry += (int32_t)(recoil_v_global * 170 * mod); // ry+ empurra para baixo
        rx += (int32_t)(recoil_h_global * 140 * mod);
    }

    // Aim Assist Jitter
    if (sticky_aim_global && out->l2_state > 30) {
        float angle = zen_tick * 0.30f;
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    out->left_x = (int16_t)CLAMP(lx); out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx); out->right_y = (int16_t)CLAMP(ry);
}

// Funções obrigatórias para o Linker
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { s->buttons = 0; s->left_x = s->left_y = s->right_x = s->right_y = 0; }
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { return a->buttons == b->buttons; }
