/*
*
* Wetterstation.cpp - Das Programm Wetterstation liest ueber die serielle Schnittstelle die von der Wetterstation
*                     gesammelten Werte ein, verarbeitet diese und schreibt die Ergebnisse anschliessend in eine Datenbank
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

#include <QCoreApplication>
#include "serielle.h"
#include <iostream>
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

void destruct(int);
void start_daemon();

Serielle *s;

int main(int argc, char ** argv)
{
        start_daemon();
        // Verbindung zum Syslog-Dämon herstellen
        openlog("Wetterstation",LOG_CONS|LOG_NDELAY,LOG_LOCAL0);
        syslog(LOG_NOTICE,"Wetterstation Daemon start\n");
        QCoreApplication app( argc, argv );
        // Registrieren der Ereignisbehandlungsmethode
        signal(SIGINT,destruct);
        s = new Serielle();
        // Wenn ein Fehler bei der seriellen Verbindung auftritt, terminiert das Programm
        if(s->Start()==-1)
            return -1;
        return app.exec();
}

/**
 * Startet den Prozess als Daemon, indem die Methode
 * den aktuellen Prozess klont und den Elternprozess terminieren laesst.
 * Der Kindprozess geht nun zum init-Prozess ueber.
 * Ausserdem werden alle Streams geschlossen, sodass keine
 * Ausgaben mehr nach stdout usw geschrieben werden
 */
void start_daemon()
{
 int i;
 /*
  * Fork liefert eine Zahl verschieden von 0 zurueck,
  * falls es sich bei dem Prozess um den Elternprozess handelt.
  * Ist dies der Fall, so terminiert der Elternprozess sofort.
  * Das Kind wird nach der Terminierung des Elternprozesses vom System
  * an den init-Prozess uebergeben.
  */
 if(fork()!=0)
     exit(0);
 /*
  * Kindprozess uebernimmt Session
  */
 if(setsid() == -1)
 {
     syslog(LOG_ERR,"Fehler bei setsid()");
     exit(0);
 }
 // Rechtestruktur setzen
 umask(0);
 // Alle Streams schliessen
 for(i=sysconf(_SC_OPEN_MAX); i>=0; i--)
     close(i);
}


/**
 * Diese Ereignisbehandlungsmethode wird fuer das Keyboardinterrupt SIGINT registriert, welches bei STRG+C
 * ausgeloest wird
 * @param signal
 */
void destruct(int signal){
    // Loese Destruktoren aus
    delete s;
    // Verlasse Programm
    exit(signal);
}
