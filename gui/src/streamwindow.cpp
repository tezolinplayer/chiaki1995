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

// --- LINKER BRIDGE: CONEXÃO COM O MOTOR DO CONTROLLER.C ---
extern "C" {
    int recoil_v_global = 16, recoil_h_global = 0;
    int lock_power_global = 100, start_delay_global = 2, sticky_power_global = 750, anti_dz_global = 0;
    bool sticky_aim_global = false, rapid_fire_global = false;
}

// FIX CONSTRUTOR: Agora passa a 'connect_info' corretamente para evitar erro de compilação
StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent) 
    : QMainWindow(parent), connect_info(connect_info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    session = new StreamSession(connect_info, this);
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) session->Stop();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 1. SELEÇÃO DE ARMA (VISUAL v4.6)
    QGroupBox *topBox = new QGroupBox("SELEÇÃO DE ARMA", this);
    topBox->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btnSave = new QPushButton("SALVAR PERFIL", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    mainLayout->addWidget(topBox);

    // 2. AJUSTES COM NÚMEROS (0-100)
    auto addRow = [&](QString name, int min, int max, int *var) {
        QLabel *lab = new QLabel(name + QString(": %1").arg(*var), this);
        QSlider *sld = new QSlider(Qt::Horizontal, this);
        sld->setRange(min, max); sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ 
            *var = val; 
            lab->setText(name + QString(": %1").arg(val)); 
        });
        mainLayout->addWidget(lab); mainLayout->addWidget(sld);
    };

    addRow("Recoil Vertical (Força)", 0, 100, &recoil_v_global);
    addRow("Lock Power (Trava)", 0, 100, &lock_power_global);
    addRow("Magnetismo (Sticky AA)", 0, 1500, &sticky_power_global);

    // 3. MACROS
    QGroupBox *macroBox = new QGroupBox("MACROS", this);
    QHBoxLayout *mLayout = new QHBoxLayout(macroBox);
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    cbS->setChecked(sticky_aim_global);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mLayout->addWidget(cbS);
    mainLayout->addWidget(macroBox);

    // Lógica de Salvamento
    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v", recoil_v_global);
        s.setValue(combo->currentText() + "/lock", lock_power_global);
    });

    setCentralWidget(central);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    resize(560, 950);
    show();
    session->Start();
}

// FUNÇÕES OBRIGATÓRIAS PARA COMPILAR E PARA O PERSONAGEM ANDAR
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
