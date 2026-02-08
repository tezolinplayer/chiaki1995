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
#define CLAMP(x) ((x) > 32767 ? 32767 : ((x) < -32768 ? -32768 : (x)))

// Função auxiliar para escolher o analógico com maior magnitude
static int16_t select_stick_input(int16_t a_val, int16_t b_val) {
    if (ABS(a_val) > ABS(b_val)) {
        return a_val;
    }
    return b_val;
}

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;
    
    // 1. Combinar botões (teclado + controle)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // 2. Analógicos BÁSICOS - pega o input com maior movimento
    int16_t base_lx = select_stick_input(a->left_x, b->left_x);
    int16_t base_ly = select_stick_input(a->left_y, b->left_y);
    int16_t base_rx = select_stick_input(a->right_x, b->right_x);
    int16_t base_ry = select_stick_input(a->right_y, b->right_y);
    
    // 3. Converter para int32 para cálculos (evitar overflow)
    int32_t lx = base_lx;
    int32_t ly = base_ly;
    int32_t rx = base_rx;
    int32_t ry = base_ry;
    
    // 4. DETECÇÃO DE TIRO SEGURADO
    if (out->r2_state > 30) {
        fire_duration++;
    } else {
        fire_duration = 0;
    }
    
    // 5. APLICAÇÃO DE RECOIL (apenas se estiver atirando)
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t pull_v = 0;
        int32_t pull_h = 0;
        
        if (ms <= 300) {
            pull_v = v_stage1; 
            pull_h = h_stage1;
        } else if (ms <= 800) {
            pull_v = v_stage2; 
            pull_h = h_stage2;
        } else {
            float factor = (float)lock_power_global / 100.0f;
            pull_v = (int32_t)(v_stage3 * factor);
            pull_h = (int32_t)(h_stage3 * factor);
        }
        
        // Aplicar recoil SOMENTE no analógico direito (mira)
        ry += pull_v * 8;  // Reduzido de 160 para 8
        rx += pull_h * 6;  // Reduzido de 130 para 6
    }
    
    // 6. MAGNETISMO (suave, sem jitter)
    if (sticky_aim_global && out->l2_state > 30) {
        float rad = (float)zen_tick * 0.08f;
        int32_t mag_x = (int32_t)(cosf(rad) * sticky_power_global * 0.3f);
        int32_t mag_y = (int32_t)(sinf(rad) * sticky_power_global * 0.15f);
        
        rx += mag_x;
        ry += mag_y;
    }
    
    // 7. SAÍDA FINAL com clamp
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);
    
    // 8. Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) {
            out->touches[i] = a->touches[i];
        } else if (b->touches[i].id >= 0) {
            out->touches[i] = b->touches[i];
        } else {
            out->touches[i].id = -1;
        }
    }
}

// Funções de Sistema
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons = 0; 
    s->l2_state = 0; 
    s->r2_state = 0; 
    s->left_x = 0; 
    s->left_y = 0; 
    s->right_x = 0; 
    s->right_y = 0;
    
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        s->touches[i].id = -1;
    }
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { 
    return (a->buttons == b->buttons && 
            a->l2_state == b->l2_state && 
            a->r2_state == b->r2_state &&
            a->left_x == b->left_x &&
            a->left_y == b->left_y &&
            a->right_x == b->right_x &&
            a->right_y == b->right_y); 
}
