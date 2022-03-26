#ifndef __ESC_PWM_H__
#define __ESC_PWM_H__
#include <drivers/pwm.h>
#include <logging/log.h>
#include <shell/shell.h>

/* -------------------------------------------------------------------------- */

/* node identifier of the node with alias equal to ... */
#define PWM_LED0_NODE_ID DT_ALIAS(pwm_led0)
#define PWM_LED1_NODE_ID DT_ALIAS(pwm_led1)
#define PWM_LED2_NODE_ID DT_ALIAS(pwm_led2)

/* ... */
#if !DT_NODE_HAS_STATUS(PWM_LED0_NODE_ID, okay) || !DT_NODE_HAS_STATUS(PWM_LED1_NODE_ID, okay) || !DT_NODE_HAS_STATUS(PWM_LED2_NODE_ID, okay)
#error "... some devicetree alias is not defined"
#endif

/* node identifier of the pwm controller, of some node ... with pwm property */
#define PWM0_CTLR DT_PWMS_CTLR(PWM_LED0_NODE_ID)
#define PWM1_CTLR DT_PWMS_CTLR(PWM_LED1_NODE_ID)
#define PWM2_CTLR DT_PWMS_CTLR(PWM_LED2_NODE_ID)
// will be same for all nodes?

/* channel cell value of a pwm specifier? */
#define PWM0_CHANNEL DT_PWMS_CHANNEL(PWM_LED0_NODE_ID)
#define PWM1_CHANNEL DT_PWMS_CHANNEL(PWM_LED1_NODE_ID)
#define PWM2_CHANNEL DT_PWMS_CHANNEL(PWM_LED2_NODE_ID)
// diff for all

/* ... */
#define PWM0_FLAGS DT_PWMS_FLAGS(PWM_LED0_NODE_ID)
#define PWM1_FLAGS DT_PWMS_FLAGS(PWM_LED1_NODE_ID)
#define PWM2_FLAGS DT_PWMS_FLAGS(PWM_LED2_NODE_ID)
// same for all?

/* ... */
#define PWM0_PERIOD DT_PWMS_PERIOD(PWM_LED0_NODE_ID)
// #define PWM1_PERIOD DT_PWMS_PERIOD(PWM_LED1_NODE_ID)
// #define PWM2_PERIOD DT_PWMS_PERIOD(PWM_LED2_NODE_ID)
// same for all?

/* -------------------------------------------------------------------------- */
/**
 * TODO: put into separate file? default values, if non other included?
 *
 * ESC general (Same for all ESCs):
 * - ESC frequency = 50Hz ~ Period = 20 millisec.
 * - (ESC duty cycle in range 5-10%? => min, max=1, 2 millisec)
 * ESC specific:
 * - MAX_PULSE_WIDTH = 2 millisec
 * - MINIMUM_PULSE_WIDTH = 2 millisec
 * - NEUTRAL_PULSE_WIDTH = 1 millisec
 */
#define ESC_PERIOD_USEC 20 * USEC_PER_MSEC

#define MAX_PULSE_WIDTH_USEC 10 * ESC_PERIOD_USEC / 100                        // 10% duty_cycle
#define MIN_PULSE_WIDTH_USEC 5 * ESC_PERIOD_USEC / 100                         // 5% duty_cycle
#define MID_PULSE_WIDTH_USEC (MAX_PULSE_WIDTH_USEC + MIN_PULSE_WIDTH_USEC) / 2 //

#define PWM_ARM_SLEEP_MSEC 2500
#define PWM_CALIB_SLEEP_MSEC 3500
#define PWM_POST_CALIB_SLEEP_MSEC 3500

/* -------------------------------------------------------------------------- */
/**
 * ESC calibration / configuration guide:
 * ----------------------------------------------------------------------------
 * - CHECK THE ESC's USER MANUAL or SPEC SHEET
 *   - ESC pwm frequency standart = 50Hz ~ period = 20 millisec = 20 000 microsec (usec)
 *   - find out default MIN/MAX PULSE WIDTHs ~ to the 0/100% throttle for the given ESC
 *   - find out in what range the MIN/MAX pulse widths have to be for calibration
 *   - (LOW, HIGH ~ MIN, MAX PULSE WIDTHS, MID~(MIN+MAX)/2)
 *
 * (Note: set_pwm(pulse_width) ~ pwm_pin_set_usec(..., ..., ESC_PERIOD=20 000 microsec, pulse_width, ...))
 *
 * TO START USING THE ESC
 * - Start with all (BLIP, ESC) turned off
 * - flash and reset BLIP
 * - init_pwm(), make sure all devices available
 * - FOLLOW 1 of possible options:
 *   - A) ARMING SEQUENCE
 *       - set throttle to 0% (set pwm LOW)
 *         - // if throttle < MID => nothing will happen until set to LOW
 *         - // if MID < throttle < HIGH will initiate calibration
 *       - power up the ESC (plug to a battery)
 *       - wait 2 sec
 *       - should hear 2 sets of beeps (..) => indicating ESC is armed and ready to use
 *       - ...
 *
 *   - B) CALIBRATE (set new  pwm HIGH/LOW)
 *       - set throttle to 100% (set pwm "new" HIGH) // has to be in some range, check ESC manual
 *       - power up the ESC (plug to a battery)
 *       - wait 2-3 sec
 *       - should hear 4 groups of 2 sets of fast beeps (.. .. .. ..)
 *       - set throttle to   0% (set pwm "new" LOW)
 *       - wait 2-3 sec
 *       - should hear ... 2 fast beeps? => indicating ESC is calibrated
 *       - ...
 *
 *   - C) ENTER PROGAMMING MODE
 *       - set throttle to 100% (set pwm to HIGH) // should be calibrated already, or default?
 *       - power up the ESC (plug to a battery)
 *       - wait for 2 seconds,
 *       - should hear 4 groups of two sets of fast beeps (.. .. .. ..) // may differ with other ESCs
 *         - => confirmation PROGRAMMING MODE ENTERED
 *       - check the Programming Tone Reference Table
 *       - to select an option set throttle to 0% (set pwm LOW)
 *       - the system will exit the programming mode and save the setting
 *         - only 1 item at a time can be configured (to program another, unplug the ESC and power it on again, i.e. repeat from step 1 again)
 *
 */

/* -------------------------------------------------------------------------- */

/**
 * @brief  get device bindings, check if all devices are ready
 * @retval   0 on success
 * @retval < 0 on error
 */
int pwm_init(void);

/** @brief set pwm LOW */
void pwm_arm(void);

/**
 * @brief  set all devices pwm pulse width to pulse_width_usec
 *
 * @note  has to be in range pulse_width_usec in [MINIMUM_PULSE_WIDTH, MAX_PULSE_WIDTH]
 * @retval   0 on success
 * @retval < 0 on error
 */
int pwm_set(uint32_t pulse_width_usec);

/**
 * @brief  set all devices pwm pulse width to some percantage of MIN_PULSE_WIDTH_USEC and MAX_PULSE_WIDTH_USEC
 * @retval   0 on success
 * @retval < 0 on error
 */
int pwm_set_throttle(uint32_t percentage);

/* -------------------------------------------------------------------------- */

#endif //__ESC_PWM_H__
