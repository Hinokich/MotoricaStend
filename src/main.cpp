#include <mbed.h>
#include <list>

#define PIN_SCK PB_3
#define PIN_MISO PB_4
#define PIN_ACH_1 PA_8
#define PIN_ACH_2 PF_1
#define PIN_PWM PF_0
#define PIN_CURRENT PA_4

const char updateFlag = 0x5e;

DigitalOut fSck(PIN_SCK);
DigitalIn fDat(PIN_MISO);
DigitalOut ach1(PIN_ACH_1);
DigitalOut ach2(PIN_ACH_2);
PwmOut pwm(PIN_PWM);
AnalogIn currentSensor(PIN_CURRENT);

void loop();
void reset();
void open();
void close();
void start();
void pause();
void stop();
void shakeSync(); //синхронный (не блок.) код
void shakeCurrentControl(); //синхронный (не блок.) код с контролем по току


int force;
int getForce();
int forceCalib();
int getForceSmoothed();
int forceOffset = 0;
float forceGain = 150;
float forceSmooth = 0.97f;

int current = 0;
int getCurrent();
int stopCurrent = 2500; //мА
int currentGain = 185;
int currentOffset = 2525;
int currentDelay = 300;
int currentMinimumTriggerTime = 3000; //если за это время (мс) ток не достигнет упора, испытания прекратятся 
float currentSmooth = 0.98f;

int shakesCount = 0;
int shakeTime = 600;
int coolingTime = 700;
int testState = 3;
int prosthesisState = 1;
int motionState = 0; //0 не двигается, 1 октрывается, 2 закрывается

char parcel[256];
char* rxBuf;
int rxBufSize;
void flushSerialBuffer(void);
void onDataRecieved();
void parse();
int combineParcel();

/* Unused variables */
int stopStrength = 0;
int temp = 0;
int nominalTemp = 0;
int stopTemp = 0;
int noise = 0;
int voltage = 0;
/* Unused variables */

Serial pc(USBTX, USBRX);
Timer timerMotion;
Timer timerCooling;

int main(){
  pc.baud(115200);
  forceOffset = forceCalib();
  pc.printf("Force offset: %d\n", forceOffset);
  timerCooling.start();
  pwm.period_ms(20); //для мотора
  pwm.write(1.0f);
  pc.attach(&onDataRecieved, SerialBase::RxIrq);
  while(1) {
    loop();
  }
}

void loop(){
  combineParcel();
  pc.printf("%s", parcel);
  //force = getForceSmoothed();
  //pc.printf("%d\n",force);
  if(testState==1){ //if running
   shakeCurrentControl();
  }
}

int getForce(){
  int buf = 0;
  fSck = 0;
  while(fDat==1){}
  for(int i=0; i<24; i++){
    fSck=1;
    buf = buf << 1;
    fSck=0;
    if(fDat==1){
      buf++;
    }
  }
  for(int i=0; i<2; i++){
    fSck = 1;
    fSck = 0;
  }
  return abs(buf-forceOffset);
}

int forceCalib(){
  int sum = 0;
  int ave = 0;
  for(int i=0; i<40; i++){
    sum += getForce();
  }
  ave = sum / 40;
  return ave;
}

