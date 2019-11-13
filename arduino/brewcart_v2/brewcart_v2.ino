#include <Elegoo_GFX.h>    // Core graphics library
#include <Elegoo_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
#include <TimerOne.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// The control pins for the LCD can be assigned to any digital or
// analog pins...but we'll use the analog pins as this allows us to
// double up the pins with the touch screen (see the TFT paint example).
#define LCD_CS A3 // Chip Select goes to Analog 3
#define LCD_CD A2 // Command/Data goes to Analog 2
#define LCD_WR A1 // LCD Write goes to Analog 1
#define LCD_RD A0 // LCD Read goes to Analog 0
#define LCD_RESET A4 // Can alternately just connect to Arduino's reset pin
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin
//Touch For New ILI9341 TP
#define TS_MINX 120
#define TS_MAXX 900
#define TS_MINY 70
#define TS_MAXY 920
Elegoo_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define PIN_FLOWMETER 53
#define PIN_RELAY 51
#define PIN_LED 13
#define PIN_THERMOMETER 49

// Thermometer addresses for RIMS temp sensor and mash tun temp sensor
DeviceAddress ADDR_RIMS_THERMO = { 0x28, 0xFF, 0x7D, 0x40, 0xA0, 0x16, 0x03, 0x7B };
DeviceAddress ADDR_MT_THERMO = { 0x28, 0xFF, 0x5A, 0x5E, 0xA0, 0x16, 0x03, 0xFF };
const float RIMS_THERMO_OFFSET_F = 2.0;
const float MT_THERMO_OFFSET_F = 2.0;

// OneWire/Thermometer handles
OneWire oneWire(PIN_THERMOMETER);
DallasTemperature thermometers(&oneWire);
float degC2degF( float degC ){return degC*1.8+32.0;}

#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define TOP_LABEL_X 10
#define TOP_LABEL_Y 10

#define BAR_Y 80
#define BAR_HEIGHT 130

#define TEMPSENSOR_MIN_F 32.0
#define TEMPSENSOR_MAX_F 212.0

// Stored variables
float var_grainweight_lbs = 0.0;
float var_mashtemp_F = 0.0;
float var_watervol_gal = 0.0;
float var_mashtime_min = 0.0;
float var_boiltime_min = 0.0;
float var_tempsetpoint_F = 150.0;
float var_temp = TEMPSENSOR_MIN_F-1.0;

unsigned long var_starttime_ms = 0;

int var_displaytime_sec = 0;
char t_displaytime[] = "   :  ";

float var_kw = 0.0;
char t_kw_num[] = " . ";
char t_kw[] = "kw";
float var_gpm = 0.0;
char t_gpm_num[] = " . ";
char t_gpm[] = "gpm";
#define MAX_GPM 4.0
#define GPM_MASH_MIN 0.5
#define MAX_KW 5.5
char t_mintemp[] ="   ";
char t_maxtemp[] ="   ";
char t_settemp[] ="   ";
char t_currtemp[] ="   ";
char t_set[] ="SET";
#define MAX_TEMP 170.0
#define MIN_TEMP 130.0

// Limits
#define LL_GRAINWEIGHT 0.1
#define UL_GRAINWEIGHT 100.0
#define LL_MASHTEMP 75.0
#define UL_MASHTEMP 175.0
#define LL_WATERVOL 0.5
#define UL_WATERVOL 30.0
#define LL_TIME 0.5
#define UL_TIME 300.0

// Text prompts
char t_enterweight[] = "Enter grain weight:";
char t_lbs[] = "lbs";
char t_entertemp[] = "Enter mash temp:";
char t_F[] = "deg F";
char t_entervol[] = "Enter water vol:";
char t_gal[] = "gal";
char t_entermashtime[] = "Enter mash time:";
char t_min[] = "mins";
char t_enterboiltime[] = "Enter boil time:";
char t_remainingtime[] = "Remaining time:";
char t_waitboil[] = "WAIT BOIL";
char t_space[] = "   ";

