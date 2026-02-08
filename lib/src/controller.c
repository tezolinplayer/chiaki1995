// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// MÁSCARAS DE BITS E DEFINIÇÕES
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
#define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif

#ifndef CHIAKI_CONTROLLER_BUTTON_R2
#define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// ------------------------------------------------------------------
// LINKER BRIDGE - VARIÁVEIS GLOBAIS (C PURO)
// ------------------------------------------------------------------
// Removido o extern "C" que causava erro no compilador de C
extern int v_stage1, h_stage1;
extern int v_stage2, h_stage2;
extern int v_stage3, h_stage3;
extern int anti_dz_global;
extern int sticky_power_global;
extern int lock_power_global;
extern int start_delay_global;
extern bool sticky_aim_global;
extern bool rapid_fire_global;
extern bool crouch_spam_global;
extern bool drop_shot_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

// --- FUNÇÕES DE ESTADO PADRÃO ---

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state) {
    state->buttons = 0;
    state->l2_state = state->r2_state = 0;
    state->left_x = state->left_y = state->right_x = state->right_y = 0;
    state->touch_id_next = 0;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        state->touches[i].id = -1;
        state->touches[i].x = state->touches[i].y = 0;
    }
    state->gyro_x = state->gyro_y = state->gyro_z = 0.0f;
    state->accel_x = 0.0f; state->accel_y = 1.0f; state->accel_z = 0.0f;
    state->orient_x = state->orient_y = state->orient_z = 0.0f; state->orient_w = 1.0f;
}

CHIAKI_EXPORT int8_t chiaki_controller_state_start_touch(ChiakiControllerState *state, uint16_t x, uint16_t y) {
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if(state->touches[i].id < 0) {
            state->touches[i].id = state->touch_id_next;
            state->touch_id_next = (state->touch_id_next + 1) & TOUCH_ID_MASK;
            state->touches[i].x = x; state->touches[i].y = y;
            return state->touches[i].id;
        }
    }
    return -1;
}

CHIAKI_EXPORT void chiaki_controller_state_stop_touch(ChiakiControllerState *state, uint8_t id) {
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if(state->touches[i].id == id) { state->touches[i].id = -1; break; }
    }
}

CHIAKI_EXPORT void chiaki_controller_state_set_touch_pos(ChiakiControllerState *state, uint8_t id, uint16_t x, uint16_t y) {
    id &= TOUCH_ID_MASK;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if(state->touches[i].id == id) { state->touches[i].x = x; state->touches[i].y = y; break; }
    }
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state &&
            a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - SMART ACTIONS v5.5
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    int32_t lx = a->left_x;
    int32_t ly = a->left_y;
    int32_t rx = (abs(a->right_x) > abs(b->right_x)) ? a->right_x : b->right_x;
    int32_t ry = (abs(a->right_y) > abs(b->right_y)) ? a->right_y : b->right_y;

    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. RECOIL POR ETAPAS (SMART ACTIONS)
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t target_v = 0;
        int32_t target_h = 0;

        if (ms <= 300) { 
            target_v = v_stage1; target_h = h_stage1;
        } 
        else if (ms > 300 && ms <= 800) {
            target_v = v_stage2; target_h = h_stage2;
        } 
        else {
            float modifier = (float)lock_power_global / 100.0f;
            target_v = (int32_t)(v_stage3 * modifier);
            target_h = (int32_t)(h_stage3 * modifier);
        }

        ry += (target_v * 150);
        rx += (target_h * 120);
    }

    // 2. AIM ASSIST
    if (sticky_aim_global && (out->l2_state > 30)) {
        float angle = (float)zen_tick * 0.28f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.6f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.8f);
    }

    // 3. RAPID FIRE 
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 10) >= 5) out->r2_state = 0;
    }

    // 4. MACROS
    if (drop_shot_global && fire_duration > 1 && fire_duration < 12) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 20) {
        if ((fire_duration / 15) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // 5. ANTI-DEADZONE
    if (abs(rx) > 50 && abs(rx) < 3000) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;
    if (abs(ry) > 50 && abs(ry) < 3000) ry += (ry > 0) ? anti_dz_global : -anti_dz_global;

    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Preservação do Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}
