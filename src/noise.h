int noiseGain = 60;

AnalogIn noiseSensor(NOISE_PIN);

int getNoise(){
    int a = noiseSensor * noiseGain;
    return a;
}