// Buttons
char b_preheat_text[] = "PRE-HEAT";
Elegoo_GFX_Button b_preheat;
char b_mash_text[] = "MASH";
Elegoo_GFX_Button b_mash;
char b_boil_text[] = "BOIL";
Elegoo_GFX_Button b_boil;
char b_back_text[] = "BACK";
Elegoo_GFX_Button b_back;
char b_next_text[] = "NEXT";
Elegoo_GFX_Button b_next;
char b_home_text[] = "HOME";
Elegoo_GFX_Button b_home;
char b_powerup_text[] = "POWER++";
Elegoo_GFX_Button b_powerup;
char b_powerdown_text[] = "POWER--";
Elegoo_GFX_Button b_powerdown;
char b_digits_text[12][4] = {"1","2","3","4","5","6","7","8","9","CLR","0","."};
Elegoo_GFX_Button b_digits[12];

// Screen state
enum Screen {HOME, WATER_VOL, GRAIN_WEIGHT, STRIKE_TEMP, PREHEAT, MASH_TEMP, MASH_TIME, MASH, BOIL_TIME, WAIT_BOIL, BOIL};
Screen curr_screen = HOME;

// Set burner kilowatts
volatile int var_dutycycle_ms = 0;
void set_kw(float kw){
  if(kw<0.0){
    var_kw = 0.0;
    var_dutycycle_ms = 0;
  }else if(kw > MAX_KW){
    var_kw = MAX_KW;
    var_dutycycle_ms = 1000;
  }else{
    var_kw = kw;
    var_dutycycle_ms = (int)(var_kw / MAX_KW * 1000.0);
  }
}

