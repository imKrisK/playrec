/****************************************************************************
** Meta object code from reading C++ file 'capture_thread.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/capture_thread.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'capture_thread.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
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
struct qt_meta_tag_ZN13CaptureThreadE_t {};
} // unnamed namespace

template <> constexpr inline auto CaptureThread::qt_create_metaobjectdata<qt_meta_tag_ZN13CaptureThreadE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "CaptureThread",
        "captureStarted",
        "",
        "captureStopped",
        "captureError",
        "error",
        "frameReady",
        "frame",
        "statsUpdated",
        "fps",
        "frames",
        "dropped",
        "fileSize"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'captureStarted'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'captureStopped'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'captureError'
        QtMocHelpers::SignalData<void(const QString &)>(4, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 5 },
        }}),
        // Signal 'frameReady'
        QtMocHelpers::SignalData<void(const QImage &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QImage, 7 },
        }}),
        // Signal 'statsUpdated'
        QtMocHelpers::SignalData<void(int, int, int, qint64)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 9 }, { QMetaType::Int, 10 }, { QMetaType::Int, 11 }, { QMetaType::LongLong, 12 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<CaptureThread, qt_meta_tag_ZN13CaptureThreadE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject CaptureThread::staticMetaObject = { {
    QMetaObject::SuperData::link<QThread::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13CaptureThreadE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13CaptureThreadE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN13CaptureThreadE_t>.metaTypes,
    nullptr
} };

void CaptureThread::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<CaptureThread *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->captureStarted(); break;
        case 1: _t->captureStopped(); break;
        case 2: _t->captureError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->frameReady((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        case 4: _t->statsUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (CaptureThread::*)()>(_a, &CaptureThread::captureStarted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (CaptureThread::*)()>(_a, &CaptureThread::captureStopped, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (CaptureThread::*)(const QString & )>(_a, &CaptureThread::captureError, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (CaptureThread::*)(const QImage & )>(_a, &CaptureThread::frameReady, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (CaptureThread::*)(int , int , int , qint64 )>(_a, &CaptureThread::statsUpdated, 4))
            return;
    }
}

const QMetaObject *CaptureThread::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CaptureThread::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN13CaptureThreadE_t>.strings))
        return static_cast<void*>(this);
    return QThread::qt_metacast(_clname);
}

int CaptureThread::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QThread::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void CaptureThread::captureStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CaptureThread::captureStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CaptureThread::captureError(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void CaptureThread::frameReady(const QImage & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}

// SIGNAL 4
void CaptureThread::statsUpdated(int _t1, int _t2, int _t3, qint64 _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1, _t2, _t3, _t4);
}
QT_WARNING_POP
