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

// Linker Bridge
extern "C" {
    int v_stage1=0, h_stage1=0, v_stage2=0, h_stage2=0, v_stage3=0, h_stage3=0;
    int sticky_power_global=650, lock_power_global=100, start_delay_global=2;
    bool sticky_aim_global=false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) 
    : QMainWindow(parent), connect_info(info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    session = new StreamSession(info, this);
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    Init();
}

StreamWindow::~StreamWindow() { if (session) session->Stop(); }

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLay = new QVBoxLayout(central);

    // Perfis
    QGroupBox *boxP = new QGroupBox("PERFIL DA ARMA", this);
    QHBoxLayout *layP = new QHBoxLayout(boxP);
    QComboBox *combo = new QComboBox(this); combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    QPushButton *btnS = new QPushButton("SALVAR", this);
    btnS->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    layP->addWidget(combo); layP->addWidget(btnS);
    mainLay->addWidget(boxP);

    // Sliders
    auto addStage = [&](QString t, int *v, int *h) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lblV = new QLabel(QString("V: %1").arg(*v));
        QSlider *sldV = new QSlider(Qt::Horizontal); sldV->setRange(0, 100); sldV->setValue(*v);
        connect(sldV, &QSlider::valueChanged, [=](int val){ *v = val; lblV->setText(QString("V: %1").arg(val)); });
        row->addWidget(new QLabel(t)); row->addWidget(lblV); row->addWidget(sldV);
        mainLay->addLayout(row);
    };

    addStage("KICK", &v_stage1, &h_stage1);
    addStage("FINAL", &v_stage3, &h_stage3);

    // Checkbox
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mainLay->addWidget(cbS);

    setCentralWidget(central);
    resize(560, 950);
    show();
    session->Start();
}

// Funções de Input (Obrigatórias para o Analógico mexer)
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
