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

#ifndef PARSER_H
#define PARSER_H

#include <QRegExp>
#include <QHash>
#include <QVector>
#include <QStack>
#include <QStringList>

#include "makefile.h"

namespace NMakeFile {

class Preprocessor;
class PPExpression;

class Parser
{
public:
    Parser();
    virtual ~Parser();

    void apply(Preprocessor* pp,
               Makefile* mkfile,
               const QStringList& activeTargets = QStringList());
    MacroTable* macroTable();

private:
    void readLine();
    bool isEmptyLine(const QString& line);
    bool isDescriptionBlock(int& separatorPos, int& separatorLength, int& commandSeparatorPos);
    bool isInferenceRule(const QString& line);
    bool isDotDirective(const QString& line);
    DescriptionBlock* createTarget(const QString& targetName);
    void parseDescriptionBlock(int separatorPos, int separatorLength, int commandSeparatorPos);
    void parseInferenceRule();
    void parseDotDirective();
    bool parseCommand(QList<Command>& commands, bool inferenceRule);
    void parseCommandLine(const QString& cmdLine, QList<Command>& commands, bool inferenceRule);
    void parseInlineFiles(Command& cmd, bool inferenceRule);
    void checkForCycles(DescriptionBlock* target);
    void resetCycleChecker(DescriptionBlock* target);
    QVector<InferenceRule*> findRulesByTargetName(const QString& targetFilePath);
    void preselectInferenceRules(DescriptionBlock *target);
    void error(const QString& msg);

private:
    Preprocessor*               m_preprocessor;
    QString                     m_line;
    bool                        m_silentCommands;
    bool                        m_ignoreExitCodes;

    QRegExp                     m_rexDotDirective;
    QRegExp                     m_rexInferenceRule;
    QRegExp                     m_rexSingleWhiteSpace;

    Makefile*                   m_makefile;
    QStringList                 m_suffixes;
    QStringList                 m_activeTargets;
    QHash<QString, QStringList> m_syncPoints;
    QHash<QString, QVector<InferenceRule *> > m_ruleIdxByToExtension;
};

} // namespace NMakeFile

#endif // PARSER_H
