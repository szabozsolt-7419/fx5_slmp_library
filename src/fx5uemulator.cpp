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
#include "fx5uemulator.h"

#include <QTcpServer>
#include <QMap>
#include <QSet>
#include <QTcpSocket>
#include <QSet>
#include <QTime>

class Logger {
public:
    static void log(const QString &functionName,const QString &message) {
        qDebug() << functionName << ": " << message;
    }
};

Logger Logger;

QSet<QString> Fx5uEmulator::m_BistableCylinders = {
  "C7", "C9", "C10", "C15", "C16", "C17", "C19", "C23", "C24", "C26"
};


Fx5uEmulator::Fx5uEmulator(QObject *parent,quint16 port) :
    QObject(parent),
    m_Server(new QTcpServer(this)),
    m_Port(port),
    m_DebugEnabled(false),
    m_CarStatus(CarStatus::Idle)
{
    connect(m_Server,SIGNAL(newConnection()),this,SLOT(slotNewConnection()));
    m_StatusTimer.setInterval(1000);
    m_StatusTimer.setSingleShot(false);
    connect(&m_StatusTimer,SIGNAL(timeout()),this,SLOT(slotChangeStatus()));
    connect(&m_InspectionTimer,SIGNAL(timeout()),this,SLOT(slotInspectionTimeout()));
    m_InspectionTimer.setSingleShot(true);
    connect(&m_BoxChangeTimer,SIGNAL(timeout()),this,SLOT(slotBoxChangeTimeout()));
    m_BoxChangeTimer.setSingleShot(true);
    connect(&m_BoxMoveTimer,SIGNAL(timeout()),this,SLOT(slotBoxMoveTimeout()));
    m_BoxMoveTimer.setSingleShot(true);
    connect(&m_StorageChangeTimer,SIGNAL(timeout()),this,SLOT(slotStorageChangeTimeout()));
    m_StorageChangeTimer.setSingleShot(true);

    srand(static_cast<unsigned int>(QTime::currentTime().msecsSinceStartOfDay()));
    initializeDevices();
}

Fx5uEmulator::~Fx5uEmulator()
{
    disconnect(m_Server,SIGNAL(newConnection()),this,SLOT(slotNewConnection()));
}

void Fx5uEmulator::slotNewConnection()
{
    QTcpSocket *socket = m_Server->nextPendingConnection();
    connect(socket,SIGNAL(disconnected()),this,SLOT(slotClientDisconnected()));
    connect(socket,SIGNAL(readyRead()),this,SLOT(slotReadyRead()));
    m_Connections.insert(socket);
}

void Fx5uEmulator::slotClientDisconnected()
{
    QTcpSocket *socket = dynamic_cast<QTcpSocket *>(sender());
    if (socket) {
        m_Connections.remove(socket);
        socket->disconnectFromHost();
        socket->deleteLater();
    }
}

void Fx5uEmulator::slotReadyRead()
{
    QTcpSocket *socket = dynamic_cast<QTcpSocket *>(sender());
    if (socket) {
        char buffer[2048] = {0};
      int count = qMin(1024,static_cast<int>(socket->bytesAvailable()));
      socket->peek(buffer,count);
      QByteArray msg = QByteArray(buffer,count);

      if (count >=512) {
          socket->read(buffer,qMin(1024,count));
          Logger.log(Q_FUNC_INFO,QString("Buffer overflow on client %1").arg(socket->peerAddress().toString()));
          Logger.log(Q_FUNC_INFO,QString::fromStdString(msg.toHex().toStdString()));
          return;
      }

      if (m_DebugEnabled) {
        Logger.log(Q_FUNC_INFO,QString("Received response from %1").arg(socket->peerAddress().toString()));
        Logger.log(Q_FUNC_INFO,QString::fromStdString(msg.toHex().toStdString()));
      }


      int total = processBuffer(socket, buffer,count);
      socket->read(buffer, total); //remove all data from the buffer
    }

}

int Fx5uEmulator::processBuffer(QTcpSocket *socket, char *buffer, int count)
{
    int total = 0;

    if (count >= 15) {//we have a full header
        quint32 serial = readInt(buffer,2) & 0xffff; //ignore the higher two bytes
        quint16 size = readShort(buffer,11);

        if ( count >= size + 13) {
            quint16 command = readShort(buffer,15);
            quint16 subcommand = readShort(buffer,17);
            quint16 start;
            quint8 device;
            quint16 blocks;
            quint16 value;
            switch (command) {
            case Command::BlockRead:
                start = readShort(buffer,19);
                device = readByte(buffer,22);
                blocks = readShort(buffer,23);
                if (m_DebugEnabled) {
                    Logger.log(Q_FUNC_INFO,QString("Device: %1 start: %2 blocks: %3").arg(device,4,16,QChar('0')).arg(start).arg(blocks));
                }
                switch(subcommand) {
                case SubCommand::Bit:
                    sendBits(socket, serial, start, device, blocks);
                    break;
                case SubCommand::Word:
                    sendWords(socket, serial, start, device, blocks);
                    break;
                default:
                    Logger.log(Q_FUNC_INFO,QString("SubCommand %1 not handled!").arg(subcommand,4,16,QChar('0')));
                    break;
                }
                break;
            case Command::BlockWrite:
                start = readShort(buffer,19);
                device = readByte(buffer,22);
                blocks = readShort(buffer,23);
                if (blocks != 1) {
                    Logger.log(Q_FUNC_INFO,QString("Only one block is handled when writing devices!"));
                } else {
                    switch(subcommand) {
                    case SubCommand::Bit:
                        value = readByte(buffer,25);
                        setBit(start,device,value > 0);
                        sendStatus(socket,serial,0x0000);
                        break;
                    case SubCommand::Word:
                        value = readShort(buffer,25);
                        setWord(start,device,value);
                        sendStatus(socket,serial, 0x0000);
                        break;
                    default:
                        Logger.log(Q_FUNC_INFO,QString("SubCommand %1 not handled!").arg(subcommand,4,16,QChar('0')));
                        break;
                    }
                }
                break;
            default:
                Logger.log(Q_FUNC_INFO,QString("Command %1 not handled!").arg(command,4,16,QChar('0')));
                break;
            }
            total += size + 13;
        }
    }

    return total;
}

