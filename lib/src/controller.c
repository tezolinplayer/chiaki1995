// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// DANIEL MOD v4.8: MAGNETISMO ELÍPTICO + STATIC LOCK
// ------------------------------------------------------------------
#ifndef CHIAKI_CONTROLLER_BUTTON_CIRCLE
#define CHIAKI_CONTROLLER_BUTTON_CIRCLE 0x0004 
#endif

#ifndef CHIAKI_CONTROLLER_BUTTON_R2
#define CHIAKI_CONTROLLER_BUTTON_R2     0x0080 
#endif

// Link com as variáveis globais do C++
extern int recoil_v_global;
extern int recoil_h_global;
extern int anti_dz_global;
extern int sticky_power_global;
extern bool sticky_aim_global;
extern bool rapid_fire_global;
extern bool crouch_spam_global; 
extern bool drop_shot_global;   

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; 

// ... (Funções chiaki_controller_state_set_idle, start_touch, etc. permanecem iguais) ...

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
	zen_tick++;

	out->buttons = a->buttons | b->buttons;
	out->l2_state = MAX(a->l2_state, b->l2_state);
	out->r2_state = MAX(a->r2_state, b->r2_state);
	out->left_x = MAX_ABS(a->left_x, b->left_x);
	out->left_y = MAX_ABS(a->left_y, b->left_y);

	// Gerenciamento de Disparo
	if (out->r2_state > 40) {
		fire_duration++;
	} else {
		fire_duration = 0;
	}

	// 1. MACROS DE MOVIMENTAÇÃO (ELITE v4.8)
	if (drop_shot_global && fire_duration > 0 && fire_duration < 10) {
		out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; 
	}
	if (crouch_spam_global && fire_duration > 0) {
		if ((fire_duration / 15) % 2 == 0) {
			out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
		}
	}

	// 2. RAPID FIRE
	if (rapid_fire_global && out->r2_state > 40) {
		if ((zen_tick % 10) >= 5) {
			out->r2_state = 0;
			out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2; 
		}
	}

	int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
	int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

	// 3. MAGNETISMO ELÍPTICO (TRACKING PRO - PUBG FOCUS)
	if (sticky_aim_global && (out->l2_state > 30 || out->r2_state > 30)) {
		// Frequência otimizada para os 14 núcleos do seu Xeon
		float angle = (float)zen_tick * 0.12f; 
		float power = (float)sticky_power_global; 

		// ELIPSE: 1.4x Horizontal para "grudar" em inimigos correndo
		// 0.7x Vertical para não interferir na sua trava de recuo
		int32_t sticky_x = (int32_t)(cosf(angle) * power * 1.4f);
		int32_t sticky_y = (int32_t)(sinf(angle) * power * 0.7f);

		rx += sticky_x;
		ry += sticky_y;
		
		// Anti-Friction horizontal para vencer a zona morta
		if (zen_tick % 2 == 0) rx += 5; else rx -= 5;
	}

    // ... (Continua com Anti-Deadzone e Recuo Estático) ...
