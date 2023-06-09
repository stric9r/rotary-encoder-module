# rotary-encoder-module
Rotary encoder interface.  Developed using KY-040 rotary encoder but can be used with any quadrature encoder.

Short demo of this module working here: https://www.youtube.com/watch?v=H6LCRhiRQaU

The module is made in a way to allow multiple rotary encoders to be used with other GPIO interrupts.
This is meant for bare metal use but could be easily modified for an operating system.
SW or HW debounce up to the developer, not handled here!

One encoder requires two GPIO pins, a DL and CLK for knob turns.
This encoder also has a push button (non latching) which requires an optional 3rd GPIO pin

Each encoder is referred to as an ```instance_num```  You can have up to ```ROTARY_ENCODER_INSTANCES```; this is configurable.

## Configuration
Look at ```ROTARY_ENCODER_INSTANCES``` in rotary_encoders.h for how many encoders allowed.

## Interrupts
The developer will need assign required pins for input, and write interrupt routine trigger on the CLK line.
See Example Code section below for clarification.

When an interrupt is triggered, the CLK must be compared agains the DT to determine if clockwise or counter clockwise motion.
CLK HIGH and DT HIGH is clockwise, CLK HIGH and DT LOW is counter clockwise.

For the module to act on any interrupt events in ```rotary_encoder_task(void)``` the following must be set.
 - Call ```g_rotary_encoder_set_flags(...)``` during an interrupt
    - Specify the encoder instance number
    - Specify flags to set```ROTARY_ENCODER_FLAG_CW``` or ```ROTARY_ENCODER_FLAG_CDW``` or ```ROTARY_ENCODER_FLAG_SW```

## Usage
Code is documented using Doxygen. The example code below is the best to reveiw, but here are some definitions to help understand:

 - **instance** - each rotary encoder is an instance.
 - **min/max value** - each rotary encoder can be set to a minimum and maximum value.
 - **step on** regarding - the min/max values, there is an option to step on the value if the min/max is reached.  If not set, then there will be a roll over.
 - **clockwise or counter clockwise** - the direction of the knob turn.  Clockwise is considered positive, while counter clockwise is considered negative.


## Example Code

 ```
  #include "rotary_encoder.h"

  int main(...)
  {

       uint8_t const volume_instance_id = 0;
       rotary_encoder_init(volume_instance_id,
                           0,     // min value
                           255,   // max value
                           true,  // step on max/min values, no roll over
                           true); // clockwise turn is positive increment

       //MCU specific that setups up gpio and interrupts
       mcu_gpio_init(...);

       for (;;)
       {
           rotary_encoder_task();

           // Was an interrupt captured and handled?  Update app
           if(rotary_encoder_check_event(instance_id))
           {
               app_update_volume(rotary_encoder_get_knob_value(volume_instance_idinstance_id));
           }

           // Was the value stepped on?  Ding the user
           if(rotary_encoder_check_alert(instance_id))
           {
               app_alert_user_with_sound();
           }
       }

       return 0;
  }

  // MCU specific, psuedo-ish code
  // Software or HW debounce up to the developer, not handled here
  MCU_GPIO_INTERRUPT()
  {
     // Interrupt on rotary encoder clock?
    if (0u != (GPIO_IntGetEnabled() & ROTARY0_CLK_PIN_MASK))
    {
        //Check state of ROTARY0_DT_PIN.  High == CCW rotation
       uint8_t flag = GPIO_PinInGet(ROTARY0_DT_PORT, ROTARY0_DT_PIN) ?
                      ROTARY_ENCODER_FLAG_CCW :
                      ROTARY_ENCODER_FLAG_CW;

       rotary_encoder_set_flags(0, flag);

    }

    // Interrupt on rotary encoder 0 switch?
    if (0u != (GPIO_IntGetEnabled() & ROTARY0_SW_PIN_MASK))
    {
       rotary_encoder_set_flags(0, ROTARY_ENCODER_FLAG_SW);;
    }
  }
```