quint8 Fx5uEmulator::readByte(char *buffer, quint16 position)
{
    return static_cast<quint8>(buffer[position]);
}

quint16 Fx5uEmulator::readShort(char *buffer, quint16 position)
{
    quint8 low = readByte(buffer,position);
    quint8 high = readByte(buffer,position+1);

    return static_cast<quint16>(low | (high << 8));
}

quint32 Fx5uEmulator::readInt(char *buffer, quint16 position)
{
    quint8 b1 = readByte(buffer,position);
    quint8 b2 = readByte(buffer,position+1);
    quint8 b3 = readByte(buffer,position+2);
    quint8 b4 = readByte(buffer,position+3);

    return static_cast<quint32>(b1 | (b2 << 8) | (b3 << 16) | (b4 << 24));
}

void Fx5uEmulator::sendBits(QTcpSocket *socket, quint32 serial, quint16 start, quint16 device, quint16 blocks)
{
    //!!!We don't handle even block numbers!!!!

    QByteArray packet;
    addHeader(packet,serial,2 + (blocks / 2));  //size is return code size(2) + half byte for each bits

    if (m_DebugEnabled) {
        Logger.log(Q_FUNC_INFO,QString("%1 vs %2").arg(device).arg(static_cast<quint16>(Device::M)));
    }
    if (static_cast<Device>(device) == Device::M) {
        for(quint16 index=0;index < blocks;index+=2) {
            quint8 value = 0x00;
            if (m_M.value(start+index)) {
                value |= 0x10;
            }
            if (m_DebugEnabled) {
                Logger.log(Q_FUNC_INFO,QString("Returning M%1(%2)").arg(start + index).arg(m_M.value(start+index)));
            }
            if (m_M.value(start + index + 1)) {
                value |= 0x01;
            }
            if (m_DebugEnabled) {
                Logger.log(Q_FUNC_INFO,QString("Returning M%1(%2)").arg(start + index + 1).arg(m_M.value(start+index+1)));
            }
            addByte(packet, value);
        }
    } else {
        for(int index=0;index < blocks;index++) {
            addByte(packet, 0x00);
        }

    }

    socket->write(packet);

}

void Fx5uEmulator::sendWords(QTcpSocket *socket, quint32 serial, quint16 start, quint16 device, quint16 blocks)
{
    Q_UNUSED(blocks)

    QByteArray packet;
    addHeader(packet,serial,2 + 2*blocks);
    quint16 value = 0x0000;
    if (static_cast<Device>(device) == Device::D) {
        for(quint16 index=0;index < blocks;index++) {
            value = m_D.value(start + index);
            //Logger.log(Q_FUNC_INFO,QString("Returning D%1(%2)").arg(start + index).arg(value));
            addShort(packet,value);
        }
    } else {
        for(quint16 index=0;index < blocks;index++) {
            addShort(packet,0x0000);
        }
    }

    socket->write(packet);
}

void Fx5uEmulator::setBit(quint16 start, quint16 device, bool value)
{
    if (static_cast<Device>(device) == Device::M) {
        QString name = memoryName(start);
        Logger.log(Q_FUNC_INFO,QString("%1(M%2) := %3").arg(name).arg(start).arg(value));
        m_M.insert(start, value > 0);

        processCommand(start,value);
    }

}

void Fx5uEmulator::setWord(quint16 start, quint16 device, quint16 value)
{
    if (static_cast<Device>(device) == Device::D) {
        QString name = dataName(start);
        Logger.log(Q_FUNC_INFO,QString("%1(D%2) := %3").arg(name).arg(start).arg(value));
        m_D.insert(start,value);
    }
}

void Fx5uEmulator::sendStatus(QTcpSocket *socket, quint32 serial, quint16 code)
{
    QByteArray packet;
    addHeader(packet,serial,0x0c, code);

    socket->write(packet);
}



void Fx5uEmulator::addHeader(QByteArray &packet, quint32 serial, quint16 size, quint16 result, quint8 network, quint8 pc,quint16 io,quint8 station)
{
    addShort(packet,0x00D4);
    addInt(packet,serial);
    addByte(packet,network); //default network no
    addByte(packet,pc); //default pc no
    addShort(packet,io); //default dest io
    addByte(packet,station); //default request dest station
    addShort(packet,size); //data length, fix 0x12 for read commands
    addShort(packet,result); //monitor timer
}

