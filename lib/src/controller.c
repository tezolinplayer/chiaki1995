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

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state) {
    state->buttons = 0;
    state->l2_state = state->r2_state = 0;
    state->left_x = state->left_y = state->right_x = state->right_y = 0;
    state->touch_id_next = 0;
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        state->touches[i].id = -1;
        state->touches[i].x = state->touches[i].y = 0;
    }
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - FIX RECOIL SEGURADO (R2)
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // Analógico Esquerdo (Movimento livre para andar)
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    
    // Analógico Direito (Mira)
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- DETECÇÃO DE DISPARO SEGURADO ---
    if (out->r2_state > 35) { // Sensibilidade aumentada para o gatilho
        fire_duration++; 
    } else { 
        fire_duration = 0; // Soltou o R2, para de puxar a mira NA HORA
    }

    // --- SISTEMA DE ETAPAS XIM MATRIX ---
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t force_v = 0;
        int32_t force_h = 0;

        if (ms <= 350) { 
            force_v = v_stage1; force_h = h_stage1; 
        } 
        else if (ms <= 900) { 
            force_v = v_stage2; force_h = h_stage2; 
        } 
        else {
            float mod = (float)lock_power_global / 100.0f;
            force_v = (int32_t)(v_stage3 * mod);
            force_h = (int32_t)(h_stage3 * mod);
        }

        // Aplicação da força: ry positivo EMPURRA para baixo
        ry += (force_v * 175); // Multiplicador aumentado para garantir a descida
        rx += (force_h * 175);
    }

    // Sticky Aim (Magnetismo) sem travar o personagem
    if (sticky_aim_global && (out->l2_state > 30)) {
        lx += (zen_tick % 4 < 2) ? 700 : -700; // Micro-strafe para AA
        float angle = (float)zen_tick * 0.30f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    // Finalização com travas de segurança (Clamp)
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        out->touches[i] = (a->touches[i].id >= 0) ? a->touches[i] : b->touches[i];
    }
}
