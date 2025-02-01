#ifndef DENSISTICKSETTINGS_H
#define DENSISTICKSETTINGS_H

#include <QByteArray>

#include "tsl2585calibration.h"

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

    Tsl2585Calibration readCalTsl2585();
    bool writeCalTsl2585(const Tsl2585Calibration &calData);

private:
    M24C08 *eeprom_;
    QByteArray headerPage_;
    bool headerValid_;
};

#endif // DENSISTICKSETTINGS_H
