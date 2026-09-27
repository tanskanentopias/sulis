#include <stdlib.h>
#include <string.h>
#include "TimeParser.h"

// time format: HHMMSS (6 characters)
int time_parse(char *time) {


	// TODO: Check that string is not null

	// Parse values from time string
	if (time == NULL) {
		return TIME_VALUE_ERROR;
	}
			// For example: 124033 -> 12hour 40min 33sec
			int values[3];
			values[2] = atoi(time+4); // seconds
			time[4] = 0;
			values[1] = atoi(time+2); // minutes
			time[2] = 0;
			values[0] = atoi(time); // hours
			// Now you have:
			// values[0] hour
			// values[1] minute
			// values[2] second
			if (values[2] > 59 || values[2] < 0) {
				
				return TIME_VALUE_ERROR;
			}
			if (values[1] > 59 || values[1] < 0) {
				
				return TIME_VALUE_ERROR;
			}
			if (values[0] > 23 || values[0] < 0) {

				
				return TIME_VALUE_ERROR;

			}
			
		
		return ((values[2])+(values[1]*60)+(values[0]*3600));

		
			// TODO: Add boundary check
			// time values: below zero or above limit not allowed
			// limits are 59 for minutes, 23 for hours, etc

			// TODO: Calculate return value from the parsed minutes and seconds
			// Otherwise error will be returned!
			// seconds = ...

	
}

