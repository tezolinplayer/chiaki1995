// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// MÁSCARAS DE BITS E DEFINIÇÕES
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
#define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif

#ifndef CHIAKI_CONTROLLER_BUTTON_R2
#define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// ------------------------------------------------------------------
// LINKER BRIDGE - SISTEMA SMART ACTIONS (3 ESTÁGIOS)
// ------------------------------------------------------------------
extern "C" {
    // Estágios de Recoil (V = Vertical, H = Horizontal)
    int v_stage1 = 0, h_stage1 = 0; // 0-300ms (Kick Inicial)
    int v_stage2 = 0, h_stage2 = 0; // 300-800ms (Transição)
    int v_stage3 = 0, h_stage3 = 0; // 800ms+ (Estabilização Final)

    int anti_dz_global;       
    int sticky_power_global;  
    int lock_power_global;    // Multiplicador de precisão final
    int start_delay_global;   
    bool sticky_aim_global;
    bool rapid_fire_global;
    bool crouch_spam_global; 
    bool drop_shot_global;    
}

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

// --- FUNÇÕES DE AUXÍLIO ---
#define MAX_ABS(a, b)  (abs(a) > abs(b) ? (a) : (b))
#define CLAMP(v)       (v > 32767 ? 32767 : (v < -32768 ? -32768 : v))

// ------------------------------------------------------------------
// MOTOR DANIEL GHOST ELITE - SMART ACTIONS v5.5
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
    zen_tick++;

    // Sincronização básica de botões e gatilhos
    out->buttons = a->buttons | b->buttons;
    out->l2_state = (a->l2_state > b->l2_state) ? a->l2_state : b->l2_state;
    out->r2_state = (a->r2_state > b->r2_state) ? a->r2_state : b->r2_state;
    
    int32_t lx = (int32_t)MAX_ABS(a->left_x, b->left_x);
    int32_t ly = (int32_t)MAX_ABS(a->left_y, b->left_y);
    int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
    int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

    // Contador de tempo de disparo
    if (out->r2_state > 40) { fire_duration++; } else { fire_duration = 0; }

    // 1. SMART ACTIONS - RECOIL POR ETAPAS (XIM STYLE)
    if (fire_duration > (uint32_t)start_delay_global) { 
        uint32_t ms = fire_duration * 10; // Aproximação de ms (base 100Hz)
        int32_t target_v = 0;
        int32_t target_h = 0;

        if (ms <= 300) { 
            // ESTÁGIO 1: O estouro inicial da arma
            target_v = v_stage1; 
            target_h = h_stage1;
        } 
        else if (ms > 300 && ms <= 800) {
            // ESTÁGIO 2: A curva de aprendizado do spray
            target_v = v_stage2;
            target_h = h_stage2;
        } 
        else {
            // ESTÁGIO 3: Estabilização para pentes longos
            float modifier = (float)lock_power_global / 100.0f;
            target_v = (int32_t)(v_stage3 * modifier);
            target_h = (int32_t)(h_stage3 * modifier);
            
            // Micro-jitter vertical para manter o Aim Assist ativo no alvo
            if (zen_tick % 2 == 0) ry += 12; else ry -= 12;
        }

        // Aplicação final com ganho de escala
        ry += (target_v * 150);
        rx += (target_h * 120);
    }

    // 2. AIM ASSIST MAGNÉTICO & ROTACIONAL
    if (sticky_aim_global && (out->l2_state > 30)) {
        // Rotacional (Strafe Jitter)
        lx += (zen_tick % 4 < 2) ? 650 : -650; 

        // Magnético (Elíptico)
        float angle = (float)zen_tick * 0.28f; 
        rx += (int32_t)(cosf(angle) * sticky_power_global * 1.6f);
        ry += (int32_t)(sinf(angle) * sticky_power_global * 0.8f);
    }

    // 3. RAPID FIRE 
    if (rapid_fire_global && out->r2_state > 40) {
        if ((zen_tick % 10) >= 5) out->r2_state = 0;
    }

    // 4. MACROS DE MOVIMENTO
    if (drop_shot_global && fire_duration > 1 && fire_duration < 12) {
        out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
    }
    if (crouch_spam_global && fire_duration > 20) {
        if ((fire_duration / 15) % 2 == 0) out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
    }

    // 5. ANTI-DEADZONE
    if (abs(rx) > 50 && abs(rx) < 3000) rx += (rx > 0) ? anti_dz_global : -anti_dz_global;
    if (abs(ry) > 50 && abs(ry) < 3000) ry += (ry > 0) ? anti_dz_global : -anti_dz_global;

    // --- FINALIZAÇÃO ---
    out->left_x = (int16_t)CLAMP(lx);
    out->left_y = (int16_t)CLAMP(ly);
    out->right_x = (int16_t)CLAMP(rx);
    out->right_y = (int16_t)CLAMP(ry);

    // Preservação do Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}
