/*
* Created by Jason Godson
* V 1.00 Released March 21, 2015
*
* This is used to find an ideal location for a solar panel or solar heating setup and after leaving it for a day will give you a
* good indication of how many hours of sun you will get in a certain spot/direction. Also has other uses as the setpoint is adjustable.
* Could be used to see how long lights were on in a dark room, if there was too much light getting into a certain spot, or possibly
* use it as a start point for a project to turn on lights when it gets too dark in a room or to turn on pumps in the sun but not shade, etc.
*
* I referenced many different sources in order to learn how to do certain things. The only code directly copied was from the LCD
* example from the OSEPP Arduino BASICS starter kit. You can use any of this code in your project. I'm sure it's not coded
* the greatest or most efficent/secure, but this is my first project and it worked great after a weekend of working on it.
* 
*/

// Include the LCD library
#include <LiquidCrystal.h>

// LCD pins (Pins don't change and are declared as constants)
const int lcdRSPin = 12;
const int lcdEPin = 11;
const int lcdD4Pin = 5;
const int lcdD5Pin = 4;
const int lcdD6Pin = 3;
const int lcdD7Pin = 2;
const int lcdBright = 6; // This is an extra pin I used to control the LCD backlight  NOTE: PWM pin

// Initialize the library with the interface pins
LiquidCrystal lcd(lcdRSPin, lcdEPin, lcdD4Pin, lcdD5Pin, lcdD6Pin, lcdD7Pin);
                  
// Other pins (Pins don't change and are declared as constants)
const int sunSensePin = A0; // This is our photocell
const int buttonLPin = 9; // This is the left push button
const int buttonRPin = 8; // This is the right push button

// Decare variables  NOTE: I put most of them here, so I could change them in any function
int sunSenseRead = 0; // Max sensor value for photocell (by testing in full sun) is 1018 counts. Min value is 0.
int setpoint = 1000; // Minimum reading to count as time in sun  DEFAULT = 1000
long sampleRate = 250; // Sample rate of light sensor in miliseconds  DEFAULT = 250  NOTE: Must be a long as it is tested against a long
long timeout = 60000; // LCD timeout in miliseconds. DEFAULT = 60000  NOTE: Must be a long as it is tested against a long
long sunTotalTime = 0; // This is the total time in the sun  NOTE: I used a long as an int will not go high enough
long sunCurrentTime = 0; // Used for adding the difference between the current and last check to the total time
long sunLastTime = 0; // Used for adding the difference between the current and last check to the total time
int timer = 0; // Used to see if counting time towards sunTotalTime
long activityLastTime = 0; // Used for adding the difference between the current and last check to the total time
long activityCurrentTime = 0; // Used for adding the difference between the current and last check to the total time
long timeSinceActivity = 0; // Used to total how long since a button was pushed
long mainTimer = 0; // Used to see if it's time to check sensor readings and add to timer
long mainLastTime = 0; // Used for adding the difference between the current and last check to the total time
long mainCurrentTime = 0; // Used for adding the difference between the current and last check to the total time
int buttonLState = 1; // Used to store the state of the left push button while debouncing
int buttonRState = 1; // Used to store the state of the right push button while debouncing
int buttonState = 0; // Used to store the state of the pushed button 1. Left button 2. Right button 3. Both buttons
int calibrationMode = 0; // Used to stay in calibration mode until finished
int debounceLoop = 0; // Used to stay in debounce mode until positive of button push
int brightness = 255; // Used to set brightness of LCD  NOTE: 0-255 for PWM
int activity = 1; // Used to indicate that a button was pushed to turn LCD on and update
int hours = 0; // Hours of time above setpoint
int minutes = 0; // Minutes of time above setpoint
int seconds = 0; // Seconds of time above setpoint

void setup()
{
    lcd.begin(16, 2); // Set up the LCD with the number of columns and rows: 
    // Set buttons as input pins with internal pullup resistors
    pinMode(buttonLPin, INPUT_PULLUP);
    pinMode(buttonRPin, INPUT_PULLUP);
    analogWrite(lcdBright, brightness); // Set the LCD to initially be full brightness
}

