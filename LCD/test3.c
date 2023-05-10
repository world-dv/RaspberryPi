#include <stdio.h>
#include <lcd.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <mysql.h>

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

void dayTime(int lcd);
int connectionSQL(int lcd);

int main(void) {
	int lcd;
	if(wiringPiSetup() == -1) return -1;

	if(lcd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7, 0, 0, 0, 0)) {
		printf("lcd init failed !\n");
		return -1;
	}

	connectionSQL(lcd);
	dayTime(lcd);

	return 0;
}

void dayTime(int lcd) {
	time_t tim;
	struct tm *t;
	char buf[32];
	while(1) {
		lcdPosition(lcd, 0, 1);
		tim = time(NULL);
		t = localtime(&tim);
		sprintf(buf, "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
		lcdPuts(lcd, buf);
	}
}

int connectionSQL(int lcd) {
	MYSQL *connection = NULL, conn;
	MYSQL_RES *sql_result;
	MYSQL_ROW sql_row;
	int query_start;

	while(1) {
		mysql_init(&conn);
		connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);
		if(connection == NULL) {
			fprintf(stderr, "SQL connection error\n");
			return 1;
		}
		char query[1000];
		sprintf(query, "select status from reservation where bus_id=1");
		query_start  = mysql_query(connection, query);
		if(query_start != 0) {
			fprintf(stderr, "SQL query error\n");
			return 1;
		}

		sql_result = mysql_store_result(connection);
		sql_row = mysql_fetch_row(sql_result);
		printf("%s\n", sql_row[0]);

		char buf[1000];
		lcdPosition(lcd, 0,0);
		if(strcmp(sql_row[0], "1") == 0) {
			sprintf(buf, "Reserved seats at %02dh:%02dm"); //scroll
			lcdPuts(lcd, buf);
		} else {
			sprintf(buf, "This is an empty seat"); //scroll
			lcdPuts(lcd, buf);
		}

	}
}
