/*
*
* buildinformation.cpp - Stellt die Klasse BuildInformation zur verfuegung, welche empfangene Datenbytes
*                        zu Informationspaketen verarbeitet
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

// ALLE 2 MINUTEN WERDEN NEUE WERTE IN DIE DATENBANK EINGETRAGEN
#define TIME_INTERVAL 120000
#include "buildinformation.h"
#include "dbconnection.h"
#include <iostream>
#include <QObject>
#include <QDateTime>
#include <QList>
#include <QDateTime>

int BuildInformation::verbleibendeBytes = 0;

BuildInformation::BuildInformation(QObject *eltern) : QObject(eltern)
{
    status=NICHTS;
    d = new DBConnection();
    d->connect();
    d->exec("UPDATE sensoren SET sonline=0;");
}

BuildInformation::~BuildInformation(){
    // In der Datenbank wird der Status aller Sensoren auf offline gesetzt
    d->exec("UPDATE sensoren SET sonline=0;");
    delete d;
}

/**
 * Diese Methode reagiert auf die Signale, welche ausgeloest werden, wenn an der Seriellen Schnittstelle
 * ein Byte empfangen wurde.
 *
 * Die Routine funktioniert als Automat, welcher folgende internen Zustaende besitzt:
 *      NICHTS:     Es wurden keine Daten empfangen
 *      HEAD1:      Es wurde der erste Teil des Headers empfangen
 *      HEAD2:      Es wurde der zweite Teil des Headers empfangen
 *      WIND:       Es werden Windinformationen empfangen
 *      REGEN:      Es werden Regeninformationen empfangen
 *      TEMPERATUR: Es werden Temperaturdaten und Luftfeuchtigkeitsdaten empfangen
 *      MINUTEN:    Es werden die neuen Minutenziffern empfangen
 *      DATUM:      Es werden die neuen Stundenziffern und das Datum empfangen
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::data_received(char byte){
    switch(status){
        case NICHTS:
            // Headerinformationen wurden empfangen
            if(byte==char(0xFF))
                status=HEAD1;
        break;
        // Der erste Teil des Headers wurde empfangen
        case HEAD1:
            if(byte==char(0xFF))
                // Der Header wurde vollstaendig empfangen
                status=HEAD2;
            else
                status=NICHTS;
        break;
        case HEAD2:
            /*
             * Der Header wurde vollstaendig empfangen. Nun wird der Typ der
             * folgenden Pakete ermittelt.
             *
             * Dabei kann es sich um folgende Typen handeln:
             *      Winddaten
             *      Regendaten
             *      Temperatur und Luftfeuchtigkeitsdaten
             *      Zeit (Minuten)
             *      Stunden und Datum
             */
            switch(byte){
                // Windrichtung und Windgeschwindigkeit folgen
                case char(0x00):
                    status=WIND;
                    verbleibendeBytes=WIND_PACKET_LENGTH;
                break;
                // Regenmenge folgt
                case char(0x01):
                    status=REGEN;
                    verbleibendeBytes=RAIN_PACKET_LENGTH;
                break;
                // Temperatur und Luftfeuchtigkeit folgen
                case char(0x03):
                    status=TEMPERATUR;
                    verbleibendeBytes=TEMPERATURE_PACKET_LENGTH;
                break;
                // Minuten folgen
                case char(0x0E):
                    status=MINUTEN;
                    verbleibendeBytes=MINUTE_PACKET_LENGTH;
                break;
                // Stunde und Datum folgt
                case char(0x0F):
                    status=DATUM;
                    verbleibendeBytes=DATE_PACKET_LENGTH;
                break;
                // Unbekannter Sensor
                default:
                    status=NICHTS;
            }
        break;
        // Es werden aktuell Winddaten empfangen
        case WIND:
            if(byte==char(0xFF)){
                status=HEAD1;
                break;
            }
            buildWindPackage(byte);
        break;
        // Es werden aktuell Regendaten empfangen
        case REGEN:
            if(byte==char(0xFF)){
                status=HEAD1;
                break;
            }
            buildRainPackage(byte);
        break;
        // Es werden aktuell Temperaturdaten empfangen
        case TEMPERATUR:
            if(byte==char(0xFF)){
                status=HEAD1;
                break;
            }
           buildTemperaturePackage(byte);
        break;
        // Es werden aktuell Minutenziffern empfangen
        case MINUTEN:
            if(byte==char(0xFF)){
                status=HEAD1;
                break;
            }
            buildMinutePackage(byte);
        break;
        // Es werden aktuell Stundenziffern und Datum empfangen
        case DATUM:
            if(byte==char(0xFF)){
                status=HEAD1;
                break;
            }
            buildDatePackage(byte);
        break;
        default:
            status=NICHTS;
    }

}

