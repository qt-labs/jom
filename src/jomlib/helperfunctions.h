#pragma once

#include <QtCore/QString>
#include <QtCore/QStringList>

inline QString fileNameFromFilePath(const QString& filePath)
{
    int idx = qMax(filePath.lastIndexOf(QLatin1Char('/')), filePath.lastIndexOf(QLatin1Char('\\')));
    if (idx == -1)
        return filePath;
    QString fileName = filePath;
    fileName.remove(0, idx+1);
    return fileName;
}

inline bool isSpaceOrTab(const QChar& ch)
{
    return ch == QLatin1Char(' ') || ch == QLatin1Char('\t');
}

inline bool startsWithSpaceOrTab(const QString& str)
{
    Q_ASSERT(!str.isEmpty());
    return isSpaceOrTab(str.at(0));
}

inline void removeDirSeparatorAtEnd(QString& directory)
{
    if (directory.endsWith(QLatin1Char('/')) || directory.endsWith(QLatin1Char('\\')))
        directory.chop(1);
}

/**
 * Splits the string, respects "foo bar" and "foo ""knuffi"" bar".
 */
QStringList splitCommandLine(QString commandLine);
