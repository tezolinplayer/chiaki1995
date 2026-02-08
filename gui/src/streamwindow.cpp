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

// --- LINKER BRIDGE: CONEXÃO COM O MOTOR DO CONTROLE ---
extern "C" {
    int recoil_v_global = 16, recoil_h_global = 0;
    int lock_power_global = 100, start_delay_global = 2, sticky_power_global = 750, anti_dz_global = 0;
    bool sticky_aim_global = false, rapid_fire_global = false;
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

    // 1. SELEÇÃO DE ARMA E SALVAR (Igual à v4.6)
    QGroupBox *topBox = new QGroupBox("SELEÇÃO DE ARMA", this);
    topBox->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *topLayout = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "AKM", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    
    QPushButton *btnSave = new QPushButton("SALVAR PERFIL", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    
    topLayout->addWidget(combo); topLayout->addWidget(btnSave);
    mainLayout->addWidget(topBox);

    // 2. CONTROLES COM NÚMEROS (0-100) - Igual à image_de8b83.png
    QGroupBox *recoilGroup = new QGroupBox("CONTROLE DE RECOIL PERSONALIZADO", this);
    recoilGroup->setStyleSheet("border: 1px solid #00FF41; padding: 10px;");
    QVBoxLayout *rLayout = new QVBoxLayout(recoilGroup);

    auto addRow = [&](QString name, int min, int max, int *var) {
        QLabel *lab = new QLabel(name + QString(": %1").arg(*var), this);
        QSlider *sld = new QSlider(Qt::Horizontal, this);
        sld->setRange(min, max); 
        sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ 
            *var = val; 
            lab->setText(name + QString(": %1").arg(val)); 
        });
        rLayout->addWidget(lab); rLayout->addWidget(sld);
    };

    addRow("Recoil Vertical (Andar)", 0, 100, &recoil_v_global);
    addRow("Lock Power (Trava)", 0, 100, &lock_power_global);
    addRow("Start Delay (Ticks)", 0, 15, &start_delay_global);
    addRow("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);
    mainLayout->addWidget(recoilGroup);

    // 3. MACROS (STICKY AIM)
    QGroupBox *macroBox = new QGroupBox("MACROS", this);
    QHBoxLayout *mLayout = new QHBoxLayout(macroBox);
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    cbS->setChecked(sticky_aim_global);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mLayout->addWidget(cbS);
    mainLayout->addLayout(mLayout);

    // Lógica do Botão Salvar
    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v", recoil_v_global);
        s.setValue(combo->currentText() + "/lock", lock_power_global);
    });

    setCentralWidget(central);
    resize(560, 950);
    show();
    session->Start();
}

// --- FUNÇÕES OBRIGATÓRIAS PARA O LINKER E PARA O PERSONAGEM ANDAR ---
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
