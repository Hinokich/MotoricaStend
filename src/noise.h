int noiseGain = 10;

AnalogIn noiseSensor(NOISE_PIN);

int getNoise(){
    int a = (noiseSensor*3300) / noiseGain;
    return a;
}