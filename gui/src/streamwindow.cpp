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
#include <QDebug> // Para ver os valores de recoil no console de debug

// ------------------------------------------------------------------
// VARIÁVEIS GLOBAIS DE RECOIL (DANIEL MOD)
// Essas variáveis são lidas pelo controller.c via 'extern'
// ------------------------------------------------------------------
int recoil_v_global = 0; 
int recoil_h_global = 0;

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | Stream (GHOST MODE)");
		
	session = nullptr;
	av_widget = nullptr;

	// Resetar valores ao abrir a janela
	recoil_v_global = 0;
	recoil_h_global = 0;

	try
	{
		if(connect_info.fullscreen)
			showFullScreen();
		Init();
	}
	catch(const Exception &e)
	{
		QMessageBox::critical(this, tr("Stream failed"), tr("Failed to initialize Stream Session: %1").arg(e.what()));
		close();
	}
}

StreamWindow::~StreamWindow()
{
	if (av_widget) {
		delete av_widget;
	}
}

void StreamWindow::Init()
{
	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	// --- MODO GHOST ATIVADO ---
	// Background preto simples para economizar processamento do Xeon e GPU
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

	// Janela compacta para focar apenas no processamento de input
	resize(480, 270); 
	show();
}

// ------------------------------------------------------------------
// SISTEMA DE AJUSTE DE RECOIL EM TEMPO REAL (DANIEL MOD)
// ------------------------------------------------------------------
void StreamWindow::keyPressEvent(QKeyEvent *event)
{
	// Ajuste Vertical (PageUp aumenta o puxão para baixo, PageDown diminui)
	if (event->key() == Qt::Key_PageUp) {
		recoil_v_global++;
		qDebug() << "Recoil Vertical Ajustado para:" << recoil_v_global;
	} 
	else if (event->key() == Qt::Key_PageDown) {
		recoil_v_global--;
		qDebug() << "Recoil Vertical Ajustado para:" << recoil_v_global;
	}
	
	// Ajuste Horizontal (Home aumenta para direita, End aumenta para esquerda)
	else if (event->key() == Qt::Key_Home) {
		recoil_h_global++;
		qDebug() << "Recoil Horizontal Ajustado para:" << recoil_h_global;
	}
	else if (event->key() == Qt::Key_End) {
		recoil_h_global--;
		qDebug() << "Recoil Horizontal Ajustado para:" << recoil_h_global;
	}

	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::keyReleaseEvent(QKeyEvent *event)
{
	if(session)
		session->HandleKeyboardEvent(event);
}

void StreamWindow::mousePressEvent(QMouseEvent *event)
{
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::mouseReleaseEvent(QMouseEvent *event)
{
	if(session)
		session->HandleMouseEvent(event);
}

void StreamWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
	ToggleFullscreen();
	QMainWindow::mouseDoubleClickEvent(event);
}

void StreamWindow::closeEvent(QCloseEvent *event)
{
	if(session)
	{
		if(session->IsConnected())
		{
			bool sleep = false;
			switch(connect_info.settings->GetDisconnectAction())
			{
				case DisconnectAction::Ask: {
					sleep = false; 
					break;
				}
				case DisconnectAction::AlwaysSleep:
					sleep = true;
					break;
				default:
					break;
			}
			if(sleep)
				session->GoToBed();
		}
		session->Stop();
	}
}

void StreamWindow::SessionQuit(ChiakiQuitReason reason, const QString &reason_str)
{
	if(reason != CHIAKI_QUIT_REASON_STOPPED)
	{
		QString m = tr("Chiaki Session has quit") + ":\n" + chiaki_quit_reason_string(reason);
		if(!reason_str.isEmpty())
			m += "\n" + tr("Reason") + ": \"" + reason_str + "\"";
		QMessageBox::critical(this, tr("Session has quit"), m);
	}
	close();
}

void StreamWindow::LoginPINRequested(bool incorrect)
{
	auto dialog = new LoginPINDialog(incorrect, this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(dialog, &QDialog::finished, this, [this, dialog](int result) {
		grabKeyboard();

		if(!session)
			return;

		if(result == QDialog::Accepted)
			session->SetLoginPIN(dialog->GetPIN());
		else
			session->Stop();
	});
	releaseKeyboard();
	dialog->show();
}

void StreamWindow::ToggleFullscreen()
{
	if(isFullScreen())
		showNormal();
	else
	{
		showFullScreen();
	}
}

void StreamWindow::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
}

void StreamWindow::moveEvent(QMoveEvent *event)
{
	QMainWindow::moveEvent(event);
}

void StreamWindow::changeEvent(QEvent *event)
{
	QMainWindow::changeEvent(event);
}

void StreamWindow::UpdateVideoTransform()
{
    // Ghost Mode: Sem vídeo para transformar.
}
