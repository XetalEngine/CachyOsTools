/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../mainwindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "MainWindow",
        "on_refreshButton_clicked",
        "",
        "on_mountButton_clicked",
        "on_forceMountButton_clicked",
        "on_unmountButton_clicked",
        "on_mount777Button_clicked",
        "on_takeOwnershipButton_clicked",
        "on_reloadAliasButton_clicked",
        "on_addAliasButton_clicked",
        "on_editAliasButton_clicked",
        "on_removeAliasButton_clicked",
        "on_userServicesToggle_stateChanged",
        "state",
        "on_serviceSearchBar_textChanged",
        "text",
        "on_startServiceButton_clicked",
        "on_stopServiceButton_clicked",
        "on_restartServiceButton_clicked",
        "on_enableServiceButton_clicked",
        "on_disableServiceButton_clicked",
        "on_configTable_cellDoubleClicked",
        "row",
        "column",
        "on_refreshLogsButton_clicked",
        "on_clearLogButton_clicked",
        "on_applyFilterButton_clicked",
        "on_copyLogButton_clicked",
        "on_saveLogButton_clicked",
        "on_logFileComboBox_currentTextChanged",
        "onLogProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "onProcessFinished",
        "onProcessError",
        "QProcess::ProcessError",
        "error",
        "refreshDrives",
        "refreshServices",
        "handleServiceAction",
        "action",
        "on_zramToggle_toggled",
        "checked",
        "on_zramConfigButton_clicked",
        "on_cpuGovernorCombo_currentIndexChanged",
        "index",
        "on_cpuGovernorConfigButton_clicked",
        "on_ipv6Toggle_toggled",
        "on_ipv6ConfigButton_clicked",
        "on_trimToggle_toggled",
        "on_trimConfigButton_clicked",
        "on_tmpfsToggle_toggled",
        "on_tmpfsConfigButton_clicked",
        "on_dnsConfigButton_clicked",
        "on_showHiddenFilesToggle_toggled",
        "on_showHiddenFilesConfigButton_clicked",
        "on_mitigationsToggle_toggled",
        "on_mitigationsConfigButton_clicked",
        "on_destDriveCombo_currentIndexChanged",
        "on_restoreSourceDriveCombo_currentIndexChanged",
        "on_restoreDestDriveCombo_currentIndexChanged",
        "on_startBackupButton_clicked",
        "on_startRestoreButton_clicked",
        "on_sourcePathTree_clicked",
        "QModelIndex",
        "on_destPathTree_clicked",
        "on_restoreSourcePathTree_clicked",
        "on_restoreDestPathTree_clicked",
        "handleBackupProgress",
        "handleBackupFinished",
        "on_createIsoButton_clicked",
        "on_estimateSizeButton_clicked",
        "on_loadPresetButton_clicked",
        "on_savePresetButton_clicked",
        "on_isoOutputBrowseButton_clicked"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'on_refreshButton_clicked'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_mountButton_clicked'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_forceMountButton_clicked'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_unmountButton_clicked'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_mount777Button_clicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_takeOwnershipButton_clicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_reloadAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_addAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_editAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_removeAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_userServicesToggle_stateChanged'
        QtMocHelpers::SlotData<void(int)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 13 },
        }}),
        // Slot 'on_serviceSearchBar_textChanged'
        QtMocHelpers::SlotData<void(const QString &)>(14, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'on_startServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_stopServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_restartServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_enableServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_disableServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_configTable_cellDoubleClicked'
        QtMocHelpers::SlotData<void(int, int)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 22 }, { QMetaType::Int, 23 },
        }}),
        // Slot 'on_refreshLogsButton_clicked'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_clearLogButton_clicked'
        QtMocHelpers::SlotData<void()>(25, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_applyFilterButton_clicked'
        QtMocHelpers::SlotData<void()>(26, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_copyLogButton_clicked'
        QtMocHelpers::SlotData<void()>(27, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_saveLogButton_clicked'
        QtMocHelpers::SlotData<void()>(28, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_logFileComboBox_currentTextChanged'
        QtMocHelpers::SlotData<void(const QString &)>(29, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 15 },
        }}),
        // Slot 'onLogProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(30, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 31 }, { 0x80000000 | 32, 33 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(34, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 31 }, { 0x80000000 | 32, 33 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(35, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 36, 37 },
        }}),
        // Slot 'refreshDrives'
        QtMocHelpers::SlotData<void()>(38, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'refreshServices'
        QtMocHelpers::SlotData<void()>(39, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleServiceAction'
        QtMocHelpers::SlotData<void(const QString &)>(40, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 41 },
        }}),
        // Slot 'on_zramToggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(42, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_zramConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(44, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_cpuGovernorCombo_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(45, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 46 },
        }}),
        // Slot 'on_cpuGovernorConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(47, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_ipv6Toggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(48, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_ipv6ConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(49, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_trimToggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(50, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_trimConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(51, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_tmpfsToggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(52, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_tmpfsConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(53, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_dnsConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(54, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_showHiddenFilesToggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(55, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_showHiddenFilesConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(56, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_mitigationsToggle_toggled'
        QtMocHelpers::SlotData<void(bool)>(57, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 43 },
        }}),
        // Slot 'on_mitigationsConfigButton_clicked'
        QtMocHelpers::SlotData<void()>(58, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_destDriveCombo_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(59, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 46 },
        }}),
        // Slot 'on_restoreSourceDriveCombo_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(60, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 46 },
        }}),
        // Slot 'on_restoreDestDriveCombo_currentIndexChanged'
        QtMocHelpers::SlotData<void(int)>(61, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 46 },
        }}),
        // Slot 'on_startBackupButton_clicked'
        QtMocHelpers::SlotData<void()>(62, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_startRestoreButton_clicked'
        QtMocHelpers::SlotData<void()>(63, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_sourcePathTree_clicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(64, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 46 },
        }}),
        // Slot 'on_destPathTree_clicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(66, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 46 },
        }}),
        // Slot 'on_restoreSourcePathTree_clicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(67, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 46 },
        }}),
        // Slot 'on_restoreDestPathTree_clicked'
        QtMocHelpers::SlotData<void(const QModelIndex &)>(68, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 65, 46 },
        }}),
        // Slot 'handleBackupProgress'
        QtMocHelpers::SlotData<void()>(69, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleBackupFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(70, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 31 }, { 0x80000000 | 32, 33 },
        }}),
        // Slot 'on_createIsoButton_clicked'
        QtMocHelpers::SlotData<void()>(71, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_estimateSizeButton_clicked'
        QtMocHelpers::SlotData<void()>(72, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_loadPresetButton_clicked'
        QtMocHelpers::SlotData<void()>(73, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_savePresetButton_clicked'
        QtMocHelpers::SlotData<void()>(74, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_isoOutputBrowseButton_clicked'
        QtMocHelpers::SlotData<void()>(75, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10MainWindowE_t>.metaTypes,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->on_refreshButton_clicked(); break;
        case 1: _t->on_mountButton_clicked(); break;
        case 2: _t->on_forceMountButton_clicked(); break;
        case 3: _t->on_unmountButton_clicked(); break;
        case 4: _t->on_mount777Button_clicked(); break;
        case 5: _t->on_takeOwnershipButton_clicked(); break;
        case 6: _t->on_reloadAliasButton_clicked(); break;
        case 7: _t->on_addAliasButton_clicked(); break;
        case 8: _t->on_editAliasButton_clicked(); break;
        case 9: _t->on_removeAliasButton_clicked(); break;
        case 10: _t->on_userServicesToggle_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->on_serviceSearchBar_textChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->on_startServiceButton_clicked(); break;
        case 13: _t->on_stopServiceButton_clicked(); break;
        case 14: _t->on_restartServiceButton_clicked(); break;
        case 15: _t->on_enableServiceButton_clicked(); break;
        case 16: _t->on_disableServiceButton_clicked(); break;
        case 17: _t->on_configTable_cellDoubleClicked((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 18: _t->on_refreshLogsButton_clicked(); break;
        case 19: _t->on_clearLogButton_clicked(); break;
        case 20: _t->on_applyFilterButton_clicked(); break;
        case 21: _t->on_copyLogButton_clicked(); break;
        case 22: _t->on_saveLogButton_clicked(); break;
        case 23: _t->on_logFileComboBox_currentTextChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->onLogProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 25: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 26: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 27: _t->refreshDrives(); break;
        case 28: _t->refreshServices(); break;
        case 29: _t->handleServiceAction((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 30: _t->on_zramToggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 31: _t->on_zramConfigButton_clicked(); break;
        case 32: _t->on_cpuGovernorCombo_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 33: _t->on_cpuGovernorConfigButton_clicked(); break;
        case 34: _t->on_ipv6Toggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 35: _t->on_ipv6ConfigButton_clicked(); break;
        case 36: _t->on_trimToggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 37: _t->on_trimConfigButton_clicked(); break;
        case 38: _t->on_tmpfsToggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 39: _t->on_tmpfsConfigButton_clicked(); break;
        case 40: _t->on_dnsConfigButton_clicked(); break;
        case 41: _t->on_showHiddenFilesToggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 42: _t->on_showHiddenFilesConfigButton_clicked(); break;
        case 43: _t->on_mitigationsToggle_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 44: _t->on_mitigationsConfigButton_clicked(); break;
        case 45: _t->on_destDriveCombo_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 46: _t->on_restoreSourceDriveCombo_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 47: _t->on_restoreDestDriveCombo_currentIndexChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 48: _t->on_startBackupButton_clicked(); break;
        case 49: _t->on_startRestoreButton_clicked(); break;
        case 50: _t->on_sourcePathTree_clicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 51: _t->on_destPathTree_clicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 52: _t->on_restoreSourcePathTree_clicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 53: _t->on_restoreDestPathTree_clicked((*reinterpret_cast< std::add_pointer_t<QModelIndex>>(_a[1]))); break;
        case 54: _t->handleBackupProgress(); break;
        case 55: _t->handleBackupFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 56: _t->on_createIsoButton_clicked(); break;
        case 57: _t->on_estimateSizeButton_clicked(); break;
        case 58: _t->on_loadPresetButton_clicked(); break;
        case 59: _t->on_savePresetButton_clicked(); break;
        case 60: _t->on_isoOutputBrowseButton_clicked(); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 61)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 61;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 61)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 61;
    }
    return _id;
}
QT_WARNING_POP
