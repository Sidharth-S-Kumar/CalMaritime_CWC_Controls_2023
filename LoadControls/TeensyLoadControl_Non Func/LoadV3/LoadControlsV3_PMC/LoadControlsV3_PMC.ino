#include <Arduino_MachineControl.h>
using namespace machinecontrol;
#include <Wire.h>
#include <ArduinoRS485.h>
#include <ArduinoModbus.h>





// Pin Definition
#define vBus_Pin 0
#define iBus_Pin 1
#define vGate_Pin 0
#define relay_Pin 0

//Global variables 

//Timing Vars 
unsigned long tikTok = millis() ; //Clock Reference 
unsigned long previousTime = 0; //Control Law Clock
unsigned long messageClock = 0; //Message Trigger Clock

//Analog I/O Globals
float res_divider = 0.28057; //Internal Voltage Divider
float reference = 3.3; //3.3 Volt Reference System
float vBus,iBus,pBus,rBus; //Global Variable for Voltage and Current
float busRes_divider = .2029; //External Voltage Divider
float iConversion = 0.9363; //Converting from Volts to Amps
float n = 200; //Sampling for Averaging function

//Control Law Globals 
float setPoint = 0;
float err;
float errPrev = 0;
float errSum = 0;
float errMin = -10000; 
float errMax = +10000; 
float outPut =0.25; 
float outMax = 10.4; 
float outMin = 2.25; 


// PID Gains
float Kp =0.1;
float Kd =0.1;
float Ki =0.1;

//MPPT Globals
float vPrev = 0; 
float iPrev = 0; 
float delta = .05; 
float dPwr_band = .06; 

///=========================================================
// Function Group

void readParams(){ // Read Analog Control Parameters
    float vSum, iSum = 0; 
    float raw_voltage_ch0,raw_voltage_ch1; 
    float voltage_ch0,voltage_ch1; 

    for (int i = 0; i<n;i++){
      raw_voltage_ch0 = analog_in.read(0);
      float voltage_ch0 = (raw_voltage_ch0 * reference) / 65535 / res_divider / busRes_divider;
      vSum = voltage_ch0 + vSum;

      float raw_voltage_ch1 = analog_in.read(1);
      float voltage_ch1 = (raw_voltage_ch1 * reference) / 65535 / res_divider;
      iSum = voltage_ch1/0.9363 + iSum; 
    }
      vBus = vSum/(float)n; 
      iBus = iSum/(float)n; 
      pBus = vBus*iBus;
      rBus = vBus/iBus; 
    if((tikTok-messageClock)>1000){
        Serial.print("Bus V: ");
        Serial.println(vBus,4);
        Serial.print("Bus I: ");
        Serial.println(iBus,4);
        Serial.print("pwr: ");
        Serial.println(pBus,4); 
        messageClock = tikTok; 
    }
}

void ctrlLaw(float sP, float mV){

  float dt = (float)(tikTok-previousTime)/1000.00; //time increment dt
  err = sP - mV;
  err = constrain(err,errMax,errMin); 

  //Porportional Control
  float P = Kp*err; 

  //Integral Control
  errSum += err*dt; 
  float I = Ki*errSum; 

  //Derivative Control 
  float errDer = (err-errPrev)/dt; 
  float D = Kd*errDer; 
  errPrev = err;

  previousTime =tikTok; 
  
  outPut =  (10.4-(P+I+D))+outPut;
  outPut = constrain(outPut,outMax,outMin);

}

void gateVoltage(float comm){
  analog_out.write(0, comm);



}

/*void mppt(float v, float i){
  float dV = v-vPrev; 
  float dI = i-iPrev; 
  if (dV==0){
    if(dI==0){
    }
    else if (dI>0){
            setPoint -= delta; 
    }
    else{
      setPoint += delta; 
    }
  }
  else if((abs((v*dI)+(i*dV)))<dPwr_band){
  }
  else if(((v*dI)+(i*dV))>0){
    if(dV>0){
      setPoint-=delta; 
    }
    else
    {
      setPoint+=delta; 
    }
  }
  else if(dV>0){
      setPoint+=delta; 
  }
  else
  {
    setPoint-=delta; 
  } 
  
  if (setPoint<0){
    setPoint=0;
  }


}
*/
void stateMachine(){

}



void setup() {
  // put your setup code here, to run once:
  analog_out.period_ms(0, 4);
  analog_out.period_ms(1, 4);
  analog_out.period_ms(2, 4);
  analog_out.period_ms(3, 4);


  analogReadResolution(16);
  analog_in.set0_10V();


  
  Serial.begin(9600);
  Serial.println("Initialized");

}

void loop() {
  //mppt(vBus,iBus);
  readParams();
  if (Serial.available() > 0) {
    setPoint =  Serial.parseFloat();
    Serial.print("Setpoint:");
    Serial.println(setPoint); //write setpoint to serial monitor
  }
  ctrlLaw(setPoint, rBus);
  float command = outPut;
  gateVoltage(command);




}
