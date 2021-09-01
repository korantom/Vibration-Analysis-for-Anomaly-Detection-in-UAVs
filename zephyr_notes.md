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

