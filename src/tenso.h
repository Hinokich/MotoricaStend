int force;
int forceMaximum;
int getForce();
int forceCalib();
int forceTare(int mass); //more accurate calibration required before each test
int getForceSmoothed();
int forceOffset = 0;
int forceVirtualOffset = 0;
int stopForce = 5000;
int stopForceShakes = 5;
float forceVirtualGain = 1.0f;
float forceGain = 1.0f;
float forceSmooth = SMOOTH_RATIO;

int transform(int value);

DigitalOut fSck(PIN_SCK);
DigitalIn fDat(PIN_MISO);

int getForce(){
  #ifdef TENSO
  static int lastforce = 0;
  int buf = 0;
  fSck = 0;
  int timeout = 5000;
  while((fDat==1)&&(timeout>0)){
    timeout--;
  }
  if(timeout<=0){
    return lastforce;
  }
  for(int i=0; i<24; i++){
    fSck=1;
    buf = buf << 1;
    fSck=0;
    wait_us(25);
    if(fDat==1){
      buf++;
    }
  }

  fSck = 1;
  wait_us(25);
  fSck = 0;
  wait_us(25);


  buf = transform(buf);
  buf = buf - forceOffset;
  lastforce = buf;
  return lastforce;
  #else
  int a = 0;
  return 42;
  #endif
}

int forceCalib(){
  int sum = 0;
  int ave = 0;
  int qtt = 50;
  for(int i=0; i<qtt; i++){
    sum += getForce();
  }
  ave = sum / qtt;
  return ave;
}

int forceTare(int mass){
  if(mass==0){
    forceVirtualOffset = force;
  }else{
    float m = mass;
    float f = force;
    forceVirtualGain = m/f;
  }
  return 0;
}

int getForceSmoothed(){
  static int fRaw;
  static int fOld;
  static int fSm;
  fRaw = getForce();
  fRaw = fRaw - forceVirtualOffset;
  fRaw = fRaw * forceVirtualGain;
  fRaw = abs(fRaw);
  fSm = fOld * forceSmooth + fRaw * (1-forceSmooth);
  fOld = fSm;
  return fSm;
}

int transform(int value){
  uint8_t bit = value >> 23;
	value &= ~(1 << 23);
	value |= bit << 31;
  return value;
}