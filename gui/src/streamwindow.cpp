// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL
#include <streamwindow.h>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QSettings>

extern "C" {
    int recoil_v_global = 0, recoil_h_global = 0;
    int anti_dz_global = 0, sticky_power_global = 650;
    int lock_power_global = 100, start_delay_global = 2;
    bool sticky_aim_global = false, rapid_fire_global = false;
}

void StreamWindow::Init() {
    QWidget *central = new QWidget(this);
    central->setStyleSheet("background-color: #050505; color: #00FF41; font-family: 'Consolas';");
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    // Seleção de Arma e Salvar
    QHBoxLayout *pLayout = new QHBoxLayout();
    QComboBox *combo = new QComboBox(this);
    combo->addItems({"M416", "BERYL", "SCAR-L", "GENERIC"});
    QPushButton *btnSave = new QPushButton("SALVAR PERFIL", this);
    btnSave->setStyleSheet("background-color: #004400; border: 1px solid #00FF41;");
    pLayout->addWidget(combo); pLayout->addWidget(btnSave);
    mainLayout->addLayout(pLayout);

    // Ajuste com Números 0-100
    auto addAdjuster = [&](QString name, int min, int max, int *var) {
        QLabel *lab = new QLabel(name + QString(": %1").arg(*var), this);
        QSlider *sld = new QSlider(Qt::Horizontal, this);
        sld->setRange(min, max); sld->setValue(*var);
        connect(sld, &QSlider::valueChanged, [=](int val){ *var = val; lab->setText(name + QString(": %1").arg(val)); });
        mainLayout->addWidget(lab); mainLayout->addWidget(sld);
    };

    addAdjuster("Recoil Vertical (Andar)", 0, 100, &recoil_v_global);
    addAdjuster("Estabilização (Lock)", 0, 100, &lock_power_global);
    addAdjuster("Magnetismo (Sticky)", 0, 1500, &sticky_power_global);

    // Lógica do Botão Salvar
    connect(btnSave, &QPushButton::clicked, [=](){
        QSettings settings("DanielGhost", "ChiakiZen");
        settings.beginGroup(combo->currentText());
        settings.setValue("v", recoil_v_global);
        settings.setValue("lock", lock_power_global);
        settings.endGroup();
    });

    setCentralWidget(central);
    show();
    session->Start();
}

// Funções obrigatórias
void StreamWindow::SessionQuit(ChiakiQuitReason, const QString&) { close(); }
void StreamWindow::OnNewWebConnection() {}
void StreamWindow::ToggleFullscreen() { isFullScreen() ? showNormal() : showFullScreen(); }
