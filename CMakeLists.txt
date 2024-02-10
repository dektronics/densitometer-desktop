cmake_minimum_required(VERSION 3.5)

project(densitometer VERSION 0.3 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets SerialPort LinguistTools)

set(TS_FILES assets/translations/densitometer_en_US.ts)
set(QRC_FILES assets/densitometer.qrc)

set(PROJECT_SOURCES
    src/calibrationbaselinetab.cpp src/calibrationbaselinetab.h src/calibrationbaselinetab.ui
    src/calibrationuvvistab.cpp src/calibrationuvvistab.h src/calibrationuvvistab.ui
    src/calibrationtab.cpp src/calibrationtab.h
    src/connectdialog.cpp src/connectdialog.h src/connectdialog.ui
    src/denscalvalues.cpp src/denscalvalues.h
    src/denscommand.cpp src/denscommand.h
    src/densinterface.cpp src/densinterface.h
    src/diagnosticstab.cpp src/diagnosticstab.h src/diagnosticstab.ui
    src/floatitemdelegate.cpp src/floatitemdelegate.h
    src/gaincalibrationdialog.cpp src/gaincalibrationdialog.h src/gaincalibrationdialog.ui
    src/headlesstask.cpp src/headlesstask.h
    src/logger.cpp src/logger.h
    src/logwindow.cpp src/logwindow.h src/logwindow.ui
    src/main.cpp
    src/mainwindow.cpp src/mainwindow.h src/mainwindow.ui
    src/remotecontroldialog.cpp src/remotecontroldialog.h src/remotecontroldialog.ui
    src/settingsexporter.cpp src/settingsexporter.h
    src/settingsimportdialog.cpp src/settingsimportdialog.h src/settingsimportdialog.ui
    src/slopecalibrationdialog.cpp src/slopecalibrationdialog.h src/slopecalibrationdialog.ui
    src/util.cpp src/util.h
    src/qsimplesignalaggregator.cpp src/qsimplesignalaggregator.h src/qsignalaggregator.h
    ${TS_FILES}
)

set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/deploy/windows/resources/info.rc")

qt_add_executable(densitometer
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${QRC_FILES}
    ${app_icon_resource_windows}
)

qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_link_libraries(densitometer PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::SerialPort
)

set_target_properties(densitometer PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/deploy/macOS/Info.plist"
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS densitometer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(densitometer)
