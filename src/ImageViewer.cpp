/* ============================================================
* Date        : 2008-08-26
* Description : Preview image viewer.
*
* SPDX-FileCopyrightText: 2008-2012 Kåre Särs <kare.sars@iki .fi>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*
* ============================================================ */

#include "ImageViewer.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QScrollBar>
#include <QAction>
#include <QIcon>
#include <QWheelEvent>

#include <KLocalizedString>

struct ImageViewer::Private {
    QGraphicsScene      *scene = nullptr;
    QImage              *img = nullptr;

    QAction *zoomInAction = nullptr;
    QAction *zoomOutAction = nullptr;
    QAction *zoom100Action = nullptr;
    QAction *zoom2FitAction = nullptr;
};

ImageViewer::ImageViewer(QWidget *parent) : QGraphicsView(parent), d(new Private)
{
    //setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    //setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setMouseTracking(true);

    // Init the scene
    d->scene = new QGraphicsScene;
    setScene(d->scene);

    // create context menu
    d->zoomInAction = new QAction(QIcon::fromTheme(QStringLiteral("zoom-in")), i18n("Zoom In"), this);
    connect(d->zoomInAction, &QAction::triggered, this, &ImageViewer::zoomIn);

    d->zoomOutAction = new QAction(QIcon::fromTheme(QStringLiteral("zoom-out")), i18n("Zoom Out"), this);
    connect(d->zoomOutAction, &QAction::triggered, this, &ImageViewer::zoomOut);

    d->zoom100Action = new QAction(QIcon::fromTheme(QStringLiteral("zoom-original")), i18n("Zoom to Actual size"), this);
    connect(d->zoom100Action, &QAction::triggered, this, &ImageViewer::zoomActualSize);

    d->zoom2FitAction = new QAction(QIcon::fromTheme(QStringLiteral("zoom-fit-best")), i18n("Zoom to Fit"), this);
    connect(d->zoom2FitAction, &QAction::triggered, this, &ImageViewer::zoom2Fit);

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
    if (img == nullptr) {
        return;
    }

    d->img = img;
    const auto dpr = devicePixelRatioF();
    d->img->setDevicePixelRatio(dpr);
    d->scene->setSceneRect(0, 0, img->width() / dpr, img->height() / dpr);
}

// ------------------------------------------------------------------------
void ImageViewer::drawBackground(QPainter *painter, const QRectF &rect)
{
    painter->fillRect(rect, QColor(0x70, 0x70, 0x70));
    QRectF r = rect & sceneRect();
    const auto dpr = d->img->devicePixelRatio();
    QRectF srcRect = QRectF(r.topLeft() * dpr, r.size() * dpr);
    painter->drawImage(r, *d->img, srcRect);
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
    resetTransform();
}

// ------------------------------------------------------------------------
void ImageViewer::zoom2Fit()
{
    QRectF r = d->img->rect();
    const auto dpr = d->img->devicePixelRatio();
    r = QRectF(r.topLeft() / dpr, r.size() / dpr);
    fitInView(r, Qt::KeepAspectRatio);
}

// ------------------------------------------------------------------------
void ImageViewer::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() == Qt::ControlModifier) {
        if (e->angleDelta().y() > 0) {
            zoomIn();
        }
        else {
            zoomOut();
        }
    }
    else {
        QGraphicsView::wheelEvent(e);
    }
}


#include "moc_ImageViewer.cpp"
