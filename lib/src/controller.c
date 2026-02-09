// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

// Variáveis da Interface
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0;

// Macros de Segurança
#define MAX_VAL(x, y) (((x) > (y)) ? (x) : (y))
#define ABS_VAL(x) (((x) < 0) ? -(x) : (x))
#define MAX_ABS_VAL(x, y) ((ABS_VAL(x) > ABS_VAL(y)) ? (x) : (y))
#define CLAMP_VAL(x) ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;
    
    // 1. REPASSE DE BOTÕES E GATILHOS (Habilita Mira e Tiro)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX_VAL(a->l2_state, b->l2_state);
    out->r2_state = MAX_VAL(a->r2_state, b->r2_state);
    
    // 2. ANALÓGICOS (Habilita Andar e Olhar) - Prioriza o sinal mais forte
    int32_t lx = MAX_ABS_VAL(a->left_x, b->left_x);
    int32_t ly = MAX_ABS_VAL(a->left_y, b->left_y);
    int32_t rx = MAX_ABS_VAL(a->right_x, b->right_x);
    int32_t ry = MAX_ABS_VAL(a->right_y, b->right_y);

    // 3. DETECÇÃO DE DISPARO
    if (out->r2_state > 35) fire_duration++; else fire_duration = 0;

    // 4. RECOIL DINÂMICO (3 ESTÁGIOS)
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t pull_v = 0, pull_h = 0;

        if (ms <= 300) { pull_v = v_stage1; pull_h = h_stage1; }
        else if (ms <= 800) { pull_v = v_stage2; pull_h = h_stage2; }
        else { 
            float mod = (float)lock_power_global / 100.0f;
            pull_v = (int32_t)(v_stage3 * mod); pull_h = (int32_t)(h_stage3 * mod); 
        }

        // Aplica força real (RY+ desce a mira)
        ry += (pull_v * 165); 
        rx += (pull_h * 140);
    }

    // 5. MAGNETISMO SUAVE
    if (sticky_aim_global && out->l2_state > 30) {
        float rad = (float)zen_tick * 0.12f;
        rx += (int32_t)(cosf(rad) * sticky_power_global * 0.4f);
        ry += (int32_t)(sinf(rad) * sticky_power_global * 0.15f);
    }

    // 6. SAÍDA FINAL
    out->left_x = (int16_t)CLAMP_VAL(lx);
    out->left_y = (int16_t)CLAMP_VAL(ly);
    out->right_x = (int16_t)CLAMP_VAL(rx);
    out->right_y = (int16_t)CLAMP_VAL(ry);

    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        out->touches[i] = (a->touches[i].id >= 0) ? a->touches[i] : b->touches[i];
    }
}

// Funções de Sistema (Obrigatórias)
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons=0; s->l2_state=0; s->r2_state=0; s->left_x=0; s->left_y=0; s->right_x=0; s->right_y=0; 
}
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return (a->buttons == b->buttons && a->r2_state == b->r2_state); 
}
