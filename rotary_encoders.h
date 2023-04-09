///
/// rotary_encoders module
///
/// Used to track the state of a rotary encoder
///
/// This is a driver based off the KY-040 rotary encoder:
/// https://www.epitran.it/ebayDrive/datasheet/25.pdf
///
/// Author: Stric Roberts
/// Date: 4/7/2023
///
/// Usage:
/// The driver is made in a way to allow multiple encoders to be used
/// with other GPIO interrupts.
/// This is meant for bare metal use.
/// Software or HW debounce up to the developer, not handled here!
///
/// One encoder requires two GPIO pins, a DL and CLK for knob turns
/// This encoder also has a push button (non latching) with requires a 3rd GPIO pin
///
/// Example code:
/// NOTE: Look at define ROTARY_ENCODER_INSTANCES for how many encoders allowed
///
/// #include "rotary_encoder.h"
///
/// int main(...)
/// {
///
///      uint8_t const volume_instance_id = 0;
///      rotary_encoder_init(volume_instance_id,
///                          0,     // min value
///                          255,   // max value
///                          true,  // step on max/min values, no roll over
///                          true); // clockwise turn is positive increment
///
///      //mcu specific, psuedo-ish code
///      mcu_gpio_init(...);
///
///      for (;;)
///      {
///          rotary_encoder_task();
///
///          // Was an interrupt captured and handled?  Update app
///          if(rotary_encoder_check_event(instance_id))
///          {
///              app_update_volume(rotary_encoder_get_knob_value(volume_instance_idinstance_id));
///          }
///
///          // Was the value stepped on?  Ding the user
///          if(rotary_encoder_check_alert(instance_id))
///          {
///              app_alert_user_with_sound();
///          }
///      }
///
///      return 0;
/// }
///
/// // MCU specific, psuedo-ish code
/// // Software or HW debounce up to the developer, not handled here
/// MCU_GPIO_INTERRUPT()
/// {
///      // Rotary encoder 0 instance_id CLK pin interrupt flag
///      // transition low to high
///      if (PORT0_FLAGS & pinCLK_IF)
///      {
///         g_rotary_encoder_instance_num = 0;
///
///         // Clock high, DT high == Counter clockwise rotation
///         // Clock high, DT low == Clockwise rotation
///         g_rotary_encoder_flags |= mcu_get_pin_state(pinDT) ?
///                                   ROTARY_ENCODER_FLAG_CDW :
///                                   ROTARY_ENCODER_FLAG_CW;
///      }
///
///      // Rotary encoder 0 instance_id SW pin interrupt flag
///      if (PORT0_FLAGS & pinSW_IF)
///      {
///         g_rotary_encoder_instance_num = 0;
///
///         // Clock high, DT low == Counter clockwise rotation
///         // Clock high, DT high == Clockwise rotation
///         g_rotary_encoder_flags |= ROTARY_ENCODER_FLAG_SW;
///      }
/// }
///
#ifndef ROTARY_ENCODERS_H_
#define ROTARY_ENCODERS_H_

#include <stdint.h>
#include <stdbool.h>

/// Max number of instances this program supports
/// Increase or decrease for your needs
/// Can be up to UINT8_MAX before having to change code
#define ROTARY_ENCODER_INSTANCES 4u

/// Flags that are set in g_rotary_encoder_flags
#define ROTARY_ENCODER_FLAG_CW    0x01u
#define ROTARY_ENCODER_FLAG_CCW   0x02u
#define ROTARY_ENCODER_FLAG_SW    0x04u


/// Flags that interrupt triggers
extern volatile uint8_t g_rotary_encoder_flags;
/// Instance number for flags
extern volatile uint8_t g_rotary_encoder_instance_num;


bool rotary_encoder_init(uint8_t const instance_num,
                         int16_t const min_value,
                         int16_t const max_value,
                         bool    const step_on,
                         bool    const cw_rot_pos);

bool rotary_encoder_get_switch_value(uint8_t const instance_num);
int16_t rotary_encoder_get_knob_value(uint8_t const instance_num);

bool rotary_encoder_set_knob_value(uint8_t const instance_num,
                                   int16_t const value);

bool rotary_encoder_inc_knob_value(uint8_t const instance_num);
bool rotary_encoder_dec_knob_value(uint8_t const instance_num);

bool rotary_encoder_tog_switch_value(uint8_t const instance_num);

bool rotary_encoder_check_event(uint8_t const instance_num);
bool rotary_encoder_check_alert(uint8_t instance_num);
void rotary_encoder_task(void);

#endif /* ROTARY_ENCODERS_H_ */
