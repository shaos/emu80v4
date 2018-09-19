﻿/*
 *  Emu80 v. 4.x
 *  © Viktor Pykhonin <pyk@mail.ru>, 2017-2018
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QSettings>
#include <QMessageBox>

#include "qtSettingsDialog.h"
#include "ui_qtSettingsDialog.h"

#include "qtMainWindow.h"

#include "../EmuCalls.h"


SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    m_mainWindow = (MainWindow*)parent;
    m_platform = m_mainWindow->getPlatformObjectName();
}


SettingsDialog::~SettingsDialog()
{
    delete ui;
}


void SettingsDialog::initConfig()
{
    clearConfig();
    readRunningConfig();
    writeInitialSavedConfig();
    loadSavedConfig();
    saveRunningConfig();
    fillControlValues();
}


void SettingsDialog::updateConfig()
{
    readRunningConfig();
    fillControlValues();
}


void SettingsDialog::saveConfig()
{
    saveStoredConfig();
}


void SettingsDialog::execute()
{
    updateConfig();
    show();
}


QString SettingsDialog::getRunningConfigValue(QString option)
{
    int dotPos = option.lastIndexOf(".");
    std::string obj = option.mid(0, dotPos).toUtf8().constData();
    std::string prop = option.mid(dotPos + 1).toUtf8().constData();

    if (obj != "emulation" && obj != "wavReader")
        return QString::fromUtf8(emuGetPropertyValue(m_platform + "." + obj, prop).c_str());
    else
        return QString::fromUtf8(emuGetPropertyValue(obj, prop).c_str());
}


void SettingsDialog::setRunningConfigValue(QString option, QString value)
{
    int dotPos = option.lastIndexOf(".");
    std::string obj = option.mid(0, dotPos).toUtf8().constData();
    std::string prop = option.mid(dotPos + 1).toUtf8().constData();
    std::string val = value.toUtf8().constData();

    if (obj != "emulation" && obj != "wavReader")
        emuSetPropertyValue(m_platform + "." + obj, prop, val);
    else
        emuSetPropertyValue(obj, prop, val);
}


void SettingsDialog::loadRunningConfigValue(QString option)
{
    QString value = getRunningConfigValue(option);
    if (value != "")
        m_options[option] = value;
}


// Очищает настройки в m_options
void SettingsDialog::clearConfig()
{
    m_options.clear();
}


// Считывает текущие работающие настройки в m_options
void SettingsDialog::readRunningConfig()
{
    m_platform = m_mainWindow->getPlatformObjectName();
    m_platformGroup = QString::fromUtf8(m_mainWindow->getPlatformGroupName().c_str());

    loadRunningConfigValue("emulation.volume");
    loadRunningConfigValue("window.windowStyle");
    loadRunningConfigValue("window.frameScale");
    loadRunningConfigValue("window.antialiasing");
    loadRunningConfigValue("window.aspectCorrection");
    loadRunningConfigValue("window.fieldsMixing");
    loadRunningConfigValue("window.defaultWindowWidth");
    loadRunningConfigValue("window.defaultWindowHeight");
    loadRunningConfigValue("cpu.debugOnHalt");
    loadRunningConfigValue("cpu.debugOnIllegalCmd");
    // add mnemonics case options here
    loadRunningConfigValue("crtRenderer.colorMode");
    loadRunningConfigValue("crtRenderer.altRenderer");
    loadRunningConfigValue("crtRenderer.visibleArea");
    loadRunningConfigValue("wavReader.channel");
    loadRunningConfigValue("wavReader.speedUpFactor");
    loadRunningConfigValue("tapeGrp.enabled");
    loadRunningConfigValue("tapeInHook.suspendAfterResetForMs");
    loadRunningConfigValue("loader.allowMultiblock");
    loadRunningConfigValue("kbdLayout.layout");
    loadRunningConfigValue("keyboard.matrix");
}


// Обновляет ini-файл, записывая из m_options значения настроек, которых еще нет в ini-файле
void SettingsDialog::writeInitialSavedConfig()
{
    m_initialOptions = m_options;

    QSettings settings;

    settings.beginGroup(m_platformGroup);
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (value != "" && option != "locale" /*&& option != "glDriver"*/ &&
                option != "maxFps" && option != "sampleRate" && option != "vsync" && option != "limitFps" &&
                !settings.contains(option))
            settings.setValue(option, value);
    }
    settings.endGroup();
}