void loop()
{
  updateMainTime(); // Update the main timer
  // Read sensor data if it's been long enough and reset timer
  if (mainTimer >= sampleRate)
  {
    mainTimer = 0;
    mainLastTime = millis();
    readData();
  }
  activity = checkButtonState(); // Check for a button push
  if (activity > 0)
  {
    // If a button was pushed, turn backlight on and reset activity timer
    timeSinceActivity = 0;
    activityLastTime = millis();
    brightness = 255; // Set LCD to full brightness
    analogWrite(lcdBright, brightness);
    displayData(); // Display data on LCD
    // Enter calibration mode if both buttons were pushed
    if (activity == 3)
    {
      calibrationMode = 1;
      calibration();
    }
  }
  else if (timeSinceActivity > timeout)
  {
    // If no buttons pushed lately, turn backlight off to save power and don't update LCD
    brightness = 0;
    analogWrite(lcdBright, brightness);
  }
  else
  {
    // If no activity but not long enough to turn LCD off, add to the no activity timer
    displayData();
    activityCurrentTime = millis();
    timeSinceActivity += activityCurrentTime - activityLastTime;
    activityLastTime = activityCurrentTime;
  }
}

void readData()
{
   // Read our photocell
    sunSenseRead = analogRead(sunSensePin);
    
    // Check if we need to start or stop counting time in the sun
    timeInSun();
}

void displayData()
{
    // Print our constant text on LCD
    lcd.setCursor(0, 0);
    lcd.print("L Level");
    lcd.setCursor(9, 0);
    lcd.print("Hours");
    
    // Print sensor value on LCD
    lcd.setCursor(0, 1);
    if (sunSenseRead >= 1015)
    {
      // Display max when close to sensor limit
      lcd.print("Max "); 
    }
    else
    {
      lcd.print(sunSenseRead);
      // Spaces make sure that if previous value had less characters than the current one, they don't stay on the LCD
      lcd.print("   ");
    }
    
    // Print length of time in light above setpoint on LCD
    convertTime(); // Convert miliseconds to hours, minutes, seconds
    if (hours >= 10)
    {
      // If two characters for hours, then move one left to make room
      lcd.setCursor(8, 1);
      lcd.print(hours);
    }
    else
    {
      lcd.setCursor(9, 1);
      lcd.print(hours);
    }
    lcd.print(":");
    if (minutes >= 10)
      {
        lcd.print(minutes);
      }
    else
    {
      // Add a 0 in front if there is single digit
      lcd.print("0");
      lcd.print(minutes);
    }
    lcd.print(" ");
    if (seconds >= 10)
      {
        lcd.print(seconds);
      }
    else
    {
      lcd.print("0");
      lcd.print(seconds);
    }
}

int checkButtonState()
{
  int returnVal = 0;
  debounceLoop = 1;
  // Loop until we get a good reading, then return which button was pressed
  while (debounceLoop > 0)
  {
    buttonLState = digitalRead(buttonLPin);
    buttonRState = digitalRead(buttonRPin);
    if (buttonLState < 1 && buttonRState < 1)
    {
       buttonLState = digitalRead(buttonLPin);
       buttonRState = digitalRead(buttonRPin);
       if (buttonLState < 1 && buttonRState < 1)
       {
         returnVal = 3; // Both buttons pushed
         debounceLoop = 0;
       }
     }
     else if (buttonLState < 1 && buttonRState > 0)
     {
       buttonLState = digitalRead(buttonLPin);
       buttonRState = digitalRead(buttonRPin);
       if (buttonLState < 1 && buttonRState > 0)
       {
         returnVal = 1; // Left button pushed
         debounceLoop = 0;
       }
     }
     else if (buttonLState > 0 && buttonRState < 1)
     {
       buttonLState = digitalRead(buttonLPin);
       buttonRState = digitalRead(buttonRPin);
       if (buttonLState > 0 && buttonRState < 1)
       {
         returnVal = 2; // Right button pushed
         debounceLoop = 0;
       }
     }
     else 
     {
       returnVal = 0; // Nothing pushed
       debounceLoop = 0;
     }
  }
  return returnVal;
}

