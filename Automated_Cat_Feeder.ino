//Automated Cat Feeder

#include <EEPROM.h> //For saving settings in memory

#include "AudioTools.h" //Audio tools needed for Talkie library
#include "TalkiePCM.h" //Robotic Voice PCM Output Library
#include "Vocab_US_Large.h" //Vocabulary for Talkie

const int buttonA = 2; //Button A
const int buttonB = 3; //Button B
const int motorPin = 9; //Dispenser Motor

const float secsPerTeespoon = 0.25568182; //Seconds per teespoon (Experimental Value)

unsigned long timerMillis;
unsigned long tempMillis = 0;
unsigned long clockSetMillis = 0;
unsigned long refStartMillis = 0;

//Settings Navigation Variables
int oldButtonA = LOW;
int oldButtonB = LOW;
int pageA = 0; //Settings groups
int pageB = 0; //Setting parameters
bool edit = false; //Editing parameter
int timeEditIndex = 0; //Which time/unit volume is being edited
bool settingsMenu = false;

//Dispense Time
byte setHours[8] = {};
byte setMinutes[8] = {};
bool setAMPM[8] = {};
byte numTimes = 0; //Number of times set

//Clock Time
byte clockHour = 12;
byte clockMinute = 0;
bool clockAMPM = false;

byte oldClock = 0; //Used to check for update every minute

//Serving Amount (Teaspoons)
byte unitWeights[8] = {};

//Creates an audio stream on pin A0
AnalogAudioStream out;
AudioInfo info(8000, 1, 16); //Sample Rate: 8000Hz; 1 Mono Channel; Bit Depth: 16
TalkiePCM voice(out, info.channels);

//Accurate Modulus Operator
float mod(float m, float n){
  return m - n * floor(m / n);
}

//Say any number from 0 to 59
void sayNum(int num){
  switch((int) floor(num / 10)){ //Tens place
    case(2):
      voice.say(sp2_TWENTY);
    break;
    case(3):
      voice.say(sp3_THIRTY);
    break;
    case(4):
      voice.say(sp3_FOURTY);
    break;
    case(5):
      voice.say(sp3_FIFTY);
    break;
  }
  switch(num){ //0-20
    case(0):
      voice.say(sp2_ZERO);
    break;
    case(10):
      voice.say(sp2_TEN);
    break;
    case(11):
      voice.say(sp2_ELEVEN);
    break;
    case(12):
      voice.say(sp2_TWELVE);
    break;
    case(13):
      voice.say(sp2_THIR_);
      voice.say(sp2__TEEN);
    break;
    case(14):
      voice.say(sp2_FOUR);
      voice.say(sp2__TEEN);
    break;
    case(15):
      voice.say(sp2_FIF_);
      voice.say(sp2__TEEN);
    break;
    case(16):
      voice.say(sp2_SIX);
      voice.say(sp2__TEEN);
    break;
    case(17):
      voice.say(sp2_SEVEN);
      voice.say(sp2__TEEN);
    break;
    case(18):
      voice.say(sp2_EIGHT);
      voice.say(sp2__TEEN);
    break;
    case(19):
      voice.say(sp2_NINE);
      voice.say(sp2__TEEN);
    break;
    default:
      switch(num % 10){ //Ones Place
        case(1):
          voice.say(sp2_ONE);
        break;
        case(2):
          voice.say(sp2_TWO);
        break;
        case(3):
          voice.say(sp2_THREE);
        break;
        case(4):
          voice.say(sp2_FOUR);
        break;
        case(5):
          voice.say(sp2_FIVE);
        break;
        case(6):
          voice.say(sp2_SIX);
        break;
        case(7):
          voice.say(sp2_SEVEN);
        break;
        case(8):
          voice.say(sp2_EIGHT);
        break;
        case(9):
          voice.say(sp2_NINE);
        break;
      }
    break;
  }
}

//Say the full time given the hour, minute, and AM/PM
void sayTime(int hour, int minute, bool AMPM){
  sayNum(hour);

  if(minute == 0){
    voice.say(sp3_OCLOCK);
  }else{
    sayNum(minute);
  }

  if(AMPM){
    voice.say(sp2_P);
    voice.say(sp2_M);
  }else{
    voice.say(sp2_A);
    voice.say(sp2_M);
  }
}

