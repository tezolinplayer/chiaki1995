// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <streamsession.h>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSettings>

extern "C" {
    int recoil_v_global = 16, recoil_h_global = 0;
    int lock_power_global = 100, start_delay_global = 2, sticky_power_global = 750, anti_dz_global = 0;
    bool sticky_aim_global = false, rapid_fire_global = false;
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas'; font-weight: bold;");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // 1. SELEÇÃO E SALVAR
    QGroupBox *topBox = new QGroupBox("SELEÇÃO DE ARMA", this);
    QHBoxLayout *topLayout = new QHBoxLayout(topBox);
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "AKM"});
    QPushButton *btnSave = new QPushButton("SALVAR", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    topLayout->addWidget(combo); topLayout->addWidget(btnSave);
    mainLayout->addWidget(topBox);

    // 2. CONTROLES COM NÚMEROS (0-100)
    auto addRow = [&](QString name, int min, int max, int *var) {
        QLabel *lab = new QLabel(name + QString(": %1").arg(*var), this);
        QSlider *sld = new QSlider(Qt::Horizontal, this);
        sld->setRange(min, max); sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ *var = val; lab->setText(name + QString(": %1").arg(val)); });
        mainLayout->addWidget(lab); mainLayout->addWidget(sld);
    };

    addRow("Recoil Vertical", 0, 100, &recoil_v_global);
    addRow("Lock Power (Trava)", 0, 100, &lock_power_global);
    addRow("Start Delay (Ticks)", 0, 15, &start_delay_global);
    addRow("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);

    // 3. MACROS
    QHBoxLayout *mLayout = new QHBoxLayout();
    QCheckBox *cbS = new QCheckBox("STICKY AIM", this);
    connect(cbS, &QCheckBox::toggled, [](bool c){ sticky_aim_global = c; });
    mLayout->addWidget(cbS);
    mainLayout->addLayout(mLayout);

    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings s("GhostZen", "Profiles");
        s.setValue(combo->currentText() + "/v", recoil_v_global);
    });

    setCentralWidget(central);
    setWindowTitle("DANIEL GHOST ZEN ELITE | v5.5");
    resize(560, 950);
    show();
    session->Start();
}

// Funções de Linker obrigatórias
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::LoginPINRequested(bool) {}
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
