#ifndef SETTINGSUVVISIMPORTDIALOG_H
#define SETTINGSUVVISIMPORTDIALOG_H

#include <QDialog>

#include "denscalvalues.h"

namespace Ui {
class SettingsUvVisImportDialog;
}

class QTableWidget;
class DensInterface;

class SettingsUvVisImportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsUvVisImportDialog(QWidget *parent = nullptr);
    ~SettingsUvVisImportDialog();

    bool loadFile(const QString &filename);
    void sendSelectedSettings(DensInterface *densInterface);

private slots:
    void onCheckBoxChanged(Qt::CheckState state);

private:
    bool parseHeader(const QJsonObject &root);
    void parseCalSensor(const QJsonObject &jsonCalSensor);
    void parseCalTarget(const QJsonObject &jsonCalTarget);
    void parseCalTemperature(const QJsonObject &jsonCalTemp);
    void calTemperatureAssignColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues);

    Ui::SettingsUvVisImportDialog *ui;

    DensUvVisCalGain calGain_;
    DensCalTemperature calVisTemp_;
    DensCalTemperature calUvTemp_;

    DensCalTarget calReflection_;
    DensCalTarget calVisTransmission_;
    DensCalTarget calUvTransmission_;
};

#endif // SETTINGSUVVISIMPORTDIALOG_H
