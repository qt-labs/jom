/****************************************************************************
 **
 ** Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 ** Contact: Qt Software Information (qt-info@nokia.com)
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
#include "makefile.h"
#include <QFileInfo>
#include <QDebug>

namespace NMakeFile {

size_t DescriptionBlock::m_nextId = 0;

InlineFile::InlineFile()
:   m_keep(false),
    m_unicode(false)
{
}

InlineFile::InlineFile(const InlineFile& rhs)
:   m_keep(rhs.m_keep),
    m_unicode(rhs.m_unicode),
    m_filename(rhs.m_filename),
    m_content(rhs.m_content)
{
}

Command::Command()
:   m_inlineFile(0),
    m_maxExitCode(0),
    m_silent(false)
{
}

Command::Command(const Command& rhs)
:   m_maxExitCode(rhs.m_maxExitCode),
    m_commandLine(rhs.m_commandLine),
    m_silent(rhs.m_silent)
{
    if (rhs.m_inlineFile)
        m_inlineFile = new InlineFile(*rhs.m_inlineFile);
    else
        m_inlineFile = 0;
}

Command::~Command()
{
    delete m_inlineFile;
}

DescriptionBlock::DescriptionBlock()
:   m_bFileExists(false),
    m_canAddCommands(ACSUnknown),
    m_bExecuting(false)
{
    m_id = m_nextId++;
}

InferenceRule::InferenceRule()
{
}

InferenceRule::InferenceRule(const InferenceRule& rhs)
:   CommandContainer(rhs),
    m_batchMode(rhs.m_batchMode),
    m_fromExtension(rhs.m_fromExtension),
    m_fromSearchPath(rhs.m_fromSearchPath),
    m_toExtension(rhs.m_toExtension),
    m_toSearchPath(rhs.m_toSearchPath)
{
}

bool InferenceRule::operator == (const InferenceRule& rhs) const
{
    return m_batchMode == rhs.m_batchMode &&
           m_fromExtension == rhs.m_fromExtension &&
           m_fromSearchPath == rhs.m_fromSearchPath &&
           m_toExtension == rhs.m_toExtension &&
           m_toSearchPath == rhs.m_toSearchPath;
}

void Makefile::clear()
{
    QHash<QString, DescriptionBlock*>::iterator it = m_targets.begin();
    QHash<QString, DescriptionBlock*>::iterator itEnd = m_targets.end();
    for (; it != itEnd; ++it)
        delete it.value();

    m_firstTarget = 0;
    m_targets.clear();
    m_suffixesLists.clear();
    m_preciousTargets.clear();
    m_inferenceRules.clear();
}

void Makefile::dumpTarget(DescriptionBlock* db, uchar level) const
{
    QString indent = "";
    if (level > 0)
        for (int i=0; i < level; ++i)
            indent.append("  ");

    qDebug() << indent + db->m_target << db->m_timeStamp.toString();
    foreach (const QString depname, db->m_dependents)
        dumpTarget(target(depname), level+1);
}

void Makefile::filterRulesByDependent(QList<InferenceRule*>& rules, const QString& targetName)
{
    QFileInfo fi(targetName);
    QString baseName = fi.baseName();

    QList<InferenceRule*>::iterator it = rules.begin();
    while (it != rules.end()) {
        const InferenceRule* rule = *it;
        QString dependentName = rule->m_fromSearchPath + QLatin1Char('\\') +
                                baseName + rule->m_fromExtension;

        DescriptionBlock* depTarget = m_targets.value(dependentName);
        if ((depTarget && depTarget->m_bFileExists) || QFile::exists(dependentName)) {
            ++it;
            continue;
        }

        it = rules.erase(it);
    }    
}

void Makefile::sortRulesBySuffixes(QList<InferenceRule*>& rules, const QStringList& suffixes)
{
    Q_UNUSED(rules);
    Q_UNUSED(suffixes);
    //TODO: assign each rule with its .SUFFIXES array index and
    //      find the first rule with the lowest index number. Choose this one.
}

void Makefile::applyInferenceRules(DescriptionBlock* target)
{
    if (target->m_inferenceRules.isEmpty())
        return;

    QList<InferenceRule*> rules = target->m_inferenceRules;
    filterRulesByDependent(rules, target->m_target);
    //sortRulesBySuffixes(rules, *target->m_suffixes.data());

    if (rules.isEmpty()) {
        //qDebug() << "XXX" << target->m_target << "no matching inference rule found.";
        return;
    }

    // take the last matching inference rule
    const InferenceRule* matchingRule = rules.last();
    applyInferenceRule(target, matchingRule);
    target->m_inferenceRules.clear();
}

static void replaceFileMacros(QString& str, const QString& dependent)
{
    // TODO: handle more file macros here
    str.replace("$<", dependent);
}

void Makefile::applyInferenceRule(DescriptionBlock* target, const InferenceRule* rule)
{
    const QString& targetName = target->m_target;
    //qDebug() << "----> applyInferenceRule for" << targetName;

    QFileInfo fi(targetName);
    QString inferredDependent = fi.baseName() + rule->m_fromExtension;
    if (rule->m_fromSearchPath != QLatin1String("."))
        inferredDependent.prepend(rule->m_fromSearchPath + QLatin1Char('\\'));

    if (!target->m_dependents.contains(inferredDependent))
        target->m_dependents.append(inferredDependent);
    target->m_commands = rule->m_commands;

    //qDebug() << "----> inferredDependent:" << inferredDependent;

    QList<Command>::iterator it = target->m_commands.begin();
    QList<Command>::iterator itEnd = target->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& command = *it;
        if (command.m_inlineFile) {
            replaceFileMacros(command.m_inlineFile->m_content, inferredDependent);
            command.m_inlineFile->m_content = m_macroTable->expandMacros(command.m_inlineFile->m_content);
        }
        replaceFileMacros(command.m_commandLine, inferredDependent);
        command.m_commandLine = m_macroTable->expandMacros(command.m_commandLine);
    }
}

} // namespace NMakeFile
