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

// Definição das variáveis globais para o Linker
extern "C" {
    int v_stage1=0, h_stage1=0, v_stage2=0, h_stage2=0, v_stage3=0, h_stage3=0;
    int sticky_power_global=600, lock_power_global=100, start_delay_global=2;
    bool sticky_aim_global=false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) 
    : QMainWindow(parent), connect_info(info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5 (FINAL)");
    
    // Habilita teclado e mouse
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
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 1. PERFIL
    QGroupBox *boxProfile = new QGroupBox("PERFIL DA ARMA", this);
    QHBoxLayout *layProfile = new QHBoxLayout(boxProfile);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    layProfile->addWidget(combo); layProfile->addWidget(btnSave);
    mainLayout->addWidget(boxProfile);

    // 2. RECOIL
    auto addStage = [&](QString txt, int *v, int *h) {
        QHBoxLayout *row = new QHBoxLayout();
        
        QLabel *lblV = new QLabel(QString("V: %1").arg(*v)); lblV->setFixedWidth(60);
        QSlider *sldV = new QSlider(Qt::Horizontal); sldV->setRange(0, 100); sldV->setValue(*v);
        connect(sldV, &QSlider::valueChanged, [=](int val){ *v = val; lblV->setText(QString("V: %1").arg(val)); });

        QLabel *lblH = new QLabel(QString("H: %1").arg(*h)); lblH->setFixedWidth(60);
        QSlider *sldH = new QSlider(Qt::Horizontal); sldH->setRange(-100, 100); sldH->setValue(*h);
        connect(sldH, &QSlider::valueChanged, [=](int val){ *h = val; lblH->setText(QString("H: %1").arg(val)); });

        row->addWidget(new QLabel(txt)); row->addWidget(lblV); row->addWidget(sldV); row->addWidget(lblH); row->addWidget(sldH);
        mainLayout->addLayout(row);
    };

    addStage("KICK (Início)", &v_stage1, &h_stage1);
    addStage("MEIO (Controle)", &v_stage2, &h_stage2);
    addStage("FINAL (Lock)", &v_stage3, &h_stage3);

    // 3. EXTRAS
    auto addExtra = [&](QString txt, int max, int *var) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lbl = new QLabel(txt + QString(": %1").arg(*var));
        QSlider *sld = new QSlider(Qt::Horizontal); sld->setRange(0, max); sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ *var = val; lbl->setText(txt + QString(": %1").arg(val)); });
        row->addWidget(lbl); row->addWidget(sld);
        mainLayout->addLayout(row);
    };
    addExtra("Lock Power", 100, &lock_power_global);
    addExtra("Magnetismo", 1500, &sticky_power_global);

    // CHECKBOX
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mainLayout->addWidget(cbS);

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

// Eventos de Input (Para mouse e teclado funcionarem)
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
