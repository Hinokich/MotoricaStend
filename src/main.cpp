#include <mbed.h>
#include <list>
#include "config.h"
#include "tenso.h"
#include "current.h"
#include "noise.h"
#include "temperature.h"
#include "servo.h"

DigitalOut ach1(PIN_ACH_1);
DigitalOut ach2(PIN_ACH_2);

void loop();
void reset();
void open();
void close();
void start();
void pause();
void stop();
void shakeDC(); //синхронный (не блок.) код с контролем по току
void shakeACH();
void checkTemperature();
void checkShakesTarget();

int controlType = 2;
int shakesCount = 0;
int shakesTarget = -1;
int shakeTime = 600;
int coolingTime = 1000;
int testState = 3; 
int prosthesisState = 1;
int motionState = 0; //0 не двигается, 1 октрывается, 2 закрывается
int driveType = 3; //1 HDLC, 2 Servo, 3 DC, 4 ACH

char parcel[256];
char* rxBuf;
int rxBufSize;
void flushSerialBuffer(void);
void onDataRecieved();
void parse();
int combineParcel();

int temp = 0;
int nominalTemp = 0;
int stopTemp = 60;
int noise = 0;
int voltage = 0;

Serial pc(USBTX, USBRX);
Timer timerMotion;
Timer timerCooling;

int main(){
  pc.baud(115200);
  forceOffset = forceCalib();
  timerCooling.start();
  pwm.period_ms(20); //для мотора
  pwm.write(1.0f);
  pc.attach(&onDataRecieved, SerialBase::RxIrq);
  while(1) {
    loop();
  }
}

void loop(){
  temp = getTemp();
  noise = getNoise();
  current = getCurrent();
  combineParcel();
  pc.printf("%s", parcel);
  if(testState==1){ //if running
    if(driveType==3){//if DC
      shakeDC();
    }
    if(driveType==4){ //if ACH
      shakeACH();
    }
  }
  checkTemperature();
  checkShakesTarget();
}

void shakeDC(){
  static int cycleStep;
  current = getCurrent();
  cycleStep++;
  currentCycleSum+=current;
  if(force>forceMaximum){
    forceMaximum=force;
  }
  if(current>=stopCurrent){
    if(timerMotion.read_ms()>=currentDelay){ //остановка
      currentAverage = currentCycleSum / cycleStep; //вычисление среднего тока за цикл
      ach1=0;
      ach2=0;
      if(motionState==1){
        prosthesisState=1;
        motionState=0;
        shakesCount++;
      }
      if(motionState==2){
        prosthesisState=2;
        motionState=0;
      }
      timerCooling.reset();
      timerCooling.start();
      timerMotion.stop();
    }
  }
  if(motionState==0){
    if(timerCooling.read_ms()>=coolingTime){
      timerCooling.stop();
      timerMotion.reset();
      timerMotion.start();
      if(prosthesisState==1){ //начало закрытия
        currentCycleSum=0;
        cycleStep=0;
        forceMaximum=0;
        motionState=2;
        ach1=1;
        ach2=0;
      }
      if(prosthesisState==2){
        motionState=1;
        ach1=0;
        ach2=1;
      }
    }
  }
  if(timerMotion.read_ms() >= currentMinimumTriggerTime){ //если все сломалось
    ach1=0;
    ach2=0;
    if(motionState!=0){
      prosthesisState=motionState;
    }
    motionState=0;
    timerMotion.stop();
    timerMotion.reset();
    timerCooling.stop();
    timerCooling.reset();
    testState = 5; //все сломалось
    wait_ms(5);
  }
}

void shakeACH(){
  if((timerCooling.read_ms()>=coolingTime)&&(motionState==0)){
    if(prosthesisState==1){
      motionState=2;
      ach1=1;
      ach2=0;
    }
    if(prosthesisState==2){
      motionState=1;
      ach1=0;
      ach2=1;
    }
    timerCooling.stop();
    timerCooling.reset();
    timerMotion.reset();
    timerMotion.start();
  }
  if((motionState!=0)&&(timerMotion.read_ms()>=shakeTime)){
    prosthesisState=motionState;
    if(motionState==1){
      shakesCount++;
    }
    motionState=0;
    ach1=0;
    ach2=0;
    timerMotion.stop();
    timerMotion.reset();
    timerCooling.reset();
    timerCooling.start();
  }
}

void open(){
  testState = 2; //пауза
  timerCooling.stop();
  timerCooling.reset();
  timerMotion.stop();
  timerMotion.reset();
  ach1 = 0;
  ach2 = 1;
  wait_ms(shakeTime);
  ach1 = 0;
  ach2 = 0;
  motionState = 0;
  prosthesisState = 1; 
}

