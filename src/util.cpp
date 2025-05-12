#include "util.h"

#include <QIntValidator>
#include <QDoubleValidator>
#include <QWidget>
#include <QPixmap>
#include <QSvgWidget>
#include <QFile>
#include <QTableWidget>
#include <QClipboard>
#include <QThread>
#include <QApplication>
#include <QMimeData>
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

void gaussEliminationLS(int m, int n, double **a /*[m][n]*/, double *x /*[n-1]*/)
{
    for (int i = 0; i < m-1; i++) {
        // Partial Pivoting
        for (int k = i+1; k < m; k++) {
            // If diagonal element(absolute vallue) is smaller than any of the terms below it
            if (std::abs(a[i][i]) < std::abs(a[k][i])) {
                // Swap the rows
                for (int j=0; j < n; j++) {
                    double temp;
                    temp = a[i][j];
                    a[i][j] = a[k][j];
                    a[k][j] = temp;
                }
            }
        }
        // Begin Gauss Elimination
        for (int k = i + 1; k < m; k++) {
            double term = a[k][i] / a[i][i];
            for (int j = 0; j < n; j++) {
                a[k][j] = a[k][j] - term * a[i][j];
            }
        }
    }
    // Begin Back-substitution
    for (int i = m-1; i >= 0; i--) {
        x[i] = a[i][n-1];
        for (int j = i+1; j < n-1; j++) {
            x[i] = x[i] - a[i][j] * x[j];
        }
        x[i] = x[i] / a[i][i];
    }
}

