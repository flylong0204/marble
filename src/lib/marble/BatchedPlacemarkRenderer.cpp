//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2016 Torsten Rahn    <tackat@kde.org>
//

#include "BatchedPlacemarkRenderer.h"
#include "GeoPainter.h"

#include <QDebug>

namespace Marble
{

BatchedPlacemarkRenderer:: BatchedPlacemarkRenderer(GeoPainter * painter)
    : m_painter(painter)
{
    m_pixmapCache.setCacheLimit(4096);
}

BatchedPlacemarkRenderer::~BatchedPlacemarkRenderer()
{
}

void BatchedPlacemarkRenderer::addTextFragment(  const QPoint& position, const QString& text,
                                                 const QColor& color, QFlags<BatchedPlacemarkRenderer::Frames> flags )
{
    TextFragment fragment;
    fragment.text = text;
    fragment.position = position;
    fragment.color = color;
    fragment.flags = flags;
    m_textFragments.append(fragment);
}

void BatchedPlacemarkRenderer::clearTextFragments()
{
    m_textFragments.clear();
}

void BatchedPlacemarkRenderer::drawTextFragments()
{
    QPixmap pixmap(10,10);
    QPainter textPainter;

    textPainter.begin(&pixmap);
    QFontMetrics metrics = textPainter.fontMetrics();
    textPainter.end();


    for (int i = 0; i < m_textFragments.size(); ++i)
    {
        QString key = m_textFragments[i].text + ":" + QString::number( static_cast<int>(m_textFragments[i].flags) );

        if (!m_pixmapCache.find(key, &pixmap)) {
            bool hasRoundFrame = m_textFragments[i].flags.testFlag(RoundFrame);

            int width = metrics.width(m_textFragments[i].text);
            int height = metrics.height();
            QSize size = hasRoundFrame
                                  ? QSize(qMax(1.2*width, 1.1*height), 1.2*height)
                                  : QSize(width, height);
            pixmap = QPixmap(size);
            pixmap.fill(Qt::transparent);
            QRect labelRect(QPoint(), size);
            textPainter.begin(&pixmap);
            textPainter.setRenderHint(QPainter::Antialiasing, true);

            QColor const brushColor = m_textFragments[i].color;
            if (hasRoundFrame) {
              QColor lighterColor = brushColor.lighter(110);
              lighterColor.setAlphaF(0.9);
              textPainter.setBrush(lighterColor);
              textPainter.drawRoundedRect(labelRect, 3, 3);
            }

            textPainter.setBrush(brushColor);
            textPainter.drawText(labelRect, Qt::AlignHCenter , m_textFragments[i].text);

            if (hasRoundFrame) {
                textPainter.setBrush(brushColor);
            }

            textPainter.end();
            m_pixmapCache.insert(key, pixmap);
        }
        m_painter->drawPixmap(m_textFragments[i].position, pixmap);
    }
}

}
