#include "util.h"

#include <QIntValidator>
#include <QDoubleValidator>
#include <QWidget>
#include <QPixmap>
#include <string.h>

namespace util
{

void copy_from_u32(uint8_t *buf, uint32_t val)
{
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

uint32_t copy_to_u32(const uint8_t *buf)
{
    return (uint32_t)buf[0] << 24
        | (uint32_t)buf[1] << 16
        | (uint32_t)buf[2] << 8
        | (uint32_t)buf[3];
}

void copy_from_f32(uint8_t *buf, float val)
{
    uint32_t int_val;
    memcpy(&int_val, &val, sizeof(float));
    copy_from_u32(buf, int_val);
}

float copy_to_f32(const uint8_t *buf)
{
    float val;
    uint32_t int_val = copy_to_u32(buf);
    memcpy(&val, &int_val, sizeof(float));
    return val;
}

QString encode_f32(float val)
{
    QByteArray buf(4, Qt::Uninitialized);
    copy_from_f32(reinterpret_cast<uint8_t *>(buf.data()), val);
    QString result = buf.toHex().toUpper();
    return result;
}

float decode_f32(const QString &val)
{
    const QByteArray bytes = QByteArray::fromHex(val.toLatin1());
    return copy_to_f32((const uint8_t *)bytes.data());
}

uint32_t stmCrc32Fast(uint32_t crc, uint32_t data)
{
    // Calculate the CRC-32 checksum on a block of data, using the same algorithm
    // used by the hardware CRC module inside the STM32F4 microcontroller.
    //
    // Based on the C implementation posted here:
    // https://community.st.com/s/question/0D50X0000AIeYIb/stm32f4-crc32-algorithm-headache

    static const uint32_t crcTable[] = {
        0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9,
        0x130476DC, 0x17C56B6B, 0x1A864DB2, 0x1E475005,
        0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61,
        0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD
    };

    crc = crc ^ data;

    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];
    crc = (crc << 4) ^ crcTable[crc >> 28];

    return crc;
}

uint32_t calculateStmCrc32(uint32_t *data, size_t len)
{
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        crc = stmCrc32Fast(crc, data[i]);
    }

    return crc;
}

uint16_t calculateFtdiChecksum(const uint8_t *data, size_t len)
{
    uint16_t crc = 0xAAAA;

    if (len % 2 != 0) { return 0; }

    for (size_t i = 0; i < len; i += 2) {
        crc ^= data[i] | (data[i + 1] << 8);
        crc = (crc << 1) | (crc >> 15);
    }
    return crc;
}

double **make2DArray(const size_t rows, const size_t cols)
{
    double **array;

    array = new double*[rows];
    for (size_t i = 0; i < rows; i++) {
        array[i] = new double[cols];
    }

    for (size_t i = 0; i < rows; i++) {
        for (size_t j = 0; j < cols; j++) {
            array[i][j] = 0.;
        }
    }

    return array;
}

void free2DArray(double **array, const size_t rows)
{
    for (size_t i = 0; i < rows; i++) {
        delete[] array[i];
    }
    delete[] array;
}

QValidator *createIntValidator(int min, int max, QObject *parent)
{
    QIntValidator *validator = new QIntValidator(min, max, parent);
    return validator;
}

QValidator *createFloatValidator(double min, double max, int decimals, QObject *parent)
{
    QDoubleValidator *validator = new QDoubleValidator(min, max, decimals, parent);
    validator->setNotation(QDoubleValidator::StandardNotation);
    return validator;
}

QPixmap createThemeColoredPixmap(const QWidget *widget, const QString &fileName)
{
    if (!widget || fileName.isEmpty()) { return QPixmap(); }

    const QColor baseColor = widget->palette().color(QPalette::Text);
    const QPixmap basePixmap(fileName);

    QImage tmp = basePixmap.toImage();

    for(int y = 0; y < tmp.height(); y++) {
        for(int x= 0; x < tmp.width(); x++) {
            QColor color(baseColor.red(), baseColor.green(), baseColor.blue(), tmp.pixelColor(x,y).alpha());
            tmp.setPixelColor(x, y, color);
        }
    }
    QPixmap pixmap = QPixmap::fromImage(tmp);
    return pixmap;
}

}