void setup(){
  Serial.begin(9600);

  //Retrieve saved settings from EEPROM
  EEPROM.get(0, setHours);
  EEPROM.get(8, setMinutes);
  EEPROM.get(16, setAMPM);
  EEPROM.get(24, unitWeights);
  EEPROM.get(32, numTimes);
  
  //Find default EEPROM values and replace with compatible values
  if(setHours[0] == -1) memset(setHours, 0, sizeof(setHours));
  if(setMinutes[0] == -1) memset(setMinutes, 0, sizeof(setMinutes));
  if(((byte*)setAMPM)[0] == 255) memset(setAMPM, false, sizeof(setAMPM));
  if(unitWeights[0] == -1) memset(unitWeights, 0, sizeof(unitWeights));
  if(numTimes == -1) numTimes = 0;

  //Define Pins
  pinMode(motorPin, OUTPUT);

  //Start audio streaming
  auto config = out.defaultConfig(TX_MODE); //Get the default Analog configuration for Output (TX) mode
  config.copyFrom(info); //Map the 8kHz, 1-channel, 16-bit settings into the configuration
  out.begin(config); //Start the analog audio stream

  voice.say(sp2_POWER);
  voice.say(sp2_ON);
}

void loop(){
  if(!settingsMenu){
    timerMillis = millis();

    if(tempMillis - timerMillis >= 216000000000){ //If Millis() resets, adjust the starting measurement time
      clockSetMillis = timerMillis + 47296; //Add leftover time since the last minute before reset (mod(2^32, 60000))
    }else{
      clockHour = mod((floor(mod(refStartMillis + timerMillis - clockSetMillis, 43200000) / 3600000) - 1), 12) + 1;
      clockMinute = floor(mod(refStartMillis + timerMillis - clockSetMillis, 3600000) / 60000);
      clockAMPM = (bool) floor(mod(refStartMillis + timerMillis - clockSetMillis, 86400000) / 3600000);
    }

    tempMillis = timerMillis;

    if(clockMinute != oldClock){
      Serial.print(String(clockHour) + ":" + String(clockMinute) + " ");

      if(clockAMPM){
        Serial.println("PM");
      }else{
        Serial.println("AM");
      }

      for(int i = 0; i < sizeof(setHours); i++){
        if(clockHour == setHours[i] && clockMinute == setMinutes[i] && clockAMPM == setAMPM[i]){
          analogWrite(motorPin, 255);
          delay(1000 * unitWeights[i] * secsPerTeespoon);
          analogWrite(motorPin, 0);
          break;
        }
      }

      oldClock = clockMinute;
    }
  }

  handleSettingsNav(); //Navigate settings menu
}

