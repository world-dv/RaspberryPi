#include <stdio.h>
#include <wiringPi.h> //wriringPi > LED 사용

#include <mysql.h> //mysql 사용 
#include <string.h>

//#define DB_HOST "root.crvhhmocmghd.eu-north-1.rds.amazonaws.com"
//#define DB_NAME "test"
//#define DB_USER "root"
#define DB_HOST "bus.crvhhmocmghd.eu-north-1.rds.amazonaws.com"
#define DB_NAME "bus"
#define DB_PASS "12345678"
#define DB_USER "bus"

int main(void) {
	int i;
	if(wiringPiSetup() == -1) return -1; //wiringPi 오류 시 종료
	pinMode(29, OUTPUT); //Led 출력 핀 29

	MYSQL *connection=NULL, conn; 
	MYSQL_RES *sql_result;
	MYSQL_ROW sql_row;
	int query_start;

	while(1) {
	mysql_init(&conn); //mysql 초기화

	connection=mysql_real_connect(&conn,DB_HOST,DB_USER,DB_PASS,DB_NAME,3306,(char*)NULL,0); //mysql 연결
	if(connection==NULL) { //연결 실패 시
		fprintf(stderr, "SQL connection error\n");
		return 1; //에러 메세지 출력 후 종료
	}
	query_start=mysql_query(connection, "select status from reservation where bus_ID=100;\n"); //쿼리문 실행
	if(query_start!=0) { //쿼리문 실행 오류 시
		fprintf(stderr, "SQL query error");
		return 1; //에러 메세지 출력 후 종료
	}
	sql_result=mysql_store_result(connection); //연결한 db에서 쿼리문 결과 가져옴
	sql_row=mysql_fetch_row(sql_result); //한 행을 받아옴
	printf("%s\n",sql_row[0]); //첫번째 값 출력

	if(strcmp(sql_row[0],"1")==0){ //값이 1이라면
		digitalWrite(29,1); //29번 핀 on
	} else {
		digitalWrite(29,0); //아니라면 off
	}
	}
	return 0; //종료
}
