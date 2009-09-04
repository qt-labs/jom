#pragma once

#include <QtCore/QString>

inline QString fileNameFromFilePath(const QString& filePath)
{
    int idx = qMax(filePath.lastIndexOf('/'), filePath.lastIndexOf('\\'));
    if (idx == -1)
        return filePath;
    QString fileName = filePath;
    fileName.remove(0, idx+1);
    return fileName;
}
