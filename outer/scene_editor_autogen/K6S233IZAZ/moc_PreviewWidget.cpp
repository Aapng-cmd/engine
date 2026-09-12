/****************************************************************************
** Meta object code from reading C++ file 'PreviewWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.19)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../headers/PreviewWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PreviewWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.19. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PreviewWidget_t {
    QByteArrayData data[11];
    char stringdata0[125];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PreviewWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PreviewWidget_t qt_meta_stringdata_PreviewWidget = {
    {
QT_MOC_LITERAL(0, 0, 13), // "PreviewWidget"
QT_MOC_LITERAL(1, 14, 12), // "objectPicked"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 5), // "index"
QT_MOC_LITERAL(4, 34, 10), // "meshEdited"
QT_MOC_LITERAL(5, 45, 16), // "playStateChanged"
QT_MOC_LITERAL(6, 62, 17), // "debugLayerChanged"
QT_MOC_LITERAL(7, 80, 5), // "layer"
QT_MOC_LITERAL(8, 86, 15), // "transformEdited"
QT_MOC_LITERAL(9, 102, 11), // "cameraMoved"
QT_MOC_LITERAL(10, 114, 10) // "onPlayTick"

    },
    "PreviewWidget\0objectPicked\0\0index\0"
    "meshEdited\0playStateChanged\0"
    "debugLayerChanged\0layer\0transformEdited\0"
    "cameraMoved\0onPlayTick"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PreviewWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   49,    2, 0x06 /* Public */,
       4,    1,   52,    2, 0x06 /* Public */,
       5,    0,   55,    2, 0x06 /* Public */,
       6,    1,   56,    2, 0x06 /* Public */,
       8,    1,   59,    2, 0x06 /* Public */,
       9,    0,   62,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    0,   63,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void PreviewWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PreviewWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->objectPicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->meshEdited((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->playStateChanged(); break;
        case 3: _t->debugLayerChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->transformEdited((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->cameraMoved(); break;
        case 6: _t->onPlayTick(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PreviewWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::objectPicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PreviewWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::meshEdited)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PreviewWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::playStateChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PreviewWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::debugLayerChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (PreviewWidget::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::transformEdited)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (PreviewWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PreviewWidget::cameraMoved)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PreviewWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QOpenGLWidget::staticMetaObject>(),
    qt_meta_stringdata_PreviewWidget.data,
    qt_meta_data_PreviewWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PreviewWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PreviewWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PreviewWidget.stringdata0))
        return static_cast<void*>(this);
    return QOpenGLWidget::qt_metacast(_clname);
}

int PreviewWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QOpenGLWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void PreviewWidget::objectPicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PreviewWidget::meshEdited(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PreviewWidget::playStateChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void PreviewWidget::debugLayerChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void PreviewWidget::transformEdited(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void PreviewWidget::cameraMoved()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