void close(){
  testState = 2; //пауза
  timerCooling.stop();
  timerCooling.reset();
  timerMotion.stop();
  timerMotion.reset();
  ach1 = 1;
  ach2 = 0;
  wait_ms(shakeTime);
  ach1 = 0;
  ach2 = 0;
  motionState = 0;
  prosthesisState = 2; 
}

void pause(){
  testState = 2;
  motionState = 0;
  timerCooling.stop();
  timerMotion.stop();
  ach1=0;
  ach2=0;
}

void start(){
  if((testState==3)or(testState==0)){
    reset();
  }
  testState = 1;
  timerCooling.start();
}

void stop(){
  testState = 3;
  motionState = 0;
  timerCooling.stop();
  timerMotion.stop();
  ach1=0;
  ach2=0;
}

void reset(){
  shakesCount = 0;
  ach1 = 0;
  ach2 = 1;
  wait_ms(shakeTime*2);
  ach1 = 0;
  ach2 = 0;
  testState = 2; //пауза
  prosthesisState = 1; //открыто
  motionState = 0; //не двигается
  timerCooling.stop();
  timerCooling.reset();
  timerMotion.stop();
  timerMotion.reset();
} 

void parse(){
  int a,b,c=0;
  sscanf(rxBuf,"%d,%d,%d", &a, &b, &c);
  if(a==1){
  switch (b)
    {
      case 1:{
        switch (c){
          case 1: start(); break;
          case 2: pause(); break;
          case 3: stop(); break;
          default: break;
        }
        break;
      }

      case 2:{
        shakeTime = c;
        break;
      }

      case 3:{
        coolingTime = c;
        break;
      }

      case 4:{
        stopCurrent = c;
        break;
      }

      case 5:{
        stopForce = c;
        break;
      }
      
      case 6:{
        switch (c){
          case 1: prosthesisState=1; open(); break;
          case 2: prosthesisState=2; close(); break;
          case 3: prosthesisState=3; break;
          case 4: prosthesisState=4; break;
          default: break;
        }
        break;
      }

      case 7:{
        driveType=c;
        stop();
        reset();
        break;
      }

      case 10:{
        stopTemp = c;
        break;
      }

      case 13:{
        forceTare(c);
        break;
      }

      case 14:{
        if(c==0){
          currentSmooth = 0;
        }else{
          currentSmooth = SMOOTH_RATIO;
        }
        break;
      }

      case 15:{
        if(c==0){
          forceSmooth = 0;
        }else{
          forceSmooth = SMOOTH_RATIO;
        }
        break;
      }

      case 16:{
        controlType = 0;
        break;
      }

      case 17:{
        if(c<=0){
          shakesTarget=-1;
        }else{
          shakesTarget=c;
        }
        break;
      }

      default:
        break;
    }  
  }
}

int combineParcel(){
  current = getCurrent();
  force = getForceSmoothed();
  int sz = sprintf(parcel, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
  shakesCount,
  current, 
  force,
  shakeTime,
  coolingTime,
  stopCurrent,
  stopForce,
  temp,
  nominalTemp,
  stopTemp,
  noise,
  voltage,
  testState,
  currentAverage,
  forceMaximum,
  driveType,
  motionState
  );
  return sz;
}

void flushSerialBuffer(void){ //очистка буфера порта
  char char1 = 0;
  wait(0.02); //костыль, без задержки не работает и падает
   while (pc.readable()) {
      char1 = pc.getc();
      wait(0.02);
      } 
      return;
}

void onDataRecieved(){
  pc.attach(NULL, SerialBase::RxIrq);
  free(rxBuf);
    rxBuf = (char*)malloc(256*sizeof(char));
    rxBufSize = pc.scanf("%s",rxBuf);
    char tempBuf[256];
    rxBufSize = sprintf(tempBuf, "%s",rxBuf);
    if(rxBufSize>=1){
      parse();
    }
    flushSerialBuffer();
  pc.attach(&onDataRecieved, SerialBase::RxIrq);
}

void checkTemperature(){
  if((testState!=4)&&(temp>=stopTemp)&&(controlType==2)){
    testState=4;
    ach1=0;
    ach2=0;
    timerMotion.stop();
    timerCooling.stop();
    timerMotion.reset();
    timerCooling.reset();
  }
}

void checkShakesTarget(){
  if(testState==1){ //if running
    if((shakesTarget!=-1)&&(shakesCount>=shakesTarget)){
      stop();
      testState=0; //успешное завершение
    }
  }
}