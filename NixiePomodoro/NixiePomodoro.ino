#include "timer/src/timermngmnt.h"
#include "OneButton/src/OneButton.cpp"

#define DIN_PIN   5          ///< Nixie driver (shift register) serial data input pin             
#define CLK_PIN   6          ///< Nixie driver clock input pin
#define EN_PIN    7          ///< Nixie driver enable input pin
#define SETTINGS_BUTTON 8    ///< Settings button pin

#define TIME_LUT_SIZE 6
#define NUMBER_OF_USAGE_PIN_FOR_ARRY 20 ///<Total Number of usage pin

volatile ut32_timer systick_cnt;

timer_t t1;
int time_holder;
OneButton set_button(SETTINGS_BUTTON);

void init_timer_to_1ms()
{
 // Prescaler 64 ==> resolution changes from 0,0000000625 to 0,000004
 // Need interval of 1Ms ==> 0,001/((1/16000000)*64) = 250 ticks 
 
 // Set prescaler to 64 ; (1 << CS01)|(1 << CS00)
 // Clear Timer on Compare (CTC) mode ; (1 << WGM02)
 TCCR0A = 0 ;    
 TCCR0B |= (1 << WGM02) | (1 << CS01) | (1 << CS00); 

 // set Output Compare Register to (250 - 1) ticks
 OCR0A = 0xF9;
 
 // Timer count = (required delay/clock time period) - 1
 // 249 = (0,001/0,000004) - 1
 
 // initialize counter
 TCNT0 = 0;

 // Set Timer Interrupt Mask Register to 
 // Clear Timer on Compare channel A for timer 0
 TIMSK0 |= (1 << OCIE0A);
}

ISR(TIMER0_COMPA_vect) 
{
 ++systick_cnt;
}

void ShiftOutData(uint32_t nixie_bits)
{
  digitalWrite(EN_PIN, 0); 
 
  // Send data to the nixie drivers 
  for (int i = NUMBER_OF_USAGE_PIN_FOR_ARRY; i >= 0; --i)
  {
    digitalWrite(DIN_PIN, (nixie_bits & (1UL << i)) ? 1 : 0);  
    digitalWrite(CLK_PIN, 1);
    digitalWrite(CLK_PIN, 0);  
  }   
  
  // Return the EN pin high to signal chip that it 
  // no longer needs to listen for data
  digitalWrite(EN_PIN, 1);   
}

void nixie_display(uint8_t time)
{  
  // Cathodes assignment to the position in the 24 bit array
  static const uint8_t nixie1[] = {
  //0   1   2   3   4   5   6   7   8   9  
    0,  9,  8,  7,  6,  5,  4,  3,  2,  1 };
  static const uint8_t nixie2[] = {
  //0   1   2   3   4   5   6   7   8   9
    10, 14, 15, 16, 17, 18, 19, 13, 12, 11 };
  
  uint8_t digit1 = (time) / 10;
  uint8_t digit2 = (time) % 10;

  uint32_t nixie_bits = 0;
  
  nixie_bits |= (1UL << nixie1[digit1]) | (1UL << nixie2[digit2]);
  
  ShiftOutData(nixie_bits);
}

void nixie_tmr_cb(void* arg)
{  
  int* disp_val = (int*)arg;
  if (0 == *disp_val)
  {
    unlink_timer(&t1);
  }
  else
  {
    *disp_val -= 1;
  }
  
  nixie_display(*disp_val); 
}

void set_button_one_click_cb(void)
{   
  unlink_timer(&t1);

  //Minutes
  static const uint8_t time_lut[TIME_LUT_SIZE] = {
    25, 50, 60, 70, 80, 90
  };

  static uint8_t button_cnt = 0;
 
  time_holder = time_lut[button_cnt % TIME_LUT_SIZE];
 
  ++button_cnt;
  nixie_display(time_holder);
}

void set_button_after_long_press_cb(void* arg)
{
  init_timer(&t1, LOOP, arg, nixie_tmr_cb, 1000);
  start_timer(&t1);
}

void setup() 
{
  set_button.attachClick(set_button_one_click_cb);
  set_button.attachLongPressStop(set_button_after_long_press_cb, &time_holder);

  pinMode(DIN_PIN, OUTPUT); 
  digitalWrite(DIN_PIN, LOW);    
  
  pinMode(CLK_PIN, OUTPUT);
  digitalWrite(CLK_PIN, LOW);         

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);

  init_timer_to_1ms();

  nixie_display(0);
}

void loop ()
{
  timer_pool();
  set_button.tick();
}