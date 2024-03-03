#include <Arduino.h>
#pragma once

enum WateringDuration {
    Option_Invalid,
    OneHour,
    TwoHours,
    ThreeHours
};

WateringDuration resolveParameters(String input);