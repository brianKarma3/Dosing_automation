/*
This program is designed to control the 2D platform to achieve automated dog treat
dosing.

// 1000 steps for 200mm appox. 
^ Y
|
|
|
|_________> x

|----------
|
| Doser
|
|
|----------
*/
// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#include <AccelStepper.h>
#include <HX711.h>
#include <stdlib.h> 
#include <string.h>






#define LC_DOUT_Pin 24
#define LC_CLK_pin 25



#define slider_dirPin 28
#define slider_pulPin 27
#define slider_enaPin 29

#define x1_pulPin 30
#define x1_dirPin 31
#define x1_enaPin 32

#define x2_pulPin 33
#define x2_dirPin 34
#define x2_enaPin 35

#define y_pulPin 38
#define y_dirPin 37
#define y_enaPin 36

#define motorInterfaceType 1


#define slider_control_left 40
#define slider_control_right 41

#define x_control_left 42
#define x_control_right 43

#define y_control_down 44
#define y_control_up 45 

// Define E-stop pin (Need to be pulled up. )
#define e_stop_pin 2

// Start pin (Need to add pull down resistor)
#define start_button_pin 3

// 0 for manual, 1 for auto
#define manual_auto_pin 46

// 1 for full sized mould. 

#define mould_size_pin 47   

// Switch to select to dose one or two moulds at a time.  0 for single.
#define single_double_pin 48

// Switch to select Large or small treats
#define treat_size_selection 49  // 1 for small sized treat and 0 for large sized treats. 


// Define limit switch pins
#define LS_load 26
#define LS_x_left 53
#define LS_x_right 52
#define LS_y_bottom 51
#define LS_y_top 50

#define LS_slider_left 17 
#define LS_slider_right 16


// Digital input 
#define Nozzle_signal_in 23

//Digital out
#define Dosing_control_pin  22

void x_moving_left(); 
void x_moving_right(); 
void y_moving_down(); 
void y_moving_up(); 
void slider_moving_left(); 
void slider_moving_right(); 
void dose_once(); 
void checking_start_button_toggle();
bool checking_dosing_completion(); 

void Load_cell_tare(); 
float Load_cell_read(); 

int Calc_slider_position(int weight_difference); 

// Create a new instance of the AccelStepper class:
AccelStepper slider_stepper = AccelStepper(motorInterfaceType, slider_pulPin, slider_dirPin);
AccelStepper x1_stepper = AccelStepper(motorInterfaceType, x1_pulPin , x1_dirPin);
AccelStepper x2_stepper = AccelStepper(motorInterfaceType, x2_pulPin , x2_dirPin);
AccelStepper y_stepper = AccelStepper(motorInterfaceType, y_pulPin , y_dirPin);

 
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(13,12, 11, 10, 9, 8 );
enum State {halt, manual_halt, auto_halt, initiliasation,centering,  calibration, ready, dosing, recalibration}; 
State current_state, previous_state, state_before_recalib; 


enum auto_halt_options {initilisation_option, move_to_centre, calibration_option, ready_option}; 

auto_halt_options current_option, previous_option; 

// Set the desired speeds of stepper motors
float slider_speed  = 400;
float x_speed = 400;
float y_speed = 400; 

// Centre position absolute position

float centre_x_position = -1510; 
float centre_y_position = 600; 

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime_start = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTime_next = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTime_nozzle = 0; 
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
unsigned long debounceDelay_nozzle = 2; 

bool manual_weight_display_flag = false; 

int start_button_state  = LOW; 
int last_start_button_state = LOW; 
int last_nozzle_state = HIGH; 
bool start_flag = false; 

// Next button is the 
int next_button_state = LOW; 
int last_next_button_state = LOW; 
bool next_flag = false ; 


// Flags for initialisation state. 
bool x_done_flag = false; 

bool y_done_flag = false; 
bool slider_done_flag = false; 

// Calibration constants and flags
int calib_count =0; 
int calib_times = 20;   // Should be 40, using 10 for dev

int tuning_count = 0; 
int tuning_times = 4; 

bool nozzle_signal_flag = true;  // Flag indicates that there is a falling edge of nozzle valve signal.
                                  // It means the dosing has just been completed. 
int nozzle_valve_state = 0; 

HX711 scale;


