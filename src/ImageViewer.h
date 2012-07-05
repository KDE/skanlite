/* ============================================================
* Date        : 2009-08-26
* Description : Preview image viewer.
*
* Copyright (C) 2008-2012 by Kåre Särs <kare.sars@iki .fi>
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

#ifndef IMAGE_VIEWER_H
#define IMAGE_VIEWER_H

#include <QGraphicsView>
#include <QWheelEvent>

class ImageViewer : public QGraphicsView
{
    Q_OBJECT
    public:
        ImageViewer(QWidget *parent = 0);
        ~ImageViewer();

        void setQImage(QImage *img);

    public Q_SLOTS:
        void zoomIn();
        void zoomOut();
        void zoom2Fit();
        void zoomActualSize();
        
    protected:
        void wheelEvent(QWheelEvent *e);
        void drawBackground(QPainter *painter, const QRectF &rect); 

    private:
        struct Private;
        Private * const d;

};

#endif


