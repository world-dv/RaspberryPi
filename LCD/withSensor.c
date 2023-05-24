#include <stdio.h>
#include <lcd.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql.h>
#include <wiringPiI2C.h>

#define LCD_RS 11
#define LCD_E 10
#define LCD_D4 0
#define LCD_D5 1
#define LCD_D6 2
#define LCD_D7 3

#define DB_HOST
#define DB_NAME 
#define DB_PASS 
#define DB_USER 
#define PORT 

#define Device_Address 0x48     /*Device Address/Identifier for MPU6050*/

#define PWR_MGMT_1   0x6B
#define SMPLRT_DIV   0x19
#define CONFIG       0x1A
#define GYRO_CONFIG  0x1B
#define INT_ENABLE   0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_YOUT_H 0x3D
#define ACCEL_ZOUT_H 0x3F
#define GYRO_XOUT_H  0x43
#define GYRO_YOUT_H  0x45
#define GYRO_ZOUT_H  0x47

#define BUS 13
#define BUS_STOP 112

void dayTime(int lcd);
void reserved(int lcd);
void seatSoon(int lcd, char time_meg[]);
void noReserved(int lcd);
void getOff(int lcd);
MYSQL_ROW sendSQL(MYSQL *connection, char query[]);
int setStatus(MYSQL *connection, int lcd);

void MPU6050_Init(void);
short read_raw_data(int addr);
void changeSeatGradient(float Ax, MYSQL*connection);

int fd;
char time_meg[1000];

int main(void) {

	int lcd;
	if(wiringPiSetup() == -1) return -1;

	if(lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0)) {
		printf("lcd init failed !\n");
		return -1;
	}

	float Ax = 0, Acc_x = 0;
	fd = wiringPiI2CSetup(Device_Address);
        MPU6050_Init();

	MYSQL *connection, conn;

        mysql_init(&conn);
        connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);
        if(connection == NULL) {
                fprintf(stderr, "SQL connection error\n");
                return 1;
        }

	int status = 0;
	while (1) {
		Acc_x = read_raw_data(ACCEL_XOUT_H);
		Ax = Acc_x / 16384.0;
		fprintf(stderr, "Ax = %f\n", Ax);
		changeSeatGradient(Ax, connection);

		status = setStatus(connection, lcd);

		switch(status) {
		case 1:
			reserved(lcd);
			break;
		case 2:
			getOff(lcd);
			break;
		case 3:
			seatSoon(lcd, time_meg);
			break;
		default :
			noReserved(lcd);
			break;
		}
		delay(1000);
	}
	return 0;
}

void MPU6050_Init(){

        wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);    /* Write to sample rate register */
        wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);    /* Write to power management register */
        wiringPiI2CWriteReg8 (fd, CONFIG, 0);           /* Write to Configuration register */
        wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24);     /* Write to Gyro Configuration register */
        wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);    /*Write to interrupt enable register */
}

short read_raw_data(int addr){
        short high_byte,low_byte,value;
        high_byte = wiringPiI2CReadReg8(fd, addr);
        low_byte = wiringPiI2CReadReg8(fd, addr+1);
        value = (high_byte << 8) | low_byte;
        return value;
}


void dayTime(int lcd) {
	time_t tim;
	struct tm *t;
	char buf[32];
	lcdPosition(lcd, 0, 1);
	tim = time(NULL);
	t = localtime(&tim);
	sprintf(buf, "At %02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
	lcdPuts(lcd, buf);
}

MYSQL_ROW sendSQL(MYSQL *connection, char query[]) {
	MYSQL_ROW sql_row;
	MYSQL_RES *sql_result;
	int query_start;

	query_start = mysql_query(connection, query);
	if(query_start != 0) {
		fprintf(stderr, "SQL query error\n");
		return 0;
	}

	sql_result = mysql_store_result(connection);
	sql_row = mysql_fetch_row(sql_result);

	return sql_row;
}

int setStatus(MYSQL *connection, int lcd) {
	MYSQL_ROW sql_row;
        char query[1000];

        sprintf(query, "select count(start), min(start), end from reservation where status = 'INCOMPLETE' and bus_id = %d", BUS);
        sql_row = sendSQL(connection, query);

	int status = 0;

        if(strcmp(sql_row[0], "0") != 0) {
		int time_id = atoi(sql_row[1]);
                int endTime_id = atoi(sql_row[2]);

                sprintf(query, "select (case when timediff(time, time(now())) < 0 then 'start' else 'end' end) as diff from timetable where id = %d", time_id);
                sql_row = sendSQL(connection, query);

                if(strcmp(sql_row[0], "start") == 0) {
                	sprintf(query, "select time <= time(date_add(now(), interval 1 minute)) from timetable where id = %d", endTime_id);
                        sql_row = sendSQL(connection, query);

			status = 1; //reserved(lcd);

                        int getOffSeat = atoi(sql_row[0]);
                        if(getOffSeat == 1) {
				status = 2; //getOff(lcd);

				sprintf(query, "select time < time(now()) from timetable where id = %d", endTime_id);
				sql_row = sendSQL(connection, query);
				int end = atoi(sql_row[0]);
				if(end == 1) {
					status = 0; //noReserved(lcd);
					sprintf(query, "update reservation set status = 'COMPLETE' where bus_id = %d and start = %d and end = %d", BUS, time_id, endTime_id);
                        		sendSQL(connection, query);
				}
                        }
               	} else {
			sprintf(query, "select time from timetable where id = %d", time_id);
                                sql_row = sendSQL(connection, query);

			strcpy(time_meg, sql_row[0]);
			status = 3; //seatSoon(lcd, sql_row);
			return status;
                }
       	} else {
		status = 0; //noReserved(lcd);
	}
	return status;
}

void reserved(int lcd) {
	lcdClear(lcd);
	lcdPosition(lcd, 0, 0);
	lcdPuts(lcd, "Reserved Seat");
	dayTime(lcd);
}
void seatSoon(int lcd, char time_meg[]) {
	lcdClear(lcd);
	lcdPosition(lcd, 0, 0);
	lcdPuts(lcd, "Reserved Seat");

	lcdPosition(lcd, 0, 1);
	lcdPuts(lcd, time_meg);
}
void noReserved(int lcd) {
	lcdClear(lcd);
	lcdPosition(lcd, 0, 0);
	lcdPuts(lcd, "Empty Seat");
	dayTime(lcd);
}
void getOff(int lcd) {
	lcdClear(lcd);
	lcdPosition(lcd, 0, 0);
	lcdPuts(lcd, "Get Off in");
	lcdPosition(lcd, 0, 1);
	lcdPuts(lcd, "1 minute");
}

void changeSeatGradient(float Ax, MYSQL *connection){
         MYSQL_ROW sql_row;

         char query[1000];

         if(Ax<-1){ //NoSeat
                sprintf(query, "update seat set gradient = 1 where bus_id = %d", BUS);
                sendSQL(connection, query);

                sprintf(query, "select min(id) from timetable where bus_stop_id = %d and time >= now()", BUS_STOP);
                sql_row = sendSQL(connection, query);

                int id = atoi(sql_row[0]);

                sprintf(query, "update timetable set seat_status =2 where seat_status = 0 and time >= time(now()) and bus_id = %d and id <= %d", BUS, id);
                sendSQL(connection, query);
         }

         if(Ax>-1 && Ax <0){ //EmptySeat
                sprintf(query, "update seat set gradient = 0 where bus_id = 1");
                sendSQL(connection, query);

                sprintf(query, "update timetable set seat_status =0 where seat_status = 2 and bus_id = 1");
                sendSQL(connection, query);
         }
}

