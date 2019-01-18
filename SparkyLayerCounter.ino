#include <LiquidCrystal.h>


const int photoPin = A0;      // input: phototransistor
const int buttonAPin = 8;     // input: - button
const int buttonBPin = 7;     // input: + button
const int backlightPin = A2;  // output: backlight enable
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int layer_no=0;         // current layer
int layer_count = 100;  // total number of layers
int layer_time = 0;     // duration of last layer
long start_time = 0;    // time elapsed since first layer
long last_refresh = 0;  // last time the display was updated
long backlight_time = 0;  // last time the backlight was switched on

// simple debouncing class
struct Debounced
{
  int pin;
  int cnt;
  int stat;
  int hysteresis;

  Debounced(int pinNumber, int hysteresis=10) : pin(pinNumber), cnt(0), stat(false), hysteresis(hysteresis) {}

  bool status()
  { 
    cnt += digitalRead(pin) ? -1 : 1;
    cnt = max(0, min(hysteresis, cnt));
  
    if(cnt == 0)
      stat = false;
    else if(cnt == hysteresis)
      stat = true;
  
    return stat;
  }

};

Debounced photoInput = Debounced(photoPin, 20),
          buttonA    = Debounced(buttonAPin),
          buttonB    = Debounced(buttonBPin);


// helper
void seconds_to_hm(char* buf, long seconds)
{
  sprintf(buf, "%dh%dm", int(min(99,seconds/3600)), int((seconds/60)%60));
}

//helper
void seconds_to_hms(char* buf, long seconds)
{
  sprintf(buf, "%dh%dm%ds", int(min(99,seconds/3600)), int((seconds/60)%60), int(seconds%60));
}


// print all the info on screen
void update_screen()
{
char buf[17], elapsed[10], remaining[7];

    last_refresh = millis();
    lcd.setCursor(0, 0);
    sprintf(buf, "%3ds %4d/%4d  ", layer_time, min(9999,layer_no), min(9999,layer_count));
    lcd.print(buf);  
    lcd.setCursor(0,1);
    seconds_to_hms(elapsed, (start_time>0) ? (millis()-start_time)/1000 : 0);
    seconds_to_hm(remaining, long(layer_count - layer_no)*layer_time);
    sprintf(buf, "%9s %6s", elapsed, remaining);
    lcd.write(buf);
}



// handle button actions (reset/set layer count)
void menu()
{
long last_time = 0, last_held = 0;
bool a, b, a_held = false, b_held = false;

  // switch on the backlight
  digitalWrite(backlightPin, 1);
  
  // wait for button release
  bool both_pressed = false;
  do {
    delay(10);
    a = buttonA.status();
    b = buttonB.status();
    if(a and b) 
      both_pressed = true;
  } while(a or b);

  if(both_pressed)
  // reset menu
  {
    lcd.clear();
    lcd.setCursor(1, 0);
    lcd.write("Reset counter?");
    lcd.setCursor(3, 1);
    lcd.write("yes   no");
    last_time = millis();
    while(millis() - last_time < 4000) // assume no after period of inactivity
    {
      if(buttonA.status()) 
      {
        layer_no = 0;
        start_time=0;
        break;
      }
      if(buttonB.status())
      {
        break;
      }
    }

    do {
      delay(10);
    } while(buttonA.status() or buttonB.status());
    update_screen();
    backlight_time = millis();
    return;
    
  }
  else
  // num layers menu
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write("number of layers");
    lcd.setCursor(0,1);
    char buf[17];
    sprintf(buf, " -    %4d    + ", layer_count);
    lcd.print(buf);
    last_time = millis();
    int stepSize = 1;
    while(millis() - last_time < 4000) // return after period of inactivity
    {
      if(buttonA.status() or a_held)
      {
        layer_count = max(1, layer_count-stepSize);
        last_time = millis();
        do 
        {
          delay(10);
          if(not buttonA.status()) {
            stepSize = 1;
            a_held = false;
            break;
          }
          if(millis() - last_time > 1000) 
          {
            a_held = true;
            last_held = millis();
            break;
          }
        }while(not a_held);
        last_time = millis();
        
        sprintf(buf, " -    %4d    + ", layer_count);
        lcd.setCursor(0,1);
        lcd.print(buf);
      }
      if(buttonB.status() or b_held)
      {
        layer_count = min(9999, layer_count+stepSize);
        last_time = millis();
        do
        {
          delay(10);
          if(not buttonB.status()) {
            stepSize = 1;
            b_held = false;
            break;
          }
          if(millis() - last_time > 1000) 
          {
            b_held = true;
            last_held = millis();
            break;
          }
        }while(not b_held);
        last_time = millis();
        
        sprintf(buf, " -    %4d    + ", layer_count);
        lcd.setCursor(0,1);
        lcd.print(buf);
      }

      if((a_held or b_held) and (millis() - last_held > 5000)) {
        stepSize += 20;
        last_held = millis();
      }
      
    }
    lcd.clear();
    update_screen();
    backlight_time = millis();
  }
}




void setup() {

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(photoPin, INPUT);
  pinMode(buttonAPin, INPUT_PULLUP);
  pinMode(buttonBPin, INPUT_PULLUP);
  pinMode(backlightPin, OUTPUT);
  lcd.begin(16, 2);
  update_screen();

}  



void loop() {
static long last_time = 0;

    // any activity on the buttons?
    if(buttonA.status() or buttonB.status()) {
      menu();
      //digitalWrite(LED_BUILTIN,0);
    }

    // check the phototransistor for (uv) light
    long now = millis();
    if(photoInput.status())
    {
      if(layer_no == 0) {  // let's start the show
        start_time = now;
        //digitalWrite(LED_BUILTIN,1);
      }
      layer_no++;
      if(layer_no > layer_count)
        layer_count = layer_no;
      layer_time = (now - last_time) / 1000;
      last_time = now;
      update_screen();
      backlight_time = now;
      
      // wait for the light to switch off before continuing
      while(photoInput.status()) { 
        now = millis();
        delay(10);
        // make sure to update the screen every now and then to refresh the elapsed time
        if(now - last_refresh > 1000) 
          update_screen();
        // switch off the backlight after a bit
        digitalWrite(backlightPin, (layer_no == 0 or now - backlight_time < 5000) ? 1 : 0);
      }
    }
    delay(10);
    if(now - last_refresh > 1000)  // make sure to update the screen every now and then to refresh the elapsed time
      update_screen();

    // switch off the backlight after a bit
    digitalWrite(backlightPin, (layer_no == 0 or now - backlight_time < 5000) ? 1 : 0);
}
