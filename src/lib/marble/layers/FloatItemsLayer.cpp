//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2012      Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#include "FloatItemsLayer.h"

#include "AbstractFloatItem.h"
#include "ViewportParams.h"

#include <QPainter>

namespace Marble
{

FloatItemsLayer::FloatItemsLayer( QObject *parent ) :
    QObject( parent ),
    m_floatItems()
{
}

void FloatItemsLayer::paint( QPainter *painter, const ViewportParams &viewport )
{
    foreach ( AbstractFloatItem *item, m_floatItems ) {
        if ( !item->enabled() )
            continue;

        if ( !item->isInitialized() ) {
            item->initialize();
            emit renderPluginInitialized( item );
        }

        if ( item->visible() ) {
            item->paintEvent( painter, &viewport );
        }
    }
}

void FloatItemsLayer::addFloatItem( AbstractFloatItem *floatItem )
{
    Q_ASSERT( floatItem );

    connect( floatItem, SIGNAL(settingsChanged(QString)),
             this,      SIGNAL(pluginSettingsChanged()) );
    connect( floatItem, SIGNAL(repaintNeeded(QRegion)),
             this,      SIGNAL(repaintNeeded(QRegion)) );
    connect( floatItem, SIGNAL(visibilityChanged(bool,QString)),
             this,      SLOT(updateVisibility(bool,QString)) );

    m_floatItems.append( floatItem );
}

QList<AbstractFloatItem *> FloatItemsLayer::floatItems() const
{
    return m_floatItems;
}

void FloatItemsLayer::updateVisibility( bool visible, const QString &nameId )
{
    emit visibilityChanged( nameId, visible );
}

}

#include "FloatItemsLayer.moc"
