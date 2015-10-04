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

#include "osm/OsmPresetLibrary.h"
#include "osm/OsmObjectManager.h"
#include <GeoDataPlacemark.h>

namespace Marble {

void OsmNode::parseCoordinates(const QXmlStreamAttributes &attributes)
{
    qreal const lon = attributes.value( "lon" ).toDouble();
    qreal const lat = attributes.value( "lat" ).toDouble();
    m_coordinates = GeoDataCoordinates(lon, lat, 0, GeoDataCoordinates::Degree);
}

void OsmNode::create(GeoDataDocument *document) const
{
    GeoDataFeature::GeoDataVisualCategory const category = OsmPresetLibrary::determineVisualCategory(m_osmData);
    if (category == GeoDataFeature::None ||
       (category >= GeoDataFeature::HighwaySteps && category <= GeoDataFeature::HighwayMotorway)) {
        return;
    }

    GeoDataPlacemark* placemark = new GeoDataPlacemark;
    placemark->setOsmData(m_osmData);
    placemark->setCoordinate(m_coordinates);

    placemark->setName(m_osmData.tagValue("name"));
    placemark->setZoomLevel( 18 );
    placemark->setVisualCategory(category);
    placemark->setStyle( 0 );

    OsmObjectManager::registerId(m_osmData.id());
    document->append(placemark);
}

GeoDataCoordinates OsmNode::coordinates() const
{
    return m_coordinates;
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