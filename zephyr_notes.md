# Zephyr notes

## Node identifiers
- [Node identifiers are not values](https://docs.zephyrproject.org/latest/guides/dts/api-usage.html#node-identifiers-are-not-values)
  - meaning, for example when:
      - `#define LED0_NODE_IDENTIFIER DT_ALIAS(led0_alias)`
      - `LED0_NODE_IDENTIFIER` cannot be assigned to a variable, or even printed
        - when expanded by preprocessor, `DT_N_S_leds_S_led_0` might be returned, which then causes compilation error
      - instead it should be used along another macro, that will use it / concat it with another macro, that will actually return a value like
        - `DT_ALIAS(led0)` -> `DT_N_S_leds_S_led_0`
        - `DT_NODE_HAS_STATUS(DT_ALIAS(led0), okay)` -> `DT_NODE_HAS_STATUS_INTERNAL(DT_N_S_leds_S_led_0, okay)`
          - `DT_NODE_HAS_STATUS_INTERNAL(DT_N_S_leds_S_led_0, okay)` -> `IS_ENABLED(DT_CAT(DT_N_S_leds_S_led_0, _STATUS_ ## okay))`
            - `DT_CAT(DT_N_S_leds_S_led_0, _STATUS_okay)` -> `DT_N_S_leds_S_led_0 ## _STATUS_okay` -> `DT_N_S_leds_S_led_0_STATUS_okay`
            - `DT_N_S_leds_S_led_0_STATUS_okay` -> `1`
            - `IS_ENABLED(DT_N_S_leds_S_led_0_STATUS_okay)` -> .... magic .... -> `1`
              - `IS_ENABLED`: Checks for macro definition in compiler-visible expressions
                - e.i. if macro is defined can be checked during runtime, instead of failing during compilation
      - `##` macro operator takes two separate tokens and pastes them together to form a single token
      - `#` macro operator converts macro parameters to string literals without expanding the parameter definition
        - arguments to a macro aren't expanded if they appear along with a # or ##
          - that's why an intermediate macro is necessary
        - (by defining 2 macros we can print out what is being expanded instead of having to run the preprocessor)
          ```c
          #define QUOTE(str) #str
          #define EXPAND_AND_QUOTE(str) QUOTE(str)
          ```

## Node aliases
- a way to refer to a device node, that makes it easily accessible in code (DT_ALIAS(alias))
  ```dts
  / {
     aliases {
             my-uart = &uart0;
     };
  };
  ```
- to access in code
  ```c
  #define UART_NODE	DT_ALIAS(my_uart)
  ```
- !although alias was defined using dash "-", to access it in code we replace all dashes with underscores "_"!

## Add a device to device-tree using overlay
- [medium article](https://medium.com/home-wireless/using-a-pwm-device-in-zephyr-7100d089f15c)
- [example app using overlay](https://github.com/zephyrproject-rtos/zephyr/tree/main/samples/bluetooth/hci_spi)
- [youtube video](https://www.youtube.com/watch?v=oOoyRDXzO6g)
- inside a zephyr_app folder, that is somewhere within your zephyrproject (the env created by west), create a boards/ directory and place an overlay file there
- **zephyrproject/** #created by west
  - bootloader/
  - modules/
  - tools/
  - zephyr/
  - ..../
    - zephyr_app/
      - src/
      - ...
      - ...
      - boards/
        - name_of_board.overlay #place file here
- inside the overlay file we can define new nodes, aliases etc. or overwrite them


## PWM (Pulse Width modulation)
- https://en.wikipedia.org/wiki/Pulse-width_modulation
- https://www.youtube.com/watch?v=5nwNKPs2gco
- https://www.youtube.com/watch?v=GQLED3gmONg
- TODO
