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

// --- LINKER BRIDGE ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0, sticky_power_global = 750;
    int lock_power_global = 100, start_delay_global = 2;
    bool sticky_aim_global = false, rapid_fire_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) 
    : QMainWindow(parent), connect_info(info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    
    // FIX ANALÓGICOS: Garante foco para capturar WASD e Mouse
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true); 

    session = new StreamSession(info, this);
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) session->Stop();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *main = new QVBoxLayout(central);

    // 1. SELEÇÃO DE ARMA
    QGroupBox *topBox = new QGroupBox("PERFIL DANIEL GHOST", this);
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    main->addWidget(topBox);

    // 2. SMART ACTIONS (3 ESTÁGIOS)
    auto addStage = [&](QString n, int *v, int *h) {
        QHBoxLayout *hB = new QHBoxLayout();
        QLabel *lV = new QLabel(QString("V: %1").arg(*v)); lV->setFixedWidth(60);
        QSlider *sV = new QSlider(Qt::Horizontal); sV->setRange(0, 100); sV->setValue(*v);
        connect(sV, &QSlider::valueChanged, [=](int val){ *v = val; lV->setText(QString("V: %1").arg(val)); });
        
        QLabel *lH = new QLabel(QString("H: %1").arg(*h)); lH->setFixedWidth(60);
        QSlider *sH = new QSlider(Qt::Horizontal); sH->setRange(-100, 100); sH->setValue(*h);
        connect(sH, &QSlider::valueChanged, [=](int val){ *h = val; lH->setText(QString("H: %1").arg(val)); });

        hB->addWidget(new QLabel(n)); hB->addWidget(lV); hB->addWidget(sV); hB->addWidget(lH); hB->addWidget(sH);
        main->addLayout(hB);
    };

    addStage("KICK (0-300ms)", &v_stage1, &h_stage1);
    addStage("MEIO (300-800ms)", &v_stage2, &h_stage2);
    addStage("FINAL (800ms+)", &v_stage3, &h_stage3);

    // 3. AJUSTES GLOBAIS
    auto addGlobal = [&](QString n, int min, int max, int *var) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *l = new QLabel(n + QString(": %1").arg(*var)); l->setFixedWidth(200);
        QSlider *s = new QSlider(Qt::Horizontal); s->setRange(min, max); s->setValue(*var);
        connect(s, &QSlider::valueChanged, [=](int v){ *var = v; l->setText(n + QString(": %1").arg(v)); });
        row->addWidget(l); row->addWidget(s);
        main->addLayout(row);
    };
    
    addGlobal("Trava (Lock Power)", 0, 100, &lock_power_global);
    addGlobal("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);
    addGlobal("Delay (Ticks)", 0, 15, &start_delay_global);

    // 4. CHECKBOX
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    main->addWidget(cbS);

    // SALVAR
    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v1", v_stage1);
        s.setValue(combo->currentText() + "/v3", v_stage3);
    });

    setCentralWidget(central);
    resize(600, 950);
    show();
    session->Start();
}

// --- FUNÇÕES DE INPUT PADRÃO (Sem mouseMoveEvent para evitar erro) ---
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }

// SISTEMA
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
