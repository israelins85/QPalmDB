#include "qpalmdbrecord.hpp"

#include <QBuffer>

class QPalmDBRecordPrivate {
public:
    QByteArray data;
    qint32 offset = 0;

    QPalmDBRecordPrivate(const QByteArray& a_data)
    {
        data = a_data;
    }

    const char* currentCursor()
    {
        return data.constData() + offset;
    }

    void skip(qint32 a_size)
    {
        offset += a_size;
    }

    const char* src(qint32 a_size)
    {
        const char* r = currentCursor();
        offset += a_size;
        return r;
    }
};

QPalmDBRecord::QPalmDBRecord(const QByteArray& data)
    : d_ptr(new QPalmDBRecordPrivate(data))
{
}

QPalmDBRecord::~QPalmDBRecord()
{
    delete d_ptr;
}

const char* QPalmDBRecord::rawField(qint32 a_size)
{
    Q_D(QPalmDBRecord);
    return d->src(a_size);
}

bool QPalmDBRecord::isEmpty()
{
    Q_D(QPalmDBRecord);
    return d->data.isEmpty();
}

void QPalmDBRecord::rewind()
{
    Q_D(QPalmDBRecord);
    d->offset = 0;
}

QByteArray QPalmDBRecord::byteArrayField(qint32 a_size)
{
    Q_D(QPalmDBRecord);
    return QByteArray(d->src(a_size), a_size);
}

QString QPalmDBRecord::stringField()
{
    Q_D(QPalmDBRecord);
    QString ret = QString::fromLatin1(d->currentCursor());
    d->skip(ret.length() + 1); // pula incluindo o zero
    return ret;
}
