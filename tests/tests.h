/****************************************************************************
 **
 ** Copyright (C) 2008-2010 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the jom project on Trolltech Labs.
 **
 ** This file may be used under the terms of the GNU General Public
 ** License version 2.0 or 3.0 as published by the Free Software Foundation
 ** and appearing in the file LICENSE.GPL included in the packaging of
 ** this file.  Please review the following information to ensure GNU
 ** General Public Licensing requirements will be met:
 ** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
 ** http://www.gnu.org/copyleft/gpl.html.
 **
 ** If you are unsure which license is appropriate for your use, please
 ** contact the sales department at qt-sales@nokia.com.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ****************************************************************************/
#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QProcess>

namespace NMakeFile
{
    class Preprocessor;
    class MakefileFactory;
}

class ParserTest : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    // preprocessor tests
    void includeFiles();
    void includeCycle();
    void macros();
    void preprocessorExpressions_data();
    void preprocessorExpressions();
    void preprocessorDivideByZero();
    void preprocessorInvalidExpressions_data();
    void preprocessorInvalidExpressions();
    void conditionals();
    void dotDirectives();

    // parser tests
    void descriptionBlocks();
    void inferenceRules_data();
    void inferenceRules();
    void cycleInTargets();
    void dependentsWithSpace();
    void multipleTargets();
    void comments();
    void fileNameMacros();
    void fileNameMacrosInDependents();
    void windowsPathsInTargetName();

    // black-box tests
    void ignoreExitCodes();
    void inlineFiles();
    void unicodeFiles();

private:
    bool openMakefile(const QString& fileName);
    bool runJom(const QStringList &args);

private:
    QString m_oldCurrentPath;
    NMakeFile::Preprocessor* m_preprocessor;
    NMakeFile::MakefileFactory* m_makefileFactory;
    QProcess *m_jomProcess;
};
