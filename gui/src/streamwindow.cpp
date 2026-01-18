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
// LINKER BRIDGE (DANIEL MOD v4.0)
// ------------------------------------------------------------------
extern "C" {
    int recoil_v_global = 0; 
    int recoil_h_global = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 0;
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
	setWindowTitle(qApp->applicationName() + " | DANIEL GHOST ZEN ELITE v4.0");
		
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

	// --- ESTILO VISUAL ---
	QWidget *central = new QWidget(this);
	central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
	QVBoxLayout *mainLayout = new QVBoxLayout(central);

	// HEADER E STATUS
    QHBoxLayout *header = new QHBoxLayout();
	QLabel *title = new QLabel("DANIEL ZEN GHOST - ELITE PANEL", this);
    label_rapid_status = new QLabel("[RF: OFF]", this);
    label_rapid_status->setStyleSheet("color: #555;");
    header->addWidget(title);
    header->addStretch();
    header->addWidget(label_rapid_status);
	mainLayout->addLayout(header);

    // 1. SISTEMA DE PERFIS (PRESETS DE ARMAS)
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

	// 2. RECOIL E AIM ASSIST
    QGroupBox *aimGroup = new QGroupBox("RECOIL & MAGNETISMO", this);
    aimGroup->setStyleSheet("border: 1px solid #00FF41;");
    QVBoxLayout *aimLayout = new QVBoxLayout();
	
    label_v = new QLabel("Recoil Vertical: 0", this);
	slider_v = new QSlider(Qt::Horizontal, this);
	slider_v->setRange(-150, 150);
    
    label_sticky_power = new QLabel("Força Magnetismo: 0", this);
    slider_sticky_power = new QSlider(Qt::Horizontal, this);
    slider_sticky_power->setRange(0, 2000);

    aimLayout->addWidget(label_v); aimLayout->addWidget(slider_v);
    aimLayout->addWidget(label_sticky_power); aimLayout->addWidget(slider_sticky_power);
    aimGroup->setLayout(aimLayout);
    mainLayout->addWidget(aimGroup);

	// 3. MACROS DE MOVIMENTAÇÃO (ELITE v4.0)
    QGroupBox *macroGroup = new QGroupBox("MACROS DE MOVIMENTO", this);
    macroGroup->setStyleSheet("border: 1px solid #00FF41;");
    QHBoxLayout *mLayout = new QHBoxLayout();
    check_crouch_spam = new QCheckBox("CROUCH SPAM", this);
    check_drop_shot = new QCheckBox("DROP SHOT", this);
    check_sticky_aim = new QCheckBox("STICKY AIM", this);
    check_rapid_fire = new QCheckBox("RAPID FIRE", this);
    mLayout->addWidget(check_crouch_spam); mLayout->addWidget(check_drop_shot);
    macroGroup->setLayout(mLayout);
    mainLayout->addWidget(macroGroup);

    QHBoxLayout *botChecks = new QHBoxLayout();
    botChecks->addWidget(check_sticky_aim); botChecks->addWidget(check_rapid_fire);
    mainLayout->addLayout(botChecks);

	setCentralWidget(central);

	// --- CONEXÕES ---
	connect(slider_v, &QSlider::valueChanged, this, [this](int val){ recoil_v_global = val; label_v->setText(QString("Recoil Vertical: %1").arg(val)); });
    connect(slider_sticky_power, &QSlider::valueChanged, this, [this](int val){ sticky_power_global = val; label_sticky_power->setText(QString("Força Magnetismo: %1").arg(val)); });
	
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

    // ATALHO F1
    auto rapidAction = new QAction(this);
    rapidAction->setShortcut(Qt::Key_F1);
    addAction(rapidAction);
    connect(rapidAction, &QAction::triggered, this, [this](){ check_rapid_fire->setChecked(!check_rapid_fire->isChecked()); });

	grabKeyboard();
	session->Start();
    StartWebBridge(); // Inicia o servidor para o celular
    LoadProfile("GENERIC");
	resize(500, 550); 
	show();
}

// --- PONTE WEB (CONTROLE PELO CELULAR) ---
void StreamWindow::StartWebBridge() {
    web_server = new QTcpServer(this);
    if (web_server->listen(QHostAddress::Any, 8080)) {
        qDebug() << "Ponte Web ativa na porta 8080!";
        connect(web_server, &QTcpServer::newConnection, this, &StreamWindow::OnNewWebConnection);
    }
}

void StreamWindow::OnNewWebConnection() {
    QTcpSocket *socket = web_server->nextPendingConnection();
    connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
        QByteArray data = socket->readAll();
        QString request = QString::fromUtf8(data);
        
        // Exemplo de comando simples vindo do celular: "PROFILE:BERYL"
        if (request.contains("PROFILE:")) {
            QString prof = request.split(":").at(1).trimmed();
            combo_profiles->setCurrentText(prof);
        } else if (request.contains("RF:TOGGLE")) {
            check_rapid_fire->setChecked(!check_rapid_fire->isChecked());
        }

        socket->write("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
        socket->disconnectFromHost();
    });
}

// --- LOGICA DE PERFIL ---
void StreamWindow::SaveProfile(const QString &name) {
    QSettings s("DanielMods", "ZenGhost");
    s.beginGroup(name);
    s.setValue("recoil_v", slider_v->value());
    s.setValue("sticky_power", slider_sticky_power->value());
    s.setValue("crouch", check_crouch_spam->isChecked());
    s.setValue("drop", check_drop_shot->isChecked());
    s.setValue("rapid", check_rapid_fire->isChecked());
    s.endGroup();
}

void StreamWindow::LoadProfile(const QString &name) {
    QSettings s("DanielMods", "ZenGhost");
    s.beginGroup(name);
    slider_v->setValue(s.value("recoil_v", 0).toInt());
    slider_sticky_power->setValue(s.value("sticky_power", 0).toInt());
    check_crouch_spam->setChecked(s.value("crouch", false).toBool());
    check_drop_shot->setChecked(s.value("drop", false).toBool());
    check_rapid_fire->setChecked(s.value("rapid", false).toBool());
    s.endGroup();
}

// Eventos padrão preservados
void StreamWindow::keyPressEvent(QKeyEvent *event) { 
    if (event->key() == Qt::Key_PageUp) slider_v->setValue(slider_v->value() + 1);
    else if (event->key() == Qt::Key_PageDown) slider_v->setValue(slider_v->value() - 1);
    if(session) session->HandleKeyboardEvent(event); 
}
void StreamWindow::keyReleaseEvent(QKeyEvent *event) { if(session) session->HandleKeyboardEvent(event); }
void StreamWindow::mousePressEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event) { ToggleFullscreen(); QMainWindow::mouseDoubleClickEvent(event); }
void StreamWindow::closeEvent(QCloseEvent *event) { if(session) { session->Stop(); } }
void StreamWindow::SessionQuit(ChiakiQuitReason r, const QString &s) { close(); }
void StreamWindow::LoginPINRequested(bool i) { /* Dialog logic... */ }
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
void StreamWindow::UpdateVideoTransform() {}
