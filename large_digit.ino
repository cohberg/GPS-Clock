#include <SoftwareSerial.h>
#include <Adafruit_GPS.h>
#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

SoftwareSerial mySerial(3, 2);

byte segmentClock = 6;
byte segmentLatch = 5;
byte segmentData = 7;

int ampm = 0; //am or pm to light up decimal point
int lux = 0; //light level in the room
char c;

// Set to false to display time in 12 hour format, or true to use 24 hour:
#define TIME_24_HOUR false

// Offset the hours from UTC (universal time) to your local time by changing
// this value.  The gps time will be in UTC so lookup the offset for your
// local time from a site like:
//   https://en.wikipedia.org/wiki/List_of_UTC_time_offsets
// This value, -7, will set the time to UTC-7 or Pacific Standard Time during
// daylight savings time.
double HOUR_OFFSET = -8;

Adafruit_GPS gps( & mySerial);

#define gpsECHO false

void setup() {

  pinMode(10, OUTPUT);
  Wire.begin();
  lightMeter.begin();

  pinMode(segmentClock, OUTPUT);
  pinMode(segmentData, OUTPUT);
  pinMode(segmentLatch, OUTPUT);

  digitalWrite(segmentClock, LOW);
  digitalWrite(segmentData, LOW);
  digitalWrite(segmentLatch, LOW);
  // Setup function runs once at startup to initialize the display and gps.

  // Setup Serial port to print debug output.
  Serial.begin(115200);

  // Setup the gps using a 9600 baud connection (the default for most gps modules).
  gps.begin(9600);

  // Configure gps to only output minimum data (location, time, fix).
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_10HZ);

  // Enable the interrupt to parse gps data.
  enablegpsInterrupt();
}

uint32_t timer = millis();

void loop() {
  // Loop function runs over and over again to implement the clock logic.

  // Check if gps has new data and parse it.
  if (gps.newNMEAreceived()) {
    gps.parse(gps.lastNMEA());
  }

  int DST = 0;

  // ********************* Calculate offset for Sunday ********************* 
  int y = gps.year; // DS3231 uses two digit year (required here)
  int x = (y + y / 4 + 2) % 7; // remainder will identify which day of month
  // is Sunday by subtracting x from the one
  // or two week window.  First two weeks for March
  // and first week for November
  // *********** Test DST: BEGINS on 2nd Sunday of March @ 2:00 AM ********* 
  if (gps.month == 3 && gps.day == (14 - x) && gps.hour >= 2) {
    DST = 1; // Daylight Savings Time is TRUE (add one hour)
  }
  if ((gps.month == 3 && gps.day > (14 - x)) || gps.month > 3) {
    DST = 1;
  }
  // ************* Test DST: ENDS on 1st Sunday of Nov @ 2:00 AM ************       
  if (gps.month == 11 && gps.day == (7 - x) && gps.hour >= 2) {
    DST = 0; // daylight savings time is FALSE (Standard time)
  }

  if ((gps.month == 11 && gps.day > (7 - x)) || gps.month > 11 || gps.month < 3) {
    DST = 0;
  }

  // Grab the current hours, minutes, seconds from the gps.
  int hours = gps.hour + HOUR_OFFSET + DST; // Add hour offset + DST to convert from UTC

  // to local time.
  // Handle when UTC + offset wraps around to a negative or > 23 value.
  if (hours < 0) {
    hours = 24 + hours;
  }
  if (hours > 23) {
    hours = 24 - hours;
  }
  int minutes = gps.minute;

  int seconds = gps.seconds;

  int fix = gps.fix;

  // Show the time on the display by turning it into a numeric
  // value, like 3:30 turns into 330, by multiplying the hour by
  // 100 and then adding the minutes.
  int displayValue = hours * 100 + minutes;

  // Do 24 hour to 12 hour format conversion when required.
  if (!TIME_24_HOUR) {
    // Handle when hours are past 12 by subtracting 12 hours (1200 value).
    if (hours > 12) {
      displayValue -= 1200;
      ampm = 0;
    }
    // Handle hour 0 (midnight) being shown as 12.
    else if (hours == 0) {
      displayValue += 1200;
      ampm = 1;
    }
  }

  // Now print the time value to the display.
  //Serial.println(seconds);

  if (timer > millis()) timer = millis();

  // approximately every 150 miliseconds or so, print out the current stats
  if (millis() - timer > 150) {
    timer = millis(); // reset the timer
    lux = lightMeter.readLightLevel();
    Serial.print("Light: ");
    Serial.println(lux);

    if (lux < 2) {
      digitalWrite(10, HIGH);
    } else {
      digitalWrite(10, LOW);
    }
    
    Serial.print("Time: ");
    Serial.print(gps.hour, DEC);
    Serial.print(':');
    Serial.println(gps.minute, DEC);

    Serial.print("Fix: ");
    Serial.print(fix);
    Serial.print(" | quality: ");
    Serial.print(gps.fixquality);
    Serial.print(" | satellites: ");
    Serial.println(gps.satellites);

    //blink the lights if no gps lock
    if (gps.year == 80) { 
      postNumber(' ', true);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', true);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', true);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', true);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      delay(150);
      postNumber(' ', false);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', false);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', false);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      postNumber(' ', false);
      digitalWrite(segmentLatch, LOW);
      digitalWrite(segmentLatch, HIGH);
      delay(150);

    } else {
      showNumber(displayValue);
    }
  }
}

