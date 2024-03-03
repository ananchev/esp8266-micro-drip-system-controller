#include <Arduino.h>
#include <helperfunctions.h>

WateringDuration resolveParameters(String input) {
    if( input == "1hr" ) return OneHour;
    if( input == "2hrs" ) return TwoHours;  
    if( input == "3hrs" ) return ThreeHours;
    assert(false); //tell the compiler that the else case deliberately omitted
};