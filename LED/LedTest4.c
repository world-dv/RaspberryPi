#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <mysql.h>
#include <string.h>

#define times 2

#define DB_HOST
#define DB_NAME 
#define DB_PASS 
#define DB_USER 
#define PORT 

pthread_t pthread[12];
int busNumber[12] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000, 1100, 1200};
int busStop[12] = { 21, 22, 23, 24, 25, 14, 26, 27, 28, 29, 11, 10};

int glitter(int pinNumber);
int turnOn(int pinNumber);
int turnOff(int pinNumber);

void *busThread(void *arg) {
	int *pinNumber = (int*)arg[0];
	int pin = *pinNumber;

	int *busStop = (int*)arg[1];
	int bus = *busStop;
	connectionSQL(pin, bus);
}

int main(void) {
	if(wiringPiSetup() == -1) return -1;

	for(int i = 0; i < 12; i++) {
		pinMode(busStop[i], OUTPUT);
	}

	int args[2];
	for(int i = 0; i < 12; i++) {
		args[0] = busNumber[i];
		args[1] = busStop[i];
		pthread_create(&pthread[i], NULL, busThread, (void *)args);
	}
	for(int i = 0; i < 12; i++) {
		pthread_join(pthread[i], NULL);
	}
	return 0;
}

void connectSQL(int busNumber, int busStop) {
	MYSQL *connection = NULL, conn;
	MYSQL_RES *sql_result;
	MYSQL_ROW sql_row;
	int query_start;

	while(1) {
		mysql_init(&conn);
		connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);
		if(connection == NULL) {
			fprintf(stderr, "SQL Connection error\n");
			return 1;
		}

		char query[1000];
		sprint(query, "select status from reservation where ID = %d",busNumber);
		query_start = mysql_query(connection, query);
		if(query_start != 0) {
			fprintf(stderr, "SQL query error\n");
			return 1;
		}

		sql_result = mysql_store_result(connection);
		sql_row = mysql_fetch_row(sql_result);
		printf("%s\n", sql_row[0]);

		if(strcmp(sql_row[0], "1") == 0) {
			turnOn(busStop);
		} else if(strcmp(sql_row[0], "0") == 0) {
			turnOff(busStop);
		}
	}
}

int glitter(int pinNumber) {
	int count = 0;
	while(count < times) {
		digitalWrite(pinNumber, 1);
		delay(500);
		digitalWrite(pinNumber, 0);
		delay(500);
		count++;
	}
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
