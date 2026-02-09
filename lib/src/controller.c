// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>

// Variáveis vindas da interface da imagem
extern int v_stage1, h_stage1, v_stage2, h_stage2, v_stage3, h_stage3;
extern int sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define CLAMP(x) ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;

    // 1. Unificação de Inputs (Botões e Gatilhos)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);

    // 2. Processamento dos Analógicos (Captura o valor real primeiro)
    int32_t lx = (ABS(a->left_x) > ABS(b->left_x)) ? a->left_x : b->left_x;
    int32_t ly = (ABS(a->left_y) > ABS(b->left_y)) ? a->left_y : b->left_y;
    int32_t rx = (ABS(a->right_x) > ABS(b->right_x)) ? a->right_x : b->right_x;
    int32_t ry = (ABS(a->right_y) > ABS(b->right_y)) ? a->right_y : b->right_y;

    // 3. Temporizador de Disparo (R2)
    if (out->r2_state > 40) { // Deadzone leve para evitar disparo acidental
        fire_duration++;
    } else {
        fire_duration = 0;
    }

    // 4. Lógica de ANTI-RECOIL (Baseada nos 3 Estágios da imagem)
    if (fire_duration > (uint32_t)start_delay_global) {
        int32_t pull_v = 0;
        int32_t pull_h = 0;
        uint32_t current_ms = fire_duration * 10; // Aproximação de tempo

        if (current_ms <= 300) {
            // Estágio 1 (0-300ms)
            pull_v = v_stage1;
            pull_h = h_stage1;
        } else if (current_ms <= 800) {
            // Estágio 2 (300-800ms)
            pull_v = v_stage2;
            pull_h = h_stage2;
        } else {
            // Estágio 3 (Lock Final +800ms)
            float factor = (float)lock_power_global / 100.0f;
            pull_v = (int32_t)(v_stage3 * factor);
            pull_h = (int32_t)(h_stage3 * factor);
        }

        // Aplicando a compensação (Multiplicadores ajustados para sensibilidade do PS5)
        ry += (pull_v * 150); 
        rx += (pull_h * 150);
    }

    // 5. Lógica de STICKY AIM (Magnetismo ao mirar L2)
    if (sticky_aim_global && out->l2_state > 40) {
        float rad = (float)zen_tick * 0.15f;
        // Cria um micro-movimento circular para "acordar" o Aim Assist do jogo
        rx += (int32_t)(cosf(rad) * (float)sticky_power_global * 0.4f);
        ry += (int32_t)(sinf(rad) * (float)sticky_power_global * 0.4f);
    }

    // 6. Saída Final com Travamento de Limites (Evita que o analógico "trave" no canto)
    out->left_x = (int16_t)lx;
    out->left_y = (int16_t)ly;
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // 7. Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons=0; s->l2_state=0; s->r2_state=0; 
    s->left_x=0; s->left_y=0; s->right_x=0; s->right_y=0; 
    for(size_t i=0; i<CHIAKI_CONTROLLER_TOUCHES_MAX; i++) s->touches[i].id = -1;
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state); 
}
