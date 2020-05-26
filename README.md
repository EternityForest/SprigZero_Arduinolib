# Sprig Zero Arduino libs

Provides support for the Sprig Zero hardware. These libraries are important, they contain the battery charging logic. Without them,
the battery will float at 2.7v(Which will not cause immediate damage but is not ideal),and the charger will draw too much current at voltages above 5v, and may enter a thermal reboot loop.


## Functions:

### startTimer2RTCExt()
Starts the external 32KHz timer on timer2.

### getUnixTime()
Returns a 32-bit seconds count. Uses timer2, works in sleep mode.

### setUnixTime(uint32_t s)
Set the unix clock.  If you are using SG1, you should set the unix time from the SG1 clock before sleeping, and load it after.


### readmV(pin)

Reads an ADC input in millivolts. The accuracy should be much better than 1%, as this performs a calibration using the onboard voltage
reference.

### readVCC(pin)

Reads the *current* Vcc.

### doPowerManagement(sleep)

Adjust the charging current. This needs to be called very soon after boot(within milliseconds ideally), and periodically every few seconds.

It will sleep for the number of seconds specified. If external power is available and the battery is not fully charged, it uses simple delay. Otherwise, it uses true sleep.

Note that because you cannot directly measure the battery, we have to calculate this through a rather complex algorithm that involves many measurements. With sleep=0, it will still take ariound 10ms.

*There must not be any extra loads besides the MCU here*. That would cause a voltage drop accross the battery resistor and reduce accuracy. However, the MCU can still be *sinking* current, and you can have other loads directly connected to the battery(The 10-pin expansion connector gives direct access).  It will also still work correctly if an extenal source is directly charging the battery.

Turn off anything using more than a few uA before calling this(Which you need to do anyway).

Returns the battery voltage in mV. Fully charged is 2575, nearly empty is 2400.


This function should still work correctly when not using a battery. It shoul

*Note that charging current is extremely low, in the 20-5mA range reducing as you get close to fully charged*. It may take several days to ch


## Compiler settings

Use the (MiniCore)[https://github.com/MCUdude/MiniCore] Arduino support package, 4MHz internal, 1.8v BOR, 328PB variant. This uses features that are not available in standard Arduino.



