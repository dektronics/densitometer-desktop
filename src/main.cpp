#include <iostream>

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QCommandLineParser>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>

#include "mainwindow.h"
#include "headlesstask.h"

using namespace Qt::StringLiterals;

namespace
{
HeadlessTask::Command headlessCommand = HeadlessTask::CommandUnknown;
QString headlessArg;
QString connectPort;
}

bool handleCommandLine(const QCoreApplication &app)
{
    // Setup the command line parser
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();

    // Add specific command line options
    QCommandLineOption listOption(QStringList() << "l"_L1 << "list"_L1,
                                  QCoreApplication::translate("main", "List attached devices."));
    parser.addOption(listOption);

    QCommandLineOption portOption(QStringList() << "p"_L1 << "port"_L1,
                                  QCoreApplication::translate("main", "Connect to device at the selected port."),
                                  QCoreApplication::translate("main", "port"));
    parser.addOption(portOption);

    QCommandLineOption infoOption(QStringList() << "i"_L1 << "info"_L1,
                                  QCoreApplication::translate("main", "Query device system info."));
    parser.addOption(infoOption);

    QCommandLineOption exportOption(QStringList() << "export"_L1,
                                    QCoreApplication::translate("main", "Export device settings to file."),
                                    QCoreApplication::translate("main", "file"));
    parser.addOption(exportOption);

    // Parse the command line
    parser.process(app);

    if (parser.isSet(listOption)) {
        bool hasDevices = false;
        const auto infos = QSerialPortInfo::availablePorts();
        for (const QSerialPortInfo &info : infos) {
            if (DensInterface::portDeviceType(info) == DensInterface::DeviceUnknown) {
                continue;
            }
            std::cout << info.portName().toStdString()
                      << " " << info.description().toStdString()
                      << " [" << info.serialNumber().toStdString() << "]"
                      << std::endl;
            hasDevices = true;
        }
        if (!hasDevices) {
            std::cout << "No attached devices." << std::endl;
        }

        return true;
    }

    QString portValue = parser.value(portOption);
    if (!portValue.isEmpty()) {
        std::cout << "Connecting to " << portValue.toStdString() << std::endl;
        connectPort = portValue;
    }

    if (parser.isSet(infoOption) && headlessCommand == HeadlessTask::CommandUnknown) {
        headlessCommand = HeadlessTask::CommandSystemInfo;
    }

    if (parser.isSet(exportOption) && headlessCommand == HeadlessTask::CommandUnknown) {
        headlessCommand = HeadlessTask::CommandExportSettings;
        headlessArg = parser.value(exportOption);
    }

    return false;
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName(QCoreApplication::translate("main", "Printalyzer Densitometer Desktop"));
    QApplication::setApplicationVersion("1.1.1"_L1);
    QApplication::setOrganizationName(u"Dektronics, Inc."_s);
    QApplication::setOrganizationDomain(u"dektronics.com"_s);
    QApplication::setWindowIcon(QIcon(u":/icons/appicon.png"_s));

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "densitometer_"_L1 + QLocale(locale).name();
        if (translator.load(":/i18n/"_L1 + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    if (handleCommandLine(a)) {
        return 0;
    }

    if (headlessCommand != HeadlessTask::CommandUnknown) {
        HeadlessTask *task = new HeadlessTask(&a);
        task->setPort(connectPort);
        task->setCommand(headlessCommand, headlessArg);
        QTimer::singleShot(0, task, &HeadlessTask::run);
        QObject::connect(task, &HeadlessTask::finished, &a, &QCoreApplication::quit);
        return a.exec();
    } else {
        MainWindow w;
        w.show();

        if (!connectPort.isEmpty()) {
            w.connectToPort(connectPort);
        }
        return a.exec();
    }
}
