#include <QString>
#include <QtEndian>

#pragma once

class QPalmDBRecordPrivate;
class QPalmDBRecord {
    friend class QPalmDB;

private:
    QPalmDBRecord(const QByteArray& data);

    const char* rawField(qint32 a_size);

public:
    ~QPalmDBRecord();

    bool isEmpty();

    void rewind();

    template <class T>
    T integerField()
    {
        return qFromBigEndian<T>(rawField(sizeof(T)));
    }
    QByteArray byteArrayField(qint32 a_size);
    QString stringField();

private:
    QPalmDBRecordPrivate* d_ptr = nullptr;
    Q_DECLARE_PRIVATE(QPalmDBRecord)
};
