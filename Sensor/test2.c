#include <wiringPiI2C.h>
#include <stdlib.h>
#include <stdio.h>
#include <wiringPi.h>
#include <string.h>
#include <time.h>
#include <mysql.h>

#define Device_Address 0x48	/*Device Address/Identifier for MPU6050*/

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

#define DB_HOST
#define DB_NAME 
#define DB_PASS 
#define DB_USER
#define PORT 

int fd;
void changeSeatGradient(float Ax, float Ay, MYSQL*connection);

void MPU6050_Init(){

	wiringPiI2CWriteReg8 (fd, SMPLRT_DIV, 0x07);	/* Write to sample rate register */
	wiringPiI2CWriteReg8 (fd, PWR_MGMT_1, 0x01);	/* Write to power management register */
	wiringPiI2CWriteReg8 (fd, CONFIG, 0);		/* Write to Configuration register */
	wiringPiI2CWriteReg8 (fd, GYRO_CONFIG, 24);	/* Write to Gyro Configuration register */
	wiringPiI2CWriteReg8 (fd, INT_ENABLE, 0x01);	/*Write to interrupt enable register */

	}
short read_raw_data(int addr){
	short high_byte,low_byte,value;
	high_byte = wiringPiI2CReadReg8(fd, addr);
	low_byte = wiringPiI2CReadReg8(fd, addr+1);
	value = (high_byte << 8) | low_byte;
	return value;
}

void ms_delay(int val){
	int i,j;
	for(i=0;i<=val;i++)
		for(j=0;j<1200;j++);
}

int main(){

        float Acc_x,Acc_y,Acc_z;
        float Gyro_x,Gyro_y,Gyro_z;
        float Ax=0, Ay=0, Az=0;
        float Gx=0, Gy=0, Gz=0;

	fd = wiringPiI2CSetup(Device_Address);   /*Initializes I2C with device Address*/
	MPU6050_Init();		                 /* Initializes MPU6050 */

        MYSQL *connection = NULL, conn;

        mysql_init(&conn);
        connection = mysql_real_connect(&conn, DB_HOST, DB_USER, DB_PASS, DB_NAME, PORT, (char*)NULL, 0);
        if(connection == NULL){
                fprintf(stderr, "SQL connection error\n");
                return 1;
        }


        while(1)
        {

                /*Read raw value of Accelerometer and gyroscope from MPU6050*/
                Acc_x = read_raw_data(ACCEL_XOUT_H);
                Acc_y = read_raw_data(ACCEL_YOUT_H);
                Acc_z = read_raw_data(ACCEL_ZOUT_H);

                Gyro_x = read_raw_data(GYRO_XOUT_H);
                Gyro_y = read_raw_data(GYRO_YOUT_H);
                Gyro_z = read_raw_data(GYRO_ZOUT_H);

                /* Divide raw value by sensitivity scale factor */
                Ax = Acc_x/16384.0;
                Ay = Acc_y/16384.0;
                Az = Acc_z/16384.0;

                Gx = Gyro_x/131;
                Gy = Gyro_y/131;
                Gz = Gyro_z/131;

                printf("\n Gx=%.3f °/s\tGy=%.3f °/s\tGz=%.3f °/s\tAx=%.3f g\tAy=%.3f g\tAz=%.3f g\n",Gx,Gy,Gz,Ax,Ay,Az);

                changeSeatGradient(Ax, Ay, connection);


                delay(1000);

        }

	return 0;
}

void changeSeatGradient(float Ax, float Ay, MYSQL *connection){
	 MYSQL_RES *sql_result;
	 MYSQL_ROW sql_row;
	 int query_start;

         char query[1000];
	 char seatQuery[1000];
	 char gradientQuery[500];

	 if(Ax<-1){//NoSeat
                sprintf(query, "update seat set gradient = 1 where bus_id = 1");
                query_start =  mysql_query(connection, query);
                if(query_start !=0){
                	fprintf(stderr, "SQL query error\n");
                        return;
                 }

                sprintf(query, "select min(id) from timetable where bus_stop_id = 11 and time >= now()");
                query_start =  mysql_query(connection, query);
                if(query_start !=0){
                        fprintf(stderr, "SQL query error\n");
                        return;
                 }

		sql_result = mysql_store_result(connection);
		sql_row = mysql_fetch_row(sql_result);

		int id = atoi(sql_row[0]);

                sprintf(seatQuery, "update timetable set seat_status =2 where seat_status = 0 and time >= time(now()) and bus_id = 1 and id <= %d", id);
                query_start = mysql_query(connection,seatQuery);
                if(query_start !=0){
                        fprintf(stderr, "SQL query error\n");
                        return;
                 }		
         }

         if(Ax>-1 && Ax <0){//EmptySeat
                sprintf(query, "update seat set gradient = 0 where bus_id = 1");
                query_start =  mysql_query(connection, query);
                if(query_start !=0){ 
                	fprintf(stderr, "SQL query error\n");
                        return;
                }

                sprintf(seatQuery, "update timetable set seat_status =0 where seat_status = 2 and bus_id = 1");
                query_start = mysql_query(connection, seatQuery);
                if(query_start !=0){
                        fprintf(stderr, "SQL query error\n");
                        return;
		}

         }

} 
