//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2009 Torsten Rahn <tackat@kde.org>
//

#include "GraticulePlugin.h"
#include "ui_GraticuleConfigWidget.h"

#include "GeoLineStringGraphicsItem.h"
#include "GeoDataLineString.h"
#include "GeoDataPlacemark.h"
#include "GeoDataStyle.h"
#include "GeoDataLabelStyle.h"
#include "GeoDataLatLonAltBox.h"
#include "GeoDataLineStyle.h"
#include "GeoPainter.h"
#include "MarbleDebug.h"
#include "MarbleDirs.h"
#include "MarbleModel.h"
#include "Planet.h"
#include "PluginAboutDialog.h"
#include "ViewportParams.h"

// Qt
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtCore/QRect>
#include <QtGui/QColor>
#include <QtGui/QPixmap>
#include <QtGui/QBrush>
#include <QColorDialog>


namespace Marble
{

/* A helper class to ensure placemarks are destroyed along with their
   GeoLineStringGraphicsItems */
class GraticulePlugin::ItemHelper
{
    public:
        ItemHelper( const QString &name,
                    GeoDataLineString *line,
                    const GeoDataStyle *style )
            : placemark( name ),
              graphicsItem( &placemark, line )
        {
            graphicsItem.setStyle( style );
        }

        GeoDataPlacemark placemark;
        GeoLineStringGraphicsItem graphicsItem;
};

GraticulePlugin::GraticulePlugin()
    : RenderPlugin( 0 ),
      ui_configWidget( 0 ),
      m_configDialog( 0 )
{
}

GraticulePlugin::GraticulePlugin( const MarbleModel *marbleModel )
    : RenderPlugin( marbleModel ),
      m_isInitialized( false ),
      ui_configWidget( 0 ),
      m_configDialog( 0 )
{
    m_gridStyle.lineStyle().setColor( QColor( Qt::white ) );
    m_tropicsStyle.lineStyle().setColor( QColor( Qt::yellow ) );
    m_equatorStyle.lineStyle().setColor( QColor( Qt::yellow ) );

    m_boldGridStyle.lineStyle().setWidth( 1.5 );

    m_tropicsStyle.lineStyle().setPenStyle( Qt::DotLine );

    m_gridStyle.labelStyle().setAlignment( GeoDataLabelStyle::Corner );
    m_boldGridStyle.labelStyle().setAlignment( GeoDataLabelStyle::Center );
    m_tropicsStyle.labelStyle().setAlignment( GeoDataLabelStyle::Center );
    m_equatorStyle.labelStyle().setAlignment( GeoDataLabelStyle::Center );
}

GraticulePlugin::~GraticulePlugin()
{
    qDeleteAll( m_items );
}

QStringList GraticulePlugin::backendTypes() const
{
    return QStringList( "graticule" );
}

QString GraticulePlugin::renderPolicy() const
{
    return QString( "ALWAYS" );
}

QStringList GraticulePlugin::renderPosition() const
{
    return QStringList( "SURFACE" );
}

QString GraticulePlugin::name() const
{
    return tr( "Coordinate Grid" );
}

QString GraticulePlugin::guiString() const
{
    return tr( "Coordinate &Grid" );
}

QString GraticulePlugin::nameId() const
{
    return QString( "coordinate-grid" );
}

QString GraticulePlugin::version() const
{
    return "1.0";
}

QString GraticulePlugin::description() const
{
    return tr( "A plugin that shows a coordinate grid." );
}

QString GraticulePlugin::copyrightYears() const
{
    return "2009";
}

QList<PluginAuthor> GraticulePlugin::pluginAuthors() const
{
    return QList<PluginAuthor>()
            << PluginAuthor( "Torsten Rahn", "tackat@kde.org" );
}

QIcon GraticulePlugin::icon () const
{
    return QIcon(":/icons/coordinate.png");
}

void GraticulePlugin::initialize ()
{
    // Initialize range maps that map the zoom to the number of coordinate grid lines.
    
    initLineMaps( GeoDataCoordinates::defaultNotation() );                

    m_isInitialized = true;
}

bool GraticulePlugin::isInitialized () const
{
    return m_isInitialized;
}

QDialog *GraticulePlugin::configDialog()
{
    if ( !m_configDialog ) {
        m_configDialog = new QDialog();
        ui_configWidget = new Ui::GraticuleConfigWidget;
        ui_configWidget->setupUi( m_configDialog );

        connect( ui_configWidget->gridPushButton, SIGNAL(clicked()), this,
                SLOT(gridGetColor()) );
        connect( ui_configWidget->tropicsPushButton, SIGNAL(clicked()), this,
                SLOT(tropicsGetColor()) );
        connect( ui_configWidget->equatorPushButton, SIGNAL(clicked()), this,
                SLOT(equatorGetColor()) );


        connect( ui_configWidget->m_buttonBox, SIGNAL(accepted()), this, 
                SLOT(writeSettings()) );
        connect( ui_configWidget->m_buttonBox->button( QDialogButtonBox::Reset ), SIGNAL(clicked()),
                 SLOT(restoreDefaultSettings()) );
        QPushButton *applyButton = ui_configWidget->m_buttonBox->button( QDialogButtonBox::Apply );
        connect( applyButton, SIGNAL(clicked()),
                 this,        SLOT(writeSettings()) );
    }

    readSettings();

    return m_configDialog;
}


QHash<QString,QVariant> GraticulePlugin::settings() const
{
    QHash<QString, QVariant> settings = RenderPlugin::settings();

    settings.insert( "gridColor", m_gridStyle.lineStyle().color().name() );
    settings.insert( "tropicsColor", m_tropicsStyle.lineStyle().color().name() );
    settings.insert( "equatorColor", m_equatorStyle.lineStyle().color().name() );

    return settings;
}

void GraticulePlugin::setSettings( const QHash<QString,QVariant> &settings )
{
    RenderPlugin::setSettings( settings );

    const QColor gridColor = settings.value( "gridColor", QColor( Qt::white ) ).value<QColor>();
    const QColor tropicsColor = settings.value( "tropicsColor", QColor( Qt::yellow ) ).value<QColor>();
    const QColor equatorColor = settings.value( "equatorColor", QColor( Qt::yellow ) ).value<QColor>();

#ifdef Q_OS_MACX
    int defaultFontSize = 10;
#else
    int defaultFontSize = 8;
#endif
    QFont gridFont("Sans Serif");
    gridFont.setPointSize( defaultFontSize );
    gridFont.setBold( true );

    m_gridStyle.lineStyle().setColor( gridColor );
    m_gridStyle.labelStyle().setFont( gridFont );
    m_gridStyle.labelStyle().setColor( gridColor );

    m_boldGridStyle.lineStyle().setColor( gridColor );
    m_boldGridStyle.labelStyle().setFont( gridFont );
    m_boldGridStyle.labelStyle().setColor( gridColor );

    m_tropicsStyle.lineStyle().setColor( tropicsColor );
    m_tropicsStyle.labelStyle().setFont( gridFont );
    m_tropicsStyle.labelStyle().setColor( tropicsColor );

    m_equatorStyle.lineStyle().setColor( equatorColor );
    m_equatorStyle.labelStyle().setFont( gridFont );
    m_equatorStyle.labelStyle().setColor( equatorColor );

    readSettings();
}


void GraticulePlugin::readSettings()
{
    if ( !m_configDialog )
        return;

    QPalette gridPalette;
    gridPalette.setColor( QPalette::Button, m_gridStyle.lineStyle().color() );
    ui_configWidget->gridPushButton->setPalette( gridPalette );

    QPalette tropicsPalette;
    tropicsPalette.setColor( QPalette::Button, m_tropicsStyle.lineStyle().color() );
    ui_configWidget->tropicsPushButton->setPalette( tropicsPalette );


    QPalette equatorPalette;
    equatorPalette.setColor( QPalette::Button, m_equatorStyle.lineStyle().color() );
    ui_configWidget->equatorPushButton->setPalette( equatorPalette );
}

void GraticulePlugin::gridGetColor()
{
    const QColor c = QColorDialog::getColor( m_gridStyle.lineStyle().color(), 
        0, tr("Please choose the color for the coordinate grid.") );

    if ( c.isValid() ) {
        QPalette palette = ui_configWidget->gridPushButton->palette();
        palette.setColor( QPalette::Button, c );
        ui_configWidget->gridPushButton->setPalette( palette );
    }
}

void GraticulePlugin::tropicsGetColor()
{
    const QColor c = QColorDialog::getColor( m_tropicsStyle.lineStyle().color(),
        0, tr("Please choose the color for the tropic circles.") );

    if ( c.isValid() ) {
        QPalette palette = ui_configWidget->tropicsPushButton->palette();
        palette.setColor( QPalette::Button, c );
        ui_configWidget->tropicsPushButton->setPalette( palette );
    }
}

void GraticulePlugin::equatorGetColor()
{
    const QColor c = QColorDialog::getColor( m_equatorStyle.lineStyle().color(),
        0, tr("Please choose the color for the equator.") );

    if ( c.isValid() ) {
        QPalette palette = ui_configWidget->equatorPushButton->palette();
        palette.setColor( QPalette::Button, c );
        ui_configWidget->equatorPushButton->setPalette( palette );
    }
}

void GraticulePlugin::writeSettings()
{
    const QColor gridColor = ui_configWidget->gridPushButton->palette().color( QPalette::Button);
    const QColor equatorColor = ui_configWidget->equatorPushButton->palette().color( QPalette::Button );
    const QColor tropicsColor = ui_configWidget->tropicsPushButton->palette().color( QPalette::Button );

    m_gridStyle.lineStyle().setColor( gridColor );
    m_gridStyle.labelStyle().setColor( gridColor );

    m_boldGridStyle.lineStyle().setColor( gridColor );
    m_boldGridStyle.labelStyle().setColor( gridColor );

    m_equatorStyle.lineStyle().setColor( equatorColor );
    m_equatorStyle.labelStyle().setColor( equatorColor );

    m_tropicsStyle.lineStyle().setColor( tropicsColor );
    m_tropicsStyle.labelStyle().setColor( tropicsColor );

    emit settingsChanged( nameId() );
}

bool GraticulePlugin::render( GeoPainter *painter, ViewportParams *viewport,
				const QString& renderPos,
				GeoSceneLayer * layer )
{
    Q_UNUSED( layer )
    Q_UNUSED( renderPos )

    if ( m_currentNotation != GeoDataCoordinates::defaultNotation() ) {
        initLineMaps( GeoDataCoordinates::defaultNotation() );
    }

    qDeleteAll( m_items );
    m_items.clear();

    renderGrid( viewport );

    for( int i = m_items.size() - 1; i >= 0; --i ) {
        m_items.at( i )->graphicsItem.paint( painter, viewport );
    }

    return true;
}

qreal GraticulePlugin::zValue() const
{
    return 1.0;
}

void GraticulePlugin::renderGrid( const ViewportParams *viewport )
{
    GeoDataLatLonAltBox viewLatLonAltBox = viewport->viewLatLonAltBox();

    // Render UTM grid zones
    if ( m_currentNotation == GeoDataCoordinates::UTM ) {
        renderLatitudeLine( m_gridStyle, 84.0, viewLatLonAltBox );

        renderLongitudeLines( m_gridStyle, viewLatLonAltBox,
                              6.0, 18.0, 154.0 );
        renderLongitudeLines( m_gridStyle, viewLatLonAltBox,
                              6.0, 34.0, 10.0 );

        // Paint longitudes with exceptions
        renderLongitudeLines( m_gridStyle, viewLatLonAltBox,
                              6.0, 6.0, 162.0 );
        renderLongitudeLines( m_gridStyle, viewLatLonAltBox,
                              6.0, 26.0, 146.0 );

        renderLatitudeLines( m_gridStyle, viewLatLonAltBox, 8.0 );

        return;
    }

    // Render the normal grid

    // calculate the angular distance between coordinate lines of the normal grid
    qreal normalDegreeStep = 360.0 / m_normalLineMap.lowerBound(viewport->radius()).value();

    renderLongitudeLines( m_gridStyle,
                          viewLatLonAltBox,
                          normalDegreeStep, normalDegreeStep, normalDegreeStep );
    renderLatitudeLines(  m_gridStyle,
                          viewLatLonAltBox,
                          normalDegreeStep );

    // Render some non-cut off longitude lines ..
    renderLongitudeLine( m_gridStyle, +90.0, viewLatLonAltBox );
    renderLongitudeLine( m_gridStyle, -90.0, viewLatLonAltBox );

    // Render the bold grid

    // calculate the angular distance between coordinate lines of the bold grid
    qreal boldDegreeStep = 360.0 / m_boldLineMap.lowerBound(viewport->radius()).value();

    renderLongitudeLines( m_boldGridStyle,
                          viewLatLonAltBox, boldDegreeStep, normalDegreeStep );
    renderLatitudeLines( m_boldGridStyle,
                         viewLatLonAltBox, boldDegreeStep );

    // Render the equator
    renderLatitudeLine( m_equatorStyle, 0.0, viewLatLonAltBox, tr( "Equator" ) );

    // Render the Prime Meridian and Antimeridian
    GeoDataCoordinates::Notation notation = GeoDataCoordinates::defaultNotation();
    if (marbleModel()->planet()->id() != "sky" && notation != GeoDataCoordinates::Astro) {
        renderLongitudeLine( m_equatorStyle,
                             0.0, viewLatLonAltBox, 0.0, 0.0,
                             tr( "Prime Meridian" ) );
        renderLongitudeLine( m_equatorStyle,
                             180.0, viewLatLonAltBox, 0.0, 0.0,
                             tr( "Antimeridian" ) );
    }

    // Determine the planet's axial tilt
    qreal axialTilt = RAD2DEG * marbleModel()->planet()->epsilon();

    if ( axialTilt > 0 ) {
        // Render the tropics
        renderLatitudeLine( m_tropicsStyle,
                            +axialTilt, viewLatLonAltBox,
                            tr( "Tropic of Cancer" )  );
        renderLatitudeLine( m_tropicsStyle,
                            -axialTilt, viewLatLonAltBox,
                            tr( "Tropic of Capricorn" ) );

        // Render the arctics
        renderLatitudeLine( m_tropicsStyle,
                            +90.0 - axialTilt, viewLatLonAltBox,
                            tr( "Arctic Circle" ) );
        renderLatitudeLine( m_tropicsStyle,
                            -90.0 + axialTilt, viewLatLonAltBox,
                            tr( "Antarctic Circle" ) );
    }    
}

void GraticulePlugin::renderLatitudeLine( const GeoDataStyle &style,
                                          qreal latitude,
                                          const GeoDataLatLonAltBox& viewLatLonAltBox,
                                          const QString& lineLabel )
{
    qreal fromSouthLat = viewLatLonAltBox.south( GeoDataCoordinates::Degree );
    qreal toNorthLat   = viewLatLonAltBox.north( GeoDataCoordinates::Degree );

    // Coordinate line is not displayed inside the viewport
    if ( latitude < fromSouthLat || toNorthLat < latitude ) {
        // mDebug() << "Lat: Out of View";
        return;
    }

    GeoDataLineString *line = new GeoDataLineString( Tessellate | RespectLatitudeCircle ) ;

    qreal fromWestLon = viewLatLonAltBox.west( GeoDataCoordinates::Degree );
    qreal toEastLon   = viewLatLonAltBox.east( GeoDataCoordinates::Degree );

    if ( fromWestLon < toEastLon ) {
        qreal step = ( toEastLon - fromWestLon ) * 0.25;

        for ( int i = 0; i < 5; ++i ) {
            *line << GeoDataCoordinates( fromWestLon + i * step, latitude, 0.0, GeoDataCoordinates::Degree );
        }
    }
    else {
        qreal step = ( +180.0 - toEastLon ) * 0.25;

        for ( int i = 0; i < 5; ++i ) {
            *line << GeoDataCoordinates( toEastLon + i * step, latitude, 0.0, GeoDataCoordinates::Degree );
        }

        step = ( +180 + fromWestLon ) * 0.25;

        for ( int i = 0; i < 5; ++i ) {
            *line << GeoDataCoordinates( -180 + i * step, latitude, 0.0, GeoDataCoordinates::Degree );
        }
    }

    m_items.append( new GraticulePlugin::ItemHelper( lineLabel, line, &style ) );
}

void GraticulePlugin::renderLongitudeLine( const GeoDataStyle &style,
                                           qreal longitude,
                                           const GeoDataLatLonAltBox& viewLatLonAltBox, 
                                           qreal northPolarGap, qreal southPolarGap,
                                           const QString& lineLabel )
{
    const qreal fromWestLon = viewLatLonAltBox.west();
    const qreal toEastLon   = viewLatLonAltBox.east();

    // Coordinate line is not displayed inside the viewport
    if ( ( !viewLatLonAltBox.crossesDateLine() 
           && ( longitude * DEG2RAD < fromWestLon || toEastLon < longitude * DEG2RAD ) ) ||
         (  viewLatLonAltBox.crossesDateLine() &&
            longitude * DEG2RAD < toEastLon && fromWestLon < longitude * DEG2RAD &&
            fromWestLon != -M_PI && toEastLon != +M_PI )
       ) {
        // mDebug() << "Lon: Out of View:" << viewLatLonAltBox.toString() << " Crossing: "<< viewLatLonAltBox.crossesDateLine() << "Longitude: " << longitude;
        return;
    }

    qreal fromSouthLat = viewLatLonAltBox.south( GeoDataCoordinates::Degree );
    qreal toNorthLat   = viewLatLonAltBox.north( GeoDataCoordinates::Degree );
    
    qreal southLat = ( fromSouthLat < -90.0 + southPolarGap ) ? -90.0 + southPolarGap : fromSouthLat;
    qreal northLat = ( toNorthLat   > +90.0 - northPolarGap ) ? +90.0 - northPolarGap : toNorthLat;

    GeoDataCoordinates n1( longitude, southLat, 0.0, GeoDataCoordinates::Degree );
    GeoDataCoordinates n3( longitude, northLat, 0.0, GeoDataCoordinates::Degree );

    GeoDataLineString *line = new GeoDataLineString( Tessellate );

    if ( northLat > 0 && southLat < 0 )
    {
        GeoDataCoordinates n2( longitude, 0.0, 0.0, GeoDataCoordinates::Degree );
        *line << n1 << n2 << n3;
    }
    else {
        *line << n1 << n3;
    }

    m_items.append( new GraticulePlugin::ItemHelper( lineLabel, line, &style ) );
}

void GraticulePlugin::renderLatitudeLines( const GeoDataStyle &style,
                                           const GeoDataLatLonAltBox& viewLatLonAltBox,
                                           qreal step )
{
    if ( step <= 0 ) {
        return;
    }

    // Latitude
    qreal southLat = viewLatLonAltBox.south( GeoDataCoordinates::Degree );
    qreal northLat = viewLatLonAltBox.north( GeoDataCoordinates::Degree );

    qreal southLineLat = step * static_cast<int>( southLat / step ); 
    qreal northLineLat = step * ( static_cast<int>( northLat / step ) + 1 );
    
    if ( m_currentNotation == GeoDataCoordinates::UTM ) {
    	if ( northLineLat > 84.0 )
	    	northLineLat = 76.0;
	    	
	    if ( southLineLat < -80.0 )
	    	southLineLat = -80.0;
    }

    qreal itStep = southLineLat;

    GeoDataCoordinates::Notation notation = GeoDataCoordinates::defaultNotation();

    while ( itStep < northLineLat ) {
        // Create a matching label
        QString label = GeoDataCoordinates::latToString( itStep, notation,
                                 GeoDataCoordinates::Degree, -1, 'g' );

        // No additional labels for the equator
        if ( ( style.labelStyle().alignment() == GeoDataLabelStyle::Center ) && itStep == 0.0 ) {
            label.clear();
        }

        // Paint all latitude coordinate lines except for the equator
        if ( itStep != 0.0 ) {
            renderLatitudeLine( style, itStep, viewLatLonAltBox, label );
        }

        itStep += step;
    }
}


void GraticulePlugin::renderUtmExceptions( const GeoDataStyle &style,
                                           const GeoDataLatLonAltBox& viewLatLonAltBox,
                                           qreal itStep, qreal northPolarGap, qreal southPolarGap,
                                           const QString & label )
{
    // This code renders the so called "exceptions" in the UTM coordinate grid
    // See: http://en.wikipedia.org/wiki/Universal_Transverse_Mercator_coordinate_system#Exceptions
    if ( northPolarGap == 6.0 && southPolarGap == 162.0) {
        if ( label == "31" ) {
            renderLongitudeLine( style,
                                 itStep+3.0, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        } else if ( label == "33" ) {
            renderLongitudeLine( style,
                                 itStep+3.0, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        } else if ( label == "35" ) {
            renderLongitudeLine( style,
                                 itStep+3.0, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        } else if ( label == "37" ) {
            renderLongitudeLine( style,
                                 itStep, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        } else if ( label == "32" || label == "34" || label == "36" ) {
            // paint nothing
        } else {
            renderLongitudeLine( style,
                                 itStep, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        }
    }
    else if ( northPolarGap == 26.0 && southPolarGap == 146.0 ) {
        if ( label == "31" ) {
            renderLongitudeLine( style,
                                 itStep-3.0, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        } else {
            renderLongitudeLine( style,
                                 itStep, viewLatLonAltBox, northPolarGap,
                                 southPolarGap, label );
        }
    }
    else {
        renderLongitudeLine( style, itStep, viewLatLonAltBox, northPolarGap,
                             southPolarGap, label );
    }
}

void GraticulePlugin::renderLongitudeLines( const GeoDataStyle &style,
                                            const GeoDataLatLonAltBox& viewLatLonAltBox,
                                            qreal step, qreal northPolarGap, qreal southPolarGap )
{
    if ( step <= 0 ) {
        return;
    }

    GeoDataCoordinates::Notation notation = marbleModel()->planet()->id() == "sky" ? GeoDataCoordinates::Astro :
                                                                                     GeoDataCoordinates::defaultNotation();

    // Longitude
    qreal westLon = viewLatLonAltBox.west( GeoDataCoordinates::Degree );
    qreal eastLon = viewLatLonAltBox.east( GeoDataCoordinates::Degree );

    qreal westLineLon = step * static_cast<int>( westLon / step );
    qreal eastLineLon = step * ( static_cast<int>( eastLon / step ) + 1 );

    if ( !viewLatLonAltBox.crossesDateLine() ||
         ( westLon == -180.0 && eastLon == +180.0 ) ) {
        qreal itStep = westLineLon;

        while ( itStep < eastLineLon ) {
            // Create a matching label
            QString label = GeoDataCoordinates::lonToString( itStep,
                                  notation, GeoDataCoordinates::Degree,
                                  -1, 'g' );

            // No additional labels for the prime meridian and the antimeridian

            if ( ( style.labelStyle().alignment() == GeoDataLabelStyle::Center ) &&
                 ( itStep == 0.0 || itStep == 180.0 || itStep == -180.0 ) )
            {
                label.clear();
            }

            // Paint all longitude coordinate lines except for the meridians
            if ( itStep != 0.0 && itStep != 180.0 && itStep != -180.0 ) {
                // handle exceptions for UTM grid
                if (notation == GeoDataCoordinates::UTM ) {
                    renderUtmExceptions( style,
                                         viewLatLonAltBox, itStep, northPolarGap,
                                         southPolarGap, label );
                } else {
                    renderLongitudeLine( style,
                                         itStep, viewLatLonAltBox, northPolarGap,
                                         southPolarGap, label );
                }
            }
            itStep += step;
        }
    }
    else {
        qreal itStep = eastLineLon;

        while ( itStep < 180.0 ) {

            // Create a matching label
            QString label = GeoDataCoordinates::lonToString( itStep,
                                  notation, GeoDataCoordinates::Degree,
                                  -1, 'g' );

            // No additional labels for the prime meridian and the antimeridian

            if ( ( style.labelStyle().alignment() == GeoDataLabelStyle::Center ) &&
                 ( itStep == 0.0 || itStep == 180.0 || itStep == -180.0 ) )
            {
                label.clear();
            }

            // Paint all longitude coordinate lines except for the meridians
            if ( itStep != 0.0 && itStep != 180.0 && itStep != -180.0 ) {
                if (notation == GeoDataCoordinates::UTM ) {
                    renderUtmExceptions( style,
                                         viewLatLonAltBox, itStep, northPolarGap,
                                         southPolarGap, label );
                } else {
                    renderLongitudeLine( style,
                                         itStep, viewLatLonAltBox, northPolarGap,
                                         southPolarGap );
                }
            }
            itStep += step;
        }

        itStep = -180.0;
        while ( itStep < westLineLon ) {

            // Create a matching label
            QString label = GeoDataCoordinates::lonToString( itStep,
                                  notation, GeoDataCoordinates::Degree,
                                  -1, 'g' );

            // No additional labels for the prime meridian and the antimeridian
            if ( ( style.labelStyle().alignment() == GeoDataLabelStyle::Center ) &&
                 ( itStep == 0.0 || itStep == 180.0 || itStep == -180.0 ) )
            {
                label.clear();
            }

            // Paint all longitude coordinate lines except for the meridians
            if ( itStep != 0.0 && itStep != 180.0 && itStep != -180.0 ) {
                if (notation == GeoDataCoordinates::UTM ) {
                    renderUtmExceptions( style,
                                         viewLatLonAltBox, itStep, northPolarGap,
                                         southPolarGap, label );
                } else {
                    renderLongitudeLine( style,
                                         itStep, viewLatLonAltBox, northPolarGap,
                                         southPolarGap, label );
                }
            }
            itStep += step;
        }
    }
}

void GraticulePlugin::initLineMaps( GeoDataCoordinates::Notation notation)
{
    /* Define Upper Bound keys and associated values:
       The key number is the globe radius in pixel.
       The value number is the amount of grid lines for the full range.

       Example: up to a 100 pixel radius the globe is covered
       with 4 longitude lines (4 half-circles).
     */

    if (marbleModel()->planet()->id() == "sky" || notation == GeoDataCoordinates::Astro) {
        m_normalLineMap[100]     = 4;          // 6h
        m_normalLineMap[1000]    = 12;          // 2h
        m_normalLineMap[2000]   = 24;         // 1h
        m_normalLineMap[4000]   = 48;         // 30 min
        m_normalLineMap[8000]   = 96;         // 15 min
        m_normalLineMap[16000]  = 288;        // 5 min
        m_normalLineMap[100000]  = 24 * 60;     // 1 min
        m_normalLineMap[200000]  = 24 * 60 * 2; // 30 sec
        m_normalLineMap[400000]  = 24 * 60 * 4; // 15 sec
        m_normalLineMap[1200000] = 24 * 60 * 12; // 5 sec
        m_normalLineMap[6000000] = 24 * 60 * 60; // 1 sec
        m_normalLineMap[12000000] = 24 * 60 * 60 * 2; // 0.5 sec
        m_normalLineMap[24000000] = 24 * 60 * 60 * 4; // 0.25 sec

        m_boldLineMap[1000]     = 0;        // 0h
        m_boldLineMap[2000]    = 4;         //  6h
        m_boldLineMap[16000]    = 24;       //  30 deg
        return;
    }

    m_normalLineMap[100]     = 4;          // 90 deg
    m_normalLineMap[1000]    = 12;          // 30 deg
    m_normalLineMap[4000]   = 36;         // 10 deg
    m_normalLineMap[16000]   = 72;         // 5 deg
    m_normalLineMap[64000]  = 360;         //  1 deg
    m_normalLineMap[128000] = 720;        //  0.5 deg

    m_boldLineMap[1000]     = 0;         //  0 deg
    m_boldLineMap[4000]    = 12;         //  30 deg
    m_boldLineMap[16000]   = 36;         //  10 deg

    switch ( notation )
    {
        case GeoDataCoordinates::Decimal :
            
            m_normalLineMap[512000]  = 360 * 10;       //  0.1 deg
            m_normalLineMap[2048000] = 360 * 20;       //  0.05 deg
            m_normalLineMap[8192000] = 360 * 100;      //  0.01 deg
            m_normalLineMap[16384000] = 360 * 200;      //  0.005 deg
            m_normalLineMap[32768000] = 360 * 1000;    //  0.001 deg
            m_normalLineMap[131072000] = 360 * 2000;    //  0.0005 deg
            m_normalLineMap[524288000] = 360 * 10000;  //  0.00001 deg

            m_boldLineMap[512000]     = 360;          // 0.1 deg
            m_boldLineMap[2048000]    = 720;          // 0.05 deg
            m_boldLineMap[8192000]   = 360 * 10;     // 0.01 deg
            m_boldLineMap[1638400]   = 360 * 20;     // 0.005 deg
            m_boldLineMap[32768000]  = 360 * 100;    // 0.001 deg
            m_boldLineMap[131072000]  = 360 * 200;    // 0.0005 deg
            m_boldLineMap[524288000] = 360 * 1000;   // 0.00001 deg

        break;
        default:
        case GeoDataCoordinates::DMS :            
            m_normalLineMap[512000]  = 360 * 6;         //  10'
            m_normalLineMap[1024000] = 360 * 12;        //  5'
            m_normalLineMap[4096000] = 360 * 60;        //  1'
            m_normalLineMap[8192000] = 360 * 60 * 2;    //  30"
            m_normalLineMap[16384000] = 360 * 60 * 6;   //  10"
            m_normalLineMap[65535000] = 360 * 60 * 12;  //  5"
            m_normalLineMap[524288000] = 360 * 60 * 60; //  1"

            m_boldLineMap[512000]     = 360;          // 10'
            m_boldLineMap[1024000]    = 720;          // 5'
            m_boldLineMap[4096000]   = 360 * 6;      // 1'
            m_boldLineMap[8192000]   = 360 * 12;     // 30"
            m_boldLineMap[16384000]  = 360 * 60;     // 10"
            m_boldLineMap[65535000]  = 360 * 60 * 2; // 5"
            m_boldLineMap[524288000] = 360 * 60 * 6; // 1"

        break;
    }
    m_normalLineMap[999999999] = m_normalLineMap.value(262144000);     //  last
    m_boldLineMap[999999999]   = m_boldLineMap.value(262144000);     //  last

    m_currentNotation = notation;
}

}

Q_EXPORT_PLUGIN2(GraticulePlugin, Marble::GraticulePlugin)

#include "GraticulePlugin.moc"
