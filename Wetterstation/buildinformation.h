/*
*
* buildinformation.h - Headerfile fuer die Klasse BuildInformation
*
* Copyright (C) 2012 Simon Kofler
*
* This program is free software;
* you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation;
* either version 3 of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License along with this program;
*
* if not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef BUILDINFORMATION_H
#define BUILDINFORMATION_H
#include <QObject>
#include <syslog.h>
#include "dbconnection.h"

#define MINUTE_PACKET_LENGTH 2
#define DATE_PACKET_LENGTH 6
#define WIND_PACKET_LENGTH 8
#define RAIN_PACKET_LENGTH 13
#define TEMPERATURE_PACKET_LENGTH 6

class BuildInformation : public QObject
{
    Q_OBJECT
public:
    BuildInformation(QObject *eltern = 0);
    virtual ~BuildInformation();
private:
    enum{
                        NICHTS,
                        // Das erste Header-Byte ist da
                        HEAD1,
                        // Das zweite Header-Byte ist da
                        HEAD2,
                        // Die Daten welche folgen sind Informationen
                        // zur Minute
                        MINUTEN,
                        DATUM,
                        WIND,
                        REGEN,
                        TEMPERATUR
                }status;
    static int verbleibendeBytes;
    void buildMinutePackage(char);
    void buildDatePackage(char);
    void buildWindPackage(char);
    void buildRainPackage(char);
    void buildTemperaturePackage(char);
    DBConnection *d;
public slots:
    void data_received(char);
};

#endif // BUILDINFORMATION_H
