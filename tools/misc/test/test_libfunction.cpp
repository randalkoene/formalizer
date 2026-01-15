/**
 * Modify this to test the output of any core library function.
 * 
 * Randal A. Koene, 20241009
 */

#include <iostream>
#include <string>

#include "TimeStamp.hpp"

using namespace fz;

int main(int argc, char **argv) {

	std::string time_in_day = "202410091510";
	time_t t = ymd_stamp_time(time_in_day);

	std::cout << "Time in day: " << time_in_day << '\n';
	std::cout << "Start of day: " << TimeStampYmdHM(day_start_time(t)) << '\n';
	std::cout << "End of day: " << TimeStampYmdHM(day_end_time(t)) << '\n';

}
