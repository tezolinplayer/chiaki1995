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
    int recoil_v_global = 0;
    int recoil_h_global = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 650;
    int lock_power_global = 100;
    int start_delay_global = 2;
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
    : QMainWindow(parent), connect_info(connect_info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
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

    // --- 1. SELEÇÃO DE ARMA E SALVAR ---
    QGroupBox *profileGroup = new QGroupBox("SELEÇÃO DE ARMA", this);
    profileGroup->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *pLayout = new QHBoxLayout();
    QComboBox *combo_profiles = new QComboBox(this);
    combo_profiles->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo_profiles->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    
    QPushButton *btn_save = new QPushButton("SALVAR PERFIL", this);
    btn_save->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    
    pLayout->addWidget(combo_profiles); pLayout->addWidget(btn_save);
    profileGroup->setLayout(pLayout);
    mainLayout->addWidget(profileGroup);

    // --- 2. CONTROLE DE RECOIL (COM NÚMEROS 0-100) ---
    QGroupBox *recoilGroup = new QGroupBox("CONTROLE DE RECOIL PERSONALIZADO", this);
    recoilGroup->setStyleSheet("border: 1px solid #00FF41; padding: 10px;");
    QVBoxLayout *rLayout = new QVBoxLayout(recoilGroup);

    auto addAdjuster = [&](QString title, int min, int max, int *var) {
        QLabel *label = new QLabel(title + QString(": %1").arg(*var), this);
        QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(min, max);
        slider->setValue(*var);
        connect(slider, &QSlider::valueChanged, [label, title, var](int val){
            *var = val;
            label->setText(title + QString(": %1").arg(val));
        });
        rLayout->addWidget(label); rLayout->addWidget(slider);
    };

    addAdjuster("Recoil Vertical (Andar)", 0, 100, &recoil_v_global);
    addAdjuster("Lock Power (Estabilizar)", 0, 100, &lock_power_global);
    addAdjuster("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);
    mainLayout->addWidget(recoilGroup);

    // --- 3. BOTÃO SALVAR (LOGICA) ---
    connect(btn_save, &QPushButton::clicked, [=](){
        QSettings settings("DanielGhost", "ChiakiZen");
        settings.beginGroup(combo_profiles->currentText());
        settings.setValue("v", recoil_v_global);
        settings.setValue("lock", lock_power_global);
        settings.endGroup();
    });

    setCentralWidget(central);
    resize(560, 950);
    show();
    session->Start();
}

// --- FUNÇÕES OBRIGATÓRIAS PARA O LINKER (RESOLVE O ERRO) ---
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }

void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
