// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <streamwindow.h>
#include <streamsession.h>
// #include <avopenglwidget.h> // DANIEL MOD: Não precisamos mais disso
#include <loginpindialog.h>
#include <settings.h>

#include <QLabel>
#include <QMessageBox>
#include <QCoreApplication>
#include <QAction>

StreamWindow::StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent)
	: QMainWindow(parent),
	connect_info(connect_info)
{
	setAttribute(Qt::WA_DeleteOnClose);
	setWindowTitle(qApp->applicationName() + " | Stream (GHOST MODE)"); // Mudei o título pra você saber que funcionou
		
	session = nullptr;
	av_widget = nullptr;

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
	// DANIEL MOD: av_widget será sempre nulo agora, mas é seguro deletar nullptr
	delete av_widget;
}

void StreamWindow::Init()
{
	session = new StreamSession(connect_info, this);

	connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
	connect(session, &StreamSession::LoginPINRequested, this, &StreamWindow::LoginPINRequested);

	// ------------------------------------------------------------------
	// MODIFICAÇÃO DANIEL: REMOÇÃO DA RENDERIZAÇÃO
	// ------------------------------------------------------------------
	// Originalmente aqui ele criava o AVOpenGLWidget (pesado).
	// Agora forçamos sempre o modo "sem vídeo" (tela preta leve).
	
	QWidget *bg_widget = new QWidget(this);
	bg_widget->setStyleSheet("background-color: black;"); // Tela preta leve
	setCentralWidget(bg_widget);
	
	// OBSERVAÇÃO IMPORTANTE:
	// O av_widget fica NULL. Isso impede qualquer chamada OpenGL.
	
	// Mantemos o teclado capturado para seus scripts funcionarem
	grabKeyboard();

	session->Start();

	auto fullscreen_action = new QAction(tr("Fullscreen"), this);
	fullscreen_action->setShortcut(Qt::Key_F11);
	addAction(fullscreen_action);
	connect(fullscreen_action, &QAction::triggered, this, &StreamWindow::ToggleFullscreen);

	// DANIEL MOD: Forçar janela pequena para economizar renderização do Windows
	// Em vez de abrir em 1080p/4K, abrimos pequeno.
	resize(480, 270); 
	show();
}

void StreamWindow::keyPressEvent(QKeyEvent *event)
{
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
					// DANIEL MOD: Removido a pergunta ao fechar para ser mais rápido.
					// Se fechar a janela, desconecta direto sem perguntar.
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
		// DANIEL MOD: Removemos a chamada av_widget->HideMouse()
		// pois o widget não existe mais.
	}
}

void StreamWindow::resizeEvent(QResizeEvent *event)
{
	// DANIEL MOD: Não atualizamos transformação de vídeo pois não tem vídeo
	// UpdateVideoTransform(); 
	QMainWindow::resizeEvent(event);
}

void StreamWindow::moveEvent(QMoveEvent *event)
{
	// DANIEL MOD: Idem acima
	// UpdateVideoTransform();
	QMainWindow::moveEvent(event);
}

void StreamWindow::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::ActivationChange)
		// UpdateVideoTransform();
	QMainWindow::changeEvent(event);
}

void StreamWindow::UpdateVideoTransform()
{
	// DANIEL MOD: Função esvaziada.
	// Não calculamos geometria de vídeo porque não existe vídeo.
}
