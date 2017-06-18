//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2010      Dennis Nienhüser <nienhueser@kde.org>
// Copyright 2012      Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#include "GosmoreRoutingRunner.h"

#include "MarbleDebug.h"
#include "MarbleDirs.h"
#include "routing/Route.h"
#include "routing/RouteRequest.h"
#include "routing/instructions/WaypointParser.h"
#include "routing/instructions/InstructionTransformation.h"

#include <QProcess>
#include <QMap>

namespace Marble
{

class GosmoreRunnerPrivate
{
public:
    QFileInfo m_gosmoreMapFile;

    WaypointParser m_parser;

    /** Static to share the cache among all instances */
    static QMap<QString, QByteArray> m_partialRoutes;

    QByteArray retrieveWaypoints( const QString &query ) const;

    static GeoDataLineString parseGosmoreOutput( const QByteArray &content );

    static void merge( GeoDataLineString* one, const GeoDataLineString& two );

    Route parseGosmoreInstructions(const QByteArray &content);

    GosmoreRunnerPrivate();
};

GosmoreRunnerPrivate::GosmoreRunnerPrivate()
{
    m_parser.setLineSeparator("\r");
    m_parser.setFieldSeparator(QLatin1Char(','));
    m_parser.setFieldIndex( WaypointParser::RoadName, 4 );
    m_parser.addJunctionTypeMapping( "Jr", RoutingWaypoint::Roundabout );
}

QMap<QString, QByteArray> GosmoreRunnerPrivate::m_partialRoutes;

void GosmoreRunnerPrivate::merge( GeoDataLineString* one, const GeoDataLineString& two )
{
    Q_ASSERT( one );

    QVector<GeoDataCoordinates>::const_iterator iter = two.constBegin();
    for( ; iter != two.constEnd(); ++iter ) {
        /** @todo: It might be needed to cut off some points at the start or end */
        one->append( *iter );
    }
}

QByteArray GosmoreRunnerPrivate::retrieveWaypoints( const QString &query ) const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("QUERY_STRING", query);
    env.insert("LC_ALL", "C");
    QProcess gosmore;
    gosmore.setProcessEnvironment(env);

    gosmore.start("gosmore", QStringList() << m_gosmoreMapFile.absoluteFilePath() );
    if (!gosmore.waitForStarted(5000)) {
        mDebug() << "Couldn't start gosmore from the current PATH. Install it to retrieve routing results from gosmore.";
        return QByteArray();
    }

    if ( gosmore.waitForFinished(15000) ) {
        return gosmore.readAllStandardOutput();
    }
    else {
        mDebug() << "Couldn't stop gosmore";
    }

    return QByteArray();
}

GeoDataLineString GosmoreRunnerPrivate::parseGosmoreOutput( const QByteArray &content )
{
    GeoDataLineString routeWaypoints;

    QStringList lines = QString::fromLocal8Bit( content ).split(QLatin1Char('\r'));
    for( const QString &line: lines ) {
        const QStringList fields = line.split(QLatin1Char(','));
        if (fields.size() >= 5) {
            qreal lon = fields.at(1).toDouble();
            qreal lat = fields.at(0).toDouble();
            GeoDataCoordinates coordinates( lon, lat, 0.0, GeoDataCoordinates::Degree );
            routeWaypoints.append( coordinates );
        }
    }

    return routeWaypoints;
}

Route GosmoreRunnerPrivate::parseGosmoreInstructions(const QByteArray &content)
{
    // Determine gosmore version
    QStringList lines = QString::fromUtf8(content).split(QLatin1Char('\r'));
    if ( lines.size() > 2 ) {
        const QStringList fields = lines.at(lines.size()-2).split(QLatin1Char(','));
        m_parser.setFieldIndex( WaypointParser::RoadName, fields.size()-1 );
        if ( fields.size() < 5 || fields.size() > 6 ) {
            // Can happen when gosmore changes the output format, returns garbage
            // or the last street name contains a comma. We may still parse it correctly, just try.
            mDebug() << "Unexpected number of fields. This gosmore version may be unsupported.";
        }
    }

    Route result;
    QTextStream stream( content );
    stream.setCodec("UTF8");
    stream.setAutoDetectUnicode( true );

    RoutingInstructions directions = InstructionTransformation::process( m_parser.parse( stream ) );
    for( int i=0; i<directions.size(); ++i ) {
        Maneuver maneuver;
        maneuver.setInstructionText( directions[i].instructionText() );
        maneuver.setDirection( static_cast<Maneuver::Direction>( directions[i].turnType() ) );
        maneuver.setRoadName( directions[i].roadName() );
        RouteSegment segment;
        segment.setManeuver( maneuver );
        Q_ASSERT( !directions[i].points().isEmpty() );
        GeoDataLineString path;
        QVector<RoutingWaypoint> items = directions[i].points();
        for (int j=0; j<items.size(); ++j ) {
            RoutingPoint point = items[j].point();
            GeoDataCoordinates coordinates( point.lon(), point.lat(), 0.0, GeoDataCoordinates::Degree );
            path.append( coordinates );
        }
        segment.setPath( path );
        result.addRouteSegment( segment );
    }

    return result;
}

GosmoreRunner::GosmoreRunner( QObject *parent ) :
        RoutingRunner( parent ),
        d( new GosmoreRunnerPrivate )
{
    // Check installation
    QDir mapDir(MarbleDirs::localPath() + QLatin1String("/maps/earth/gosmore/"));
    d->m_gosmoreMapFile = QFileInfo ( mapDir, "gosmore.pak" );
}

GosmoreRunner::~GosmoreRunner()
{
    delete d;
}

void GosmoreRunner::retrieveRoute( const RouteRequest *route )
{
    if ( !d->m_gosmoreMapFile.exists() )
    {
        emit routeCalculated( Route() );
        return;
    }

    GeoDataLineString* wayPoints = new GeoDataLineString;
    QByteArray completeOutput;

    for( int i=0; i<route->size()-1; ++i )
    {
        QString queryString = "flat=%1&flon=%2&tlat=%3&tlon=%4&fastest=1&v=motorcar";
        GeoDataCoordinates source = route->at(i);
        double fLon = source.longitude( GeoDataCoordinates::Degree );
        double fLat = source.latitude( GeoDataCoordinates::Degree );
        queryString = queryString.arg(fLat, 0, 'f', 8).arg(fLon, 0, 'f', 8);
        GeoDataCoordinates destination = route->at(i+1);
        double tLon = destination.longitude( GeoDataCoordinates::Degree );
        double tLat = destination.latitude( GeoDataCoordinates::Degree );
        queryString = queryString.arg(tLat, 0, 'f', 8).arg(tLon, 0, 'f', 8);

        QByteArray output;
        if ( d->m_partialRoutes.contains( queryString ) ) {
            output = d->m_partialRoutes[queryString];
        }
        else {
            output = d->retrieveWaypoints( queryString );
        }

        GeoDataLineString points = d->parseGosmoreOutput( output );
        d->merge( wayPoints, points );
        completeOutput.append( output );
    }

    const Route result = d->parseGosmoreInstructions( completeOutput );

    emit routeCalculated( result );
}

} // namespace Marble
