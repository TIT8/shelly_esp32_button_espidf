# Native ESP code for Shelly's toggle button
❗ [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) version of the Arduino repo: https://github.com/TIT8/shelly_button_esp32_arduino

## Description

A push button is connected to the [ESP32](https://github.com/espressif/arduino-esp32) microcontroller. When its state changes, it will trigger an [MQTT](https://mqtt.org/) publish which will toggle the light controlled by a [`Shelly plus 1 relay`](https://www.shelly.com/en-it/products/product-overview/shelly-plus-1). 

❗Keep in mind that you should either use a capacitor (better) or providing some delay in the code to debounce the push button and filter out spurious changes. Pull down the push button via a 10k resistor, **if you don't set the pull-up mode on the input pin**.

<br>

<p align="center"><img src="https://github.com/TIT8/shelly_esp32_button_espidf/assets/68781644/42b67ad6-4091-4f7f-9a1e-e24e876d9295" alt="Schematich" width='600' /></p>

<br>

- For testing I use the [Hive MQ public broker](https://www.hivemq.com/mqtt/public-mqtt-broker/).
- For production I use [Mosquitto](https://mosquitto.org/) from a Docker container inside my local environment (see the [Docker compose file](https://github.com/TIT8/shelly_button_esp32/blob/master/compose.yaml)).

❗ The software and the hardware (watchdog timer) can already handle reconnections if something goes wrong.

## Prerequisities

1. Look at the [PlatformIO documentation](https://docs.platformio.org/en/stable/tutorials/espressif32/espidf_debugging_unit_testing_analysis.html) to start.
2. If you already have the ESP-IDF installed (via Platformio or not), all the dependencies come with it, so you won't import anything. Simply build and upload 💪.
3. An ESP32 board (with IDF, also other versions like S2/S3/C3).
4. Remember to add the [CP2102 driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads) to connect old ESP32 development board (with CP2102 as USB-UART bridge).
5. [How to](https://github.com/sukesh-ak/setup-mosquitto-with-docker) setup a local broker in a Docker container.

## Why busy wait the button status and not polling it or be notified by interrupts?

Because the ESP32 has two cores, Core 1 is [designated](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32/api-guides/performance/speed.html#task-priorities) for very low priority tasks, which are rescheduled by FreeRTOS (alternating with the idle task responsible for resetting the watchdog). In this specific case, I prefer using busy waiting for a quick response to input, although I acknowledge it sacrifices efficiency. An interrupt-notified task (unblocked via queue or semaphore) would enhance efficiency. A task blocked on I/O might not be rescheduled, leaving only the idle task on Core 1, which can put the core in low power mode.   
However, the circuit from the button to the ESP32 input pin must include some form of debouncing (hardware-based, with RC filters), or else the task will continuously block and unblock when pressing and releasing the button, resulting in multiple MQTT command transmissions (undesirable for my application). Alternatively, a timer can be employed to prompt the task to check the input value when it fires, reducing CPU workload (although the task is rescheduled less frequently than with busy waiting, it still wastes CPU when rescheduled).

In a multitasking RTOS environment like ESP-IDF-FreeRTOS, busy waiting allows for software-based debouncing, simplifying hardware requirements. Laziness is the primary motivation; I seek a quick solution with minimal components, as I'm utilizing the ESP32 development board with a small push button on a compact breadboard, making it challenging to add resistors and capacitors in the limited space available.

Ideally, I would prefer a hardware debouncing solution coupled with interrupts. Perhaps in the future, with a custom PCB, I can implement this. However, for now, I'm constrained to using [software debouncing with busy waiting](https://github.com/TIT8/shelly_esp32_button_espidf/blob/e734a4e6457348e04da7f35a4adea39093754591/src/main.c#L182) on the low-priority task of Core 1 of the ESP32 (as Core 0 handles MQTT, Wi-Fi, and event loop tasks and is already busy). Depending on the MCU, RTOS, or hardware setup, I might opt for a different approach entirely.

For further insights, refer to this discussions: [difference between polling and busy waiting](https://stackoverflow.com/questions/10594426/what-is-the-difference-between-busy-wait-and-polling) and [difference between polling and interrupt for IO](https://stackoverflow.com/questions/10388757/polling-vs-interrupt).

## ESP-IDF vs Arduino

| Features | ESP-IDF | Arduino |
| :-------- | ---- | ----- |
| Dependencies | All included in the official SDK | <ul><li>ArduinoJson</li><li>Pubsubclient</li><li>ArduinoOta</li></ul> |
| Clear advantage | Extreme control on execution contexts | Portability of code to other Arduino compatible board |
| JSON response | Handled via cJSON (well maintained) | Handled via ArduinoJson (wrapper of cJSON) |
| MQTT features | <ul><li>Well maintained library</li><li>QoS 0,1,2 on publish and subscribe</li><li>Ability to have multiple client</li><li>MQTT over Websocket and SSL/TLS</li><li>MQTT 5 also available</li></ul> | <ul><li>Unmaintained library [^1]</li><li>QoS 0 on publish and QoS 0, 1 on subscribe</li><li>MQTT 3 only</li><li>MQTT over Websocket and SSL/TLS not available</li><li>Work on board of different manufacturers</li></ul> |
| GPIO pin handling | Same as Arduino, but more control on interrupt ISR and FreeRTOS queue | Easieast to start, you know... ❤️ |
| OTA updates | Great flexibility, but difficult to start without strong motivation | Less flexibility, but easy to get the job done |
| Memory footprint | <ul><li>RAM 27.1 KB</li><li>Flash 701.4 KB [^2]</li></ul> | <ul><li>RAM 34.6 KB</li><li>Flash 637.8 KB</li></ul> |
| RTOS | Yes, you see clearly how it works [^3] | Yes, but hidden |
| Hardware configurations | Easy via the [menu config](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html) (like _STM32CubeMX_, less graphic though) | You have to explicitly write the code to configure the peripherals/timers you need |
| Feelings | Faster 🚀 | Just works ⭐ |

[^1]: Still relevant in performance and reliability for general use cases.
[^2]: I know that can be less than Arduino, but I'm still a beginner with the official IDF. Forgive me.
[^3]: **This [Youtube channel](https://www.youtube.com/watch?v=EegFVATVvJU) can be your starting point in RTOS (you'll never come back then).**

## Shelly options

- Useful [installation video](https://www.youtube.com/watch?v=-i3d_4FLR0k) for Shelly's relays.
- How Shelly handle MQTT connections in the [official doc](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Mqtt#mqtt-control).

![Screenshot (31)](https://github.com/TIT8/shelly_button_esp32/assets/68781644/e6de6e83-4aeb-428b-a845-5be89e2eb7bd)

## Why don't you use Bluetooth?

Yea, it will be far less power consuming (two devices that talk via BLE, instead of two + broker + router via WiFi). But Shelly cannot receive command via Bluetooth, it sends only informations.

## Adventure with speech recognition

You can make it with _I2S MIC + ESP-EYE_ on Edge Impulse for example. I tried with different boards, take a look [here](https://github.com/TIT8/shelly_button_esp32_arduino/tree/master/speech_recognition) for offline recognition and [here](https://github.com/TIT8/BLE-sensor_PDM-microphone) for online.

<br>
