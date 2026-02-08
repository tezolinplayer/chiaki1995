// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h> 
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>
#include <QSettings>
#include <QTcpServer>
#include <QTcpSocket>

// ------------------------------------------------------------------
// LINKER BRIDGE (DANIEL MOD v5.1) - COMUNICAÇÃO COM O CONTROLLER.C
// ------------------------------------------------------------------
extern "C" {
    int recoil_v_global = 0; 
    int recoil_h_global = 0;   // Adicionado Horizontal
    int anti_dz_global = 0;    // Adicionado Anti-Deadzone
    int sticky_power_global = 750;
    int lock_power_global = 160;   
    int start_delay_global = 2;    
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
    bool crouch_spam_global = false;
    bool drop_shot_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | DANIEL GHOST ZEN ELITE v5.1");
		
	session = nullptr;
	av_widget = nullptr;
    web_server = nullptr;
	
	try {
		if(connect_info.fullscreen)
			showFullScreen();
		Init();
	}
	catch(const Exception &e) {
		QMessageBox::critical(this, tr("Stream failed"), tr("Failed to initialize: %1").arg(e.what()));
		close();
	}
}

StreamWindow::~StreamWindow() {
	if (av_widget) delete av_widget;
    if (web_server) web_server->close();
}

void StreamWindow::Init() {
	session = new StreamSession(connect_info, this);
	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	// --- ESTILO VISUAL GHOST ---
	QWidget *central = new QWidget(this);
	central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
	QVBoxLayout *mainLayout = new QVBoxLayout(central);

	// HEADER
    QHBoxLayout *header = new QHBoxLayout();
	header->addWidget(new QLabel("DANIEL ZEN GHOST - ELITE PANEL", this));
    label_rapid_status = new QLabel("[RF: OFF]", this);
    label_rapid_status->setStyleSheet("color: #555;");
    header->addStretch();
    header->addWidget(label_rapid_status);
	mainLayout->addLayout(header);

    // 1. SISTEMA DE PERFIS
    QGroupBox *profileGroup = new QGroupBox("SELEÇÃO DE ARMA", this);
    profileGroup->setStyleSheet("border: 1px solid #00FF41; padding: 5px;");
    QHBoxLayout *pLayout = new QHBoxLayout();
    combo_profiles = new QComboBox(this);
    combo_profiles->addItems({"M416", "BERYL", "MINI-14", "SKS", "GENERIC"});
    combo_profiles->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41;");
    btn_save_profile = new QPushButton("SALVAR", this);
    btn_save_profile->setStyleSheet("background-color: #003300; border: 1px solid #00FF41;");
    pLayout->addWidget(combo_profiles);
    pLayout->addWidget(btn_save_profile);
    profileGroup->setLayout(pLayout);
    mainLayout->addWidget(profileGroup);

	// 2. CONTROLE DE RECOIL PERSONALIZADO (ATUALIZADO)
    QGroupBox *aimGroup = new QGroupBox("CONTROLE DE MIRA ELITE", this);
    aimGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *aimLayout = new QVBoxLayout(aimGroup);
    
    // Recoil Vertical
    label_v = new QLabel("Recoil Vertical: 0", this);
	slider_v = new QSlider(Qt::Horizontal, this);
	slider_v->setRange(-150, 150);
    
    // NOVO: Recoil Horizontal
    label_h = new QLabel("Recoil Horizontal: 0", this);
    slider_h = new QSlider(Qt::Horizontal, this);
    slider_h->setRange(-100, 100);
    slider_h->setValue(0);

    // NOVO: Anti-Deadzone
    label_anti_dz = new QLabel("Anti-Deadzone: 0", this);
    slider_anti_dz = new QSlider(Qt::Horizontal, this);
    slider_anti_dz->setRange(0, 5000);
    slider_anti_dz->setValue(0);
    
    label_lock_power = new QLabel("Lock Power (Trava): 1.60x", this);
    slider_lock_power = new QSlider(Qt::Horizontal, this);
    slider_lock_power->setRange(100, 250); 
    slider_lock_power->setValue(160);

    label_start_delay = new QLabel("Start Delay: 2 ticks", this);
    slider_start_delay = new QSlider(Qt::Horizontal, this);
    slider_start_delay->setRange(0, 15);
    slider_start_delay->setValue(2);

    label_sticky_power = new QLabel("Força Magnetismo: 750", this);
    slider_sticky_power = new QSlider(Qt::Horizontal, this);
    slider_sticky_power->setRange(0, 2000);
    slider_sticky_power->setValue(750);

    // Adicionando ao Layout
    aimLayout->addWidget(label_v); aimLayout->addWidget(slider_v);
    aimLayout->addWidget(label_h); aimLayout->addWidget(slider_h);
    aimLayout->addWidget(label_anti_dz); aimLayout->addWidget(slider_anti_dz);
    aimLayout->addWidget(label_lock_power); aimLayout->addWidget(slider_lock_power);
    aimLayout->addWidget(label_start_delay); aimLayout->addWidget(slider_start_delay);
    aimLayout->addWidget(label_sticky_power); aimLayout->addWidget(slider_sticky_power);
    mainLayout->addWidget(aimGroup);

	// 3. MACROS E FUNÇÕES
    QGroupBox *macroGroup = new QGroupBox("MACROS & FUNÇÕES", this);
    macroGroup->setStyleSheet("border: 1px solid #00FF41;");
    QGridLayout *mLayout = new QGridLayout(macroGroup);
    check_crouch_spam = new QCheckBox("CROUCH SPAM", this);
    check_drop_shot = new QCheckBox("DROP SHOT", this);
    check_sticky_aim = new QCheckBox("STICKY AIM", this);
    check_rapid_fire = new QCheckBox("RAPID FIRE", this);
    mLayout->addWidget(check_crouch_spam, 0, 0); mLayout->addWidget(check_drop_shot, 0, 1);
    mLayout->addWidget(check_sticky_aim, 1, 0); mLayout->addWidget(check_rapid_fire, 1, 1);
    mainLayout->addWidget(macroGroup);

	setCentralWidget(central);

	// --- CONEXÕES DOS SLIDERS (LOGICA BRIDGE) ---
	connect(slider_v, &QSlider::valueChanged, this, [this](int val){ recoil_v_global = val; label_v->setText(QString("Recoil Vertical: %1").arg(val)); });
    connect(slider_h, &QSlider::valueChanged, this, [this](int val){ recoil_h_global = val; label_h->setText(QString("Recoil Horizontal: %1").arg(val)); });
    connect(slider_anti_dz, &QSlider::valueChanged, this, [this](int val){ anti_dz_global = val; label_anti_dz->setText(QString("Anti-Deadzone: %1").arg(val)); });
    
    connect(slider_sticky_power, &QSlider::valueChanged, this, [this](int val){ sticky_power_global = val; label_sticky_power->setText(QString("Força Magnetismo: %1").arg(val)); });
    connect(slider_lock_power, &QSlider::valueChanged, this, [this](int val){ 
        lock_power_global = val; 
        label_lock_power->setText(QString("Lock Power (Trava): %1x").arg(val/100.0, 0, 'f', 2)); 
    });
    connect(slider_start_delay, &QSlider::valueChanged, this, [this](int val){ 
        start_delay_global = val; 
        label_start_delay->setText(QString("Start Delay: %1 ticks").arg(val)); 
    });

	// --- CONEXÕES DE CHECKBOXES ---
	connect(check_crouch_spam, &QCheckBox::toggled, this, [](bool checked){ crouch_spam_global = checked; });
    connect(check_drop_shot, &QCheckBox::toggled, this, [](bool checked){ drop_shot_global = checked; });
	connect(check_sticky_aim, &QCheckBox::toggled, this, [](bool checked){ sticky_aim_global = checked; });
    connect(check_rapid_fire, &QCheckBox::toggled, this, [this](bool checked){ 
        rapid_fire_global = checked; 
        label_rapid_status->setText(checked ? "[RAPID FIRE ON]" : "[RF: OFF]");
        label_rapid_status->setStyleSheet(checked ? "color: red;" : "color: #555;");
    });

    connect(combo_profiles, &QComboBox::currentTextChanged, this, &StreamWindow::LoadProfile);
    connect(btn_save_profile, &QPushButton::clicked, this, [this](){ SaveProfile(combo_profiles->currentText()); });

	grabKeyboard();
	session->Start();
    StartWebBridge(); 
    LoadProfile("GENERIC");
	resize(540, 850); // Aumentado para caber os novos sliders
	show();
}

