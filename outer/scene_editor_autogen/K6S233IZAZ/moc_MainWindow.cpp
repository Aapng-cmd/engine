/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.17)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../headers/MainWindow.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.17. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[25];
    char stringdata0[346];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 6), // "onOpen"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 6), // "onSave"
QT_MOC_LITERAL(4, 26, 8), // "onSaveAs"
QT_MOC_LITERAL(5, 35, 13), // "onBuildViewer"
QT_MOC_LITERAL(6, 49, 12), // "onAddTexture"
QT_MOC_LITERAL(7, 62, 15), // "onRemoveTexture"
QT_MOC_LITERAL(8, 78, 9), // "addFigure"
QT_MOC_LITERAL(9, 88, 4), // "type"
QT_MOC_LITERAL(10, 93, 19), // "addFigureFromPreset"
QT_MOC_LITERAL(11, 113, 18), // "CustomFigurePreset"
QT_MOC_LITERAL(12, 132, 6), // "preset"
QT_MOC_LITERAL(13, 139, 14), // "onRemoveObject"
QT_MOC_LITERAL(14, 154, 24), // "onObjectSelectionChanged"
QT_MOC_LITERAL(15, 179, 21), // "onPreviewObjectPicked"
QT_MOC_LITERAL(16, 201, 5), // "index"
QT_MOC_LITERAL(17, 207, 20), // "applyTransformFromUi"
QT_MOC_LITERAL(18, 228, 21), // "onTextureIndexChanged"
QT_MOC_LITERAL(19, 250, 16), // "onRescanTextures"
QT_MOC_LITERAL(20, 267, 15), // "onMergeSelected"
QT_MOC_LITERAL(21, 283, 21), // "onObjectItemActivated"
QT_MOC_LITERAL(22, 305, 16), // "QListWidgetItem*"
QT_MOC_LITERAL(23, 322, 4), // "item"
QT_MOC_LITERAL(24, 327, 18) // "onSaveCustomFigure"

    },
    "MainWindow\0onOpen\0\0onSave\0onSaveAs\0"
    "onBuildViewer\0onAddTexture\0onRemoveTexture\0"
    "addFigure\0type\0addFigureFromPreset\0"
    "CustomFigurePreset\0preset\0onRemoveObject\0"
    "onObjectSelectionChanged\0onPreviewObjectPicked\0"
    "index\0applyTransformFromUi\0"
    "onTextureIndexChanged\0onRescanTextures\0"
    "onMergeSelected\0onObjectItemActivated\0"
    "QListWidgetItem*\0item\0onSaveCustomFigure"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   99,    2, 0x08 /* Private */,
       3,    0,  100,    2, 0x08 /* Private */,
       4,    0,  101,    2, 0x08 /* Private */,
       5,    0,  102,    2, 0x08 /* Private */,
       6,    0,  103,    2, 0x08 /* Private */,
       7,    0,  104,    2, 0x08 /* Private */,
       8,    1,  105,    2, 0x08 /* Private */,
      10,    1,  108,    2, 0x08 /* Private */,
      13,    0,  111,    2, 0x08 /* Private */,
      14,    0,  112,    2, 0x08 /* Private */,
      15,    1,  113,    2, 0x08 /* Private */,
      17,    0,  116,    2, 0x08 /* Private */,
      18,    1,  117,    2, 0x08 /* Private */,
      19,    0,  120,    2, 0x08 /* Private */,
      20,    0,  121,    2, 0x08 /* Private */,
      21,    1,  122,    2, 0x08 /* Private */,
      24,    0,  125,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 22,   23,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->onOpen(); break;
        case 1: _t->onSave(); break;
        case 2: _t->onSaveAs(); break;
        case 3: _t->onBuildViewer(); break;
        case 4: _t->onAddTexture(); break;
        case 5: _t->onRemoveTexture(); break;
        case 6: _t->addFigure((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->addFigureFromPreset((*reinterpret_cast< const CustomFigurePreset(*)>(_a[1]))); break;
        case 8: _t->onRemoveObject(); break;
        case 9: _t->onObjectSelectionChanged(); break;
        case 10: _t->onPreviewObjectPicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->applyTransformFromUi(); break;
        case 12: _t->onTextureIndexChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->onRescanTextures(); break;
        case 14: _t->onMergeSelected(); break;
        case 15: _t->onObjectItemActivated((*reinterpret_cast< QListWidgetItem*(*)>(_a[1]))); break;
        case 16: _t->onSaveCustomFigure(); break;
        default: ;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
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
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 17;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