// PWM CONTROL
// Should be called once every ms using an ISR
int pwm_counter_ms = 0;
int pwm_top_ms = 1000;
void service_pwm()
{
  if( pwm_counter_ms >= var_dutycycle_ms )
  {
    digitalWrite( PIN_RELAY, LOW);
    digitalWrite (PIN_LED, LOW);
  }else
  {
    digitalWrite( PIN_RELAY, HIGH);
    digitalWrite (PIN_LED, HIGH);
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
const float MIN_RIMS_FLOWRATE_GPM = 0.5; // Minimum acceptable flow rate to keeps RIMS on
void service_flowmeter()
{
  // If it's been > 1 second, calculate and update flow rate
  unsigned long millis_now = millis();
  if( millis_now - millis_lastflowrate > FLOW_UPDATE_INTERVAL_MS )
  {
    var_gpm = ((float)pulses) / ((float) (millis_now - millis_lastflowrate) ) * 1000.0 / 485.0 * LPS_2_GPM;
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
  // Service flow meter
  service_flowmeter();

  // Service PWM relay output
  service_pwm();
}

uint16_t rgb_to_565(uint8_t r, uint8_t g, uint8_t b){
  return ( (r & 0xF8) << 8 ) | ( (g & 0xFC) << 3 ) | ( (b & 0xF8) >> 3 );
}

bool check_button(Elegoo_GFX_Button * b, TSPoint * p){
  if(b->contains(p->x,p->y) && p->z > MINPRESSURE && p->z < MAXPRESSURE){
    b->press(true);
  }else{
    b->press(false);
  }
  if(b->justReleased()){
    b->drawButton();
    return true;
  }
  if(b->justPressed()){
    b->drawButton(true);
  }
  return false;
}

void update_remainingtime(){
  if(var_displaytime_sec < 0 ){
    sprintf(t_displaytime, "---:--");
  }else{
    sprintf(t_displaytime, "%3d:%02d", var_displaytime_sec/60, var_displaytime_sec%60 );
  }
  tft.setCursor( TOP_LABEL_X+70, TOP_LABEL_Y + 30 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(t_displaytime);
}

#define TEMP_X 180
#define BUG_W 10
int lastY_temp = 500;
void draw_temp(){
  lastY_temp = 500;
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.setCursor( TEMP_X-30, BAR_Y+BAR_HEIGHT +20 );
  tft.print(t_F);
  tft.drawFastVLine(TEMP_X, BAR_Y, BAR_HEIGHT, rgb_to_565(0,0,0));
  int w = 10;
  tft.drawFastHLine(TEMP_X-w, BAR_Y, w*2, rgb_to_565(0,0,0)); // Top
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT/4, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT/2, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT*3/4, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT, w*2, rgb_to_565(0,0,0)); // Bottom
  w = 5;
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT/8, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT*3/8, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT*5/8, w*2, rgb_to_565(0,0,0));
  tft.drawFastHLine(TEMP_X-w, BAR_Y+BAR_HEIGHT*7/8, w*2, rgb_to_565(0,0,0));
  sprintf(t_mintemp, "%3d", (int)MIN_TEMP );
  sprintf(t_maxtemp, "%3d", (int)MAX_TEMP );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(1);
  tft.setCursor( TEMP_X-10, BAR_Y-10 );
  tft.print(t_maxtemp);
  tft.setCursor( TEMP_X-10, BAR_Y+BAR_HEIGHT+5 );
  tft.print(t_mintemp);

  sprintf(t_settemp, "%3d", (int)var_tempsetpoint_F );
  int thisY = get_Y( BAR_Y, BAR_HEIGHT, MIN_TEMP, MAX_TEMP, var_tempsetpoint_F);
  tft.setTextSize(2);
  tft.setCursor( TEMP_X+22, thisY-8 );
  tft.print(t_settemp);
  tft.setCursor( TEMP_X+22, thisY+10 );
  tft.print(t_set);
  tft.fillTriangle(TEMP_X+10, thisY, TEMP_X+10+BUG_W, thisY-BUG_W,TEMP_X+10+BUG_W, thisY+BUG_W, rgb_to_565(0,0,0));
}

int get_Y( int offset, int height, float min, float max, float val ){
  if( val < min ){
    return offset+height;
  }
  if( val > max ){
    return offset;
  }
  return offset + height - (int)( ((float)height)* (val-min)/(max-min) );
}


void update_temp(){
  int thisY = get_Y( BAR_Y, BAR_HEIGHT, MIN_TEMP, MAX_TEMP, var_temp);
  if( lastY_temp != thisY){
    // Erase last temp
    tft.setTextSize(2);
    tft.setCursor( TEMP_X-58, lastY_temp-8 );
    tft.print(t_space);
    tft.fillTriangle(TEMP_X-10, lastY_temp, TEMP_X-10-BUG_W, lastY_temp-BUG_W,TEMP_X-10-BUG_W, lastY_temp+BUG_W, rgb_to_565(255,255,255));
  
    // Draw new bug
    tft.fillTriangle(TEMP_X-10, thisY, TEMP_X-10-BUG_W, thisY-BUG_W,TEMP_X-10-BUG_W, thisY+BUG_W, rgb_to_565(0,0,0));
    lastY_temp = thisY;
  }

  if( var_temp >= TEMPSENSOR_MIN_F && var_temp <= TEMPSENSOR_MAX_F ){
    sprintf(t_currtemp, "%3d", (int)var_temp ); 
  }else{
    sprintf(t_currtemp, "ERR" ); 
  }
  tft.setTextSize(2);
  tft.setCursor( TEMP_X-58, thisY-8 );
  tft.print(t_currtemp);
  
}

#define KW_X 20
int lastY_kw = 500;
void draw_kw(){
  lastY_kw = 500;
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.setCursor( KW_X+5, BAR_Y+BAR_HEIGHT +20 );
  tft.print(t_kw);
  tft.drawRect(KW_X-1, BAR_Y-1, 33+2, BAR_HEIGHT+1, rgb_to_565(0,0,0));
  // 2/3 to 1 gpm
  tft.fillRect(KW_X, BAR_Y, 33, BAR_HEIGHT/3-1, rgb_to_565(255,0,0));
  // 1/3 to 2/3 gpm
  tft.fillRect(KW_X, BAR_Y+BAR_HEIGHT/3, 33, BAR_HEIGHT/3-1, rgb_to_565(0xF9,0xCB,0x9C));
  // 0 to 1/3 gpm
  tft.fillRect(KW_X, BAR_Y+BAR_HEIGHT*2/3, 33, BAR_HEIGHT/3, rgb_to_565(0xFF,0xF2,0xCC));
}

void update_kw(){
  int thisY = get_Y( BAR_Y, BAR_HEIGHT, 0.0, MAX_KW, var_kw);
    if( lastY_kw != thisY ){
      // Erase last bug
    tft.fillTriangle(KW_X-2, lastY_kw, KW_X-2-BUG_W, lastY_kw-BUG_W,KW_X-2-BUG_W, lastY_kw+BUG_W, rgb_to_565(255,255,255));
    // Draw new bug
    uint16_t bugcolor = rgb_to_565(0,0,0);
    if( var_kw > 0.66*MAX_KW){
      bugcolor = rgb_to_565(255,0,0);
    }else if( var_kw > 0.33*MAX_KW ){
      bugcolor = rgb_to_565(0xF9,0xCB,0x9C);
    }else if(var_kw > 0.0){
      bugcolor = rgb_to_565(0xFF,0xF2,0xCC);
    }
    tft.fillTriangle(KW_X-2, thisY, KW_X-2-BUG_W, thisY-BUG_W,KW_X-2-BUG_W, thisY+BUG_W, bugcolor);
    lastY_kw = thisY;
  }
  
  int tmp = (int)(var_kw*10.0);
  sprintf(t_kw_num, "%1d.%1d", tmp/10, tmp%10 );
  tft.setCursor( KW_X, BAR_Y+BAR_HEIGHT +5 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(t_kw_num);
}

#define GPM_X 75
int lastY_gpm = 500;
void draw_gpm(){
  lastY_gpm = 500;
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.setCursor( GPM_X, BAR_Y+BAR_HEIGHT +20 );
  tft.print(t_gpm);
  tft.drawRect(GPM_X-1, BAR_Y-1, 33+2, BAR_HEIGHT+1, rgb_to_565(0,0,0));
  // 1/8 to 1/4 maxgpm
  tft.fillRect(GPM_X, BAR_Y+BAR_HEIGHT*3/4, 33, BAR_HEIGHT/8-1, rgb_to_565(255,255,0));
  // 0 to 1/8 maxgpm
  tft.fillRect(GPM_X, BAR_Y+BAR_HEIGHT*7/8, 33, BAR_HEIGHT/8, rgb_to_565(255,0,0));
  // 1/4 to 1 maxgpm
  tft.fillRect(GPM_X, BAR_Y, 33, 3*BAR_HEIGHT/4-1, rgb_to_565(0,255,0));
}

void update_gpm(){
  int thisY = get_Y( BAR_Y, BAR_HEIGHT, 0.0, MAX_GPM, var_gpm);
  if( lastY_gpm != thisY ){
    // Erase last bug
    tft.fillTriangle(GPM_X-2, lastY_gpm, GPM_X-2-BUG_W, lastY_gpm-BUG_W,GPM_X-2-BUG_W, lastY_gpm+BUG_W, rgb_to_565(255,255,255));
  
    // Draw new bug
    uint16_t bugcolor = rgb_to_565(255,0,0);
    if( var_gpm > 0.25*MAX_GPM){
      bugcolor = rgb_to_565(0,255,0);
    }else if( var_gpm > 0.125*MAX_GPM ){
      bugcolor = rgb_to_565(255,255,0);
    }
    
    tft.fillTriangle(GPM_X-2, thisY, GPM_X-2-BUG_W, thisY-BUG_W,GPM_X-2-BUG_W, thisY+BUG_W, bugcolor);
  
    lastY_gpm = thisY;
  }
  
  int tmp = (int)(var_gpm*10);
  sprintf(t_gpm_num, "%1d.%1d", tmp/10, tmp%10 );
  tft.setCursor( GPM_X, BAR_Y+BAR_HEIGHT +5 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(t_gpm_num);
}

const int len_data_entry = 5;
int ind_data_entry = 0;
char s_data_entry[len_data_entry+1] = "     ";
void draw_dataentry(){
  tft.setCursor( TOP_LABEL_X+15, TOP_LABEL_Y + 30 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(s_data_entry);
}

bool parse_dataentry(float &val, float lower_limit, float upper_limit){
  val = atof(s_data_entry);
  if(val>=lower_limit && val <= upper_limit){
    return true;
  }
  tft.drawRect(TOP_LABEL_X, TOP_LABEL_Y+22, 220, 30, rgb_to_565(255,0,0));
  return false;
}

void reset_dataentry(){
  for(int i=0;i<len_data_entry;i++){
    s_data_entry[i]=' ';
  }
  ind_data_entry = 0;
}

void draw_keypad(){
  for(int i=0;i<12;i++){
    b_digits[i].drawButton();
  }
}

void service_keypad(TSPoint * p){
  for(int i=0;i<12;i++){
    if(check_button(&b_digits[i], p)){
      if(i==9){
        if(ind_data_entry > 0){
          ind_data_entry--;
          s_data_entry[ind_data_entry] = ' ';
        }
      }else{
        if(ind_data_entry<len_data_entry){
          s_data_entry[ind_data_entry] = b_digits_text[i][0];
          ind_data_entry++;
        }
      }
      draw_dataentry();
    }
  }
  
}

void draw_prompt(char * prompt, char * units){
  tft.fillRect(TOP_LABEL_X, TOP_LABEL_Y+22, 220, 30, rgb_to_565(255,255,255));
  tft.setCursor( TOP_LABEL_X, TOP_LABEL_Y );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(prompt);
  tft.setCursor( TOP_LABEL_X+155, TOP_LABEL_Y + 30 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(units);
  tft.drawRect(TOP_LABEL_X, TOP_LABEL_Y+22, 220, 30, rgb_to_565(0,0,0));
}

void draw_home(){
  tft.fillScreen(rgb_to_565(255,255,255));
  b_preheat.drawButton();
  b_mash.drawButton();
  b_boil.drawButton();
}

void service_home(TSPoint * p){
  if(check_button(&b_preheat, p)){
    curr_screen = WATER_VOL;
    draw_watervol();
  }
  if(check_button(&b_mash,p)){
    curr_screen = MASH_TEMP;
    draw_mashtemp();
  }
  if(check_button(&b_boil,p)){
    curr_screen = BOIL_TIME;
    draw_boiltime();
  }
}

void draw_watervol(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_entervol, t_gal);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}

void service_watervol(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_home();
    curr_screen = HOME;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_WATERVOL, UL_WATERVOL)){
      var_watervol_gal = tmp;
      Serial.print("Water volume: ");
      Serial.println(var_watervol_gal);
      curr_screen = GRAIN_WEIGHT;
      draw_grainweight();
    }
  }
}

void draw_grainweight(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_enterweight, t_lbs);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}

void service_grainweight(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_watervol();
    curr_screen = WATER_VOL;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_GRAINWEIGHT, UL_GRAINWEIGHT)){
      var_grainweight_lbs = tmp;
      Serial.print("Grain weight: ");
      Serial.println(var_grainweight_lbs);
      curr_screen = STRIKE_TEMP;
      draw_striketemp();
    }
  }
}

