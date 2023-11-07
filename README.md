# Native ESP code for Shelly's toggle button via MQTT
❗ [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) version of the Arduino repo: https://github.com/TIT8/shelly_button_esp32_arduino

## Description

A push button is connected to the [ESP32](https://github.com/espressif/arduino-esp32) microcontroller. When its state changes, it will trigger an MQTT publish which will toggle the light controlled by a [`Shelly plus 1 relay`](https://www.shelly.com/en-it/products/product-overview/shelly-plus-1). 

❗Keep in mind that you should either use a capacitor (better) or providing some delay in the code to debounce the push button and filter out spurious changes. Pull down the push button via a 10k resistor, **if you don't set the pull-up mode on the input pin**.

<br>

<p align="center"><img src="https://github.com/TIT8/shelly_button_esp32/assets/68781644/708438ba-4cfb-46ab-8b4e-c0fcf803dfa8" alt="Schematich" width='300' /></p>

<br>

- For testing I use the [Hive MQ public broker](https://www.hivemq.com/mqtt/public-mqtt-broker/).

- For production I use [Mosquitto](https://mosquitto.org/) from a Docker container inside my local environment (see the [Docker compose file](https://github.com/TIT8/shelly_button_esp32/blob/master/compose.yaml)).


## Prerequisities

- Look at the [PlatformIO documentation](https://docs.platformio.org/en/stable/tutorials/espressif32/espidf_debugging_unit_testing_analysis.html) to start.
- If you already have the ESP-IDF installed (via Platformio or not), all the dependencies come with it, so you won't import nothing. Simply build and upload 💪.


## ESP-IDF vs Arduino


## Shelly options

![Screenshot (31)](https://github.com/TIT8/shelly_button_esp32/assets/68781644/e6de6e83-4aeb-428b-a845-5be89e2eb7bd)