void Fx5uEmulator::addShort(QByteArray &packet, quint16 data)
{
    packet.append(static_cast<char>(data & 0xff)); //low byte
    packet.append(static_cast<char>(data >> 8)); //high byte
}

void Fx5uEmulator::addInt(QByteArray &packet, quint32 data)
{
    packet.append(static_cast<char>(data & 0xff)); //low byte
    packet.append(static_cast<char>((data >> 8) & 0xff)); //second low byte
    packet.append(static_cast<char>((data >> 16) & 0xff)); //third low byte
    packet.append(static_cast<char>((data >> 24) & 0xff)); //high byte
}

void Fx5uEmulator::addByte(QByteArray &packet, quint8 data)
{
    packet.append(static_cast<char>(data));
}




void Fx5uEmulator::setPort(quint16 port)
{    
    m_Port = port;
}

void Fx5uEmulator::start()
{    
    m_Server->listen(QHostAddress::AnyIPv4,m_Port);
}

void Fx5uEmulator::stop()
{    
    m_Server->close();
}

void Fx5uEmulator::setMemory(quint16 index, bool value)
{    
    m_M.insert(index,value);
}

void Fx5uEmulator::setData(quint16 index, quint16 value)
{    
    m_D.insert(index,value);
}

void Fx5uEmulator::setDataBit(PLC::D alias, bool value)
{
    quint16 index = static_cast<quint16>(alias);
    quint16 start = index >> 4;
    quint16 position = index & 0x0f;
    quint16 current = data(start);
    if (value) {
        current |= (1<<position);
    } else {
        current &= ~(1<<position);
    }
    setData(start,current);
}

bool Fx5uEmulator::dataBit(PLC::D alias)
{
    quint16 index = static_cast<quint16>(alias);
    quint16 start = index >> 4;
    quint16 position = index & 0x0f;

    quint16 value = data(start);

    return (value & (1 << position) ) > 0;
}

void Fx5uEmulator::setDataWord(PLC::DW alias, quint16 value)
{
    setData(static_cast<quint16>(alias),value);
}

quint16 Fx5uEmulator::dataWord(PLC::DW alias)
{
    return data(alias);
}

void Fx5uEmulator::setDataWord(QString name, quint16 value)
{
    std::string stdName = name.toStdString();
    QMetaEnum metaEnum = QMetaEnum::fromType<PLC::DW>();

    bool ok;
    PLC::DW alias = static_cast<PLC::DW>(metaEnum.keyToValue(stdName.c_str(),&ok));

    if (ok) setDataWord(alias,value);
}

void Fx5uEmulator::setDataBit(QString name, bool value)
{
    std::string stdName = name.toStdString();
    QMetaEnum metaEnum = QMetaEnum::fromType<PLC::D>();

    bool ok;
    PLC::D alias = static_cast<PLC::D>(metaEnum.keyToValue(stdName.c_str(),&ok));

    if (ok) setDataBit(alias,value);
}

bool Fx5uEmulator::memory(quint16 index)
{    
    if (m_M.contains(index)) return m_M.value(index);

    return false;
}

quint16 Fx5uEmulator::data(quint16 index)
{    
    if (m_D.contains(index)) return m_D.value(index);

    return false;
}

quint16 Fx5uEmulator::randomWord(quint16 start, quint16 range)
{
    int value = qrand();
    quint16 number = static_cast<quint16>((value*range)/RAND_MAX);
    //Logger.log(Q_FUNC_INFO,QString("%1 with range %2 is %3").arg(value).arg(range).arg(number));
    return start + number;
}

void Fx5uEmulator::addDelayedTask(quint16 delay, QString name, bool value)
{
    QTimer *timer = new QTimer();
    timer->setInterval(delay*1000);
    timer->setProperty("alias",name);
    timer->setProperty("value",value);
    Logger.log(Q_FUNC_INFO,QString("Name: %1 Value: %2 Delay: %3").arg(name).arg(value).arg(delay));
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(slotTaskTimeout()));
    timer->start();
}

void Fx5uEmulator::slotTaskTimeout()
{
    QTimer *timer = dynamic_cast<QTimer *>(sender());
    if (timer != nullptr) {
        m_Timers.remove(timer);
        QString name = timer->property("alias").toString();
        bool value = timer->property("value").toBool();
        Logger.log(Q_FUNC_INFO,QString("Name: %1 value: %2").arg(name).arg(value));
        setDataBit(name,value);

        timer->deleteLater();
    } else {
        Logger.log(Q_FUNC_INFO,QString("Invalid task timer!"));
    }
}

void Fx5uEmulator::slotChangeStatus()
{
    setDataWord(PLC::DW::Temperature,randomWord(300,60));
    setDataWord(PLC::DW::Conductivity1,randomWord(5,3));
    setDataWord(PLC::DW::Conductivity2,randomWord(10,3));

    if (dataBit(PLC::D::CarMoving)) {//We have a moving car here!
        if ((m_CarStatus == CarStatus::DrivingInwards) || (m_CarStatus == CarStatus::GoingIn)) {
            addDistance(20);
        } else {
            removeDistance(20);
        }

    }
}