std::tuple<float, float, float> polyfit(const QList<float> &xList, const QList<float> &yList)
{
    // Polynomial Fitting, based on this implementation:
    // https://www.bragitoff.com/2018/06/polynomial-fitting-c-program/

    if (xList.isEmpty() || xList.size() != yList.size()) {
        return {qSNaN(), qSNaN(), qSNaN()};
    }

    // Number of data points
    const int N = xList.size();

    // Degree of polynomial
    const int n = 2;

    // An array of size 2*n+1 for storing N, Sig xi, Sig xi^2, ....
    // which are the independent components of the normal matrix
    double X[2*n+1];
    for (int i=0; i <= 2 * n; i++) {
        X[i] = 0;
        for (int j=0; j < N; j++) {
            X[i] = X[i] + std::pow((double)xList[j], i);
        }
    }

    // The normal augmented matrix
    //double B[n+1][n+2];
    double **B = make2DArray(n+1, n+2);
    // rhs
    double Y[n+1];
    for (int i = 0; i <= n; i++) {
        Y[i] = 0;
        for (int j=0; j < N; j++) {
            Y[i] = Y[i] + std::pow((double)xList[j], i) * (double)yList[j];
        }
    }
    for (int i = 0; i <= n; i++) {
        for (int j = 0; j <= n; j++) {
            B[i][j] = X[i + j];
        }
    }
    for (int i = 0; i <= n; i++) {
        B[i][n + 1] = Y[i];
    }

    double A[n+1];
    gaussEliminationLS(n+1, n+2, B, A);

    for(int i = 0; i <= n; i++) {
        qDebug().nospace() << "B[" << i << "] = " << A[i];
    }

    free2DArray(B, n+1);

    return {(float)A[0], (float)A[1], (float)A[2]};
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

QPixmap createThemeColoredPixmap(const QWidget *refWidget, const QString &fileName)
{
    if (!refWidget || fileName.isEmpty()) { return QPixmap(); }

    const QColor baseColor = refWidget->palette().color(QPalette::Text);
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

QSvgWidget *createThemeColoredSvgWidget(const QWidget *refWidget, const QString &fileName)
{
    // This function assumes the provided SVG is monochrome and
    // has all elements explicitly set to black

    QSvgWidget *widget = new QSvgWidget();

    if (!refWidget || fileName.isEmpty()) { return widget; }

    const QColor baseColor = refWidget->palette().color(QPalette::Text);

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return widget;
    }

    QByteArray svgData = file.readAll();
    if (svgData.isEmpty()) {
        return widget;
    }

    const QByteArray themeColor = baseColor.name(QColor::HexRgb).toUtf8();
    svgData.replace(QByteArray("#000000"), themeColor);

    widget->load(svgData);
    return widget;
}

void tableWidgetCut(QTableWidget *tableWidget)
{
    tableWidgetCopy(tableWidget);
    tableWidgetDelete(tableWidget);
}

void tableWidgetCopy(const QTableWidget *tableWidget)
{
    QString copiedText;
    for (int i = 0; i < tableWidget->rowCount(); i++) {
        QString copiedLine;
        for (int j = 0; j < tableWidget->columnCount(); j++) {
            QTableWidgetItem *item = tableWidget->item(i, j);
            if (!item || !item->isSelected()) { continue; }
            if (!copiedLine.isEmpty()) {
                copiedLine.append(QLatin1String("\t"));
            }
            copiedLine.append(item->text());
        }
        if (!copiedLine.isEmpty()) {
            if (!copiedText.isEmpty()) {
#if defined(Q_OS_WIN)
                copiedText.append(QLatin1String("\r\n"));
#else
                copiedText.append(QLatin1String("\n"));
#endif
            }
            copiedText.append(copiedLine);
        }
    }

    // Move to the clipboard
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(copiedText, QClipboard::Clipboard);

    if (clipboard->supportsSelection()) {
        clipboard->setText(copiedText, QClipboard::Selection);
    }

#if defined(Q_OS_UNIX)
    QThread::msleep(1);
#endif
}

void tableWidgetPaste(QTableWidget *tableWidget)
{
    static QRegularExpression reLine("\n|\r\n|\r");
    static QRegularExpression reRow("[,;]\\s*|\\s+");

    // Capture and split the text to be pasted
    // This keeps valid floats in string representation, so the target widget
    // can decide how it wants to format them.
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();
    QList<QList<QString>> numList;
    if (mimeData->hasText()) {
        const QString text = mimeData->text();
        const QStringList elements = text.split(reLine, Qt::SkipEmptyParts);
        for (const QString& element : elements) {
            QStringList rowElements = element.split(reRow, Qt::SkipEmptyParts);
            float num;
            bool ok;
            bool hasNums = false;

            QList<QString> rowNumList;
            for (const QString& rowElement : std::as_const(rowElements)) {
                rowElement.toFloat(&ok);

                if (ok) {
                    rowNumList.append(rowElement);
                    hasNums = true;
                } else {
                    rowNumList.append(QString());
                }
            }

            if (hasNums) {
                numList.append(rowNumList);
            }
        }
    }

    // Find the upper-left corner of the paste area
    int row = -1;
    int col = -1;
    QModelIndexList selected = tableWidget->selectionModel()->selectedIndexes();
    selected.append(tableWidget->selectionModel()->currentIndex());
    for (const QModelIndex &index : std::as_const(selected)) {
        if (row == -1 || index.row() < row) {
            row = index.row();
        }
        if (col == -1 || index.column() < col) {
            col = index.column();
        }
    }

    // Paste the values
    if (!numList.isEmpty() && row >= 0 && col >= 0) {
        for (auto rowNumList : numList) {
            if (row >= tableWidget->rowCount()) { break; }

            for (int i = 0; i < rowNumList.size(); i++) {
                if (col + i >= tableWidget->columnCount()) { break; }
                QTableWidgetItem *item = tableWidget->item(row, col + i);
                if (!item) {
                    item = new QTableWidgetItem();
                    tableWidget->setItem(row, col + i, item);
                }
                item->setText(rowNumList[i]);
            }

            row++;
        }
    }
}

void tableWidgetDelete(QTableWidget *tableWidget)
{
    for (int i = 0; i < tableWidget->rowCount(); i++) {
        for (int j = 0; j < tableWidget->columnCount(); j++) {
            QTableWidgetItem *item = tableWidget->item(i, j);
            if (item && item->isSelected()) {
                tableWidget->setItem(i, j, nullptr);
            }
        }
    }
    tableWidget->clearSelection();
}

QTableWidgetItem *tableWidgetItem(QTableWidget *table, int row, int column)
{
    QTableWidgetItem *item = table->item(row, column);
    if (!item) {
        item = new QTableWidgetItem();
        table->setItem(row, column, item);
    }
    return item;
}

bool tableWidgetHasEmptyCells(QTableWidget *tableWidget)
{
    bool hasEmpty = false;

    for (int i = 0; i < tableWidget->rowCount(); i++) {
        for (int j = 0; j < tableWidget->columnCount(); j++) {
            QTableWidgetItem *item = tableWidget->item(i, j);
            if (!item || item->text().isEmpty()) {
                hasEmpty = true;
                break;
            }
        }
    }
    return hasEmpty;
}

}
