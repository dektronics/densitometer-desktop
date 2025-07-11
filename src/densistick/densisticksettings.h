#ifndef DENSISTICKSETTINGS_H
#define DENSISTICKSETTINGS_H

#include <QByteArray>

#include "peripheralcalvalues.h"

class M24C08;

class DensiStickSettings
{
public:
    explicit DensiStickSettings(M24C08 *eeprom);

    bool init();

    bool headerValid() const;

    quint8 probeType() const;
    void setProbeType(quint8 probeType);

    quint8 probeRevisionMajor() const;
    quint8 probeRevisionMinor() const;
    void setProbeRevision(quint8 major, quint8 minor);

    bool writeHeaderPage();

    DensiStickCalibration readCalibration();
    bool writeCalibration(const DensiStickCalibration &calibrationData);

private:
    M24C08 *eeprom_;
    QByteArray headerPage_;
    bool headerValid_;
};

#endif // DENSISTICKSETTINGS_H
