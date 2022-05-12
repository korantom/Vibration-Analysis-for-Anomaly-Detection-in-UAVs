# Vibration-Analysis-for-Anomaly-Detection-in-UAVs

---
# Blip board info
- [ Blip | Crowd Supply](https://www.crowdsupply.com/electronut-labs/blip)
- [ GitHub - electronut/ElectronutLabs-blip: A Nordic nRF52840 dev board with sensors and built-in debugger](https://github.com/electronut/ElectronutLabs-blip)
- [ Electronut Labs Blip — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/boards/arm/nrf52840_blip/doc/index.html)

---
# Zephyr RTOS
- [Zephyr getting started](https://docs.zephyrproject.org/latest/getting_started/index.html#)
  - [ARM toolchain](https://docs.zephyrproject.org/latest/getting_started/toolchain_3rd_party_x_compilers.html#gnu-arm-embedded)
- [Sample applications](https://docs.zephyrproject.org/latest/samples/index.html#samples-and-demos)
---

## Project Setup
<!-- Create a zephyr dev env and clone this repo into the top dir to match this folder structure -->

- **zephyrproject/** #created by west
  - bootloader/
  - modules/
  - tools/
  - zephyr/
  - ---
  - **Vibration-Analysis-for-Anomaly-Detection-in-UAVs/** #clone project into zephyrproject
    - zephyr_app/
      - boards/
        - board_name.overlay
      - src/
        - main.c
      - CMakeLists.txt
      - prj.conf

---
## TODO some fixes needed to be done in zephyr, either pprovide a fork with fixes or instruction on what to fix
- e.g. to fix the `sdmmc_spi: Timeout while waiting for !255` error in `zephyr/drivers/disk/sdmmc_spi.c`
add `.delay = 10` to ... `.cs = {...}`
---
# Other useful
- [App development](https://docs.zephyrproject.org/latest/application/index.html#application)
- Device tree
  - (on zephyr works differently then on linux)
    - linux: describe HW, compile, get a blob(binary format), on boot the blob is used ...
    - blob is often too big, zephyr doesn't use the blob, instead we use it to generate include info
  - [Zephyr Device tree guide](https://docs.zephyrproject.org/latest/guides/dts/index.html)
  - [Device tree general youtube video](https://www.youtube.com/watch?v=Nz6aBffv-Ek)
- [other](https://www.youtube.com/watch?v=oOoyRDXzO6g)
- [Qt Creator - zephyr ide](https://www.qt.io/product/development-tools)
---
# Build Guide
- `cd ~/zephyrproject`
- `west build --pristine -b nrf52840_blip Vibration-Analysis-for-Anomaly-Detection-in-UAVs/zephyr_app_samples/hello_world`
- `minicom -D /dev/cu.usbmodemC1E9B7D03`
- `west flash --gdb-serial /dev/cu.usbmodemC1E9B7D01`
- restart device to run app

## Run just preprocessor
- make error in code
- run west build, should fail -> you will see how it was compiled
- take the gcc compilation command and:
  - add -E
  - remove error flags until it works (-MD -MT CMakeFiles/app.dir/src/main.c.obj -MF CMakeFiles/app.dir/src/main.c.obj.d -o CMakeFiles/app.dir/src/main.c.obj)

# Testing
- KCONFIG (`prj.conf`) options
  - If dropping logs
    - set larger log buffer?
    - or switch from `CONFIG_LOG_MODE_DEFERRED` to `CONFIG_LOG_MODE_IMMEDIATE`
  - limiting logs to severity ...
    - `CONFIG_LOG_MAX_LEVEL=4` ~ all logs (ERR, WRN, INF, DBG)
    - `CONFIG_LOG_MAX_LEVEL=3` ~ ERR, WRN, INF
    - `CONFIG_LOG_MAX_LEVEL=2` ~ ERR, WRN


# HW used
- https://hobbyking.com/en_us/hobbyking-20a-blueseries-brushless-speed-controller.html
  - https://cdn-global-hk.hobbyking.com/media/file/214300043X9340X50.pdf
    - TODO: download and add to repo
