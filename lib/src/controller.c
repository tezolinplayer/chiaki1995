// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// DANIEL GHOST ZEN - MÁSCARAS DE BITS (PS5)
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
#define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif

#ifndef CHIAKI_CONTROLLER_BUTTON_R2
#define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// ------------------------------------------------------------------
// LINKER BRIDGE - VARIÁVEIS GLOBAIS
// ------------------------------------------------------------------
extern int recoil_v_global;
extern int recoil_h_global;
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

// --- FUNÇÕES DE ESTADO BÁSICAS ---

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

#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define ABS(a)         ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b)  (ABS(a) > ABS(b) ? (a) : (b))

// ------------------------------------------------------------------
// MOTOR DE PROCESSAMENTO DANIEL GHOST ELITE (v5.1)
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // Captura o movimento REAL para não travar o personagem
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. RASTREIO ROTACIONAL (STRAFE JITTER - FIX)
    if (sticky_aim_global && out->l2_state > 40) {
        lx += (zen_tick % 4 < 2) ? 650 : -650; 
    }

    // 2. MACROS DE MOVIMENTAÇÃO
    if (drop_shot_global && fire_duration > 0 && fire_duration < 10) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 0) {
        if ((fire_duration / 15) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // 3. RAPID FIRE
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 10) >= 5) {
            out->r2_state = 0;
            out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2; 
        }
    }

    // 4. SUPER MAGNETISMO ELÍPTICO (TRACKING FOLLOW)
    if (sticky_aim_global && (out->l2_state > 30 || out->r2_state > 30)) {
        float angle = (float)zen_tick * 0.28f; 
        float power = (float)sticky_power_global; 
        rx += (int32_t)(cosf(angle) * power * 1.8f);
        ry += (int32_t)(sinf(angle) * power * 0.8f);
        if (zen_tick % 2 == 0) rx += 20; else rx -= 20;
    }

    // 5. ANTI-DEADZONE
    if (rx > 500) rx += anti_dz_global; else if (rx < -500) rx -= anti_dz_global;
    if (ry > 500) ry += anti_dz_global; else if (ry < -500) ry -= anti_dz_global;

    // 6. RECUO ESTÁTICO (SPRAY LOCK)
    if (fire_duration > (uint32_t)start_delay_global) { 
        float multiplier = 1.0f;
        if (fire_duration > 45) {
            multiplier = (float)lock_power_global / 100.0f; 
            if (zen_tick % 2 == 0) ry += 12; else ry -= 12;
            rx += (int32_t)(recoil_h_global * 45); 
        } else {
            multiplier = 1.35f;
        }
        ry += (int32_t)(recoil_v_global * 150 * multiplier);
        rx += (int32_t)(recoil_h_global * 150);
    }

    // --- SEGURANÇA E ATRIBUIÇÃO FINAL (TODOS OS EIXOS) ---
    #define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))
    
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Processamento de touches preservado
    out->touch_id_next = 0;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        ChiakiControllerTouch *touch = a->touches[i].id >= 0 ? &a->touches[i] : (b->touches[i].id >= 0 ? &b->touches[i] : NULL);
        if(!touch) { out->touches[i].id = -1; continue; }
        out->touches[i] = *touch;
    }
}
