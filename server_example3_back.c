/*
 *  server_example3.c
 *
 *  - How to use simple control models
 *  - How to serve analog measurement data
 */

#include "iec61850_server.h"
#include "hal_thread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "static_model.h"

/* import IEC 61850 device model created from SCL-File */
extern IedModel iedModel;

static int running = 0;
static IedServer iedServer = NULL;

void
sigint_handler(int signalId)
{
    running = 0;
}

bool
controlHandlerForBinaryOutput(void* parameter, MmsValue* value, bool test)
{
    if (MmsValue_getType(value) == MMS_BOOLEAN) {
        printf("received binary control command: ");

        if (MmsValue_getBoolean(value))
            printf("on\n");
        else
            printf("off\n");
    }
    else
        return false;

    uint64_t timeStamp = Hal_getTimeInMs();

    return true;
}


static void
connectionHandler (IedServer self, ClientConnection connection, bool connected, void* parameter)
{
    if (connected)
        printf("Connection opened\n");
    else
        printf("Connection closed\n");
}

int
main(int argc, char** argv)
{

    iedServer = IedServer_create(&iedModel);

    /* MMS server will be instructed to start listening to client connections. */
    IedServer_start(iedServer, 102);

    if (!IedServer_isRunning(iedServer)) {
        printf("Starting server failed! Exit.\n");
        IedServer_destroy(iedServer);
        exit(-1);
    }

    running = 1;

    signal(SIGINT, sigint_handler);

    float t = 0.f;

    while (running) {
        uint64_t timestamp = Hal_getTimeInMs();

        t += 0.1f;

        float an1 = sinf(t);
        float an2 = sinf(t + 1.f);
        float an3 = sinf(t + 2.f);
        float an4 = sinf(t + 3.f);

        IedServer_lockDataModel(iedServer);


	IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_Device1_MMXN1_Vol_mag_f, an1);
	IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_Device1_MMXN1_Vol_t, timestamp);

        IedServer_unlockDataModel(iedServer);

        Thread_sleep(100);
    }

    /* stop MMS server - close TCP server socket and all client sockets */
    IedServer_stop(iedServer);

    /* Cleanup - free all resources */
    IedServer_destroy(iedServer);

} /* main() */