int getForceSmoothed(){
  static int fRaw;
  static int fOld;
  static int fSm;
  fRaw = (getForce())/forceGain;
  fSm = fOld * forceSmooth + fRaw * (1-forceSmooth);
  fOld = fSm;
  return fSm;
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

int combineParcel(){
  current = getCurrent();
  force = getForceSmoothed();
  int sz = sprintf(parcel, "FCD> %d %d %d %d %d %d %d %d %d %d %d %d\r",
  shakesCount,
  current, 
  force,
  shakeTime,
  coolingTime,
  stopCurrent,
  stopStrength,
  temp,
  nominalTemp,
  stopTemp,
  noise,
  voltage
  );
  return sz;
}

void shakeSync(){
  if(motionState == 0){
    if(timerCooling.read_ms()>=coolingTime){
      if(prosthesisState==1){ //если открыт, закрыть
        timerCooling.stop();
        timerMotion.reset();
        timerMotion.start();
        ach1 = 1;
        ach2 = 0;
        motionState = 2; //состояние движения = закрывается
      }
      if(prosthesisState==2){ //если закрыт, открыть
        timerCooling.stop();
        timerMotion.reset();
        timerMotion.start();
        ach1 = 0;
        ach2 = 1;
        motionState = 1; //состояние движения = открывается
      }
    }
  }else{
    if(timerMotion.read_ms()>=shakeTime){
      timerMotion.stop();
      timerCooling.reset();
      timerCooling.start();
      ach1=0;
      ach2=0;
      if(motionState==1){ //если открывалась, сменить на открыто (и добавить 1 полный жамк)
        prosthesisState = 1;
        shakesCount++;
      }
      if(motionState==2){
        prosthesisState = 2; //если закрывалась, сменить на закрыто
      }
      motionState = 0; //прохлаждается
    }
  }
} 

void shakeCurrentControl(){ //Attention! Здесь timerMotion используется для других целей!
  current = getCurrent();
  if(current>=stopCurrent){
    if(timerMotion.read_ms()>=currentDelay){
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
    }
  }
  if(timerMotion.read_ms() >= currentMinimumTriggerTime){
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
    testState=3;
  }
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

void parse(){
 if(rxBuf[0]==0x01){
    if(rxBufSize>=3){
      char f = rxBuf[1];
      switch (f)
      {
      case 0x01:{
        switch (rxBuf[2]){
          case 0x01: start(); break;
          case 0x02: pause(); break;
          case 0x03: stop(); break;
          default: break;
        }
        break;
      }

      case 0x02:{
        uint8_t hi = rxBuf[2];
        uint8_t lo = rxBuf[3];
        uint16_t val = (hi << 8) + lo;
        shakeTime = val;
        break;
      }

      case 0x03:{
        uint8_t hi = rxBuf[2];
        uint8_t lo = rxBuf[3];
        uint16_t val = (hi << 8) + lo;
        coolingTime = val;
        break;
      }

      case 0x04:{
        uint8_t hi = rxBuf[2];
        uint8_t lo = rxBuf[3];
        uint16_t val = (hi << 8) + lo;
        stopCurrent = val;
        break;
      }
      
      case 0x06:{
        switch (rxBuf[2]){
          case 0x01: prosthesisState=1; open(); break;
          case 0x02: prosthesisState=2; close(); break;
          case 0x03: prosthesisState=3; break;
          case 0x04: prosthesisState=4; break;
          default: break;
        }
        break;
      }

      default:
        break;
      }
    }
  }
}

void onDataRecieved(){
  pc.attach(NULL, SerialBase::RxIrq);
  free(rxBuf);
    rxBuf = (char*)malloc(256*sizeof(char));
    rxBufSize = pc.scanf("%s",rxBuf);
    char tempBuf[256];
    rxBufSize = sprintf(tempBuf, "%s",rxBuf);
    if(rxBufSize>=1){
      if(rxBuf[0]==updateFlag){
        combineParcel();
        pc.printf("%s", parcel);
      }else{
      parse();
      }
    }
    flushSerialBuffer();
  pc.attach(&onDataRecieved, SerialBase::RxIrq);
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
  if(testState==3){
    reset();
  }
  testState=1;
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

int getCurrent(){
  static int c;
  static int old;
  static int raw;
  //int a = ((abs((currentSensor*3300)-currentOffset))/currentGain)*1000;
  raw = ((abs((currentSensor*3300)-currentOffset))/currentGain)*1000;
  c = old * currentSmooth + raw * (1-currentSmooth);
  old = c;
  return c;
}