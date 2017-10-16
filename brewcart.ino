#include <SoftwareSerial.h>
#include <TimerOne.h>
#include <ClickEncoder.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin defs
int PIN_ENC1_A  = 14;
int PIN_ENC1_B  = 15;
int PIN_BUTTON1 = 16;
int PIN_ENC2_A  = 17;
int PIN_ENC2_B  = 18;
int PIN_BUTTON2 = 19;

int PIN_FLOWMETER = 13;
int PIN_THERMOMETER = 2;
int PIN_THERMOMETER_PU = 3;

int PIN_RELAY_RIMS = 9;
int PIN_RELAY_KETTLE = 10;

int PIN_LCD_TX = 8;
int PIN_LCD_RX = 7;// Not used, but need for softserial initialization

// Rotary encoder handles
ClickEncoder *encoder1;
ClickEncoder *encoder2;

// Thermometer addresses for RIMS temp sensor and mash tun temp sensor
DeviceAddress ADDR_RIMS_THERMO = { 0x28, 0xFF, 0x7D, 0x40, 0xA0, 0x16, 0x03, 0x7B };
DeviceAddress ADDR_MT_THERMO = { 0x28, 0xFF, 0x5A, 0x5E, 0xA0, 0x16, 0x03, 0xFF };
const float RIMS_THERMO_OFFSET_F = 2.0;
const float MT_THERMO_OFFSET_F = 2.0;


// Globals for measured temps
float tempF_RIMS = 0.0;
float tempF_MT = 0.0;

// OneWire/Thermometer handles
OneWire oneWire(PIN_THERMOMETER);
DallasTemperature thermometers(&oneWire);

// KETTLE WATTAGE CONTROL
const float STEP_WATTS = 50.0;
const float PWM_PERIOD_MS = 100.0;
const float KETTLE_MAX_WATTS = 3000.0;
const float KETTLE_TOTAL_WATTS = 3000.0;
float kettle_watts = 0.0;
int kettle_dutycycle_ms = 0;
void setKettleWatts ( float watts )
{
  kettle_dutycycle_ms = (int) ( kettle_watts / KETTLE_TOTAL_WATTS * PWM_PERIOD_MS );
}

// RIMS WATTAGE CONTROL
const float RIMS_MIN_DWATTS = -25.0;
const float RIMS_MAX_DWATTS = 5.0;
const float RIMS_MAX_WATTS = 2000.0;
const float RIMS_TOTAL_WATTS = 2000.0;
float rims_watts = 0.0;
int rims_dutycycle_ms = 0;
void setRIMSWatts ( float watts )
{
  rims_dutycycle_ms = (int) ( rims_watts / RIMS_TOTAL_WATTS * PWM_PERIOD_MS );
}

// Helper conversion functions
float degF2degC( float degF ){return (degF-32.0)/1.8;}
float degC2degF( float degC ){return degC*1.8+32.0;}

// RIMS TEMPERATURE SET POINT
const float RIMS_SETPOINT_MIN_F = 50.0;
const float RIMS_SETPOINT_MAX_F = 185.0;
float rims_setpoint_f = RIMS_SETPOINT_MIN_F;
void setRIMStempF( float tempF )
{
}

// PWM CONTROL
// Should be called once every ms using an ISR
int pwm_counter_ms = 0;
int pwm_top_ms = (int)PWM_PERIOD_MS;
void service_pwm()
{
  if( pwm_counter_ms >= rims_dutycycle_ms )
  {
    digitalWrite( PIN_RELAY_RIMS, LOW);
  }else
  {
    digitalWrite( PIN_RELAY_RIMS, HIGH);
  }

  if( pwm_counter_ms >= kettle_dutycycle_ms )
  {
    digitalWrite( PIN_RELAY_KETTLE, LOW);
  }else
  {
    digitalWrite( PIN_RELAY_KETTLE, HIGH);
  }
  
  pwm_counter_ms++;
  if( pwm_counter_ms >= pwm_top_ms )
    pwm_counter_ms = 0;
  
}


// FLOW METER
// For 1.5 gallon/min we'd get ~50 pulses per second,
// so calling this service routine every 1ms should be plenty
volatile uint8_t lastflowpinstate=HIGH;
unsigned long millis_lastflowrate=0;
unsigned int pulses=0;
volatile float flowrate_gpm=0.0;
const float LPS_2_GPM = 15.8503; // liters/sec to gal/min conversion
const unsigned long FLOW_UPDATE_INTERVAL_MS = 1000; // Interval to trigger flow rate update from accumlated pulses
const float MIN_RIMS_FLOWRATE_GPM = 0.75; // Minimum acceptable flow rate to keeps RIMS on
void service_flowmeter()
{
  // If it's been > 1 second, calculate and update flow rate
  unsigned long millis_now = millis();
  if( millis_now - millis_lastflowrate > FLOW_UPDATE_INTERVAL_MS )
  {
    flowrate_gpm = ((float)pulses) / ((float) (millis_now - millis_lastflowrate) ) * 1000.0 / 485.0 * LPS_2_GPM;
    pulses = 0;
    millis_lastflowrate = millis_now;
  }

  uint8_t x = digitalRead(PIN_FLOWMETER);
  if (x == lastflowpinstate) {
    return; // nothing changed!
  }
  if (x == HIGH) {
    //low to high transition!
    pulses++;
  }
  lastflowpinstate = x;
}


// TIMER 1 ISR (1000Hz)
void timerIsr() {
  // Service encoders/buttons
  encoder1->service();
  encoder2->service();

  // Service flow meter
  service_flowmeter();

  // Service PWM relay outputs
  service_pwm();
}

