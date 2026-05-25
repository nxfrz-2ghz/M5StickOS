#pragma once

// Battery display configuration
const int BAT_MIN_MV = 3000; // нижняя граница (3.00V)
const int BAT_MAX_MV = 4180; // верхняя граница (4.18V)
const int BAT_TICKS = 5;     // количество делений внутри индикатора

void print_battery();
