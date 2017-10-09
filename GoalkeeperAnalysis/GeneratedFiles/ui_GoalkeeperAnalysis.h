/********************************************************************************
** Form generated from reading UI file 'GoalkeeperAnalysis.ui'
**
** Created by: Qt User Interface Compiler version 5.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_GOALKEEPERANALYSIS_H
#define UI_GOALKEEPERANALYSIS_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_GoalkeeperAnalysisClass
{
public:
    QWidget *centralWidget;
    QPushButton *startCaptureButton;
    QPushButton *stopCaptureButton;
    QPushButton *showPlaybackButton;
    QPushButton *startCalibrationButton;
    QPushButton *getTestGaze;
    QLabel *label;
    QLabel *label_2;
    QFrame *line;
    QLabel *label_3;
    QFrame *line_2;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *GoalkeeperAnalysisClass)
    {
        if (GoalkeeperAnalysisClass->objectName().isEmpty())
            GoalkeeperAnalysisClass->setObjectName(QStringLiteral("GoalkeeperAnalysisClass"));
        GoalkeeperAnalysisClass->resize(375, 230);
        centralWidget = new QWidget(GoalkeeperAnalysisClass);
        centralWidget->setObjectName(QStringLiteral("centralWidget"));
        startCaptureButton = new QPushButton(centralWidget);
        startCaptureButton->setObjectName(QStringLiteral("startCaptureButton"));
        startCaptureButton->setGeometry(QRect(100, 70, 111, 21));
        stopCaptureButton = new QPushButton(centralWidget);
        stopCaptureButton->setObjectName(QStringLiteral("stopCaptureButton"));
        stopCaptureButton->setGeometry(QRect(220, 70, 111, 23));
        showPlaybackButton = new QPushButton(centralWidget);
        showPlaybackButton->setObjectName(QStringLiteral("showPlaybackButton"));
        showPlaybackButton->setGeometry(QRect(100, 100, 111, 21));
        startCalibrationButton = new QPushButton(centralWidget);
        startCalibrationButton->setObjectName(QStringLiteral("startCalibrationButton"));
        startCalibrationButton->setGeometry(QRect(100, 20, 111, 23));
        getTestGaze = new QPushButton(centralWidget);
        getTestGaze->setObjectName(QStringLiteral("getTestGaze"));
        getTestGaze->setGeometry(QRect(100, 150, 111, 23));
        label = new QLabel(centralWidget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(30, 150, 47, 13));
        label_2 = new QLabel(centralWidget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(30, 20, 71, 16));
        line = new QFrame(centralWidget);
        line->setObjectName(QStringLiteral("line"));
        line->setGeometry(QRect(30, 50, 311, 16));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        label_3 = new QLabel(centralWidget);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setGeometry(QRect(30, 70, 47, 13));
        line_2 = new QFrame(centralWidget);
        line_2->setObjectName(QStringLiteral("line_2"));
        line_2->setGeometry(QRect(30, 130, 311, 16));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);
        GoalkeeperAnalysisClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(GoalkeeperAnalysisClass);
        menuBar->setObjectName(QStringLiteral("menuBar"));
        menuBar->setGeometry(QRect(0, 0, 375, 21));
        GoalkeeperAnalysisClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(GoalkeeperAnalysisClass);
        mainToolBar->setObjectName(QStringLiteral("mainToolBar"));
        GoalkeeperAnalysisClass->addToolBar(Qt::TopToolBarArea, mainToolBar);
        statusBar = new QStatusBar(GoalkeeperAnalysisClass);
        statusBar->setObjectName(QStringLiteral("statusBar"));
        GoalkeeperAnalysisClass->setStatusBar(statusBar);

        retranslateUi(GoalkeeperAnalysisClass);

        QMetaObject::connectSlotsByName(GoalkeeperAnalysisClass);
    } // setupUi

    void retranslateUi(QMainWindow *GoalkeeperAnalysisClass)
    {
        GoalkeeperAnalysisClass->setWindowTitle(QApplication::translate("GoalkeeperAnalysisClass", "GoalkeeperAnalysis", Q_NULLPTR));
        startCaptureButton->setText(QApplication::translate("GoalkeeperAnalysisClass", "2. Start capture", Q_NULLPTR));
        stopCaptureButton->setText(QApplication::translate("GoalkeeperAnalysisClass", "Stop ", Q_NULLPTR));
        showPlaybackButton->setText(QApplication::translate("GoalkeeperAnalysisClass", "3. Show Playback", Q_NULLPTR));
        startCalibrationButton->setText(QApplication::translate("GoalkeeperAnalysisClass", "1. Calibrate", Q_NULLPTR));
        getTestGaze->setText(QApplication::translate("GoalkeeperAnalysisClass", "Test Gaze", Q_NULLPTR));
        label->setText(QApplication::translate("GoalkeeperAnalysisClass", "Debug", Q_NULLPTR));
        label_2->setText(QApplication::translate("GoalkeeperAnalysisClass", "Calibration", Q_NULLPTR));
        label_3->setText(QApplication::translate("GoalkeeperAnalysisClass", "Capturing", Q_NULLPTR));
    } // retranslateUi

};

namespace Ui {
    class GoalkeeperAnalysisClass: public Ui_GoalkeeperAnalysisClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_GOALKEEPERANALYSIS_H