// Загружает в m_options настройки из ini-файла
void SettingsDialog::loadSavedConfig()
{
    QSettings settings;

    settings.beginGroup("system");
    m_options["locale"] = settings.value("locale").toString();
    //m_options["glDriver"] = settings.value("glDriver").toString();
    m_options["maxFps"] = settings.value("maxFps").toString();
    m_options["limitFps"] = settings.value("limitFps").toString();
    m_options["vsync"] = settings.value("vsync").toString();
    m_options["sampleRate"] = settings.value("sampleRate").toString();
    settings.endGroup();

    settings.beginGroup(m_platformGroup);
    foreach (QString option, m_options.keys()) {
        if (settings.contains(option))
            m_options[option] = settings.value(option).toString();
    }
    settings.endGroup();
}


// Устанавливает параметры элементов оправления в соответствии с m_options
void SettingsDialog::fillControlValues()
{
    QString val;

    // Language
    val = m_options["locale"];
    if (val == "en")
        ui->langComboBox->setCurrentIndex(1);
    else if (val == "ru")
        ui->langComboBox->setCurrentIndex(2);
    else // if (val == "system")
        ui->langComboBox->setCurrentIndex(0);

    // OpenGL driver
    /*val = m_options["glDriver"];
    ui->driverComboBox->setCurrentIndex(val == "es" ? 1 : 0);*/

    // Frame rate
    int fr = m_options["maxFps"].toInt();
    ui->fpsSpinBox->setValue(fr);

    // Frame rate limitation
    bool limitFps = m_options["limitFps"] == "true";
    ui->maxFpsCheckBox->setChecked(!limitFps);

    // VSync
    val = m_options["vsync"];
    ui->vsyncCheckBox->setChecked(val == "yes");

    // Sample rate
    val = m_options["sampleRate"];
    ui->srComboBox->setCurrentText(val);

    // Volume
    val = m_options["emulation.volume"];
    int volume = val.toInt();
    ui->muteCheckBox->setVisible(false); //временно!
    ui->volumeSpinBox->setValue(volume);
    ui->volumeSlider->setValue(volume);

    // Window style
    val = m_options["window.windowStyle"];
    ui->autoSizeRadioButton->setChecked(val == "autosize");
    ui->userSizeRadioButton->setChecked(val == "sizable");
    ui->fixedSizeRadioButton->setChecked(val == "fixed");
    bool fixed = ui->fixedSizeRadioButton->isChecked();
    ui->widthLineEdit->setEnabled(fixed);
    ui->heightLineEdit->setEnabled(fixed);

    ui->widthLineEdit->setText(m_options["window.defaultWindowWidth"]);
    ui->heightLineEdit->setText(m_options["window.defaultWindowHeight"]);

    // Scaling
    val = m_options["window.frameScale"];
    if (val == "1x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleSpinBox->setValue(1);
    } else if (val == "2x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleSpinBox->setValue(2);
    } else if (val == "3x") {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleSpinBox->setValue(3);
    } else {
        ui->fixedScaleRadioButton->setChecked(false);
        ui->stretchRadioButton->setChecked(val == "fit");
        ui->stretchPropRadioButton->setChecked(val == "fitKeepAR");
        ui->stretchPropIntRadioButton->setChecked(val == "bestfit");
    }

    // Smoothing
    val = m_options["window.antialiasing"];
    ui->smoothCheckBox->setChecked(val == "yes");

    // Aspect ratio
    val = m_options["window.aspectCorrection"];
    ui->aspectCheckBox->setChecked(val == "yes");

    // Debug on HLT
    val = m_options["cpu.debugOnHalt"];
    ui->debugHltCheckBox->setChecked(val == "yes");

    // Debug on illegal instruction
    val = m_options["cpu.debugOnIllegalCmd"];
    ui->debugIllegalCheckBox->setChecked(val == "yes");

    // Color mode
    val = m_options["crtRenderer.colorMode"];
    ui->colorGroupBox->setVisible(m_platformGroup == "apogey" || m_platformGroup == "rk86" || m_platformGroup == "spec");
    if (m_platformGroup == "apogey") {
        ui->color1RadioButton->setText(tr("Color"));
        ui->color2RadioButton->setVisible(false);
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "color");
    } else if (m_platformGroup == "rk86") {
        ui->color1RadioButton->setText(tr("Color mode 1 (Tolkalin)"));
        ui->color2RadioButton->setVisible(true);
        ui->color2RadioButton->setText(tr("Color mode 2 (Akimenko)"));
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "color1");
        ui->color2RadioButton->setChecked(val == "color2");
    } else if (m_platformGroup == "spec") {
        ui->color1RadioButton->setText(tr("4-color mode"));
        ui->color2RadioButton->setVisible(true);
        ui->color2RadioButton->setText(tr("8-color mode"));
        ui->bwRadioButton->setChecked(val == "mono");
        ui->color1RadioButton->setChecked(val == "4color");
        ui->color2RadioButton->setChecked(val == "8color");
    }

    // Fields mixing
    val = m_options["window.fieldsMixing"];
    ui->mixingOffRadioButton->setChecked(val == "none");
    ui->mixingMixRadioButton->setChecked(val == "mix");
    ui->mixingInterlaceRadioButton->setChecked(val == "interlace");
    ui->mixingScanlineRadioButton->setChecked(val == "scanline");

    // Visible area
    val = m_options["crtRenderer.visibleArea"];
    ui->cropCheckBox->setEnabled(val != "");
    ui->cropCheckBox->setChecked(val == "yes");

    // Alternate font
    val = m_options["crtRenderer.altRenderer"];
    ui->altFontCheckBox->setEnabled(val != "");
    ui->altFontCheckBox->setChecked(val == "yes");

    // WAV channel
    val = m_options["wavReader.channel"];
    if (val == "left")
        ui->wavChannelComboBox->setCurrentIndex(0);
    else if (val == "right")
        ui->wavChannelComboBox->setCurrentIndex(1);
    else //if (val == "mix")
        ui->wavChannelComboBox->setCurrentIndex(2);

    // Speed up factor
    val = m_options["wavReader.speedUpFactor"];
    int suf = val.toInt();
    ui->speedUpSpinBox->setValue(suf);
    ui->speedUpCheckBox->setChecked(suf != 1);

    // Tape redirect
    val = m_options["tapeGrp.enabled"];
    ui->tapeRedirectCheckBox->setVisible(val != "");
    ui->tapeRedirectCheckBox->setChecked(val == "yes");

    // Suppress file opening on reset
    val = m_options["tapeInHook.suspendAfterResetForMs"];
    ui->tapeSuppressOpeningCheckBox->setVisible(val != "");
    int sar = val.toInt();
    ui->tapeSuppressOpeningCheckBox->setChecked(sar > 0);

    // Allow multiblock
    val = m_options["loader.allowMultiblock"];
    ui->tapeMultiblockCheckBox->setVisible(val != "");
    ui->tapeMultiblockCheckBox->setChecked(val == "yes");

    // Keyboard layout
    val = m_options["kbdLayout.layout"];
    ui->qwertyRadioButton->setChecked(val == "qwerty");
    ui->jcukenRadioButton->setChecked(val == "jcuken");
    ui->smartRadioButton->setChecked(val == "smart");

    // Keyboard matrix
    val = m_options["keyboard.matrix"];
    ui->kbdTypeGroupBox->setVisible(val != "");
    ui->kbdOriginalRadioButton->setChecked(val == "original");
    ui->kbdRamfosRadioButton->setChecked(val == "ramfos");
}


