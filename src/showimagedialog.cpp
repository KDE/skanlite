/* ============================================================
 *
 * SPDX-FileCopyrightText: 2007-2012 Kåre Särs <kare.sars@iki .fi>
 * SPDX-FileCopyrightText: 2009 Arseniy Lartsev <receive-spam at yandex dot ru>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2018 Alexander Volkov <a.volkov@rusbitech.ru>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 * ============================================================ */

#include "showimagedialog.h"
#include "ImageViewer.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

ShowImageDialog::ShowImageDialog(QWidget *parent)
    : QDialog(parent)
{
    auto *mainLayout = new QVBoxLayout(this);

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

#include "moc_showimagedialog.cpp"