// --- PERSISTÊNCIA DE PERFIS ---
void StreamWindow::SaveProfile(const QString &name) {
    QSettings s("DanielMods", "ZenGhost");
    s.beginGroup(name);
    s.setValue("recoil_v", slider_v->value());
    s.setValue("recoil_h", slider_h->value());      // SALVA HORIZONTAL
    s.setValue("anti_dz", slider_anti_dz->value()); // SALVA DZ
    s.setValue("sticky_power", slider_sticky_power->value());
    s.setValue("lock_power", slider_lock_power->value());
    s.setValue("start_delay", slider_start_delay->value());
    s.setValue("crouch", check_crouch_spam->isChecked());
    s.setValue("drop", check_drop_shot->isChecked());
    s.setValue("sticky_aim", check_sticky_aim->isChecked());
    s.setValue("rapid", check_rapid_fire->isChecked());
    s.endGroup();
}

void StreamWindow::LoadProfile(const QString &name) {
    QSettings s("DanielMods", "ZenGhost");
    s.beginGroup(name);
    slider_v->setValue(s.value("recoil_v", 0).toInt());
    slider_h->setValue(s.value("recoil_h", 0).toInt());
    slider_anti_dz->setValue(s.value("anti_dz", 0).toInt());
    slider_sticky_power->setValue(s.value("sticky_power", 750).toInt());
    slider_lock_power->setValue(s.value("lock_power", 160).toInt());
    slider_start_delay->setValue(s.value("start_delay", 2).toInt());
    check_crouch_spam->setChecked(s.value("crouch", false).toBool());
    check_drop_shot->setChecked(s.value("drop", false).toBool());
    check_sticky_aim->setChecked(s.value("sticky_aim", false).toBool());
    check_rapid_fire->setChecked(s.value("rapid", false).toBool());
    s.endGroup();
}

// --- RESTANTE DAS FUNÇÕES (EVENTOS E WEBBRIDGE) ---
void StreamWindow::OnNewWebConnection() {
    QTcpSocket *socket = web_server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QString request = QString::fromUtf8(socket->readAll());
        if (request.contains("RF:TOGGLE")) check_rapid_fire->toggle();
        socket->write("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\n\r\nOK");
        socket->disconnectFromHost();
    });
}

void StreamWindow::StartWebBridge() {
    web_server = new QTcpServer(this);
    web_server->listen(QHostAddress::Any, 8080);
}

void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); QMainWindow::mouseDoubleClickEvent(e); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
void StreamWindow::UpdateVideoTransform() {}
