/*
 * Copyright © 2001-2010 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * COMPILE WITH
 *  gcc random-test-client.c -o random-test-client `pkg-config --libs --cflags libmodbus``mysql_config --cflags --libs`
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <mysql.h>
#include <my_global.h>

/* The goal of this program is to check all major functions of
   libmodbus:
   - write_coil
   - read_bits
   - write_coils
   - write_register
   - read_registers
   - write_registers
   - read_registers

   All these functions are called with random values on a address
   range defined by the following defines.
*/
#define LOOP             1
#define SERVER_ID        1
#define ADDRESS_START    0
#define ADDRESS_END      2

typedef struct {
        unsigned int id;
        unsigned int adr;
        int val;
} sqldata;

int getNumberOfConfigurationEntries(MYSQL *con) ;
MYSQL* connectToDatabase(void);
void insertValuesIntoDatabase(int numberOfRows, MYSQL *con, sqldata sqlData[]);
int readFromRegister(modbus_t *ctx, int addr, uint16_t *tab_rp_registers);

int main(void)
{
    modbus_t *ctx;
    int rc;
    int addr;
    int nb;
    uint16_t *tab_rp_registers;
    char query[100];
    int reg_address;
    int database_id;
    int counter = 0;
    double value;
    MYSQL_ROW row;

    /* RTU */
    ctx = modbus_new_rtu("/dev/ttyUSB0", 9600, 'N', 8, 1);
    modbus_set_slave(ctx, SERVER_ID);

    if (modbus_connect(ctx) == -1) {
        fprintf(stderr, "Connection failed: %s\n",
                modbus_strerror(errno));
        modbus_free(ctx);
        return -1;
    }

    MYSQL *con = connectToDatabase();

    nb = getNumberOfConfigurationEntries(con);

    if (mysql_query(con, "select id, address from configuration")) {
        fprintf(stderr, "%s\n", mysql_error(con));
        exit(1);
    }

    MYSQL_RES * res = mysql_use_result(con);

    sqldata sqlData[nb];
    tab_rp_registers = (uint16_t *) malloc(nb * sizeof(uint16_t));
    memset(tab_rp_registers, 0, nb * sizeof(uint16_t));

    /* output table name */
    while ((row = mysql_fetch_row(res)) != NULL) {
        database_id = atoi(row[0]);
        reg_address = atoi(row[1]);
        addr = reg_address - 30000;
	value = readFromRegister(ctx, addr, tab_rp_registers);
        sqlData[counter].id = database_id;
        sqlData[counter].adr = reg_address;
        sqlData[counter].val = value;
        counter++;
    }
    mysql_free_result(res);

    insertValuesIntoDatabase(counter, con, sqlData);
    free(tab_rp_registers);

    /* Close the connection */
    modbus_close(ctx);
    modbus_free(ctx);
    mysql_close(con);

    return 0;
}

int getNumberOfConfigurationEntries(MYSQL *con) {
    int nb;
    mysql_query(con, "select count(*) from configuration");
    MYSQL_RES * res = mysql_use_result(con);
    MYSQL_ROW row = mysql_fetch_row(res);
    nb = atoi(row[0]);
    mysql_free_result(res);
    return nb;
}

MYSQL* connectToDatabase() {
    MYSQL *con = mysql_init(NULL);

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

    return con;
}

void insertValuesIntoDatabase(int numberOfRows, MYSQL *con, sqldata sqlData[]) {
    int i = 0;
    char query[100];
    MYSQL_RES * res;
    MYSQL_ROW row;
    for (i = 0; i < numberOfRows; i++) {
        sprintf(query, "select count(*) from solar_panel_data where id = %i", sqlData[i].id);
        mysql_query(con, query);
        res = mysql_use_result(con);
        row = mysql_fetch_row(res);

        if (atoi(row[0]) == 0) {
            sprintf(query, "INSERT INTO solar_panel_data (id, address, value) VALUES (%i, %i, %d)", sqlData[i].id, sqlData[i].adr, sqlData[i].val);
        } else {
            sprintf(query, "UPDATE solar_panel_data SET value = %d, reading_date = CURRENT_TIMESTAMP WHERE id = %i", sqlData[i].val, sqlData[i].id);
        }
        mysql_free_result(res);
        if (mysql_query(con, query)) {
            printf("ERROR writing to database\n");
        }
    }
}

int readFromRegister(modbus_t *ctx, int addr, uint16_t *tab_rp_registers) {
	int rc = modbus_read_input_registers(ctx, addr, 1, tab_rp_registers);
    if (rc == -1) {
	printf("ERROR modbus_read_input_registers (%d)\n", rc);
    } else{
        printf("Address = %d, value %d \n",addr, tab_rp_registers[0]);
    }
    return tab_rp_registers[0];
}
