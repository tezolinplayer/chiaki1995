// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

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
// LINKER BRIDGE - VARIÁVEIS GLOBAIS (AJUSTÁVEIS)
// ------------------------------------------------------------------
extern int recoil_v_global;      // Força Vertical
extern int recoil_h_global;      // Força Horizontal (Negativo = Esquerda, Positivo = Direita)
extern int anti_dz_global;       // Anti-Deadzone
extern int sticky_power_global;  // Força do Aim Assist
extern int lock_power_global;    // Estabilização de spray (0-100)
extern int start_delay_global;   // Atraso para iniciar recoil
extern bool sticky_aim_global;
extern bool rapid_fire_global;
extern bool crouch_spam_global; 
extern bool drop_shot_global;    

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

// --- FUNÇÕES DE ESTADO AUXILIARES ---

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
#define CLAMP(v)       (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - VERSÃO OTIMIZADA
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // Controle de tempo de disparo
    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. RASTREIO ROTACIONAL (STRAFE JITTER)
    // Ativa o Aim Assist rotacional através do micro-movimento do analog esquerdo
    if (sticky_aim_global && out->l2_state > 40) {
        lx += (zen_tick % 4 < 2) ? 650 : -650; 
    }

    // 2. MACROS DE MOVIMENTAÇÃO (DROP & CROUCH)
    if (drop_shot_global && fire_duration > 0 && fire_duration < 12) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 15) {
        if ((fire_duration / 12) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // 3. RAPID FIRE
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 10) >= 5) {
            out->r2_state = 0;
            out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2; 
        }
    }

    // 4. AIM ASSIST MAGNÉTICO (ELÍPTICO)
    if (sticky_aim_global && (out->l2_state > 30 || out->r2_state > 30)) {
        float angle = (float)zen_tick * 0.30f; 
        float power = (float)sticky_power_global; 
        rx += (int32_t)(cosf(angle) * power * 1.5f);
        ry += (int32_t)(sinf(angle) * power * 0.7f);
    }

    // 5. ANTI-DEADZONE (PONTO MORTO)
    if (ABS(rx) > 100 && ABS(rx) < 2500) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;
    if (ABS(ry) > 100 && ABS(ry) < 2500) ry += (ry > 0) ? anti_dz_global : -anti_dz_global;

    // 6. RECUO DINÂMICO (ESQUERDA / DIREITA / VERTICAL)
    if (fire_duration > (uint32_t)start_delay_global) { 
        float modifier = 1.0f;
        
        // Fase de estabilização do spray
        if (fire_duration > 40) {
            modifier = (float)lock_power_global / 100.0f;
            // Jitter de micro-ajuste para evitar que a mira trave num ponto fixo
            if (zen_tick % 2 == 0) ry += 10; else ry -= 10;
        } else {
            modifier = 1.30f; // Compensação extra para o chute inicial (kick)
        }

        // Aplicação do Recoil Vertical
        ry += (int32_t)(recoil_v_global * 140 * modifier);
        
        // --- NOVO: AJUSTE DE RECOIL HORIZONTAL ---
        // Se recoil_h_global > 0: compensa arma que puxa para a esquerda
        // Se recoil_h_global < 0: compensa arma que puxa para a direita
        rx += (int32_t)(recoil_h_global * 110 * modifier);
    }

    // --- ATRIBUIÇÃO FINAL COM TRAVA DE SEGURANÇA ---
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Processamento de touches (Touchpad)
    out->touch_id_next = 0;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        ChiakiControllerTouch *touch = a->touches[i].id >= 0 ? &a->touches[i] : (b->touches[i].id >= 0 ? &b->touches[i] : NULL);
        if(!touch) { out->touches[i].id = -1; continue; }
        out->touches[i] = *touch;
    }
}
