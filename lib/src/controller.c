// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// MÁSCARAS DE BITS - BOTÕES PS5
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
    #define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif
#ifndef CHIAKI_CONTROLLER_BUTTON_R2
    #define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// ------------------------------------------------------------------
// LINKER BRIDGE - DEFINIÇÃO DAS VARIÁVEIS (CONECTA COM SUA INTERFACE)
// ------------------------------------------------------------------
int recoil_v_global = 0;
int recoil_h_global = 0;
int anti_dz_global = 0;
int sticky_power_global = 0;
int lock_power_global = 100;   
int start_delay_global = 0;  
bool sticky_aim_global = false;
bool rapid_fire_global = false;
bool crouch_spam_global = false; 
bool drop_shot_global = false;   

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

// --- MACROS DE APOIO ---
#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define ABS(a)         ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b)  (ABS(a) > ABS(b) ? (a) : (b))
#define CLAMP(v)       (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

// ------------------------------------------------------------------
// FUNÇÕES OBRIGATÓRIAS DO SISTEMA CHIAKI
// ------------------------------------------------------------------

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state) {
    state->buttons = 0;
    state->l2_state = state->r2_state = 0;
    state->left_x = state->left_y = state->right_x = state->right_y = 0;
    state->touch_id_next = 0;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        state->touches[i].id = -1;
    }
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state &&
            a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// ------------------------------------------------------------------
// MOTOR DE PROCESSAMENTO DANIEL GHOST ELITE (v5.5 FINAL)
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    // Unifica Inputs de botões e gatilhos
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // Captura movimento real dos analógicos
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // Detecção de tempo de disparo (R2)
    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // --- PARTE 1: RASTREIO ROTACIONAL (STRAFE JITTER) ---
    if (sticky_aim_global && out->l2_state > 40) {
        lx = CLAMP(lx + ((zen_tick % 4 < 2) ? 5500 : -5500));
    }

    // --- PARTE 2: MACROS (DROP SHOT & CROUCH SPAM) ---
    if (drop_shot_global && fire_duration > 1 && fire_duration < 12) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 10) {
        if ((fire_duration / 20) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // --- PARTE 3: RAPID FIRE ---
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 6) < 3) {
            out->r2_state = 0;
            out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2; 
        }
    }

    // --- PARTE 4: SUPER MAGNETISMO ELÍPTICO (TRACKING) ---
    if (sticky_aim_global && (out->l2_state > 30 || out->r2_state > 30)) {
        float angle = (float)zen_tick * 0.22f; 
        float power = (float)sticky_power_global; 
        rx = CLAMP(rx + (int32_t)(cosf(angle) * power * 1.6f));
        ry = CLAMP(ry + (int32_t)(sinf(angle) * power * 0.8f));
    }

    // --- PARTE 5: ANTI-DEADZONE ---
    if (ABS(rx) > 150) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;
    if (ABS(ry) > 150) ry += (ry > 0) ? anti_dz_global : -anti_dz_global;

    // --- PARTE 6: ANTI-RECOIL AJUSTÁVEL ---
    if (fire_duration > (uint32_t)start_delay_global) { 
        float multiplier = (fire_duration > 45) ? ((float)lock_power_global / 100.0f) : 1.30f;
        
        ry = CLAMP(ry + (int32_t)(recoil_v_global * 145 * multiplier));
        rx = CLAMP(rx + (int32_t)(recoil_h_global * 145));
    }

    // --- ATRIBUIÇÃO FINAL COM SEGURANÇA ---
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Processamento de touches (Touchpad)
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}