void timeInSun()
{
  // Check to see if current sensor reading is higher than the setpoint that was set
  if (sunSenseRead >= setpoint)
  {
    if (timer == 1)
    {
      sunCurrentTime = millis(); // Get the current time
      sunTotalTime += sunCurrentTime - sunLastTime; // Add the time elapsed to total time
      sunLastTime = sunCurrentTime; // Set the current time as the last time
    }
    else
    {
      sunLastTime = millis(); // Update the last time the setpoint was checked
      timer = 1; // Turn on the timer
    }
  }
  else
  {
    if (timer == 1)
    {
      sunCurrentTime = millis(); // Get the current time
      sunTotalTime += sunCurrentTime - sunLastTime; // Add the time elapsed to total time
      sunLastTime = sunCurrentTime; // Set the current time as the last time
      timer = 0; // Turn off the timer
    }
    else
    {
      sunLastTime = millis(); // Update the last time the setpoint was checked
    }
  }
}

void convertTime()
{
  // Reset the hour, minute and second variables
  hours = 0;
  minutes = 0;
  seconds = 0;
  // We don't want to change the sunTotalTime variable, so we need a different variable
  double convertTime = sunTotalTime; 
  // Convert the number of miliseconds into hours, minutes and seconds
  while (convertTime > 1000)
  {
    while (convertTime >= 3600000)
    {
      hours++;
      convertTime -= 3600000; // 3,600,000 miliseconds in an hour
    }
    while (convertTime >= 60000)
    {
      minutes++;
      convertTime -= 60000; // 60,000 miliseconds in a minute
    }
    while (convertTime >= 1000)
    {
      seconds++;
      convertTime -= 1000; // 1,000 miliseconds in a second
    }
  }
}

void calibration()
{
  // Change cutoff for counting towards total time
  lcd.clear();
  lcd.print("Calibration Mode"); // Let the person know what's going on
  delay(750);
  lcd.clear();
  lcd.print("L: Down R: Up"); // Show instructions to adjust
  /* 
  * Loop until setpoint is changed, sensors are not read during this time, but time will still increase based on the last reading
  * If the light goes over/under the setpoint while in calibration it will not start/stop the timer, so whatever it was doing
  * previously is what it will count towards while in this mode.
  */
  while (calibrationMode > 0)
  {
    lcd.setCursor(0, 1);
    lcd.print("Setpoint: ");
    // Print setpoint on LCD, or Min/Max if limits reached
    if (setpoint < 5)
    {
      lcd.print("Min");
      setpoint += 5;
      delay(250);
    }
    else if (setpoint > 1015)
    {
      lcd.print("Max ");
      setpoint -= 5;
      delay(250);
    }
    else
    {
      lcd.print(setpoint);
      lcd.print("  "); // Spaces to overwrite any characters that might get left from previous value
    }
    buttonState = checkButtonState(); // Check what button is pushed, if any
    switch (buttonState)
    {
      case 1: // Left button pressed - setpoint down
        setpoint -= 5;
        delay(75);
        break;
      case 2: // Right button pressed - setpoint up
        setpoint += 5;
        delay(75);
        break;
      case 3: // Both buttons pressed - exit calibration
        calibrationMode = 0;
        lcd.clear();
        lcd.print("Setpoint is ");
        lcd.print(setpoint); // Show the setpoint when they exit calibration mode
        delay(1000);
        lcd.clear();
        activityLastTime = millis(); // Set the last time any buttons were pushed to now
        break;
    }
    delay(75);
  }
}

void updateMainTime()
{
  // Timer for when to read sensors, to avoid having a delay which can cause missed button presses
  mainCurrentTime = millis();
  mainTimer += mainCurrentTime - mainLastTime;
  mainLastTime = mainCurrentTime;
}