void Fx5uEmulator::slotInspectionTimeout()
{
    QTimer *timer = dynamic_cast<QTimer *>(sender());
    if (timer) {
        int step = timer->property("step").toInt();
        int delay = 10;

        switch(step) {
        case 0:
            setDataBit(PLC::D::ControlPoint2MasterKey,true);
            break;
        case 1:
            setDataBit(PLC::D::PersonnelDoorClosed,false);
            break;
        case 2:
            setDataBit(PLC::D::ControlPoint1MasterKey,false);
            setDataBit(PLC::D::OperatorIn,true);
            delay = 20;
            break;
        case 3:
            setDataBit(PLC::D::ControlPoint1MasterKey,true);
            setDataBit(PLC::D::OperatorIn,false);
            break;
        case 4:
            setDataBit(PLC::D::PersonnelDoorClosed,true);
            break;
        case 5:
            setDataBit(PLC::D::ControlPoint2MasterKey,false);
            setDataBit(PLC::D::InspectionReady,true);
            break;
        case 6:
            setDataBit(PLC::D::InspectionReady,false);
            return;
        default:
            return;
        }

        step++;
        timer->setProperty("step",step);
        timer->setInterval(delay*1000);
        timer->start();
    }

}

void Fx5uEmulator::slotBoxChangeTimeout()
{
    QTimer *timer = dynamic_cast<QTimer *>(sender());
    if (timer) {
        int step = timer->property("step").toInt();
        quint16 delay = 10;

        switch(step) {
        case 0:
            delay = 1;
            startStorageChange();
            break;
        case 1:
            delay = 1;
            if (m_StorageChangeTimer.isActive()) {
                m_BoxChangeTimer.start();
                return;
            }
            sendCarIn();
            break;
        case 2:
            delay = 1;
            if (dataBit(PLC::D::CarMoving)) {
                m_BoxChangeTimer.start();
                return;
            }
            openCylinder("C8",delay);
            break;
        case 3:
            delay = 18;
            openCylinder("C6",delay);
            break;
        case 4:
            delay = 12;
            closeCylinder("C6",delay);
            break;
        case 5:
            delay = 16;
            openCylinder("C18",delay);
            break;
        case 6:
            delay = 16;
            closeCylinder("C18",delay);
            break;
        case 7:
            delay = 18;
            openCylinder("C17",delay);
            break;
        case 8:
            delay = 12;
            openCylinder("C11",delay);
            openCylinder("C13",delay);
            break;
        case 9:
            delay = 18;
            closeCylinder("C11",delay);
            closeCylinder("C13",delay);
            closeCylinder("C17",delay);
            openCylinder("C8",delay);
            break;
        case 10:
            delay = 24;
            openCylinder("C3",delay);
            openCylinder("C5",delay);
            break;
        case 11:
            delay = 24;
            closeCylinder("C3",delay);
            closeCylinder("C5",delay);
            closeCylinder("C8",delay);
            break;
        case 12:
            delay = 12;
            sendCarOut();
            break;
        case 13:
            delay = 12;
            openCylinder("C7",delay);
            openCylinder("C9",delay);
            openCylinder("C15",delay);
            openCylinder("C16",delay);
            break;
        case 14:
            delay = 18;
            openCylinder("C2",delay);
            openCylinder("C4",delay);
            openCylinder("C6",delay);
            break;
        case 15:
            delay = 12;
            closeCylinder("C2",delay);
            closeCylinder("C4",delay);
            closeCylinder("C6",delay);
            break;
        case 16:
            delay = 18;
            closeCylinder("C7",delay);
            closeCylinder("C9",delay);
            closeCylinder("C10",delay);
            break;
        case 17:
            delay = 12;
            openCylinder("C12",delay);
            openCylinder("C14",delay);
            break;
        case 18:
            delay = 12;
            closeCylinder("C12",delay);
            closeCylinder("C14",delay);
            break;
        case 19:
            delay = 18;
            closeCylinder("C15",delay);
            break;
        default: //end of storage change
            if (dataBit(PLC::D::BoxMoveOperationMode)) {
                setDataBit(PLC::D::BoxMoveOperationMode,false);
                setDataBit(PLC::D::ProcessNormalStopTimeIsOver,true);
            }
            return;
        }

        step++;
        timer->setProperty("step",step);
        timer->setInterval(delay*1000);
        timer->start();
    }
}

