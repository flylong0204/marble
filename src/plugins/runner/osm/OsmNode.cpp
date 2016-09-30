//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2015      Dennis Nienhüser <nienhueser@kde.org>
//

#include <OsmNode.h>

#include "osm/OsmObjectManager.h"
#include <GeoDataPlacemark.h>
#include <GeoDataStyle.h>
#include <GeoDataIconStyle.h>
#include <GeoDataDocument.h>
#include <MarbleDirs.h>
#include <StyleBuilder.h>

#include <QXmlStreamAttributes>

namespace Marble {

void OsmNode::parseCoordinates(const QXmlStreamAttributes &attributes)
{
    static const QString latKey = QLatin1String("lat");
    static const QString lonKey = QLatin1String("lon");

    const QString lonValue = attributes.value(lonKey).toString();
    const QString latValue = attributes.value(latKey).toString();

    m_osmData.insertTag(latKey, latValue);
    m_osmData.insertTag(lonKey, lonValue);
}

void OsmNode::setCoordinates(const GeoDataCoordinates &coordinates)
{
    static const QString latKey = QLatin1String("lat");
    static const QString lonKey = QLatin1String("lon");

    const QString latValue = QString::number(coordinates.latitude(GeoDataCoordinates::Degree), 'f', 7);
    const QString lonValue = QString::number(coordinates.longitude(GeoDataCoordinates::Degree), 'f', 7);

    m_osmData.insertTag(latKey, latValue);
    m_osmData.insertTag(lonKey, lonValue);
}

void OsmNode::create(GeoDataDocument *document) const
{
    GeoDataFeature::GeoDataVisualCategory const category = StyleBuilder::determineVisualCategory(m_osmData);

    if (category == GeoDataFeature::None) {
        return;
    }

    GeoDataPlacemark* placemark = new GeoDataPlacemark;
    placemark->setOsmData(m_osmData);
    placemark->setCoordinate(coordinates());

    QHash<QString, QString>::const_iterator tagIter;
    if ((category == GeoDataFeature::TransportCarShare || category == GeoDataFeature::MoneyAtm)
            && (tagIter = m_osmData.findTag(QStringLiteral("operator"))) != m_osmData.tagsEnd()) {
        placemark->setName(tagIter.value());
    } else {
        placemark->setName(m_osmData.tagValue(QStringLiteral("name")));
    }
    if (placemark->name().isEmpty()) {
        placemark->setName(m_osmData.tagValue(QStringLiteral("ref")));
    }
    placemark->setVisualCategory(category);
    placemark->setStyle( GeoDataStyle::Ptr() );

    placemark->setZoomLevel( 18 );
    if (category >= GeoDataFeature::PlaceCity && category <= GeoDataFeature::PlaceVillageCapital) {
        int const population = m_osmData.tagValue(QStringLiteral("population")).toInt();
        placemark->setPopulation(qMax(0, population));
        if (population > 0) {
            placemark->setZoomLevel(populationIndex(population));
            placemark->setPopularity(population);
        } else {
            switch (category) {
            case GeoDataFeature::PlaceCity:
            case GeoDataFeature::PlaceCityCapital:
                placemark->setZoomLevel(9);
                break;
            case GeoDataFeature::PlaceSuburb:
                placemark->setZoomLevel(13);
                break;
            case GeoDataFeature::PlaceHamlet:
                placemark->setZoomLevel(15);
                break;
            case GeoDataFeature::PlaceLocality:
                placemark->setZoomLevel(15);
                break;
            case GeoDataFeature::PlaceTown:
            case GeoDataFeature::PlaceTownCapital:
                placemark->setZoomLevel(11);
                break;
            case GeoDataFeature::PlaceVillage:
            case GeoDataFeature::PlaceVillageCapital:
                placemark->setZoomLevel(13);
                break;
            default:
                placemark->setZoomLevel(10); break;
            }
        }
    }

    OsmObjectManager::registerId(m_osmData.id());
    document->append(placemark);
}

int OsmNode::populationIndex(qint64 population) const
{
    int popidx = 3;

    if ( population < 2500 )        popidx=10;
    else if ( population < 5000)    popidx=9;
    else if ( population < 25000)   popidx=8;
    else if ( population < 75000)   popidx=7;
    else if ( population < 250000)  popidx=6;
    else if ( population < 750000)  popidx=5;
    else if ( population < 2500000) popidx=4;

    return popidx;
}

const GeoDataCoordinates OsmNode::coordinates() const
{
    if (!m_osmData.containsTagKey("lon") || !m_osmData.containsTagKey("lat")) {
        return GeoDataCoordinates();
    }

    const qreal lon = m_osmData.tagValue(QLatin1String("lon")).toDouble();
    const qreal lat = m_osmData.tagValue(QLatin1String("lat")).toDouble();

    return GeoDataCoordinates(lon, lat, 0, GeoDataCoordinates::Degree);
}

OsmPlacemarkData &OsmNode::osmData()
{
    return m_osmData;
}

const OsmPlacemarkData &OsmNode::osmData() const
{
    return m_osmData;
}

}
