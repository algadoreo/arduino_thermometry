# Thermometry with Arduinos

A cheap yet effective method for thermometry for the SPIDER and BIT experiments.

We use Arduino DUE boards because of the cost-per-channel of their 12-bit ADCs, and because each has two independent communication channels.

## Overview

Thermometry does not typically require high sampling rates, since we don’t anticipate rapid temperature variations in the Earth’s atmosphere. However, the noise (error) on the signal is the same whether we sample at 5 or 500 Hz, since the samples come from the same underlying distribution. This reduces the effective bit depth of the ADC. A common technique to improve the signal-to-noise ratio (SNR), or the dynamic range, is to _oversample_: the signal – temperature in our case – is first sampled at a much higher rate, a moving average (boxcar) filter is applied, and finally, the filtered timestream is read out at the lower rate.

We find that we can achieve a dynamic range of 85 dB (SNR = 20,000) using this technique.

## Setup

The basic setup has one Arduino DUE device connected to the main computer via a serial connection, while the Arduino DUE devices themselves communicate via two-wire I<sup>2</sup>C connections. Each device has the exact same code, with a boolean `isDev0` flag defaulted to `false`. Upon receipt of a serial command, this flag flips to `true` – indicating the device is Device 0 – and a message chain is initiated.

The Arduino DUE, unlike other models, has two separate I<sup>2</sup>C ports, allowing two independent channels of communication. Because only one of these ports (`SDA` and `SCL`; pins 20 and 21) come equipped with pull-up resistors, that will be designated as the WRITE port. The other port (`SDA1` and `SCL1`; pins 70 and 71) will be designated as the READ port.

(Should pull-up resistors be desired for the READ port, connect the two lines in parallel to the Arduino’s 3.3 V output through 4.7 kΩ resistors.)

Once the message chain has completed its circuit – i.e., readings have been taken from every device and returned to Device 0 – it is sent back to the main computer by Device 0 via serial, and the `isDev0` flag reverts to `false`. The cycle begins anew.

![Arduino chain setup](https://github.com/algadoreo/arduino_thermometry/img/arduino_due_chain.png)
