/*
 * client_example1.c
 *
 * This example is intended to be used with server_example3 or server_example_goose.
 */

#include "iec61850_client.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hal_thread.h"

typedef struct{
	unsigned int id;
	unsigned int adr;
	int val;
}sqldata;

int main(int argc, char** argv) {

    char* hostname;
    int tcpPort = 102;
    char* variable;
    int varlength;
    char str[80];
    int nb;
char query[100];


MYSQL *con = mysql_init(NULL);
	if (con == NULL)
	  {
		fprintf(stderr, "%s\n", mysql_error(con));
		exit(1);
	  }
	if(mysql_real_connect(con, "localhost", "root", "root",
		NULL, 0, NULL, 0) == NULL)
	  {
		fprintf(stderr, "%s\n", mysql_error(con));
	  	mysql_close(con);
		exit(1);
	  }

	if(mysql_select_db(con, "embedded")==0)
		printf ("Database selected \n");
	else
		printf("Failed connecting to the database: Error: \n");

	mysql_query(con, "select (*) from configuration")
	MYSQL_RES * res = mysql_use_result(con);
	MYSQL_ROW row;

	row = mysql_fetch_row(res);
	nb = atoi(row[0]);
	mysql_free_result(res);
	
	if (mysql_query(con "select id, name from configuration"))
	  {
		fprintf(stderr, "%s\n", mysql_error(con));
		exit(1);
	  }
	res = mysql_use_result(con);
	sqldata sqlData[nb];
	tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
	memset(tab_rp_registers, 0, nb * sizeof(uint16_t));

		sprintf(query, "select (value) from solar panel data where id = %i" , sglData.id);

    variable = argv[1];
    strcpy(str, "SampleIEDDevice1/");
    strcat(str, variable);

    printf("String: %s\n", str);
    if (argc > 2)
        hostname = argv[2];
    else
        hostname = "localhost";

    if (argc > 3)
        tcpPort = atoi(argv[3]);

    IedClientError error;

    IedConnection con = IedConnection_create();

    IedConnection_connect(con, &error, hostname, tcpPort);

    if (error == IED_ERROR_OK) {

        /* read an analog measurement value from server */
        MmsValue* value = IedConnection_readObject(con, &error, str, MX);

        if (value != NULL) {
            float fval = MmsValue_toFloat(value);
            printf("Value: %f\n", fval);
            MmsValue_delete(value);
        }

        IedConnection_close(con);
    }
    else {
        printf("Failed to connect to %s:%i\n", hostname, tcpPort);
    }

    IedConnection_destroy(con);
}


