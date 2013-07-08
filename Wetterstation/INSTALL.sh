#!/bin/sh
##
#
# INSTALL.sh - Installationsskript fuer das Softwarepaket
#
# Copyright (C) 2013 Simon Kofler
#
# This program is free software;
# you can redistribute it and/or modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation;
# either version 3 of the License, or (at your option) any later version.
# This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with this program;
#
# if not, see <http://www.gnu.org/licenses/>.
#
#
# PLEASE RUN THIS SCRIPT AS A SUPER-USER
#
#
##

if [ `id -u` -ne "0" ]
then
	echo "Please run this script as a super-user"
	exit 0
fi
DBUSER="root"
echo "Bitte geben Sie das Password fuer den Benutzer root der MySQL-Datenbank ein:"
read DBPASS
# QMake anweisen das Makefile zu erstellen
qmake ./Wetterstation.pro -r -spec linux-g++ CONFIG+=debug
# Projekt compilieren
make -r -w
# Ausführbare Dateien nach /usr/bin kopieren
cp Wetterstation /usr/bin
# Init-Script nach /etc/init.d kopieren
cp Wetterstation_INIT /etc/init.d/Wetterstation
# Dienst registrieren
update-rc.d Wetterstation defaults
# Datenbanken anlegen
echo "CREATE DATABASE wetterdaten; CREATE DATABASE wetterdaten_users" | mysql -u$DBUSER -p$DBPASS
# Tabellen anlegen
echo "CREATE TABLE sensoren(
  snummer int NOT NULL PRIMARY KEY AUTO_INCREMENT,
  sname varchar(200) NOT NULL,
  sonline bool NOT NULL DEFAULT '0',
  sbatterieschwach bool NOT NULL DEFAULT '0'
)ENGINE=InnoDB CHARSET=latin1 COLLATE=latin1_german1_ci;
CREATE TABLE messtypen(
  mnummer int NOT NULL PRIMARY KEY AUTO_INCREMENT,
  snummer int NOT NULL,
  mbeschreibung varchar(200) NOT NULL,
  KEY snummer (snummer),
  FOREIGN KEY(snummer) REFERENCES sensoren(snummer)
    ON UPDATE RESTRICT ON DELETE RESTRICT
)ENGINE=InnoDB CHARSET=latin1 COLLATE=latin1_german1_ci;
CREATE TABLE wetterdaten(
  wnummer int NOT NULL PRIMARY KEY AUTO_INCREMENT,
  wwert decimal(4,1) NOT NULL,
  mnummer int(11) NOT NULL,
  wzeitpunkt timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  KEY mnummer(mnummer),
  FOREIGN KEY(mnummer) REFERENCES messtypen(mnummer)
    ON UPDATE RESTRICT ON DELETE RESTRICT
) ENGINE=InnoDB CHARSET=latin1 COLLATE=latin1_german1_ci;" | mysql -u$DBUSER -p$DBPASS wetterdaten
# User-Tabelle anlegen
echo "CREATE TABLE benutzer(
  username varchar(200) NOT NULL PRIMARY KEY,
  password varchar(32) NOT NULL,
  lang varchar(4) DEFAULT 'en',
  email varchar(200) NOT NULL,
)Engine=InnoDB CHARSET=latin1 COLLATE=latin1_german1_ci;" | mysql -u$DBUSER -p$DBPASS wetterdaten_users
# Datenbankbenutzer anlegen
echo "CREATE USER wetter@localhost IDENTIFIED BY 'wetter';
GRANT SELECT,UPDATE,INSERT ON wetterdaten.* to wetter@localhost;
GRANT SELECT,UPDATE,INSERT ON wetterdaten_users.* to wetter@localhost;" | mysql -u$DBUSER -p$DBPASS
exit 1
