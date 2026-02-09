// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// DEFINIÇÕES DE BOTÕES PS5
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
    #define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif
#ifndef CHIAKI_CONTROLLER_BUTTON_R2
    #define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// ------------------------------------------------------------------
// LINKER BRIDGE - DEFINIÇÕES REAIS (RESOLVE ERRO DE COMPILAÇÃO)
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

// --- FUNÇÕES DE AUXÍLIO ---
#define MAX(a, b)      ((a) > (b) ? (a) : (b))
#define ABS(a)         ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b)  (ABS(a) > ABS(b) ? (a) : (b))
#define CLAMP(v)       (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

// --- FUNÇÕES DE ESTADO OBRIGATÓRIAS ---

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

    // Unificação de botões e gatilhos
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // Captura inicial dos eixos analógicos
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // Lógica de tempo de disparo
    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. RASTREIO ROTACIONAL (JITTER) - Melhora o Aim Assist parado
    if (sticky_aim_global && (out->l2_state > 40)) {
        lx = CLAMP(lx + ((zen_tick % 4 < 2) ? 5500 : -5500));
    }

    // 2. MACROS (DROP SHOT & CROUCH SPAM)
    if (drop_shot_global && fire_duration > 1 && fire_duration < 15) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 5) {
        if ((fire_duration / 20) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // 3. RAPID FIRE
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 6) < 3) {
            out->r2_state = 0;
            out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2; 
        }
    }

    // 4. STICKY AIM ELÍPTICO (TRACKING)
    if (sticky_aim_global && (out->l2_state > 30 || out->r2_state > 30)) {
        float angle = (float)zen_tick * 0.20f; 
        float p = (float)sticky_power_global; 
        rx = CLAMP(rx + (int32_t)(cosf(angle) * p * 1.5f));
        ry = CLAMP(ry + (int32_t)(sinf(angle) * p * 0.7f));
    }

    // 5. ANTI-DEADZONE
    if (ABS(rx) > 100) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;
    if (ABS(ry) > 100) ry += (ry > 0) ? anti_dz_global : -anti_dz_global;

    // 6. ANTI-RECOIL AJUSTÁVEL
    if (fire_duration > (uint32_t)start_delay_global) { 
        float mult = (fire_duration > 50) ? ((float)lock_power_global / 100.0f) : 1.25f;
        
        ry = CLAMP(ry + (int32_t)(recoil_v_global * 140 * mult));
        rx = CLAMP(rx + (int32_t)(recoil_h_global * 140));
        
        // Micro-jitter vertical para quebrar o padrão estático
        ry += (zen_tick % 2 == 0) ? 15 : -15;
    }

    // ATRIBUIÇÃO FINAL COM SEGURANÇA
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