SIGNAL(TIMER0_COMPA_vect) {
  char c = gps.read();
  // if you want to debug, this is a good time to do it!
  #
  ifdef UDR0
  if (gpsECHO)
    if (c) UDR0 = c;
    // writing direct to UDR0 is much much faster than Serial.print
    // but only one character can be written at a time.
    #
  endif
}

void enablegpsInterrupt() {
  // Function to enable the timer interrupt that will parse gps data.
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function above
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}

//Takes a number and displays 2 numbers. Displays absolute value (no negatives)
void showNumber(float value) {
  int number = abs(value); //Remove negative signs and any decimals

  //Serial.print("number: ");
  //Serial.println(number);
  if (number < 1000) {
    for (byte x = 0; x < 4; x++) {
      int remainder = number % 10;

      if (lux < 2) {
        if (x == 0) {
          postNumber(remainder, false);
        } else if (x == 1) {
          postNumber(remainder, false);
        } else if (x == 2) {
          postNumber(remainder, false);
        } else if (x == 3) {
          postNumber(' ', false);
        } else {
          postNumber(remainder, false);
        }
      } else {
        if (x == 0) {
          if (ampm == 0) {
            postNumber(remainder, true);
          } else {
            postNumber(remainder, false);
          }
        } else if (x == 1) {
          if (ampm == 1) {
            postNumber(remainder, true);
          } else {
            postNumber(remainder, false);
          }
        } else if (x == 2) {
          postNumber(remainder, true);
        } else if (x == 3) {
          postNumber(' ', true);
        } else {
          postNumber(remainder, false);
        }
      }
      number /= 10;
    }

    //Latch the current segment data
    digitalWrite(segmentLatch, LOW);
    digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
  } else {
    for (byte x = 0; x < 4; x++) {
      int remainder = number % 10;

      if (lux < 2) {
        if (x == 0) {
          postNumber(remainder, false);
        } else if (x == 1) {
          postNumber(remainder, false);
        } else if (x == 2) {
          postNumber(remainder, false);
        } else if (x == 3) {
          postNumber(remainder, false);
        } else {
          postNumber(remainder, false);
        }
      } else {
        if (x == 0) {
          if (ampm == 0) {
            postNumber(remainder, true);
          } else {
            postNumber(remainder, false);
          }
        } else if (x == 1) {
          if (ampm == 1) {
            postNumber(remainder, true);
          } else {
            postNumber(remainder, false);
          }
        } else if (x == 2) {
          postNumber(remainder, true);
        } else if (x == 3) {
          postNumber(remainder, true);
        } else {
          postNumber(remainder, false);
        }
      }
      number /= 10;
    }

    //Latch the current segment data
    digitalWrite(segmentLatch, LOW);
    digitalWrite(segmentLatch, HIGH); //Register moves storage register on the rising edge of RCK
  }
}

//Given a number, or '-', shifts it out to the display
void postNumber(byte number, boolean decimal) {
  //    -  A
  //   / / F/B
  //    -  G
  //   / / E/C
  //    -. D/DP

  #
  define a 1 << 0# define b 1 << 6# define c 1 << 5# define d 1 << 4# define e 1 << 3# define f 1 << 1# define g 1 << 2# define dp 1 << 7

  byte segments;

  switch (number) {
  case 1:
    segments = b | c;
    break;
  case 2:
    segments = a | b | d | e | g;
    break;
  case 3:
    segments = a | b | c | d | g;
    break;
  case 4:
    segments = f | g | b | c;
    break;
  case 5:
    segments = a | f | g | c | d;
    break;
  case 6:
    segments = a | f | g | e | c | d;
    break;
  case 7:
    segments = a | b | c;
    break;
  case 8:
    segments = a | b | c | d | e | f | g;
    break;
  case 9:
    segments = a | b | c | d | f | g;
    break;
  case 0:
    segments = a | b | c | d | e | f;
    break;
  case ' ':
    segments = 0;
    break;
  case 'c':
    segments = g | e | d;
    break;
  case '-':
    segments = g;
    break;
  }

  if (decimal) segments |= dp;

  //Clock these bits out to the drivers
  for (byte x = 0; x < 8; x++) {
    digitalWrite(segmentClock, LOW);
    digitalWrite(segmentData, segments & 1 << (7 - x));
    digitalWrite(segmentClock, HIGH); //Data transfers to the register on the rising edge of SRCK
  }
}