void SettingsDialog::on_volumeSlider_valueChanged(int value)
{
    ui->volumeSpinBox->setValue(value);
}


void SettingsDialog::on_volumeSpinBox_valueChanged(int arg1)
{
    ui->volumeSlider->setValue(arg1);
}


void SettingsDialog::on_presetComboBox_currentIndexChanged(int index)
{
    if (!m_presetComboBoxEventsAllowed)
        return;
    switch (index) {
    case 1:
    case 2:
    case 3:
        ui->autoSizeRadioButton->setChecked(true);
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleSpinBox->setValue(index);
        ui->smoothCheckBox->setChecked(false);
        ui->aspectCheckBox->setChecked(false);
        break;
    case 4:
        ui->fixedSizeRadioButton->setChecked(true);
        ui->fixedScaleRadioButton->setChecked(true);
        ui->fixedScaleSpinBox->setValue(2);
        ui->smoothCheckBox->setChecked(false);
        ui->aspectCheckBox->setChecked(false);
        break;
    case 5:
        ui->userSizeRadioButton->setChecked(true);
        ui->stretchPropRadioButton->setChecked(true);
        ui->smoothCheckBox->setChecked(true);
        ui->aspectCheckBox->setChecked(true);
        break;
    }
}


void SettingsDialog::on_fixedSizeRadioButton_toggled(bool checked)
{
    ui->widthLineEdit->setEnabled(checked);
    ui->heightLineEdit->setEnabled(checked);
    if (checked) {
        ui->stretchRadioButton->setEnabled(true);
        ui->stretchPropRadioButton->setEnabled(true);
        ui->stretchPropIntRadioButton->setEnabled(true);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::adjustPresetComboBoxState()
{
    m_presetComboBoxEventsAllowed = false;
    if (ui->autoSizeRadioButton->isChecked() &&
            ui->fixedScaleRadioButton->isChecked() &&
            ui->fixedScaleSpinBox->value() == 1 &&
            !ui->smoothCheckBox->isChecked() &&
            !ui->aspectCheckBox->isChecked())
        ui->presetComboBox->setCurrentIndex(1);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleSpinBox->value() == 2 &&
             !ui->smoothCheckBox->isChecked() &&
             !ui->aspectCheckBox->isChecked())
         ui->presetComboBox->setCurrentIndex(2);
    else if (ui->autoSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleSpinBox->value() == 3 &&
             !ui->smoothCheckBox->isChecked() &&
             !ui->aspectCheckBox->isChecked())
         ui->presetComboBox->setCurrentIndex(3);
    else if (ui->fixedSizeRadioButton->isChecked() &&
             ui->fixedScaleRadioButton->isChecked() &&
             ui->fixedScaleSpinBox->value() == 2 &&
             !ui->smoothCheckBox->isChecked() &&
             !ui->aspectCheckBox->isChecked())
         ui->presetComboBox->setCurrentIndex(4);
    else if (ui->userSizeRadioButton->isChecked() &&
             ui->stretchPropRadioButton->isChecked() &&
             ui->smoothCheckBox->isChecked() &&
             ui->aspectCheckBox->isChecked())
         ui->presetComboBox->setCurrentIndex(5);
    else
        ui->presetComboBox->setCurrentIndex(0);
    m_presetComboBoxEventsAllowed = true;
}


void SettingsDialog::on_autoSizeRadioButton_toggled(bool checked)
{
    if (checked) {
        ui->fixedScaleRadioButton->setChecked(true);
        ui->stretchRadioButton->setEnabled(false);
        ui->stretchPropRadioButton->setEnabled(false);
        ui->stretchPropIntRadioButton->setEnabled(false);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::on_userSizeRadioButton_toggled(bool checked)
{
    if (checked) {
        ui->stretchRadioButton->setEnabled(true);
        ui->stretchPropRadioButton->setEnabled(true);
        ui->stretchPropIntRadioButton->setEnabled(true);
        adjustPresetComboBoxState();
    }
}


void SettingsDialog::on_fixedScaleRadioButton_toggled(bool checked)
{
    ui->fixedScaleSpinBox->setEnabled(checked);
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_stretchRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_stretchPropRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_stretchPropIntRadioButton_toggled(bool checked)
{
    if (checked)
        adjustPresetComboBoxState();
}


void SettingsDialog::on_smoothCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    adjustPresetComboBoxState();
}


void SettingsDialog::on_aspectCheckBox_toggled(bool checked)
{
    Q_UNUSED(checked);
    adjustPresetComboBoxState();
}


void SettingsDialog::on_fixedScaleSpinBox_valueChanged(const QString &arg1)
{
    Q_UNUSED(arg1);
    adjustPresetComboBoxState();
}


void SettingsDialog::on_speedUpCheckBox_toggled(bool checked)
{
    ui->speedUpSpinBox->setEnabled(checked);
}


void SettingsDialog::on_applyPushButton_clicked()
{
    QString val;
    bool rebootFlag = false;

    switch (ui->langComboBox->currentIndex()) {
        case 0:
            val = "system";
            break;
        case 1:
            val = "en";
            break;
        case 2:
            val = "ru";
            break;
        default:
            val = "";
            break;
    }
    if (val != m_options["locale"]) {
        m_options["locale"] = val;
        rebootFlag = true;
    }

    /*switch (ui->driverComboBox->currentIndex()) {
        case 0:
            val = "desktop";
            break;
        case 1:
            val = "es";
            break;
        default:
            val = "";
            break;
    }
    if (val != m_options["glDriver"]) {
        m_options["glDriver"] = val;
        rebootFlag = true;
    }*/

    val = QString::number(ui->fpsSpinBox->value());
    if (val != m_options["maxFps"]) {
        m_options["maxFps"] = val;
        rebootFlag = true;
    }

    bool limitFps = !ui->maxFpsCheckBox->isChecked();
    if (limitFps != (m_options["limitFps"] == "true")) {
        m_options["limitFps"] = limitFps ? "true" : "false";
        rebootFlag = true;
    }

    val = ui->vsyncCheckBox->isChecked() ? "yes" : "no";
    if (val != m_options["vsync"]) {
        m_options["vsync"] = val;
        rebootFlag = true;
    }

    val = ui->srComboBox->currentText(); // !!!
    if (val != m_options["sampleRate"]) {
        m_options["sampleRate"] = val;
        rebootFlag = true;
    }

    val = QString::number(ui->volumeSlider->value());
    m_options["emulation.volume"] = val;

    val = "";
    if (ui->autoSizeRadioButton->isChecked())
        val = "autosize";
    else if (ui->userSizeRadioButton->isChecked())
        val = "sizable";
    else if (ui->fixedSizeRadioButton->isChecked())
        val = "fixed";
    m_options["window.windowStyle"] = val;

    m_options["window.defaultWindowWidth"] = ui->widthLineEdit->text();
    m_options["window.defaultWindowHeight"] = ui->heightLineEdit->text();

    val = "";
    if (ui->fixedScaleRadioButton->isChecked())
        val = QString::number(ui->fixedScaleSpinBox->value()) + "x";
    else if (ui->stretchRadioButton->isChecked())
        val = "fit";
    else if (ui->stretchPropRadioButton->isChecked())
        val = "fitKeepAR";
    else if (ui->stretchPropIntRadioButton->isChecked())
        val = "bestFit";
    m_options["window.frameScale"] = val;

    m_options["window.antialiasing"] = ui->smoothCheckBox->isChecked() ? "yes" : "no";


    m_options["window.aspectCorrection"] = ui->aspectCheckBox->isChecked() ? "yes" : "no";

    m_options["crtRenderer.visibleArea"] = ui->cropCheckBox->isChecked() ? "yes" : "no";

    m_options["cpu.debugOnHalt"] = ui->debugHltCheckBox->isChecked() ? "yes" : "no";

    m_options["cpu.debugOnOllegalCmd"] = ui->debugIllegalCheckBox->isChecked() ? "yes" : "no";

    val = "";
    if (ui->colorGroupBox->isVisible()) {
        if (ui->bwRadioButton->isChecked())
            val = "mono";
        else if (ui->color1RadioButton->isChecked()) {
            if (m_platformGroup == "apogey")
                val = "color";
            else if (m_platformGroup == "rk86")
                val = "color1";
            else if (m_platformGroup == "spec")
                val = "4color";
        } else if (ui->color2RadioButton->isChecked()) {
            if (m_platformGroup == "rk86")
                val = "color2";
            else if (m_platformGroup == "spec")
                val = "8color";
        }
    }
    if (val != "")
        m_options["crtRenderer.colorMode"] = val;

    val = "";
    if (ui->mixingGroupBox->isEnabled()) {
        if (ui->mixingOffRadioButton->isChecked())
            val = "none";
        else if (ui->mixingMixRadioButton->isChecked())
            val = "mix";
        else if (ui->mixingInterlaceRadioButton->isChecked())
            val = "interlace";
        else if (ui->mixingScanlineRadioButton->isChecked())
            val = "scanline";
    }
    if (val != "")
        m_options["window.fieldsMixing"] = val;

    if (ui->altFontCheckBox->isVisible())
        m_options["crtRenderer.altRenderer"] = ui->altFontCheckBox->isChecked() ? "yes" : "no";

    val = "";
    switch (ui->wavChannelComboBox->currentIndex()) {
    case 0:
        val = "left";
        break;
    case 1:
        val = "right";
        break;
    case 2:
        val = "mix";
        break;
    default:
        break;
    }
    if (val != "")
        m_options["wavReader.channel"] = val;

    if (!ui->speedUpCheckBox->isChecked())
        val = "1";
    else
        val = QString::number(ui->speedUpSpinBox->value());
    m_options["wavReader.speedUpFactor"] = val;

    if (ui->tapeRedirectCheckBox->isVisible())
        m_options["tapeGrp.enabled"] = ui->tapeRedirectCheckBox->isChecked() ? "yes" : "no";

    if (ui->tapeSuppressOpeningCheckBox->isVisible())
        m_options["tapeInHook.suspendAfterResetForMs"] = ui->tapeSuppressOpeningCheckBox->isChecked() ? "200" : "0"; // !!! добавить поле ms

    if (ui->tapeMultiblockCheckBox->isVisible())
        m_options["loader.allowMultiblock"] = ui->tapeMultiblockCheckBox->isChecked() ? "yes" : "no";

    val = "";
    if (ui->qwertyRadioButton->isChecked())
        val = "qwerty";
    else if (ui->jcukenRadioButton->isChecked())
        val = "jcuken";
    else if (ui->smartRadioButton->isChecked())
        val = "smart";
    m_options["kbdLayout.layout"] = val;

    saveRunningConfig();
    saveStoredConfig();

    m_mainWindow->updateActions();

    if (rebootFlag) {
        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Emu80: warning"));
        msgBox.setText(tr("Some settings will take effect after emulator restart"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.addButton(QMessageBox::Ok);
        msgBox.exec();
    }
}


// Применяет текущие настройки из m_options
void SettingsDialog::saveRunningConfig()
{
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (value != "" && option != "locale" /*&& option != "glDriver"*/ && option != "maxFps" &&
                           option != "limitFps" && option != "sampleRate" && option != "vsync") {
            setRunningConfigValue(option, value);
        }
    }
}


// Сохраняет настройки из m_options в ini-файле
void SettingsDialog::saveStoredConfig()
{
    QSettings settings;

    settings.beginGroup(m_platformGroup);
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (value != "" && option != "locale" /*&& option != "glDriver"*/ && option != "maxFps" && option != "limitFps" && option != "sampleRate" && option != "vsync")
            settings.setValue(option, value);
    }
    settings.endGroup();

    settings.beginGroup("system");
    foreach (QString option, m_options.keys()) {
        QString value = m_options.value(option);
        if (value != "" && (option == "locale" /*|| option == "glDriver"*/ || option == "maxFps" || option == "limitFps" || option == "sampleRate" || option == "vsync"))
            settings.setValue(option, value);
    }
    settings.endGroup();
}


void SettingsDialog::resetPlatformOptions()
{
    QSettings settings;
    settings.beginGroup(m_platformGroup);
    settings.remove("");
    settings.endGroup();
    settings.sync();

    m_options.clear();
    m_options = m_initialOptions;
    fillControlValues();
    saveStoredConfig();
    saveRunningConfig();
    m_mainWindow->updateActions();
}


void SettingsDialog::resetAllOptions()
{
    QSettings settings;
    settings.clear();
    settings.sync();

    m_options.clear();
    m_options = m_initialOptions;
    fillControlValues();
    saveStoredConfig();
    saveRunningConfig();
    m_mainWindow->updateActions();
}


void SettingsDialog::on_okPushButton_clicked()
{
    on_applyPushButton_clicked();
    accept();
}


void SettingsDialog::on_vsyncCheckBox_toggled(bool checked)
{
    ui->maxFpsCheckBox->setChecked(checked);
}


void SettingsDialog::on_maxFpsCheckBox_toggled(bool checked)
{
    ui->fpsSpinBox->setEnabled(!checked);
}