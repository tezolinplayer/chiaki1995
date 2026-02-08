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

// --- LINKER BRIDGE: CONEXÃO COM O SEU NOVO MOTOR ---
extern "C" {
    int recoil_v_global = 0;   // Força Vertical
    int recoil_h_global = 0;   // Força Horizontal
    int anti_dz_global = 0;
    int sticky_power_global = 600;
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
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    session = new StreamSession(connect_info, this);
    
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);
    
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) session->Stop();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // --- GRUPO DE RECOIL COM AJUSTE 0-100 ---
    QGroupBox *recoilGroup = new QGroupBox("CONTROLE DE RECOIL PERSONALIZADO", this);
    recoilGroup->setStyleSheet("border: 1px solid #00FF41; padding: 10px;");
    QVBoxLayout *rLayout = new QVBoxLayout(recoilGroup);

    auto addLabeledSlider = [&](QString title, int min, int max, int *var) {
        QHBoxLayout *hBox = new QHBoxLayout();
        QLabel *label = new QLabel(title + QString(": %1").arg(*var), this);
        label->setFixedWidth(200);
        
        QSlider *slider = new QSlider(Qt::Horizontal, this);
        slider->setRange(min, max);
        slider->setValue(*var);
        
        connect(slider, &QSlider::valueChanged, [label, title, var](int val){
            *var = val;
            label->setText(title + QString(": %1").arg(val));
        });
        
        hBox->addWidget(label);
        hBox->addWidget(slider);
        rLayout->addLayout(hBox);
    };

    // Agora usa EXATAMENTE as variáveis que o motor exige (recoil_v e recoil_h)
    addLabeledSlider("Recoil Vertical (0-100)", 0, 100, &recoil_v_global);
    addLabeledSlider("Recoil Horizontal (L/R)", -100, 100, &recoil_h_global);
    addLabeledSlider("Lock Power (Estabilizar)", 0, 100, &lock_power_global);
    addLabeledSlider("Start Delay (Ticks)", 0, 15, &start_delay_global);
    
    mainLayout->addWidget(recoilGroup);

    // --- GRUPO DE PRECISÃO ---
    QGroupBox *aimGroup = new QGroupBox("AJUSTES DE PRECISÃO", this);
    QVBoxLayout *aLayout = new QVBoxLayout(aimGroup);
    addLabeledSlider("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);
    addLabeledSlider("Anti-Deadzone", 0, 5000, &anti_dz_global);
    mainLayout->addWidget(aimGroup);

    // --- FUNÇÕES ---
    QHBoxLayout *fLayout = new QHBoxLayout();
    QCheckBox *cb_sticky = new QCheckBox("STICKY AIM", this);
    QCheckBox *cb_rapid = new QCheckBox("RAPID FIRE", this);
    connect(cb_sticky, &QCheckBox::toggled, [](bool chk){ sticky_aim_global = chk; });
    connect(cb_rapid, &QCheckBox::toggled, [](bool chk){ rapid_fire_global = chk; });
    fLayout->addWidget(cb_sticky); fLayout->addWidget(cb_rapid);
    mainLayout->addLayout(fLayout);

    setCentralWidget(central);
    resize(560, 950);
    show();
    session->Start();
}

// --- FUNÇÕES OBRIGATÓRIAS PARA O LINKER ---
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
