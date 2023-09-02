#include <Arduino.h>
#include <TimerOne.h>
#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <Wire.h>


#define BUTTON_PIN 2
#define POWER_SERVO_PIN 10
#define TIMER_SERVO_PIN 11

#define TIMER_ON 0
#define TIMER_OFF 1

#define STEP1 0 // 분 입력
#define STEP2 1 // 초 입력
#define STEP3 2 // 강도 입력

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
// A 해동
// B 약
// C 중
// D 강
// # enter
// * del
// LCD 로 표시

// 분 초
// 처음 0 입력 -> 초로 넘어가기
// 처음 1~9 입력 -> 다음 버튼 눌러야 넘어가고, 그렇지 않으면 숫자로 인식

// 강도 입력 단계에서 엔터(다음 버튼) 누르면 바로 작동

byte rowPins[ROWS] = {6, 7, 8, 9};
byte colPins[COLS] = {5, 4, 3, 2};

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

LiquidCrystal_I2C lcd(0x27, 16, 2);

String lcdOutput = "";

Servo timerServo;
Servo powerServo;

int seconds;
int step;

String str_time_setup = "Set the time!";
  
int mm = 0;
int ss = 0;
String strmm ="__";
String strss = "__";

String str_power_setup = "Set the power!";
char power = 'C';
String strpw = "_";

void initialize();
void readUsersCommand();
void servoMove(int servoPin, int pos);
// intertupt -> mw 작동중에 0 버튼 누르면 중단
void startWork();
void displayLeftTime(int leftTime);
void printLCD();


void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(POWER_SERVO_PIN, OUTPUT);
  pinMode(TIMER_SERVO_PIN, OUTPUT);

  lcd.init();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.backlight();
  // lcd.print("hello world");

  step = STEP1;
  timerServo.attach(TIMER_SERVO_PIN);
  powerServo.attach(POWER_SERVO_PIN);
  servoMove(POWER_SERVO_PIN, 'C');
  servoMove(TIMER_SERVO_PIN, TIMER_OFF);
}

void loop() {
  readUsersCommand();
  startWork();
  // print 연속으로 하면 옆에 이어서 써짐.
}


void initialize()
{
  step = STEP1;
  ss = 0;
  mm = 0;
  strss = "__";
  strmm = "__";
  lcd.clear();
  lcd.setCursor(0, 0);
  servoMove(TIMER_SERVO_PIN, TIMER_OFF);
  servoMove(POWER_SERVO_PIN, 'C');
}

void servoMove(int servoPin, int pos)
{
  if(servoPin == TIMER_SERVO_PIN)
  {
    if(pos == TIMER_ON) timerServo.write(10);
    else timerServo.write(100);
  }
  else if(servoPin == POWER_SERVO_PIN)
  {
    switch (pos)
    {
      case 'A':
        powerServo.write(0);
        break;
      case 'B':
        powerServo.write(30);
        break;
      case 'C':
        powerServo.write(60);
        break;
      case 'D':
        powerServo.write(90);
        break;
      
      default:
        // invalid -> re-press button
        break;
    }
  }
}

void readUsersCommand()
{
  bool done = false;

  while(!done)
  {
    printLCD();

    char key = keypad.waitForKey();
    if(step == STEP1)
    {
      // minutes
      ss = 0;
      strss = "__";

      switch (key)
      {
        case '#':
          step = STEP2;
          if(mm >= 10) strmm = String(mm);
          else strmm = "0" + String(mm);
          break;
        case '*':
          if(strmm.charAt(1) != '_') {
            // 00:__ -> 0_:__
            strmm[1] = '_';
            mm /= 10;
          }
          else if(strmm.charAt(0) != '_') {
            // 0_:__ -> __:__
            strmm = "__";
            mm = 0;
          }
          break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
          break;
        
        default: // 0..9
          if(mm >= 10)
          {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("It's too long");
            lcd.setCursor(0, 1);
            lcd.print("time. Dangerous");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            break;//while 을 끝내진 않겠지? -> while or switch?. purpose: switch
          }
          mm *= 10;
          mm += key - '0';
          if(mm >= 10) strmm = String(mm);
          else strmm = String(mm) + "_";
          break;
      }
    } // STEP1

    else if(step == STEP2)
    {
      // seconds
      // 60 이상이면 59로 처리
      switch (key)
      {
        case '#':
          step = STEP3;
          if(ss >= 10) strss = String(ss);
          else strss = "0" + String(ss);
          break;
        case '*':
          if(strss.charAt(1) != '_') {
            // 00:00 -> 00:0_
            strss[1] = '_';
            ss /= 10;
          }
          else if(strss.charAt(0) != '_') {
            // 00:0_ -> 00:__
            strss[0] = '_';
            ss = 0;
          }
          else {
            // 00:__ -> STEP1
            step = STEP1;
            // strmm[1] = '_'; // 00:__ -> 0_:__
            // ss %= 10;
          }
          break;
        
        case 'A':
        case 'B':
        case 'C':
        case 'D':
          break;
        
        default: // 0..9
          if(ss >= 10) break;
          ss *= 10;
          ss += key - '0';
          if(ss >= 60) ss = 59;

          if(ss >= 10) strss = String(ss);
          else strss = String(ss) + "_";
          break;
      }
    } // STEP2

    else if(step == STEP3)
    {
      // power
      seconds = mm * 60 + ss;
      switch (key)
      {
        case '#':
          done = true;
          break;
        case '*':
          if(strpw == "_") step = STEP2;
          strpw = "_";
          break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
          power = key;
          strpw = String(key);
          break;
        
        default:
          break;
      }

    } // STEP3

    // delay(500);
  } // while
}

void startWork()
{
  // start work for the user's command
  servoMove(POWER_SERVO_PIN, power);
  servoMove(TIMER_SERVO_PIN, TIMER_ON);
  displayLeftTime(seconds);
  servoMove(TIMER_SERVO_PIN, TIMER_OFF);

  initialize();
}

void displayLeftTime(int leftTime)
{
  unsigned int t1, t2;
  do
  {
    t1 = millis();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(leftTime) + "s left..");
    lcd.setCursor(0, 1);
    lcd.print("Press * to stop");

    char key = keypad.getKey(); // check whether any character is stored in the buffer
    if(key == '*')
    {
      initialize();
      break;
    }

    t2 = millis();
    delay(1000 + t1 - t2);
  } while (leftTime--);
  step = STEP1;
}

void printLCD()
{
  lcd.clear();
  lcd.setCursor(0, 0);

  if(step == STEP3)
  {
    lcd.print(str_power_setup);
    lcd.setCursor(0, 1);
    lcd.print(strpw);
  }
  else
  {
    lcd.print(str_time_setup);
    lcd.setCursor(0, 1);
    lcd.print(strmm + ":" + strss);
  }
}

