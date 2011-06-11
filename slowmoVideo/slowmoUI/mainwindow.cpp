/*
slowmoUI is a user interface for slowmoVideo.
Copyright (C) 2011  Simon A. Eugster (Granjow)  <simon.eu@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "newprojectdialog.h"
#include "progressDialogExtractFrames.h"
#include "renderDialog.h"

#include "../project/flow_sV.h"
#include "../project/renderTask_sV.h"
#include "../project/xmlProjectRW_sV.h"

#include <QtCore>
#include <QObject>
#include <QDockWidget>
#include <QDebug>

#include <QDir>
#include <QFileDialog>

#include <QShortcut>
#include <QSignalMapper>
#include <QTime>
#include <QFuture>

#include <QPainter>

#include <functional>

QStringList MainWindow::m_commands;

void MainWindow::fillCommandList()
{
    m_commands.clear();
    m_commands << "h:\tHelp";
    m_commands << "q-q:\tQuit";
    m_commands << "n:\tNew";
    m_commands << "o:\tOpen";
    m_commands << "s:\tSave";
    m_commands << "x:\tAbort current action";
    m_commands << "x-s:\tAbort selection";
    m_commands << "d-n:\tDelete selected nodes";
    m_commands << "t-s:\tSelect tool";
    m_commands << "t-m:\tMove tool";
    m_commands << "t-t:\tTag";
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_project = new Project_sV(QDir::tempPath() + "/noexist", QDir::tempPath());
    qDebug() << "Project location: " << &m_project;

    m_wCanvas = new Canvas(m_project, this);
    setCentralWidget(m_wCanvas);

    m_wInputMonitor = new InputMonitor(this);
    m_wInputMonitorDock = new QDockWidget("Input monitor", this);
    m_wInputMonitorDock->setWidget(m_wInputMonitor);
    addDockWidget(Qt::TopDockWidgetArea, m_wInputMonitorDock);


    // Set up shortcut bindings
    m_keyList.insert(MainWindow::Help, "h");
    m_keyList.insert(MainWindow::Quit, "q");
    m_keyList.insert(MainWindow::Quit_Quit, "q");
    m_keyList.insert(MainWindow::New, "n");
    m_keyList.insert(MainWindow::Open, "o");
    m_keyList.insert(MainWindow::Save, "s");
    m_keyList.insert(MainWindow::Abort, "x");
    m_keyList.insert(MainWindow::Abort_Selection, "s");
    m_keyList.insert(MainWindow::Delete, "d");
    m_keyList.insert(MainWindow::Delete_Node, "n");
    m_keyList.insert(MainWindow::Tool, "t");
    m_keyList.insert(MainWindow::Tool_Select, "s");
    m_keyList.insert(MainWindow::Tool_Move, "m");
    m_keyList.insert(MainWindow::Tool_Tag, "t");
    fillCommandList();

    QList<QString> uniqueKeys;
    QList<QString> keys = m_keyList.values();
    for (int i = 0; i < keys.size(); i++) {
        if (!uniqueKeys.contains(keys[i])) {
            uniqueKeys.append(keys[i]);
            qDebug() << "Added to key list: " << keys[i];
        }
    }


    bool b = true;

    m_signalMapper = new QSignalMapper(this);
    for (int i = 0; i < uniqueKeys.length(); i++) {

        // Create a new shortcut for each unique key
        QShortcut *shortcut = new QShortcut(QKeySequence(uniqueKeys[i]), this);

        m_shortcutList.append(shortcut);
        m_signalMapper->setMapping(shortcut, uniqueKeys[i]);

        // Connect shortcut to the signal mapper
        b &= connect(shortcut, SIGNAL(activated()), m_signalMapper, SLOT(map()));
    }
    b &= connect(m_signalMapper, SIGNAL(mapped(QString)), this, SLOT(shortcutUsed(QString)));

    b &= connect(this, SIGNAL(deleteNodes()), m_wCanvas, SLOT(slotDeleteNodes()));
    b &= connect(this, SIGNAL(setMode(Canvas::ToolMode)), m_wCanvas, SLOT(slotSetToolMode(Canvas::ToolMode)));
    b &= connect(this, SIGNAL(abort(Canvas::Abort)), m_wCanvas, SLOT(slotAbort(Canvas::Abort)));
    b &= connect(this, SIGNAL(addTag()), m_wCanvas, SLOT(slotAddTag()));

    b &= connect(m_wCanvas, SIGNAL(signalMouseInputTimeChanged(qreal)),
                 this, SLOT(slotForwardInputPosition(qreal)));

    b &= connect(ui->actionRender, SIGNAL(triggered()), this, SLOT(showRenderDialog()));


    Q_ASSERT(b);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete m_wInputMonitor;
    delete m_wInputMonitorDock;

    if (m_project != NULL) {
        delete m_project;
    }

    delete m_signalMapper;
    for (int i = 0; i < m_shortcutList.length(); i++) {
        delete m_shortcutList[i];
    }
}

void MainWindow::loadProject(Project_sV *project)
{
    if (m_project != NULL) {
        delete m_project;
    }
    if (project != NULL) {
        m_project = project;
        m_wCanvas->load(m_project);
    }

    ProgressDialogExtractFrames progress;
    bool b = true;
    b &= connect(
                m_project, SIGNAL(signalFramesExtracted(FrameSize)),
                &progress, SLOT(slotExtractionFinished(FrameSize))
            );
    b &= connect(
                m_project, SIGNAL(signalProgressUpdated(FrameSize,int)),
                &progress, SLOT(slotProgressUpdated(FrameSize,int))
            );
    Q_ASSERT(b);
    m_project->extractFrames();
    progress.exec();
}


void MainWindow::displayHelp(QPainter &davinci)
{
    QRect content(10, 10, 400, 200);
    QRect text(content.topLeft() + QPoint(10, 10), content.size() - QSize(20,20));
    davinci.fillRect(content, QColor(0,0,40, 200));
    QString helpText;
    for (int i = 0; i < m_commands.size(); i++) {
        helpText.append(m_commands.at(i) + "\n");
    }
    davinci.drawText(text, helpText);
}

void MainWindow::newProject()
{
    NewProjectDialog npd(this);
    if (npd.exec() == QDialog::Accepted) {
        Project_sV *newProject = new Project_sV(npd.m_inputFile, npd.m_projectDir);
        if (newProject->validDirectories()) {

            loadProject(newProject);


        } else {
            qDebug() << "Project directories not writable.";
            delete newProject;
        }
    }
}


void MainWindow::shortcutUsed(QString which)
{
    TimedShortcut ts;
    ts.shortcut = which;
    ts.start = QTime::currentTime();

    qDebug() << which << " pressed. Last shortcut: " << m_lastShortcut.start.elapsed();

//    QString at = QString(" @ %1.%2::%3")
//            .arg(ts.start.minute())
//            .arg(ts.start.second())
//            .arg(ts.start.msec());

    bool handled = false;

    // Use a timeout. Otherwise pressing a key may lead to unpredictable results
    // since it may depend on the key you pressed several minutes ago.
    if (m_lastShortcut.start.elapsed() < 600) {

        // Handle combined shortcuts here.

        if (m_lastShortcut.shortcut == m_keyList[MainWindow::Quit]) {
            if (which == m_keyList[MainWindow::Quit_Quit]) {
                qApp->quit();
            }
        }
        else if (m_lastShortcut.shortcut == m_keyList[MainWindow::Abort]) {
            if (which == m_keyList[MainWindow::Abort_Selection]) {
                emit abort(Canvas::Abort_Selection);
                handled = true;
            }
        }
        else if (m_lastShortcut.shortcut == m_keyList[MainWindow::Delete]) {
            if (which == m_keyList[MainWindow::Delete_Node]) {
                emit deleteNodes();
                handled = true;
            }
        }
        else if (m_lastShortcut.shortcut == m_keyList[MainWindow::Tool]) {
            if (which == m_keyList[MainWindow::Tool_Select]) {
                emit setMode(Canvas::ToolMode_Select);
                handled = true;
            } else if (which == m_keyList[MainWindow::Tool_Move]) {
                emit setMode(Canvas::ToolMode_Move);
                handled = true;
            } else if (which == m_keyList[MainWindow::Tool_Tag]) {
                emit addTag();
                handled = true;
            }
        }
    } else {
        if (which == m_keyList[MainWindow::Abort]) {
            emit abort(Canvas::Abort_General);
            handled = true;
        } else {
            qDebug() << "(Shortcut timed out.)";
        }
    }
    if (!handled) {
        if (which == m_keyList[MainWindow::Help]) {
            m_wCanvas->toggleHelp();
            handled = true;
        } else if (which == m_keyList[MainWindow::New]) {
            newProject();
            handled = true;
        } else if (which == m_keyList[MainWindow::Open]) {
            QFileDialog dialog(this, "Load project");
            dialog.setAcceptMode(QFileDialog::AcceptOpen);
            dialog.setDefaultSuffix("sVproj");
            dialog.setNameFilter("slowmoVideo projects (*.sVproj)");
            dialog.setFileMode(QFileDialog::ExistingFile);
            if (dialog.exec() == QDialog::Accepted) {
                XmlProjectRW_sV reader;
                Project_sV *project = reader.loadProject(dialog.selectedFiles().at(0));
                loadProject(project);
            }
            handled = true;
        } else if (which == m_keyList[MainWindow::Save]) {
            QFileDialog dialog(this, "Save project");
            dialog.setAcceptMode(QFileDialog::AcceptSave);
            dialog.setDefaultSuffix("sVproj");
            dialog.setNameFilter("slowmoVideo projects (*.sVproj)");
            dialog.setFileMode(QFileDialog::AnyFile);
            if (dialog.exec() == QDialog::Accepted) {
                XmlProjectRW_sV writer;
                writer.saveProject(m_project, dialog.selectedFiles().at(0));
            }
            handled = true;
        }
    }

    m_lastShortcut = ts;
}

void MainWindow::slotForwardInputPosition(qreal frame)
{
    if (0 <= frame && frame < m_project->videoInfo().framesCount) {
        qDebug() << "Signal: Load image at " << frame;
        m_wInputMonitor->slotLoadImage(m_project->frameFileStr(qFloor(frame), FrameSize_Small));
    }
}

void MainWindow::showRenderDialog()
{
    RenderDialog renderDialog(m_project);

    bool b = true;
    b &= connect(&renderDialog, SIGNAL(signalChangeFps(float)), m_project, SLOT(slotSetFps(float)));
    b &= connect(&renderDialog, SIGNAL(signalChangeRenderFrameSize(FrameSize)), m_project, SLOT(slotSetRenderFrameSize(FrameSize)));
//    b &= connect(&renderDialog, SIGNAL(signalChangeRenderFrameSize(FrameSize)), m_project->renderTask(), SLOT(slotUpdateRenderFrameSize(FrameSize)));
    b &= connect(&renderDialog, SIGNAL(signalAbortRendering()), m_project->renderTask(), SLOT(slotAbortRendering()));
    b &= connect(&renderDialog, SIGNAL(signalContinueRendering()), m_project->renderTask(), SLOT(slotContinueRendering()));
    b &= connect(m_project->renderTask(), SIGNAL(signalFrameRendered(qreal,int)), &renderDialog, SLOT(slotFrameRendered(qreal,int)));
    b &= connect(m_project->renderTask(), SIGNAL(signalRenderingAborted()), &renderDialog, SLOT(slotRenderingAborted()));
    b &= connect(m_project->renderTask(), SIGNAL(signalRenderingFinished()), &renderDialog, SLOT(slotRenderingFinished()));
    Q_ASSERT(b);

    renderDialog.exec();
}