void draw_striketemp(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_entertemp, t_F);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}


void service_striketemp(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_grainweight();
    curr_screen = GRAIN_WEIGHT;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_MASHTEMP, UL_MASHTEMP)){
      var_mashtemp_F = tmp;
      Serial.print("Mash temp: ");
      Serial.println(var_mashtemp_F);
      var_tempsetpoint_F = 0.2 / ( var_watervol_gal * 4.0 / var_grainweight_lbs ) * (var_mashtemp_F - 65.0) + var_mashtemp_F;
      curr_screen = PREHEAT;
      draw_preheat();
    }
  }
}


void draw_mashtemp(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_entertemp, t_F);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}

void service_mashtemp(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_home();
    curr_screen = HOME;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_MASHTEMP, UL_MASHTEMP)){
      var_mashtemp_F = tmp;
      Serial.print("Mash temp: ");
      Serial.println(var_mashtemp_F);
      curr_screen = MASH_TIME;
      draw_mashtime();
    }
  }
}

void draw_mashtime(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_entermashtime, t_min);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}

void service_mashtime(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_mashtemp();
    curr_screen = MASH_TEMP;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_TIME, UL_TIME)){
      var_mashtime_min = tmp;
      Serial.print("Mash time: ");
      Serial.println(var_mashtime_min);
      var_tempsetpoint_F = var_mashtemp_F;
      var_starttime_ms = millis();
      curr_screen = MASH;
      draw_mash();
    }
  }
}

