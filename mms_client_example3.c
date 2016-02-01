
#include <stdlib.h>
#include <stdio.h>
#include "mms_client_connection.h"

int main(int argc, char** argv) {

	char* hostname;
	char* variable;
	int tcpPort = 102;
//	char varname; //variable for searching with just the name 
	variable = argv[1];

	if (argc > 2)
		hostname = argv[2];
	else
		hostname = "localhost";

	if (argc > 3)
		tcpPort = atoi(argv[3]);

//	if (argc > 4)
//		varname = atoi(argv[4]); // varname is the 4th command given when client is run

	MmsConnection con = MmsConnection_create();

	MmsError error;

	if (!MmsConnection_connect(con, &error, hostname, tcpPort)) {
  		printf("MMS connect failed!\n");
		goto exit;
	}
	else
		printf("MMS connected.\n\n");

	//MmsValue* value = MmsConnection_readVariable(con, &error, "simpleIOGenericIO", variable);

	 MmsValue* value = IedConnection_readObject(con, &error, "SampleIEDDevice1/MMXN1.Vol.mag.f");

	if (value == NULL)
		printf("reading value failed!\n");
	else {
		float fval = MmsValue_toFloat(value);
            	printf("read float value: %f\n", fval);
		MmsValue_delete(value);
	}

exit:
	MmsConnection_destroy(con);
}

