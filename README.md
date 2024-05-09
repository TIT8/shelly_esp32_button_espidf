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

## Difference with branch V2?

On the master branch, I'm using [task notifications](https://www.freertos.org/RTOS_Task_Notification_As_Binary_Semaphore.html) from FreeRTOS to unblock a high-priority (relative to [core 1](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-guides/performance/speed.html#choosing-application-task-priorities)) task responsible for debouncing in the software, despite the use of interrupts (which are disabled and enabled within that task). Therefore, when the button is not in use, the task remains blocked on a type of semaphore, conserving CPU resources. This allows the idle task to run uninterrupted until the button is pressed, ensuring the watchdog timer is consistently reset.

<ins>The main thread is more efficient than the one in the V2 branch</ins> (which employs [busy wait](https://github.com/TIT8/shelly_esp32_button_espidf/tree/v2?tab=readme-ov-file#why-busy-wait-the-button-status-and-not-polling-it-or-be-notified-by-interrupts) for monitoring button status instead of being notified by interrupts).   
In the main task, interrupts are enabled and disabled only on the GPIO pin used and [on the core](https://github.com/TIT8/shelly_esp32_button_espidf/blob/1ccbeb450f857612228f9be3fbcbf5ec52a6ecf3/src/main.c#L190) where the task is pinned, akin to a critical section, as outlined in the [official documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/freertos_idf.html#disabling-interrupts). [^4]

The code handles the scenario where the button is continuously pressed; after a while, the main thread will enter a blocked state. You can refer to [the relevant part of the firmware](https://github.com/TIT8/shelly_esp32_button_espidf/blob/88ec38896c9c9199dd878153d043701d34ed38f2/src/main.c#L265) for more details.   

Obviously, putting the ESP32 to sleep until it's awakened by the GPIO would be much more efficient. However, the latency from button press to waking the CPU, connecting to WiFi and MQTT, and sending commands will decrease the responsiveness of the application (on the ESP32, you cannot put the CPU into deep sleep without turning off the radio).

[^4]: Also a useful [topic](https://esp32.com/viewtopic.php?t=35019).

## Shelly options

- Useful [installation video](https://www.youtube.com/watch?v=-i3d_4FLR0k) for Shelly's relays.
- How Shelly handle MQTT connections in the [official doc](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Mqtt#mqtt-control).

![Screenshot (31)](https://github.com/TIT8/shelly_button_esp32/assets/68781644/e6de6e83-4aeb-428b-a845-5be89e2eb7bd)

## Why don't you use Bluetooth?

Yea, it will be far less power consuming (two devices that talk via BLE, instead of two + broker + router via WiFi). But Shelly cannot receive command via Bluetooth, it sends only informations.

## Adventure with speech recognition

You can make it with _I2S MIC + ESP-EYE_ on Edge Impulse for example. I tried with different boards, take a look [here](https://github.com/TIT8/shelly_button_esp32_arduino/tree/master/speech_recognition) for offline recognition and [here](https://github.com/TIT8/BLE-sensor_PDM-microphone) for online.

<br>