void draw_boiltime(){
  tft.fillScreen(rgb_to_565(255,255,255));
  reset_dataentry();
  draw_prompt(t_enterboiltime, t_min);
  draw_dataentry();
  draw_keypad();
  b_back.drawButton();
  b_next.drawButton();
}

void service_boiltime(TSPoint * p){
  service_keypad(p);
  if(check_button(&b_back,p)){
    draw_home();
    curr_screen = HOME;
  }
  if(check_button(&b_next,p)){
    float tmp;
    if(parse_dataentry(tmp, LL_TIME, UL_TIME)){
      var_boiltime_min = tmp;
      Serial.print("Boil time: ");
      Serial.println(var_boiltime_min);
      set_kw(MAX_KW);
      curr_screen = WAIT_BOIL;
      draw_waitboil();
    }
  }
}

unsigned long first_temp_reading_ms = 0;
float first_temp_reading_F = TEMPSENSOR_MIN_F - 1.0;
unsigned long take_temp_reading_ms = 0;
long est_time_done_ms = -1;
void draw_preheat(){
  tft.fillScreen(rgb_to_565(255,255,255));
  draw_kw();
  draw_gpm();
  draw_temp();
  draw_prompt(t_remainingtime, t_space);
  b_back.drawButton();
  b_home.drawButton();

  // Prep variables for preheat remaining time estimate
  est_time_done_ms = -1;
  first_temp_reading_F = TEMPSENSOR_MIN_F - 1.0;
  take_temp_reading_ms = millis() + 1000; // Take first reading one second from now
  var_temp = TEMPSENSOR_MIN_F - 1.0; // Ensure a new valid temp reading is used
}

