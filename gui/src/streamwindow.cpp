// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h> 
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>

// ------------------------------------------------------------------
// LINKER BRIDGE (DANIEL MOD)
// ------------------------------------------------------------------
extern "C" {
    int recoil_v_global = 0; 
    int recoil_h_global = 0;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | Chiaki Ghost Recoil Box");
		
	session = nullptr;
	av_widget = nullptr;
	
	// Reset inicial
	recoil_v_global = 0;
	recoil_h_global = 0;

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
	// CONSTRUÇÃO DA RECOIL BOX (DANIEL MOD)
	// ------------------------------------------------------------------
	QWidget *central = new QWidget(this);
	central->setStyleSheet("background-color: #1a1a1a; color: #00ff00; font-family: 'Consolas';");
	QVBoxLayout *layout = new QVBoxLayout(central);

	// Título
	QLabel *title = new QLabel("DANIEL RECOIL SYSTEM v1.0", this);
	title->setAlignment(Qt::AlignCenter);
	layout->addWidget(title);

	// Slider Vertical
	label_v = new QLabel("Compensação Vertical (Y): 0", this);
	slider_v = new QSlider(Qt::Horizontal, this);
	slider_v->setRange(-150, 150); // Faixa de ajuste para armas do PUBG
	layout->addWidget(label_v);
	layout->addWidget(slider_v);

	// Slider Horizontal
	label_h = new QLabel("Compensação Horizontal (X): 0", this);
	slider_h = new QSlider(Qt::Horizontal, this);
	slider_h->setRange(-100, 100);
	layout->addWidget(label_h);
	layout->addWidget(slider_h);

	setCentralWidget(central);

	// Conectando os Sliders às Variáveis Globais
	connect(slider_v, &QSlider::valueChanged, this, [this](int val){
		recoil_v_global = val;
		label_v->setText(QString("Compensação Vertical (Y): %1").arg(val));
	});

	connect(slider_h, &QSlider::valueChanged, this, [this](int val){
		recoil_h_global = val;
		label_h->setText(QString("Compensação Horizontal (X): %1").arg(val));
	});

	grabKeyboard();
	session->Start();

	auto fs_action = new QAction(tr("Fullscreen"), this);
	fs_action->setShortcut(Qt::Key_F11);
	addAction(fs_action);
	connect(fs_action, &QAction::triggered, this, &StreamWindow::ToggleFullscreen);

	// Tamanho ideal para o seu monitor Xeon
	resize(400, 250); 
	show();
}

void StreamWindow::keyPressEvent(QKeyEvent *event) {
	// Também mantemos o ajuste via teclado para maior agilidade
	if (event->key() == Qt::Key_PageUp) slider_v->setValue(slider_v->value() + 1);
	else if (event->key() == Qt::Key_PageDown) slider_v->setValue(slider_v->value() - 1);
	else if (event->key() == Qt::Key_Home) slider_h->setValue(slider_h->value() + 1);
	else if (event->key() == Qt::Key_End) slider_h->setValue(slider_h->value() - 1);

	if(session) session->HandleKeyboardEvent(event);
}

// ... (Mantenha as demais funções: keyRelease, mousePress, closeEvent iguais ao Ghost Mode anterior)

void StreamWindow::keyReleaseEvent(QKeyEvent *event) { if(session) session->HandleKeyboardEvent(event); }
void StreamWindow::mousePressEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *event) { if(session) session->HandleMouseEvent(event); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event) { ToggleFullscreen(); QMainWindow::mouseDoubleClickEvent(event); }

void StreamWindow::closeEvent(QCloseEvent *event) {
	if(session) {
		if(session->IsConnected()) {
			if(connect_info.settings->GetDisconnectAction() == DisconnectAction::AlwaysSleep)
				session->GoToBed();
		}
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
