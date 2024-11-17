#include "calibrationtab.h"

#include <QtGui/QGuiApplication>
#include <QtGui/QStyleHints>
#include <QtWidgets/QLineEdit>

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

    if (lineEdit->text().isNull() || lineEdit->text().isEmpty()
        || lineEdit->text() == QString::number(value)) {
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

    if (lineEdit->text().isNull() || lineEdit->text().isEmpty()
        || (prec < 0 && lineEdit->text().toFloat() == QString::number(value).toFloat())
        || (lineEdit->text() == QString::number(value, 'f', prec))) {
        lineEdit->setStyleSheet(styleSheet());
    } else {
        if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
            lineEdit->setStyleSheet("QLineEdit { background-color: darkslategrey; }");
        } else {
            lineEdit->setStyleSheet("QLineEdit { background-color: lightgoldenrodyellow; }");
        }
    }
}