void Fx5uEmulator::slotBoxMoveTimeout()
{
    QTimer *timer = dynamic_cast<QTimer *>(sender());
    if (timer) {
        int step = timer->property("step").toInt();
        quint16 delay = 10;

        switch(step) {
        case 0:
            delay = 18;
            openCylinder("C6",delay);
            break;
        case 1:
            delay = 12;
            openCylinder("C1",delay);
            openCylinder("C3",delay);
            break;
        case 2:
            delay = 12;
            closeCylinder("C1",delay);
            closeCylinder("C3",delay);
            closeCylinder("C6",delay);
            break;
        case 3:
            delay = 6;
            openCylinder("C18",delay);
            break;
        case 4:
            delay = 3;
            openCylinder("C7",delay);
            openCylinder("C9",delay);
            openCylinder("C17",delay);
            break;
        case 5:
            delay = 18;
            closeCylinder("C18",delay);
            break;
        case 6:
            delay = 12;
            openCylinder("C2",delay);
            openCylinder("C4",delay);
            break;
        case 7:
            delay = 20;
            closeCylinder("C2",delay);
            closeCylinder("C4",delay);
            break;
        case 8:
            delay = 18;
            closeCylinder("C7",delay);
            closeCylinder("C9",delay);
            openCylinder("C10",delay);
            break;
        case 9:
            delay = 12;
            openCylinder("C11",delay);
            openCylinder("C13",delay);
            break;
        case 10:
            delay = 12;
            closeCylinder("C11",delay);
            closeCylinder("C13",delay);
            break;
        case 11:
            delay = 12;
            openCylinder("C15",delay);
            openCylinder("C16",delay);
            break;
        case 12:
            delay = 12;
            openCylinder("C12",delay);
            openCylinder("C14",delay);
            break;
        case 13:
            delay = 12;
            closeCylinder("C12",delay);
            closeCylinder("C14",delay);
            break;
        case 14:
            delay = 18;
            closeCylinder("C15",delay);
            closeCylinder("C16",delay);
            closeCylinder("C17",delay);
            closeCylinder("C10",delay);
            break;
        default: //end of storage change
            if (dataBit(PLC::D::BoxMoveOperationMode)) {
                setDataBit(PLC::D::BoxMoveOperationMode,false);
                setDataBit(PLC::D::ProcessNormalStopTimeIsOver,true);
            }
            return;
        }

        step++;
        timer->setProperty("step",step);
        timer->setInterval(delay*1000);
        timer->start();
    }
}

void Fx5uEmulator::slotStorageChangeTimeout()
{
    QTimer *timer = dynamic_cast<QTimer *>(sender());
    if (timer) {
        int step = timer->property("step").toInt();
        quint16 delay = 10;

        switch(step) {
        case 0:
            delay = 1;
            openCylinder("C21",delay);
            openCylinder("C24",delay);

            break;
        case 1:
            delay = 12;
            openCylinder("C22",delay);
            break;
        case 2:
            delay = 8;
            closeCylinder("C21",delay);
            break;
        case 3:
            delay = 12;
            closeCylinder("C22",delay);
            openCylinder("C25",delay);
            break;
        case 4:
            delay = 15;
            openCylinder("C26",delay);
            break;
        case 5:
            delay = 18;
            closeCylinder("C25",delay);
            break;
        case 6:
            delay = 20;
            openCylinder("C24",delay);
            break;
        case 7:
            delay = 20;
            closeCylinder("C23",delay);
            break;
        default: //end of storage change
            if (dataBit(PLC::D::StorageChangeOperationMode)) {
                setDataBit(PLC::D::StorageChangeOperationMode,false);
                setDataBit(PLC::D::ProcessNormalStopTimeIsOver,true);
            }
            return;
        }

        step++;
        timer->setProperty("step",step);
        timer->setInterval(delay*1000);
        timer->start();
    }
}

PLC::M Fx5uEmulator::memoryAlias(quint16 index)
{
    return static_cast<PLC::M>(index);
}

QString Fx5uEmulator::memoryName(quint16 index)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PLC::M>();

    return metaEnum.valueToKey(index);
}

PLC::D Fx5uEmulator::dataAlias(quint16 index)
{
    return static_cast<PLC::D>(index);
}

QString Fx5uEmulator::dataName(quint16 index)
{
    QMetaEnum metaEnum = QMetaEnum::fromType<PLC::D>();

    return metaEnum.valueToKey(index);
}

void Fx5uEmulator::processCommand(quint16 index, bool value)
{
    QString name = memoryName(index);
    if (name.startsWith("Set") && name.endsWith("Open")) {
        name.remove(0,3);
        name.chop(4);
        if (value) openCylinder(name,randomWord(2,3));
    } else if (name.startsWith("Set") && name.endsWith("Close")) {
        name.remove(0,3);
        name.chop(5);
        if (value) closeCylinder(name,randomWord(2,3));
    } else {
        switch(index) {
        case PLC::M::VentilationStartMotor1:
            if (value) startArm(1,true);
            break;
        case PLC::M::VentilationStartMotor2:
            if (value) startArm(2,true);
            break;
        case PLC::M::StartSelectedProcess:
            startSelectedProcess();
            break;
        case PLC::M::ServiceDriveCarInwards:
            driveCarInwards(value);
            break;
        case PLC::M::ServiceDriveCarOutwards:
            driveCarOutwards(value);
            break;
        case PLC::M::ServiceSendCarIn:
            if (value) sendCarIn();
            break;
        case PLC::M::ServiceSendCarOut:
            if (value) sendCarOut();
            break;
        case PLC::M::ServiceOpenStorageDoor:
            if (value) openStorageDoor();
            break;
        case PLC::M::ServiceCloseStorageDoor:
            if (value) closeStorageDoor();
            break;
        case PLC::M::StartOfInspection:
            startInspection();
            break;
        case PLC::M::StartPTIRRackMove:
            setDataBit(PLC::D::ProcessNormalStopTimeIsOver,false);
            startBoxMove();
            break;
        case PLC::M::StartPTIRRackChange:
            setDataBit(PLC::D::ProcessNormalStopTimeIsOver,false);
            startBoxChange();
            break;
        default:
            break;
        }

    }


}

