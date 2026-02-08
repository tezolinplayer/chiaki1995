// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))
#define MAX_ABS(a, b) (abs(a) > abs(b) ? (a) : (b))

extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int anti_dz_global, sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global, rapid_fire_global, crouch_spam_global, drop_shot_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

// Funções de estado obrigatórias para o Chiaki
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state) {
    state->buttons = 0;
    state->l2_state = state->r2_state = 0;
    state->left_x = state->left_y = state->right_x = state->right_y = 0;
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - FIX RECOIL CONTÍNUO (SEGURADO)
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // Mantém analógico esquerdo livre para andar
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- LÓGICA DE DISPARO SEGURADO ---
    // Aumentei o tempo de persistência para o No-Recoil não "piscar"
    if (out->r2_state > 30) { 
        fire_duration++; 
    } else { 
        fire_duration = 0; // Reset instantâneo ao soltar o dedo
    }

    // Só entra no No-Recoil se estiver segurando o gatilho além do delay
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t active_v = 0;
        int32_t active_h = 0;

        if (ms <= 350) { 
            active_v = v_stage1; active_h = h_stage1; 
        } 
        else if (ms <= 950) { 
            active_v = v_stage2; active_h = h_stage2; 
        } 
        else {
            float lock_mod = (float)lock_power_global / 100.0f;
            active_v = (int32_t)(v_stage3 * lock_mod);
            active_h = (int32_t)(h_stage3 * lock_mod);
        }

        // Aplica a força de descida contínua enquanto fire_duration > 0
        // Multiplicador 180 garante que os sliders 0-100 tenham força total
        ry += (active_v * 180); 
        rx += (active_h * 180);
    }

    // Sticky Aim sem travar o movimento do personagem
    if (sticky_aim_global && (out->l2_state > 30)) {
        lx += (zen_tick % 4 < 2) ? 750 : -750;
        float angle = (float)zen_tick * 0.32f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.6f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.8f);
    }

    // Finalização com travas de limite (Clamp)
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);
}