// float calibration_factor = -948.76; //Calibrated for the 2kg load cell

float calibration_factor = -720.8; //Calibrated for the 3kg load cell

long zero_factor; 


float desired_weight_small_treat = 6;// in grams
float desired_weight_large_treat = 9; 
float weight_diff; 

bool treat_size_is_small = true; // True for small treat and false for large treat. 
bool full_mould = true; // True for a full mould; false for a 3/4 mould. 

int dosing_times = 0; // Total number of times to dose in a dosing run. 
int dosing_count = 0; // Used to record ho

// Large treat full size array original set up
// int Full_large_x[] =  {-550, -950, -1350, -1750, -2150, -2150 , -1750 , -1350, -950, -550, -550, -950, -1350, -1750, -2150, -2150 , -1750 , -1350, -950, -550} ;
// int Full_large_y[]= {0, 0, 0, 0, 0, 375, 375, 375, 375, 375, 750, 750,750,750,750, 1125,  1125, 1125, 1125, 1125};  

int Full_large_x[] =  {-190, -190, -190, -190, -190, -570 , -570, -570, -570, -570, -950, -950, -950, -950, -950, -1340 , -1340 , -1340, -1340, -1340} ;
int Full_large_y[]= {400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400, 400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400};  


int Full_small_x[] = {-190, -190, -190, -190, -190, -190,  -570 , -570, -570, -570, -570, -570 -950, -950, -950, -950, -950, -950,  -1340 , -1340 , -1340, -1340, -1340, -1340};
int Full_small_y[] = {0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 , 0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 }; 

int Two_large_x[] = {-190, -190, -190, -190, -190, -570 , -570, -570, -570, -570, -950, -950, -950, -950, -950, -1340 , -1340 , -1340, -1340, -1340, -1720, -1720, -1720, -1720, -1720, -2100, -2100, -2100, -2100, -2100, -2480, -2480, -2480, -2480, -2480,  -2760, -2760, -2760, -2760, -2760} ;
int Two_large_y[] = {400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400, 400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400, 400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400, 400, 775, 1150, 1525, 1900, 1900, 1525, 1150, 775, 400};  


int Two_small_x[] = {-190, -190, -190, -190, -190, -190,-570 , -570, -570, -570, -570, -570, -950, -950, -950, -950, -950, -950,  -1340 , -1340 , -1340, -1340, -1340, -1340, -1720, -1720, -1720, -1720, -1720, -1720,  -2100, -2100, -2100, -2100, -2100, -2100,  -2480, -2480, -2480, -2480, -2480, -2480, -2760, -2760, -2760, -2760, -2760, -2760};
int Two_small_y[] = {0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 , 0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 , 0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 , 0, 325, 650, 975, 1300, 1625 , 1625, 1300, 975, 650, 325, 0 }; 
int* x_sequence; 
int* y_sequence; 
bool to_next_postion; 

// float desired_weight_small = 9.5;
// float desired_weight_large = 12.5; 
float treat_weight_diff = 0; 

// For calibration during dosing. 
int calibration_size =4; 
int calibration_count = 0 ; 

float weight_n = 0 ; 
float weight_n_1= 0; 

float desired_treat_weight; 


