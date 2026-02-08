// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

// --- VARIÁVEIS EXTERN DO PAINEL (LINKER PRECISA DISSO) ---
extern int v_stage1, h_stage1;
extern int v_stage2, h_stage2;
extern int v_stage3, h_stage3;
extern int anti_dz_global, sticky_power_global, lock_power_global, start_delay_global;
extern bool sticky_aim_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b) (ABS(a) > ABS(b) ? (a) : (b))
#define CLAMP(v) (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    zen_tick++;
    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;

    // 1. MOVIMENTO DO PERSONAGEM (ANALÓGICO ESQUERDO)
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // 2. DISPARO SEGURADO (R2)
    if (out->r2_state > 40) {
        fire_duration++; // Acumula tempo enquanto segura
    } else {
        fire_duration = 0; // Zera na hora que solta
    }

    // 3. LOGICA DE 3 ESTÁGIOS (SMART ACTIONS)
    if (fire_duration > (uint32_t)start_delay_global) {
        uint32_t ms = fire_duration * 10;
        int32_t t_v = 0, t_h = 0;

        if (ms <= 300) { 
            // Estágio 1: Kick inicial
            t_v = v_stage1; t_h = h_stage1; 
        } 
        else if (ms <= 800) { 
            // Estágio 2: Transição
            t_v = v_stage2; t_h = h_stage2; 
        } 
        else { 
            // Estágio 3: Estabilização Final (Usa Lock Power)
            float mod = (float)lock_power_global / 100.0f;
            t_v = (int32_t)(v_stage3 * mod); 
            t_h = (int32_t)(h_stage3 * mod); 
        }

        // Aplica força (RY positivo desce a mira)
        ry += (t_v * 170); 
        rx += (t_h * 140);
    }

    // 4. STICKY AIM (MAGNETISMO)
    if (sticky_aim_global && out->l2_state > 30) {
        float angle = (float)zen_tick * 0.30f;
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    out->left_x = (int16_t)CLAMP(lx); out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx); out->right_y = (int16_t)CLAMP(ry);
}

// Funções obrigatórias para o Linker não reclamar
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { s->buttons = 0; s->left_x = 0; s->left_y = 0; s->right_x = 0; s->right_y = 0; }
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { return a->buttons == b->buttons; }
