//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2015      Stanciu Marius-Valeriu <stanciumarius94@gmail.com>
//

// Self
#include "OsmTagEditorWidget.h"
#include "OsmTagEditorWidget_p.h"
#include "ui_OsmTagEditorWidget.h"

// Qt
#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QIcon>
#include <QBitmap>
#include <QDebug>

// Marble
#include "GeoDataTypes.h"
#include "GeoDataPlacemark.h"
#include "GeoDataGeometry.h"
#include "OsmPlacemarkData.h"
#include "OsmPresetLibrary.h"

namespace Marble
{

OsmTagEditorWidget::OsmTagEditorWidget( GeoDataPlacemark *placemark, QWidget *parent )
    : QWidget( parent ),
      d( new OsmTagEditorWidgetPrivate( this ) )
{
    d->m_placemark = placemark;
    d->init( this );
}

OsmTagEditorWidget::~OsmTagEditorWidget()
{
    delete d;
}


void OsmTagEditorWidget::update()
{   d->m_currentTagsList->clear();
    d->m_recommendedTagsList->clear();
    d->populatePresetTagsList();
    d->populateCurrentTagsList();
    emit placemarkChanged( d->m_placemark );
}

QString OsmTagEditorWidget::suitableTag()
{
    /* The most suitable tag is the first tag in the list for which the OsmPresetLibrary
     * has an assigned visual category ( a QMap entry )
     * Maybe there's a better option.
     */
    for ( int index = 0; index < d->m_currentTagsList->topLevelItemCount(); ++index ) {
        const QTreeWidgetItem *item = d->m_currentTagsList->topLevelItem( index );
        OsmTagEditorWidgetPrivate::OsmTag tag( item->text( 0 ), item->text( 1 ) );
        if ( OsmPresetLibrary::hasVisualCategory( tag ) ) {
            return tag.first + "=" + tag.second;
        }
    }

    return QString();
}

void OsmTagEditorWidget::addSelectedTag()
{
    QTreeWidgetItem *selectedTag = d->m_recommendedTagsList->currentItem();

    if ( !selectedTag ) {
        return;
    }

    // Adding the tag to the placemark's osmData
    QString key = selectedTag->text( 0 );
    QString value = selectedTag->text( 1 );

    // If the value is <value>, the user has to type a value for that particular key
    if ( value == QString( "<%1>" ).arg( tr( "value" ) ) ) {
        int lastIndex = d->m_currentTagsList->topLevelItemCount() - 1;
        QTreeWidgetItem *adderItem = d->m_currentTagsList->topLevelItem( lastIndex );
        adderItem->setText( 0, key );
        d->m_currentTagsList->editItem( adderItem, 1 );
        d->m_currentTagsList->setCurrentItem( adderItem );
    }
    else {
        d->m_placemark->osmData().addTag( key, value );

        QTreeWidgetItem *newItem = d->tagWidgetItem( OsmTagEditorWidgetPrivate::OsmTag( key, value ) );
        newItem->setFlags( newItem->flags() | Qt::ItemIsUserCheckable );
        newItem->setCheckState( 0, Qt::Unchecked );
        d->m_currentTagsList->addTopLevelItem( newItem );
        update();
    }

}

void OsmTagEditorWidget::removeSelectedTag()
{
    QTreeWidgetItem *selectedTag = d->m_currentTagsList->currentItem();

    if ( !selectedTag ) {
        return;
    }

    // Adding the tag to the placemark's osmData
    QString key = selectedTag->text( 0 );
    d->m_placemark->osmData().removeTag( key );

    update();
}

void OsmTagEditorWidget::handleItemChanged( QTreeWidgetItem *item, int column )
{
    Q_UNUSED( column );
    QString key = item->text( 0 );
    QString value = item->text( 1 );

    // If any of the fields is still empty ( or the first field is "Add custom tag..."
    // the editing is not yet finished.
    if ( key.isEmpty() || value.isEmpty() || key == d->m_customTagAdderText ) {
        return;
    }

    d->m_placemark->osmData().addTag( key, value );

    update();
}

void OsmTagEditorWidget::handleDoubleClick( QTreeWidgetItem *item, int column )
{
    Q_UNUSED( column );
    int index = d->m_currentTagsList->indexOfTopLevelItem( item );
    int lastIndex = d->m_currentTagsList->topLevelItemCount() - 1;

    // The user double-clicked on the "Add custom tag..." element, so the text is cleared
    if ( index == lastIndex ) {
        QString key = item->text( 0 );

        if ( key == d->m_customTagAdderText ) {
            item->setText( 0, QString() );
        }
    }
    // The user double-clicked on a valid tag, so the tag is removed
    else if ( !item->isDisabled() ) {
        d->m_placemark->osmData().removeTag( item->text( 0 ) );
        update();
    }

}

}

#include "moc_OsmTagEditorWidget.cpp"