void Fx5uEmulator::openCylinder(QString name, quint16 delay)
{
        Logger.log(Q_FUNC_INFO,QString("Opening cylinder %1").arg(name));
        setDataBit(QString("Set%1OpenStatus").arg(name),true);
        setDataBit(QString("%1In").arg(name),false);
        if (delay == 0) delay  = randomWord(2,3);
        setDataWord(QString("%1TimeValue").arg(name),delay);
        addDelayedTask(delay,QString("%1Out").arg(name),true);
        addDelayedTask(delay,QString("Set%1OpenStatus").arg(name),false);
        if (isBistableCylinder(name)) {
            setDataBit(QString("%1MoveOutStatus").arg(name),true);
            addDelayedTask(delay,QString("%1MoveOutStatus").arg(name),false);
        } else {
            setDataBit(QString("%1MoveStatus").arg(name),true);
        }
}

void Fx5uEmulator::closeCylinder(QString name, quint16 delay)
{    
        Logger.log(Q_FUNC_INFO,QString("Closing cylinder %1").arg(name));
        setDataBit(QString("Set%1CloseStatus").arg(name),true);
        setDataBit(QString("%1Out").arg(name),false);
        if (delay == 0) delay = randomWord(2,3);
        setDataWord(QString("%1TimeValue").arg(name),delay);
        addDelayedTask(delay,QString("%1In").arg(name),true); 
        addDelayedTask(delay,QString("Set%1CloseStatus").arg(name),false);
        if (isBistableCylinder(name)) {
            setDataBit(QString("%1MoveInStatus").arg(name),true);
            addDelayedTask(delay,QString("%1MoveInStatus").arg(name),false);
        } else {
            setDataBit(QString("%1MoveStatus").arg(name),false);
        }
}

void Fx5uEmulator::startArm(int index, bool value)
{
    quint16 delay = randomWord(3,5);
    if (index == 1) {
        setData(PLC::D::VentilationButterflyV1,true);
        setData(PLC::D::VentilationButterflyV2,false);
    } else {
        setData(PLC::D::VentilationButterflyV1,false);
        setData(PLC::D::VentilationButterflyV2,true);
    }
    addDelayedTask(delay,QString("Ventilation%1MotorRun").arg(index),value);
}

void Fx5uEmulator::startSelectedProcess()
{
    setDataWord(PLC::DW::SelectedBoxNumber,data(PLC::NumberOfBox));
    setDataWord(PLC::DW::SelectedStepNumber,data(PLC::NumberOfSteps));
    setDataBit(PLC::D::ContinuousOperationMode,memory(PLC::M::SelectContinuousMode));
    setDataBit(PLC::D::BatchOperationMode,memory(PLC::M::SelectBatchMode));
    setDataBit(PLC::D::StayingOperationMode,memory(PLC::M::SelectStayingMode));
    setDataBit(PLC::D::FillEmptyOperationMode,memory(PLC::M::SelectFillEmptyMode));
    setDataBit(PLC::D::StorageChangeOperationMode,memory(PLC::M::SelectStorageChangeMode));
    setDataBit(PLC::D::BoxChangeOperationMode,memory(PLC::M::SelectBoxChangeMode));
    setDataBit(PLC::D::BoxMoveOperationMode,memory(PLC::M::SelectBoxMoveMode));
    setDataBit(PLC::D::Source1Selected,memory(PLC::M::SelectSource1));
    setDataBit(PLC::D::Source2Selected,memory(PLC::M::SelectSource2));
    setDataBit(PLC::D::Source3Selected,memory(PLC::M::SelectSource3));
    setDataWord(PLC::DW::SelectedIrradiationHours,PLC::DW::IrradiationHours);
    setDataWord(PLC::DW::SelectedIrradiationMinutes,PLC::DW::IrradiationMinutes);
    setDataWord(PLC::DW::SelectedIrradiationSeconds,PLC::DW::IrradiationSeconds);
}

void Fx5uEmulator::driveCarInwards(bool value)
{
    setDataBit(PLC::D::CarMoving,value);
    if (value) {
        m_CarStatus = CarStatus::DrivingInwards;
    } else {
        m_CarStatus = CarStatus::Idle;
    }
}

void Fx5uEmulator::driveCarOutwards(bool value)
{
    setDataBit(PLC::D::CarMoving,value);
    if (value) {
        m_CarStatus = CarStatus::DrivingOutwards;
    } else {
        m_CarStatus = CarStatus::Idle;
    }
}

void Fx5uEmulator::sendCarIn()
{
    setDataBit(PLC::D::CarMoving,true);
    m_CarStatus = CarStatus::GoingIn;
}

void Fx5uEmulator::sendCarOut()
{
    setDataBit(PLC::D::CarMoving,true);
    m_CarStatus = CarStatus::ComingOut;
}

