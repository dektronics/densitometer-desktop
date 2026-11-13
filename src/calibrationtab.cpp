#include "calibrationtab.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTableWidgetItem>

#include "util.h"

CalibrationTab::CalibrationTab(DensInterface *densInterface, QWidget *parent)
    : QWidget{parent}
    , densInterface_(densInterface)
{
}

CalibrationTab::~CalibrationTab()
{
}

void CalibrationTab::updateLineEditDirtyState(QLineEdit *lineEdit, int value)
{
    if (!lineEdit) { return; }

    const QLocale locale = util::numberLocale();

    if (lineEdit->text().isNull() || lineEdit->text().isEmpty()
        || lineEdit->text() == locale.toString(value)) {
        lineEdit->setStyleSheet(styleSheet());
    } else {
        if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            lineEdit->setStyleSheet("QLineEdit { background-color: darkslategrey; }");
        } else {
            lineEdit->setStyleSheet("QLineEdit { background-color: lightgoldenrodyellow; }");
        }
    }
}

void CalibrationTab::updateLineEditDirtyState(QLineEdit *lineEdit, float value, int prec)
{
    if (!lineEdit) { return; }

    const QLocale locale = util::numberLocale();

    if (lineEdit->text().isNull() || lineEdit->text().isEmpty()
        || (prec < 0 && locale.toFloat(lineEdit->text()) == locale.toFloat(locale.toString(value)))
        || (lineEdit->text() == locale.toString(value, 'f', prec))) {
        lineEdit->setStyleSheet(styleSheet());
    } else {
        if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            lineEdit->setStyleSheet("QLineEdit { background-color: darkslategrey; }");
        } else {
            lineEdit->setStyleSheet("QLineEdit { background-color: lightgoldenrodyellow; }");
        }
    }
}

void CalibrationTab::updateItemDirtyState(QTableWidgetItem *item, float value, int prec)
{
    if (!item) { return; }

    const QLocale locale = util::numberLocale();

    if (item->text().isNull() || item->text().isEmpty()
        || (prec < 0 && locale.toFloat(item->text()) == locale.toFloat(locale.toString(value)))
        || (item->text() == locale.toString(value, 'f', prec))) {
        item->setBackground(QBrush());
    } else {
        if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            item->setBackground(QBrush(QColor(47, 79, 79)));
        } else {
            item->setBackground(QBrush(QColor(250, 250, 210)));
        }
    }
}
