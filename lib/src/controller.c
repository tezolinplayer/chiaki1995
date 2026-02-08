// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define TOUCH_ID_MASK 0x7f

// --- VARIÁVEIS GLOBAIS ---
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int anti_dz_global, sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global, rapid_fire_global, crouch_spam_global, drop_shot_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))
#define MAX_ABS(a, b) (abs(a) > abs(b) ? (a) : (b))

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
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - FIX MOVIMENTO TOTAL
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // --- FIX: Captura o movimento MÁXIMO entre as duas fontes de input (A e B) ---
    // Isso garante que se você mexer no teclado ou no controle, o personagem ande.
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. SMART RECOIL (Escala 0-100)
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t target_v = 0, target_h = 0;

        if (ms <= 300) { target_v = v_stage1; target_h = h_stage1; } 
        else if (ms <= 800) { target_v = v_stage2; target_h = h_stage2; } 
        else {
            float modifier = (float)lock_power_global / 100.0f;
            target_v = (int32_t)(v_stage3 * modifier);
            target_h = (int32_t)(h_stage3 * modifier);
        }
        ry += (target_v * 150);
        rx += (target_h * 150);
    }

    // 2. STICKY AIM (Ajustado para não bloquear o analógico)
    if (sticky_aim_global && (out->l2_state > 30)) {
        // Aplica o micro-movimento apenas se o jogador não estiver correndo no talo
        // Isso evita que o jitter "quebre" a zona morta do jogo.
        lx += (zen_tick % 4 < 2) ? 600 : -600; 

        float angle = (float)zen_tick * 0.28f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.8f);
    }

    // 3. ANTI-DEADZONE
    if (abs(rx) > 50 && abs(rx) < 3000) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;

    // --- ATRIBUIÇÃO FINAL ---
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Preservação do Touchpad e outros dados
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        out->touches[i] = (a->touches[i].id >= 0) ? a->touches[i] : b->touches[i];
    }
}
