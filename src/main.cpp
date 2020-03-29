/*
This program is designed to control the 2D platform to achieve automated dog treat
dosing.

*/
// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#include <AccelStepper.h>


#define slider_dirPin 28
#define slider_pulPin 27
#define slider_enaPin 29

#define x1_pulPin 30
#define x1_dirPin 31
#define x1_enaPin 32

#define x2_pulPin 33
#define x2_dirPin 34
#define x2_enaPin 35

#define y_pulPin 36
#define y_dirPin 37
#define y_enaPin 38

#define motorInterfaceType 1


#define slider_control_left 40
#define slider_control_right 41

#define x_control_left 42
#define x_control_right 43

#define y_control_left 44
#define y_control_right 45

// 0 for manual, 1 for auto
#define manual_auto_pin 46

// Switch to select to dose one or two moulds at a time.  0 for single.
#define single_double_pin 48







// Create a new instance of the AccelStepper class:
AccelStepper slider_stepper = AccelStepper(motorInterfaceType, slider_pulPin, slider_dirPin);

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("hello, world!");

}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 1);
  // print the number of seconds since reset:
  lcd.print(millis() / 1000);
}
