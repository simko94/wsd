/*
*
* dbconnection.h - Headerfile fuer die Klasse DBConnection
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

#ifndef __DBCONNECTION_H__
#define __DBCONNECTION_H__

#include <syslog.h>
#include <QtSql>
#include <QString>
class DBConnection{
public:
    DBConnection();
    ~DBConnection();
    int connect();
    void close();
    void exec(QString);
private:
    QSqlDatabase db;
    bool verbunden;
    bool getVerbindungsParameter();
    QString dbHost;
    QString dbName;
    QString dbUser;
    QString dbPass;
    bool initialisiert;
};


#endif // __DBConnection_H__
