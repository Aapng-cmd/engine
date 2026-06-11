/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../headers/MainWindow.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
        "onOpen",
        "",
        "onSave",
        "onSaveAs",
        "onBuildViewer",
        "onAddTexture",
        "onRemoveTexture",
        "addFigure",
        "type",
        "addFigureFromPreset",
        "CustomFigurePreset",
        "preset",
        "onRemoveObject",
        "onObjectSelectionChanged",
        "onPreviewObjectPicked",
        "index",
        "applyTransformFromUi",
        "onTextureIndexChanged",
        "onRescanTextures",
        "onMergeSelected",
        "onObjectItemActivated",
        "QListWidgetItem*",
        "item",
        "onSaveCustomFigure"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'onOpen'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSave'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onSaveAs'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onBuildViewer'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onAddTexture'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveTexture'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'addFigure'
        QtMocHelpers::SlotData<void(const QString &)>(8, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 9 },
        }}),
        // Slot 'addFigureFromPreset'
        QtMocHelpers::SlotData<void(const CustomFigurePreset &)>(10, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 11, 12 },
        }}),
        // Slot 'onRemoveObject'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onObjectSelectionChanged'
        QtMocHelpers::SlotData<void()>(14, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onPreviewObjectPicked'
        QtMocHelpers::SlotData<void(int)>(15, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Slot 'applyTransformFromUi'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onTextureIndexChanged'
        QtMocHelpers::SlotData<void(int)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 16 },
        }}),
        // Slot 'onRescanTextures'
        QtMocHelpers::SlotData<void()>(19, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMergeSelected'
        QtMocHelpers::SlotData<void()>(20, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onObjectItemActivated'
        QtMocHelpers::SlotData<void(QListWidgetItem *)>(21, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 22, 23 },
        }}),
        // Slot 'onSaveCustomFigure'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
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
        case 0: _t->onOpen(); break;
        case 1: _t->onSave(); break;
        case 2: _t->onSaveAs(); break;
        case 3: _t->onBuildViewer(); break;
        case 4: _t->onAddTexture(); break;
        case 5: _t->onRemoveTexture(); break;
        case 6: _t->addFigure((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->addFigureFromPreset((*reinterpret_cast<std::add_pointer_t<CustomFigurePreset>>(_a[1]))); break;
        case 8: _t->onRemoveObject(); break;
        case 9: _t->onObjectSelectionChanged(); break;
        case 10: _t->onPreviewObjectPicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 11: _t->applyTransformFromUi(); break;
        case 12: _t->onTextureIndexChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 13: _t->onRescanTextures(); break;
        case 14: _t->onMergeSelected(); break;
        case 15: _t->onObjectItemActivated((*reinterpret_cast<std::add_pointer_t<QListWidgetItem*>>(_a[1]))); break;
        case 16: _t->onSaveCustomFigure(); break;
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
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
