// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <chiaki/controller.h>
#include <stdint.h>
#include <stdlib.h>

// Macros básicas para evitar erros de compilação
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define ABS(x) (((x) < 0) ? -(x) : (x))

CHIAKI_EXPORT void chiaki_controller_state_or(ChiakiControllerState *out, ChiakiControllerState *a, ChiakiControllerState *b) {
    
    // 1. Unifica os Botões e Gatilhos (L2/R2)
    out->buttons = a->buttons | b->buttons;
    out->l2_state = MAX(a->l2_state, b->l2_state);
    out->r2_state = MAX(a->r2_state, b->r2_state);
    
    // 2. Analógico ESQUERDO (Movimento) - Pega o maior input entre A e B
    out->left_x = (ABS(a->left_x) > ABS(b->left_x)) ? a->left_x : b->left_x;
    out->left_y = (ABS(a->left_y) > ABS(b->left_y)) ? a->left_y : b->left_y;
    
    // 3. Analógico DIREITO (Mira) - Pega o maior input entre A e B (SEM MODIFICAÇÕES)
    out->right_x = (ABS(a->right_x) > ABS(b->right_x)) ? a->right_x : b->right_x;
    out->right_y = (ABS(a->right_y) > ABS(b->right_y)) ? a->right_y : b->right_y;
    
    // 4. Touchpad
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) {
        if (a->touches[i].id >= 0) out->touches[i] = a->touches[i];
        else if (b->touches[i].id >= 0) out->touches[i] = b->touches[i];
        else out->touches[i].id = -1;
    }
}

CHIAKI_EXPORT void chiaki_controller_state_set_idle(ChiakiControllerState *s) { 
    s->buttons = 0; s->l2_state = 0; s->r2_state = 0; 
    s->left_x = 0; s->left_y = 0; s->right_x = 0; s->right_y = 0; 
    for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++) s->touches[i].id = -1;
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
