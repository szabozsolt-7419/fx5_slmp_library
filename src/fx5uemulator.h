/*
  * MsController
  *
  * Copyright (C) 2017- Isotoptech Zrt.
  *	Email: szabozsolt@isotoptech.hu
  *
  * This program is free software; you can redistribute it and/or modify it
  * under the terms of the GNU Library General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or (at your
  * option) any later version.
  *
  *  This program is distributed in the hope that it will be useful, but WITHOUT
  *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  *  License for more details.
  *
  *  You should have received a copy of the GNU General Public License
  *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef FX5UEMULATOR_H
#define FX5UEMULATOR_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QMap>

class QTcpSocket;
class QTcpServer;

#include "plc.h"



class Fx5uEmulator : QObject
{
    Q_OBJECT
public:
    enum Command {BlockRead = 0x0401, BlockWrite = 0x1401};
    enum SubCommand {Word = 0x0000, Bit = 0x0001};
    enum class Device {
            SM = 0x91,
            SD = 0xA9,
            X  = 0x9C,
            Y  = 0x9D,
            M  = 0x90,
            L  = 0x92,
            F  = 0x93,
            V  = 0x94,
            B  = 0xA0,
            D  = 0xA8,
            W  = 0xB4,
            TS = 0xC1,
            TC = 0xC0,
            SS = 0xC7,
            SC = 0xC6,
            SN = 0xC8,
            CS = 0xC4,
            CC = 0xC3,
            CN = 0xC5,
            SB = 0xA1,
            SW = 0xB5,
            S  = 0x98,
            DX = 0xA2,
            DY = 0xA3,
            Z  = 0xCC,
            R  = 0xAF,
            ZR = 0xB0,
        };

    enum class CarStatus {
        Idle = 0,
        DrivingInwards = 1,
        DrivingOutwards = 2,
        GoingIn = 3,
        ComingOut = 4
    };
    Fx5uEmulator(QObject *parent = nullptr, quint16 port = 5588);
    ~Fx5uEmulator();
    int processBuffer(QTcpSocket *socket, char *buffer, int count);
    quint8 readByte(char *buffer, quint16 position);
    quint16 readShort(char *buffer, quint16 position);
    quint32 readInt(char *buffer, quint16 position);
    void sendBits(QTcpSocket *socket, quint32 serial, quint16 start, quint16 device, quint16 blocks);
    void sendWords(QTcpSocket *socket, quint32 serial, quint16 start, quint16 device, quint16 blocks);
    void setBit(quint16 start, quint16 device, bool value);
    void setWord(quint16 start, quint16 device, quint16 value);
    void sendStatus(QTcpSocket *socket, quint32 serial, quint16 code);
    void addHeader(QByteArray &packet, quint32 serial, quint16 size=0xc, quint16 result = 0x0000, quint8 network = 0x00, quint8 pc = 0xff, quint16 io = 0x03ff, quint8 station = 0x00);
    void addShort(QByteArray &packet, quint16 data);
    void addInt(QByteArray &packet, quint32 data);
    void addByte(QByteArray &packet, quint8 data);
    void setPort(quint16 port);
    quint16 port() {return m_Port;}
    void start();
    void stop();
    void setMemory(quint16 index, bool value);
    void setData(quint16 index, quint16 value);
    void setDataBit(PLC::D alias, bool value);
    bool dataBit(PLC::D alias);
    void setDataWord(PLC::DW alias,quint16 value);    
    quint16 dataWord(PLC::DW alias);
    void setDataWord(QString name,quint16 value);
    void setDataBit(QString name,bool value);
    bool memory(quint16 index);
    quint16 data(quint16 index);
    quint16 randomWord(quint16 start,quint16 range);
    void addDelayedTask(quint16 delay,QString name,bool value);
    void enableDebug(bool state) {m_DebugEnabled = state;}
private slots:
    void slotNewConnection();
    void slotClientDisconnected();
    void slotReadyRead();
    void slotTaskTimeout();
    void slotChangeStatus();
    void slotInspectionTimeout();
    void slotBoxChangeTimeout();
    void slotBoxMoveTimeout();
    void slotStorageChangeTimeout();
private:
    PLC::M memoryAlias(quint16 index);
    QString memoryName(quint16 index);
    PLC::D dataAlias(quint16 index);
    QString dataName(quint16 index);
    void processCommand(quint16 index, bool value);
    void openCylinder(QString name, quint16 delay = 5);
    void closeCylinder(QString name, quint16 delay = 5);
    void startArm(int index,bool value);
    void startSelectedProcess();
    void driveCarInwards(bool value);
    void driveCarOutwards(bool value);
    void sendCarIn();
    void sendCarOut();
    void addDistance(quint16 value);
    void removeDistance(quint16 value);
    void updateCarStatus(quint32 distance);
    void openStorageDoor();
    void closeStorageDoor();
    void startInspection();
    void startStorageChange();
    void startBoxChange();
    void startBoxMove();
    void hoistSource(int index);
    void lowerSource(int index);
    void initializeDevices();
    bool isBistableCylinder(QString name);
private:    
    QSet<QTimer *> m_Timers;
    QTcpServer *m_Server;
    QMap<quint16,bool> m_M;
    QMap<quint16,quint16> m_D;
    QSet<QTcpSocket *> m_Connections;
    quint16 m_Port;
    bool m_DebugEnabled;
    QTimer m_StatusTimer;
    CarStatus m_CarStatus;
    QTimer m_InspectionTimer;
    QTimer m_BoxChangeTimer;
    QTimer m_BoxMoveTimer;
    QTimer m_StorageChangeTimer;
    static QSet<QString> m_BistableCylinders;
    quint16 m_Padding;  //added just to remove the annoying padding warning.
};

#endif // FX5UEMULATOR_H