void Fx5uEmulator::addDistance(quint16 value)
{
    quint32 distance = dataWord(PLC::DW::Distance_1) + 65536*dataWord(PLC::DW::Distance_2);
    if (distance + value >= 1890) {
        distance = 1890;
        m_CarStatus = CarStatus::Idle;
        setDataBit(PLC::D::CarMoving,false);
    } else {
        distance += value;
    }

    updateCarStatus(distance);


}

void Fx5uEmulator::removeDistance(quint16 value)
{
    quint32 distance = dataWord(PLC::DW::Distance_1) + 65536*dataWord(PLC::DW::Distance_2);
    if (distance < value) {
        distance = 0;
        m_CarStatus = CarStatus::Idle;
        setDataBit(PLC::D::CarMoving,false);
    } else {
        distance -= value;
    }

    updateCarStatus(distance);
}

void Fx5uEmulator::updateCarStatus(quint32 distance)
{
    if (distance == 0) {
        setDataBit(PLC::D::E2Sensor,true);
    } else {
        setDataBit(PLC::D::E2Sensor,false);
    }

    if (distance == 1890) {
        setDataBit(PLC::D::E4Sensor,true);
    } else {
        setDataBit(PLC::D::E4Sensor,false);
    }


    if ((distance > 2500) && (distance < 3200)) {
        setDataBit(PLC::D::BeforeDoorSensor,true);
    } else {
        setDataBit(PLC::D::BeforeDoorSensor,false);
    }

    if ((distance > 3200) && (distance < 5800)) {
        setDataBit(PLC::D::StorageDoorClosed,false);
    } else {
        setDataBit(PLC::D::StorageDoorClosed,true);
    }

    if ((distance > 4900) && (distance < 5300)) {
        setDataBit(PLC::D::StorageDoorSensor,true);
    } else {
        setDataBit(PLC::D::StorageDoorSensor,true);
    }

    if ((distance > 5800) && (distance < 6900)) {
        setDataBit(PLC::D::AfterDoorSensor,true);
    } else {
        setDataBit(PLC::D::AfterDoorSensor,true);
    }

    setDataWord(PLC::DW::Distance_1,distance & 0xffff);
    setDataWord(PLC::DW::Distance_2, distance >> 16);
}

void Fx5uEmulator::openStorageDoor()
{
    quint16 delay1 = randomWord(1,1);
    quint16 delay2 = delay1 + randomWord(3,3);
    openCylinder("C20",delay1);
    openCylinder("C19",delay2);
    setDataBit(PLC::D::StorageDoorClosed,false);    
}

void Fx5uEmulator::closeStorageDoor()
{
    quint16 delay1 = randomWord(3,3);
    quint16 delay2 = delay1 + randomWord(1,1);
    closeCylinder("C19",delay1);
    closeCylinder("C20",delay2);
    addDelayedTask(delay2,"StorageDoorClosed",true);
}

void Fx5uEmulator::startInspection()
{
    m_InspectionTimer.setProperty("step",0);
    m_InspectionTimer.setInterval(1000);
    m_InspectionTimer.start();
}

void Fx5uEmulator::startStorageChange()
{
    m_StorageChangeTimer.setProperty("step",0);
    m_StorageChangeTimer.setInterval(1000);
    m_StorageChangeTimer.start();
}

void Fx5uEmulator::startBoxChange()
{
    m_BoxChangeTimer.setProperty("step",0);
    m_BoxChangeTimer.setInterval(1000);
    m_BoxChangeTimer.start();
}

void Fx5uEmulator::startBoxMove()
{
    m_BoxMoveTimer.setProperty("step",0);
    m_BoxMoveTimer.setInterval(1000);
    m_BoxMoveTimer.start();
}

void Fx5uEmulator::hoistSource(int index)
{
    setDataBit(QString("Source%1StoragePosition").arg(index),false);
    setDataBit(QString("Source%1MoveUp").arg(index),true);
    quint16 delay = randomWord(18,8);
    addDelayedTask(delay,QString("Source%1IrradiationPosition").arg(index),true);
    setDataBit(PLC::D::RadiationHazard,true);
}

void Fx5uEmulator::lowerSource(int index)
{
    setDataBit(QString("Source%1IrradiationPosition").arg(index),false);
    setDataBit(QString("Source%1MoveUp").arg(index),false);
    quint16 delay = randomWord(18,8);
    addDelayedTask(delay,QString("Source%1StoragePosition").arg(index),true);
    addDelayedTask(delay,QString("RadiationHazard"),false);
}


