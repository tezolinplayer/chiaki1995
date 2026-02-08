// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

// --- LINKER BRIDGE: CONEXÃO COM AS VARIÁVEIS DO CONTROLLER.C ---
// O extern "C" aqui é obrigatório para o C++ conversar com o C puro
extern "C" {
    extern int v_stage1, h_stage1;
    extern int v_stage2, h_stage2;
    extern int v_stage3, h_stage3;
    extern int anti_dz_global;
    extern int sticky_power_global;
    extern int lock_power_global;
    extern int start_delay_global;
    extern bool sticky_aim_global;
    extern bool rapid_fire_global;
    extern bool crouch_spam_global;
    extern bool drop_shot_global;
}

// Inicializando os valores padrão (evita valores lixo na memória)
int v_stage1 = 0, h_stage1 = 0;
int v_stage2 = 0, h_stage2 = 0;
int v_stage3 = 0, h_stage3 = 0;
int anti_dz_global = 0;
int sticky_power_global = 750;
int lock_power_global = 100;
int start_delay_global = 2;
bool sticky_aim_global = false;
bool rapid_fire_global = false;
bool crouch_spam_global = false;
bool drop_shot_global = false;

void StreamWindow::Init() {
    // Configuração visual Daniel Ghost
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // --- PAINEL SMART ACTIONS (XIM MATRIX STYLE) ---
    QGroupBox *ximGroup = new QGroupBox("SMART ACTIONS - RECOIL DINÂMICO", this);
    ximGroup->setStyleSheet("border: 1px solid #FFD700; color: #FFD700; padding: 10px;");
    QVBoxLayout *xLayout = new QVBoxLayout(ximGroup);

    // Gerador automático de sliders para os estágios
    auto addStageControl = [&](QString labelText, int *v_var, int *h_var) {
        xLayout->addWidget(new QLabel(labelText, this));
        QHBoxLayout *hBox = new QHBoxLayout();
        
        QSlider *sv = new QSlider(Qt::Horizontal, this);
        sv->setRange(0, 150);
        sv->setValue(*v_var);
        connect(sv, &QSlider::valueChanged, [v_var](int val){ *v_var = val; });
        
        QSlider *sh = new QSlider(Qt::Horizontal, this);
        sh->setRange(-100, 100);
        sh->setValue(*h_var);
        connect(sh, &QSlider::valueChanged, [h_var](int val){ *h_var = val; });

        hBox->addWidget(new QLabel("V:", this)); hBox->addWidget(sv);
        hBox->addWidget(new QLabel("H:", this)); hBox->addWidget(sh);
        xLayout->addLayout(hBox);
    };

    addStageControl("ESTÁGIO 1: KICK INICIAL (0-300ms)", &v_stage1, &h_stage1);
    addStageControl("ESTÁGIO 2: TRANSIÇÃO (300-800ms)", &v_stage2, &h_stage2);
    addStageControl("ESTÁGIO 3: FINAL (800ms+)", &v_stage3, &h_stage3);

    mainLayout->addWidget(ximGroup);

    // --- CONFIGURAÇÕES DE PRECISÃO ---
    QGroupBox *globalGroup = new QGroupBox("AJUSTES DE PRECISÃO", this);
    globalGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *gLayout = new QVBoxLayout(globalGroup);

    QLabel *label_dz = new QLabel(QString("Anti-Deadzone: %1").arg(anti_dz_global), this);
    QSlider *slider_dz = new QSlider(Qt::Horizontal, this);
    slider_dz->setRange(0, 5000);
    slider_dz->setValue(anti_dz_global);
    connect(slider_dz, &QSlider::valueChanged, [label_dz](int val){ 
        anti_dz_global = val; 
        label_dz->setText(QString("Anti-Deadzone: %1").arg(val)); 
    });
    
    gLayout->addWidget(label_dz);
    gLayout->addWidget(slider_dz);
    mainLayout->addWidget(globalGroup);

    // --- BOTÕES DE ATIVAÇÃO RÁPIDA ---
    QHBoxLayout *checkLayout = new QHBoxLayout();
    QCheckBox *cb_sticky = new QCheckBox("STICKY AIM", this);
    QCheckBox *cb_rapid = new QCheckBox("RAPID FIRE", this);
    cb_sticky->setChecked(sticky_aim_global);
    cb_rapid->setChecked(rapid_fire_global);

    connect(cb_sticky, &QCheckBox::toggled, [](bool chk){ sticky_aim_global = chk; });
    connect(cb_rapid, &QCheckBox::toggled, [](bool chk){ rapid_fire_global = chk; });
    
    checkLayout->addWidget(cb_sticky);
    checkLayout->addWidget(cb_rapid);
    mainLayout->addLayout(checkLayout);

    setCentralWidget(central);
    resize(540, 920); 
    show();
}
