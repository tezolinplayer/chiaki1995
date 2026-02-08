// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

// Link das variáveis da interface
extern int recoil_v_global;      
extern int recoil_h_global;      
extern int anti_dz_global;       
extern int sticky_power_global;  
extern int lock_power_global;    
extern int start_delay_global;   
extern bool sticky_aim_global;
extern bool rapid_fire_global;

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

#define ABS(a)           ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b)  (ABS(a) > ABS(b) ? (a) : (b))
#define CLAMP(v)       (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    // Analógico esquerdo livre (Personagem anda normal)
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // --- LÓGICA DE DISPARO SEGURADO ---
    if (out->r2_state > 45) { 
        fire_duration++; // Só conta enquanto segurar o R2
    } else { 
        fire_duration = 0; // Soltou, o no-recoil para NA HORA
    }

    // Só aplica Recoil se estiver segurando o gatilho
    if (fire_duration > (uint32_t)start_delay_global) { 
        float modifier = (fire_duration > 40) ? ((float)lock_power_global / 100.0f) : 1.2f;
        
        // ry positivo empurra para BAIXO
        ry += (int32_t)(recoil_v_global * 165 * modifier);
        rx += (int32_t)(recoil_h_global * 140 * modifier);
    }

    // Aim Assist (Magnetismo)
    if (sticky_aim_global && (out->l2_state > 30)) {
        float angle = (float)zen_tick * 0.30f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.5f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.7f);
    }

    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);
}

// Funções obrigatórias para o Linker não dar erro
CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { s->buttons = 0; s->left_x = 0; s->left_y = 0; s->right_x = 0; s->right_y = 0; }
CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b) { return (a->right_y == b->right_y && a->r2_state == b->r2_state); }
