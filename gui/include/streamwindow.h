// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>
#include "streamsession.h"

// Forward declarations para manter o build rápido no seu Xeon
class QLabel;
class QSlider;
class QCheckBox;
class QComboBox;
class QPushButton;
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
		// DANIEL ZEN GHOST v3.0: COMPONENTES DA INTERFACE
		// ------------------------------------------------------------------
		
		// 1. Recoil Control
		QLabel  *label_v;
		QLabel  *label_h;
		QSlider *slider_v;
		QSlider *slider_h;

		// 2. Anti-Deadzone
		QLabel  *label_anti_dz;
		QSlider *slider_anti_dz;

		// 3. Magnetismo / Sticky Aim
		QLabel  *label_sticky_power;
		QSlider *slider_sticky_power;

		// 4. Funções Especiais e Indicadores
		QCheckBox *check_sticky_aim;
		QCheckBox *check_rapid_fire;
		QLabel    *label_rapid_status; // Indica visualmente se o Rapid Fire está ON

		// 5. Sistema de Perfis (Salvar/Carregar presets de armas)
		QComboBox   *combo_profiles;   // Lista de armas (M416, Beryl, etc)
		QPushButton *btn_save_profile; // Botão para salvar o preset atual

		// Variáveis de Estado Interno
		int recoil_v = 0;
		int recoil_h = 0;
		bool is_firing = false;

		// Métodos de Lógica de Perfil
		void Init();
		void UpdateVideoTransform();
		void SaveProfile(const QString &name);
		void LoadProfile(const QString &name);

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
