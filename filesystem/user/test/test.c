#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#define __NR_set_sensor_information 244
struct sensor_information {
    int microlatitude; /* GPS Latitude in millionths */
    int microlongitude; /* GPS Longitude in millionths */
    int centilux; /* Luminosity in hundredths */
    int centiproximity; /* Proximity in hundredths */
    int centilinearaccelx;
    /* Linear acceleration in the x dimension in hundredths */
    int centilinearaccely;
    /* Linear acceleration in the y dimension in hundredths */
    int centilinearaccelz;
    /* Linear acceleration in the z dimension in hundredths */
};


int main (void)
{
	/* Write your test program here */
	int ret;
	struct sensor_information si;
	
	ret = syscall(__NR_set_sensor_information, &si);
	printf("syscall %d returns %d\n",
	       __NR_set_sensor_information, ret);

	return 0;
}
