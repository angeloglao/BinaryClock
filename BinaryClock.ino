/*
 * DOCUMENTATION:
 * 'BinaryClock', a program for the Arduino MEGA by Angelo Lao, Anshuman Banka and Caleb Choi
 * Last edited on: (11/19/17)
 *
 * OVERVIEW:
 * This program contains code that will run an LED binary clock, along with the regular time (and other info) on the LCD.
 * 
 * Using integer counter variables for seconds, minutes and hours, the program
 * will turn on the appropriate LEDs on the breadboard to indicate a value in a binary pattern
 * (where an LED which is ON represents a 1, and an LED that is OFF represents a 0).
 * 
 * For accuracy, the time displayed will be shown in 24-hour format (a.k.a., military time).
 * Seconds and minutes can be represented in binary using 6 bits each (i.e., 6 LEDs will be used to represent each).
 * Like on a clock, seconds and minutes will start at 0, and increase until 59, then reset to 0. Also,
 * hours will start at 0, and increase until 23, then reset to 0.
 * Hours can be represented in binary using 5 bits (i.e., 5 LEDs will be used to represent hours).
 * Along with the LEDs, an LCD will display the current time in regular base 10 and the current mode of the program
 *
 * To set the time, a the program pauses other functions and redirects to the set_time function.
 * The user can increment the value for a specific unit (hours, minutes, seconds) by either pressing or holding down
 * the function 1 button (the start/stop button). The user can also switch to another unit (s -> m -> h) by pressing the 
 * function 3 button (h/m/s button), and clear a specific unit using the function 2 button (clear button)
 * Milliseconds cannot be set, and is set to 0 when the time is changed
 *
 * The program also contains a stopwatch with the following functions: start/stop, and clear. Incrementing time on the
 * stopwatch works similarly to the global clock.
 * There is also a countdown timer, which initial time can be set similarly to the set time functionality on the global clock.
 * Upon start, it will count down until reaching zero, whereupon it will start blinking all of its LEDs (to catch attention)
 * If at any point the clear button is pressed while the stopwatch/timer is in operation, it will cancel it and set
 * it back to zero. 
 *
 * Note that each mode runs concurrently. So the clock, stopwatch, and timer can all run independently of one another.
 * 
 *
 * CHANGELOG:
 * (11/18/17): Program created.  Functionality includes printing the time to serial monitor and controlling the 'seconds' LEDs
 * (11/19/17): Control for the 'minutes' and 'hours' LEDs was added.
 * (11/19/17): Adujstment factor was added to the main loop, and timing mechanism has been switched to microseconds for more precision.
 *             Stopwatch has also been fully implemented.
 * (11/20/17): Added code for a timer, reformatted & commented more code for readability.
 *             Specifically, added underscores to function names and used camel case for variables.
 *             Renamed globalTime to timeOfDay, to avoid confusion with its global scope, and to provide clarity as to its purpose.
 * (11/21/17): Implemented setting time, LCD printing, and an absolute load of bugfixing
 * (11/22/17): More bugfixing, and building the clock box
 */


// Library needed for displaying stuff on the LCD
#include <LiquidCrystal.h>

// Variable that determines the rate at which the timer updates
// It must be a divisor of 1000. Preferably, it should be a multiple of 5 as most timing functions use multiples of 5 for intervals
const int TICKRATE = 8;

// A structure for time that will store values for hours, minutes, seconds, and milliseconds (each initialized with values of 0):
struct timeStruct
{
  int hours = 0;
  int minutes = 0;
  int seconds = 0;
  int milliseconds = 0;
}
// When combined with the above statement, these timeStruct structs are created (their scope is global).

/*
 * SUMMARY OF timeStruct STRUCTS:
 * timeOfDay: The actual time of day.  The program uses this to constantly calculate the time even when it is not in Clock mode.
 * stopWatchTime:  The time recorded when using the stopwatch.
 * timer: While the timer is running, the time in this struct decreases until all variables are 0.
 */
timeOfDay, stopWatchTime, timer;



