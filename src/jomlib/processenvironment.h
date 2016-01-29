/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of jom.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
