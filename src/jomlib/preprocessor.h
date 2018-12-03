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

#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "makefilelinereader.h"

#include <QRegExp>
#include <QStack>
#include <QStringList>

class PPExprParser;

namespace NMakeFile {

class MacroTable;
class MakefileLineReader;

class Preprocessor
{
public:
    Preprocessor();
    ~Preprocessor();

    void setMacroTable(MacroTable* macroTable);
    MacroTable* macroTable() { return m_macroTable; }
    bool openFile(const QString& filename);
    QString readLine();
    uint lineNumber() const;
    QString currentFileName() const;
    int evaluateExpression(const QString& expr);
    bool isInlineFileMode() const { return m_bInlineFileMode; }
    void setInlineFileModeEnabled(bool enabled) { m_bInlineFileMode = enabled; }

    static void removeInlineComments(QString& line);

private:
    bool internalOpenFile(QString fileName);
    MakefileLine basicReadLine();
    typedef void (*JoinFunc)(MakefileLine &, const MakefileLine &);
    static void joinLines(MakefileLine &line, const MakefileLine &next);
    static void joinPreprocessingDirectiveLines(MakefileLine &line, const MakefileLine &next);
    void completeLineImpl(MakefileLine &line, JoinFunc joinFunc);
    void completeLine(MakefileLine &line);
    void completePreprocessingDirectiveLine(MakefileLine &line);
    bool parseMacro(const QString& line);
    bool parsePreprocessingDirective(const QString& line);
    QString findIncludeFile(const QString &filePathToInclude);
    bool isPreprocessingDirective(const QString& line, QString& directive, QString& value);
    void skipUntilNextMatchingConditional();
    void error(const QString& msg);
    void enterConditional(bool followElseBranch);
    void exitConditional();
    int conditionalDepth() { return m_conditionalStack.count(); }

private:
    struct TextFile
    {
        MakefileLineReader* reader;
        QString fileDirectory;

        TextFile()
            : reader(0)
        {}

        TextFile(const TextFile& rhs)
            : reader(rhs.reader), fileDirectory(rhs.fileDirectory)
        {}

        TextFile& operator=(const TextFile& rhs)
        {
            reader = rhs.reader;
            fileDirectory = rhs.fileDirectory;
            return *this;
        }
    };

    QStack<TextFile>    m_fileStack;
    MacroTable*         m_macroTable;
    QRegExp             m_rexPreprocessingDirective;
    QStack<bool>        m_conditionalStack;
    PPExprParser*       m_expressionParser;
    QStringList         m_linesPutBack;
    bool                m_bInlineFileMode;
};

} //namespace NMakeFile

#endif // PREPROCESSOR_H
