#include "fx5tester.h"



void Fx5Tester::onConnected() {
    qDebug() << "Connected, starting the test...";
    runNextTest();
}

void Fx5Tester::onReadyRead() {
    _rxByteArray.append(_socket.readAll());
    moveToCBuffer();

    _status = fx5_parse_response(&_transaction,&_rx_cbuffer);
    if (_status != FX5_ST_ERR_NEED_MORE) {
        qDebug() << "Response code is: " << _transaction.response.response_code;
        if (_status == FX5_ST_OK) {//itt kellene meghívni a következő tesztet
            qDebug() << "Executing the next test...";
            runNextTest();
        } else {//feladjuk
            qDebug() << "Giving up...";
            qApp->quit();
        }
    }
}

void Fx5Tester::moveToCBuffer() {
    for(int k = 0; k < _rxByteArray.size();k++) {
        bufferAddToEnd(&_rx_cbuffer,(uint8_t)_rxByteArray[k]);
    }
    _rxByteArray.clear();
}

Fx5Tester::Fx5Tester(int port, QObject *parent) : QObject(parent) {
    bufferInit(&_rx_cbuffer,_rx_buffer,BUFFER_SIZE);
    fx5_transaction_init(&_transaction);
    _currentTest = 0;

    connect(&_socket,&QTcpSocket::readyRead, this, &Fx5Tester::onReadyRead);
    _socket.connectToHost(QHostAddress::LocalHost,port);
}


bool Fx5Tester::hasMoreTest() {
    return _currentTest < _requests.size();
}

void Fx5Tester::runNextTest() {
    _transaction.request.address = _requests[_currentTest].address;
    _transaction.request.command = _requests[_currentTest].command;
    _transaction.request.count = _requests[_currentTest].count;
    _transaction.request.device = _requests[_currentTest].device;
    for(int k = 0; k < _transaction.request.count;k++) {
        _transaction.request.values[k]  = _requests[_currentTest].values[k];
    }

    _status = fx5_build_request(&_transaction,_tx_buffer,BUFFER_SIZE,&_actual_size);

    if (_status != FX5_ST_OK) {
        qDebug() << "Test" << _currentTest << " failed!";
        qApp->quit();
    }
    _currentTest++;

    _socket.write((const char *)_tx_buffer,_actual_size);
}
