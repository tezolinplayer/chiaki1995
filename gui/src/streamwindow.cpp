// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <streamsession.h>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSettings>

extern "C" {
    int v_stage1 = 0, h_stage1 = 0, v_stage2 = 0, h_stage2 = 0, v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0, sticky_power_global = 750, lock_power_global = 100, start_delay_global = 2;
    bool sticky_aim_global = false, rapid_fire_global = false, crouch_spam_global = false, drop_shot_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) : QMainWindow(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    session = new StreamSession(info, this);
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    Init();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *main = new QVBoxLayout(central);

    // 1. SELEÇÃO DE ARMA
    QGroupBox *topBox = new QGroupBox("SELEÇÃO DE ARMA", this);
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    main->addWidget(topBox);

    // 2. SMART ACTIONS - 3 ESTÁGIOS
    QGroupBox *ximBox = new QGroupBox("SMART ACTIONS - RECOIL DINÂMICO", this);
    QVBoxLayout *xLay = new QVBoxLayout(ximBox);
    auto addStage = [&](QString n, int *v, int *h) {
        xLay->addWidget(new QLabel(n, this));
        QHBoxLayout *hB = new QHBoxLayout();
        auto slV = new QSlider(Qt::Horizontal, this); slV->setRange(0, 100); slV->setValue(*v);
        auto slH = new QSlider(Qt::Horizontal, this); slH->setRange(-100, 100); slH->setValue(*h);
        connect(slV, &QSlider::valueChanged, [=](int val){ *v = val; });
        connect(slH, &QSlider::valueChanged, [=](int val){ *h = val; });
        hB->addWidget(new QLabel("V:")); hB->addWidget(slV); hB->addWidget(new QLabel("H:")); hB->addWidget(slH);
        xLay->addLayout(hB);
    };
    addStage("KICK (0-300ms)", &v_stage1, &h_stage1);
    addStage("ESTABILIZAÇÃO (300ms+)", &v_stage3, &h_stage3);
    main->addWidget(ximBox);

    // 3. AJUSTES GLOBAIS COM NÚMEROS
    auto addRow = [&](QString n, int min, int max, int *var) {
        QLabel *l = new QLabel(n + QString(": %1").arg(*var), this);
        QSlider *s = new QSlider(Qt::Horizontal, this); s->setRange(min, max); s->setValue(*var);
        connect(s, &QSlider::valueChanged, [=](int v){ *var = v; l->setText(n + QString(": %1").arg(v)); });
        main->addWidget(l); main->addWidget(s);
    };
    addRow("Lock Power (Trava)", 0, 100, &lock_power_global);
    addRow("Start Delay", 0, 15, &start_delay_global);
    addRow("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);

    // 4. CHECKBOXES
    QHBoxLayout *checkLay = new QHBoxLayout();
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    checkLay->addWidget(cbS);
    main->addLayout(checkLay);

    setCentralWidget(central);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    resize(560, 950);
    show();
    session->Start();
}

// Funções obrigatórias para o Linker e Personagem Andar
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
StreamWindow::~StreamWindow() { if (session) session->Stop(); }
