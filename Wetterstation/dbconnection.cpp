/*
*
* dbconnection.cpp - Stellt die Klasse DBConnection zur verfuegung, welche die Verbindung zur Datenbank
*                    darstellt
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

#include "dbconnection.h"
#include <QTextStream>
#include <QFile>
#include <QDateTime>



DBConnection::DBConnection(){
    verbunden = false;
    initialisiert = false;
    /*
     * Standardparameter:
     * Host: localhost
     * User: wetter
     * Pass: wetter
     * Datenbank: wetterdaten
     */
    dbHost = "localhost";
    dbUser = "wetter";
    dbPass = "wetter";
    dbName = "wetterdaten";
}

DBConnection::~DBConnection(){

}

int DBConnection::connect(){
    int ret = 0;
    //if(!initialisiert)
      //  if(!getVerbindungsParameter())
        //    return -1;
    if(QSqlDatabase::isDriverAvailable("QMYSQL")){
        db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName(dbHost);
        db.setDatabaseName(dbName);
        db.setUserName(dbUser);
        db.setPassword(dbPass);
        if (!db.open()){
            syslog(LOG_ERR,"Verbindung zur Datenbank Fehlgeschlagen");
            ret = -1;
    }
    else{
        verbunden = true;
        syslog(LOG_NOTICE,"Verbindung zur Datenbank hergestellt");
    }
   }else{
        syslog(LOG_ERR,"QMYSQL ist nicht Verfuegbar");
        ret = -2;
    }
    return ret;
}
void DBConnection::close(){
    if(verbunden){
        db.close();
        if(!db.isOpen())
            syslog(LOG_NOTICE,"Verbindung zur Datenbank getrennt");
        else
            syslog(LOG_ERR,"Fehler beim Trennen der Datenbankverbindung");
    }else{
        syslog(LOG_NOTICE,"Verbindung bereits getrennt");
    }
}
void DBConnection::exec(QString befehl){
    if(verbunden){
        QSqlQuery query(db);
        query.prepare(befehl);
        if(query.exec(befehl)){
            while(query.next()){
                qDebug()<<query.value(1).toString();
            };
        }
        else{
            syslog(LOG_ERR,"Fehler in DBConnection::exec()");
        }
    }
}

/**
 *  TODO
 * @return
 */
bool DBConnection::getVerbindungsParameter(){
    qDebug()<<QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)<<"Hole Verbindungsparameter aus 'wetter_db.conf'";
    QFile config("wetter_db.conf");
    QTextStream in(&config);
    QString befehl;
    if(!config.open(QIODevice::ReadOnly)){
        qDebug()<<QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)<<"Fehler beim Oeffnen von 'wetter_db.conf'";
        return false;
    }
    while(!in.atEnd()){
        befehl = in.readLine();
        if(befehl.contains("[Database]")){
            befehl = in.readLine();
             dbName = befehl;
        }
        else if(befehl.contains("[User]")){
            befehl = in.readLine();
            dbUser = befehl;
        }else if(befehl.contains("[Pass]")){
            befehl = in.readLine();
            dbPass = befehl;
        }else if(befehl.contains("[Host]")){
            befehl = in.readLine();
            dbHost = befehl;
        }
    }
    config.close();
    qDebug()<<QDateTime::currentDateTime().toString(Qt::SystemLocaleShortDate)<<"Verbindungsparameter erfolgreich geladen";
    initialisiert = true;
    return true;
}