void service_preheat(TSPoint * p){
  // Is it time to update the remaining time estimate?
  if( millis() > take_temp_reading_ms ){
    // Set remaining time to invalid by default
    est_time_done_ms = -1;
    
    // Has the first reading been taken?
    if( first_temp_reading_F < TEMPSENSOR_MIN_F ){
      if( var_temp >= TEMPSENSOR_MIN_F && var_temp <= TEMPSENSOR_MAX_F ){
        first_temp_reading_F = var_temp;
        first_temp_reading_ms = millis();
      }
    }else{
      // Make sure current reading is valid
      if( var_temp >= TEMPSENSOR_MIN_F && var_temp <= TEMPSENSOR_MAX_F ){
        // Make sure current temp is greater than first reading
        if( var_temp > first_temp_reading_F ){
            est_time_done_ms = millis() + (long)( ( var_tempsetpoint_F - var_temp ) * (float)( (millis()-first_temp_reading_ms) ) / (var_temp - first_temp_reading_F) );
        }
      }
    }
    // Update estimate 20 seconds from now
    take_temp_reading_ms = millis() + 20000;
  }

  if(est_time_done_ms < 0 ){
    var_displaytime_sec = -1;
  }else{
    var_displaytime_sec = (est_time_done_ms - millis())/1000;
    if(var_displaytime_sec < 0){
      var_displaytime_sec = 0;
    }
  }
  
  // Update UI
  update_kw();
  update_gpm();
  update_temp();
  update_remainingtime();
  if(check_button(&b_back,p)){
    draw_striketemp();
    curr_screen = STRIKE_TEMP;
  }
  if(check_button(&b_home,p)){
      curr_screen = HOME;
      draw_home();
  }
}

void draw_mash(){
  tft.fillScreen(rgb_to_565(255,255,255));
  draw_kw();
  draw_gpm();
  draw_temp();
  draw_prompt(t_remainingtime, t_space);
  b_back.drawButton();
  b_home.drawButton();
}

void service_mash(TSPoint * p){
  long sec_since_start =  (millis() - var_starttime_ms)/1000;
  var_displaytime_sec = (int)(var_mashtime_min*60.0) - sec_since_start;
  if(var_displaytime_sec < 0){
    var_displaytime_sec = 0;
  }
  update_kw();
  update_gpm();
  update_temp();
  update_remainingtime();
  if(check_button(&b_back,p)){
    draw_mashtime();
    curr_screen = MASH_TIME;
  }
  if(check_button(&b_home,p)){
      curr_screen = HOME;
      draw_home();
  }
}

void draw_waitboil(){
  tft.fillScreen(rgb_to_565(255,255,255));
  draw_prompt(t_remainingtime, t_space);
  draw_kw();
  tft.setCursor( TOP_LABEL_X+55, TOP_LABEL_Y + 30 );
  tft.setTextColor( rgb_to_565(0,0,0), rgb_to_565(255,255,255));
  tft.setTextSize(2);
  tft.print(t_waitboil);
  b_back.drawButton();
  b_next.drawButton();
  b_powerup.drawButton();
  b_powerdown.drawButton();
}

