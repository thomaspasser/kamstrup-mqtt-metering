# kamstrup-mqtt-metering

A two-piece solution for reading Kamstrup Omnipower meters with an ESP8266, MQTT and Python3.
The new meter firmware from 1st half of 2020 is required.

Repository folders:
- esp8266-mqtt-publisher: ESP8266 firmware for publishing raw, encrypted MBUS frames on MQTT.
- decrypter-mqtt-client: Python3 MQTT client which decrypts the encrypted MBUS frames and publishes the readable data back on MQTT.
