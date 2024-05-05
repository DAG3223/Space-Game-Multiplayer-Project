#pragma once
#include "raylib.h"
#include <iostream>


float perSecond(float value) {
	return value * GetFrameTime();
}

float angleBetween(float x1, float y1, float x2, float y2) {
	return atan2f(y2 - y1, x2 - x1);
}