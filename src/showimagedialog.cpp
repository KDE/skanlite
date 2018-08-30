/* ============================================================
*
* Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
* Copyright (C) 2009 by Arseniy Lartsev <receive-spam at yandex dot ru>
* Copyright (C) 2014 by Gregor Mitsch: port to KDE5 frameworks
* Copyright (C) 2018 by Alexander Volkov <a.volkov@rusbitech.ru>
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

#include "showimagedialog.h"
#include "ImageViewer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>

ShowImageDialog::ShowImageDialog(QWidget *parent)
    : QDialog(parent)
{
    auto *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);

    m_imageViewer = new ImageViewer;

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Discard);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ShowImageDialog::saveRequested);
    connect(buttonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, this, &QDialog::reject);
    m_saveButton = buttonBox->button(QDialogButtonBox::Save);

    auto *buttonsLayout = new QHBoxLayout;
    const auto imageViewerActions = m_imageViewer->actions();
    for (auto *action : imageViewerActions) {
        auto *toolButton = new QToolButton;
        toolButton->setDefaultAction(action);
        buttonsLayout->addWidget(toolButton);
    }
    buttonsLayout->addWidget(buttonBox);

    mainLayout->addWidget(m_imageViewer);
    mainLayout->addLayout(buttonsLayout);

    resize(640, 480);
}

void ShowImageDialog::setQImage(QImage *img)
{
    m_imageViewer->setQImage(img);
}

void ShowImageDialog::zoom2Fit()
{
    m_imageViewer->zoom2Fit();
}

void ShowImageDialog::showEvent(QShowEvent *e)
{
    m_saveButton->setFocus();
    QDialog::showEvent(e);
}
