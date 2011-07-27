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
#include "makefile.h"
#include "fileinfo.h"
#include "exception.h"
#include "options.h"
#include <QDebug>

namespace NMakeFile {

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
:   m_maxExitCode(0),
    m_silent(false),
    m_singleExecution(false)
{
}

Command::Command(const Command& rhs)
:   m_maxExitCode(rhs.m_maxExitCode),
    m_commandLine(rhs.m_commandLine),
    m_silent(rhs.m_silent),
    m_singleExecution(rhs.m_singleExecution)
{
    foreach (InlineFile* inlineFile, rhs.m_inlineFiles)
        m_inlineFiles.append(new InlineFile(*inlineFile));
}

Command::~Command()
{
    qDeleteAll(m_inlineFiles);
}

DescriptionBlock::DescriptionBlock(Makefile* mkfile)
:   m_bFileExists(false),
    m_canAddCommands(ACSUnknown),
    m_bVisitedByCycleCheck(false),
    m_pMakefile(mkfile)
{
}

void DescriptionBlock::setTargetName(const QString& name)
{
    m_targetName = name;
    m_targetFilePath = name;
    if (m_targetFilePath.startsWith(QLatin1Char('"')) && m_targetFilePath.endsWith(QLatin1Char('"'))) {
        m_targetFilePath.remove(0, 1);
        m_targetFilePath.chop(1);
    }
}

/**
 * Expands the following macros for the dependents of this target.
 */
void DescriptionBlock::expandFileNameMacrosForDependents()
{
    QStringList::iterator it = m_dependents.begin();
    for (; it != m_dependents.end(); ++it) {
        QString& dependent = *it;
        dependent.replace(QLatin1String("$$@"), QLatin1String("$@"));
        dependent.replace(QLatin1String("$$(@"), QLatin1String("$(@"));
        expandFileNameMacros(dependent, -1, true);
    }
}

void DescriptionBlock::expandFileNameMacros()
{
    QList<Command>::iterator it = m_commands.begin();
    while (it != m_commands.end()) {
        if ((*it).m_singleExecution) {
            Command origCommand = *it;
            it = m_commands.erase(it);
            for (int i=0; i < m_dependents.count(); ++i) {
                Command newCommand = origCommand;
                newCommand.m_singleExecution = false;
                expandFileNameMacros(newCommand, i);
                it = m_commands.insert(it, newCommand);
                ++it;
            }
        } else {
            expandFileNameMacros(*it, -1);
            ++it;
        }
    }
}

void DescriptionBlock::expandFileNameMacros(Command& command, int depIdx)
{
    expandFileNameMacros(command.m_commandLine, depIdx, false);
    foreach (InlineFile* inlineFile, command.m_inlineFiles) {
        expandFileNameMacros(inlineFile->m_filename, depIdx, false);
        expandFileNameMacros(inlineFile->m_content, depIdx, false);
    }
}

inline void quoteStringIfNeeded(QString& str)
{
    if (str.contains(QLatin1Char(' ')) || str.contains(QLatin1Char('\t'))) {
        str.prepend(QLatin1Char('"'));
        str.append(QLatin1Char('"'));
    }
}

/**
 * Expands the filename macros in a given string.
 *
 * If parameter depIdx == -1, then all dependents are considered.
 * Otherwise depIdx is the index of the dependent we want to put into the command.
 * This is used for commands with the ! specifier.
 *
 * If dependentsForbidden is true, then only the file name macros that reference
 * the target name are taken into account. This is used for file name macros in
 * dependencies.
 */
void DescriptionBlock::expandFileNameMacros(QString& str, int depIdx, bool dependentsForbidden)
{
    int idx = 0;
    int lastEscapedIdx = -1;
    forever {
        idx = str.indexOf(QLatin1Char('$'), idx);
        if (idx == -1 || ++idx >= str.count() + 1)
            break;

        if (lastEscapedIdx == idx - 1)
            continue;

        if (idx - 2 >= 0) {
            char chBefore = str.at(idx - 2).toLatin1();
            if (chBefore == '^') {
                lastEscapedIdx = idx - 1;
                continue;
            }
            if (chBefore == '$' && lastEscapedIdx < idx - 2) {
                idx -= 2;
                lastEscapedIdx = idx;
                str.remove(idx, 1);
                continue;
            }
        }

        int replacementLength = 0;
        char ch = str.at(idx).toLatin1();
        if (ch == '(') {
            int substitutionIdx = -1;
            bool substitutionStateKnown = false;
            bool fileNameReturned;
            QString macroValue = getFileNameMacroValue(str.midRef(idx+1), replacementLength,
                                                       depIdx, dependentsForbidden, fileNameReturned);
            if (!macroValue.isNull()) {
                int k;
                ch = str.at(idx + replacementLength + 1).toLatin1();
                switch (ch)
                {
                case 'D':
                    k = macroValue.lastIndexOf(QLatin1Char('\\'));
                    if (k == -1)
                        macroValue = QLatin1String(".");
                    else
                        macroValue = macroValue.left(k);
                    break;
                case 'B':
                    macroValue = FileInfo(macroValue).baseName();
                    break;
                case 'F':
                    macroValue = FileInfo(macroValue).fileName();
                    break;
                case 'R':
                    k = macroValue.lastIndexOf(QLatin1Char('.'));
                    if (k > -1)
                        macroValue = macroValue.left(k);
                    break;
                case ':':
                    substitutionStateKnown = true;
                    substitutionIdx = idx + replacementLength + 2;
                    break;
                case ')':
                    // No file name modifier given.
                    substitutionStateKnown = true;
                    break;
                default:
                    // TODO: yield error? ignore for now
                    continue;
                }

                // We've seen D, B, F or R and don't know yet whether we should substitute something.
                if (!substitutionStateKnown && str.at(idx + replacementLength + 2) == QLatin1Char(':'))
                    substitutionIdx = idx + replacementLength + 3;

                if (substitutionIdx > 0) {
                    int macroInvokationEnd;
                    MacroTable::parseSubstitutionStatement(str, substitutionIdx, macroValue, macroInvokationEnd);
                    replacementLength = macroInvokationEnd - idx - 2;  // because we're later adding 4
                }

                if (fileNameReturned)
                    quoteStringIfNeeded(macroValue);
                str.replace(idx - 1, replacementLength + 4, macroValue);
            }
        } else {
            bool fileNameReturned;
            QString macroValue = getFileNameMacroValue(str.midRef(idx), replacementLength, depIdx,
                                                       dependentsForbidden, fileNameReturned);
            if (!macroValue.isNull()) {
                if (fileNameReturned)
                    quoteStringIfNeeded(macroValue);
                str.replace(idx - 1, replacementLength + 1, macroValue);
            }
        }
    }
}

QString DescriptionBlock::getFileNameMacroValue(const QStringRef& str, int& replacementLength,
                                                int depIdx, bool dependentsForbidden, bool& returnsFileName)
{
    QString result;
    QStringList dependentCandidates;
    returnsFileName = false;
    if (!dependentsForbidden) {
        if (depIdx == -1)
            dependentCandidates = m_dependents;
        else
            dependentCandidates << m_dependents.at(depIdx);
    }

    switch (str.at(0).toLatin1()) {
        case '@':
            replacementLength = 1;
            result = targetFilePath();
            returnsFileName = true;
            break;
        case '*':
            {
                if (str.length() >= 2 && str.at(1) == QLatin1Char('*')) {
                    if (dependentsForbidden) {
                        throw Exception(QLatin1String("Macro $** not allowed here."));
                    }
                    replacementLength = 2;
                    result = dependentCandidates.join(QLatin1String(" "));
                } else {
                    returnsFileName = true;
                    replacementLength = 1;
                    result = targetFilePath();
                    int idx = result.lastIndexOf(QLatin1Char('.'));
                    if (idx > -1)
                        result.resize(idx);
                }
            }
            break;
        case '?':
            {
                if (dependentsForbidden) {
                    throw Exception(QLatin1String("Macro $** not allowed here."));
                }
                replacementLength = 1;
                result = QLatin1String("");
                bool firstAppend = true;
                const FileTime currentTimeStamp = FileTime::currentTime();
                FileTime targetTimeStamp = FastFileInfo(targetFilePath()).lastModified();
                if (!targetTimeStamp.isValid())
                    targetTimeStamp = currentTimeStamp;

                foreach (const QString& dependentName, dependentCandidates) {
                    FileTime dependentTimeStamp = FastFileInfo(dependentName).lastModified();
                    if (!dependentTimeStamp.isValid())
                        dependentTimeStamp = currentTimeStamp;

                    if (targetTimeStamp <= dependentTimeStamp) {
                        if (firstAppend)
                            firstAppend = false;
                        else
                            result.append(QLatin1Char(' '));

                        result.append(dependentName);
                    }
                }
            }
            break;
    }

    return result;
}

InferenceRule::InferenceRule()
:   m_batchMode(false),
    m_priority(-1)
{
}

InferenceRule::InferenceRule(const InferenceRule& rhs)
:   CommandContainer(rhs),
    m_batchMode(rhs.m_batchMode),
    m_fromExtension(rhs.m_fromExtension),
    m_fromSearchPath(rhs.m_fromSearchPath),
    m_toExtension(rhs.m_toExtension),
    m_toSearchPath(rhs.m_toSearchPath),
    m_priority(rhs.m_priority)
{
}

bool InferenceRule::operator == (const InferenceRule& rhs) const
{
    return m_batchMode == rhs.m_batchMode &&
           m_fromExtension == rhs.m_fromExtension &&
           m_fromSearchPath == rhs.m_fromSearchPath &&
           m_toExtension == rhs.m_toExtension &&
           m_toSearchPath == rhs.m_toSearchPath &&
           m_priority == rhs.m_priority;
}

/**
 * Returns the name of the inferred dependent if this rule was applied to the
 * target with the given name. The target name is assumed to be a file name.
 * This function also takes quoted file names.
 */
QString InferenceRule::inferredDependent(const QString &targetName) const
{
    QString dependent = FileInfo(targetName).fileName();
    dependent.chop(m_toExtension.length());
    dependent.append(m_fromExtension);

    if (m_fromSearchPath != QLatin1String("."))
        dependent.prepend(m_fromSearchPath + QLatin1Char('\\'));

    return dependent;
}

Makefile::Makefile()
:   m_macroTable(0),
    m_firstTarget(0),
    m_options(0)
{
}

Makefile::~Makefile()
{
    delete m_macroTable;
    delete m_options;
}

void Makefile::clear()
{
    QHash<QString, DescriptionBlock*>::iterator it = m_targets.begin();
    QHash<QString, DescriptionBlock*>::iterator itEnd = m_targets.end();
    for (; it != itEnd; ++it)
        delete it.value();

    m_firstTarget = 0;
    m_targets.clear();
    m_preciousTargets.clear();
    m_inferenceRules.clear();
}

void Makefile::dumpTarget(DescriptionBlock* db, uchar level) const
{
    QString indent;
    if (level > 0)
        for (int i=0; i < level; ++i)
            indent.append(QLatin1String("  "));

    qDebug() << indent + db->targetName() << db->m_timeStamp.toString();
    foreach (const QString depname, db->m_dependents)
        dumpTarget(target(depname), level+1);
}

void Makefile::dumpTargets() const
{
    QHash<QString, DescriptionBlock*>::const_iterator it=m_targets.begin();
    for (; it != m_targets.end(); ++it) {
        DescriptionBlock* target = *it;
        printf(qPrintable(target->targetName()));
        printf(":\n\tdependents:");
        foreach (const QString& dependent, target->m_dependents) {
            printf("\t");
            printf(qPrintable(dependent));
            printf("\n");
        }
        printf("\tcommands:");
        foreach (const Command& cmd, target->m_commands) {
            printf("\t");
            printf(qPrintable(cmd.m_commandLine));
            printf("\n");
        }
        printf("\n");
    }

}

void Makefile::dumpInferenceRules() const
{
    foreach (const InferenceRule& ir, m_inferenceRules) {
        if (ir.m_fromSearchPath != QLatin1String(".")) {
            printf("{");
            printf(qPrintable(ir.m_fromSearchPath));
            printf("}");
        }
        printf(qPrintable(ir.m_fromExtension));
        if (ir.m_toSearchPath != QLatin1String(".")) {
            printf("{");
            printf(qPrintable(ir.m_toSearchPath));
            printf("}");
        }
        printf(qPrintable(ir.m_toExtension));
        if (ir.m_batchMode)
            printf("::\n");
        else
            printf(":\n");
        foreach (const Command& cmd, ir.m_commands) {
            printf("\t");
            printf(qPrintable(cmd.m_commandLine));
            printf("\n");
        }
        printf("\n");
    }
}

void Makefile::filterRulesByDependent(QList<InferenceRule*>& rules, const QString& targetName)
{
    FileInfo fi(targetName);
    QString targetFileName = fi.fileName();

    QList<InferenceRule*>::iterator it = rules.begin();
    while (it != rules.end()) {
        const InferenceRule* rule = *it;

        // Thanks to Parser::preselectInferenceRules the target name
        // is guaranteed to end with rule->m_toExtension.
        QString baseName = targetFileName;
        baseName.chop(rule->m_toExtension.length());
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

void Makefile::applyInferenceRules(QList<DescriptionBlock*> targets)
{
    foreach (DescriptionBlock *t, targets)
        applyInferenceRules(t);

    if (!m_batchModeTargets.isEmpty()) {
        foreach (const InferenceRule *rule, m_batchModeRules) {
            QList<DescriptionBlock*> allBatchTargets = m_batchModeTargets.values(rule);
            int filesPerBatch = qMax(1, qRound(float(allBatchTargets.count()) / float(g_options.maxNumberOfJobs)));
            do {
                QList<DescriptionBlock*> batch;
                for (int i=0; i < filesPerBatch && !allBatchTargets.isEmpty(); ++i)
                    batch.append( allBatchTargets.takeFirst() );
                applyInferenceRule(batch, rule);
            } while (!allBatchTargets.isEmpty());
        }
        m_batchModeRules.clear();
        m_batchModeTargets.clear();
    }
}

static bool infRulesPriorityGreaterThan(const InferenceRule *lhs, const InferenceRule *rhs)
{
    return lhs->m_priority > rhs->m_priority;
}

void Makefile::addInferenceRule(const InferenceRule& rule)
{
    QList<InferenceRule>::iterator it = qFind(m_inferenceRules.begin(),
                                              m_inferenceRules.end(),
                                              rule);
    if (it != m_inferenceRules.end())
        m_inferenceRules.erase(it);
    m_inferenceRules.append(rule);
}

void Makefile::calculateInferenceRulePriorities(const QStringList &suffixes)
{
    // inference rule priority is determined by .SUFFIXES
    QList<InferenceRule>::iterator it = m_inferenceRules.begin();
    for (; it != m_inferenceRules.end(); ++it) {
        InferenceRule &rule = *it;
        for (int i=0; i < suffixes.count(); ++i) {
            if (rule.m_fromExtension == suffixes.at(i)) {
                rule.m_priority = i;
                break;
            }
        }
    }
}

void Makefile::addPreciousTarget(const QString& targetName)
{
    if (!m_preciousTargets.contains(targetName))
        m_preciousTargets.append(targetName);
}

void Makefile::invalidateTimeStamps()
{
    QHash<QString, DescriptionBlock*>::iterator it = m_targets.begin();
    QHash<QString, DescriptionBlock*>::iterator itEnd = m_targets.end();
    for (; it != itEnd; ++it) {
        DescriptionBlock* target = it.value();
        target->m_timeStamp = FileTime();
        target->m_bFileExists = false;
    }
}

void Makefile::applyInferenceRules(DescriptionBlock* target)
{
    if (target->m_inferenceRules.isEmpty())
        return;

    QList<InferenceRule*> rules = target->m_inferenceRules;
    filterRulesByDependent(rules, target->targetName());
    qStableSort(rules.begin(), rules.end(), infRulesPriorityGreaterThan);

    if (rules.isEmpty()) {
        //qDebug() << "XXX" << target->m_targetName << "no matching inference rule found.";
        return;
    }

    // take the last matching inference rule
    const InferenceRule* matchingRule = rules.last();
    applyInferenceRule(target, matchingRule);
    target->m_inferenceRules.clear();
}

void Makefile::applyInferenceRule(DescriptionBlock* target, const InferenceRule* rule, bool applyingBatchMode)
{
    if (!applyingBatchMode && m_options->batchModeEnabled && rule->m_batchMode) {
        m_batchModeRules.insert(rule);
        m_batchModeTargets.insert(rule, target);
        return;
    }

    target->m_inferenceRules.clear();
    //qDebug() << "----> applyInferenceRule for" << target->targetName();

    QString inferredDependent = rule->inferredDependent(target->targetName());
    if (!target->m_dependents.contains(inferredDependent))
        target->m_dependents.append(inferredDependent);
    target->m_commands = rule->m_commands;

    //qDebug() << "----> inferredDependent:" << inferredDependent;

    QList<Command>::iterator it = target->m_commands.begin();
    QList<Command>::iterator itEnd = target->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& command = *it;
        foreach (InlineFile* inlineFile, command.m_inlineFiles) {
            inlineFile->m_content.replace(QLatin1String("$<"), inferredDependent);
            inlineFile->m_content = m_macroTable->expandMacros(inlineFile->m_content);
        }
        command.m_commandLine = m_macroTable->expandMacros(command.m_commandLine);
        command.m_commandLine.replace(QLatin1String("$<"), inferredDependent);
    }
}

void Makefile::applyInferenceRule(QList<DescriptionBlock*> &batch, const InferenceRule *rule)
{
    QString inferredDependents;
    DescriptionBlock *executingTarget = batch.first();
    foreach (DescriptionBlock *target, batch) {
        target->m_inferenceRules.clear();
        QString inferredDependent = rule->inferredDependent(target->targetFilePath());
        if (!executingTarget->m_dependents.contains(inferredDependent))
            executingTarget->m_dependents.append(inferredDependent);

        inferredDependents.append(inferredDependent);
        inferredDependents.append(QLatin1Char(' '));
    }

    executingTarget->m_commands = rule->m_commands;
    QList<Command>::iterator it = executingTarget->m_commands.begin();
    QList<Command>::iterator itEnd = executingTarget->m_commands.end();
    for (; it != itEnd; ++it) {
        Command& command = *it;
        foreach (InlineFile* inlineFile, command.m_inlineFiles) {
            inlineFile->m_content.replace(QLatin1String("$<"), inferredDependents);
            inlineFile->m_content = m_macroTable->expandMacros(inlineFile->m_content);
        }
        command.m_commandLine = m_macroTable->expandMacros(command.m_commandLine);
        command.m_commandLine.replace(QLatin1String("$<"), inferredDependents);
    }
}

} // namespace NMakeFile