/**
 * Diese Methode sammelt alle einzelnen Bytes bis das vollstaendige Paket empfangen wurde
 * und wertet es anschliessend aus.
 *
 * Wenn das Paket vollstaendig empfangen wurde wird eine Datenstruktur mit den aufbereiteten
 * empfangenen Werten zurueckgegeben
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::buildMinutePackage(char byte){
    /*
     * Ein Minuten-Paket hat folgenden Inhalt:
     * data[0]: 1. Datenbyte: Minutenziffern, Batterie der Basisstation schwach
     * data[1]: Checksumme
     */
    static char data[MINUTE_PACKET_LENGTH];
    data[MINUTE_PACKET_LENGTH-verbleibendeBytes]=byte;
    verbleibendeBytes--;
    if(verbleibendeBytes==0){
        // Paket wurde vollstaendig empfangen
        std::cout<<"Minute Empfangen"<<std::endl;
        /*
         * Das erste Datenbyte ist wie folgt aufgebaut (von Hoeherwertigen zu niederwertigeren Bits)
         * BATTERY LOW | 3 Bits = 10er-Stelle der Minute | 4 Bits = 1er-Stelle der Minute
         */
        /*
         * Die unteren 4 Bits stellen die 1er-Stelle der Minute dar.
         */
        float min1=(data[0]&0x0F);
        /*
         * Die oberen 3 Bits stellen die 10er-Stelle der Minute dar
         */
        float min10=(data[0]&0x70)>>4;
        float min = 10*min10 + min1;
        std::cout<<min<<std::endl;
        /*
         * Das oberste Bit beschreibt den Status der Batterie der Basisstation
         *
         *  1 = Batterie schwach
         *  0 = Batterie ausreichend
         *
         * Es wird herausmaskiert und um 7 stellen heruntergeshiftet
         */
        float battery = (data[0]&0x80)>>7;
        if(battery==1)
            syslog(LOG_WARNING,"Batterie der Basisstation schwach");
        status=NICHTS;
    }
}