void service_waitboil(TSPoint * p){
  update_kw();
  if(check_button(&b_back,p)){
    draw_boiltime();
    curr_screen = BOIL_TIME;
  }
  if(check_button(&b_next,p)){
      curr_screen = BOIL;
      draw_boil();
      var_starttime_ms = millis();
  }
  if(check_button(&b_powerup,p)){
      set_kw(var_kw + 0.5);
  }
  if(check_button(&b_powerdown,p)){
      set_kw(var_kw - 0.5);
  }
}

void draw_boil(){
  tft.fillScreen(rgb_to_565(255,255,255));
  draw_prompt(t_remainingtime, t_space);
  draw_kw();
  b_back.drawButton();
  b_home.drawButton();
  b_powerup.drawButton();
  b_powerdown.drawButton();
}

void service_boil(TSPoint * p){
  long sec_since_start =  (millis() - var_starttime_ms)/1000;
  var_displaytime_sec = (int)(var_boiltime_min*60.0) - sec_since_start;
  if(var_displaytime_sec < 0){
    var_displaytime_sec = 0;
  }
  update_remainingtime();
  update_kw();
  if(check_button(&b_back,p)){
    draw_boiltime();
    curr_screen = BOIL_TIME;
  }
  if(check_button(&b_home,p)){
      curr_screen = HOME;
      draw_home();
  }
  if(check_button(&b_powerup,p)){
      set_kw(var_kw + 0.5);
  }
  if(check_button(&b_powerdown,p)){
      set_kw(var_kw - 0.5);
  }
}


void setup() {
  // LED
  pinMode (PIN_LED, OUTPUT);
  
  // Flow meter setup
  pinMode(PIN_FLOWMETER, INPUT);
  
   // Relay output setup
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite( PIN_RELAY, LOW);

  // Service timer setup
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); // timerIsr will be called every 1000us = 1ms

  // Initialize thermometers and set resolution
  // 10-bit resolution gets 0.25 degC resolution w/ ~200ms sample time
  thermometers.begin();
  thermometers.setResolution(ADDR_RIMS_THERMO, 10);
  //thermometers.setResolution(ADDR_MT_THERMO, 10);
  
  Serial.begin(115200);
  #ifdef USE_Elegoo_SHIELD_PINOUT
  Serial.println(F("Using Elegoo 2.8\" TFT Arduino Shield Pinout"));
#else
  Serial.println(F("Using Elegoo 2.8\" TFT Breakout Board Pinout"));
#endif

  Serial.print("TFT size is "); Serial.print(tft.width()); Serial.print("x"); Serial.println(tft.height());

  tft.reset();

  uint16_t identifier = tft.readID();
  if(identifier == 0x9325) {
    Serial.println(F("Found ILI9325 LCD driver"));
  } else if(identifier == 0x9328) {
    Serial.println(F("Found ILI9328 LCD driver"));
  } else if(identifier == 0x4535) {
    Serial.println(F("Found LGDP4535 LCD driver"));
  }else if(identifier == 0x7575) {
    Serial.println(F("Found HX8347G LCD driver"));
  } else if(identifier == 0x9341) {
    Serial.println(F("Found ILI9341 LCD driver"));
  } else if(identifier == 0x8357) {
    Serial.println(F("Found HX8357D LCD driver"));
  } else if(identifier==0x0101)
  {     
      identifier=0x9341;
       Serial.println(F("Found 0x9341 LCD driver"));
  }else {
    Serial.print(F("Unknown LCD driver chip: "));
    Serial.println(identifier, HEX);
    Serial.println(F("If using the Elegoo 2.8\" TFT Arduino shield, the line:"));
    Serial.println(F("  #define USE_Elegoo_SHIELD_PINOUT"));
    Serial.println(F("should appear in the library header (Elegoo_TFT.h)."));
    Serial.println(F("If using the breakout board, it should NOT be #defined!"));
    Serial.println(F("Also if using the breakout, double-check that all wiring"));
    Serial.println(F("matches the tutorial."));
    identifier=0x9341;
   
  }

  tft.begin(identifier);
  tft.setRotation(2);

  // Color defs
  uint16_t color_outline = rgb_to_565(0x59,0x59,0x59);
  uint16_t color_lightyellow = rgb_to_565(0xFF,0xF2,0xCC);
  uint16_t color_lightorange = rgb_to_565(0xF9,0xCB,0x9C);
  uint16_t color_lightred = rgb_to_565(0xEA,0x99,0x99);
  uint16_t color_lightgreen = rgb_to_565(0xB6,0xD7,0xA8);
  uint16_t color_lightblue = rgb_to_565(0xA4,0xC2,0xF4);
  uint16_t color_black = rgb_to_565(0,0,0);
  uint16_t color_white = rgb_to_565(255,255,255);

  // Buttons init
  b_preheat.initButton(&tft, 120, 55, 200, 75, color_outline, color_lightyellow, color_black, b_preheat_text, 3);
  b_mash.initButton(&tft, 120, 160, 200, 75, color_outline, color_lightorange, color_black, b_mash_text, 3);
  b_boil.initButton(&tft, 120, 266, 200, 75, color_outline, color_lightred, color_black, b_boil_text, 3);
  b_back.initButton(&tft, 60, 290, 100, 50, color_outline, color_lightred, color_black, b_back_text, 3);
  b_next.initButton(&tft, 180, 290, 100, 50, color_outline, color_lightgreen, color_black, b_next_text, 3);
  b_home.initButton(&tft, 180, 290, 100, 50, color_outline, color_lightblue, color_black, b_home_text, 3);
  b_powerup.initButton(&tft, 150, 110, 150, 60, color_outline, rgb_to_565(255,0,0), color_black, b_powerup_text, 3);
  b_powerdown.initButton(&tft, 150, 180, 150, 60, color_outline, color_lightyellow, color_black, b_powerdown_text, 3);
  for(int row=0;row<4;row++){
    for(int col=0;col<3;col++){
      b_digits[col+3*row].initButton( &tft, 240/6*(2*col+1), 90+row*48, 73, 45, color_outline, color_black, color_white, b_digits_text[col+3*row], 3);
    }
  }

  // Start at home screen
  draw_home();
  curr_screen = HOME;
}

