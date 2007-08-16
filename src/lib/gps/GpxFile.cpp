
//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2007      Andrew Manson  <g.real.ate@gmail.com>
//

#include "GpxFile.h"
#include "WaypointContainer.h"
#include "TrackContainer.h"
#include "RouteContainer.h"
#include "BoundingBox.h"
#include "GpxSax.h"

#include <QtCore/QFile>
#include <QtXml/QXmlInputSource>
#include <QDebug>

GpxFile::GpxFile( const QString &fileName )
{
    m_tracks = new TrackContainer;
    m_waypoints = new WaypointContainer;
    m_routes = new RouteContainer;
    
    m_name = QString (fileName);
    
    QFile gpxFile( fileName );
    QXmlInputSource gpxInput( &gpxFile );
    
    QXmlSimpleReader gpxReader;
    GpxSax gpxSaxHandler( this );
    
    gpxReader.setContentHandler( &gpxSaxHandler );
    gpxReader.setErrorHandler( &gpxSaxHandler );
    
    gpxReader.parse( &gpxInput );
    
    m_checkState = Qt::Checked;
    setVisible( true );
}

GpxFile::GpxFile()
{
    m_tracks = new TrackContainer;
    m_waypoints = new WaypointContainer;
    m_routes = new RouteContainer;
    
    m_checkState = Qt::Checked;
    setVisible( true );
}

void GpxFile::draw( ClipPainter *painter, const QPoint &point )
{
    if ( !m_visible ){ 
        return;
    }
//      m_waypoints->draw( painter, point );
//     m_routes->draw( painter, point );
//     m_tracks->draw( painter, point );
}

void GpxFile::draw( ClipPainter *painter, 
                    const QSize &canvasSize, double radius,
                    Quaternion invRotAxis )
{
    if ( !m_visible ){
        return;
    }
    m_waypoints->draw( painter, canvasSize, radius, invRotAxis);
//     m_routes->draw( painter, canvasSize, radius, invRotAxis);
    m_tracks->draw( painter, canvasSize, radius, invRotAxis);
}

void GpxFile::draw( ClipPainter *painter, 
                    const QSize &canvasSize, double radius,
                    Quaternion invRotAxis, BoundingBox box )
{
    if ( !m_visible ){
        return;
    }
//     m_waypoints->draw( painter, canvasSize, radius, invRotAxis,
// box);
    m_routes->draw( painter, canvasSize, radius, invRotAxis, box);
    m_tracks->draw( painter, canvasSize, radius, invRotAxis, box);
}

void GpxFile::addWaypoint( Waypoint *waypoint )
{
    m_waypoints->append( (AbstractLayerData*)waypoint );
}

void GpxFile::addTrack( Track *track )
{
    m_tracks->append( (AbstractLayerData*)track );
}

void GpxFile::addRoute( Route *route )
{
    m_routes->append( (AbstractLayerData*)route );
}

void    GpxFile::setName( const QString &name )
{
    m_name = name;
}

Qt::ItemFlags   GpxFile::flags() const
{
    return Qt::ItemFlags( Qt::ItemIsUserCheckable | 
                          Qt::ItemIsEnabled | 
                          Qt::ItemIsSelectable );
}

QString GpxFile::display()
{
    return m_name;
}

Qt::CheckState  GpxFile::checkState()
{
    return m_checkState;
}

void    GpxFile::setCheckState( Qt::CheckState state )
{
    m_checkState = state;
}

void    GpxFile::setCheckState( bool state )
{
    setVisible( state );
    
    if ( state ) {
        m_checkState = Qt::Checked;
        return;
    }
    
    m_checkState = Qt::Unchecked;
}