/**
 * Diese Methode sammelt alle einzelnen Bytes bis das vollstaendige Paket empfangen wurde
 * und wertet es anschliessend aus.
 *
 * Wenn das Paket vollstaendig empfangen wurde wird eine Datenstruktur mit den aufbereiteten
 * empfangenen Werten zurueckgegeben
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::buildDatePackage(char byte){
    static char data[DATE_PACKET_LENGTH];
    data[DATE_PACKET_LENGTH-verbleibendeBytes]=byte;
    verbleibendeBytes--;
    if(verbleibendeBytes==0){
        // Paket wurde vollstaendig empfangen
        status=NICHTS;
    }
}

/**
 * Diese Methode sammelt alle einzelnen Bytes bis das vollstaendige Paket empfangen wurde
 * und wertet es anschliessend aus.
 *
 * Wenn das Paket vollstaendig empfangen wurde wird eine Datenstruktur mit den aufbereiteten
 * empfangenen Werten zurueckgegeben
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::buildTemperaturePackage(char byte){
    /*
     * Ein Temperatur-Paket hat folgenden Inhalt:
     * data[0]: 1. Datenbyte: Taupunkt-Temperatur ausser dem Wertebereich, Batterie des Temperatursensors schwach
     * data[1]: 2. Datenbyte: Temperatur in Dezigraden
     * data[2]: 3. Datenbyte: Temperatur in Dekagraden, Temperatur ausser Wertebereich, Vorzeichen 1=negativ 0=positiv
     * data[3]: 4. Datenbyte: Luftfeuchtigkeit in Prozent
     * data[4]: 5. Datenbyte: Taupunkt-Temperatur
     * data[5]: 6. Datenbyte: Checksumme
     */
    static char data[TEMPERATURE_PACKET_LENGTH];
    data[TEMPERATURE_PACKET_LENGTH-verbleibendeBytes]=byte;
    verbleibendeBytes--;
    if(verbleibendeBytes==0){
        static QDateTime begin = QDateTime::currentDateTime();
        /*
         *
         * Folgende Listen sammeln alle Daten, die während der definierten Zeitspanne
         * anfallen.
         * Ist das Zeitintervall abgelaufen, wird der Mittelwert dieser Daten
         * in die Datenbank geschrieben
         */
        static QList<float> templist;
        static QList<float> dewlist;
        static QList<float> humlist;
        /*
         * Das 6. Bit des ersten Datenbytes beschreibt den Batteriestand des Sensors (0=OK, 1=schwach)
         */
       char battery_low =(data[0]&0x40)>>6;
       QString sql = QString("UPDATE sensoren SET sonline=1 AND sbatterieschwach=\"%1\" WHERE sname=\"Temperatursensor\"").arg(battery_low);
       d->exec(sql);
        /*
         * Das 2. Datenbyte ist wie folgt aufgebaut:
         *
         * Die oberen 4 Bits stellen die 1er-Stelle der Temperatur dar.
         */
        float temp01=(data[1]&0x0F);
        float temp1=(data[1]&0xF0)>>4;
        /*
         * Das 3. Datenbyte ist wie folgt aufgebaut:
         *
         * Das oberste Bit beschreibt das Vorzeichen, das zweithoechste Bit gibt an, ob sich der Wert
         * ausserhalb des messbaren Bereichs befindet.
         *
         * Das zwei niedrigsten Bits der oberen Gruppe beschreiben die 100er-Stelle der Temperatur
         *
         * Die unteren 4 Bits beschreiben die 10er-Stelle der Temperatur
         */
        float temp10=(data[2]&0x0F);
        float temp100=(data[2]&0x30)>>4;
        float temp = 100*temp100 + 10*temp10 + temp1 + temp01/10;
        // Wenn das Oberste Bit 1 ist, ist das Vorzeichen der Temperatur negativ
        if(((data[2]&0x80)>>7)==1)
            temp *=-1;
        float hum1 = (data[3]&0x0F);
        float hum10 = (data[3]&0xF0)>>4;
        float hum = 10*hum10 + hum1;
        float dew1 = (data[4]&0x0F);
        float dew10 = (data[4]&0xF0)>>4;
        float dew = 10*dew10 + dew1;
        std::cout<<"Temperatur: "<<temp<<"°C"<<std::endl;
        std::cout<<"Relative Luftfeuchtigkeit: "<<hum<<"%"<<std::endl;
        std::cout<<"Taupunkt liegt bei "<<dew<<"°C"<<std::endl;

        humlist<<hum;
        templist<<temp;
        dewlist<<dew;
        // Wenn das Zeitintervall abgelaufen ist, werden die Mittelwerte der Listen eingetragen
        if((QDateTime::currentMSecsSinceEpoch()-TIME_INTERVAL) > begin.toMSecsSinceEpoch()){
            QListIterator<float> itemp(templist);
            QListIterator<float> idew(dewlist);
            QListIterator<float> ihum(humlist);
            float sum = 0;
            // Mittelwert der gesammelten Temperaturen rechnen
            while(itemp.hasNext()){
                sum+=itemp.next();
            }
            temp = sum/templist.count();
            sum = 0;
            // Mittelwert der gesammelten Taupunkttemperaturen rechnen
            while(ihum.hasNext()){
                sum+=ihum.next();
            }
            // Mittelwert der gesammelten Luftfeuchtigkeitswerte rechnen
            hum = sum/humlist.count();
            sum = 0;
            while(idew.hasNext()){
                sum+=idew.next();
            }
            dew = sum/dewlist.count();
            sum = 0;
            sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,1)").arg(temp);
            d->exec(sql);
            sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,2)").arg(hum);
            d->exec(sql);
            sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,3)").arg(dew);
            d->exec(sql);
            templist.clear();
            humlist.clear();
            dewlist.clear();
            begin = QDateTime::currentDateTime();
        }
        // Paket wurde vollstaendig empfangen
        status=NICHTS;
    }
}

