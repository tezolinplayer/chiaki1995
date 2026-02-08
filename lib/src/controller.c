// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

// Variáveis vindas da Interface
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0;

// Macros de Segurança para Input
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define MAX_ABS(x, y) ((ABS(x) > ABS(y)) ? (x) : (y))
#define CLAMP(x) ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;

    // 1. PASSO FUNDAMENTAL: Juntar Inputs A (Teclado) e B (Controle)
    // Se isso não for feito direito, os botões não funcionam.
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state); // Mira
    out->r2_state = MAX(a->r2_state, b->r2_state); // Tiro
    
    // Analógicos: Pega quem estiver movendo mais
    int32_t lx = MAX_ABS(a->left_x, b->left_x);
    int32_t ly = MAX_ABS(a->left_y, b->left_y);
    int32_t rx = MAX_ABS(a->right_x, b->right_x);
    int32_t ry = MAX_ABS(a->right_y, b->right_y);

    // 2. DETECÇÃO DE TIRO SEGURADO (Para o Recoil)
    if (out->r2_state > 30) {
        fire_duration++;
    } else {
        fire_duration = 0;
    }

    // 3. APLICAÇÃO DE RECOIL SUAVE (Sem travar botões)
    // Só mexe no eixo RY (Vertical) e RX (Horizontal) se o tempo de disparo permitir
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t pull_v = 0;
        int32_t pull_h = 0;

        if (ms <= 300) {
            pull_v = v_stage1; pull_h = h_stage1;
        } else if (ms <= 800) {
            pull_v = v_stage2; pull_h = h_stage2;
        } else {
            // Estágio final suavizado
            float factor = (float)lock_power_global / 100.0f;
            pull_v = (int32_t)(v_stage3 * factor);
            pull_h = (int32_t)(h_stage3 * factor);
        }

        // SOMA o recoil ao movimento do mouse, não substitui
        ry += (pull_v * 160); 
        rx += (pull_h * 130);
    }

    // 4. MAGNETISMO (Sem Jitter agressivo)
    // Apenas uma leve atração se ativado, sem tremedeira
    if (sticky_aim_global && out->l2_state > 30) {
        float rad = (float)zen_tick * 0.15f; // Rotação lenta
        rx += (int32_t)(cosf(rad) * sticky_power_global);
        ry += (int32_t)(sinf(rad) * sticky_power_global * 0.5f);
    }

    // 5. SAÍDA FINAL COM PROTEÇÃO
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Repasse do Touchpad (Obrigatório)
    for(size_t i=0; i<CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}

// Funções de Sistema (Não mexer, garantem que o Chiaki não feche)
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons = 0; s->l2_state=0; s->r2_state=0; 
    s->left_x=0; s->left_y=0; s->right_x=0; s->right_y=0; 
}
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state); 
}
