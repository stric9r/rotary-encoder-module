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
#include "rotary_encoders.h"

/// Flags used to track events from interrupts, each bit is the instance flagged
volatile uint32_t rotary_encoder_cw_flags = 0;
volatile uint32_t rotary_encoder_ccw_flags = 0;
volatile uint32_t rotary_encoder_sw_flags = 0;

///@todo If knob_min/max == INT16_MIN/MAX there will be some glitches.
///      If your rotary encoder requires 32767 points then this code is probably
///      not the best thing to use.
typedef struct rotary_encoder
{
    bool b_initialized;          /// Is this instance being used

    int16_t knob_value;          /// Relative knob turn value
    int16_t knob_max_value;      /// Max value of knob
    int16_t knob_min_value;      /// Min value of knob
    bool b_knob_allow_step_on;   /// Step on the max knob value, or allow rollover
    bool b_knob_cw_rot_positive; /// True if clockwise rotation leads to increment
                                 /// False if counter clockwise rotation to increment

    int16_t switch_value;        /// Switch value

    bool b_event_occured;       /// Event occurred, cleared after being read
                                /// Used to find out if a value was updated
    bool b_alert_occured;       /// Used to find out if a value was stepped on

} rotary_encoder_t;

/// Array that tracks instances
static rotary_encoder_t instance_arr[ROTARY_ENCODER_INSTANCES] = {0};


static bool rotary_encoder_force_bounds(uint8_t instance_num);
static bool rotary_encoder_initialized(uint8_t instance_num);

/// Init instance of rotary encoder
/// @param instance_num Instance number to track in module
/// @param min_value    Min value the knob can report
/// @param max_value    Max value the knob can report
/// @param step_on      True if step on value if meets max/min
///                     False if allow rollover from max to min, and min to max
/// @param cw_rot_ps    True if clockwise rotation is positive, false if negative
/// @return True on success, false on error
bool rotary_encoder_init(uint8_t const instance_num,
                         int16_t const min_value,
                         int16_t const max_value,
                         bool    const step_on,
                         bool    const cw_rot_pos)
{
    bool b_status = false;

    // Check that instance is within array bounds
    if(ROTARY_ENCODER_INSTANCES > instance_num)
    {
      instance_arr[instance_num].b_initialized = true;

      instance_arr[instance_num].knob_value = 0;
      instance_arr[instance_num].knob_max_value = max_value;
      instance_arr[instance_num].knob_min_value = min_value;
      instance_arr[instance_num].b_knob_allow_step_on = step_on;
      instance_arr[instance_num].b_knob_cw_rot_positive = cw_rot_pos;

      instance_arr[instance_num].switch_value = 0;

      instance_arr[instance_num].b_event_occured = false;
      instance_arr[instance_num].b_alert_occured = false;

      b_status = true;
    }

    return b_status;
}

/// Get the rotary encoder relative knob value
/// @param instance_num Instance number of encoder to get
/// @return The knob value, 0 if not valid instance
int16_t rotary_encoder_get_knob_value(uint8_t const instance_num)
{
    int16_t status = 0;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
        status = instance_arr[instance_num].knob_value;
    }

    return status;
}

/// Get the rotary encoder switch value
/// @param instance_num Instance number of encoder to get
/// @return The switch value
bool rotary_encoder_get_switch_value(uint8_t const instance_num)
{
    bool b_status = 0;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
        b_status = instance_arr[instance_num].switch_value;
    }

    return b_status;
}

/// Set the rotary encoder relative knob value
/// @param instance_num Instance number of encoder to set
/// @param value        Value to set the knob.
/// @return True on success, false on error
bool rotary_encoder_set_knob_value(uint8_t const instance_num,
                                   int16_t const value)
{
    bool b_status = false;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
        instance_arr[instance_num].knob_value = value;
        rotary_encoder_force_bounds(instance_num);

        b_status = true;;
    }

    return b_status;
}

/// Increment the rotary encoder relative knob value
/// @param instance_num Instance number of encoder to set
/// @return True on success, false on error
bool rotary_encoder_inc_knob_value(uint8_t const instance_num)
{
    bool b_status = false;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
            ++instance_arr[instance_num].knob_value;
            rotary_encoder_force_bounds(instance_num);

            b_status = true;
    }

    return b_status;
}

/// Decrement the rotary encoder relative knob value
/// @param instance_num Instance number of encoder to set
/// @return True on success, false on error
bool rotary_encoder_dec_knob_value(uint8_t const instance_num)
{
    bool b_status = false;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
            --instance_arr[instance_num].knob_value;
            rotary_encoder_force_bounds(instance_num);

            b_status = true;
    }

    return b_status;
}

/// Toggle the rotary encoder switch value
/// @param instance_num Instance number of encoder to set
/// @return True on success, false on error
bool rotary_encoder_tog_switch_value(uint8_t const instance_num)
{
    bool b_status = false;

    // Check that instance is within array bounds
    if(rotary_encoder_initialized(instance_num))
    {
        instance_arr[instance_num].switch_value =
                !instance_arr[instance_num].switch_value;
        b_status = true;
    }

    return b_status;
}

