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
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>
#include <QSettings>

// ------------------------------------------------------------------
// LINKER BRIDGE (DANIEL MOD)
// Variáveis globais acessadas pelo motor de controle em C (controller.c)
// ------------------------------------------------------------------
extern "C" {
    int recoil_v_global = 0; 
    int recoil_h_global = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 0;
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | DANIEL GHOST ZEN v3.0");
		
	session = nullptr;
	av_widget = nullptr;
	
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
}

void StreamWindow::Init() {
	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	// ------------------------------------------------------------------
	// CONSTRUÇÃO DA INTERFACE ZEN GHOST (DANIEL MOD)
	// ------------------------------------------------------------------
	QWidget *central = new QWidget(this);
	central->setStyleSheet("background-color: #0A0A0A; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
	QVBoxLayout *layout = new QVBoxLayout(central);

	// Título e Status do Rapid Fire
    QHBoxLayout *header_layout = new QHBoxLayout();
	QLabel *title = new QLabel("DANIEL ZEN GHOST PANEL", this);
    label_rapid_status = new QLabel("[RF: OFF]", this);
    label_rapid_status->setStyleSheet("color: gray;");
    header_layout->addWidget(title);
    header_layout->addStretch();
    header_layout->addWidget(label_rapid_status);
	layout->addLayout(header_layout);

    // --- SISTEMA DE PERFIS ---
    QHBoxLayout *profile_layout = new QHBoxLayout();
    combo_profiles = new QComboBox(this);
    combo_profiles->addItems({"M416", "BERYL", "MINI-14", "SKS", "GENERIC"});
    combo_profiles->setStyleSheet("background-color: #222; color: #00FF41; border: 1px solid #00FF41;");
    
    btn_save_profile = new QPushButton("SALVAR PERFIL", this);
    btn_save_profile->setStyleSheet("background-color: #004400; color: white; border: 1px solid #00FF41; padding: 5px;");
    
    profile_layout->addWidget(new QLabel("PERFIL:", this));
    profile_layout->addWidget(combo_profiles);
    profile_layout->addWidget(btn_save_profile);
    layout->addLayout(profile_layout);

	// 1. Sliders de Recoil
	label_v = new QLabel("Recoil Vertical: 0", this);
	slider_v = new QSlider(Qt::Horizontal, this);
	slider_v->setRange(-150, 150);
	layout->addWidget(label_v);
	layout->addWidget(slider_v);

	label_h = new QLabel("Recoil Horizontal: 0", this);
	slider_h = new QSlider(Qt::Horizontal, this);
	slider_h->setRange(-100, 100);
	layout->addWidget(label_h);
	layout->addWidget(slider_h);

	// 2. Anti-Deadzone e Sticky Power
	label_anti_dz = new QLabel("Anti-Deadzone: 0", this);
	slider_anti_dz = new QSlider(Qt::Horizontal, this);
	slider_anti_dz->setRange(0, 6000); 
	layout->addWidget(label_anti_dz);
	layout->addWidget(slider_anti_dz);

    label_sticky_power = new QLabel("Magnetismo (Sticky): 0", this);
    slider_sticky_power = new QSlider(Qt::Horizontal, this);
    slider_sticky_power->setRange(0, 2000);
    layout->addWidget(label_sticky_power);
    layout->addWidget(slider_sticky_power);

	// 3. Checkboxes
    QHBoxLayout *check_layout = new QHBoxLayout();
	check_sticky_aim = new QCheckBox("STICKY AIM", this);
	check_rapid_fire = new QCheckBox("RAPID FIRE", this);
    check_layout->addWidget(check_sticky_aim);
    check_layout->addWidget(check_rapid_fire);
    layout->addLayout(check_layout);

	setCentralWidget(central);

	// --- CONEXÕES ---
	connect(slider_v, &QSlider::valueChanged, this, [this](int val){ recoil_v_global = val; label_v->setText(QString("Recoil Vertical: %1").arg(val)); });
	connect(slider_h, &QSlider::valueChanged, this, [this](int val){ recoil_h_global = val; label_h->setText(QString("Recoil Horizontal: %1").arg(val)); });
	connect(slider_anti_dz, &QSlider::valueChanged, this, [this](int val){ anti_dz_global = val; label_anti_dz->setText(QString("Anti-Deadzone: %1").arg(val)); });
    connect(slider_sticky_power, &QSlider::valueChanged, this, [this](int val){ sticky_power_global = val; label_sticky_power->setText(QString("Magnetismo: %1").arg(val)); });

	connect(check_sticky_aim, &QCheckBox::toggled, this, [](bool checked){ sticky_aim_global = checked; });
	
    connect(check_rapid_fire, &QCheckBox::toggled, this, [this](bool checked){ 
        rapid_fire_global = checked; 
        label_rapid_status->setText(checked ? "[RAPID FIRE ON]" : "[RF: OFF]");
        label_rapid_status->setStyleSheet(checked ? "color: red;" : "color: gray;");
    });

    connect(combo_profiles, &QComboBox::currentTextChanged, this, &StreamWindow::LoadProfile);
    connect(btn_save_profile, &QPushButton::clicked, this, [this](){ SaveProfile(combo_profiles->currentText()); });

    // --- HOTKEYS ---
    auto rapid_toggle_action = new QAction(this);
    rapid_toggle_action->setShortcut(Qt::Key_F1);
    addAction(rapid_toggle_action);
    connect(rapid_toggle_action, &QAction::triggered, this, [this](){ check_rapid_fire->setChecked(!check_rapid_fire->isChecked()); });

	grabKeyboard();
	session->Start();
    
    LoadProfile("GENERIC"); // Carrega perfil inicial
	resize(500, 500); 
	show();
}

void StreamWindow::SaveProfile(const QString &name) {
    QSettings settings("DanielMods", "ZenGhost");
    settings.beginGroup(name);
    settings.setValue("recoil_v", slider_v->value());
    settings.setValue("recoil_h", slider_h->value());
    settings.setValue("anti_dz", slider_anti_dz->value());
    settings.setValue("sticky_power", slider_sticky_power->value());
    settings.setValue("sticky_aim", check_sticky_aim->isChecked());
    settings.setValue("rapid_fire", check_rapid_fire->isChecked());
    settings.endGroup();
    qDebug() << "Perfil" << name << "Salvo!";
}

void StreamWindow::LoadProfile(const QString &name) {
    QSettings settings("DanielMods", "ZenGhost");
    settings.beginGroup(name);
    slider_v->setValue(settings.value("recoil_v", 0).toInt());
    slider_h->setValue(settings.value("recoil_h", 0).toInt());
    slider_anti_dz->setValue(settings.value("anti_dz", 0).toInt());
    slider_sticky_power->setValue(settings.value("sticky_power", 0).toInt());
    check_sticky_aim->setChecked(settings.value("sticky_aim", false).toBool());
    check_rapid_fire->setChecked(settings.value("rapid_fire", false).toBool());
    settings.endGroup();
    qDebug() << "Perfil" << name << "Carregado!";
}

void StreamWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_PageUp) slider_v->setValue(slider_v->value() + 1);
	else if (event->key() == Qt::Key_PageDown) slider_v->setValue(slider_v->value() - 1);
	if(session) session->HandleKeyboardEvent(event);
}

