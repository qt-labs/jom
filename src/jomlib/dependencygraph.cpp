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
#include "dependencygraph.h"
#include "makefile.h"
#include "options.h"
#include "fileinfo.h"

#include <QFile>
#include <QDebug>

namespace NMakeFile {

DependencyGraph::DependencyGraph()
:   m_root(0),
    m_bDirtyLeaves(true)
{
}

DependencyGraph::~DependencyGraph()
{
    clear();
}

DependencyGraph::Node* DependencyGraph::createNode(DescriptionBlock* target, Node* parent)
{
    Node* node = new Node;
    node->target = target;
    node->state = Node::UnknownState;
    if (parent) {
        addEdge(parent, node);
    }

    m_nodeContainer[target] = node;
    return node;
}

void DependencyGraph::deleteNode(Node* node)
{
    m_nodeContainer.remove(node->target);
    if (node == m_root) m_root = 0;
    delete node;
}

void DependencyGraph::build(DescriptionBlock* target)
{
    m_bDirtyLeaves = true;
    m_root = createNode(target, 0);
    internalBuild(m_root);
    //dump();
    //qDebug() << "\n\n-------------------------------------------------\n";

    //do {
    //    target = findAvailableTarget();
    //    if (target) qDebug() << "XXX" << target->m_targetName << target->m_bFileExists;
    //} while(target);

    //qDebug() << "\nFINISHED";
}

bool DependencyGraph::isTargetUpToDate(DescriptionBlock* target)
{
    bool targetIsExistingFile = target->m_bFileExists;
    if (!targetIsExistingFile) {
        FileInfo fi(target->targetName());
        targetIsExistingFile = fi.exists();  // could've been created in the mean time
        if (targetIsExistingFile)
            target->m_timeStamp = fi.lastModified();
    }

    if (!targetIsExistingFile || !target->m_timeStamp.isValid())
        return false;

    // find latest timestamp of all dependents
    QDateTime ts(QDate(1900, 1, 1));
    foreach (const QString& dependentName, target->m_dependents) {
        FileInfo fi(dependentName);
        if (fi.exists()) {
            QDateTime ts2 = fi.lastModified();
            if (ts < ts2)
                ts = ts2;
        } else {
            ts = QDateTime::currentDateTime();
            break;
        }
    }

    bool isUpToDate = (target->m_timeStamp >= ts);
    if (isUpToDate && !target->m_inferenceRules.isEmpty()) {
        // The target is up-to-date but it still has unapplied inference rules.
        // That means there could be dependents we didn't take into account yet.

        QStringList savedDependents = target->m_dependents;
        QList<InferenceRule*> savedRules = target->m_inferenceRules;
        target->m_inferenceRules.clear();

        bool inferredDependentAdded = false;
        foreach (InferenceRule *rule, savedRules) {
            QString inferredDependent = rule->inferredDependent(target->targetFilePath());
            if (!target->m_dependents.contains(inferredDependent) && FileInfo(inferredDependent).exists()) {
                inferredDependentAdded = true;
                target->m_dependents.append(inferredDependent);
            }
        }

        if (inferredDependentAdded)
            isUpToDate = isTargetUpToDate(target);

        target->m_dependents = savedDependents;
        target->m_inferenceRules = savedRules;
    }

    return isUpToDate;
}

void DependencyGraph::internalBuild(Node* node)
{
    if (node->target->m_dependents.isEmpty()) {
        m_leaves.insert(node);
        return;
    }

    foreach (const QString& dependentName, node->target->m_dependents) {
        Makefile* const makefile = node->target->makefile();
        DescriptionBlock* dependent = makefile->target(dependentName);
        if (!dependent) {
            if (!FileInfo(dependentName).exists()) {
                QString msg = "Error: dependent '" + dependentName + "' does not exist.";
                qFatal(qPrintable(msg));
            }
            continue;
        }

        Node* child;
        if (m_nodeContainer.contains(dependent)) {
            child = m_nodeContainer.value(dependent);
            addEdge(node, child);
        } else
            child = createNode(dependent, node);

        internalBuild(child);
    }
}

void DependencyGraph::dump()
{
    QString indent;
    internalDump(m_root, indent);
}

void DependencyGraph::internalDump(Node* node, QString& indent)
{
    printf(qPrintable(indent + node->target->targetName() + "\n"));
    indent.append(' ');
    foreach (Node* child, node->children) {
        internalDump(child, indent);
    }
    indent.resize(indent.length() - 1);
}

void DependencyGraph::dotDump()
{
    printf("digraph G {\n");
    QString parent;
    internalDotDump(m_root, parent);
    printf("}\n");
}

void DependencyGraph::internalDotDump(Node* node, const QString& parent)
{
    if (!parent.isNull()) {
        QString line = "  \"" + parent + "\" -> \"" + node->target->targetName() + "\";\n";
        printf(line.toAscii());
    }
    foreach (Node* child, node->children) {
        internalDotDump(child, node->target->targetName());
    }
}

void DependencyGraph::clear()
{
    m_root = 0;
    qDeleteAll(m_nodeContainer);
    m_nodeContainer.clear();
    m_leaves.clear();
}

void DependencyGraph::addEdge(Node* parent, Node* child)
{
    if (!parent->children.contains(child))
        parent->children.append(child);
    if (!child->parents.contains(parent))
        child->parents.append(parent);
}

bool DependencyGraph::isEmpty() const
{
    return m_nodeContainer.isEmpty();
}

void DependencyGraph::removeLeaf(DescriptionBlock* target)
{
    Node* nodeToRemove = m_nodeContainer.value(target);
    if (nodeToRemove)
        removeLeaf(nodeToRemove);
}

void DependencyGraph::removeLeaf(Node* node)
{
    Q_ASSERT(node);
    Q_ASSERT(node->children.isEmpty());

    m_leaves.remove(node);

    QList<Node*>::iterator it;
    foreach (Node* parent, node->parents) {
        it = qFind(parent->children.begin(), parent->children.end(), node);
        parent->children.erase(it);
        if (parent->children.isEmpty()) {
            m_bDirtyLeaves = true;
            m_leaves.insert(parent);
        }
    }
    deleteNode(node);
}

DescriptionBlock* DependencyGraph::findAvailableTarget()
{
    if (m_leaves.isEmpty())
        return 0;

    // remove all leaves that are not up-to-date
    QList<Node*> upToDateNodes;
    while (m_bDirtyLeaves) {
        m_bDirtyLeaves = false;
        foreach (Node *leaf, m_leaves)
            if (leaf->state != Node::ExecutingState && isTargetUpToDate(leaf->target))
                upToDateNodes.append(leaf);
        foreach (Node *leaf, upToDateNodes) {
            displayNodeBuildInfo(leaf, true);
            removeLeaf(leaf);
        }
        upToDateNodes.clear();
    }

    // apply inference rules separated by makefiles
    QSet<Makefile*> makefileSet;
    QMultiHash<Makefile*, DescriptionBlock*> multiHash;
    foreach (Node *leaf, m_leaves) {
        makefileSet.insert(leaf->target->makefile());
        multiHash.insert(leaf->target->makefile(), leaf->target);
    }
    foreach (Makefile *mf, makefileSet)
        mf->applyInferenceRules(multiHash.values(mf));

    // return the first leaf that is not currently executed
    foreach (Node *leaf, m_leaves) {
        if (leaf->state != Node::ExecutingState) {
            leaf->state = Node::ExecutingState;
            displayNodeBuildInfo(leaf, false);
            return leaf->target;
        }
    }

    return 0;
}

void DependencyGraph::displayNodeBuildInfo(Node* node, bool isUpToDate)
{
    if (node->target->makefile()->options()->displayBuildInfo) {
        QString msg;
        if (isUpToDate)
            msg = " ";
        else
            msg = "*";
        msg += node->target->m_timeStamp.toString("yy/MM/dd hh:mm:ss") + " " +
               node->target->targetName();
        msg += "\n";
        printf(qPrintable(msg));
    }
}

} // namespace NMakeFile
