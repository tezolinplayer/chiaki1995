// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/controller.h>
#include <stdint.h>
#include <math.h>

#define TOUCH_ID_MASK 0x7f

// ------------------------------------------------------------------
// LINK COM O C++ (DANIEL MOD v4.0)
// ------------------------------------------------------------------
extern int recoil_v_global;
extern int recoil_h_global;
extern int anti_dz_global;
extern int sticky_power_global;
extern bool sticky_aim_global;
extern bool rapid_fire_global;
extern bool crouch_spam_global; // Macro: Agachar/Levantar
extern bool drop_shot_global;   // Macro: Deitar ao atirar

static uint32_t zen_tick = 0;
static uint32_t fire_duration = 0; // Contador de tempo de disparo contínuo

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *state)
{
	state->buttons = 0;
	state->l2_state = 0;
	state->r2_state = 0;
	state->left_x = 0;
	state->left_y = 0;
	state->right_x = 0;
	state->right_y = 0;
	state->touch_id_next = 0;
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		state->touches[i].id = -1;
		state->touches[i].x = 0;
		state->touches[i].y = 0;
	}
	state->gyro_x = state->gyro_y = state->gyro_z = 0.0f;
	state->accel_x = 0.0f;
	state->accel_y = 1.0f;
	state->accel_z = 0.0f;
	state->orient_x = 0.0f;
	state->orient_y = 0.0f;
	state->orient_z = 0.0f;
	state->orient_w = 1.0f;
}

CHIAKI_EXPORT int8_t chiaki_controller_state_start_touch(ChiakiControllerState *state, uint16_t x, uint16_t y)
{
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		if(state->touches[i].id < 0)
		{
			state->touches[i].id = state->touch_id_next;
			state->touch_id_next = (state->touch_id_next + 1) & TOUCH_ID_MASK;
			state->touches[i].x = x;
			state->touches[i].y = y;
			return state->touches[i].id;
		}
	}
	return -1;
}

CHIAKI_EXPORT void chiaki_controller_state_stop_touch(ChiakiControllerState *state, uint8_t id)
{
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		if(state->touches[i].id == id)
		{
			state->touches[i].id = -1;
			break;
		}
	}
}

CHIAKI_EXPORT void chiaki_controller_state_set_touch_pos(ChiakiControllerState *state, uint8_t id, uint16_t x, uint16_t y)
{
	id &= TOUCH_ID_MASK;
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		if(state->touches[i].id == id)
		{
			state->touches[i].x = x;
			state->touches[i].y = y;
			break;
		}
	}
}

CHIAKI_EXPORT bool chiaki_controller_state_equals(ChiakiControllerState *a, ChiakiControllerState *b)
{
	if(!(a->buttons == b->buttons
		&& a->l2_state == b->l2_state
		&& a->r2_state == b->r2_state
		&& a->left_x == b->left_x
		&& a->left_y == b->left_y
		&& a->right_x == b->right_x
		&& a->right_y == b->right_y))
		return false;

	for(size_t i=0; i<CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		if(a->touches[i].id != b->touches[i].id)
			return false;
		if(a->touches[i].id >= 0 && (a->touches[i].x != b->touches[i].x || a->touches[i].y != b->touches[i].y))
			return false;
	}

#define CHECKF(n) if(a->n < b->n - 0.0000001f || a->n > b->n + 0.0000001f) return false
	CHECKF(gyro_x);
	CHECKF(gyro_y);
	CHECKF(gyro_z);
	CHECKF(accel_x);
	CHECKF(accel_y);
	CHECKF(accel_z);
	CHECKF(orient_x);
	CHECKF(orient_y);
	CHECKF(orient_z);
	CHECKF(orient_w);
#undef CHECKF

	return true;
}

#define MAX(a, b)	  ((a) > (b) ? (a) : (b))
#define ABS(a)		  ((a) > 0 ? (a) : -(a))
#define MAX_ABS(a, b) (ABS(a) > ABS(b) ? (a) : (b))

