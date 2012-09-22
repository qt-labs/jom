#ifndef PROCESSENVIRONMENT_H
#define PROCESSENVIRONMENT_H

#include <QtCore/QMap>
#include <QtCore/QString>

namespace NMakeFile {

/**
 * Key for the ProcessEnvironment class.
 */
class ProcessEnvironmentKey
{
public:
    ProcessEnvironmentKey(const QString &key)
        : m_key(key)
    {
    }

    ProcessEnvironmentKey(const QLatin1String &key)
        : m_key(key)
    {
    }

    const QString &toQString() const
    {
        return m_key;
    }

    int compare(const ProcessEnvironmentKey &other) const
    {
        return m_key.compare(other.m_key, Qt::CaseInsensitive);
    }

private:
    QString m_key;
};

inline bool operator < (const ProcessEnvironmentKey &lhs, const ProcessEnvironmentKey &rhs)
{
    return lhs.compare(rhs) < 0;
}

typedef QMap<ProcessEnvironmentKey, QString> ProcessEnvironment;

} // namespace NMakeFile

#endif // PROCESSENVIRONMENT_H