// Funções de evento padrão preservadas
void StreamWindow::keyReleaseEvent(QKeyEvent *event) { if(session) session->HandleKeyboardEvent(event); }
void StreamWindow::mousePressEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event) { ToggleFullscreen(); QMainWindow::mouseDoubleClickEvent(event); }
void StreamWindow::closeEvent(QCloseEvent *event) { if(session) { if(session->IsConnected() && connect_info.settings->GetDisconnectAction() == DisconnectAction::AlwaysSleep) session->GoToBed(); session->Stop(); } }
void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str) { close(); }
void StreamWindow::LoginPINRequested(bool incorrect) { auto dialog = new LoginPINDialog(incorrect, this); dialog->setAttribute(Qt::WA_DeleteOnClose); connect(dialog, &QDialog::finished, this, [this, dialog](int result) { grabKeyboard(); if(!session) return; if(result == QDialog::Accepted) session->SetLoginPIN(dialog->GetPIN()); else session->Stop(); }); releaseKeyboard(); dialog->show(); }
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
void StreamWindow::resizeEvent(QResizeEvent *event) { QMainWindow::resizeEvent(event); }
void StreamWindow::moveEvent(QMoveEvent *event) { QMainWindow::moveEvent(event); }
void StreamWindow::changeEvent(QEvent *event) { QMainWindow::changeEvent(event); }
void StreamWindow::UpdateVideoTransform() {}