/*
 * In 'setup', we set the outputs of LED pins using
 * a convention shown on the Arduino website:
 * (https://www.arduino.cc/en/Tutorial/AnalogWriteMega)
 * The 'lowestPin', and 'highestPin' variables are constant integers,
 * representing the pin numbers of the lowest pin and highest pin to be used respectively for the LEDs.
 * Other pins are initialized separately.
 */
 
const int lowestPin = 22;
const int highestPin = 38;

// The lowest value pin to be used for the 'seconds' LEDs is the same as 'lowestPin'.
// The lowest value pin to be used for the 'minutes' LEDs is 6 pins away from 'lowestPin', since 6 LEDs will be used to represent seconds.
// The lowest value pin to be used for the 'hours' LEDs is 6 pins away from 'lowestMin', since 6 LEDs will be used to represent minutes.
const int lowestSecPin = lowestPin;
const int lowestMinPin = lowestSecPin + 6;
const int lowestHourPin = lowestMinPin + 6;

// (TOGGLE) The pins to be used for toggling between 3 modes : time, stopwatch and timer
const int toggleButtonPin = 9;
int modeNumber = 0;           //Variable used for checking current mode
int toggleButtonRelease = 0;  // Keeps track of button press status

// (FUNC1) The pin to be used for start/stop functionality of stopwatch
const int f1ButtonPin = 10;
//Variable used for checking status of stopwatch button
int f1Toggle = 0;
int f1ButtonRelease = 0;

// (FUNC2) The pin used for clearing
const int f2ButtonPin = 7;
int f2ButtonRelease = 0;

// (FUNC3) The pin used for switching categories in the set time mode
const int f3ButtonPin = 8;
int f3ButtonRelease = 0;

// (SET) The pin used for switching to the setting time mode
const int setButtonPin = 6;
// Variables when setting time
int setButtonRelease = 0;      // Variable used to determine when the button is let go
int increment = 0;             // Counts milliseconds, and only increments after a certain number,
//  and ensures that the increments don't go up too fast
int segment = 0;               // Determines what segment is being incremented.
//  0 is seconds, 1 is minutes, 2 is hours
int isSettingTime = 0;         // Keeps track whether or not time is being set

// Variable used for checking status of stopwatch and timer button
int isStopwatchCounting = 0;
int isTimerCounting = 0;

// Variables used for connecting to the lcd
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);






// Setup for the program, and runs only once at the start
void setup() {

  // Initializes the lcd screen.
  lcd.begin(16, 2);

  // All output pins are set:
  for (int thisPin = lowestPin; thisPin <= highestPin; thisPin++) {
    pinMode(thisPin, OUTPUT);
  }

  pinMode(toggleButtonPin, INPUT);
  pinMode(f1ButtonPin, INPUT);
  pinMode(f2ButtonPin, INPUT);
  pinMode(f3ButtonPin, INPUT);
  pinMode(setButtonPin, INPUT);

  // Print out the LCD information
  lcd.setCursor(0, 0);
  lcd.print("TIME OF DAY:    ");

  // Output setting ends here.
  Serial.begin(9600);
}





// This function counts the time as it passes (we use this for the stopwatch and for the time of day).
// To reduce time error resulting from operation runtimes, milliseconds increments by 25, instead of 1.
void count_time(timeStruct &t)
{

  // Increment milliseconds
  t.milliseconds += TICKRATE;

  // These next if statements increment time and reset units if their limit is reached
  //(e.x., minutes increases every 60 seconds, and seconds is reset to 0):
  if (t.milliseconds == 1000) {
    t.seconds++;
    t.milliseconds = 0;
  }

  if (t.seconds == 60) {
    t.minutes++;
    t.seconds = 0;
  }

  if (t.minutes == 60) {
    t.hours++;
    t.minutes = 0;
  }

  t.hours %= 24;
}






