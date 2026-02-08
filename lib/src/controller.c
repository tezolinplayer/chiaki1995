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

// Macros
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define CLAMP(x) ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;
    
    // 1. Inputs Básicos (Botões e Gatilhos)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // 2. Analógico ESQUERDO - Soma direto (movimento)
    int32_t lx = (int32_t)a->left_x + (int32_t)b->left_x;
    int32_t ly = (int32_t)a->left_y + (int32_t)b->left_y;
    
    // 3. Analógico DIREITO - Pega o que tiver maior movimento (mira)
    int32_t rx, ry;
    
    if (ABS(a->right_x) > ABS(b->right_x)) {
        rx = (int32_t)a->right_x;
    } else {
        rx = (int32_t)b->right_x;
    }
    
    if (ABS(a->right_y) > ABS(b->right_y)) {
        ry = (int32_t)a->right_y;
    } else {
        ry = (int32_t)b->right_y;
    }
    
    // 4. Detecção de Tiro
    if (out->r2_state > 30) {
        fire_duration++;
    } else {
        fire_duration = 0;
    }
    
    // 5. RECOIL (apenas no analógico direito)
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t pull_v = 0;
        int32_t pull_h = 0;
        
        if (ms <= 300) {
            pull_v = v_stage1; pull_h = h_stage1;
        } else if (ms <= 800) {
            pull_v = v_stage2; pull_h = h_stage2;
        } else {
            float factor = (float)lock_power_global / 100.0f;
            pull_v = (int32_t)(v_stage3 * factor);
            pull_h = (int32_t)(h_stage3 * factor);
        }
        
        ry += (pull_v * 160); 
        rx += (pull_h * 140);
    }
    
    // 6. Magnetismo Suave
    if (sticky_aim_global && out->l2_state > 30) {
        float rad = (float)zen_tick * 0.12f;
        rx += (int32_t)(cosf(rad) * sticky_power_global * 0.5f);
        ry += (int32_t)(sinf(rad) * sticky_power_global * 0.2f);
    }
    
    // 7. Saída Final
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);
    
    // Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}

// Funções Obrigatórias do Sistema
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons=0; s->l2_state=0; s->r2_state=0; 
    s->left_x=0; s->left_y=0; s->right_x=0; s->right_y=0; 
    for(size_t i=0; i<CHIAKI_CONTROLLER_TOUCHES_MAX; i++) s->touches[i].id = -1;
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return (a->buttons == b->buttons && a->l2_state == b->l2_state && a->r2_state == b->r2_state); 
}
