// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>
#include "streamsession.h"

// Forward declarations para agilizar a compilação no seu Xeon E5-2690 v4
class QLabel;
class QSlider;
class QCheckBox;
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
		// DANIEL ZEN GHOST: COMPONENTES DA INTERFACE VISUAL
		// ------------------------------------------------------------------
		
		// 1. Recoil Control (Compensação de recuo)
		QLabel  *label_v;
		QLabel  *label_h;
		QSlider *slider_v;
		QSlider *slider_h;

		// 2. Anti-Deadzone (Vence a resistência inicial do jogo)
		QLabel  *label_anti_dz;
		QSlider *slider_anti_dz;

		// 3. Magnetismo / Sticky Aim (Força do tremor para Aim Assist)
		QLabel  *label_sticky_power;
		QSlider *slider_sticky_power;

		// 4. Funções Especiais (Ativar/Desativar)
		QCheckBox *check_sticky_aim; // Liga/Desliga o Magnetismo
		QCheckBox *check_rapid_fire; // Liga/Desliga o Tiro Rápido

		// Variáveis de Estado Interno
		int recoil_v = 0;
		int recoil_h = 0;
		bool is_firing = false;

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
