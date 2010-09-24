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
#include "parser.h"
#include "preprocessor.h"
#include "options.h"
#include "exception.h"
#include "helperfunctions.h"

#include <QTextStream>
#include <QDebug>

namespace NMakeFile {

Parser::Parser()
:   m_preprocessor(0)
{
    m_rexDotDirective.setPattern("^\\.(IGNORE|PRECIOUS|SILENT|SUFFIXES)\\s*:(.*)");
    m_rexInferenceRule.setPattern("^(\\{.*\\})?(\\.\\w+)(\\{.*\\})?(\\.\\w+)(:{1,2})");
    m_rexSingleWhiteSpace.setPattern("\\s");
}

Parser::~Parser()
{
}

/**
 * Parses the content, provided by the Preprocessor object and
 * creates a new Makefile object.
 */
void Parser::apply(Preprocessor* pp,
                   Makefile* mkfile,
                   const QStringList& activeTargets)
{
    m_makefile = mkfile;
    m_activeTargets = activeTargets;
    m_preprocessor = pp;
    m_conditionalDepth = 0;
    const Options* options = mkfile->options();
    m_silentCommands = options->suppressOutputMessages;
    m_ignoreExitCodes = !options->stopOnErrors;
    m_latestTimeStamp = QDateTime::currentDateTime();
    m_suffixes.clear();
    m_suffixes << ".exe" << ".obj" << ".asm" << ".c" << ".cpp" << ".cxx"
               << ".bas" << ".cbl" << ".for" << ".pas" << ".res" << ".rc";
    int dbSeparatorPos, dbSeparatorLength, dbCommandSeparatorPos;

    try {
        readLine();
        while (!m_line.isNull()) {
            QString expandedLine = m_preprocessor->macroTable()->expandMacros(m_line);
            if (isEmptyLine(expandedLine)) {
                readLine();
            } else if (isDotDirective(expandedLine)) {
                m_line = expandedLine;
                Preprocessor::removeInlineComments(m_line);
                parseDotDirective();
            } else if (isInferenceRule(expandedLine)) {
                m_line = expandedLine;
                Preprocessor::removeInlineComments(m_line);
                parseInferenceRule();
            } else if (isDescriptionBlock(dbSeparatorPos, dbSeparatorLength, dbCommandSeparatorPos)) {
                parseDescriptionBlock(dbSeparatorPos, dbSeparatorLength, dbCommandSeparatorPos);
            } else {
                error("syntax error");
                readLine();
            }
        }
    } catch (FileException&) {
        throw;
    } catch (Exception &e) {
        // Enrich the error message with filename and line number.
        error(e.message());
    }

    // if the makefile doesn't contain target, we can stop here
    if (m_makefile->targets().isEmpty())
        return;

    // make sure that all active targets exist
    foreach (const QString& targetName, m_activeTargets)
        if (!m_makefile->target(targetName))
            throw Exception(QString("Target %1 doesn't exist.").arg(targetName));

    // if no active target is defined, use the first one
    if (m_activeTargets.isEmpty()) {
        m_activeTargets.append(m_makefile->firstTarget()->targetName());
    }

    // check for cycles in active targets
    foreach (const QString& targetName, m_activeTargets)
        checkForCycles(m_makefile->target(targetName));

    preselectInferenceRules();
}

MacroTable* Parser::macroTable()
{
    return m_preprocessor->macroTable();
}

void Parser::readLine()
{
    m_line = m_preprocessor->readLine();
}

bool Parser::isEmptyLine(const QString& line)
{
    return line.trimmed().isEmpty();
}

/**
 * Returns the index of the command separator or -1 if its non-existent.
 */
static int removeCommentsAndFindCommandSeparator(QString& line)
{
    if (line.isEmpty())
        return -1;

    QVector<bool> quotedVector(line.length());
    quotedVector[0] = (line.at(0) == QLatin1Char('"'));
    for (int i=1; i < line.length(); ++i) {
        bool isInDoubleQuote = quotedVector[i-1];
        if (line.at(i) == QLatin1Char('"'))
            isInDoubleQuote = !isInDoubleQuote;
        quotedVector[i] = isInDoubleQuote;
    }

    int commentIdx = -1;
    QList<int> toRemove;
    while (true) {
        commentIdx = line.indexOf(QLatin1Char('#'), commentIdx + 1);
        if (commentIdx > 0 && (line.at(commentIdx - 1) == QLatin1Char('^'))) {
            toRemove.append(commentIdx - 1);  // remove the ^ characters later
            continue;
        }
        if (commentIdx < 0 || !quotedVector[commentIdx])
            break;
    }

    int cmdSepIdx = -1;
    do {
        cmdSepIdx = line.indexOf(QLatin1Char(';'), cmdSepIdx + 1);
    } while (cmdSepIdx >= 0 && quotedVector[cmdSepIdx]);

    if (commentIdx == cmdSepIdx)  // equality means, they are both -1
        return -1;

    if ((0 <= commentIdx && commentIdx < cmdSepIdx) || cmdSepIdx == -1) {
        // The # indicates a comment.
        line.truncate(commentIdx);
        for (int i=toRemove.count() - 1; i >= 0; i--)
            line.remove(toRemove.at(i), 1);
        cmdSepIdx = -1;
    } else {
        // The # is inside the command and not a comment.
    }
    return cmdSepIdx;
}

/**
 * Test if this a description block and determine the separator position and length.
 *
 * The colon is in only one case not a separator:
 * If there's a single character before the colon.
 * In this case its interpreted as a drive letter.
 */
bool Parser::isDescriptionBlock(int& separatorPos, int& separatorLength, int& commandSeparatorPos)
{
    commandSeparatorPos = removeCommentsAndFindCommandSeparator(m_line);

    const int lineLength = m_line.length();
    if (lineLength == 0)
        return false;

    const QChar firstChar = m_line.at(0);
    if (isSpaceOrTab(firstChar))
        return false;

    separatorPos = -1;
    int i = -1;
    while (true) {
        i = m_line.indexOf(QLatin1Char(':'), i+1);
        if (i <= 0)
            break;
        if (i == 1 && firstChar.isLetter())
            continue;

        int k = i-1;
        int l = i-2;
        if (m_line.at(k).isLetter()) {
            QChar ch = m_line.at(l);
            if (ch.isSpace() || ch == '\"')
                continue;
        }

        separatorPos = i;
        break;
    }

    if (separatorPos < 0)
        return false;

    const int idx = separatorPos + 1;
    if (idx < lineLength && m_line.at(idx) == ':')
        separatorLength = 2;
    else
        separatorLength = 1;

    return true;
}

bool Parser::isInferenceRule(const QString& line)
{
    return m_rexInferenceRule.exactMatch(line);
}

bool Parser::isDotDirective(const QString& line)
{
    return m_rexDotDirective.exactMatch(line);
}

DescriptionBlock* Parser::createTarget(const QString& targetName)
{
    DescriptionBlock* target = new DescriptionBlock(m_makefile);
    target->setTargetName(targetName);
    target->m_suffixes = m_suffixes;
    m_makefile->append(target);
    return target;
}

static void split_append_helper(QStringList& lst, const QString& str, int from, int to)
{
    QString entry;
    entry = str.mid(from, to - from);
    entry = entry.trimmed();
    if (!entry.isEmpty())
        lst.append(entry);
}

static QStringList splitTargetNames(const QString& str)
{
    QStringList lst;
    bool inDoubleQuotes = false;
    int from = 0, to = 0;
    for (int i=0; i < str.length(); ++i) {
        const QChar ch = str.at(i);
        if (ch == '\"')
            inDoubleQuotes = !inDoubleQuotes;
        else if ((ch == ' ' || ch == '\t') && !inDoubleQuotes)
            to = i;
        if (from < to) {
            split_append_helper(lst, str, from, to);
            to++;
            from = to;
        }
    }

    if (from < str.length()) {
        split_append_helper(lst, str, from, str.length());
    }
    return lst;
}

void Parser::parseDescriptionBlock(int separatorPos, int separatorLength, int commandSeparatorPos)
{
    QString target = m_line.left(separatorPos).trimmed();
    target = m_preprocessor->macroTable()->expandMacros(target);
    QString value = m_line;
    if (commandSeparatorPos >= 0) value.truncate(commandSeparatorPos);
    value.remove(0, separatorPos + separatorLength);
    value = m_preprocessor->macroTable()->expandMacros(value.trimmed());
    target.replace(QLatin1Char('/'), QLatin1Char('\\'));
    value.replace(QLatin1Char('/'), QLatin1Char('\\'));

    // extract command from description block line
    QList<Command> commands;
    if (commandSeparatorPos >= 0) {
        QString inPlaceCommand;
        inPlaceCommand = m_line;
        inPlaceCommand.remove(0, commandSeparatorPos + 1);
        inPlaceCommand = inPlaceCommand.trimmed();
        parseCommandLine(inPlaceCommand, commands, false);
    }

    readLine();
    if (m_line.trimmed().isEmpty()) {
        readLine();
    } else {
        while (parseCommand(commands, false))
            readLine();
    }

    const QStringList targets = splitTargetNames(target);
    const QStringList dependents = splitTargetNames(value);
    foreach (const QString& t, targets) {
        DescriptionBlock* descblock = m_makefile->target(t);
        DescriptionBlock::AddCommandsState canAddCommands = separatorLength > 1 ? DescriptionBlock::ACSEnabled : DescriptionBlock::ACSDisabled;
        if (descblock) {
            if (canAddCommands != descblock->m_canAddCommands &&
                descblock->m_canAddCommands != DescriptionBlock::ACSUnknown)
            {
                error("cannot have : and :: dependents for same target");
                return;
            }
            descblock->m_canAddCommands = canAddCommands;
        } else {
            descblock = createTarget(t);
            descblock->m_canAddCommands = canAddCommands;
            canAddCommands = DescriptionBlock::ACSEnabled;
        }
        descblock->m_dependents.append(dependents);
        descblock->m_suffixes = m_suffixes;
        descblock->expandFileNameMacrosForDependents();

        if (!commands.isEmpty()) {
            if (canAddCommands == DescriptionBlock::ACSEnabled || descblock->m_commands.isEmpty())
                descblock->m_commands.append(commands);
            else
                qWarning("Cannot add commands to previously defined target %s.", qPrintable(t));
        }

        //qDebug() << "parseDescriptionBlock" << descblock->m_targetName << descblock->m_dependents;
    }
}

bool Parser::parseCommand(QList<Command>& commands, bool inferenceRule)
{
    // eat empty lines
    while (m_line.trimmed().isEmpty()) {
        readLine();
        if (m_line.isNull())
            return false;
    }

    // check if we have a command line
    if (!startsWithSpaceOrTab(m_line))
        return false;

    parseCommandLine(m_line, commands, inferenceRule);
    return true;
}

void Parser::parseCommandLine(const QString& cmdLine, QList<Command>& commands, bool inferenceRule)
{
    commands.append(Command());
    Command& cmd = commands.last();
    if (m_ignoreExitCodes) cmd.m_maxExitCode = 255;
    cmd.m_silent = m_silentCommands;

    if (!inferenceRule)
        cmd.m_commandLine = m_preprocessor->macroTable()->expandMacros(cmdLine.trimmed());
    else
        cmd.m_commandLine = cmdLine.trimmed();

    bool noCommandModifiersFound = false;
    do {
        if (cmd.m_commandLine.startsWith('-')) {
            cmd.m_commandLine = cmd.m_commandLine.remove(0, 1);
            cmd.m_maxExitCode = 255;
            int idx = cmd.m_commandLine.indexOf(' ');
            if (idx == -1) idx = cmd.m_commandLine.indexOf('\t');
            if (idx > -1) {
                QString numstr = cmd.m_commandLine.left(idx+1);
                bool ok;
                int exitCode = numstr.toInt(&ok);
                if (ok) cmd.m_maxExitCode = (unsigned char)exitCode;
            }
        } else if (cmd.m_commandLine.startsWith('@')) {
            cmd.m_commandLine = cmd.m_commandLine.remove(0, 1);
            cmd.m_silent = true;
        } else if (cmd.m_commandLine.startsWith('!')) {
            cmd.m_commandLine = cmd.m_commandLine.remove(0, 1);
            cmd.m_singleExecution = true;
        } else {
            noCommandModifiersFound = true;
        }
    } while (!noCommandModifiersFound);

    parseInlineFiles(cmd, inferenceRule);
}

void Parser::parseInlineFiles(Command& cmd, bool inferenceRule)
{
    // First, create the InlineFile objects from the command line.
    int fileNamePos = 0;
    while ((fileNamePos = cmd.m_commandLine.indexOf(QLatin1String("<<"), fileNamePos)) >= 0) {
        fileNamePos += 2;
        InlineFile* inlineFile = new InlineFile();
        cmd.m_inlineFiles.append(inlineFile);

        // Determine the inline file name, if given.
        if (cmd.m_commandLine.count() > fileNamePos && !cmd.m_commandLine.at(fileNamePos).isSpace()) {
            if (cmd.m_commandLine.at(fileNamePos) == QLatin1Char('"')) {
                // quoted file name
                int idx = cmd.m_commandLine.indexOf(QLatin1Char('"'), fileNamePos + 1);
                if (idx == -1) {
                    QString msg = QLatin1String("missing quote for inline file name in %0, line %1");
                    qWarning(qPrintable(msg.arg(m_preprocessor->currentFileName()).arg(m_preprocessor->lineNumber())));
                } else {
                    inlineFile->m_filename = cmd.m_commandLine.mid(fileNamePos + 1, idx - fileNamePos - 1);
                    cmd.m_commandLine.remove(fileNamePos, idx - fileNamePos + 1);
                }
            } else {
                // unquoted file name
                int idx = fileNamePos + 1;
                while (idx < cmd.m_commandLine.count() && !cmd.m_commandLine.at(idx).isSpace())
                    ++idx;
                inlineFile->m_filename = cmd.m_commandLine.mid(fileNamePos, idx - fileNamePos);
                cmd.m_commandLine.remove(fileNamePos, idx - fileNamePos);
            }
        }
    }

    // Read the content for each inline file.
    foreach (InlineFile* inlineFile, cmd.m_inlineFiles) {
        readLine();
        while (!m_line.isNull()) {
            if (m_line.startsWith("<<")) {
                QStringList options = m_line.right(m_line.length() - 2).split(m_rexSingleWhiteSpace);
                if (options.contains("KEEP"))
                    inlineFile->m_keep = true;
                if (options.contains("UNICODE"))
                    inlineFile->m_unicode = true;
                break;
            }

            QString contentLine;
            if (inferenceRule)
                contentLine = m_line;
            else
                contentLine = m_preprocessor->macroTable()->expandMacros(m_line);

            contentLine.append(QLatin1String("\n"));
            inlineFile->m_content.append(contentLine);
            readLine();
        }
    }
}

void Parser::parseInferenceRule()
{
    QString fromPath = m_rexInferenceRule.cap(1);
    QString fromExt  = m_rexInferenceRule.cap(2);
    QString toPath   = m_rexInferenceRule.cap(3);
    QString toExt    = m_rexInferenceRule.cap(4);
    bool batchMode   = m_rexInferenceRule.cap(5).length() > 1;

    if (fromPath.length() >= 2)
        fromPath = fromPath.mid(1, fromPath.length() - 2);
    if (toPath.length() >= 2)
        toPath = toPath.mid(1, toPath.length() - 2);
    if (fromPath.isEmpty())
        fromPath = QLatin1String(".");
    if (toPath.isEmpty())
        toPath = QLatin1String(".");

    //qDebug() << fromPath << fromExt
    //         << toPath << toExt << batchMode;

    removeDirSeparatorAtEnd(fromPath);
    removeDirSeparatorAtEnd(toPath);

    InferenceRule rule;
    rule.m_batchMode = batchMode;
    rule.m_fromSearchPath = fromPath;
    rule.m_fromExtension = fromExt;
    rule.m_toSearchPath = toPath;
    rule.m_toExtension = toExt;

    readLine();
    while (parseCommand(rule.m_commands, true))
        readLine();

    m_makefile->addInferenceRule(rule);
}

void Parser::parseDotDirective()
{
    QString directive = m_rexDotDirective.cap(1);
    QString value = m_rexDotDirective.cap(2);
    //qDebug() << m_rexDotDirective.cap(1) << m_rexDotDirective.cap(2);

    if (directive == "SUFFIXES") {
        QStringList splitvalues = value.simplified().split(m_rexSingleWhiteSpace, QString::SkipEmptyParts);
        //qDebug() << "splitvalues" << splitvalues;
        if (splitvalues.isEmpty())
            m_suffixes.clear();
        else
            m_suffixes.append(splitvalues);
        //qDebug() << ".SUFFIXES" << m_suffixes;
    } else if (directive == "IGNORE") {
        m_ignoreExitCodes = true;
    } else if (directive == "PRECIOUS") {
        const QStringList& splitvalues = value.split(m_rexSingleWhiteSpace);
        foreach (QString str, splitvalues)
            if (!str.isEmpty())
                m_makefile->addPreciousTarget(str);
    } else if (directive == "SILENT") {
        m_silentCommands = true;
    }

    readLine();
}

void Parser::checkForCycles(DescriptionBlock* target)
{
    if (!target)
        return;

    if (target->m_bVisitedByCycleCheck) {
        QString msg = QLatin1String("cycle in targets detected: %1");
        throw Exception(msg.arg(target->targetName()));
    }

    target->m_bVisitedByCycleCheck = true;
    foreach (const QString& depname, target->m_dependents)
        checkForCycles(m_makefile->target(depname));
    target->m_bVisitedByCycleCheck = false;
}

QList<InferenceRule*> Parser::findRulesByTargetExtension(const QString& targetFileName)
{
    QList<InferenceRule*> result;
    foreach (const InferenceRule& rule, m_makefile->inferenceRules())
        if (targetFileName.endsWith(rule.m_toExtension))
            result.append(const_cast<InferenceRule*>(&rule));
    return result;
}

void Parser::filterRulesByTargetName(QList<InferenceRule*>& rules, const QString& targetFileName)
{
    QList<InferenceRule*>::iterator it = rules.begin();
    while (it != rules.end()) {
        const InferenceRule* rule = *it;
        if (!targetFileName.endsWith(rule->m_toExtension)) {
            it = rules.erase(it);
            continue;
        }

        QString fileName = fileNameFromFilePath(targetFileName);
        QString directory = targetFileName.left(targetFileName.length() - fileName.length());
        removeDirSeparatorAtEnd(directory);
        if (directory.isEmpty()) directory = ".";
        if (directory != rule->m_toSearchPath) {
            it = rules.erase(it);
            continue;
        }

        ++it;
    }
}

void Parser::preselectInferenceRules()
{
    foreach (const QString targetName, m_activeTargets) {
        DescriptionBlock* target = m_makefile->target(targetName);
        if (target->m_commands.isEmpty())
            preselectInferenceRules(target->targetFilePath(), target->m_inferenceRules, target->m_suffixes);
        preselectInferenceRulesRecursive(target);
    }
}

void Parser::preselectInferenceRules(const QString& targetFileName,
                                     QList<InferenceRule*>& rules,
                                     const QStringList& suffixes)
{
    bool inferenceRulesApplicable = false;
    foreach (const QString& suffix, suffixes) {
        if (targetFileName.endsWith(suffix)) {
            inferenceRulesApplicable = true;
            break;
        }
    }

    if (!inferenceRulesApplicable)
        return;

    rules = findRulesByTargetExtension(targetFileName);
    filterRulesByTargetName(rules, targetFileName);
}

void Parser::preselectInferenceRulesRecursive(DescriptionBlock* target)
{
    foreach (const QString& dependentName, target->m_dependents) {
        DescriptionBlock* dependent = m_makefile->target(dependentName);
        QStringList suffixes;
        QString dependentFileName = dependentName;
        if (dependent) {
            if (!dependent->m_commands.isEmpty()) {
                preselectInferenceRulesRecursive(dependent);
                continue;
            }
            suffixes = dependent->m_suffixes;
            dependentFileName = dependent->targetFilePath();
        } else {
            suffixes = target->m_suffixes;
            if (dependentFileName.startsWith(QLatin1Char('"')) && dependentFileName.endsWith(QLatin1Char('"'))) {
                dependentFileName.remove(0, 1);
                dependentFileName.chop(1);
            }
        }

        QList<InferenceRule*> selectedRules;
        preselectInferenceRules(dependentFileName, selectedRules, suffixes);

        if (!dependent) {
            if (selectedRules.isEmpty())
                continue;
            dependent = createTarget(dependentName);
        }
        dependent->m_inferenceRules = selectedRules;
        preselectInferenceRulesRecursive(dependent);
    }
}

void Parser::error(const QString& msg)
{
    throw FileException(msg, m_preprocessor->currentFileName(), m_preprocessor->lineNumber());
}

} // namespace NMakeFile