//When the mode is 1, this function runs in loop().
void run_stopwatch()
{

  // Switches the stopwatch status upon button release (i.e., makes it start or stop counting).
  if (digitalRead(f1ButtonPin) == HIGH) f1ButtonRelease = 1;
  else if (digitalRead(f1ButtonPin) == LOW && f1ButtonRelease == 1)
  {
    if (isStopwatchCounting == 0) isStopwatchCounting = 1;
    else isStopwatchCounting = 0;
    f1ButtonRelease = 0;
  }

  // Clears the stopwatch if func2 is pressed
  if (digitalRead(f2ButtonPin) == HIGH) f2ButtonRelease = 1;
  else if (digitalRead(f2ButtonPin) == LOW && f2ButtonRelease == 1) {

    // Reset all stopwatch variables
    stopWatchTime.milliseconds = 0;
    stopWatchTime.seconds = 0;
    stopWatchTime.minutes = 0;
    stopWatchTime.hours = 0;
    isStopwatchCounting = 0;
    f2ButtonRelease = 0;
    led_display(stopWatchTime);
  }

  // Tells the stopwatch to count the time.
  if (isStopwatchCounting == 1)
  {
    count_time(stopWatchTime);
    led_display(stopWatchTime);
  }

  // Update the LCD display
  update_lcd(stopWatchTime);
}






//Function will be used to set the time on the clock.
//Can also be used for timer, which is why the timeStruct parameter has been added
void set_time(timeStruct &t) {

  // Set milliseconds to 0
  t.milliseconds = 0;

  // Increments the segment being incremented upon func3 button release
  if (digitalRead(f3ButtonPin) == HIGH) f3ButtonRelease = 1;
  else if (digitalRead(f3ButtonPin) == LOW && f3ButtonRelease == 1) {

    // Increments the segment that's being added to (seconds -> minutes -> hours)
    segment++;
    segment %= 3;

    // Reset button pin press tracker
    f3ButtonRelease = 0;
  }


  // Clears the current time unit if func2 is pressed
  if (digitalRead(f2ButtonPin) == HIGH) f2ButtonRelease = 1;
  else if (digitalRead(f2ButtonPin) == LOW && f2ButtonRelease == 1) {

    switch (segment) { // Filter by segment (seconds, minutes, hours)
      case 0: t.seconds = 0; break;
      case 1: t.minutes = 0; break;
      case 2: t.hours = 0; break;
    }

    // Reset button pin press tracker, and update LEDs and LCD
    f2ButtonRelease = 0;
    update_lcd(t);            // Update LEDs
    led_display(t);           // Update LCD
  }


  // Increments the millisecond tracker whenever the func1 button is held
  if (digitalRead(f1ButtonPin) == HIGH) {

    update_lcd(t);            // Update LEDs
    led_display(t);           // Update LCD
    f1ButtonRelease = 1;      // Keep track of the button not being released yet

    // Every ~150ms, increment the relevant segment
    // This is so that you can just hold the button and have it increment fairly quickly, and not have to press
    // the button every single time.
    if (increment % 152 == 0) {

      switch (segment) { // Filter by segment (seconds, minutes, hours)
        case 0: t.seconds++; break;   // seconds
        case 1: t.minutes++; break;   // minutes
        case 2: t.hours++; break;     // hours
      }

      // Check for overflows and carryovers
      if (t.seconds > 60) {
        t.seconds %= 60;
        t.minutes++;
      }
      if (t.minutes > 60) {
        t.minutes %= 60;
        t.hours++;
      }
      t.hours %= 24;
    }

    // Increment millisecond tracker, restrained within 1000
    increment += TICKRATE;
    increment %= 1000;

  } else if (digitalRead(f1ButtonPin) == LOW && f1ButtonRelease == 1) { // When func1 is released
    increment = 0;          // Reset increment
    f1ButtonRelease = 0;    // Reset button pin press tracker
  }

}