void loop() {
  TSPoint p = ts.getPoint();

  // if sharing pins, you'll need to fix the directions of the touchscreen pins
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  // scale from 0->1023 to tft.width
  p.x = map(p.x, TS_MINX, TS_MAXX, tft.width(), 0);
  p.y = (tft.height()-map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));
   

  switch( curr_screen ){
    case HOME:
      service_home(&p);
      break;
    case WATER_VOL:
      service_watervol(&p);
      break;
    case GRAIN_WEIGHT:
      service_grainweight(&p);
      break;
    case STRIKE_TEMP:
      service_striketemp(&p);
      break;
    case PREHEAT:
      service_preheat(&p);
      break;
    case MASH_TEMP:
      service_mashtemp(&p);
      break;
    case MASH_TIME:
      service_mashtime(&p);
      break;
    case MASH:
      service_mash(&p);
      break;
    case BOIL_TIME:
      service_boiltime(&p);
      break;
    case WAIT_BOIL:
      service_waitboil(&p);
      break;
    case BOIL:
      service_boil(&p);
      break;
   }

  // Control output relay for burner based on mode
   if(curr_screen == MASH || curr_screen == PREHEAT){
    if( var_temp < var_tempsetpoint_F && var_gpm > GPM_MASH_MIN && var_temp >= TEMPSENSOR_MIN_F && var_temp <= TEMPSENSOR_MAX_F){
      set_kw( MAX_KW );
    }else{
      set_kw( 0.0 );
    }
  }else if(curr_screen == BOIL || curr_screen == WAIT_BOIL){
    // Do nothing, controlled from UI buttons
  }else{
    set_kw( 0.0 );
  }

  // Read thermometers in relevant modes
  if(curr_screen == PREHEAT || curr_screen == MASH){
    // Send command to begin temperature conversion
    thermometers.requestTemperatures();
  
    // After conversion complete, read temperatures and verify reading in-range
    float temp = degC2degF(thermometers.getTempC(ADDR_RIMS_THERMO)) + RIMS_THERMO_OFFSET_F;
    //float temp = degC2degF(thermometers.getTempC(ADDR_MT_THERMO)) + MT_THERMO_OFFSET_F;
    if( temp >= TEMPSENSOR_MIN_F && temp < TEMPSENSOR_MAX_F ){
      var_temp = temp;
    }else{
      var_temp = TEMPSENSOR_MIN_F - 1.0;
    }
    
  }else{
    // Else delay to debounce touch events
    delay(100);
  }
  
}
