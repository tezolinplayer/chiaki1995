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

// --- VARIÁVEIS GLOBAIS ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0, sticky_power_global = 750;
    int lock_power_global = 100, start_delay_global = 2;
    bool sticky_aim_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) 
    : QMainWindow(parent), connect_info(info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    
    // Configuração de Foco para Teclado/Mouse funcionar
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
    QGroupBox *topBox = new QGroupBox("PERFIL DA ARMA", this);
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    main->addWidget(topBox);

    // 2. RECOIL 3 ESTÁGIOS
    QGroupBox *ximBox = new QGroupBox("SMART ACTIONS", this);
    QVBoxLayout *xLay = new QVBoxLayout(ximBox);
    
    auto addStage = [&](QString n, int *v, int *h) {
        QHBoxLayout *hB = new QHBoxLayout();
        QLabel *lV = new QLabel(QString("V: %1").arg(*v)); lV->setFixedWidth(60);
        QSlider *sV = new QSlider(Qt::Horizontal); sV->setRange(0, 100); sV->setValue(*v);
        connect(sV, &QSlider::valueChanged, [=](int val){ *v = val; lV->setText(QString("V: %1").arg(val)); });
        
        QLabel *lH = new QLabel(QString("H: %1").arg(*h)); lH->setFixedWidth(60);
        QSlider *sH = new QSlider(Qt::Horizontal); sH->setRange(-100, 100); sH->setValue(*h);
        connect(sH, &QSlider::valueChanged, [=](int val){ *h = val; lH->setText(QString("H: %1").arg(val)); });

        hB->addWidget(new QLabel(n)); hB->addWidget(lV); hB->addWidget(sV); hB->addWidget(lH); hB->addWidget(sH);
        xLay->addLayout(hB);
    };

    addStage("KICK", &v_stage1, &h_stage1);
    addStage("MEIO", &v_stage2, &h_stage2);
    addStage("FINAL", &v_stage3, &h_stage3);
    main->addWidget(ximBox);

    // 3. AJUSTES GERAIS
    auto addGlobal = [&](QString n, int max, int *var) {
        QHBoxLayout *r = new QHBoxLayout();
        QLabel *l = new QLabel(n + QString(": %1").arg(*var));
        QSlider *s = new QSlider(Qt::Horizontal); s->setRange(0, max); s->setValue(*var);
        connect(s, &QSlider::valueChanged, [=](int v){ *var = v; l->setText(n + QString(": %1").arg(v)); });
        r->addWidget(l); r->addWidget(s);
        main->addLayout(r);
    };
    addGlobal("Lock Power", 100, &lock_power_global);
    addGlobal("Magnetismo", 1500, &sticky_power_global);

    // 4. CHECKBOX
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    main->addWidget(cbS);

    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v1", v_stage1);
        s.setValue(combo->currentText() + "/v3", v_stage3);
    });

    setCentralWidget(central);
    resize(600, 900);
    show();
    session->Start();
}

// --- FUNÇÕES DE INPUT (CRUCIAIS PARA O ANALÓGICO MEXER) ---
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }

void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
