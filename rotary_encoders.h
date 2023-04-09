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
/// Instance number for flags that interrupt triggers
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
