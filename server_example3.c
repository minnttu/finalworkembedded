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
#include <mysql/mysql.h>
#include <my_global.h>
#include "static_model.h"

/* import IEC 61850 device model created from SCL-File */
extern IedModel iedModel;

static int running = 0;
static IedServer iedServer = NULL;

typedef struct {
        unsigned int id;
        char* name;
} sqldata;

int getNumberOfConfigurationEntries(MYSQL *con) ;

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
     char query[100];
     int nb;
     int i;
     int counter = 0;
     MYSQL_ROW row;
     MYSQL *con = mysql_init(NULL);

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

    if (con == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
    }
    if (mysql_real_connect(con, "localhost", "root", "root",
        NULL, 0, NULL, 0) == NULL) {
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
    }

    if(mysql_select_db(con, "embedded")==0)/*success*/
            printf( "Database Selected\n");
    else
       printf( "Failed to connect to Database: Error: \n");


    while (running) {
        uint64_t timestamp = Hal_getTimeInMs();

        IedServer_lockDataModel(iedServer);
	nb = getNumberOfConfigurationEntries(con);
	sqldata sqlData[nb];

	if (mysql_query(con, "select id, name from configuration")) {
        	fprintf(stderr, "%s\n", mysql_error(con));
        	exit(1);
    	}

    	MYSQL_RES * res = mysql_use_result(con);
	counter = 0;
	while ((row = mysql_fetch_row(res)) != NULL) {
        	sqlData[counter].id = atoi(row[0]);
        	sqlData[counter].name = row[1];
        	counter++;
	}
	mysql_free_result(res);

	for (i = 0; i < counter; i++) {
        	sprintf(query, "select value from solar_panel_data where id = %i", sqlData[i].id);
	        mysql_query(con, query);
        	res = mysql_use_result(con);
        	row = mysql_fetch_row(res);
		char * name = sqlData[i].name;
		row = mysql_fetch_row(res);
		IedServer_updateFloatAttributeValue(iedServer, IEDMODEL_Device1_MMXN1_Vol_mag_f, atoi(row[0]));
	        IedServer_updateUTCTimeAttributeValue(iedServer, IEDMODEL_Device1_MMXN1_Vol_t, timestamp);
		mysql_free_result(res);
        }
	
        IedServer_unlockDataModel(iedServer);

        Thread_sleep(100);
    }

    mysql_close(con);
    /* stop MMS server - close TCP server socket and all client sockets */
    IedServer_stop(iedServer);

    /* Cleanup - free all resources */
    IedServer_destroy(iedServer);

} /* main() */

int getNumberOfConfigurationEntries(MYSQL *con) {
    int nb;
    mysql_query(con, "select count(*) from configuration");
    MYSQL_RES * res = mysql_use_result(con);
    MYSQL_ROW row = mysql_fetch_row(res);
    nb = atoi(row[0]);
    mysql_free_result(res);
    return nb;
}