// ------------------------------------------------------------------
// MOTOR DE PROCESSAMENTO ELITE (DANIEL MOD v4.0)
// ------------------------------------------------------------------
CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b)
{
	zen_tick++;

	out->buttons = a->buttons | b->buttons;
	out->l2_state = MAX(a->l2_state, b->l2_state);
	out->r2_state = MAX(a->r2_state, b->r2_state);
	out->left_x = MAX_ABS(a->left_x, b->left_x);
	out->left_y = MAX_ABS(a->left_y, b->left_y);

	// Controle de Tempo de Disparo (Para Curva e Macros)
	if (out->r2_state > 40) {
		fire_duration++;
	} else {
		fire_duration = 0;
	}

	// 1. MACRO: DROP SHOT (Deita instantaneamente ao começar a atirar)
	if (drop_shot_global && fire_duration > 0 && fire_duration < 10) {
		out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE; // Segura Círculo/B
	}

	// 2. MACRO: CROUCH SPAM (Fica agachando e levantando enquanto atira)
	if (crouch_spam_global && fire_duration > 0) {
		// Alterna a cada 15 ticks (aproximadamente 200ms)
		if ((fire_duration / 15) % 2 == 0) {
			out->buttons |= CHIAKI_CONTROLLER_BUTTON_CIRCLE;
		}
	}

	// 3. RAPID FIRE (Com janela de tempo para o console registrar)
	if (rapid_fire_global && out->r2_state > 40) {
		if ((zen_tick % 10) >= 5) {
			out->r2_state = 0;
			out->buttons &= ~CHIAKI_CONTROLLER_BUTTON_R2;
		}
	}

	int32_t rx = (int32_t)MAX_ABS(a->right_x, b->right_x);
	int32_t ry = (int32_t)MAX_ABS(a->right_y, b->right_y);

	// 4. STICKY AIM (Magnetismo com ajuste de força via interface)
	if (sticky_aim_global && out->r2_state > 30) {
		float angle = (float)zen_tick * 0.6f;
		rx += (int32_t)(cosf(angle) * (float)sticky_power_global);
		ry += (int32_t)(sinf(angle) * (float)sticky_power_global);
	}

	// 5. ANTI-DEADZONE (Resposta instantânea da mira)
	if (rx > 500) rx += anti_dz_global; else if (rx < -500) rx -= anti_dz_global;
	if (ry > 500) ry += anti_dz_global; else if (ry < -500) ry -= anti_dz_global;

	// 6. RECOIL ADAPTATIVO (SPRAY CURVE)
	if (out->r2_state > 40) {
		float curve_multiplier = 1.0f;

		// Estágios da Curva (Início suave, Final agressivo)
		if (fire_duration > 50) {
			curve_multiplier = 1.6f; // Aumenta 60% a força no final do pente
			rx += (int32_t)(recoil_h_global * 40); // Compensação horizontal extra
		} else if (fire_duration > 15) {
			curve_multiplier = 1.3f; // Aumenta 30% após os primeiros tiros
		}

		ry += (int32_t)(recoil_v_global * 150 * curve_multiplier);
		rx += (int32_t)(recoil_h_global * 150);
	}

	// LIMITAÇÃO DE SEGURANÇA (CLAMPING)
	if (ry > 32767) ry = 32767;
	if (ry < -32768) ry = -32768;
	if (rx > 32767) rx = 32767;
	if (rx < -32768) rx = -32768;

	out->right_x = (int16_t)rx;
	out->right_y = (int16_t)ry;

	// Processamento do Touchpad
	out->touch_id_next = 0;
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		ChiakiControllerTouch *touch = a->touches[i].id >= 0 ? &a->touches[i] : (b->touches[i].id >= 0 ? &b->touches[i] : NULL);
		if(!touch)
		{
			out->touches[i].id = -1;
			out->touches[i].x = out->touches[i].y = 0;
			continue;
		}
		if(touch != &out->touches[i])
			out->touches[i] = *touch;
	}
}
