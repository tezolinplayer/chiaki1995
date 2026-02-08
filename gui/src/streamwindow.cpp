// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <streamsession.h>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QSettings>

// --- DEFINIÇÃO DAS VARIÁVEIS (O MOTOR PRECISA DISSO PARA O RECOIL FUNCIONAR) ---
extern "C" {
    int v_stage1 = 0, h_stage1 = 0;
    int v_stage2 = 0, h_stage2 = 0;
    int v_stage3 = 0, h_stage3 = 0;
    int anti_dz_global = 0;
    int sticky_power_global = 750;
    int lock_power_global = 100;
    int start_delay_global = 2;
    bool sticky_aim_global = false;
}

StreamWindow::StreamWindow(const StreamSessionConnectInfo &info, QWidget *parent) 
    : QMainWindow(parent), connect_info(info) 
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5 (FINAL)");
    
    // Configuração de Foco (Essencial para WASD)
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    session = new StreamSession(info, this);
    connect(session, &StreamSession::SessionQuit, this, &StreamWindow::SessionQuit);
    Init();
}

StreamWindow::~StreamWindow() {
    if (session) session->Stop();
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 1. SELEÇÃO DE ARMA
    QGroupBox *boxProfile = new QGroupBox("PERFIL DA ARMA", this);
    QHBoxLayout *layProfile = new QHBoxLayout(boxProfile);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "AKM", "M16A4", "GROZA", "GENERIC"});
    combo->setStyleSheet("background-color: #111; color: #00FF41; border: 1px solid #00FF41; padding: 5px;");
    
    QPushButton *btnSave = new QPushButton("💾 SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    
    QPushButton *btnLoad = new QPushButton("📂 CARREGAR", this);
    btnLoad->setStyleSheet("background-color: #004400; border: 1px solid #00FF41; padding: 5px;");
    
    layProfile->addWidget(combo); 
    layProfile->addWidget(btnLoad); 
    layProfile->addWidget(btnSave);
    mainLayout->addWidget(boxProfile);

    // 2. RECOIL 3 ESTÁGIOS (Conecta com as variáveis do controller.c)
    auto addStage = [&](QString txt, int *v, int *h) {
        QGroupBox *group = new QGroupBox(txt, this);
        QVBoxLayout *vbox = new QVBoxLayout(group);
        
        // Slider Vertical
        QHBoxLayout *rowV = new QHBoxLayout();
        QLabel *lblV = new QLabel(QString("Vertical: %1").arg(*v)); 
        lblV->setFixedWidth(100);
        QSlider *sldV = new QSlider(Qt::Horizontal); 
        sldV->setRange(0, 100); 
        sldV->setValue(*v);
        sldV->setStyleSheet("QSlider::handle:horizontal { background: #00FF41; width: 12px; }");
        connect(sldV, &QSlider::valueChanged, [=](int val){ 
            *v = val; 
            lblV->setText(QString("Vertical: %1").arg(val)); 
        });
        rowV->addWidget(lblV); 
        rowV->addWidget(sldV);
        
        // Slider Horizontal
        QHBoxLayout *rowH = new QHBoxLayout();
        QLabel *lblH = new QLabel(QString("Horizontal: %1").arg(*h)); 
        lblH->setFixedWidth(100);
        QSlider *sldH = new QSlider(Qt::Horizontal); 
        sldH->setRange(-100, 100); 
        sldH->setValue(*h);
        sldH->setStyleSheet("QSlider::handle:horizontal { background: #FF4100; width: 12px; }");
        connect(sldH, &QSlider::valueChanged, [=](int val){ 
            *h = val; 
            lblH->setText(QString("Horizontal: %1").arg(val)); 
        });
        rowH->addWidget(lblH); 
        rowH->addWidget(sldH);
        
        vbox->addLayout(rowV);
        vbox->addLayout(rowH);
        mainLayout->addWidget(group);
    };

    addStage("⚡ ESTÁGIO 1 - KICK INICIAL (0-300ms)", &v_stage1, &h_stage1);
    addStage("🎯 ESTÁGIO 2 - CONTROLE (300-800ms)", &v_stage2, &h_stage2);
    addStage("🔒 ESTÁGIO 3 - LOCK FINAL (+800ms)", &v_stage3, &h_stage3);

    // 3. CONFIGURAÇÕES EXTRAS
    QGroupBox *boxExtras = new QGroupBox("⚙️ CONFIGURAÇÕES AVANÇADAS", this);
    QVBoxLayout *vboxExtras = new QVBoxLayout(boxExtras);
    
    auto addExtra = [&](QString txt, int min, int max, int *var) {
        QHBoxLayout *row = new QHBoxLayout();
        QLabel *lbl = new QLabel(txt + QString(": %1").arg(*var));
        lbl->setFixedWidth(180);
        QSlider *sld = new QSlider(Qt::Horizontal); 
        sld->setRange(min, max); 
        sld->setValue(*var);
        sld->setStyleSheet("QSlider::handle:horizontal { background: #00FF41; width: 12px; }");
        connect(sld, &QSlider::valueChanged, [=](int val){ 
            *var = val; 
            lbl->setText(txt + QString(": %1").arg(val)); 
        });
        row->addWidget(lbl); 
        row->addWidget(sld);
        vboxExtras->addLayout(row);
    };
    
    addExtra("Lock Power (%)", 0, 100, &lock_power_global);
    addExtra("Magnetismo", 0, 1500, &sticky_power_global);
    addExtra("Delay Inicial (ticks)", 0, 15, &start_delay_global);
    
    mainLayout->addWidget(boxExtras);

    // 4. ATIVADORES
    QGroupBox *boxToggles = new QGroupBox("🔘 ATIVADORES", this);
    QVBoxLayout *vboxToggles = new QVBoxLayout(boxToggles);
    
    QCheckBox *cbSticky = new QCheckBox("STICKY AIM (Magnetismo ao mirar L2)", this);
    cbSticky->setStyleSheet("QCheckBox::indicator { width: 20px; height: 20px; }");
    connect(cbSticky, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    vboxToggles->addWidget(cbSticky);
    
    mainLayout->addWidget(boxToggles);

    // 5. INFO E STATUS
    QLabel *info = new QLabel(
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
        "📌 COMO USAR:\n"
        "• Ajuste os 3 estágios de recoil\n"
        "• Lock Power controla intensidade final\n"
        "• Magnetismo atrai mira ao L2\n"
        "• Delay = quanto tempo até recoil iniciar\n"
        "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    );
    info->setStyleSheet("color: #888; font-size: 10px;");
    info->setWordWrap(true);
    mainLayout->addWidget(info);

    // LÓGICA SALVAR
    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        QString profile = combo->currentText();
        s.setValue(profile + "/v1", v_stage1);
        s.setValue(profile + "/h1", h_stage1);
        s.setValue(profile + "/v2", v_stage2);
        s.setValue(profile + "/h2", h_stage2);
        s.setValue(profile + "/v3", v_stage3);
        s.setValue(profile + "/h3", h_stage3);
        s.setValue(profile + "/lock", lock_power_global);
        s.setValue(profile + "/sticky", sticky_power_global);
        s.setValue(profile + "/delay", start_delay_global);
        
        btnSave->setText("✅ SALVO!");
        QTimer::singleShot(1500, [=](){ btnSave->setText("💾 SALVAR"); });
    });

    // LÓGICA CARREGAR
    connect(btnLoad, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        QString profile = combo->currentText();
        
        v_stage1 = s.value(profile + "/v1", 0).toInt();
        h_stage1 = s.value(profile + "/h1", 0).toInt();
        v_stage2 = s.value(profile + "/v2", 0).toInt();
        h_stage2 = s.value(profile + "/h2", 0).toInt();
        v_stage3 = s.value(profile + "/v3", 0).toInt();
        h_stage3 = s.value(profile + "/h3", 0).toInt();
        lock_power_global = s.value(profile + "/lock", 100).toInt();
        sticky_power_global = s.value(profile + "/sticky", 750).toInt();
        start_delay_global = s.value(profile + "/delay", 2).toInt();
        
        // Atualiza visual (força recriação da UI ou use pointers aos widgets)
        btnLoad->setText("✅ CARREGADO!");
        QTimer::singleShot(1500, [=](){ btnLoad->setText("📂 CARREGAR"); });
    });

    setCentralWidget(central);
    resize(650, 800);
    show();
    session->Start();
}

// --- FUNÇÕES DE EVENTO ---
void StreamWindow::keyPressEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::keyReleaseEvent(QKeyEvent *e) { if(session) session->HandleKeyboardEvent(e); }
void StreamWindow::mousePressEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseReleaseEvent(QMouseEvent *e) { if(session) session->HandleMouseEvent(e); }
void StreamWindow::mouseDoubleClickEvent(QMouseEvent *e) { ToggleFullscreen(); }
void StreamWindow::moveEvent(QMoveEvent *e) { QMainWindow::moveEvent(e); }
void StreamWindow::resizeEvent(QResizeEvent *e) { QMainWindow::resizeEvent(e); }
void StreamWindow::changeEvent(QEvent *e) { QMainWindow::changeEvent(e); }
void StreamWindow::closeEvent(QCloseEvent *e) { if(session) session->Stop(); }

// STUBS DO SISTEMA
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { if(isFullScreen()) showNormal(); else showFullScreen(); }