/**
 * Diese Methode sammelt alle einzelnen Bytes bis das vollstaendige Paket empfangen wurde
 * und wertet es anschliessend aus.
 *
 * Wenn das Paket vollstaendig empfangen wurde wird eine Datenstruktur mit den aufbereiteten
 * empfangenen Werten zurueckgegeben
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::buildWindPackage(char byte){
    /*
     * Ein Wind-Paket hat folgenden Inhalt:
     * data[0]: 1. Datenbyte: Windgeschwindigkeit ausser Wertebereich, Durchschnittl. Windgeschw. ausser Wertebereich,
     *             Batterie des Sensors schwach
     * data[1]: 2. Datenbyte: Windrichtung 1er-Stelle und 10er-Stelle
     * data[2]: 3. Datenbyte: Windrichtung 100er-Stelle, Windgeschwindigkeit 0.1er-Stelle
     * data[3]: 4. Datenbyte: Windgeschwindigkeit 1er-Stelle und 10er-Stelle
     * data[4]: 5. Datenbyte: Durchschnittliche Windgeschwindigkeit 0.1er-Stelle und 1er-Stelle
     * data[5]: 6. Datenbyte: Durchschnittliche Windgeschwindigkeit 10er-Stelle, Chill no data?, Chill over?, Vorzeichen
     * data[6]: 7. Datenbyte: Gefuehlte Temperatur 1er-Stelle? und Gefuehlte Temperatur 10er-Stelle
     */
    static char data[WIND_PACKET_LENGTH];
    data[WIND_PACKET_LENGTH-verbleibendeBytes]=byte;
    verbleibendeBytes--;
    if(verbleibendeBytes==0){
        static QDateTime begin = QDateTime::currentDateTime();
        /*
         *
         * Folgende Listen sammeln alle Daten, die während der definierten Zeitspanne
         * anfallen.
         * Ist das Zeitintervall abgelaufen, wird der Mittelwert dieser Daten
         * in die Datenbank geschrieben
         */
        static QList<float> dirlist;
        static QList<float> speedlist;
        static QList<float> avglist;
        static QList<float> chilllist;
        /*
         * Das 6. Bit des ersten Datenbytes beschreibt den Batteriestand des Sensors (0=OK, 1=schwach)
         */
        char battery_low =(data[0]&0x40)>>6;
        QString sql = QString("UPDATE sensoren SET sonline=1 AND sbatterieschwach=\"%1\" WHERE sname=\"Windsensor\"").arg(battery_low);
        d->exec(sql);
        /*
         * speed_over gibt an, ob sich die aktuelle Windgeschwindigkeit ausser dem
         * Wertebereich des Sensors befindet
         *
         * 0: OK
         * 1: Wert ausser Wertebereich
         */
        float speed_over = (data[0]&0x10)>>4;
        /*
         * avg_over gibt an, ob sich die durchschnittliche Wingeschwindigkeit
         * ausser dem Wertebereich des Sensors befindet
         */
        float avg_over = (data[0]&0x20)>>5;
        float dir1 = (data[1]&0x0F);
        float dir10 = (data[1]&0xF0)>>4;
        float dir100 = (data[2]&0x0F);
        float dir = 100*dir100 + 10*dir10 + dir1;
        float speed01 = (data[2]&0xF0)>>4;
        float speed1 = (data[3]&0x0F);
        float speed10 = (data[3]&0xF0)>>4;
        float speed = 10*speed10 + speed1 + speed01/10;
        float avg01 = (data[4]&0x0F);
        float avg1 = (data[4]&0xF0)>>4;
        float avg10 = (data[5]&0x0F);
        float avg = 10*avg10 + avg1 + avg01/10;
        float chill1 = (data[6]&0x0F);
        float chill10 = (data[6]&0xF0)>>4;
        float chill = 10*chill10 + chill1;
        // Wenn das oberste Bit 1 ist, ist das Vorzeichen der Gefuehlten Temperatur negativ
        if(((data[5]&0x40)>>7)==1)
            chill *= -1;
        QString richtung;
        if(dir>0&&dir<90){
            richtung = "Nord-Ost";
        }else if(dir>90&&dir<180){
            richtung = "Sued-Ost";
        }else if(dir>180&&dir<270){
            richtung = "Sued-West";
        }else
            richtung = "Nord-West";
        // Paket wurde vollstaendig empfangen
        std::cout<<"Gefuehlte Temperatur: "<<chill<<"°C"<<std::endl;
        std::cout<<"Windrichtung: "<<dir<<"° "<<richtung.toStdString()<<std::endl;
        std::cout<<"Windgeschwindigkeit: "<<speed<<" m/sec"<<std::endl;
        std::cout<<"Durchschnittliche Windgeschwindigkeit: "<<avg<<" m/sec"<<std::endl;
        if(!speed_over && !avg_over){
            dirlist<<dir;
            speedlist<<speed;
            avglist<<avg;
            chilllist<<chill;
            // Wenn das Zeitintervall abgelaufen ist, werden die Mittelwerte der Listen eingetragen
            if((QDateTime::currentMSecsSinceEpoch()-TIME_INTERVAL) > begin.toMSecsSinceEpoch()){
                QListIterator<float> idir(dirlist);
                QListIterator<float> ispeed(speedlist);
                QListIterator<float> iavg(avglist);
                QListIterator<float> ichill(chilllist);
                float sum = 0;
                // Mittelwert der gesammelten Windrichtungen bestimmen
                while(idir.hasNext()){
                    sum+=idir.next();
                }
                dir = sum/dirlist.count();
                sum = 0;
                // Mittelwert der gesammelten Windgeschwindigkeiten bestimmen
                while(ispeed.hasNext()){
                    sum+=ispeed.next();
                }
                speed = sum/speedlist.count();
                sum = 0;
                // Mittelwert der gesammelten mittleren Windgeschwindigkeiten bestimmen
                while(iavg.hasNext()){
                    sum+=iavg.next();
                }
                avg = sum/avglist.count();
                sum = 0;
                // Mittelwert der gesammelten gefühlten Temperaturen bestimmen
                while(ichill.hasNext()){
                    sum+=ichill.next();
                }
                chill = sum/chilllist.count();
                sum = 0;
                sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,5)").arg(dir);
                d->exec(sql);

                sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,6)").arg(speed);
                d->exec(sql);

                sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,7)").arg(avg);
                d->exec(sql);

                sql = QString("INSERT INTO wetterdaten (wwert,mnummer) VALUES(%1,4)").arg(chill);
                d->exec(sql);
                dirlist.clear();
                speedlist.clear();
                avglist.clear();
                chilllist.clear();
                begin = QDateTime::currentDateTime();
            }
        }else
            syslog(LOG_WARNING,"Winddaten nicht in Datenbank geschrieben. Werte ausser Wertebereich");
        status=NICHTS;
    }
}

