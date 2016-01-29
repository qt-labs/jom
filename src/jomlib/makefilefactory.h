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

#ifndef MAKEFILEFACTORY_H
#define MAKEFILEFACTORY_H

#include "processenvironment.h"
#include <QtCore/QStringList>

namespace NMakeFile {

class Makefile;
class Options;

class MakefileFactory
{
public:
    MakefileFactory();
    void setEnvironment(const QStringList& env);
    bool apply(const QStringList& commandLineArguments, Options **outopt = 0);

    enum ErrorType {
        NoError,
        CommandLineError,
        ParserError,
        IOError
    };

    Makefile* makefile() { return m_makefile; }
    const QStringList& activeTargets() const { return m_activeTargets; }
    const QString& errorString() const { return m_errorString; }
    ErrorType errorType() const { return m_errorType; }

private:
    void clear();

private:
    Makefile*   m_makefile;
    ProcessEnvironment m_environment;
    QStringList m_activeTargets;
    QString     m_errorString;
    ErrorType   m_errorType;
};

} // namespace NMakeFile

#endif // MAKEFILEFACTORY_H
