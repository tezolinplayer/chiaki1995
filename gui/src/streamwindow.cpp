// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <streamsession.h>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

// --- LINKER BRIDGE: CONEXÃO COM AS VARIÁVEIS DO CONTROLLER.C ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 750;
    int lock_power_global = 160;
    int start_delay_global = 2;
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
    bool crouch_spam_global = false;
    bool drop_shot_global = false;
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // --- 1. SELEÇÃO DE ARMA (PERFIS) ---
    QGroupBox *profileGroup = new QGroupBox("SELEÇÃO DE ARMA", this);
    profileGroup->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *pLayout = new QHBoxLayout();
    QComboBox *combo_profiles = new QComboBox(this);
    combo_profiles->addItems({"M416", "BERYL", "MINI-14", "SKS", "GENERIC"});
    combo_profiles->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btn_save = new QPushButton("SALVAR", this);
    btn_save->setStyleSheet("background-color: #003300; border: 1px solid #00FF41;");
    pLayout->addWidget(combo_profiles); pLayout->addWidget(btn_save);
    profileGroup->setLayout(pLayout);
    mainLayout->addWidget(profileGroup);

    // --- 2. SMART ACTIONS (3 ESTÁGIOS - ESCALA 0 A 100) ---
    QGroupBox *ximGroup = new QGroupBox("SMART ACTIONS - RECOIL DINÂMICO", this);
    ximGroup->setStyleSheet("border: 1px solid #FFD700; color: #FFD700; padding: 5px;");
    QVBoxLayout *xLayout = new QVBoxLayout(ximGroup);

    auto addStage = [&](QString txt, int *v, int *h) {
        xLayout->addWidget(new QLabel(txt, this));
        QHBoxLayout *hBox = new QHBoxLayout();
        
        QSlider *sv = new QSlider(Qt::Horizontal, this);
        sv->setRange(0, 100); // ESCALA 0-100 CONFORME PEDIDO
        connect(sv, &QSlider::valueChanged, [v](int val){ *v = val; });
        
        QSlider *sh = new QSlider(Qt::Horizontal, this);
        sh->setRange(-100, 100);
        connect(sh, &QSlider::valueChanged, [h](int val){ *h = val; });

        hBox->addWidget(new QLabel("V:", this)); hBox->addWidget(sv);
        hBox->addWidget(new QLabel("H:", this)); hBox->addWidget(sh);
        xLayout->addLayout(hBox);
    };

    addStage("ESTÁGIO 1: KICK (0-300ms)", &v_stage1, &h_stage1);
    addStage("ESTÁGIO 2: TRANSIÇÃO (300-800ms)", &v_stage2, &h_stage2);
    addStage("ESTÁGIO 3: FINAL (800ms+)", &v_stage3, &h_stage3);
    mainLayout->addWidget(ximGroup);

    // --- 3. AJUSTES DE PRECISÃO GLOBAIS ---
    QGroupBox *globalGroup = new QGroupBox("AJUSTES DE PRECISÃO GLOBAIS", this);
    globalGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *gLayout = new QVBoxLayout(globalGroup);

    auto addGlobal = [&](QString labelText, int min, int max, int *var, bool isFloat = false) {
        QLabel *label = new QLabel(labelText + QString(": %1").arg(isFloat ? *var/100.0 : *var), this);
        QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(min, max);
        slider->setValue(*var);
        connect(slider, &QSlider::valueChanged, [=](int val) mutable {
            *var = val;
            label->setText(labelText + QString(": %1").arg(isFloat ? val/100.0 : (double)val));
        });
        gLayout->addWidget(label); gLayout->addWidget(slider);
    };

    addGlobal("Lock Power (Trava)", 100, 250, &lock_power_global, true);
    addGlobal("Start Delay (Ticks)", 0, 15, &start_delay_global);
    addGlobal("Força Magnetismo", 0, 2000, &sticky_power_global);
    addGlobal("Anti-Deadzone", 0, 5000, &anti_dz_global);
    mainLayout->addWidget(globalGroup);

    // --- 4. MACROS & FUNÇÕES ---
    QGroupBox *macroGroup = new QGroupBox("MACROS", this);
    QGridLayout *mLayout = new QGridLayout(macroGroup);
    QCheckBox *cb_sticky = new QCheckBox("STICKY AIM", this);
    QCheckBox *cb_rapid = new QCheckBox("RAPID FIRE", this);
    connect(cb_sticky, &QCheckBox::toggled, [](bool chk){ sticky_aim_global = chk; });
    connect(cb_rapid, &QCheckBox::toggled, [](bool chk){ rapid_fire_global = chk; });
    mLayout->addWidget(cb_sticky, 0, 0); mLayout->addWidget(cb_rapid, 0, 1);
    mainLayout->addWidget(macroGroup);

    setCentralWidget(central);
    resize(540, 980);
    show();
    session->Start();
}

// ... (Mantenha as funções de evento SessionQuit, keyPressEvent, etc., no final do arquivo)
