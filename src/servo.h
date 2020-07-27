#ifndef MBED_H
#include <mbed.h>
#endif
PwmOut pwm(PIN_PWM);

float servoPulseWidth = 0.001f; //Sec
float servoPulsePeriod = 0.02f; //Sec
float servoFullAngleRange = 180; //deg
int servoLimitLow = 0;
int servoLimitHigh = 180;

int servoWrite(int angle){
    if(angle<servoLimitLow){
        angle = servoLimitLow;
    }
    if(angle>servoLimitHigh){
        angle = servoLimitHigh;
    }
    static float add; 
    static float servoWidth;
    add = (angle / servoFullAngleRange)*0.001f;
    servoWidth = servoPulseWidth + add;
    pwm.period(servoPulsePeriod);
    pwm.pulsewidth(servoWidth);
    return angle;
}

