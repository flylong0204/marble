//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2012      Dennis Nienhüser <nienhueser@kde.org>
//


#ifndef MARBLE_OSMMAPQUESTRUNNER_H
#define MARBLE_OSMMAPQUESTRUNNER_H

#include "RoutingRunner.h"

#include "routing/Maneuver.h"
#include "routing/Route.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Marble
{

class MapQuestRunner : public RoutingRunner
{
    Q_OBJECT

public:
    explicit MapQuestRunner(QObject *parent = 0);

    ~MapQuestRunner() override;

    // Overriding MarbleAbstractRunner
    void retrieveRoute( const RouteRequest *request ) override;

private Q_SLOTS:
    void get();

    /** Route data was retrieved via http */
    void retrieveData( QNetworkReply *reply );

    /** A network error occurred */
    void handleError( QNetworkReply::NetworkError );

private:
    static void append( QString* input, const QString &key, const QString &value );

    static Maneuver::Direction maneuverType(int mapQuestId);

    Route parse(const QByteArray &input) const;

    QNetworkAccessManager m_networkAccessManager;

    QNetworkRequest m_request;
};

}

#endif