void setup() {

   Serial.begin(115200);
  // Pull up the Emergency stop pin
  pinMode(e_stop_pin, INPUT);
  digitalWrite(e_stop_pin, HIGH);  // Pull up estop pin

  // Set dosing control pin to output
  pinMode(Dosing_control_pin, OUTPUT); 
  digitalWrite(Dosing_control_pin, LOW);  // Set to low.

  pinMode(Nozzle_signal_in, INPUT); 

  digitalWrite(Nozzle_signal_in, HIGH);  

  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.setCursor(0,0);
  lcd.print("Doser setting up!");

  

  // initialise current state to be halt; 
  current_state = halt; 




  slider_stepper.setMaxSpeed(400);
  slider_stepper.setAcceleration(500);


  y_stepper.setMaxSpeed(1700);
  y_stepper.setAcceleration(4800);

  x1_stepper.setMaxSpeed(1800);
  x1_stepper.setAcceleration(4000);
  x2_stepper.setMaxSpeed(800);
  x2_stepper.setAcceleration(2800);


  scale.begin(LC_DOUT_Pin,  LC_CLK_pin);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  zero_factor = scale.read_average(); //Get a baseline reading

  Serial.println("Set up completed. ");


  // // Stepper motor testing
  // slider_stepper.setCurrentPosition(0); 
  // slider_stepper.move(20); 

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



      


      switch(current_state)
      {
      case halt: 
        // Check the manual auto pin

        if(e_stop_pressed)
        {

          lcd.begin(16, 2); 
          lcd.setCursor(0,0);
      
          lcd.print("E STOP !!!");

          lcd.setCursor(0,1);
          lcd.print("E STOP !!!");

          digitalWrite(Dosing_control_pin, LOW); 
          
          // Serial.println("Stop button detected!"); 
          
        }
        else
        {
          if(!digitalRead(manual_auto_pin))
          {
            //Auto
            current_state = manual_halt; 
            previous_state =halt; 
            lcd.begin(16, 2); 
            lcd.setCursor(0,0);
            lcd.print("Manual mode");
          }
          else
          {
            // If the E-stop is not pressed, back to the previous state. 
            current_state = previous_state;
            
         
          }
        }
        
        
        break;

      case manual_halt:
        
        

        if (current_state != previous_state)
        {
          lcd.begin(16, 2); 
          lcd.setCursor(0,0);
           lcd.print("Manual mode"); 
           manual_weight_display_flag =false; 
           x_speed = 400; 
           y_speed = 400; 
        }
        
        // For the slider:
        if(digitalRead(slider_control_left) ==1 && digitalRead(LS_slider_left)==0)
        {
          // slider_stepper.setSpeed(slider_speed);  // Need to do experiment stepper direction
          // slider_stepper.runSpeed(); 
          //slider_stepper.move(1000); 
          slider_moving_left(); 

           
        }
        else if(digitalRead(slider_control_right) ==1 && digitalRead(LS_slider_right)==0)
        {
          // slider_stepper.setSpeed(-slider_speed); 
          // slider_stepper.runSpeed(); 
          //slider_stepper.move(-1000); 
          // slider_stepper.setSpeed(-300); 
          // slider_stepper.runSpeed(); 

          slider_moving_right();

          
        }
        else
        {
          //slider_stepper.stop(); 
          // For the x-axis:
          if(digitalRead(x_control_left) ==1 && digitalRead(LS_x_left)==0)
          {
            x_moving_left(); 
          }
          else if(digitalRead(x_control_right) ==1 && digitalRead(LS_x_right)==0)
          {
            x_moving_right(); 
          }
          else 
          {
            // For the y-axis:
            if(digitalRead(y_control_down) ==1 && digitalRead(LS_y_bottom)==0)
            {
              y_moving_down();
            }
            else if(digitalRead(y_control_up) ==1 && digitalRead(LS_y_top)==0)
            {
              y_moving_up(); 
            }
            else 
            {
              checking_start_button_toggle(); 
              if(start_flag)
              {
                Load_cell_tare(); 
                manual_weight_display_flag =true; 

              }
              if(manual_weight_display_flag)
              {
                float lc_reading = Load_cell_read(); 
                // Display to the lcd. 

                lcd.clear(); 
                delay(2); 

                lcd.setCursor(0,0);

                lcd.print(lc_reading); 
                lcd.println(" g");



              }
            }
          }



        previous_state = manual_halt; 
        
        // Manual_halt state can transit to auto halt state.
        if(digitalRead(manual_auto_pin)==1)
          current_state = auto_halt; 


        break;
      
      case auto_halt:
      {

        // In this state user can choose to 
        // 1. Initializtion: move to (0,0) position
        // 2. Move to central position for doser calibration.
        // 3. Directly start dosor calibration cycle. 
        // 4. Skip to ready state.  
        // User use the x-axis direction toggle switch to select the next step. Press start button to select. 


        // Check if the start button is pressed
        checking_start_button_toggle(); 

        // Check if the previous state is autohalt
        if(previous_state != auto_halt)
        {
          current_option = auto_halt_options::initilisation_option;

          lcd.begin(16, 2);  
          lcd.setCursor(0,0);
          lcd.print("Press start");
          lcd.setCursor(0,1);
          lcd.print("To initialise");

          Serial.println("Auto halt debug 1. ");

          current_state = auto_halt;
          previous_state = auto_halt ; 
        }

        // Auto halt can be switched to manual halt mode by toggling the manual switch. 

        if(digitalRead(manual_auto_pin)==0)
        {
          // Swith to manual halt
          current_state = manual_halt ; 
        }
       

        int next_reading = digitalRead(x_control_right);           
        if (next_reading != last_next_button_state) {
          // reset the debouncing timer
          lastDebounceTime_next = millis();

          Serial.println("Next detected;");
          Serial.println(lastDebounceTime_next); 
        }

        if ((millis() - lastDebounceTime_next) > debounceDelay) {
          // whatever the reading is at, it's been there for longer than the debounce
          // delay, so take it as the actual current state:

          // if the button state has changed:
          if (next_reading != next_button_state) {
            next_button_state = next_reading;

            Serial.println("Next button debug"); 

            // only toggle the LED if the new button state is HIGH
            if (next_button_state == HIGH) {
              next_flag = true; 
            }
          }
          
        }
        last_next_button_state = next_reading ; 

        // ............ End of debouncing logic .........

        switch (current_option)
        {
          case initilisation_option:
          {
            if(current_option != previous_option)
            {
              // Display the option information on the LCD 
              lcd.begin(16, 2); 
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
              Serial.println("Going to init!"); 
              current_state = State::initiliasation;  
              previous_state = auto_halt; 
              start_flag = false; 
            }
            


            // If the 

            break;
          }

          case auto_halt_options::move_to_centre:{

            if(current_option != previous_option)
            {
              // Display the option information on the LCD
              
              lcd.setCursor(0,0);
              lcd.print("Press start to   ");
              lcd.setCursor(0,1);
              lcd.print("Centre platform   ");

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
          }

          case calibration_option:
          {
            if(current_option != previous_option)
            {
              // Display the option information on the LCD
              
              lcd.setCursor(0,0);
              lcd.print("Press start to   ");
              lcd.setCursor(0,1);
              lcd.print("calibrate:  ");

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
          }


          case ready_option:
          {
          
            if(current_option != previous_option)
            {
              // Display the option information on the LCD
              lcd.begin(16, 2); 
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
          }
          default:
            


          break;
        }



        previous_state =  auto_halt; 
        break; 

      }


      case initiliasation:
      {
        // This function moves the platform towards the bottom left corner.  
        // left on the x-axis and down on the y-axis

        // Move at constant speed and check the limit switch. If x-left limit switch is on. Set initialise_x_flag to true. 
        if(previous_state !=current_state)
        { 
          previous_state = initiliasation; 
          
          lcd.setCursor(0,0);
          lcd.print("Moving platform ");
          lcd.setCursor(0,1);
          lcd.print("to Bottom-left ");
          
          
          x_done_flag = false; 
          y_done_flag = false; 
        }
        else 
        {
          // Check if the limit x-left and y-bottom limit switch has been pressed. 
          Serial.println("initialisation loop starts"); 


          if(digitalRead(LS_x_left))
          {
            x_done_flag = true; 
          }        
          if (digitalRead(LS_y_bottom))
          {y_done_flag = true; }

          if(!x_done_flag)
          {
            x_moving_left(); 
          
          }
          if(!y_done_flag)
          {
            y_moving_down(); 
          }

          // Both flags are triggered, the platform is regarded at the origin. 
          if(x_done_flag&&y_done_flag)
          {
            lcd.begin(16, 2); 
            lcd.setCursor(0,0);
            lcd.print("Init Done       ");
            lcd.setCursor(0,1);
            lcd.print("Press start for next step.");

            checking_start_button_toggle(); 

            x1_stepper.setCurrentPosition(0); 
            x2_stepper.setCurrentPosition(0); 
            y_stepper.setCurrentPosition(0); 

            if(start_flag)
            {
              current_state = centering ; 
              start_flag = false; 
            }
          }


        }
        

        break; 
      }

      case State::centering:
      {

        if(previous_state !=current_state)
        {
          previous_state = centering; 
          

          lcd.setCursor(0,0);
          lcd.print("Moving to centre    ");
          lcd.setCursor(0,1);
          lcd.print("               "); 
          
          // Set the target position for the centre. 
          x1_stepper.moveTo(centre_x_position); 
          x2_stepper.moveTo(centre_x_position);
          y_stepper.moveTo(centre_y_position); 
          // x_done_flag = false; 
          // y_done_flag = false; 
        }

        // Check if the centre is reached in X and Y axis. 

        if(x1_stepper.currentPosition() == centre_x_position && y_stepper.currentPosition() == centre_y_position)
        {
           // Centre position is reached. 
          lcd.begin(16, 2); 
          lcd.setCursor(0,0);
          lcd.print("Press start to");
          lcd.setCursor(0,1);
          lcd.print("Calibrate.       ");

          checking_start_button_toggle(); 

          if(start_flag)
          {
            current_state = calibration ;
            previous_state = centering;  
            start_flag = false; 

          }


        }
        else
        {
          x1_stepper.run(); 
          y_stepper.run(); 

          //Debug 
          Serial.print("X_position:");
          Serial.println(x1_stepper.currentPosition()); 


        } 
        
      break; 
      }

      case calibration:
        if(previous_state !=current_state)
        {
          
          
          lcd.setCursor(0,0);
          lcd.print("Calibrating     ");
          lcd.setCursor(0,1);
          lcd.print("the doser     "); 
          if(previous_state != recalibration)
          {
            calib_count = 0; 
            tuning_count= 0 ;
          }
          else if (previous_state == halt)
          {
            digitalWrite(Dosing_control_pin, HIGH); 
            Serial.println(calib_count);
            Serial.println(tuning_count); 

          }
          else{
            calib_count = 0; 
            tuning_count =0; 
          }
          
          

          previous_state = calibration; 
          // x_done_flag = false; 
          // y_done_flag = false; 
        }


        // Only dose if there is an falling edge of doser's nozzle signal.  We should turn on and count falling edges. 
        if(calib_count<calib_times )
        {
          // turn on dosing until the the required dosing times is reached. 
          digitalWrite(Dosing_control_pin, HIGH); 
          
          
          checking_dosing_completion(); 
          if(nozzle_signal_flag)
          {
            calib_count++; 
            nozzle_signal_flag = false; 

            Serial.println("Dosing completing flag detected!"); 
          }
          
          if(calib_count == calib_times)
          {
            digitalWrite(Dosing_control_pin, LOW); 
          }
          
        }
        else if(calib_count >= calib_times)
        {
          bool weighing_flag = false; 
          // Dosing 4 times and then adjust the slider, check the weight and then adjust the slider. 
          if(tuning_count ==0)
          {
            // Tare the scale. 
            Load_cell_tare(); 

            // Turn on the doser
            digitalWrite(Dosing_control_pin, HIGH); 

            tuning_count ++; 

          }
          if(tuning_count<= tuning_times)
          {
            // Turning check the falling edge 
            checking_dosing_completion(); 
            if(nozzle_signal_flag)
            {
            tuning_count++; 
            nozzle_signal_flag = false; 

            Serial.println("Dosing completing flag detected!"); 
            }
          

            if(tuning_count > tuning_times) // Tuning time is reached. 
            {
              weighing_flag = true;
              digitalWrite(Dosing_control_pin, LOW); // Turn off the doser. 

            }

          }

          if(weighing_flag)
          {
            // After dosing pause for 1s.
            delay(1000); 
            tuning_count =0; 
            weighing_flag = false; 
            // weigh the dosed weight. 
            float avg_treat_weight = Load_cell_read()/(float)tuning_times; 
            // For debugging
            Serial.println(avg_treat_weight); 

            // Check the difference between the dosing weight and the desired weight. 

            if(digitalRead(treat_size_selection) == 1) 
            {
              treat_size_is_small = false; 
              weight_diff = -avg_treat_weight + desired_weight_large_treat; 

            }
            else 
            {
              treat_size_is_small = true; 
              weight_diff = -avg_treat_weight + desired_weight_small_treat; 
            }
            if(abs(weight_diff)>0.3)
            {

             current_state = calibration;  
             slider_stepper.move(Calc_slider_position(weight_diff));
             slider_stepper.runToPosition(); 

             
            }
            else 
            {
              current_state = auto_halt; 

              
            }
                      
          }


        }
        
        


      break; 

      case ready:

        if(previous_state !=current_state)
        { 
          
          if(previous_state == dosing || previous_state ==recalibration)
          {
            lcd.begin(16, 2); 
            lcd.setCursor(0,0);
            
            
            lcd.print(Load_cell_read()); 
          }
          else{

          
            
            lcd.setCursor(0,0);
            lcd.print("Moving platform        ");
            lcd.setCursor(0,1);
            lcd.print("to Bottom-left      ");
          }
          previous_state = ready; 
          x_done_flag = false; 
          y_done_flag = false; 
          slider_done_flag = false; 

          // Faster movement to the original position than in manaul mode. 
          x_speed = 1000;
          y_speed = 1000; 
        }
        else 
        {
          // Check if the limit x-left and y-bottom limit switch has been pressed. 

          if(digitalRead(LS_x_left))
          {
            x_done_flag = true; 
          }        
          if (digitalRead(LS_y_bottom))
          {y_done_flag = true; }
          if(digitalRead(LS_slider_left))
          {
            slider_done_flag = true; 
          }

          if(!x_done_flag)
          {
            x_moving_left(); 
          
          }
          if(!y_done_flag)
          {
            y_moving_down(); 
          }
          if(!slider_done_flag)
          {
            
          }

          // Both flags are triggered, the platform is regarded at the origin. 
          if(x_done_flag&&y_done_flag)
          {
            lcd.setCursor(0,1);
            lcd.print("Ready to dose. ");
           

            checking_start_button_toggle(); 

            x1_stepper.setCurrentPosition(0); 
            x2_stepper.setCurrentPosition(0); 
            y_stepper.setCurrentPosition(0); 

            if(start_flag)
            {
              current_state = dosing; 
              start_flag = false; 
            }
          }


        }

        // Move to initiliasing position. 

        // Wait for the user to press the start button. 

      break; 

      case dosing: 
      {
        // Move the doser in prefined pattern. 

        if(previous_state !=current_state)
        { 
          previous_state = dosing; 

          lcd.begin(16, 2); 
          lcd.setCursor(0,0);
          lcd.print("Dosing starts");
          lcd.setCursor(0,1);
          nozzle_signal_flag = false; 

          slider_stepper.setCurrentPosition(0); 
          slider_stepper.move(0); 


          // Check the treat size and mould size selection. 
          desired_treat_weight = (digitalRead(treat_size_selection)==1? desired_weight_large_treat: desired_weight_small_treat); 

          // Check if double mould pin are selected.  

          if(digitalRead(single_double_pin)) // True for double mould. 
          {
            if(digitalRead(treat_size_selection)) // Large treat
            {
              dosing_times = sizeof(Two_large_x) / sizeof(int); 
              x_sequence =  Two_large_x; 
              y_sequence = Two_large_y;
            }
            else  // Small sized treats 
            {
              // dosing_times = sizeof(Two_small_x) / sizeof(int); 
              dosing_times = 48; 
              x_sequence =  Two_small_x; 
              y_sequence = Two_small_y;
            }
          }
          
          else  // THE SINGLE MOULD SITUATAION. 
          {

          
            if(digitalRead(treat_size_selection) && digitalRead(mould_size_pin))  // Large treat and large mould. 
            {
              // Treat size is large and mould is full sized mould. 
              // dosing_times = Full_large_x::size() ; 
              dosing_times = sizeof(Full_large_x) / sizeof(int); 
              x_sequence = Full_large_x; 
              y_sequence = Full_large_y;
            }
            else if(digitalRead(treat_size_selection) && !digitalRead(mould_size_pin)) //Large treat and small mould. 
            {
              dosing_times = sizeof(Full_large_x) / sizeof(int)/4*3; 
              x_sequence = Full_large_x; 
              y_sequence = Full_large_y;
            }
            else if(!digitalRead(treat_size_selection) && digitalRead(mould_size_pin)) 
            {
              dosing_times = sizeof(Full_small_x) / sizeof(int); 
              x_sequence = Full_small_x; 
              y_sequence = Full_small_y;
              
            }
            else 
            {
              dosing_times = sizeof(Full_small_x) / sizeof(int)/4*3; 
              x_sequence = Full_small_x; 
              y_sequence = Full_small_y;
            }
          }
          
          dosing_count = 0 ; 
          to_next_postion = true; 
          
          // Serial.print("Doing times is  ");
          // Serial.println(dosing_times); 

          // Tare the schale.
          Load_cell_tare(); 

          // delay(200); 
          
          weight_n =  Load_cell_read();

          weight_n_1 = weight_n; 
          calibration_count = 0 ; 

        }
        else
        {
          if(dosing_count < dosing_times) // Still need to carry on dosing tasks
          {
            if(to_next_postion == true)
            {
              x1_stepper.moveTo((x_sequence[dosing_count])); 
              y_stepper.moveTo(y_sequence[dosing_count]); 
              to_next_postion =false; 
              Serial.println("Moving position set. "); 
            }
            else
            {
              x1_stepper.run();
              y_stepper.run();
              slider_stepper.run(); 
              if((abs(x1_stepper.currentPosition() - x_sequence[dosing_count]) < 150) && (abs(y_stepper.currentPosition() - y_sequence[dosing_count]) < 100) && !slider_stepper.isRunning())
              {
                // Dosing position reached.   `
                // turn on the dosing.
                digitalWrite(Dosing_control_pin,1);

                //delay(1000); // Temporarily to use delay function to 

                // Now we need to check if the nozzle flag if high 

                checking_dosing_completion();
                if(nozzle_signal_flag)
                {
                  digitalWrite(Dosing_control_pin, 0 );

                  to_next_postion = true; 
                  dosing_count ++ ;
                  nozzle_signal_flag = false; 

                  //Add delay of 0.3s. 
                  

                  //Logic for dosing calibration. 
                  calibration_count ++; 
                  
                  delay(150); 
                  // weight_n = Load_cell_read(); 
                  if(calibration_count ==calibration_size)
                  {
                    
                    weight_n = Load_cell_read(); 
                    
                    Serial.println(weight_n); 
                    treat_weight_diff = desired_treat_weight -  (weight_n-weight_n_1)/calibration_size; 
                    slider_stepper.move( Calc_slider_position(treat_weight_diff)) ;
                    // slider_stepper.runToPosition() ;
                    weight_n_1 = weight_n; 
                    calibration_count = 0 ; 

                    Serial.println("Weight diff per treat:");
                    Serial.println(treat_weight_diff);

                    
                    // lcd.setCursor(0,0); 
                    // lcd.print("5 treate weight:");
                    // lcd.setCursor(0,1); 
                    // lcd.print(weight_n); 
                  } 

                  

                }
               
                
              }
            }
            
          }
          else 
          {
            // Dosing has been done.  Move to the next state. Shoud be recalibration, but if calibration function is not done. We move to the ready state
            // current_state = ready; 

            // Calculate the difference between the desired and current average treat weigth. 
            float avg_treat_weight = Load_cell_read()/dosing_times; 
            if(digitalRead(treat_size_selection))
            {treat_weight_diff = desired_weight_large_treat - avg_treat_weight;}
            else
            {
              treat_weight_diff = desired_weight_small_treat -avg_treat_weight;


            }
            Serial.println("Weight diff is:"); 
            Serial.println(treat_weight_diff); 
            
            if(abs(treat_weight_diff)<0.3)
            {current_state = ready;}
            else
            {
              current_state = recalibration; 
            }
            

          }
          
        }

        break; 
      }
        

      case recalibration:
      {
        if(previous_state != recalibration)
        {
          
          previous_state = recalibration; 
          // Set the sliding moving target by using the 

          // slider_stepper.move(Calc_slider_position()); 
          lcd.begin(16, 2); 
          lcd.setCursor(0,0);
          lcd.print(treat_weight_diff);          
          slider_stepper.move(Calc_slider_position(treat_weight_diff)); 
  
        
        }
        slider_stepper.runToPosition();

        if(slider_stepper.targetPosition()==slider_stepper.currentPosition())
        {
          current_state = ready; 
          previous_state = recalibration; 
        }

        break; 
      }
        

      
      default:
        break;
        }
  } 

  // starting the state machine implementation

  



}

// The function to move left along the axis at a set speed. 
void x_moving_left()
{
    x1_stepper.move(1000); 
    x1_stepper.setSpeed(x_speed); 
    x1_stepper.runSpeedToPosition(); 

    x2_stepper.move(1000); 
    x2_stepper.setSpeed(x_speed); 
    x2_stepper.runSpeedToPosition(); 
}

void x_moving_right()
{
  x1_stepper.move(-1000); 
  x1_stepper.setSpeed(x_speed); 
  x1_stepper.runSpeedToPosition(); 

  x2_stepper.move(-1000); 
  x2_stepper.setSpeed(x_speed); 
  x2_stepper.runSpeedToPosition(); 
}


void y_moving_up()
{
  y_stepper.move(1000); 
  y_stepper.setSpeed(y_speed); 
  y_stepper.runSpeedToPosition(); 
}

void y_moving_down()
{
  y_stepper.move(-1000); 
  y_stepper.setSpeed(y_speed); 
  y_stepper.runSpeedToPosition(); 
}

void slider_moving_left()
{
  slider_stepper.setSpeed(500); 
  slider_stepper.runSpeed(); 
}

void slider_moving_right()
{
  slider_stepper.setSpeed(-500); 
  slider_stepper.runSpeed(); 
}



void checking_start_button_toggle()
{
  start_flag = false; 
  int start_reading = digitalRead(start_button_pin); 

      // ...............Debouncing logic for the start button.......
      // If the switch changed, due to noise or pressing:
      if (start_reading != last_start_button_state) {
        // reset the debouncing timer
        lastDebounceTime_start = millis();
        Serial.println("Start button toggole detecte."); 
        Serial.println(lastDebounceTime_start); 
      }

      if ((millis() - lastDebounceTime_start) > debounceDelay) {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:
        // Serial.println("Debounce successful"); 
        // if the button state has changed:
        if (start_reading != start_button_state) {
          start_button_state = start_reading;

          // only toggle the LED if the new button state is HIGH
          if (start_button_state == HIGH) {
            start_flag = true; 
            Serial.println("Start_button_triggered!"); 
          }
        }
      }
  // save the reading. Next time through the loop, it'll be the lastButtonState:
  last_start_button_state = start_reading;
}
        // .......... end of the debouncing logic..... }

// The function to check if the nozzle signal has a falling edge

bool checking_dosing_completion()
{

  int nozzle_reading = digitalRead(Nozzle_signal_in); 

      // ...............Debouncing logic for the start button.......
      // If the switch changed, due to noise or pressing:
      if (nozzle_reading != last_nozzle_state) {
        // reset the debouncing timer
        lastDebounceTime_nozzle = millis();
        

      }

      if ((millis() - lastDebounceTime_nozzle) > debounceDelay_nozzle) {
        // whatever the reading is at, it's been there for longer than the debounce
        // delay, so take it as the actual current state:

        // if the button state has changed:

        if (nozzle_reading != nozzle_valve_state) {
          
          nozzle_valve_state = nozzle_reading;
          Serial.println("Nozzle change detected!"); 
          // only toggle the LED if the new button state is HIGH
          if (nozzle_valve_state == LOW) {
            nozzle_signal_flag = true; 
          }
        }
      }
      last_nozzle_state = nozzle_reading; 

      return nozzle_signal_flag; 

        // .......... end of the debouncing logic..... 
}

// Turn on the dosing pin and wait untill the dosing is finished.  Then turn off the dosing pin. Move to the next position. 
void dose_once()
{
  digitalWrite(Dosing_control_pin, HIGH); 
  delay(100); 
  digitalWrite(Dosing_control_pin, LOW); 
}
// Tare load cell reading. 
void Load_cell_tare()
{
  scale.tare(); //Reset the scale to 0

  zero_factor = scale.read_average(5); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

}

// Read and converty load cell reading to grams
float Load_cell_read()
{
  float result = -(scale.read_average(5)-zero_factor)/calibration_factor;;
  Serial.println("scale result"); 
  Serial.println(result);
  return  result ;
}

// Move the dose sider based on the gram difference between the desired and actual weight per treat. 
// The function calculate the relave position of the current position of the slider. 
int Calc_slider_position(int weight_difference)
{
  int target_position_slider = -30 * weight_difference; 
  if(target_position_slider > 100)
  {
    target_position_slider =100;
  }
  else if (target_position_slider < -100)
  {
    target_position_slider = -100; 
  }
  Serial.println("Slider steps to move:");
  Serial.println(target_position_slider); 
  return target_position_slider; 

}



