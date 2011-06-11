/*
slowmoUI is a user interface for slowmoVideo.
Copyright (C) 2011  Simon A. Eugster (Granjow)  <simon.eu@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#ifndef CANVAS_H
#define CANVAS_H

#include "../project/node_sV.h"
#include "../project/nodelist_sV.h"
#include "../project/project_sV.h"
#include "../project/tag_sV.h"

#include <QWidget>
#include <QList>



#define NODE_RADIUS 4
#define NODE_SEL_DIST 2
#define MOVE_THRESHOLD 3



class QColor;
class QPoint;

namespace Ui {
    class Canvas;
}

class Project_sV;
class Canvas : public QWidget
{
    Q_OBJECT

public:
    explicit Canvas(const Project_sV *project, QWidget *parent = 0);
    ~Canvas();

    enum ToolMode { ToolMode_Select, ToolMode_Move };
    enum Abort { Abort_General, Abort_Selection };

    static QColor lineCol;
    static QColor nodeCol;
    static QColor gridCol;
    static QColor selectedCol;
    static QColor labelCol;
    static QColor backgroundCol;

    void load(const Project_sV *project);

    void toggleHelp();

public slots:
    void slotAbort(Canvas::Abort abort);
    void slotAddTag();
    void slotDeleteNodes();
    void slotSetToolMode(Canvas::ToolMode mode);

signals:
    void signalMouseInputTimeChanged(qreal frame);

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void leaveEvent(QEvent *);

private:
    Ui::Canvas *ui;
    const Project_sV *m_project;
    bool m_mouseWithinWidget;
    int m_distLeft;
    int m_distBottom;
    int m_distRight;
    int m_distTop;
    Node_sV m_t0;
    Node_sV m_tmax;
    int m_secResX;
    int m_secResY;

    bool m_showHelp;

    NodeList_sV *m_nodes;
    QList<Tag_sV> *m_tags;

    ToolMode m_mode;
    struct {
        bool nodesMoved;
        bool selectAttempted;
        bool moveAborted;
        QPoint prevMousePos;
        QPoint initialMousePos;
        Qt::KeyboardModifiers initialModifiers;

        void reset() {
            moveAborted = false;
            nodesMoved = false;
            selectAttempted = false;
            travelledDistance = 0;
        }
        void travel(int length) { travelledDistance += length; }
        bool countsAsMove() { return travelledDistance >= MOVE_THRESHOLD; }

    private:
        int travelledDistance;
    } m_states;

    const Node_sV convertCanvasToTime(const QPoint &p) const;
    const QPoint convertTimeToCanvas(const Node_sV &p) const;

    bool insideCanvas(const QPoint& pos);

    bool selectAt(const QPoint& pos, bool addToSelection = false);

    void drawModes(QPainter &davinci, int top, int right);
};

QDebug operator<<(QDebug qd, const Canvas::ToolMode &mode);
QDebug operator<<(QDebug qd, const Canvas::Abort &abort);

#endif // CANVAS_H