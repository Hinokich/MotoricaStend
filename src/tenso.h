
int force;
int forceMaximum;
int getForce();
int forceCalib();
int forceTare(int mass); //more accurate calibration required before each test
int getForceSmoothed();
int forceOffset = 0;
int forceVirtualOffset = 0;
int stopForce = 5000;
float forceVirtualGain = 1.0f;
float forceGain = 150;
float forceSmooth = SMOOTH_RATIO;

DigitalOut fSck(PIN_SCK);
DigitalIn fDat(PIN_MISO);

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

int forceTare(int mass){
  if(mass==0){
    forceVirtualOffset = getForceSmoothed();
  }else{
    int v = getForceSmoothed();
    forceVirtualGain = v / mass;
  }
  return 0;
}

int getForceSmoothed(){
  static int fRaw;
  static int fOld;
  static int fSm;
  fRaw = (getForce())/forceGain;
  fRaw = fRaw - forceVirtualOffset;
  fRaw = fRaw / forceVirtualGain;
  fRaw = abs(fRaw);
  fSm = fOld * forceSmooth + fRaw * (1-forceSmooth);
  fOld = fSm;
  return fSm;
}