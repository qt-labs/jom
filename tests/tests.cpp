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
#include <QTest>
#include <QDir>
#include <QDebug>

#include <preprocessor.h>
#include <parser.h>
#include <exception.h>

using namespace NMakeFile;

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

    // parser tests
    void inferenceRules_data();
    void inferenceRules();
    void cycleInTargets();
    void comments();

private:
    QString m_oldCurrentPath;
};

void ParserTest::initTestCase()
{
    m_oldCurrentPath = QDir::currentPath();
    if (QFile::exists("../makefiles"))
        QDir::setCurrent("../makefiles");
    else
        QDir::setCurrent("makefiles");
}

void ParserTest::cleanupTestCase()
{
    QDir::setCurrent(m_oldCurrentPath);
}

void ParserTest::includeFiles()
{
    MacroTable macroTable;
    Preprocessor pp;
    pp.setMacroTable(&macroTable);
    QVERIFY( pp.openFile(QLatin1String("include_test.mk")) );
    while (!pp.readLine().isNull());
    QVERIFY(macroTable.isMacroDefined("INCLUDE"));
    QCOMPARE(macroTable.macroValue("INCLUDE1"), QLatin1String("TRUE"));
    QCOMPARE(macroTable.macroValue("INCLUDE2"), QLatin1String("TRUE"));
    QCOMPARE(macroTable.macroValue("INCLUDE3"), QLatin1String("TRUE"));
    QCOMPARE(macroTable.macroValue("INCLUDE4"), QLatin1String("TRUE"));
    QCOMPARE(macroTable.macroValue("INCLUDE5"), QLatin1String("TRUE"));
}

void ParserTest::includeCycle()
{
    MacroTable macroTable;
    Preprocessor pp;
    pp.setMacroTable(&macroTable);
    QVERIFY( pp.openFile(QLatin1String("circular_include.mk")) );
    bool bExceptionCaught = false;
    try {
        while (!pp.readLine().isNull());
    } catch (Exception e) {
        qDebug() << e.message();
        bExceptionCaught = true;
    }
    QVERIFY(bExceptionCaught);
}

void ParserTest::macros()
{
    MacroTable macroTable;
    Preprocessor pp;
    pp.setMacroTable(&macroTable);
    QVERIFY( pp.openFile(QLatin1String("macrotest.mk")) );
    bool bExceptionCaught = false;
    try {
        while (!pp.readLine().isNull());
    } catch (Exception e) {
        qDebug() << e.message();
        bExceptionCaught = true;
    }
    QVERIFY(!bExceptionCaught);
    QCOMPARE(macroTable.macroValue("VERY_LONG_Macro_Name_With_mucho_mucho_characters_and_some_number_too_1234458789765421200218427824996512548989654486630110059699471421"), QLatin1String("AHA"));
    QCOMPARE(macroTable.macroValue("SEIN"), QLatin1String("ist"));
    QCOMPARE(macroTable.macroValue("vielFressen_istSonnenschein_"), QLatin1String("Hast du Weinbrand in der Blutbahn..."));
    QVERIFY(macroTable.isMacroDefined("NoContent"));
    QCOMPARE(macroTable.macroValue("NoContent"), QLatin1String(""));
    QCOMPARE(macroTable.macroValue("Literal1"), QLatin1String("# who does that anyway? #"));
    QCOMPARE(macroTable.macroValue("Literal2"), QLatin1String("thi$ i$ pricele$$"));
    QCOMPARE(macroTable.macroValue("Literal3"), QLatin1String("schnupsi\nwupsi\ndupsi"));
    QCOMPARE(macroTable.macroValue("Literal4"), QLatin1String("backslash at the end\\"));
    QCOMPARE(macroTable.macroValue("Literal5"), QLatin1String("backslash at the end\\"));
    QCOMPARE(macroTable.macroValue("Literal6"), QLatin1String("backslash at the end\\"));
    QVERIFY(!macroTable.isMacroDefined("ThisIsNotDefined"));
}

// inferenceRules test mode
static const char IRTM_Init = 0;
static const char IRTM_Cleanup = 1;
static const char IRTM_ParseTimeRule = 2;
static const char IRTM_DeferredRule = 3;

