// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

// Variáveis do Painel
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int lock_power_global, start_delay_global, sticky_power_global;
extern bool sticky_aim_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0;

// Macro simples para pegar o maior valor (evita conflito Teclado x Controle)
#define MAX_ABS_VAL(a, b) (abs(a) > abs(b) ? (a) : (b))
#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;

    // 1. REPASSE DE BOTÕES E GATILHOS (PADRÃO)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;

    // 2. MOVIMENTO PADRÃO (SEM SCRIPT) - ISSO RESOLVE O ANDAR
    // Simplesmente pega o input A ou B, quem tiver movimento.
    out->left_x = MAX_ABS_VAL(a->left_x, b->left_x);
    out->left_y = MAX_ABS_VAL(a->left_y, b->left_y);

    // 3. ANALÓGICO DIREITO (MIRA) - AQUI APLICAMOS O RECOIL
    int32_t rx = MAX_ABS_VAL(a->right_x, b->right_x);
    int32_t ry = MAX_ABS_VAL(a->right_y, b->right_y);

    // Detecta disparo segurado
    if (out->r2_state > 40) fire_duration++; else fire_duration = 0;

    // Aplica Recoil apenas na Mira (Direita)
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t t_v = 0, t_h = 0;

        if (ms <= 300) { t_v = v_stage1; t_h = h_stage1; }
        else if (ms <= 800) { t_v = v_stage2; t_h = h_stage2; }
        else { 
            float mod = (float)lock_power_global / 100.0f;
            t_v = (int32_t)(v_stage3 * mod); 
            t_h = (int32_t)(h_stage3 * mod); 
        }
        
        ry += (t_v * 165); // Força para baixo
        rx += (t_h * 140);
    }

    // Sticky Aim (Apenas na Mira)
    if (sticky_aim_global && out->l2_state > 30) {
        float angle = zen_tick * 0.30f;
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Repasse de Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        out->touches[i] = (a->touches[i].id >= 0) ? a->touches[i] : b->touches[i];
    }
}

// Funções obrigatórias
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons = 0; s->left_x = 0; s->left_y = 0; s->right_x = 0; s->right_y = 0; 
}
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return a->buttons == b->buttons; 
}
