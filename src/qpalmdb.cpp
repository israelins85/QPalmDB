#include "qpalmdb.hpp"

#include <QDateTime>
#include <QDebug>
#include <QFile>
#include <QtEndian>

class QPalmDBPrivate {
public:
    enum class HeaderFlag {
        Resource = 0x0001, /* Is this a resource file ? */
        ReadOnly = 0x0002, /* Is database read only ? */
        AppInfoDirty = 0x0004, /* Is application info block dirty ? */
        Backup = 0x0008, /* Back up to PC if no conduit defined */
        OKToInstallNewer = 0x0010, /* OK to install a newer version if current database open */
        ResetAfterInstall = 0x0020, /* Must reset machine after installation */
        Stream = 0x0080, /* Used for file streaming */
        Open = 0x8000 /* Not closed properly */
    };
    Q_DECLARE_FLAGS(HeaderFlags, HeaderFlag)

#define kMaxPDBNameSize 32

    struct Header {
        QString name; // 32 bytes
        HeaderFlags flags; // quint16
        quint16 version;
        QDateTime creationTime; // quint32
        QDateTime modificationTime; // quint32
        QDateTime backupTime; // quint32
        quint32 modificationNumber;
        quint32 appInfoOffset;
        quint32 sortInfoOffset;
        quint32 type;
        quint32 creator;
        quint32 uniqueID;
        quint32 nextRecordID;
        quint16 numRecords;
    };

    struct PDBResourceEntry {
        quint32 type;
        quint16 id;
        quint32 offset;
        quint32 length; // calculated
    };

    enum class RecordAttr {
        Secret = 0x10, /* Secret record, protected by password */
        Busy = 0x20, /* Record is in use */
        Dirty = 0x40, /* Record is in use */
        Delete = 0x80, /* Record is in use */
        CategoryMask = 0x0F /* Mask to extract category from attribute */
    };
    Q_DECLARE_FLAGS(RecordAttrs, RecordAttr)

    struct PDBRecordEntry {
        quint32 offset;
        RecordAttrs attr; // quint8
        quint32 uniqueID; // quint24
        quint32 length; // calculated
    };

    QList<PDBResourceEntry> resourcesEntries;
    QList<PDBRecordEntry> recordsEntries;

    QFile* file = nullptr;
    bool ownsFilePtr = false;
    Header header;

    QPalmDBPrivate()
    {
    }

    QPalmDBPrivate(QFile* a_file)
    {
        file = a_file;
        ownsFilePtr = false;
        load();
    }

    QPalmDBPrivate(const QString& a_fileName)
    {
        load(a_fileName);
    }

    ~QPalmDBPrivate()
    {
        if (ownsFilePtr && file) {
            delete file;
        }
    }

    QDateTime qDateTimeFromPalmEpochBigEndian(const void* src)
    {
        quint64 l_palmEpoch = qFromBigEndian<quint32>(src);
        l_palmEpoch += 2082844886LU;
        return QDateTime::fromSecsSinceEpoch(l_palmEpoch);
    }

    bool loadRecordsEntries()
    {
        recordsEntries.reserve(header.numRecords);
        for (int i = 0; i < header.numRecords; ++i) {
            PDBRecordEntry re;
            long l;

            re.offset = qFromBigEndian<quint32>(file->read(4));
            /*printf("Record %d offset = %d\n", i, p->rcp[i].offset);*/
            l = qFromBigEndian<quint16>(file->read(4));
            re.attr = RecordAttr((l >> 24) & 0xFF);
            re.uniqueID = l & 0xFFFFFF;
            if (i > 0) {
                PDBRecordEntry& pre = recordsEntries[i - 1];
                pre.length = re.offset - pre.offset;
            }
            recordsEntries.append(re);
        }
        if (header.numRecords > 0) {
            PDBRecordEntry& pre = recordsEntries[header.numRecords - 1];
            pre.length = file->size() - pre.offset;
        }
        return true;
    }

    bool loadResourcesEntries()
    {
        resourcesEntries.reserve(header.numRecords);
        for (int i = 0; i < header.numRecords; ++i) {
            PDBResourceEntry re;
            re.type = 1;
            re.id = 1;
            re.offset = 1;
            if (i > 0) {
                PDBResourceEntry& pre = resourcesEntries[i - 1];
                pre.length = re.offset - pre.offset;
            }
            resourcesEntries.append(re);
        }
        if (header.numRecords > 0) {
            PDBResourceEntry& pre = resourcesEntries[header.numRecords - 1];
            pre.length = file->size() - pre.offset;
        }
        return true;
    }

    bool load()
    {
        if (!file) {
            return false;
        }

        if (!file->isReadable()) {
            qWarning() << "File not readable" << file->fileName() << file->error();
            return false;
        }

        header.name = QString::fromLatin1(file->read(kMaxPDBNameSize));
        header.flags = QPalmDBPrivate::HeaderFlags(qFromBigEndian<quint16>(file->read(2)));
        header.version = qFromBigEndian<quint16>(file->read(2));
        header.creationTime = qDateTimeFromPalmEpochBigEndian(file->read(4));
        header.modificationTime = qDateTimeFromPalmEpochBigEndian(file->read(4));
        header.backupTime = qDateTimeFromPalmEpochBigEndian(file->read(4));
        header.modificationNumber = qFromBigEndian<quint32>(file->read(4));
        header.appInfoOffset = qFromBigEndian<quint32>(file->read(4));
        header.sortInfoOffset = qFromBigEndian<quint32>(file->read(4));
        header.type = qFromBigEndian<quint32>(file->read(4));
        header.creator = qFromBigEndian<quint32>(file->read(4));
        header.uniqueID = qFromBigEndian<quint32>(file->read(4));
        header.nextRecordID = qFromBigEndian<quint32>(file->read(4));
        header.numRecords = qFromBigEndian<quint16>(file->read(2));

        if (header.flags.testFlag(QPalmDBPrivate::HeaderFlag::Resource)) {
            return loadResourcesEntries();
        }

        return loadRecordsEntries();
    }

    bool load(const QString& a_fileName)
    {
        if (!ownsFilePtr) {
            file = new QFile();
            ownsFilePtr = true;
        }
        file->setFileName(a_fileName);
        if (!file->open(QFile::ReadOnly)) {
            qWarning() << file->error();
            return false;
        }
        return load();
    }
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QPalmDBPrivate::HeaderFlags)

QPalmDB::QPalmDB()
    : d_ptr(new QPalmDBPrivate())
{
}
QPalmDB::QPalmDB(QFile* a_file)
    : d_ptr(new QPalmDBPrivate(a_file))
{
}
QPalmDB::QPalmDB(const QString& a_fileName)
    : d_ptr(new QPalmDBPrivate(a_fileName))
{
}

QPalmDB::~QPalmDB()
{
    delete d_ptr;
}

bool QPalmDB::load(const QString& a_fileName)
{
    Q_D(QPalmDB);
    return d->load(a_fileName);
}

QString QPalmDB::name() const
{
    Q_D(const QPalmDB);
    return d->header.name;
}

qint32 QPalmDB::numRecords() const
{
    Q_D(const QPalmDB);
    return d->header.numRecords;
}

QPalmDBRecord QPalmDB::record(qint32 a_idx)
{
    Q_D(QPalmDB);

    if (d->recordsEntries.size() <= a_idx) {
        return QPalmDBRecord(QByteArray());
    }

    QPalmDBPrivate::PDBRecordEntry l_re = d->recordsEntries[a_idx];
    d->file->seek(l_re.offset);
    QByteArray l_data = d->file->read(l_re.length);
    return QPalmDBRecord(l_data);
}