void ParserTest::inferenceRules_data()
{
    QTest::addColumn<char>("mode");
    QTest::addColumn<QString>("targetName");
    QTest::addColumn<QString>("expectedCommandLine");
    QTest::addColumn<QString>("fileToCreate");

    QTest::newRow("init") << IRTM_Init << "" << "" << "";
    QTest::newRow("1") << IRTM_ParseTimeRule << "foo1.obj" << "echo {subdir}.cpp.obj (subdir\\foo1.cpp)" << "";
    QTest::newRow("2") << IRTM_ParseTimeRule << "foo2.obj" << "echo {subdir}.cpp.obj (subdir\\foo2.cpp)" << "";
    QTest::newRow("3") << IRTM_ParseTimeRule << "foo3.obj" << "echo .cpp.obj (foo3.cpp)" << "";
    QTest::newRow("4") << IRTM_ParseTimeRule << "foo4.obj" << "echo {subdir}.cpp.obj (subdir\\foo4.cpp)" << "";
    QTest::newRow("5") << IRTM_DeferredRule << "foo5.obj" << "echo {subdir}.cpp.obj (subdir\\foo5.cpp)" << "subdir\\foo5.cpp";
    QTest::newRow("6") << IRTM_DeferredRule << "foo6.obj" << "echo .cpp.obj (foo6.cpp)" << "foo6.cpp";
    QTest::newRow("cleanup") << IRTM_Cleanup << "" << "" << "";

    QStringList filesToCreate;
    filesToCreate << "subdir\\foo5.cpp" << "foo6.cpp";
    foreach (const QString& fileName, filesToCreate)
        if (QFile::exists(fileName))
            system(qPrintable(QLatin1String("del ") + fileName));
}

void ParserTest::inferenceRules()
{
    static MacroTable* macroTable = 0;
    static Preprocessor* pp = 0;
    static Parser* parser = 0;
    static Makefile* mkfile = 0;

    QFETCH(char, mode);
    QFETCH(QString, targetName);
    QFETCH(QString, expectedCommandLine);
    QFETCH(QString, fileToCreate);

    switch (mode) {
        case IRTM_Init: // init
            macroTable = new MacroTable;
            pp = new Preprocessor;
            parser = new Parser;
            QVERIFY(macroTable);
            QVERIFY(pp);
            QVERIFY(parser);
            pp->setMacroTable(macroTable);
            QVERIFY( pp->openFile(QLatin1String("infrules.mk")) );
            mkfile = parser->apply(pp);
            QVERIFY(mkfile);
            return;
        case IRTM_Cleanup: // cleanup
            delete parser;
            delete pp;
            delete macroTable;
            return;
    }

    DescriptionBlock* target = mkfile->target(targetName);
    mkfile->applyInferenceRules(target);
    QVERIFY(target);
    if (mode == IRTM_DeferredRule) {
        QVERIFY(target->m_commands.count() == 0);
        system(qPrintable(QLatin1String("echo.>") + fileToCreate));
        QVERIFY(QFile::exists(fileToCreate));
        mkfile->applyInferenceRules(target);
        system(qPrintable(QLatin1String("del ") + fileToCreate));
        QVERIFY(!QFile::exists(fileToCreate));
    }
    QVERIFY(target->m_commands.count() == 1);
    QCOMPARE(target->m_commands.first().m_commandLine, expectedCommandLine);
}

void ParserTest::cycleInTargets()
{
    MacroTable macroTable;
    Preprocessor pp;
    Parser parser;
    pp.setMacroTable(&macroTable);
    QVERIFY( pp.openFile(QLatin1String("cycle_in_targets.mk")) );

    bool exceptionThrown = false;
    try {
        parser.apply(&pp);
    } catch (...) {
        exceptionThrown = true;
    }
    QVERIFY(exceptionThrown);
}

void ParserTest::comments()
{
    MacroTable macroTable;
    Preprocessor pp;
    Parser parser;
    pp.setMacroTable(&macroTable);
    QVERIFY( pp.openFile(QLatin1String("comments.mk")) );

    Makefile* mkfile = 0;
    bool exceptionThrown = false;
    try {
        mkfile = parser.apply(&pp);
    } catch (...) {
        exceptionThrown = true;
    }
    QVERIFY(!exceptionThrown);
    QCOMPARE(macroTable.macroValue("COMPILER"), QLatin1String("Ada95"));
    QCOMPARE(macroTable.macroValue("DEF"), QLatin1String("#define"));

    QVERIFY(mkfile);
    DescriptionBlock* target = mkfile->target("first");
    QVERIFY(target);
    QCOMPARE(target->m_dependents.count(), 2);
    QCOMPARE(target->m_commands.count(), 2);
    
    Command cmd1 = target->m_commands.at(0);
    Command cmd2 = target->m_commands.at(1);
    QCOMPARE(cmd1.m_commandLine, QLatin1String("echo I'm Winneone"));
    QCOMPARE(cmd2.m_commandLine, QLatin1String("echo I'm Winnetou"));
}

QTEST_MAIN(ParserTest)
#include "tests.moc"
