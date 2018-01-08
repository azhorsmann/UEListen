#include <wiringPi.h>
#include <iostream>
#include <vector>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <cstring>
#include <cstdlib>
#include <string>

using namespace boost::interprocess;

#define	trigLeft 16
#define	echoLeft 26

#define	trigRight 12
#define	echoRight 13

struct timespec startLeft, startRight, end;

double soundSpeed = 17150;

int *i1;
int *i2;

bool measuringLeft = false;
bool measuringRight = false;

void TimingLeftEnd() 
{
	clock_gettime(CLOCK_REALTIME, &end);
	double timespent = (end.tv_nsec - startLeft.tv_nsec) / 1.0e9; // Convert nanoseconds to milliseconds

	int duration = timespent * soundSpeed;

	if (duration > 0)
		*i1 = duration;
	
	measuringLeft = false;
}

void TimingRightEnd()
{
	clock_gettime(CLOCK_REALTIME, &end);
	double timespent = (end.tv_nsec - startRight.tv_nsec) / 1.0e9; // Convert nanoseconds to milliseconds

	int duration = timespent * soundSpeed;

	if (duration > 0)
		*i2 = duration;
	
	measuringRight = false;
}

void TimingLeft()
{
	if (measuringLeft)
		return;

	digitalWrite(trigLeft, LOW);
	delayMicroseconds(2);
	digitalWrite(trigLeft, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigLeft, LOW);

	while (digitalRead(echoLeft) == 0)
	{
		clock_gettime(CLOCK_REALTIME, &startLeft);
	}
}


void TimingRight()
{
	if (measuringRight)
		return;

	digitalWrite(trigRight, LOW);
	delayMicroseconds(2);
	digitalWrite(trigRight, HIGH);
	delayMicroseconds(10);
	digitalWrite(trigRight, LOW);

	while (digitalRead(echoRight) == 0)
	{
		clock_gettime(CLOCK_REALTIME, &startRight);
	}
}

void senseInitialize()
{
	digitalWrite(trigLeft, HIGH);
	delay(1);
	digitalWrite(trigLeft, LOW);

	digitalWrite(trigRight, HIGH);
	delay(1);
	digitalWrite(trigRight, LOW);

	delay(2000);
}


int main(void)
{

	wiringPiSetupGpio();

	pinMode(trigLeft, OUTPUT);
	pinMode(echoLeft, INPUT);

	pinMode(trigRight, OUTPUT);
	pinMode(echoRight, INPUT);

	senseInitialize();

	std::vector<int> myVector;

	int measures[3] = { 0, 9999, 0 };

	struct shm_removeLeft
	{
		shm_removeLeft() { shared_memory_object::remove("UEListenMemoryLeft"); }
		~shm_removeLeft() { shared_memory_object::remove("UEListenMemoryLeft"); }
	} removerLeft;


	struct shm_removeRight
	{
		shm_removeRight() { shared_memory_object::remove("UEListenMemoryRight"); }
		~shm_removeRight() { shared_memory_object::remove("UEListenMemoryRight"); }
	} removerRight;

	//Create a shared memory object.
	shared_memory_object shmLeft(open_or_create, "UEListenMemoryLeft", read_write);
	shared_memory_object shmRight(open_or_create, "UEListenMemoryRight", read_write);

	//Set size
	shmLeft.truncate(sizeof(int));
	shmRight.truncate(sizeof(int));

	//Map the whole shared memory in this process
	mapped_region regionLeft(shmLeft, read_write);
	mapped_region regionRight(shmRight, read_write);

	i1 = static_cast<int*>(regionLeft.get_address());
	i2 = static_cast<int*>(regionRight.get_address());

	*i1 = -1;
	*i2 = -1;

	if (wiringPiISR(echoLeft, INT_EDGE_FALLING, &TimingLeftEnd) < 0)
	{
		measuringLeft = false;
		std::cout << "Timing error setting up callback failed...\n";
		return -1;
	}

	if (wiringPiISR(echoRight, INT_EDGE_FALLING, &TimingRightEnd) < 0)
	{
		measuringRight = false;
		std::cout << "Timing error setting up callback failed...\n";
		return -1;
	}

	while (true)
	{
		TimingLeft();
		delay(0);

		delay(100);

		TimingRight();
		delay(0);
		
		std::cout << "Distance: " << *i1 << "\n";
		std::cout << "Distance: " << *i2 << "\n";

		delay(100);
	}

	return 0;
}