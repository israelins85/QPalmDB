#pragma once

#include "qpalmdbrecord.hpp"

#include <QDateTime>
#include <QString>

class QFile;
class QPalmDBPrivate;
class QPalmDB {
public:
    QPalmDB();
    QPalmDB(QFile* a_file);
    QPalmDB(const QString& a_fileName);

    ~QPalmDB();

    bool load(const QString& a_fileName);

    QString name() const;
    qint32 numRecords() const;

    QPalmDBRecord record(qint32 a_idx);

private:
    QPalmDBPrivate* d_ptr = nullptr;
    Q_DECLARE_PRIVATE(QPalmDB)
};
