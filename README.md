# Nixie-Weather-station

![alt text](image.jpg "Result")​

The weather station is based on the indicators IN-12 and IN-15A. Displays indoor temperature, indoor humidity and outdoor temperature. The outside temperature is taken from a remote sensor located outside.
The whole project is made in Proteus 8.10, all the necessary files are available. The firmware was written using the 433_transaver library and Alex Gyver's libraries.
It is possible to display both degrees Celsius and degrees Fahrenheit (by closing the jumper).
In the absence of a signal from the remote sensor, the upper indicator blinks slowly. When the battery voltage is low, the upper indicator flashes quickly, but the temperature is displayed. Above 100 Fahrenheit, no decimal point is displayed.
