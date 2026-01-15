#include <QCoreApplication>
#include "fx5uemulator.h"


#include "fx5tester.h"


//ide jönnek a tesztek
QList<fx5_request_t> Fx5Tester::_requests = {
    {FX5_DEV_D, FX5_CMD_BATCH_READ, 0x000000, 1, {}}
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    const int PORT = 5588;

    Fx5uEmulator emulator;
    emulator.setPort(PORT);

    emulator.start();

    Fx5Tester tester(PORT);


    return app.exec();
}
