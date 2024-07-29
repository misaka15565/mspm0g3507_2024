#include "control.hpp"

volatile bool front_sensors[8];
volatile bool back_sensors[8];
//假设前后2*8个传感器的数据存在上面两个数组里