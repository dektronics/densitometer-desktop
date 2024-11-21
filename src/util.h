#ifndef UTIL_H
#define UTIL_H

#include <QString>
#include <stddef.h>
#include <stdint.h>

class QObject;
class QValidator;
class QWidget;
class QPixmap;
class QSvgWidget;
class QTableWidget;

namespace util
{

void copy_from_u32(uint8_t *buf, uint32_t val);
uint32_t copy_to_u32(const uint8_t *buf);
void copy_from_f32(uint8_t *buf, float val);
float copy_to_f32(const uint8_t *buf);

QString encode_f32(float val);
float decode_f32(const QString &val);

uint32_t calculateStmCrc32(uint32_t *data, size_t len);
uint16_t calculateFtdiChecksum(const uint8_t *data, size_t len);

double **make2DArray(const size_t rows, const size_t cols);
void free2DArray(double **array, const size_t rows);

std::tuple<float, float, float> polyfit(const QList<float> &xList, const QList<float> &yList);

QValidator *createIntValidator(int min, int max, QObject *parent = nullptr);
QValidator *createFloatValidator(double min, double max, int decimals, QObject *parent = nullptr);

QPixmap createThemeColoredPixmap(const QWidget *refWidget, const QString &fileName);
QSvgWidget *createThemeColoredSvgWidget(const QWidget *refWidget, const QString &fileName);

void tableWidgetCut(QTableWidget *tableWidget);
void tableWidgetCopy(const QTableWidget *tableWidget);
void tableWidgetPaste(QTableWidget *tableWidget);
void tableWidgetDelete(QTableWidget *tableWidget);

}

#endif // UTIL_H
