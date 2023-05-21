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
int busNumber[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
int bus[12] = {100, 100, 200, 300, 400, 500, 600, 700, 800, 900, 110, 120};
int busStop[12] = {21, 22, 23, 24, 25, 14, 26, 27, 28, 29, 11, 10};

void initPin(int busStop[]);
int glitter(int pinNumber);
int turnOn(int pinNumber);
int turnOff(int pinNumber);
void initPin(int busStop[]);
int setStatus(MYSQL *connection, int busNumber, int busStop);
void *busThread(void *arg);
MYSQL_ROW sendSQL(MYSQL *connection, char query[]);

void *busThread(void *arg) {
    MYSQL *connection = NULL, conn;

    mysql_init(&conn);
    connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char *)NULL, 0);

    int *pinNumber = (int *)arg;
    int pin = *pinNumber;

    int *busStop = (int *)(arg + sizeof(int));
    int bus = *busStop;
    setStatus(connection, pin, bus);

    mysql_close(connection);
    return NULL;
}

int main(void) {
    if (wiringPiSetup() == -1)
        return -1;

    initPin(busStop);

    int i;
    for (i = 0; i < 12; i++) {
        int *args = malloc(sizeof(int) * 2);
        args[0] = busNumber[i];
        args[1] = busStop[i];
        pthread_create(&pthread[i], NULL, busThread, args);
    }

    for (i = 0; i < 12; i++) {
        pthread_join(pthread[i], NULL);
    }

    return 0;
}

MYSQL_ROW sendSQL(MYSQL *connection, char query[]) {
    MYSQL_ROW sql_row;
    MYSQL_RES *sql_result;
    int query_start;

    query_start = mysql_query(connection, query);
    if (query_start != 0) {
        fprintf(stderr, "SQL query error\n");
        return NULL;
    }

    sql_result = mysql_store_result(connection);
    sql_row = mysql_fetch_row(sql_result);

    return sql_row;
}

int setStatus(MYSQL *connection, int busNumber, int busStop) {
    MYSQL_ROW sql_row;

    char query[1000];
    int time_id = 0, endTime_id = 0, diff = 0;

    while (1) {
        sprintf(query, "SELECT COUNT(start), MIN(start), end FROM reservation WHERE status = 'INCOMPLETE' AND bus_id = %d", busNumber);

        sql_row = sendSQL(connection, query);

        if (sql_row != NULL && strcmp(sql_row[0], "0") != 0) {
            turnOn(busStop);

            time_id = atoi(sql_row[1]);
            endTime_id = atoi(sql_row[2]);
            sprintf(query, "SELECT (CASE WHEN TIMEDIFF(time, TIME(NOW())) < 0 THEN 'start' ELSE 'end' END) AS diff FROM timetable WHERE id = %d", time_id);
            sql_row = sendSQL(connection, query);

            if (sql_row != NULL && strcmp(sql_row[0], "start") == 0) {
                sprintf(query, "SELECT time <= TIME(DATE_ADD(NOW(), INTERVAL 1 MINUTE)) FROM timetable WHERE id = %d", endTime_id);
                sql_row = sendSQL(connection, query);

                diff = atoi(sql_row[0]);
                if (diff) {
                    glitter(busStop);

                    sprintf(query, "SELECT time < TIME(NOW()) FROM timetable WHERE id = %d", endTime_id);
                    sql_row = sendSQL(connection, query);
                    int end = atoi(sql_row[0]);
                    if (end == 1) {
                        turnOff(busStop);
                    }
                }
            }
        } else {
            turnOff(busStop);
        }
        return 0;
    }
}

void initPin(int busStop[]) {
    int i;
    for (i = 0; i < 12; i++) {
        pinMode(busStop[i], OUTPUT);
    }
}

int glitter(int pinNumber) {
    int count = 0;
    while (count < times) {
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
