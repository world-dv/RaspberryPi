#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <mysql.h>
#include <string.h>
#include <stdlib.h>

#define times 60

#define DB_HOST
#define DB_NAME 
#define DB_PASS 
#define DB_USER
#define PORT

pthread_t pthread[12];
int busNumber[12] = {1, 2, 3, 11, 12, 6, 7, 8, 9, 10, 13};
int bus[11] = {100, 100, 200, 110, 120, 500, 600, 700, 800, 900, 5000};
int busStop[11] = {29, 28, 27, 26, 11, 10, 25, 24, 23, 22, 21};

void initPin(int busStop[]);
int glitter(int pinNumber);
int turnOn(int pinNumber);
int turnOff(int pinNumber);
void  setStatus(MYSQL *connection, int busNumber, int busStop);
void *busThread(void *arg);
MYSQL_ROW sendSQL(MYSQL *connection, char query[]);

void *busThread(void *arg) {
	MYSQL *connection = NULL, conn;

        mysql_init(&conn);
        connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);

	int *pinNumber = (int*)arg;
	int pin = *pinNumber;

	int *busStop = (int*)(arg+sizeof(int));
	int bus = *busStop;
	setStatus(connection, pin, bus);

	mysql_close(connection);
	return NULL;
}

int main(void) {
	if(wiringPiSetup() == -1) return -1;

	initPin(busStop);

	for(int i = 0; i < 12; i++) {
		int *args = malloc(sizeof(int) * 2);
		args[0] = busNumber[i];
		args[1] = busStop[i];
		pthread_create(&pthread[i], NULL, busThread, (void *)args);
	}
	for(int i = 0; i < 12; i++) {
		pthread_join(pthread[i], NULL);
	}
	return 0;
}

MYSQL_ROW sendSQL(MYSQL *connection, char query[]) {
	MYSQL_ROW sql_row;
	MYSQL_RES *sql_result;
	int query_start;

	query_start = mysql_query(connection, query);

	sql_result = mysql_store_result(connection);
	sql_row = mysql_fetch_row(sql_result);

	return sql_row;
}

void setStatus(MYSQL *connection, int busNumber, int busStop) {
	MYSQL_ROW sql_row;
	int status = 0;
	char query[1000];
	int time_id = 0, endTime_id = 0, diff = 0;
	fprintf(stderr, "Start LED\n");
	while(1) {
		sprintf(query, "select count(start), min(start), end from reservation where status = 'INCOMPLETE' and bus_id = %d", busNumber);

		sql_row = sendSQL(connection, query);
		int result = atoi(sql_row[0]);

		if(sql_row != NULL && result != 0) {
		        status = 1; //turnOn(busStop);

			time_id =  atoi(sql_row[1]);
			endTime_id = atoi(sql_row[2]);

			sprintf(query, "select (case when timediff(time, time(now())) < 0 then 'start' else 'end' end) as diff from timetable where id = %d", time_id);
			sql_row = sendSQL(connection, query);

			char results[1000];
			strcpy(results, sql_row[0]);

			if(sql_row != NULL && strcmp(results, "start") == 0) {
				sprintf(query, "select time <= time(date_add(now(), interval 1 minute)) from timetable where id = %d", endTime_id);
				sql_row = sendSQL(connection, query);
				fprintf(stderr, "%d %d run2\n", time_id, endTime_id);

				diff = atoi(sql_row[0]);
				if(sql_row != NULL && diff == 1) {
					status = 2;//glitter(busStop);

	        	                sprintf(query, "select time < time(now()) from timetable where id = %d", endTime_id);
        	        	        sql_row = sendSQL(connection, query);
                       			int end = atoi(sql_row[0]);
                        		if(sql_row != NULL && end == 1) {
                                		status = 0; //turnOff(busStop);
						sprintf(query, "update reservation set status = 'COMPLETE' where bus_id = %d and start = %d and end = %d", busNumber, time_id, endTime_id);
						sendSQL(connection, query);
						sprintf(query, "update timetable set seat_status = 0 where bus_id = %d and id >= %d and id <= %d", busNumber, time_id, endTime_id);
						sendSQL(connection, query);

                        		}
				}
			} else {
				sprintf(query, "select time <= time(date_add(now(), interval 1 minute)) from timetable where id = %d", time_id);
				sql_row = sendSQL(connection, query);
				diff = atoi(sql_row[0]);
				if(sql_row != NULL && diff == 1) {
					status = 2;
				} else {
					status = 1;
				}
			}
		} else {
      			status = 0; //turnOff(busStop);
		}

		switch(status){
		case 1:
			turnOn(busStop);
			break;
		case 2:
			glitter(busStop);
			break;
		default:
			turnOff(busStop);
			break;
		}
	delay(1000);
	}
}

void initPin(int busStop[]) {
        for(int i = 0; i < 12; i++) {
                pinMode(busStop[i], OUTPUT);
        }
}


int glitter(int pinNumber) {
	digitalWrite(pinNumber, 1);
	delay(250);
	digitalWrite(pinNumber, 0);
	delay(250);
	digitalWrite(pinNumber, 1);
	delay(250);
	digitalWrite(pinNumber, 0);
	delay(250);
	return 0;
}

int turnOn(int pinNumber) {
	digitalWrite(pinNumber, 1);
	delay(1000);
	return 0;
}

int turnOff(int pinNumber) {
	digitalWrite(pinNumber, 0);
	delay(1000);
	return 0;
}
