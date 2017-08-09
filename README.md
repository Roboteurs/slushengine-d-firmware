# SlushEngine Mode D: IO Controller

This repo contains the code that drives the IO controller on the SlushEngine Model D. The controller is an MSP430 with no firmware protection. The controller can be reprogrammed for custom use. The IO controller has the following features

  - 8 Digital Output
  - 4 Analog/Digital Inputs
  - 10-Bit ADC. 
  - ADC capable of 0.8Khz sampling. This is limited by the Pi and Pythong

### Issues

  - ADC readings sometimes returns 0 (0.3%) this is not an issue with the ADC but with the firmware or Energia.