/**
 * Diese Methode sammelt alle einzelnen Bytes bis das vollstaendige Paket empfangen wurde
 * und wertet es anschliessend aus.
 *
 * Wenn das Paket vollstaendig empfangen wurde wird eine Datenstruktur mit den aufbereiteten
 * empfangenen Werten zurueckgegeben
 *
 * @param byte welches empfangen wurde
 */
void BuildInformation::buildRainPackage(char byte){
    /*
     * Ein Regen-Paket hat folgenden Inhalt
     *
     * data[0]: 1. Datenbyte: aktuelle Niederschlagsmenge ausser Wertebereich, Totale Niederschlagsmenge ausser Wertebereich
     *                        Batterie des Regenmessers schwach, gestrige Niederschlagsmenge ausser Wertebereich
     * data[1]: 2. Datenbyte: 1er-Stelle und 10er-Stelle der aktuellen Niederschlagsmenge
     * data[2]: 3. Datenbyte: 100er-Stelle der aktuellen Niederschlagsmenge, 0.1er-Stelle der totalen Niederschlagsmenge
     * data[3]: 4. Datenbyte: 1er-Stelle der totalen Niederschlagsmenge, 10er-Stelle der totalen Niederschlagsmenge
     * data[4]: 5. Datenbyte: 100er-Stelle der totalen Niederschlagsmenge, 1000er-Stelle der totalen Niederschlagsmenge
     * data[5]: 6. Datenbyte: 1er-Stelle der gestrigen Niederschlagsmenge, 10er-Stelle der gestrigen Niederschlagsmenge
     * data[6]: 7. Datenbyte: 100er-Stelle der gestrigen Niederschlagsmenge, 1000er-Stelle der gestrigen Niederschlagsmenge
     * data[7]: 8. Datenbyte: 1er-Stelle der Minute des Startpunkts der Totalzaehlung, 10er-Stelle der Minute d. T.
     * data[8]: 9. Datenbyte: 1er-Stelle der Stunde des Startp. d. T., 10er-Stelle der Stunde d. T.
     * data[9]: 10. Datenbyte: 1er-Stelle des Tages des Startp. d. T, 10er-Stelle des Tages
     * data[10]: 11. Datenbyte: 1er-Stelle des Monats d. Startp., 10er-Stelle des Monats d. Startp.
     * data[11]: 12. Datenbyte: 1er-Stelle des Jahres d. Startp, 10er-Stelle des Jahres d. Startp.
     */
    static char data[RAIN_PACKET_LENGTH];
    data[RAIN_PACKET_LENGTH-verbleibendeBytes]=byte;
    verbleibendeBytes--;
    if(verbleibendeBytes==0){
        /*
         * Das 6. Bit des ersten Datenbytes beschreibt den Batteriestand des Sensors (0=OK, 1=schwach)
         */
        char battery_low =(data[0]&0x40)>>6;
        QString sql = QString("UPDATE sensoren SET sonline=1 AND sbatterieschwach=\"%1\" WHERE sname=\"Regensensor\"").arg(battery_low);
        d->exec(sql);
        /*
         * Out gibt an, ob sich der aktuelle Niederschlagswert ausser dem Wertebereich befindet
         *
         * 0: OK
         * 1: Wert ausser Wertebereich
         */
        float out = (data[0]&0x10)>>4;
        float akt1 = (data[1]&0x0F);
        float akt10 = (data[1]&0xF0)>>4;
        float akt100 = (data[2]&0x0F);
        float akt = 100*akt100 + 10*akt10 + akt1;
        float total01 = (data[2]&0xF0)>>4;
        float total1 = (data[3]&0x0F);
        float total10 = (data[3]&0xF0)>>4;
        float total100 = (data[4]&0x0F);
        float total1000 = (data[4]&0xF0)>>4;
        float total = 1000*total1000 + 100*total100 + 10*total10 + total1 + total01/10;
        std::cout<<"Aktuelle Niederschlagsmenge: "<<akt<<" mm/hr"<<std::endl;
        std::cout<<"Totale Niederschlagsmenge: "<<total<<" mm"<<std::endl;
        static QList<float> list;
        static QDateTime begin = QDateTime::currentDateTime();
        /*
         * Der gemessene Wert wird in die Datenbank eingetragen, sofern er nicht
         * ausser dem Wertebereich des Sensors liegt.
         *
         */
        if(!out){
            list<<akt;
            if((QDateTime::currentMSecsSinceEpoch()-TIME_INTERVAL) > begin.toMSecsSinceEpoch()){
                QListIterator<float> i(list);
                float sum = 0;
                // Mittelwert bilden
                while(i.hasNext()){
                    sum+=i.next();
                }
                akt = sum/list.count();
               sql = QString("INSERT INTO wetterdaten(wwert,mnummer) VALUES(%1,8)").arg(akt);
               d->exec(sql);
               list.clear();
               begin = QDateTime::currentDateTime();
            }
        }else
             syslog(LOG_WARNING,"Niederschlagswert nicht in Datenbank geschrieben. Werte ausser Wertebereich");
        status=NICHTS;
    }
}