/// Set rotary encoder flags
/// This is meant to be used in an interrupt, or to trigger an event manually
/// It will only set the flags, never clear them.  Flags are cleared when read.
/// @param instance_num Instance number of encoder flags to set
/// @param flag         The flag value to set. Valid options are:
///                     ROTARY_ENCODER_FLAG_CW  (Clockwise)
///                     ROTARY_ENCODER_FLAG_CCW (Counter clockwise)
///                     ROTARY_ENCODER_FLAG_SW  (Switch)
/// @return True if flags were set, false if not
bool rotary_encoder_set_flags(uint8_t const instance_num,
                              uint8_t const flag)
{
    bool b_status = false;

    if(rotary_encoder_initialized(instance_num))
    {
        if(ROTARY_ENCODER_FLAG_CW  == flag)
        {
            rotary_encoder_cw_flags |= (1u << instance_num);
            b_status = true;
        }

        if(ROTARY_ENCODER_FLAG_CCW  == flag)
        {
            rotary_encoder_ccw_flags |= (1u << instance_num);
            b_status = true;
        }

        if(ROTARY_ENCODER_FLAG_SW  == flag)
        {
            rotary_encoder_sw_flags |= (1u << instance_num);
            b_status = true;
        }
    }

    return b_status;
}

/// Was an interrupt handled for rotary encoder
/// @param instance_num Instance number of encoder to check
/// @return True if knob or switch event occurred, false otherwise
bool rotary_encoder_check_event(uint8_t instance_num)
{
    bool b_status = false;

    // Check and clear
    if(rotary_encoder_initialized(instance_num))
    {
        b_status |= instance_arr[instance_num].b_event_occured;
        instance_arr[instance_num].b_event_occured = false;
    }

    return b_status;
}

/// Was an alert for rotary encoder set
/// Right now this is only used to generate an alert of the value being stepped on
/// @param instance_num Instance number of encoder to check
/// @return True if knob or switch event occurred, false otherwise
bool rotary_encoder_check_alert(uint8_t instance_num)
{
    bool b_status = false;

    // Check and clear
    if(rotary_encoder_initialized(instance_num))
    {
        b_status = instance_arr[instance_num].b_alert_occured;
        instance_arr[instance_num].b_alert_occured = false;
    }

    return b_status;
}

/// Flagged based task to handle interrupts regarding the encoder knob
void rotary_encoder_task(void)
{

    // Read what the interrupts set, then clear them
    uint32_t const tmp_cw_flags = rotary_encoder_cw_flags;
    uint32_t const tmp_ccw_flags = rotary_encoder_ccw_flags;
    uint32_t const tmp_sw_flags = rotary_encoder_sw_flags;

     rotary_encoder_cw_flags = 0;
     rotary_encoder_ccw_flags = 0;
     rotary_encoder_sw_flags = 0;

    // Loop through and make changes as needed
    for(uint8_t i = 0; i < ROTARY_ENCODER_INSTANCES; i++)
    {
        // Check if any flags set first
        bool b_increment = (0 != ((1<<i) & tmp_cw_flags));
        bool b_decrement = (0 != ((1<<i) & tmp_ccw_flags));
        bool b_switch    = (0 != ((1<<i) & tmp_sw_flags));

        bool b_event = b_increment || b_decrement || b_switch;

        if(b_event && rotary_encoder_initialized(i))
        {
            bool b_tmp_cw_pos = instance_arr[i].b_knob_cw_rot_positive;

            // Flags set, handle them
            if(b_increment)
            {
                b_tmp_cw_pos ?
                        rotary_encoder_inc_knob_value(i) :
                        rotary_encoder_dec_knob_value(i);
            }

            if(b_decrement)
            {
                    b_tmp_cw_pos ?
                            rotary_encoder_dec_knob_value(i) :
                            rotary_encoder_inc_knob_value(i);
            }

            if(b_switch)
            {
                rotary_encoder_tog_switch_value(i);
            }

            instance_arr[i].b_event_occured = true;
        }
    }

}

/// Check if bounds are enabled for knob values, and force if so
/// @param instance Instance number to track in module
/// @return True if encoder was stepped on or rolled over, false otherwise
static bool rotary_encoder_force_bounds(uint8_t instance_num)
{
    bool b_status = false;

    int16_t value = instance_arr[instance_num].knob_value;

    bool b_above_max = (value > instance_arr[instance_num].knob_max_value);
    bool b_below_min = (value < instance_arr[instance_num].knob_min_value);

    if(b_above_max || b_below_min)
    {
        // Should the value be stepped on?
        if(instance_arr[instance_num].b_knob_allow_step_on)
        {

            value = b_above_max ?
                    instance_arr[instance_num].knob_max_value:
                    value;

            value = b_below_min ?
                    instance_arr[instance_num].knob_min_value:
                    value;


            instance_arr[instance_num].knob_value = value;

        }
        else
        {

            value = b_above_max ?
                    instance_arr[instance_num].knob_min_value:
                    value;

            value = b_below_min ?
                    instance_arr[instance_num].knob_max_value:
                    value;

        }

        instance_arr[instance_num].knob_value = value;
    }

    b_status =
            instance_arr[instance_num].b_alert_occured =
            (b_above_max || b_below_min);

    return b_status;
}

/// Check if the instance is initialized
/// @param instance Instance number to track in module
/// @return True if encoder was initialized, false otherwise
static bool rotary_encoder_initialized(uint8_t instance_num)
{
    bool b_status = (ROTARY_ENCODER_INSTANCES > instance_num);
    b_status &= instance_arr[instance_num].b_initialized;

    return b_status;
}
