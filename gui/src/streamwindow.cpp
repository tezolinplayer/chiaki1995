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
#include <QSettings>
#include <QKeyEvent>
#include <QMouseEvent>

// --- LINKER BRIDGE: CONEXÃO COMPLETA COM O MOTOR ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 750;
    int lock_power_global = 100;
    int start_delay_global = 2;
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
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
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 1. SELEÇÃO DE ARMA E SALVAR
    QGroupBox *topBox = new QGroupBox("PERFIS DANIEL GHOST", this);
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    mainLayout->addWidget(topBox);

    // 2. SMART ACTIONS - 3 ESTÁGIOS (COM NÚMEROS 0-100)
    auto addStage = [&](QString n, int *v, int *h) {
        mainLayout->addWidget(new QLabel(n, this));
        QHBoxLayout *hB = new QHBoxLayout();
        QLabel *lV = new QLabel(QString("V: %1").arg(*v));
        QSlider *sV = new QSlider(Qt::Horizontal); sV->setRange(0, 100); sV->setValue(*v);
        connect(sV, &QSlider::valueChanged, [=](int val){ *v = val; lV->setText(QString("V: %1").arg(val)); });
        
        QLabel *lH = new QLabel(QString("H: %1").arg(*h));
        QSlider *sH = new QSlider(Qt::Horizontal); sH->setRange(-100, 100); sH->setValue(*h);
        connect(sH, &QSlider::valueChanged, [=](int val){ *h = val; lH->setText(QString("H: %1").arg(val)); });
        
        hB->addWidget(lV); hB->addWidget(sV); hB->addWidget(lH); hB->addWidget(sH);
        mainLayout->addLayout(hB);
    };

    addStage("ESTÁGIO 1: KICK (0-300ms)", &v_stage1, &h_stage1);
    addStage("ESTÁGIO 2: TRANSIÇÃO (300-800ms)", &v_stage2, &h_stage2);
    addStage("ESTÁGIO 3: FINAL (800ms+)", &v_stage3, &h_stage3);

    // 3. AJUSTES GERAIS
    auto addGlobal = [&](QString n, int min, int max, int *var) {
        QLabel *l = new QLabel(n + QString(": %1").arg(*var), this);
        QSlider *s = new QSlider(Qt::Horizontal, this); s->setRange(min, max); s->setValue(*var);
        connect(s, &QSlider::valueChanged, [=](int v){ *var = v; l->setText(n + QString(": %1").arg(v)); });
        mainLayout->addWidget(l); mainLayout->addWidget(s);
    };
    addGlobal("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);
    addGlobal("Start Delay", 0, 15, &start_delay_global);

    // 4. MACROS
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mainLayout->addWidget(cbS);

    setCentralWidget(central);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    resize(560, 950);
    show();
    session->Start();
}

// Funções obrigatórias para o Linker e para o personagem andar
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
StreamWindow::~StreamWindow() { if (session) session->Stop(); }
