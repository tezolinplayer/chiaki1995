// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define TOUCH_ID_MASK 0x7f

// --- LINKER BRIDGE - VARIÁVEIS GLOBAIS ---
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int anti_dz_global, sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global, rapid_fire_global, crouch_spam_global, drop_shot_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))
#define MAX_ABS(a, b) (abs(a) > abs(b) ? (a) : (b))

// --- FUNÇÕES DE ESTADO (OBRIGATÓRIAS PARA RESOLVER O ERRO DO LINKER) ---

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

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) {
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state &&
            a->left_x == b->left_x && a->left_y == b->left_y && a->right_x == b->right_x && a->right_y == b->right_y);
}

// Funções de Touch (Stubs para integridade)
CHIAKI_EXPORT int8_t chiaki_controller_state_start_touch(ChiakiControllerState *state, uint16_t x, uint16_t y) { return -1; }
CHIAKI_EXPORT void chiaki_controller_state_stop_touch(ChiakiControllerState *state, uint8_t id) {}
CHIAKI_EXPORT void chiaki_controller_state_set_touch_pos(ChiakiControllerState *state, uint8_t id, uint16_t x, uint16_t y) {}

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - SMART ACTIONS v5.5
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // Movimento do personagem (Libera analógico esquerdo)
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    
    // Movimento da mira
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- LOGICA DE DISPARO SEGURADO ---
    if (out->r2_state > 45) { 
        fire_duration++; // Enquanto segurar R2, o tempo de spray conta
    } else { 
        fire_duration = 0; // Soltou, o no-recoil desliga na hora
    }

    // Aplica o Smart Recoil apenas se estiver segurando o gatilho
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t target_v = 0, target_h = 0;

        if (ms <= 400) { 
            target_v = v_stage1; target_h = h_stage1; 
        } 
        else if (ms <= 1000) { 
            target_v = v_stage2; target_h = h_stage2; 
        } 
        else {
            float modifier = (float)lock_power_global / 100.0f;
            target_v = (int32_t)(v_stage3 * modifier);
            target_h = (int32_t)(h_stage3 * modifier);
        }

        // ry positivo empurra a mira para baixo (segura o recoil)
        ry += (target_v * 160); 
        rx += (target_h * 160);
    }

    // Sticky Aim sem bloquear o analógico de movimento
    if (sticky_aim_global && (out->l2_state > 30)) {
        lx += (zen_tick % 4 < 2) ? 650 : -650; 
        float angle = (float)zen_tick * 0.28f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    // Atribuição final com trava de segurança
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Repasse de touches (Prevenção de erro)
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        out->touches[i] = (a->touches[i].id >= 0) ? a->touches[i] : b->touches[i];
    }
}