// a variable used for flashing the LEDs
int flash = 0;
// When in mode 2, the timer runs.
void run_timer()
{

  // Activates if you're not setting the time
  if (isSettingTime == 0) {
    
    // Switches the timer status upon button release (i.e., makes it start or stop counting).
    if (digitalRead(f1ButtonPin) == HIGH) f1ButtonRelease = 1;
    else if (digitalRead(f1ButtonPin) == LOW && f1ButtonRelease == 1)
    {
      if (isTimerCounting == 0) isTimerCounting = 1;
      else isTimerCounting = 0;
      f1ButtonRelease = 0;
    }

    // Clears the stopwatch if func2 is pressed
    if (digitalRead(f2ButtonPin) == HIGH) f2ButtonRelease = 1;
    else if (digitalRead(f2ButtonPin) == LOW && f2ButtonRelease == 1) {

      // Set all timer variables to zero
      timer.milliseconds = 0;
      timer.seconds = 0;
      timer.minutes = 0;
      timer.hours = 0;
      isTimerCounting = 0;
      f1ButtonRelease = 0;
    }
    
  }

  // If the user has activated the set time function, it'll go to this instead
  if (isSettingTime) set_time(timer);

  //If the timer finishes counting down...
  else if (timer.hours + timer.seconds + timer.milliseconds + timer.minutes == 0) {

    //Flash the LEDs on and off, and flash is used as a millisecond counter
    flash++;
    flash %= 80;

    // Will cycle every 400 ms
    if (flash == 0) {
      for (int thisPin = lowestPin; thisPin <= highestPin; thisPin++) {
        digitalWrite(thisPin, HIGH);
      }
    } else if (flash == 40) {
      for (int thisPin = lowestPin; thisPin <= highestPin; thisPin++) {
        digitalWrite(thisPin, LOW);
      }
    }

    // Update the LCD
    update_lcd(timer);
  }

  //If the timer has not finished counting down, count down.
  else if (isTimerCounting) {

    led_display(timer);
    timer.milliseconds -= TICKRATE - 2;

    if (timer.milliseconds < 0) {
      timer.seconds--;
      timer.milliseconds = 1000;
    }
    if (timer.seconds == -1) {
      timer.minutes--;
      timer.seconds = 59;
    }
    if (timer.minutes == -1) {
      timer.minutes--;
      timer.seconds = 59;
    }

    // Update the LCD
    update_lcd(timer);
  }

}






//This method displays the seconds, minutes and hours of a time struct onto the LEDs.
void led_display(timeStruct &t)
{
  // The code in this if statement runs only once per second.
  if (t.milliseconds == 0 || t.milliseconds == 1000)
  {

    // This for loop will use bitRead to look through each potential bit in the 'seconds' and 'minutes' counter,
    // and turn the corresponding LED on or off.  Seconds and minutes can be represented with 6 LEDs each, so
    // the for loop iterates 6 times.
    for (int i = 0; i < 6; i++)
    {
      digitalWrite(lowestSecPin + i, bitRead(t.seconds, i));
      digitalWrite(lowestMinPin + i, bitRead(t.minutes, i));
    }

    // This for loop functions the same way.  Hours can be represented with 5 LEDS, so
    // the for loop iterates 5 times.
    for (int i = 0; i < 5; i++)
    {
      digitalWrite(lowestHourPin + i, bitRead(t.hours, i));
    }
  }
}





// Updates the second line of the LCD with a timestamp
// For all time units, it will be right aligned and padded with zeros until format of 
//  hh:mm:ss:iiii is achieved
void update_lcd (timeStruct &t) {

  // Set cursor to the start of the second row
  lcd.setCursor(0, 1);

  // Prints out the hours
  if (t.hours < 10) lcd.print("0");
  lcd.print(t.hours);
  lcd.setCursor(2, 1);
  lcd.print(":");
  lcd.setCursor(3, 1);

  // Prints out the minutes
  if (t.minutes < 10) lcd.print("0");
  lcd.print(t.minutes);
  lcd.setCursor(5, 1);
  lcd.print(":");
  lcd.setCursor(6, 1);

  // Prints out the seconds
  if (t.seconds < 10) lcd.print("0");
  lcd.print(t.seconds);
  lcd.setCursor(8, 1);
  lcd.print(":");
  lcd.setCursor(9, 1);

  // Prints out the milliseconds
  if (t.milliseconds < 10) lcd.print("00");
  else if (t.milliseconds < 100) lcd.print("0");
  lcd.print(t.milliseconds);
}