void Fx5uEmulator::initializeDevices()
{
    for(int index=1;index < 27;index++) {
        setDataBit(QString("C%1In").arg(index),true);
        setDataBit(QString("C%1Out").arg(index),false);
    }
    setDataBit(PLC::D::VentilationButterflyV1,true);
    setDataBit(PLC::D::CarIsInGroundState,true);  //the subsystems are in ground position
    setDataBit(PLC::D::PTIRIsInGroundState,true);
    setDataBit(PLC::D::WaterTreatmentIsInGroundState,true);
    setDataBit(PLC::D::StorageIsInGroundState,true);
    setDataBit(PLC::D::WSIsInGroundState,true);

    setDataBit(PLC::D::Source1StoragePosition,true); //sources are in storage position
    setDataBit(PLC::D::Source2StoragePosition,true);
    setDataBit(PLC::D::Source3StoragePosition,true);
    setDataBit(PLC::D::SafetyRod1,true); //sources are not locked
    setDataBit(PLC::D::SafetyRod2,true);
    setDataBit(PLC::D::SafetyRod3,true);
    setDataBit(PLC::D::E2Sensor,true); //the car is out

    setDataBit(PLC::D::ControlPoint1MasterKey,true); //The master key is not in the irradiation room
    setDataBit(PLC::D::EmergencyStop1,true);
    setDataBit(PLC::D::EmergencyStop2,true);
    setDataBit(PLC::D::EmergencyStop3NC,true);
    setDataBit(PLC::D::EmergencyStop4NC,true);
    setDataBit(PLC::D::EmergencyStop51NC,true);
    setDataBit(PLC::D::EmergencyStop6,true);
    setDataBit(PLC::D::EmergencyStop7,true);
    setDataBit(PLC::D::EmergencyButton1_Message,true);
    setDataBit(PLC::D::EmergencyButton2_Message,true);
    setDataBit(PLC::D::EmergencyButton3_Message,true);
    setDataBit(PLC::D::EmergencyButton4_Message,true);
    setDataBit(PLC::D::EmergencyButton5_Message,true);
    setDataBit(PLC::D::EmergencyButton6_Message,true);
    setDataBit(PLC::D::EmergencyButton7_Message,true);

    setDataBit(PLC::D::LockBoltNC,true);
    setDataBit(PLC::D::DoorBoltNC,true);
    setDataBit(PLC::D::ChainSWNC,true);
    setDataBit(PLC::D::PersonnelDoorClosed,true);
    setDataBit(PLC::D::InternalDoorHandleNC,true);
    setDataBit(PLC::D::DRM1TubeOk,true);
    setDataBit(PLC::D::DRM2TubeOk,true);
    setDataBit(PLC::D::DRM3TubeOk,true);
    setDataBit(PLC::D::DRM4TubeOk,true);
    setDataBit(PLC::D::DRM5TubeOk,true);
    setDataBit(PLC::D::DRM6TubeOk,true);
    setDataBit(PLC::D::DRM7TubeOk,true);
    setDataBit(PLC::D::DRM8TubeOk,true);

    setDataBit(PLC::D::DRM1Alarm,true);
    setDataBit(PLC::D::DRM2Alarm,true);
    setDataBit(PLC::D::DRM3Alarm,true);
    setDataBit(PLC::D::DRM4Alarm,true);
    setDataBit(PLC::D::DRM5Alarm,true);
    setDataBit(PLC::D::DRM6Alarm,true);
    setDataBit(PLC::D::DRM7Alarm,true);
    setDataBit(PLC::D::DRM8Alarm,true);

    setDataBit(PLC::D::CeilingPlug,true);
    setDataBit(PLC::D::B0CabinetOpenNC,true);
    setDataBit(PLC::D::SourceHoistCabinet,true);
    setDataBit(PLC::D::SourceHoistPressureOk,true);
    setDataBit(PLC::D::EarthquakeDetector,true);
    setDataBit(PLC::D::NoRadiation,true);
    setDataBit(PLC::D::RadiationMonitorFailure,true);
    setDataBit(PLC::D::RadiationHazard,true);
    setDataBit(PLC::D::VentilationAirFlowOK,true);
    setDataBit(PLC::D::OzoneDetector,true);
    setDataBit(PLC::D::PTIRAirPressureOk,true);
    setDataBit(PLC::D::StorageAirPressureOK,true);
    setDataBit(PLC::D::StorageDoorClosed,true);

    setDataBit(PLC::D::S1ValveClosed,true);
    setDataBit(PLC::D::S2ValveClosed,true);
    setDataBit(PLC::D::S3ValveClosed,true);
    setDataBit(PLC::D::S4ValveClosed,true);
    setDataBit(PLC::D::S5ValveClosed,true);
    setDataBit(PLC::D::S6ValveClosed,true);
    setDataBit(PLC::D::S7ValveClosed,true);

    //setDataBit(PLC::D::VentilationInputConnected,true);
    //setDataBit(PLC::D::VentilationOutputConnected,true);
    //setDataBit(PLC::D::WaterTreatmentConnected,true);
    //setDataBit(PLC::D::CarConnected,true);
    //setDataBit(PLC::D::StorageInputConnected,true);
    //setDataBit(PLC::D::StorageOutputConnected,true);
    //setDataBit(PLC::D::PTIRInputConnected,true);
    //setDataBit(PLC::D::PTIROutputConnected,true);
    //setDataBit(PLC::D::SecurityConnected,true);



    setDataBit(PLC::D::WaterTreatmentAutomaticMode,true); //Water Treatment is in automatic mode
    setDataWord(PLC::DW::Conductivity1,9);
    setDataWord(PLC::DW::Conductivity2,11);
    setDataWord(PLC::DW::Temperature,312);



    //lots of bits need proper initialization here

}

bool Fx5uEmulator::isBistableCylinder(QString name)
{
    return m_BistableCylinders.contains(name);
}







