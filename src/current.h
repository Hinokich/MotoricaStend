#ifndef MBED_H
#include <mbed.h>
#endif

int current = 0;
int getCurrent();
int stopCurrent = 1200; //мА
int currentGain = 185;
int currentOffset = CURRENT_OFFSET;
int currentDelay = 300;
int currentAverage = 0;
int currentCycleSum = 0;
int currentMinimumTriggerTime = 3000; //если за это время (мс) ток не достигнет упора, испытания прекратятся 
float currentSmooth = SMOOTH_RATIO;

AnalogIn currentSensor(PIN_CURRENT);

int getCurrent(){
  static int c;
  static int old;
  static int raw;
  raw = ((abs((currentSensor*3300)-currentOffset))/currentGain)*1000;
  c = old * currentSmooth + raw * (1-currentSmooth);
  old = c;
  return c;
}