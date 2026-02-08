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

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // Libera analógico esquerdo (Personagem anda)
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    
    // Mira
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- CORREÇÃO DO DISPARO SEGURADO ---
    if (out->r2_state > 40) { 
        fire_duration++; // Enquanto segurar, o tempo aumenta e o recoil atua
    } else { 
        fire_duration = 0; // Soltou, para NA HORA
    }

    // Só aplica o No-Recoil se estiver SEGURANDO o gatilho
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; 
        int32_t recoil_v = 0;
        int32_t recoil_h = 0;

        // Estágios de força para segurar o spray
        if (ms <= 400) { 
            recoil_v = v_stage1; 
            recoil_h = h_stage1; 
        } 
        else if (ms <= 1000) { 
            recoil_v = v_stage2; 
            recoil_h = h_stage2; 
        } 
        else {
            // Travamento final do spray (Lock Power)
            float modifier = (float)lock_power_global / 100.0f;
            recoil_v = (int32_t)(v_stage3 * modifier);
            recoil_h = (int32_t)(h_stage3 * modifier);
        }

        // Aplica a descida: Aumentamos o multiplicador para garantir que a mira desça
        ry += (recoil_v * 160); 
        rx += (recoil_h * 160);
    }

    // Sticky Aim (Magnetismo) sem travar o movimento
    if (sticky_aim_global && (out->l2_state > 30)) {
        lx += (zen_tick % 4 < 2) ? 600 : -600; 
        float angle = (float)zen_tick * 0.28f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.4f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    // Atribuição final
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);
}
