/* ============================================================
* Date        : 2008-08-26
* Description : Preview image viewer.
*
* Copyright (C) 2008 by Kare Sars <kare dot sars at iki dot fi>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
*  by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License.
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* ============================================================ */

#include "ImageViewer.h"
#include "ImageViewer.moc"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QAction>
#include <KIcon>
#include <KLocale>

#include <KDebug>

struct ImageViewer::Private
{
    QGraphicsScene      *scene;
    QGraphicsPixmapItem *pixmapItem;
    QImage              *img;

    QAction *zoomInAction;
    QAction *zoomOutAction;
    QAction *zoom100Action;
    QAction *zoom2FitAction;
};

ImageViewer::ImageViewer(QWidget *parent) : QGraphicsView(parent), d(new Private)
{
    //setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setMouseTracking(true);
    
    // Init the scene
    d->scene = new QGraphicsScene;
    setScene(d->scene);
    d->pixmapItem = new QGraphicsPixmapItem;

    d->scene->addItem(d->pixmapItem);
    d->scene->setBackgroundBrush(Qt::gray);

    // create context menu
    d->zoomInAction = new QAction(KIcon("zoom-in"), i18n("Zoom In"), this);
    connect(d->zoomInAction, SIGNAL(triggered()), this, SLOT(zoomIn()));
    
    d->zoomOutAction = new QAction(KIcon("zoom-out"), i18n("Zoom Out"), this);
    connect(d->zoomOutAction, SIGNAL(triggered()), this, SLOT(zoomOut()));
    
    d->zoom100Action = new QAction(KIcon("zoom-fit-best"), i18n("Zoom to Actual size"), this);
    connect(d->zoom100Action, SIGNAL(triggered()), this, SLOT(zoomActualSize()));
    
    d->zoom2FitAction = new QAction(KIcon("document-preview"), i18n("Zoom to Fit"), this);
    connect(d->zoom2FitAction, SIGNAL(triggered()), this, SLOT(zoom2Fit()));
    
    addAction(d->zoomInAction);
    addAction(d->zoomOutAction);
    addAction(d->zoom100Action);
    addAction(d->zoom2FitAction);
    setContextMenuPolicy(Qt::ActionsContextMenu);
}

// ------------------------------------------------------------------------
ImageViewer::~ImageViewer()
{
    delete d;
}

// ------------------------------------------------------------------------
void ImageViewer::setQImage(QImage *img)
{
    if (img == 0) return;

    d->pixmapItem->setPixmap(QPixmap::fromImage(*img));
    d->pixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    d->scene->setSceneRect(0, 0, img->width(), img->height());
    d->pixmapItem->setZValue(0);
    zoom2Fit();
}

// ------------------------------------------------------------------------
void ImageViewer::zoomIn()
{
    scale(1.5, 1.5);
}

// ------------------------------------------------------------------------
void ImageViewer::zoomOut()
{
    scale(1.0 / 1.5, 1.0 / 1.5);
}


// ------------------------------------------------------------------------
void ImageViewer::zoomActualSize()
{
    setMatrix(QMatrix());
}

// ------------------------------------------------------------------------
void ImageViewer::zoom2Fit()
{
    fitInView(d->pixmapItem->boundingRect(), Qt::KeepAspectRatio);
}

// ------------------------------------------------------------------------
void ImageViewer::wheelEvent(QWheelEvent *e)
{
    if(e->modifiers() == Qt::ControlModifier) {
        if(e->delta() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
    } else {
        QGraphicsView::wheelEvent(e);
    }
}

