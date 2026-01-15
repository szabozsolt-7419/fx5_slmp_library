#ifndef FX5TESTER_H
#define FX5TESTER_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QCoreApplication>

#include "buffer.h"
#include "fx5.h"


class Fx5Tester :public QObject {
public:
    Fx5Tester(int port,QObject *parent = nullptr);
private slots:
    void onConnected();
    void onReadyRead();
private:
    static constexpr int BUFFER_SIZE = 256;
    void moveToCBuffer();
    bool hasMoreTest();
    void runNextTest();
private:
    QTcpSocket _socket;
    QByteArray _rxByteArray;
    cBuffer _rx_cbuffer;
    fx5_status_t _status;
    uint8_t _tx_buffer[BUFFER_SIZE];
    uint8_t _rx_buffer[BUFFER_SIZE];
    uint16_t _actual_size;
    fx5_transaction_t _transaction;
    static QList<fx5_request_t> _requests;
    int _currentTest;



};
#endif // FX5TESTER_H
