#include "m24c08.h"

#include <QThread>
#include <QDebug>

#include "ft260.h"

M24C08::M24C08(Ft260 *ft260, quint8 deviceAddress)
{
    if (!ft260) {
        return;
    }

    if (deviceAddress < 0x50 || deviceAddress > 0x56) {
        qWarning().nospace() << "Invalid device address: 0x" << Qt::hex << deviceAddress;
        return;
    }

    ft260_ = ft260;
    deviceAddress_ = deviceAddress;
    valid_ = true;
}

bool M24C08::valid() const
{
    return valid_;
}

size_t M24C08::size() const
{
    return M24C08_MEM_SIZE;
}

QByteArray M24C08::readBuffer(uint16_t address, size_t len)
{
    if (!valid_) { return QByteArray(); }

    if (address + len > M24C08_MEM_SIZE) {
        return QByteArray();
    }

    QByteArray buf;
    buf.reserve(static_cast<int>(len));

    uint16_t offset = 0;

    while (offset < len) {
        const uint16_t readAddress = address + offset;
        const quint8 readLen = static_cast<quint8>(qMin(len - offset, static_cast<size_t>(32U)));

        const uint8_t i2cAddress = static_cast<uint8_t>((uint16_t)deviceAddress_ | ((readAddress & 0x0300) >> 8));
        const uint8_t memAddress = static_cast<uint8_t>(readAddress & 0x00FF);

        const QByteArray seg = ft260_->i2cRead(i2cAddress, memAddress, readLen);

        if (seg.isEmpty()) {
            return QByteArray();
        }

        buf.append(seg);

        offset += readLen;
    }

    return buf;
}

bool M24C08::writeBuffer(uint16_t address, const QByteArray &data)
{
    if (!valid_) { return false; }

    if (address + data.size() > M24C08_MEM_SIZE) {
        qWarning() << "Data buffer too big:" << data.size();
        return false;
    }

    size_t offset = 0;

    while (offset < static_cast<size_t>(data.size())) {
        const uint16_t writeAddress = address + static_cast<uint16_t>(offset);
        const quint8 writeLen = static_cast<quint8>(qMin(data.size() - offset, static_cast<size_t>(M24C08_PAGE_SIZE)));

        const uint8_t i2cAddress = static_cast<uint8_t>((uint16_t)deviceAddress_ | ((writeAddress & 0x0300) >> 8));
        const uint8_t memAddress = static_cast<uint8_t>(writeAddress & 0x00FF);

        if (!ft260_->i2cWrite(i2cAddress, memAddress, data.mid(offset, writeLen))) {
            qWarning() << "I2C EEPROM write error";
            return false;
        }

        // Short delay may be necessary to prevent subsequent write from failing
        QThread::msleep(100);

        offset += writeLen;
    }

    return true;
}
