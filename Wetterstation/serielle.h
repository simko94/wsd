/*
*
* serielle.h - Headerfile fuer die Klasse Serielle
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

#ifndef SERIELLE_H
#define SERIELLE_H
//
#include <termios.h>
#include <syslog.h>
#include "buildinformation.h"
//
#include <QObject>
#include <QString>
#include <QSocketNotifier>

//
class Serielle : public QObject
{
        Q_OBJECT
        public:
                enum status
                {
                        OPENERROR,
                        READERROR,
                        WRITEERROR,
                        ERROR
                };
                Serielle(QObject *eltern=0);
                virtual ~Serielle();
                bool isAktiv();
                int Start (QString = "/dev/ttyUSB0");
                static bool exist();
        private:
                int port;
                bool aktiv;
                QString Zeile;
                struct termios ori;
                QSocketNotifier *lesen, *fehler;
                static bool existiert;
                BuildInformation *info;
        private slots:
                void Zeichen_da (int);
                void ereignis(int);
        signals:
                // Wenn Daten vorhanden sind
                void ByteReceived(char);
};
#endif
