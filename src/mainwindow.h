#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QAbstractItemModel>
#include <QSerialPortInfo>
#include "densinterface.h"
#include "ft260deviceinfo.h"

QT_BEGIN_NAMESPACE

class QLabel;
class QSerialPort;
class QLineEdit;
class QSpinBox;
class QStandardItemModel;

namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class DiagnosticsTab;
class CalibrationTab;
class LogWindow;
class RemoteControlDialog;
class StickInterface;
class StickRunner;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void connectToPort(const QString &portName);

private slots:
    void openConnection();
    void onOpenConnectionDialogFinished(int result);
    void closeConnection();
    void onImportSettings();
    void onExportSettings();
    void onLogger(bool checked);
    void onLoggerOpened();
    void onLoggerClosed();
    void about();

    void onMenuEditAboutToShow();

    void onConnectionOpened();
    void onConnectionClosed();
    void onConnectionError();

    void onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue);
    void onTargetDensity(float density);

    void onActionCut();
    void onActionCopy();
    void onActionPaste();
    void onActionDelete();

    void onAddReadingClicked();
    void onCopyTableClicked();
    void onClearTableClicked();

private:
    void openConnectionToSerialPort(const QSerialPortInfo &info);
    void openConnectionToFt260(const Ft260DeviceInfo &info);
    void refreshButtonState();
    void measTableAddReading(DensInterface::DensityType type, float density, float offset);
    void measTableCut();
    void measTableCopy();
    void measTableCopyList(const QModelIndexList &indexList, bool includeEmpty);
    void measTablePaste();
    void measTableDelete();

    Ui::MainWindow *ui = nullptr;
    QLabel *statusLabel_ = nullptr;
    QSerialPort *serialPort_ = nullptr;
    DensInterface *densInterface_ = nullptr;
    StickRunner *stickRunner_ = nullptr;
    DiagnosticsTab *diagnosticsTab_ = nullptr;
    CalibrationTab *calibrationTab_ = nullptr;
    LogWindow *logWindow_ = nullptr;
    QStandardItemModel *measModel_ = nullptr;
    DensInterface::DensityType lastReadingType_ = DensInterface::DensityUnknown;
    float lastReadingDensity_ = qSNaN();
    float lastReadingOffset_ = qSNaN();
    QPixmap reflTypePixmap;
    QPixmap tranTypePixmap;
    QPixmap zeroSetPixmap;
};

#endif // MAINWINDOW_H
