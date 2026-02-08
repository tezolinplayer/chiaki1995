// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <streamsession.h>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTcpServer>
#include <QTcpSocket>

// --- LINKER BRIDGE: CONEXÃO COM O CONTROLLER.C ---
// Essas variáveis são as mesmas que o motor do controle lê em tempo real
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
    bool crouch_spam_global = false;
    bool drop_shot_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
    : QMainWindow(parent), connect_info(connect_info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN | SMART ACTIONS V5.5");
    session = new StreamSession(connect_info, this);
    
    // Conecta sinais vitais da sessão
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);
    
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) {
        session->Stop();
    }
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // --- PAINEL SMART ACTIONS (ESTILO XIM MATRIX) ---
    QGroupBox *ximGroup = new QGroupBox("SMART ACTIONS - RECOIL DINÂMICO", this);
    ximGroup->setStyleSheet("QGroupBox { border: 2px solid #FFD700; color: #FFD700; margin-top: 10px; padding: 10px; }");
    QVBoxLayout *xLayout = new QVBoxLayout(ximGroup);

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
    addStageControl("ESTÁGIO 3: ESTABILIZAÇÃO (800ms+)", &v_stage3, &h_stage3);
    mainLayout->addWidget(ximGroup);

    // --- AJUSTES DE PRECISÃO ---
    QGroupBox *globalGroup = new QGroupBox("GLOBAL SETTINGS", this);
    globalGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *gLayout = new QVBoxLayout(globalGroup);

    QLabel *label_dz = new QLabel(QString("Anti-Deadzone: %1").arg(anti_dz_global), this);
    QSlider *slider_dz = new QSlider(Qt::Horizontal, this);
    slider_dz->setRange(0, 5000);
    connect(slider_dz, &QSlider::valueChanged, [label_dz](int val){ 
        anti_dz_global = val; 
        label_dz->setText(QString("Anti-Deadzone: %1").arg(val));
    });
    
    gLayout->addWidget(label_dz);
    gLayout->addWidget(slider_dz);
    mainLayout->addWidget(globalGroup);

    // --- FUNÇÕES E MACROS ---
    QGroupBox *macroGroup = new QGroupBox("MACROS", this);
    QHBoxLayout *mLayout = new QHBoxLayout(macroGroup);
    
    QCheckBox *cb_sticky = new QCheckBox("STICKY AIM", this);
    QCheckBox *cb_rapid = new QCheckBox("RAPID FIRE", this);
    
    connect(cb_sticky, &QCheckBox::toggled, [](bool chk){ sticky_aim_global = chk; });
    connect(cb_rapid, &QCheckBox::toggled, [](bool chk){ rapid_fire_global = chk; });
    
    mLayout->addWidget(cb_sticky);
    mLayout->addWidget(cb_rapid);
    mainLayout->addWidget(macroGroup);

    setCentralWidget(central);
    resize(540, 950);
    show();
    
    session->Start();
}

// --- MÉTODOS DE SESSÃO ---
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { 
    if(isFullScreen()) showNormal(); else showFullScreen(); 
}

// --- EVENTOS DE INPUT (TECLADO E MOUSE) ---
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }

// --- EVENTOS DE JANELA ---
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); QMainWindow::closeEvent(e); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
