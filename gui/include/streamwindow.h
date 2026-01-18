// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_GUI_STREAMWINDOW_H
#define CHIAKI_GUI_STREAMWINDOW_H

#include <QMainWindow>
#include "streamsession.h"

// Forward declarations para manter o build rápido no seu Xeon E5-2690 v4
class QLabel;
class QSlider;
class QCheckBox;
class QComboBox;
class QPushButton;
class QTcpServer;      // Necessário para a Ponte Web (Celular)
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
		// DANIEL ZEN GHOST v4.6: COMPONENTES DA INTERFACE
		// ------------------------------------------------------------------
		
		// 1. Recoil Control (Compensação de Recuo Básica)
		QLabel  *label_v;
		QLabel  *label_h;
		QSlider *slider_v;
		QSlider *slider_h;

		// 2. Anti-Deadzone e Magnetismo (Aim Assist)
		QLabel  *label_anti_dz;
		QSlider *slider_anti_dz;
		QLabel  *label_sticky_power;
		QSlider *slider_sticky_power;

		// 3. RECOIL PERSONALIZADO (XIM Matrix Style)
		QLabel  *label_lock_power;   // Slider para a "Trava" de mira (ex: 1.60x)
		QSlider *slider_lock_power;
		QLabel  *label_start_delay;  // Slider para o tempo de espera antes do recoil
		QSlider *slider_start_delay;

		// 4. Macros de Movimentação (Elite Tech)
		QCheckBox *check_crouch_spam; // Agachar/Levantar rápido
		QCheckBox *check_drop_shot;   // Deitar ao atirar

		// 5. Funções Especiais e Indicadores
		QCheckBox *check_sticky_aim;
		QCheckBox *check_rapid_fire;
		QLabel    *label_rapid_status; // Indicador visual na tela

		// 6. Sistema de Perfis (Salvar/Carregar presets de armas)
		QComboBox   *combo_profiles;
		QPushButton *btn_save_profile;

		// 7. Ponte Web (Controle Remoto via Celular)
		QTcpServer *web_server;

		// Variáveis de Estado Interno
		int recoil_v = 0;
		int recoil_h = 0;
		bool is_firing = false;

		// Métodos de Lógica
		void Init();
		void UpdateVideoTransform();
		void SaveProfile(const QString &name);
		void LoadProfile(const QString &name);
		void StartWebBridge(); // Inicia o servidor para o celular

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
		void OnNewWebConnection(); // Trata comandos vindos do celular
};

#endif // CHIAKI_GUI_STREAMWINDOW_H
