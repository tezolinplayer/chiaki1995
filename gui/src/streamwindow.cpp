// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
#include <avopenglwidget.h> 
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>
#include <QDebug>

// ------------------------------------------------------------------
// LINKER BRIDGE (DANIEL MOD)
// O bloco extern "C" é obrigatório para o controller.c (C) encontrar 
// estas variáveis definidas aqui no C++.
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
	setWindowTitle(qApp->applicationName() + " | Stream (GHOST MODE)");
		
	session = nullptr;
	av_widget = nullptr;

	// Reset de segurança
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
	if (av_widget) {
		delete av_widget;
	}
}

void StreamWindow::Init() {
	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	// --- MODO GHOST: TELA PRETA ULTRA LEVE ---
	QWidget *bg_widget = new QWidget(this);
	bg_widget->setStyleSheet("background-color: black;");
	setCentralWidget(bg_widget);
	av_widget = nullptr; 
	
	grabKeyboard();
	session->Start();

	auto fullscreen_action = new QAction(tr("Fullscreen"), this);
	fullscreen_action->setShortcut(Qt::Key_F11);
	addAction(fullscreen_action);
	connect(fullscreen_action, &QAction::triggered, this, &StreamWindow::ToggleFullscreen);

	resize(480, 270); 
	show();
}

void StreamWindow::keyPressEvent(QKeyEvent *event) {
	// --- CONTROLES DE RECOIL EM TEMPO REAL ---
	if (event->key() == Qt::Key_PageUp) {
		recoil_v_global++;
	} 
	else if (event->key() == Qt::Key_PageDown) {
		recoil_v_global--;
	}
	else if (event->key() == Qt::Key_Home) {
		recoil_h_global++;
	}
	else if (event->key() == Qt::Key_End) {
		recoil_h_global--;
	}

	// Mostra o valor no console para calibração
	qDebug() << "RECOIL -> Vertical:" << recoil_v_global << "| Horizontal:" << recoil_h_global;

	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::keyReleaseEvent(QKeyEvent *event) {
	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::mousePressEvent(QMouseEvent *event) {
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::mouseReleaseEvent(QMouseEvent *event) {
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	ToggleFullscreen();
	QMainWindow::mouseDoubleClickEvent(event);
}

void StreamWindow::closeEvent(QCloseEvent *event) {
	if(session) {
		if(session->IsConnected()) {
			if(connect_info.settings->GetDisconnectAction() == DisconnectAction::AlwaysSleep)
				session->GoToBed();
		}
		session->Stop();
	}
}

void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str) {
	close();
}

void StreamWindow::LoginPINRequested(bool incorrect) {
	auto dialog = new LoginPINDialog(incorrect, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
		grabKeyboard();
		if(!session) return;
		if(result == QDialog::Accepted)
			session->SetLoginPIN(dialog->GetPIN());
		else
			session->Stop();
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
