#include <stdio.h>

#define POINTS_COUNT 22

typedef struct {
    int temp;
    int lsb;
} temp_point;

static temp_point thermistorPoints[] = {
    { 850 , 129 },
    { 800 , 146 },
    { 750 , 165 },  
    { 700 , 186 },  
    { 650 , 210 },  
    { 600 , 236 },  
    { 550 , 267 },  
    { 500 , 300 },  
    { 450 , 337 },
    { 400 , 376 },  
    { 350 , 419 },  
    { 300 , 464 },  
    { 200 , 559 },  
    { 250 , 511 },   
    { 150 , 608 },  
    { 100 , 656 },
    { 50  , 703 },  
    { 0    , 748 },  
    { -50 , 789 },  
    { -100, 828 },  
    { -150, 862 },  
    { -200, 892 }
};

static inline int interpolate(int val, int rangeStart, int rangeEnd, int valStart, int valEnd) {
    return (rangeEnd - rangeStart) * (val - valStart) / (valEnd - valStart) + rangeStart;
}

static inline int interpolateVoltage(int temp, unsigned char i){
    return interpolate(temp, thermistorPoints[i-1].lsb, thermistorPoints[i].lsb, thermistorPoints[i-1].temp, thermistorPoints[i].temp);
}

static inline int interpolateTemperature(int lsb, unsigned char i){
//    printf("%d [%d -- %d] // [%d -- %d]\n", lsb, thermistorPoints[i-1].temp, thermistorPoints[i].temp, thermistorPoints[i-1].lsb, thermistorPoints[i].lsb);
    return interpolate(lsb, thermistorPoints[i-1].temp, thermistorPoints[i].temp, thermistorPoints[i-1].lsb, thermistorPoints[i].lsb);
}

/**
 * Returns the index of the first point whose temperature value is greater than argument
 **/
static inline unsigned char searchTemp(int temp) {
    unsigned char i;
    for(i = 0; i < POINTS_COUNT; i++) {
        if(thermistorPoints[i].temp > temp) {
            return i;
        }
    }
    return POINTS_COUNT-1;
}

/**
 * Returns the index of the first point whose lsb value is greater than argument
 **/
static inline unsigned char searchLsb(int lsb) {
    unsigned char i;
    for(i = 0; i < POINTS_COUNT; i++) {
        if(thermistorPoints[i].lsb > lsb) {
            return i;
        }
    }
    return POINTS_COUNT-1;
}

long thermistorLsbToTemperature(unsigned int lsb) {
	if(lsb >= thermistorPoints[POINTS_COUNT-1].lsb) {
		return thermistorPoints[POINTS_COUNT-1].temp;
	}
	if(lsb <= thermistorPoints[0].lsb) {
		return thermistorPoints[0].temp;
	}
    return interpolateTemperature(lsb, searchLsb(lsb));
}

// void main() {
// 	int i;
// 	for(i = 129; i < 1024; i++) {
// 		printf("%d - %d, \n", i, thermistorLsbToTemperature(i));
// 	}
// }
