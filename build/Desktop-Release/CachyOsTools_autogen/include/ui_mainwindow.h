/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.9.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionExit;
    QAction *actionAbout;
    QWidget *centralwidget;
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *driveToolsTab;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QTableWidget *drivesTable;
    QHBoxLayout *horizontalLayout;
    QPushButton *refreshButton;
    QPushButton *mountButton;
    QPushButton *forceMountButton;
    QPushButton *unmountButton;
    QSpacerItem *horizontalSpacer;
    QTextEdit *logOutput;
    QWidget *shellToolsTab;
    QVBoxLayout *verticalLayout_shell;
    QLabel *shellLabel;
    QTableWidget *aliasTable;
    QHBoxLayout *aliasButtonLayout;
    QPushButton *addAliasButton;
    QPushButton *editAliasButton;
    QPushButton *removeAliasButton;
    QPushButton *reloadAliasButton;
    QWidget *servicesTab;
    QVBoxLayout *verticalLayout_services;
    QHBoxLayout *servicesTopLayout;
    QLineEdit *serviceSearchBar;
    QCheckBox *userServicesToggle;
    QTableWidget *servicesTable;
    QHBoxLayout *servicesButtonLayout;
    QPushButton *startServiceButton;
    QPushButton *stopServiceButton;
    QPushButton *restartServiceButton;
    QPushButton *enableServiceButton;
    QPushButton *disableServiceButton;
    QWidget *tweaksTab;
    QVBoxLayout *verticalLayout_tweaks;
    QCheckBox *numlockStartupCheckbox;
    QWidget *configTab;
    QVBoxLayout *verticalLayout_config;
    QHBoxLayout *configTopLayout;
    QLabel *configLabel;
    QComboBox *editorComboBox;
    QSpacerItem *configSpacer;
    QTableWidget *configTable;
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuHelp;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1000, 700);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        verticalLayout = new QVBoxLayout(centralwidget);
        verticalLayout->setObjectName("verticalLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        driveToolsTab = new QWidget();
        driveToolsTab->setObjectName("driveToolsTab");
        verticalLayout_2 = new QVBoxLayout(driveToolsTab);
        verticalLayout_2->setObjectName("verticalLayout_2");
        label = new QLabel(driveToolsTab);
        label->setObjectName("label");
        QFont font;
        font.setPointSize(12);
        font.setBold(true);
        label->setFont(font);

        verticalLayout_2->addWidget(label);

        drivesTable = new QTableWidget(driveToolsTab);
        drivesTable->setObjectName("drivesTable");
        drivesTable->setAlternatingRowColors(true);
        drivesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        drivesTable->setSortingEnabled(true);

        verticalLayout_2->addWidget(drivesTable);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        refreshButton = new QPushButton(driveToolsTab);
        refreshButton->setObjectName("refreshButton");
        refreshButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(refreshButton);

        mountButton = new QPushButton(driveToolsTab);
        mountButton->setObjectName("mountButton");
        mountButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(mountButton);

        forceMountButton = new QPushButton(driveToolsTab);
        forceMountButton->setObjectName("forceMountButton");
        forceMountButton->setMinimumSize(QSize(120, 35));

        horizontalLayout->addWidget(forceMountButton);

        unmountButton = new QPushButton(driveToolsTab);
        unmountButton->setObjectName("unmountButton");
        unmountButton->setMinimumSize(QSize(100, 35));

        horizontalLayout->addWidget(unmountButton);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);


        verticalLayout_2->addLayout(horizontalLayout);

        logOutput = new QTextEdit(driveToolsTab);
        logOutput->setObjectName("logOutput");
        logOutput->setMaximumSize(QSize(16777215, 150));
        logOutput->setReadOnly(true);

        verticalLayout_2->addWidget(logOutput);

        tabWidget->addTab(driveToolsTab, QString());
        shellToolsTab = new QWidget();
        shellToolsTab->setObjectName("shellToolsTab");
        verticalLayout_shell = new QVBoxLayout(shellToolsTab);
        verticalLayout_shell->setObjectName("verticalLayout_shell");
        shellLabel = new QLabel(shellToolsTab);
        shellLabel->setObjectName("shellLabel");
        QFont font1;
        font1.setPointSize(11);
        font1.setBold(true);
        shellLabel->setFont(font1);

        verticalLayout_shell->addWidget(shellLabel);

        aliasTable = new QTableWidget(shellToolsTab);
        aliasTable->setObjectName("aliasTable");
        aliasTable->setAlternatingRowColors(true);
        aliasTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        aliasTable->setSortingEnabled(true);

        verticalLayout_shell->addWidget(aliasTable);

        aliasButtonLayout = new QHBoxLayout();
        aliasButtonLayout->setObjectName("aliasButtonLayout");
        addAliasButton = new QPushButton(shellToolsTab);
        addAliasButton->setObjectName("addAliasButton");

        aliasButtonLayout->addWidget(addAliasButton);

        editAliasButton = new QPushButton(shellToolsTab);
        editAliasButton->setObjectName("editAliasButton");

        aliasButtonLayout->addWidget(editAliasButton);

        removeAliasButton = new QPushButton(shellToolsTab);
        removeAliasButton->setObjectName("removeAliasButton");

        aliasButtonLayout->addWidget(removeAliasButton);

        reloadAliasButton = new QPushButton(shellToolsTab);
        reloadAliasButton->setObjectName("reloadAliasButton");

        aliasButtonLayout->addWidget(reloadAliasButton);


        verticalLayout_shell->addLayout(aliasButtonLayout);

        tabWidget->addTab(shellToolsTab, QString());
        servicesTab = new QWidget();
        servicesTab->setObjectName("servicesTab");
        verticalLayout_services = new QVBoxLayout(servicesTab);
        verticalLayout_services->setObjectName("verticalLayout_services");
        servicesTopLayout = new QHBoxLayout();
        servicesTopLayout->setObjectName("servicesTopLayout");
        serviceSearchBar = new QLineEdit(servicesTab);
        serviceSearchBar->setObjectName("serviceSearchBar");

        servicesTopLayout->addWidget(serviceSearchBar);

        userServicesToggle = new QCheckBox(servicesTab);
        userServicesToggle->setObjectName("userServicesToggle");

        servicesTopLayout->addWidget(userServicesToggle);


        verticalLayout_services->addLayout(servicesTopLayout);

        servicesTable = new QTableWidget(servicesTab);
        servicesTable->setObjectName("servicesTable");
        servicesTable->setAlternatingRowColors(true);
        servicesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        servicesTable->setSortingEnabled(true);

        verticalLayout_services->addWidget(servicesTable);

        servicesButtonLayout = new QHBoxLayout();
        servicesButtonLayout->setObjectName("servicesButtonLayout");
        startServiceButton = new QPushButton(servicesTab);
        startServiceButton->setObjectName("startServiceButton");

        servicesButtonLayout->addWidget(startServiceButton);

        stopServiceButton = new QPushButton(servicesTab);
        stopServiceButton->setObjectName("stopServiceButton");

        servicesButtonLayout->addWidget(stopServiceButton);

        restartServiceButton = new QPushButton(servicesTab);
        restartServiceButton->setObjectName("restartServiceButton");

        servicesButtonLayout->addWidget(restartServiceButton);

        enableServiceButton = new QPushButton(servicesTab);
        enableServiceButton->setObjectName("enableServiceButton");

        servicesButtonLayout->addWidget(enableServiceButton);

        disableServiceButton = new QPushButton(servicesTab);
        disableServiceButton->setObjectName("disableServiceButton");

        servicesButtonLayout->addWidget(disableServiceButton);


        verticalLayout_services->addLayout(servicesButtonLayout);

        tabWidget->addTab(servicesTab, QString());
        tweaksTab = new QWidget();
        tweaksTab->setObjectName("tweaksTab");
        verticalLayout_tweaks = new QVBoxLayout(tweaksTab);
        verticalLayout_tweaks->setObjectName("verticalLayout_tweaks");
        numlockStartupCheckbox = new QCheckBox(tweaksTab);
        numlockStartupCheckbox->setObjectName("numlockStartupCheckbox");

        verticalLayout_tweaks->addWidget(numlockStartupCheckbox);

        tabWidget->addTab(tweaksTab, QString());
        configTab = new QWidget();
        configTab->setObjectName("configTab");
        verticalLayout_config = new QVBoxLayout(configTab);
        verticalLayout_config->setObjectName("verticalLayout_config");
        configTopLayout = new QHBoxLayout();
        configTopLayout->setObjectName("configTopLayout");
        configLabel = new QLabel(configTab);
        configLabel->setObjectName("configLabel");

        configTopLayout->addWidget(configLabel);

        editorComboBox = new QComboBox(configTab);
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->addItem(QString());
        editorComboBox->setObjectName("editorComboBox");

        configTopLayout->addWidget(editorComboBox);

        configSpacer = new QSpacerItem(40, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        configTopLayout->addItem(configSpacer);


        verticalLayout_config->addLayout(configTopLayout);

        configTable = new QTableWidget(configTab);
        configTable->setObjectName("configTable");
        configTable->setAlternatingRowColors(true);
        configTable->setSelectionBehavior(QAbstractItemView::SelectRows);
        configTable->setSortingEnabled(true);

        verticalLayout_config->addWidget(configTable);

        tabWidget->addTab(configTab, QString());

        verticalLayout->addWidget(tabWidget);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1000, 23));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName("menuFile");
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuFile->addAction(actionExit);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "CachyOS Tools", nullptr));
        actionExit->setText(QCoreApplication::translate("MainWindow", "Exit", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "About", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Available Drives", nullptr));
        refreshButton->setText(QCoreApplication::translate("MainWindow", "\360\237\224\204 Refresh", nullptr));
        mountButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\201 Mount", nullptr));
        forceMountButton->setText(QCoreApplication::translate("MainWindow", "\342\232\241 Force Mount", nullptr));
        unmountButton->setText(QCoreApplication::translate("MainWindow", "\360\237\223\244 Unmount", nullptr));
        logOutput->setPlaceholderText(QCoreApplication::translate("MainWindow", "Command output will appear here...", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(driveToolsTab), QCoreApplication::translate("MainWindow", "Drive Tools", nullptr));
        shellLabel->setText(QCoreApplication::translate("MainWindow", "Detected Shell: ", nullptr));
        addAliasButton->setText(QCoreApplication::translate("MainWindow", "Add Alias", nullptr));
        editAliasButton->setText(QCoreApplication::translate("MainWindow", "Edit Alias", nullptr));
        removeAliasButton->setText(QCoreApplication::translate("MainWindow", "Remove Alias", nullptr));
        reloadAliasButton->setText(QCoreApplication::translate("MainWindow", "Reload", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(shellToolsTab), QCoreApplication::translate("MainWindow", "Shell Tools", nullptr));
        serviceSearchBar->setPlaceholderText(QCoreApplication::translate("MainWindow", "Search services...", nullptr));
        userServicesToggle->setText(QCoreApplication::translate("MainWindow", "Show User Services", nullptr));
        startServiceButton->setText(QCoreApplication::translate("MainWindow", "Start", nullptr));
        stopServiceButton->setText(QCoreApplication::translate("MainWindow", "Stop", nullptr));
        restartServiceButton->setText(QCoreApplication::translate("MainWindow", "Restart", nullptr));
        enableServiceButton->setText(QCoreApplication::translate("MainWindow", "Enable", nullptr));
        disableServiceButton->setText(QCoreApplication::translate("MainWindow", "Disable", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(servicesTab), QCoreApplication::translate("MainWindow", "Services/Startup", nullptr));
        numlockStartupCheckbox->setText(QCoreApplication::translate("MainWindow", "Enable NumLock at startup (KDE only)", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tweaksTab), QCoreApplication::translate("MainWindow", "Tweaks", nullptr));
        configLabel->setText(QCoreApplication::translate("MainWindow", "Text Editor:", nullptr));
        editorComboBox->setItemText(0, QCoreApplication::translate("MainWindow", "nano", nullptr));
        editorComboBox->setItemText(1, QCoreApplication::translate("MainWindow", "kate", nullptr));
        editorComboBox->setItemText(2, QCoreApplication::translate("MainWindow", "mousepad", nullptr));
        editorComboBox->setItemText(3, QCoreApplication::translate("MainWindow", "geany", nullptr));

        editorComboBox->setCurrentText(QCoreApplication::translate("MainWindow", "nano", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(configTab), QCoreApplication::translate("MainWindow", "System Config", nullptr));
        menuFile->setTitle(QCoreApplication::translate("MainWindow", "File", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "Help", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
