#ifdef TMP35
int tempOffset=0;
int tempGain=10;
#endif

#ifdef TMP36
int tempOffset=500;
int tempGain=10;
#endif

#ifdef TMP37
int tempOffset;
int tempGain=20;
#endif

AnalogIn tempSensor(TEMP_PIN);

int tempSmooth = SMOOTH_RATIO;

int getTemp(){
  static int tRaw;
  static int tOld;
  static int tSm;
  tRaw = tempSensor * 3300;
  tSm = tOld * tempSmooth + tRaw * (1-tempSmooth);
  tOld = tSm;
  int t = (tSm - tempOffset)/tempGain;
  return t;
}