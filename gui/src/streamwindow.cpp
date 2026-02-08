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
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSettings>

// --- LINKER BRIDGE: CONEXÃO COM O CONTROLLER.C ---
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

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
    : QMainWindow(parent), connect_info(connect_info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | FULL SMART ACTIONS");
    session = new StreamSession(connect_info, this);
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) session->Stop();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // --- 1. SELEÇÃO DE ARMA E PERFIS (RESTAURADO DA FOTO) ---
    QGroupBox *profileGroup = new QGroupBox("SELEÇÃO DE ARMA", this);
    profileGroup->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *pLayout = new QHBoxLayout();
    QComboBox *combo_profiles = new QComboBox(this);
    combo_profiles->addItems({"M416", "BERYL", "MINI-14", "SKS", "GENERIC"});
    combo_profiles->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btn_save = new QPushButton("SALVAR", this);
    btn_save->setStyleSheet("background-color: #003300; border: 1px solid #00FF41;");
    pLayout->addWidget(combo_profiles);
    pLayout->addWidget(btn_save);
    profileGroup->setLayout(pLayout);
    mainLayout->addWidget(profileGroup);

    // --- 2. SMART ACTIONS - RECOIL DINÂMICO ---
    QGroupBox *ximGroup = new QGroupBox("SMART ACTIONS - RECOIL DINÂMICO", this);
    ximGroup->setStyleSheet("border: 1px solid #FFD700; color: #FFD700; padding: 5px;");
    QVBoxLayout *xLayout = new QVBoxLayout(ximGroup);

    auto addStage = [&](QString txt, int *v, int *h) {
        xLayout->addWidget(new QLabel(txt, this));
        QHBoxLayout *hBox = new QHBoxLayout();
        QSlider *sv = new QSlider(Qt::Horizontal, this);
        sv->setRange(0, 150);
        sv->setValue(*v);
        connect(sv, &QSlider::valueChanged, [v](int val){ *v = val; });
        QSlider *sh = new QSlider(Qt::Horizontal, this);
        sh->setRange(-100, 100);
        sh->setValue(*h);
        connect(sh, &QSlider::valueChanged, [h](int val){ *h = val; });
        hBox->addWidget(new QLabel("V:", this)); hBox->addWidget(sv);
        hBox->addWidget(new QLabel("H:", this)); hBox->addWidget(sh);
        xLayout->addLayout(hBox);
    };

    addStage("ESTÁGIO 1: KICK (0-300ms)", &v_stage1, &h_stage1);
    addStage("ESTÁGIO 2: TRANSIÇÃO (300-800ms)", &v_stage2, &h_stage2);
    addStage("ESTÁGIO 3: FINAL (800ms+)", &v_stage3, &h_stage3);
    mainLayout->addWidget(ximGroup);

    // --- 3. AJUSTES DE PRECISÃO (RESTAURADO DA FOTO) ---
    QGroupBox *globalGroup = new QGroupBox("AJUSTES DE PRECISÃO GLOBAIS", this);
    globalGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *gLayout = new QVBoxLayout(globalGroup);

    auto addGlobalSlider = [&](QString labelText, int min, int max, int def, int *var, bool isFloat = false) {
        QLabel *label = new QLabel(labelText + QString(": %1").arg(isFloat ? def/100.0 : def), this);
        QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(min, max);
        slider->setValue(def);
        connect(slider, &QSlider::valueChanged, [=](int val) mutable {
            *var = val;
            label->setText(labelText + QString(": %1").arg(isFloat ? val/100.0 : (double)val));
        });
        gLayout->addWidget(label); gLayout->addWidget(slider);
    };

    addGlobalSlider("Lock Power (Trava)", 100, 250, 160, &lock_power_global, true);
    addGlobalSlider("Start Delay (Ticks)", 0, 15, 2, &start_delay_global);
    addGlobalSlider("Força Magnetismo", 0, 2000, 750, &sticky_power_global);
    addGlobalSlider("Anti-Deadzone", 0, 5000, 0, &anti_dz_global);
    mainLayout->addWidget(globalGroup);

    // --- 4. MACROS & FUNÇÕES ---
    QGroupBox *macroGroup = new QGroupBox("MACROS & FUNÇÕES", this);
    QGridLayout *mLayout = new QGridLayout(macroGroup);
    QCheckBox *cb1 = new QCheckBox("CROUCH SPAM", this);
    QCheckBox *cb2 = new QCheckBox("DROP SHOT", this);
    QCheckBox *cb3 = new QCheckBox("STICKY AIM", this);
    QCheckBox *cb4 = new QCheckBox("RAPID FIRE", this);
    
    connect(cb1, &QCheckBox::toggled, [](bool chk){ crouch_spam_global = chk; });
    connect(cb2, &QCheckBox::toggled, [](bool chk){ drop_shot_global = chk; });
    connect(cb3, &QCheckBox::toggled, [](bool chk){ sticky_aim_global = chk; });
    connect(cb4, &QCheckBox::toggled, [](bool chk){ rapid_fire_global = chk; });

    mLayout->addWidget(cb1, 0, 0); mLayout->addWidget(cb2, 0, 1);
    mLayout->addWidget(cb3, 1, 0); mLayout->addWidget(cb4, 1, 1);
    mainLayout->addWidget(macroGroup);

    setCentralWidget(central);
    resize(540, 980);
    show();
    session->Start();
}

// --- FUNÇÕES DE SISTEMA ---
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
