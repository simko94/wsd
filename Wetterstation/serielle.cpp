/*
*
* serielle.cpp - Stellt die Klasse Serielle zur verfuegung, welche die serielle Schnittstelle konfigurieren,
*                eine Verbindung mit dieser herstellen und Daten von dieser lesen kann
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

#include "serielle.h"
#include "buildinformation.h"
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
//
#include <QObject>
#include <QString>
#include <QSocketNotifier>
#include <QDateTime>

//
bool Serielle::existiert=false;
Serielle::Serielle(QObject *eltern)
        : QObject(eltern)
{
        port = -1;
        aktiv = false;
        existiert=true;
}

//
Serielle::~Serielle()
{
        if (aktiv)
        {
                disconnect (lesen, SIGNAL(activated(int)), this, SLOT(Zeichen_da (int)));
                disconnect (fehler, SIGNAL(activated(int)), this, SLOT(ereignis(int)));
                disconnect(this,SIGNAL(ByteReceived(char)),info,SLOT(data_received(char)));
                delete lesen;
                delete fehler;
                delete info;
                tcsetattr (port, TCSANOW, &ori);
                close(port);
        }
        existiert=false;
}

//
bool Serielle::isAktiv()
{
        return aktiv;
}

/**
 * Diese Funktion startet die serielle Kommunikation mit 9600 BAUD mit dem
 * Geraet mit dem uebergebenen Namen (standard: /dev/ttyUSB0)
 * Diese Funktion wartet auf das Signal activated(int), welches von QSocketNotifyer
 * ausgeloest wird
 * @return
 *      0: Oeffnen der Verbindung erfolgreich
 *     -1: Fehler beim Oeffnen der Verbindung
 */
int Serielle::Start (QString portname)
{
        struct termios konf;
        if ((port= open (portname.toStdString().c_str(), O_RDWR|O_NOCTTY|O_NDELAY)) < 0)
        {
                syslog(LOG_ERR,"Fehler beim Oeffnen der Seriellen Schnittstelle");
                return -1;
        }
        aktiv = true;
        tcgetattr (port, &ori);
        bzero (&konf, sizeof(konf));
        konf.c_lflag &= ~ICANON;
        konf.c_lflag &= ~(ECHO|ECHOCTL|ECHONL);
        konf.c_lflag |= HUPCL;
        konf.c_cc[VMIN]=1;
        konf.c_cc[VTIME] = 0;
        konf.c_oflag &= ~ONLCR;
        konf.c_iflag &= ~ICRNL;
        cfsetospeed(&konf, B9600);
        cfsetispeed(&konf, B9600);
        tcsetattr (port, TCSANOW, &konf);
        info = new BuildInformation();
        lesen = new QSocketNotifier(port, QSocketNotifier::Read);
        fehler = new QSocketNotifier(port, QSocketNotifier::Exception);
        connect(lesen, SIGNAL(activated(int)), this, SLOT(Zeichen_da (int)));
        connect(fehler, SIGNAL(activated(int)), this, SLOT(ereignis(int)));
        connect(this,SIGNAL(ByteReceived(char)),info,SLOT(data_received(char)));
        syslog(LOG_NOTICE,"Verbindung Hergestellt");
        return 0;
}

//
//
void Serielle::Zeichen_da(int)
{
        char zeichen;
        read(port, &zeichen, 1);
        // Loest das Signal aus, welches von BuildInformation empfangen wird
        // Die Information wird als ein Byte weitergeleitet
        emit ByteReceived(zeichen);
}

//
void Serielle::ereignis(int)
{
    syslog(LOG_ERR,"Fehler bei der seriellen Uebertragung");
}

//
bool Serielle::exist(){
        return existiert;
}
