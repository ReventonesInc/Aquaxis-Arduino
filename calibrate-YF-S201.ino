#define PIN_MED D2
volatile int pulseConter;
 
// YF-S201
//const float factorK = 7.5;
 
void ICACHE_RAM_ATTR ISRCountPulse(){
    pulseConter++;
    Serial.print("PULSOS TEMPORAL: ");
    Serial.println(pulseConter);
}

void setup(){
   pinMode(PIN_MED,INPUT);
   Serial.begin(9600);
   attachInterrupt(PIN_MED,ISRCountPulse,HIGH);        // DEFINICION DE LA INTERRUPCION
   pulseConter = 0;
}

void loop(){
  
}
