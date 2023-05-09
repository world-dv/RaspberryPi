#include <stdio.h>
#include <unistd.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>

#define times 3

pthread_t pthread[12];
int busStop[12] = { 21, 22, 23, 24, 25, 14, 26, 27, 28, 29, 11, 10};
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

	for(int i = 0; i < 12; i++) {
		pinMode(busStop[i], OUTPUT);
	}

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