// MAIN PROGRAM LOOP
// The main functions and timing of the function is done here
void loop() {

  // Records loop start time for adjustment
  unsigned long startTime = micros();

  // Switches the isSettingTime status upon button release
  if (digitalRead(setButtonPin) == HIGH) setButtonRelease = 1;
  else if (digitalRead(setButtonPin) == LOW && setButtonRelease == 1) {

    // If you aren't setting time already, enter set time mode and you're not in stopwatch mode
    if (isSettingTime == 0 && modeNumber != 1) {
      isSettingTime = 1; // Switch to time setting mode if not already in it

      // Clear the top line of the LCD and replace with message about setting time
      lcd.setCursor(0, 0);
      lcd.print("SETTING TIME    "); // Whitespace included to overwrite any leftover characters

      // Clear the timer if you're in timer mode
      if (modeNumber == 2) {
        for (int thisPin = lowestPin; thisPin <= highestPin; thisPin++) {
          digitalWrite(thisPin, LOW);
        }
        timer.milliseconds = 0;
        timer.seconds = 0;
        timer.minutes = 0;
        timer.hours = 0;
        isTimerCounting = 0;
      }

    }

    // When exiting setting mode...
    else {

      // Display the mode specific text onto the LCD
      lcd.setCursor(0, 0);
      switch (modeNumber) {
        case 0: lcd.print("TIME OF DAY:    ");
          break;
        case 1: lcd.print("STOPWATCH:      ");
          break;
        case 2: lcd.print("TIMER:          ");
          break;
      }

      // Resetting all variables
      isSettingTime = 0;
      increment = 0;
      segment = 0;
    }

    // Reset button pin press tracker
    setButtonRelease = 0;
  }

  //The countTime function is used to count the time as it passes.
  // If the toggle is at 0 (global clock mode) and time is being set, it won't increment the time
  // Otherwise, go into the set time function
  if (isSettingTime != 1 || modeNumber != 0) count_time(timeOfDay);  // Update the timeOfDay
  else set_time(timeOfDay);

  // Update the LCD display if not setting time and you're in global clock mode
  if (isSettingTime != 1  && modeNumber == 0) update_lcd(timeOfDay);

  // Changes the mode upon button release
  if (digitalRead(toggleButtonPin) == HIGH) toggleButtonRelease = 1;
  else if (digitalRead(toggleButtonPin) == LOW && toggleButtonRelease == 1) {

    modeNumber++;     // Increment mode
    modeNumber %= 3;  // Module 3, as there's only 3 modes

    // Print out the current mode in the first row of the LCD
    lcd.setCursor(0, 0);
    switch (modeNumber) {
      case 0: lcd.print("TIME OF DAY:    ");
        break;
      case 1: lcd.print("STOPWATCH:      ");
        break;
      case 2: lcd.print("TIMER:          ");
        break;
    }

    // Clears all the LEDs
    for (int thisPin = lowestPin; thisPin <= highestPin; thisPin++)
      digitalWrite(thisPin, LOW);

    // Automatically turns off setting time mode when switching to another mode
    isSettingTime = 0;

    // Reset button press tracking variable
    toggleButtonRelease = 0;

    //Special variables used in each function are reset:
    flash = 0;
  }

  // This switch is used to change modes based on the modeNumber variable.
  switch (modeNumber)
  {
    case 0: led_display(timeOfDay);
      break;
    case 1: run_stopwatch();
      break;
    case 2: run_timer();
      break;
  }

  // Records loop end time for adjustment
  unsigned long endTime = micros();

  // Adjusted delay, ensures that the loop runs at TICKRATE millisecond increments.
  // If the time it takes to run the code above is n microseconds, then the delay will be
  //  TICKRATE * 1000 - n, to ensure that the total time it takes to run through an iteration of the loop is
  //  TICKRATE * 1000 microseconds (which translates to TICKRATE milliseconds)
  if (endTime - startTime <= TICKRATE * 1000) delayMicroseconds((TICKRATE * 1000) - (endTime - startTime));
}