//Controls the navgiation of the settings menus
void handleSettingsNav(){
  if((digitalRead(buttonA) == HIGH) && (oldButtonA == LOW)){ //Button A press
    switch(pageA){
      case(0): //If not in the settings menu
        if(pageB == 0){ //Entering the settings menu
            settingsMenu = true;
            pageA = 1;
            voice.say(sp2_SET);
            voice.say(sp2_TIME);
          break;
        }
      break;
      case(1): //If 'Set Time' is selected
        switch(pageB){
          case(0): //Chose 'Set Time'
            timeEditIndex = 0;
            pageB = 1;
            if(setHours[timeEditIndex] == 0){
              setHours[timeEditIndex] = 12; //Loops the hours to 12 if initializing from 0
              numTimes++;
            }

            voice.say(sp2_SET);
            voice.say(sp3_HOUR);
          break;
          case(1): //Chose 'Set Hour'
            edit = !edit;

            if(edit){
              sayNum(setHours[timeEditIndex]);
            }else{
              sayTime(setHours[timeEditIndex], setMinutes[timeEditIndex], setAMPM[timeEditIndex]);
            }
          break;
          case(2): //Chose 'Set Minutes'
            edit = !edit;

            if(edit){
              sayNum(setMinutes[timeEditIndex]);
            }else{
              sayTime(setHours[timeEditIndex], setMinutes[timeEditIndex], setAMPM[timeEditIndex]);
            }
          break;
          case(3): //AM/PM
            setAMPM[timeEditIndex] = !setAMPM[timeEditIndex];

            if(setAMPM[timeEditIndex]){
              voice.say(sp2_P);
              voice.say(sp2_M);
            }else{
              voice.say(sp2_A);
              voice.say(sp2_M);
            }
          break;
          case(4): //Chose 'Plus other time?'
            timeEditIndex++;
            pageB = 1;
            if(setHours[timeEditIndex] == 0){
              setHours[timeEditIndex] = 12; //Loops the hours to 12 if initializing from 0
              numTimes++;
            }

            voice.say(sp2_SET);
            voice.say(sp3_HOUR);
          break;
          case(5): //Chose 'Enter'
            //Save time data in EEPROM
            EEPROM.put(0, setHours);
            EEPROM.put(8, setMinutes);
            EEPROM.put(16, setAMPM);
            EEPROM.put(32, numTimes);

            timeEditIndex = 0;
            pageB = 0;

            voice.say(sp2_SAFE);
            delay(100);
            voice.say(sp2_SET);
            voice.say(sp2_TIME);
          break;
        }
      break;
      case(2): //If 'Set Clock' is selected
        switch(pageB){
          case(0): //Chose 'Set Clock'
            pageB = 1;

            voice.say(sp2_SET);
            voice.say(sp3_HOUR);
          break;
          case(1): //Chose 'Set Hour'
            edit = !edit;
            
            if(edit){
              sayNum(clockHour);
            }else{
              sayTime(clockHour, clockMinute, clockAMPM);
            }
          break;
          case(2): //Chose 'Set Minutes'
            edit = !edit;

            if(edit){
              sayNum(clockMinute);
            }else{
              sayTime(clockHour, clockMinute, clockAMPM);
            }
          break;
          case(3): //Chose 'AM/PM'
            clockAMPM = !clockAMPM;

            if(clockAMPM){
              voice.say(sp2_P);
              voice.say(sp2_M);
            }else{
              voice.say(sp2_A);
              voice.say(sp2_M);
            }
          break;
          case(4): //Chose 'Enter'
            clockSetMillis = millis();
            refStartMillis = 3600000 * mod(clockHour, 12) + 60000 * clockMinute;
            if(clockAMPM){
              refStartMillis += 43200000;
            }

            pageB = 0;

            voice.say(sp2_SAFE);
            delay(100);
            voice.say(sp2_SET);
            voice.say(sp2_TIME);
          break;
        }
      break;
      case(3): //If 'Control Unit Weight' is selected
        if(pageB == 0){ //Chose 'Control Unit Weight'
          pageB = 1;
          voice.say(sp2_SET);
          voice.say(sp2_UNIT);
        }else if(pageB == numTimes + 1){ //Chose 'Enter'
          //Save serving amount data in EEPROM
          EEPROM.put(24, unitWeights);

          pageB = 0;

          voice.say(sp2_SAFE);
          delay(100);
          voice.say(sp2_SET);
          voice.say(sp2_TIME);
        }else{ //Chose 'Set Unit'
          edit = !edit;

          sayNum(unitWeights[pageB - 1]);

          //"Teaspoons"
          voice.say(sp2__TEEN);
          voice.say(sp5_BOOST);
        }
      break;
      case(4): //Chose 'Enter' for all settings
        settingsMenu = false;
        pageA = 0;
        pageB = 0;
        
        voice.say(sp2_SAFE);
      break;
      case(5): //Chose 'Clear All'
        //Clear all settings to default
        memset(setHours, 0, sizeof(setHours));
        memset(setMinutes, 0, sizeof(setMinutes));
        memset(setAMPM, false, sizeof(setAMPM));
        memset(unitWeights, 0, sizeof(unitWeights));
        numTimes = 0;

        //Update EEPROM after clearing
        EEPROM.put(0, setHours);
        EEPROM.put(8, setMinutes);
        EEPROM.put(16, setAMPM);
        EEPROM.put(24, unitWeights);
        EEPROM.put(32, numTimes);

        pageA = 0;
        pageB = 0;

        voice.say(sp3_CLEAR);
      break;
    }
  }
  if(digitalRead(buttonA) != oldButtonA){
    oldButtonA = digitalRead(buttonA);
  }

  if((digitalRead(buttonB) == HIGH) && (oldButtonB == LOW)){ //Button B press
    if(pageA > 0){ //If in settings menu
      //If not in edit mode, then increment page numbers
      if(!edit){
        if(pageB == 0){
          //Loop through five settings options
          pageA++;
          if(pageA > 5){
            pageA = 1;
          }
        }else{
          pageB++;
        }
      }

      switch(pageA){
        case(1):
          //Only 5 pages on 'Set Time'
          if(pageB > 5){
            pageB = 1;
          }

          switch(pageB){
            case(0): //If 'Set Time' is selected
              voice.say(sp2_SET);
              voice.say(sp2_TIME);
            break;
            case(1): //If 'Set Hour' is selected
              if(edit){ //If editing 'Set Hour'
                setHours[timeEditIndex]++;
                setHours[timeEditIndex] = mod((setHours[timeEditIndex] - 1), 12) + 1; //Loops the hours back to 1 after 12
                sayNum(setHours[timeEditIndex]);
              }else{
                voice.say(sp2_SET);
                voice.say(sp3_HOUR);
              }
            break;
            case(2): //If 'Set Minutes' is selected
              if(edit){ //If editing 'Set Minutes'
                setMinutes[timeEditIndex] += 5;
                setMinutes[timeEditIndex] = setMinutes[timeEditIndex] % 60; //Loops the minutes back to 0 after 59
                sayNum(setMinutes[timeEditIndex]);
              }else{
                voice.say(sp2_SET);
                voice.say(sp2_MINUTES);
              }
            break;
            case(3): //If AM/PM is selected
              if(setAMPM[timeEditIndex]){ //If PM
                voice.say(sp2_P);
                voice.say(sp2_M);
              }else{ //If AM
                voice.say(sp2_A);
                voice.say(sp2_M);
              }
            break;
            case(4): //If 'Plus other time' is selected
              sayTime(setHours[timeEditIndex], setMinutes[timeEditIndex], setAMPM[timeEditIndex]);

              delay(500);

              voice.say(sp2_PLUS);
              voice.say(sp4_OTHER);
              voice.say(sp2_TIME);
            break;
            case(5): //If 'Enter' for 'Set Time' is selected
              for(int i = 0; i < 8; i++){
                if(setHours[i] != 0){
                  sayTime(setHours[i], setMinutes[i], setAMPM[i]);
                  delay(200);
                }
              }

              delay(500);

              voice.say(sp2_ENTER);
            break;
          }
        break;
        case(2):
          //Only 4 pages on 'Set Clock'
          if(pageB > 4){
            pageB = 1;
          }

          switch(pageB){
            case(0): //If 'Set Clock' is selected
              voice.say(sp2_SET);
              voice.say(sp2_CLOCK);
            break;
            case(1): //If 'Set Hour' for clock is selected
              if(edit){ //If editing 'Set Hour'
                clockHour++;
                clockHour = mod((clockHour - 1), 12) + 1; //Loops the hours back to 1 after 12
                sayNum(clockHour);
              }else{
                voice.say(sp2_SET);
                voice.say(sp3_HOUR);
              }
            break;
            case(2): //If 'Set Minutes' for clock is selected
              if(edit){ //If editing 'Set Minutes'
                clockMinute++;
                clockMinute = clockMinute % 60; //Loops the minutes back to 0 after 59
                sayNum(clockMinute);
              }else{
                voice.say(sp2_SET);
                voice.say(sp2_MINUTES);
              }
            break;
            case(3): //If 'AM/PM' for clock is selected
              if(clockAMPM){ //If PM
                voice.say(sp2_P);
                voice.say(sp2_M);
              }else{ //If AM
                voice.say(sp2_A);
                voice.say(sp2_M);
              }
            break;
            case(4): //If 'Enter' for 'Set Clock' is selected
              voice.say(sp2_ENTER);
            break;
          }
        break;
        case(3):
          //Only 3 pages on 'Control Unit Weight'
          if(pageB > numTimes + 1){
            pageB = 1;
          }

          if(pageB == 0){ //If 'Control Unit Weight' is selected
            voice.say(sp2_CONTROL);
            voice.say(sp2_UNIT);
            voice.say(sp2_EIGHT);
          }else if(pageB == numTimes + 1){ //If 'Enter' for 'Control Unit Weight' is selected
            voice.say(sp2_ENTER);
          }else{ //If 'Set Unit' is selected
            if(edit){ //If editing 'Set Unit'
              unitWeights[pageB - 1] += 4;
              if(unitWeights[pageB - 1] > 24){
                unitWeights[pageB - 1] = 0;
              }

              sayNum(unitWeights[pageB - 1]);
            }else{
              voice.say(sp2_SET);
              voice.say(sp2_UNIT);
              sayNum(pageB);
            }
          }
        break;
        case(4): //If 'Enter' for all settings is selected
          voice.say(sp2_ENTER);
        break;
        case(5): //If 'Clear All' is selected
          voice.say(sp3_CLEAR);
          voice.say(sp4_ALL);
        break;
      }
    }
  }
  if(digitalRead(buttonB) != oldButtonB){
    oldButtonB = digitalRead(buttonB);
  }
}