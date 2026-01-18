// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h> 
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>

// ------------------------------------------------------------------
// LINKER BRIDGE (DANIEL MOD)
// Variáveis globais acessadas pelo motor de controle em C (controller.c)
// ------------------------------------------------------------------
extern "C" {
    int recoil_v_global = 0; 
    int recoil_h_global = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 0; // Força do tremor circular
    bool sticky_aim_global = false;
    bool rapid_fire_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | DANIEL GHOST ZEN v2.5");
		
	session = nullptr;
	av_widget = nullptr;
	
	// Reset inicial das funções para segurança
	recoil_v_global = 0;
	recoil_h_global = 0;
    anti_dz_global = 0;
    sticky_power_global = 0;
    sticky_aim_global = false;
    rapid_fire_global = false;

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
	central->setStyleSheet("background-color: #0F0F0F; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
	QVBoxLayout *layout = new QVBoxLayout(central);

	QLabel *title = new QLabel("=== DANIEL ZEN GHOST PANEL ===", this);
	title->setAlignment(Qt::AlignCenter);
	layout->addWidget(title);

	// 1. Sliders de Recoil (V e H)
	label_v = new QLabel("Recoil Vertical (Y): 0", this);
	slider_v = new QSlider(Qt::Horizontal, this);
	slider_v->setRange(-150, 150);
	layout->addWidget(label_v);
	layout->addWidget(slider_v);

	label_h = new QLabel("Recoil Horizontal (X): 0", this);
	slider_h = new QSlider(Qt::Horizontal, this);
	slider_h->setRange(-100, 100);
	layout->addWidget(label_h);
	layout->addWidget(slider_h);

	// 2. Slider Anti-Deadzone
	label_anti_dz = new QLabel("Anti-Deadzone Force: 0", this);
	slider_anti_dz = new QSlider(Qt::Horizontal, this);
	slider_anti_dz->setRange(0, 6000); 
	layout->addWidget(label_anti_dz);
	layout->addWidget(slider_anti_dz);

    // 3. Slider de Força do Magnetismo (Sticky Power)
    label_sticky_power = new QLabel("Força do Magnetismo (Sticky): 0", this);
    slider_sticky_power = new QSlider(Qt::Horizontal, this);
    slider_sticky_power->setRange(0, 2000); // Faixa ideal para o Aim Assist
    layout->addWidget(label_sticky_power);
    layout->addWidget(slider_sticky_power);

	// 4. Checkboxes de Funções Especiais
    QHBoxLayout *check_layout = new QHBoxLayout();
	check_sticky_aim = new QCheckBox("STICKY AIM", this);
	check_rapid_fire = new QCheckBox("RAPID FIRE", this);
    check_layout->addWidget(check_sticky_aim);
    check_layout->addWidget(check_rapid_fire);
    layout->addLayout(check_layout);

	setCentralWidget(central);

	// --- CONEXÕES DOS SLIDERS ---
	connect(slider_v, &QSlider::valueChanged, this, [this](int val){
		recoil_v_global = val;
		label_v->setText(QString("Recoil Vertical (Y): %1").arg(val));
	});

	connect(slider_h, &QSlider::valueChanged, this, [this](int val){
		recoil_h_global = val;
		label_h->setText(QString("Recoil Horizontal (X): %1").arg(val));
	});

	connect(slider_anti_dz, &QSlider::valueChanged, this, [this](int val){
		anti_dz_global = val;
		label_anti_dz->setText(QString("Anti-Deadzone Force: %1").arg(val));
	});

    connect(slider_sticky_power, &QSlider::valueChanged, this, [this](int val){
        sticky_power_global = val;
        label_sticky_power->setText(QString("Força do Magnetismo (Sticky): %1").arg(val));
    });

	// --- CONEXÕES DAS CHECKBOXES ---
	connect(check_sticky_aim, &QCheckBox::toggled, this, [](bool checked){
		sticky_aim_global = checked;
	});

	connect(check_rapid_fire, &QCheckBox::toggled, this, [](bool checked){
		rapid_fire_global = checked;
	});

	grabKeyboard();
	session->Start();

	resize(480, 420); 
	show();
}

void StreamWindow::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_PageUp) slider_v->setValue(slider_v->value() + 1);
	else if (event->key() == Qt::Key_PageDown) slider_v->setValue(slider_v->value() - 1);
	
	if(session) session->HandleKeyboardEvent(event);
}

void StreamWindow::keyReleaseEvent(QKeyEvent *event) { if(session) session->HandleKeyboardEvent(event); }
void StreamWindow::mousePressEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event) { ToggleFullscreen(); QMainWindow::mouseDoubleClickEvent(event); }

void StreamWindow::closeEvent(QCloseEvent *event) {
	if(session) {
		if(session->IsConnected() && connect_info.settings->GetDisconnectAction() == DisconnectAction::AlwaysSleep)
			session->GoToBed();
		session->Stop();
	}
}

void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str) { close(); }

void StreamWindow::LoginPINRequested(bool incorrect) {
	auto dialog = new LoginPINDialog(incorrect, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
		grabKeyboard();
		if(!session) return;
		if(result == QDialog::Accepted) session->SetLoginPIN(dialog->GetPIN());
		else session->Stop();
	});
	releaseKeyboard();
	dialog->show();
}

void StreamWindow::ToggleFullscreen() {
	if(isFullScreen()) showNormal();
	else showFullScreen();
}

void StreamWindow::resizeEvent(QResizeEvent *event) { QMainWindow::resizeEvent(event); }
void StreamWindow::moveEvent(QMoveEvent *event) { QMainWindow::moveEvent(event); }
void StreamWindow::changeEvent(QEvent *event) { QMainWindow::changeEvent(event); }
void StreamWindow::UpdateVideoTransform() {}