// Software serial for LCD display
SoftwareSerial lcdSerial(PIN_LCD_RX, PIN_LCD_TX);

void updateLCD()
{
  Serial.print("\n\n\n\n\n");
  Serial.print("MT temp F: ");Serial.println(tempF_MT);
  Serial.print("Flow rate: ");Serial.println(flowrate_gpm);
  Serial.print("\nKettle Watts: "); Serial.println( kettle_watts );
  Serial.print("\nRIMS setpoint F: "); Serial.println( rims_setpoint_f );
  Serial.print("RIMS temp F: ");Serial.println(tempF_RIMS);
  Serial.print("RIMS watts: ");Serial.println( rims_watts );
  
  /*char line1[17] = {0};
  char line2[17] = {0};
  sprintf(line1, "abc");
  sprintf(line2, "123");

  lcdSerial.write(254); // move cursor to beginning of first line
  lcdSerial.write(128);
  lcdSerial.write(line1);
  
  lcdSerial.write(254); // move cursor to beginning of first line
  lcdSerial.write(192);
  lcdSerial.write(line2);
*/
}

void setup() {

  // Debug serial setup
  Serial.begin( 115200 );
  
  // Encoder/button setup
  encoder1 = new ClickEncoder( PIN_ENC1_A, PIN_ENC1_B, PIN_BUTTON1);
  encoder2 = new ClickEncoder( PIN_ENC2_A, PIN_ENC2_B, PIN_BUTTON2);
  
  // Enable internal 20k pull-ups for encoder and buttons
  pinMode( PIN_ENC1_A, INPUT); digitalWrite( PIN_ENC1_A, HIGH );
  pinMode( PIN_ENC1_B, INPUT); digitalWrite( PIN_ENC1_B, HIGH );
  pinMode( PIN_ENC2_A, INPUT); digitalWrite( PIN_ENC2_A, HIGH );
  pinMode( PIN_ENC2_B, INPUT); digitalWrite( PIN_ENC2_B, HIGH );
  pinMode( PIN_BUTTON1, INPUT); digitalWrite( PIN_BUTTON1, HIGH );
  pinMode( PIN_BUTTON2, INPUT); digitalWrite( PIN_BUTTON2, HIGH );

  // Thermometer pull-up
  pinMode(PIN_THERMOMETER_PU, OUTPUT);
  digitalWrite(PIN_THERMOMETER_PU, HIGH);
  delay(100);

  // Initialize thermometers and set resolution
  // 10-bit resolution gets 0.25 degC resolution w/ ~200ms sample time
  thermometers.begin();
  thermometers.setResolution(ADDR_RIMS_THERMO, 10);
  thermometers.setResolution(ADDR_MT_THERMO, 10);
    

  // Flow meter setup
  pinMode(PIN_FLOWMETER, INPUT);

  // LCD setup
  /*lcdSerial.begin(9600);
  delay(500);
  lcdSerial.write(124); // 124 = brightness command
  lcdSerial.write(0x0C); // brightness level (128-157)
  delay(500);
  lcdSerial.write(124); // 124 = brightness command
  lcdSerial.write(130); // brightness level (128-157)
  delay(500);
  lcdSerial.write(254);
  lcdSerial.write(1);*/

  // Relay output setup
  pinMode(PIN_RELAY_RIMS, OUTPUT);
  digitalWrite( PIN_RELAY_RIMS, LOW);
  pinMode(PIN_RELAY_KETTLE, OUTPUT);
  digitalWrite( PIN_RELAY_KETTLE, LOW);

  // Service timer setup
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); // timerIsr will be called every 1000us = 1ms

}

// This loop runs the controller as fast as the temp sensor reads will allow
// For the 10-bit temp sensor setting, this will be about every 200ms or 5Hz
int lcdCount = 0;
void loop() {
  // Send command to begin temperature conversion
  thermometers.requestTemperatures();

  // After conversion complete, read temperatures
  tempF_RIMS = degC2degF(thermometers.getTempC(ADDR_RIMS_THERMO)) + RIMS_THERMO_OFFSET_F;
  tempF_MT = degC2degF(thermometers.getTempC(ADDR_MT_THERMO)) + MT_THERMO_OFFSET_F;
  
  // Set Kettle wattage from encoder
  setKettleWatts ( kettle_watts + ((float)encoder2->getValue())*STEP_WATTS);

  // Set RIMS temp from encoder
  setRIMStempF( rims_setpoint_f + ((float)encoder1->getValue())*0.5 );

  // Simple bang-bang controller for RIMS outlet temp control
  if( tempF_RIMS < rims_setpoint_f && flowrate_gpm > MIN_RIMS_FLOWRATE_GPM )
  {
    setRIMSWatts( RIMS_MAX_WATTS );
  }else
  {
    setRIMSWatts( 0 );
  }

<<<<<<< HEAD
=======
  // Calculate delta in wattage necessary to get RIMS temp to set point
  // Calculated based on flow rate of water and temperature delta
  float dwatts = (degF2degC(rims_setpoint_f) - degF2degC(tempF_RIMS))*flowrate_gpm / LPS_2_GPM * 4184.0;

  // Rate limit the delta wattage to prevent chatter
  dwatts = constrain(dwatts, RIMS_MIN_DWATTS, RIMS_MAX_DWATTS);

  // Set RIMS wattage (set function applies limits)
  setRIMSWatts( rims_watts + dwatts );

>>>>>>> fb0833d61384c16c24748783edcafbd8f74333ac
  // Update the LCD display
  lcdCount++;
  if (lcdCount % 4 == 0){updateLCD();}
  
}
