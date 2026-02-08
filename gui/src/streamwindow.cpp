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

// --- AQUI ESTÁ O SEGREDO: CRIANDO AS VARIÁVEIS PARA O MOTOR ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 750;
    int lock_power_global = 100;
    int start_delay_global = 2;
    bool sticky_aim_global = false;
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

    // 1. SELEÇÃO DE ARMA
    QGroupBox *topBox = new QGroupBox("PERFIL DA ARMA", this);
    topBox->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *topLay = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    topLay->addWidget(combo); topLay->addWidget(btnSave);
    mainLayout->addWidget(topBox);

    // 2. SMART ACTIONS (3 ESTÁGIOS)
    QGroupBox *ximBox = new QGroupBox("SMART ACTIONS (RECOIL DINÂMICO)", this);
    ximBox->setStyleSheet("border: 1px solid #00FF41; padding: 10px;");
    QVBoxLayout *xLay = new QVBoxLayout(ximBox);
    
    auto addStage = [&](QString n, int *v, int *h) {
        xLay->addWidget(new QLabel(n, this));
        QHBoxLayout *hB = new QHBoxLayout();
        
        QLabel *lV = new QLabel(QString("V: %1").arg(*v));
        QSlider *sV = new QSlider(Qt::Horizontal); sV->setRange(0, 100); sV->setValue(*v);
        connect(sV, &QSlider::valueChanged, [=](int val){ *v = val; lV->setText(QString("V: %1").arg(val)); });
        
        QLabel *lH = new QLabel(QString("H: %1").arg(*h));
        QSlider *sH = new QSlider(Qt::Horizontal); sH->setRange(-100, 100); sH->setValue(*h);
        connect(sH, &QSlider::valueChanged, [=](int val){ *h = val; lH->setText(QString("H: %1").arg(val)); });

        hB->addWidget(lV); hB->addWidget(sV); hB->addWidget(lH); hB->addWidget(sH);
        xLay->addLayout(hB);
    };

    addStage("ESTÁGIO 1 (KICK)", &v_stage1, &h_stage1);
    addStage("ESTÁGIO 2 (TRANSIÇÃO)", &v_stage2, &h_stage2);
    addStage("ESTÁGIO 3 (FINAL)", &v_stage3, &h_stage3);
    mainLayout->addWidget(ximBox);

    // 3. AJUSTES GLOBAIS
    QGroupBox *globalBox = new QGroupBox("PRECISÃO", this);
    globalBox->setStyleSheet("border: 1px solid #00FF41; padding: 10px;");
    QVBoxLayout *gLay = new QVBoxLayout(globalBox);
    
    auto addGlobal = [&](QString n, int min, int max, int *var) {
        QLabel *lab = new QLabel(n + QString(": %1").arg(*var));
        QSlider *sld = new QSlider(Qt::Horizontal); sld->setRange(min, max); sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ *var = val; lab->setText(n + QString(": %1").arg(val)); });
        gLay->addWidget(lab); gLay->addWidget(sld);
    };
    
    addGlobal("Trava (Lock Power)", 0, 100, &lock_power_global);
    addGlobal("Magnetismo (Aim Assist)", 0, 1500, &sticky_power_global);
    addGlobal("Delay de Início", 0, 15, &start_delay_global);
    mainLayout->addWidget(globalBox);

    // 4. CHECKBOX E SALVAR
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mainLayout->addWidget(cbS);

    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v1", v_stage1);
    });

    setCentralWidget(central);
    resize(560, 950);
    show();
    session->Start();
}

// --- FUNÇÕES OBRIGATÓRIAS DE EVENTOS (ISSO RESOLVE O ERRO 'keyReleaseEvent') ---
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }

void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); } // AQUI ESTAVA FALTANDO!
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
