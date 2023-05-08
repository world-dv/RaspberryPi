#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>

#define Bus100 21
#define Bus200 22
#define Bus300 23
#define Bus400 24
#define Bus500 25
#define Bus600 12
#define Bus700 13
#define Bus800 14
#define Bus900 26
#define Bus1000 27
#define Bus1100 28
#define Bus1200 29

#define times 5

pthread_t pthread[12];
int busStop[12] = { 21, 22, 23, 24, 25, 12, 13, 14, 26, 27, 28, 29}; 

int glitter(int pinNumber);
int turnOn(int pinNumber);
int turnOff(int pinNumber);

void *busThread(void *arg) {
	int *pinNumber = (int*)arg;
	int pin = *pinNumber;
	printf("%d\n", pin);
	glitter(pin);
}

int main(void) {
	if(wiringPiSetup() == -1) return -1;
	pinMode(Bus100, OUTPUT);
	pinMode(Bus200, OUTPUT);
	pinMode(Bus300, OUTPUT);
	pinMode(Bus400, OUTPUT);
	pinMode(Bus500, OUTPUT);
	pinMode(Bus600, OUTPUT);
	pinMode(Bus700, OUTPUT);
	pinMode(Bus800, OUTPUT);
	pinMode(Bus900, OUTPUT);
	pinMode(Bus1000, OUTPUT);
	pinMode(Bus1100, OUTPUT);
	pinMode(Bus1200, OUTPUT);

	int status;
	for(int i = 0; i < 12; i++) {
		pthread_create(&pthread[i], NULL, busThread, (void *)&busStop[i]);
		pthread_join(pthread[i], (void**)&status);
	}

	return 0;
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
