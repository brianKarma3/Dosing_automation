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

// Define E-stop pin (Need to be pulled up. )
#define e_stop_pin 3

// Start pin (Need to add pull down resistor)
#define start_button_pin 2

// 0 for manual, 1 for auto
#define manual_auto_pin 46

// Switch to select to dose one or two moulds at a time.  0 for single.
#define single_double_pin 48

// Switch to select Large or small treats
#define treat_size_selection 49


// Define limit switch pins
#define LS_load 26
#define LS_x_left 53
#define LS_x_right 52
#define LS_y_left 51
#define LS_y_right 50

#define LS_slider_left 17 
#define LS_slider_right 16


// Dosing control section
#define LC_Dout 24
#define LC_CLK 25

// Digital input 
#define Nozzle_signal_in 23

//Digital out
#define Dosing_control_pin  22


// Create a new instance of the AccelStepper class:
AccelStepper slider_stepper = AccelStepper(motorInterfaceType, slider_pulPin, slider_dirPin);
AccelStepper x1_stepper = AccelStepper(motorInterfaceType, x1_pulPin , x1_dirPin);
AccelStepper x2_stepper = AccelStepper(motorInterfaceType, x2_pulPin , x2_dirPin);
AccelStepper y_stepper = AccelStepper(motorInterfaceType, y_pulPin , y_dirPin);




// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(13,12, 11, 10, 9, 8 );
enum State {halt, manual_halt, auto_halt, manual_moving, initilisation,centering,  calibration, ready, dosing, recalibration}; 
State current_state, previous_state; 


enum auto_halt_options {initilisation_option, move_to_centre, calibration_option, ready_option}; 

auto_halt_options current_option, previous_option; 

// Set the desired speeds of stepper motors
  float slider_speed  = 100;
  float x_speed = 100; 
  float y_speed = 100; 



// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime_start = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTime_next = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

int start_button_state  = LOW; 
int last_start_button_state = LOW; 
bool start_flag = false; 

// Next button is the 
int next_button_state = LOW; 
int last_next_button_state = LOW; 
bool next_flag = false ; 


void setup() {
  // Pull up the Emergency stop pin
  pinMode(e_stop_pin, INPUT);
  digitalWrite(e_stop_pin, HIGH);  // Pull up estop pin


  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("Doser setting up!");
  
  

  // initialise current state to be halt; 
  current_state = halt; 


}





