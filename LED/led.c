#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <mysql.h>
#include <string.h>
#include <stdlib.h>

#define times 60 //LED 점등, 점멸 시간 

/*AWS RDS DB 연결 정보 정의*/
#define DB_HOST  
#define DB_NAME 
#define DB_PASS 
#define DB_USER
#define PORT

pthread_t pthread[12]; //thread 정의
int busNumber[12] = {1, 2, 3, 11, 12, 6, 7, 8, 9, 10, 13}; //버스 ID
int bus[11] = {100, 100, 200, 110, 120, 500, 600, 700, 800, 900, 5000}; //버스 번호
int busStop[11] = {29, 28, 27, 26, 11, 10, 25, 24, 23, 22, 21}; //버스 Pin 번호

void initPin(int busStop[]); //LED Pin 초기화 함수
int glitter(int pinNumber); //LED 점멸 함수
int turnOn(int pinNumber); //LED 점등 함수
int turnOff(int pinNumber); //LED 소등 함수
void  setStatus(MYSQL *connection, int busNumber, int busStop); //LED 상태 설정 함수
void *busThread(void *arg); //스레드 함수
MYSQL_ROW sendSQL(MYSQL *connection, char query[]); //SQL 전송 함수

void *busThread(void *arg) {
	MYSQL *connection = NULL, conn;

        mysql_init(&conn); //MYSQL 초기화
				//MYSQL 연결
        connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);

	int *pinNumber = (int*)arg;
	int pin = *pinNumber; //버스 Pin 번호 할당

	int *busStop = (int*)(arg+sizeof(int));
	int bus = *busStop; //버스 ID 할당
	setStatus(connection, pin, bus); //버스 상태 설정

	mysql_close(connection); //MYSQL 연결 해제
	return NULL;
}

int main(void) {
	if(wiringPiSetup() == -1) return -1;

	initPin(busStop); //버스 Pin 번호 초기화

	for(int i = 0; i < 12; i++) {
		int *args = malloc(sizeof(int) * 2);
		args[0] = busNumber[i]; //버스 ID 할당
		args[1] = busStop[i]; //버스 Pin 번호 할당
		pthread_create(&pthread[i], NULL, busThread, (void *)args);
		//각 버스마다 스레드 할당
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

	query_start = mysql_query(connection, query); //쿼리 전송

	sql_result = mysql_store_result(connection); //쿼리 저장
	sql_row = mysql_fetch_row(sql_result); //쿼리의 첫번째 행

	return sql_row; //SQL 쿼리 결과 반환
}

void setStatus(MYSQL *connection, int busNumber, int busStop) {
	MYSQL_ROW sql_row;
	int status = 0;
	char query[1000];
	int time_id = 0, endTime_id = 0, diff = 0;
	fprintf(stderr, "Start LED\n");
	while(1) {
		//예약된 버스 정보 가져옴
		sprintf(query, "select count(start), min(start), end from reservation where status = 'INCOMPLETE' and bus_id = %d", busNumber);

		sql_row = sendSQL(connection, query);
		int result = atoi(sql_row[0]);
		
		//결과가 있다면 점등
		if(sql_row != NULL && result != 0) {
		        status = 1; //turnOn(busStop);

			time_id =  atoi(sql_row[1]); //예약된 출발 정류장 ID
			endTime_id = atoi(sql_row[2]); //예약된 도착 정류장 ID

			//현재 시간을 기준으로 출발 정류장을 지났을 경우 'start' 아니라면 'end' 
			sprintf(query, "select (case when timediff(time, time(now())) < 0 then 'start' else 'end' end) as diff from timetable where id = %d", time_id);
			sql_row = sendSQL(connection, query);

			char results[1000];
			strcpy(results, sql_row[0]);
			//출발 정류장 지났을 경우
			if(sql_row != NULL && strcmp(results, "start") == 0) {
				//도착 정류장까지 1분 남았을 경우
				sprintf(query, "select time <= time(date_add(now(), interval 1 minute)) from timetable where id = %d", endTime_id);
				sql_row = sendSQL(connection, query);
				fprintf(stderr, "%d %d run2\n", time_id, endTime_id);

				diff = atoi(sql_row[0]);
				if(sql_row != NULL && diff == 1) {
					//LED 점멸
					status = 2;//glitter(busStop);
						//도착 정류장에 도착했을 경우	
	        			sprintf(query, "select time < time(now()) from timetable where id = %d", endTime_id);
        	  			sql_row = sendSQL(connection, query);
           	 			int end = atoi(sql_row[0]);
           				if(sql_row != NULL && end == 1) {
										//LED 소등
                    								status = 0; //turnOff(busStop);
										//예약 정보 업데이트 'Incomplete' -> 'Complete'
										sprintf(query, "update reservation set status = 'COMPLETE' where bus_id = %d and start = %d and end = %d", busNumber, time_id, endTime_id);
										sendSQL(connection, query);
										//좌석 정보 업데이트 '1' -> '0' 
										sprintf(query, "update timetable set seat_status = 0 where bus_id = %d and id >= %d and id <= %d", busNumber, time_id, endTime_id);
										sendSQL(connection, query);
            				}
				}
			} else { //출발 정류장을 지나지 않았을 경우
				sprintf(query, "select time <= time(date_add(now(), interval 1 minute)) from timetable where id = %d", time_id);
				sql_row = sendSQL(connection, query);
				diff = atoi(sql_row[0]);
				if(sql_row != NULL && diff == 1) {
					//출발 정류장까지 1분 남았을 경우 LED 점멸
					status = 2;
				} else { //출발 정류장을 지나지 않을 경우 LED 점등
					status = 1;
				}
			}
		} else { //예약된 좌석이 없을 경우 소등
      			status = 0; //turnOff(busStop);
		}

		switch(status){
		case 1:
			turnOn(busStop); //상태가 1일 경우 점등
			break;
		case 2:
			glitter(busStop); //상태가 2일 경우 점멸
			break;
		default:
			turnOff(busStop); //상태가 1, 2가 아닐 경우 소등
			break;
		} 
	delay(1000);
	}
}

/*LED Pin 초기화*/
void initPin(int busStop[]) {
        for(int i = 0; i < 12; i++) {
                pinMode(busStop[i], OUTPUT);
        }
}

/*LED 상태 정의*/
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
} //LED 점멸


int turnOn(int pinNumber) {
	digitalWrite(pinNumber, 1);
	delay(1000);
	return 0;
} //LED 점등

int turnOff(int pinNumber) {
	digitalWrite(pinNumber, 0);
	delay(1000);
	return 0;
} //LED 소등
