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
        "on_numlockStartupCheckbox_toggled",
        "checked",
        "on_configTable_cellDoubleClicked",
        "row",
        "column",
        "onProcessFinished",
        "exitCode",
        "QProcess::ExitStatus",
        "exitStatus",
        "onProcessError",
        "QProcess::ProcessError",
        "error",
        "refreshDrives",
        "refreshServices",
        "handleServiceAction",
        "action"
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
        // Slot 'on_reloadAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_addAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_editAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_removeAliasButton_clicked'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_userServicesToggle_stateChanged'
        QtMocHelpers::SlotData<void(int)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 11 },
        }}),
        // Slot 'on_serviceSearchBar_textChanged'
        QtMocHelpers::SlotData<void(const QString &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 13 },
        }}),
        // Slot 'on_startServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_stopServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_restartServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_enableServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_disableServiceButton_clicked'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'on_numlockStartupCheckbox_toggled'
        QtMocHelpers::SlotData<void(bool)>(19, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Bool, 20 },
        }}),
        // Slot 'on_configTable_cellDoubleClicked'
        QtMocHelpers::SlotData<void(int, int)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 22 }, { QMetaType::Int, 23 },
        }}),
        // Slot 'onProcessFinished'
        QtMocHelpers::SlotData<void(int, QProcess::ExitStatus)>(24, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 25 }, { 0x80000000 | 26, 27 },
        }}),
        // Slot 'onProcessError'
        QtMocHelpers::SlotData<void(QProcess::ProcessError)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 29, 30 },
        }}),
        // Slot 'refreshDrives'
        QtMocHelpers::SlotData<void()>(31, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'refreshServices'
        QtMocHelpers::SlotData<void()>(32, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'handleServiceAction'
        QtMocHelpers::SlotData<void(const QString &)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 34 },
        }}),
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
        case 4: _t->on_reloadAliasButton_clicked(); break;
        case 5: _t->on_addAliasButton_clicked(); break;
        case 6: _t->on_editAliasButton_clicked(); break;
        case 7: _t->on_removeAliasButton_clicked(); break;
        case 8: _t->on_userServicesToggle_stateChanged((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->on_serviceSearchBar_textChanged((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->on_startServiceButton_clicked(); break;
        case 11: _t->on_stopServiceButton_clicked(); break;
        case 12: _t->on_restartServiceButton_clicked(); break;
        case 13: _t->on_enableServiceButton_clicked(); break;
        case 14: _t->on_disableServiceButton_clicked(); break;
        case 15: _t->on_numlockStartupCheckbox_toggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 16: _t->on_configTable_cellDoubleClicked((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 17: _t->onProcessFinished((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QProcess::ExitStatus>>(_a[2]))); break;
        case 18: _t->onProcessError((*reinterpret_cast< std::add_pointer_t<QProcess::ProcessError>>(_a[1]))); break;
        case 19: _t->refreshDrives(); break;
        case 20: _t->refreshServices(); break;
        case 21: _t->handleServiceAction((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
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
        if (_id < 22)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 22)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 22;
    }
    return _id;
}
QT_WARNING_POP