void loop() {
  // // set the cursor to column 0, line 1
  // // (note: line 1 is the second row, since counting begins with 0):
  // lcd.setCursor(0, 1);
  // // print the number of seconds since reset:
  // lcd.print(millis() / 1000);

  // The highest priority is to check the emergency stop pin at the starting of each cycle. 
      int e_stop_pressed = digitalRead(e_stop_pin); 

      if(e_stop_pressed == 1) // All states can transit to "Halt" state with e-stop press. 
      {
        current_state = halt; 
      }



       int start_reading = digitalRead(start_button_pin); 

        // ...............Debouncing logic for the start button.......
        // If the switch changed, due to noise or pressing:
        if (start_reading != last_start_button_state) {
          // reset the debouncing timer
          lastDebounceTime_start = millis();
        }

        if ((millis() - lastDebounceTime_start) > debounceDelay) {
          // whatever the reading is at, it's been there for longer than the debounce
          // delay, so take it as the actual current state:

          // if the button state has changed:
          if (start_reading != start_button_state) {
            start_button_state = start_reading;

            // only toggle the LED if the new button state is HIGH
            if (start_button_state == HIGH) {
              start_flag = true; 
            }
          }
        }

        // .......... end of the debouncing logic..... 


      switch(current_state)
      {
      case halt: 
        // Check the manual auto pin

        if(e_stop_pressed)
        {

          lcd.clear();
          lcd.setCursor(0,0);
      
          lcd.print("E STOP !!!");

          lcd.setCursor(0,1);
          lcd.print("E STOP !!!");
        }
        else
        {
          if(digitalRead(manual_auto_pin))
          {
            //Auto
            current_state = auto_halt;
            previous_state = halt; 
          }
          else
          {
            current_state = manual_halt; 
            previous_state =halt; 

            lcd.clear(); 
            lcd.setCursor(0,0);
            lcd.print("Manual mode");
          }
        }
        
        
        break;

      case manual_halt:
        
        if (current_state != previous_state)
        {
          lcd.clear(); 
          lcd.setCursor(0,0);
           lcd.print("Manual mode"); 
        }
        
        // For the slider:
        if(digitalRead(slider_control_left) ==1 && digitalRead(LS_slider_left)==0)
        {
          slider_stepper.setSpeed(slider_speed);  // Need to do experiment stepper direction
          slider_stepper.runSpeed(); 
        }
        else if(digitalRead(slider_control_right) ==1 && digitalRead(LS_slider_right)==0)
        {
          slider_stepper.setSpeed(-slider_speed); 
          slider_stepper.runSpeed(); 
        }


        // For the y-axis:
        if(digitalRead(y_control_left) ==1 && digitalRead(LS_y_left)==0)
        {
          y_stepper.setSpeed(y_speed);  // Need to do experiment stepper direction
          y_stepper.runSpeed(); 
        }
        else if(digitalRead(y_control_right) ==1 && digitalRead(LS_y_right)==0)
        {
          y_stepper.setSpeed(-y_speed); 
          y_stepper.runSpeed(); 
        }


        // For the x-axis:
        if(digitalRead(x_control_left) ==1 && digitalRead(LS_x_left)==0)
        {
          x1_stepper.setSpeed(x_speed);  // Need to do experiment stepper direction
          x1_stepper.runSpeed(); 
          x2_stepper.setSpeed(x_speed);  // Need to do experiment stepper direction
          x2_stepper.runSpeed(); 
        }
        else if(digitalRead(x_control_right) ==1 && digitalRead(LS_x_right)==0)
        {
          x1_stepper.setSpeed(-x_speed);  // Need to do experiment stepper direction
          x1_stepper.runSpeed(); 
          x2_stepper.setSpeed(-x_speed);  // Need to do experiment stepper direction
          x2_stepper.runSpeed(); 
        }





        previous_state = manual_halt; 
        
        // Manual_halt state can transit to auto halt state.
        if(digitalRead(manual_auto_pin)==1)
          current_state = auto_halt; 


        break;
      
      case auto_halt:

      // In this state user can choose to 
      // 1. Initializtion: move to (0,0) position
      // 2. Move to central position for doser calibration.
      // 3. Directly start dosor calibration cycle. 
      // 4. Skip to ready state.  
      // User use the x-axis direction toggle switch to select the next step. Press start button to select. 

      // Check if the previous state is autohalt
        if(previous_state != auto_halt)
        {
          current_option = auto_halt_options::initilisation_option;

          lcd.clear(); 
          lcd.setCursor(0,0);
          lcd.print("Press start");
          lcd.setCursor(0,1);
          lcd.print("To initialise");

          current_state = auto_halt;
          previous_state = auto_halt ; 
        }

        // Auto halt can be switched to manual halt mode by toggling the manual switch. 

        if(digitalRead(manual_auto_pin)==0)
        {
          // Swith to manual halt
          current_state = manual_halt ; 
        }
       

        int next_reading = digitalRead(y_control_right);           
        if (next_reading != last_next_button_state) {
          // reset the debouncing timer
          lastDebounceTime_next = millis();
        }

        if ((millis() - lastDebounceTime_next) > debounceDelay) {
          // whatever the reading is at, it's been there for longer than the debounce
          // delay, so take it as the actual current state:

          // if the button state has changed:
          if (next_reading != next_button_state) {
            next_button_state = next_reading;

            // only toggle the LED if the new button state is HIGH
            if (next_button_state == HIGH) {
              next_flag = true; 
            }
          }
        }


        // ............ End of debouncing logic .........

        switch (current_option)
        {
        case initilisation_option:
          if(current_option != previous_option)
          {
             // Display the option information on the LCD 
            lcd.setCursor(0,0);
            lcd.print("Press start");
            lcd.setCursor(0,1);
            lcd.print("To initialise");
            previous_option = auto_halt_options::initilisation_option; 

          }
          // Now wait for user to press the start button to enter initializtion state. 
          if(next_flag) // user use the y-axis toggle switch to trigger the next option.
          {
            current_option = move_to_centre; 
            previous_option = auto_halt_options::initilisation_option; 
            next_flag = false; 
          }

          if(start_flag)
          {
            current_state = State::initilisation;  
            previous_state = auto_halt; 
            start_flag = false; 
          }
          


          // If the 

          break;

        case auto_halt_options::move_to_centre:

          if(current_option != previous_option)
          {
            // Display the option information on the LCD
            lcd.setCursor(0,0);
            lcd.print("Press start to");
            lcd.setCursor(0,1);
            lcd.print("Centre platform");

            previous_option = auto_halt_options::move_to_centre; 
          }

          if(next_flag) // user use the y-axis toggle switch to trigger the next option.
          {
            current_option = auto_halt_options::calibration_option; 
            previous_option = auto_halt_options::move_to_centre; 
            next_flag = false; 
          }

          if(start_flag)
          {
            current_state = centering; 
            previous_state = State::auto_halt;
            start_flag = false; 

          }


        break;

        case calibration_option:
        if(current_option != previous_option)
          {
            // Display the option information on the LCD
            lcd.setCursor(0,0);
            lcd.print("Press start to");
            lcd.setCursor(0,1);
            lcd.print("calibrate:");

            previous_option = auto_halt_options::calibration_option; 
          }

           if(next_flag) // user use the y-axis toggle switch to trigger the next option.
          {
            current_option = auto_halt_options::ready_option; 
            previous_option = auto_halt_options::calibration_option; 
            next_flag = false; 
          }

          if(start_flag)
          {
            current_state = calibration; 
            previous_state = State::auto_halt;
            start_flag = false; 

          }

        break; 


        case ready_option:
        if(current_option != previous_option)
          {
            // Display the option information on the LCD
            lcd.setCursor(0,0);
            lcd.print("Press START to");
            lcd.setCursor(0,1);
            lcd.print("begin dosing");

            previous_option = auto_halt_options::ready_option; 
          }

           if(next_flag) // user use the y-axis toggle switch to trigger the next option.
          {
            current_option = auto_halt_options::initilisation_option; 
            next_flag = false; 
          }

          if(start_flag)
          {
            current_state = ready; 

            start_flag = false; 

          }

        break; 



        
        default:
          break;
        }



        previous_state =  auto_halt; 
      break; 


      case State::initilisation:

      break; 

      case State::centering:

      break; 

      case calibration:

      break; 

      case ready:

      break; 

      case dosing: 

      break; 

      case recalibration:

      break; 

      
      default:
        break;
      }

  // starting the state machine implementation

  



}
