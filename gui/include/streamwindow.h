// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>

#include "streamsession.h"

// Forward declarations para agilizar a compilação no seu Xeon
class QLabel;
class QSlider;
class AVOpenGLWidget;

class StreamWindow: public QMainWindow
{
	Q_OBJECT

	public:
		explicit StreamWindow(const StreamSessionConnectInfo &connect_info, QWidget *parent = nullptr);
		~StreamWindow() override;

	private:
		const StreamSessionConnectInfo connect_info;
		StreamSession *session;

		AVOpenGLWidget *av_widget;

		// ------------------------------------------------------------------
		// DANIEL MOD: COMPONENTES DA RECOIL BOX
		// ------------------------------------------------------------------
		QLabel *label_v;        // Texto que mostra o valor Vertical
		QLabel *label_h;        // Texto que mostra o valor Horizontal
		QSlider *slider_v;      // Barra de ajuste para Recoil Vertical
		QSlider *slider_h;      // Barra de ajuste para Recoil Horizontal
		
		int recoil_v = 0;       // Backup local do valor Vertical
		int recoil_h = 0;       // Backup local do valor Horizontal
		bool is_firing = false; // Estado do gatilho R2

		void Init();
		void UpdateVideoTransform();

	protected:
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void closeEvent(QCloseEvent *event) override;
		void mousePressEvent(QMouseEvent *event) override;
		void mouseReleaseEvent(QMouseEvent *event) override;
		void mouseDoubleClickEvent(QMouseEvent *event) override;
		void resizeEvent(QResizeEvent *event) override;
		void moveEvent(QMoveEvent *event) override;
		void changeEvent(QEvent *event) override;

	private slots:
		void SessionQuit(ChiakiQuitReason reason, const QString &reason_str);
		void LoginPINRequested(bool incorrect);
		void ToggleFullscreen();
};

#endif // CHIAKI_GUI_STREAMWINDOW_H
