#include <stdio.h>
/* -------------------------------------------------------------------------- */
// Test setup config, all params should be saved along with the measurements
/* -------------------------------------------------------------------------- */

/**/
#define TEST_FOLDER_NAME "TEST"
#define CONFIG_FILE_NAME "config.txt"

/**/
#define TEST_VOLTAGE "22V"

/* one arm mounted static, drone mounted static, drone in air static, ... */
#define TEST_SETUP_MOUNTING_DESCRIPTION "one arm mounted static"

#define TEST_MOTOR_COUNT "1"
#define TEST_PROPELER_STATES "no damage"
#define TEST_MOTOR_STATES "no damage"
#define TEST_ESC_CONFIG "default config"

/* -------------------------------------------------------------------------- */

/* number of test sets to perform (1 set ~ itterating over all speeds)*/
#define TEST_SET_COUNT 70

/* comma separated ascending values in range [1, 100], represents speed% of motor */
#define TEST_MOTOR_SPEEDS 10, 25, 50, 75, 90, 100, 0

// TODO: to simulate motor damage, different motors might need to rotate at different speeds

/* duration of motor rotating and reading values from accelerometer (after stopping, extra 1-2 sec to read leftover values from buffer) */
#define TEST_MEASUREMENT_DURATION_SEC 8

/* time for the motor to ramp up/down before starting to read values from accelerometer */
#define TEST_MEASUREMENT_PAUSE_DURATION_SEC 4

/* -------------------------------------------------------------------------- */
#define STR2(s) #s
#define STR(s) STR2(s)

// TODO: move to tester.h?
#define TEST_DESCRIPTION "__TIMESTAMP__: " __TIMESTAMP__ "\n"                                                                                     \
                         "TEST_FOLDER_NAME: " TEST_FOLDER_NAME "\n"                                                                               \
                         "TEST_SETUP_MOUNTING_DESCRIPTION: " TEST_SETUP_MOUNTING_DESCRIPTION "\n"                                                 \
                         "TEST_VOLTAGE: " TEST_VOLTAGE "\n"                                                                                       \
                         "TEST_MOTOR_COUNT: " TEST_MOTOR_COUNT "\n"                                                                               \
                         "TEST_PROPELER_STATES: " TEST_PROPELER_STATES "\n"                                                                       \
                         "TEST_MOTOR_STATES: " TEST_MOTOR_STATES "\n"                                                                             \
                         "TEST_ESC_CONFIG: " TEST_ESC_CONFIG "\n"                                                                                 \
                         "\n"                                                                                                                     \
                         "TEST_SET_COUNT: " STR((TEST_SET_COUNT)) "\n"                                                                            \
                         "TEST_MOTOR_SPEEDS: " STR((TEST_MOTOR_SPEEDS)) "\n"                                                                      \
                         "TEST_MEASUREMENT_DURATION_SEC: " STR(TEST_MEASUREMENT_DURATION_SEC) "\n"                                                \
                         "TEST_MEASUREMENT_PAUSE_DURATION_SEC: " STR(TEST_MEASUREMENT_PAUSE_DURATION_SEC) "\n"
