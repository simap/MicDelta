MicDelta Report
================


# Application Description

Visualize the direction of sounds in 2D. Given an array of microphones, the phase difference of sound between a pair can indicate the source angle.

A small 8x8 LED matrix displays the aproximate relative 2D angle of the strongest audio signal as a dot. The field of view is about +- 60 degrees, more extreme angles displayed on the edge.

Complex audio signals work well, as do sine waves, up to about 1,700Hz. Above 1,700Hz the reliable field of view diminishes.

# Hardware Description

For this project, the STM32F303RET6 was selected for it's 4 independet ADCs, which are capable of simultaneous acquisition. 

Two pairs of microphones create an X and Y axis array. Sound arriving at each microphone will arrive at different times depending on the angle relative to the other microphone in the pair.

Microphone signals pass through a high pass filter to remove DC and shift the voltage to a programmable voltaged controlled by the MCU's DAC. The signals are then input to a Programmable Gain Amplifier (PGA) in the MCU. The PGA also acts as a low pass filter. The PGA gain can be controlled by firmware or bypassed. The DAC bias voltage is adjusted so that after PGA gain is applied the signal will be at roughly the midpoint of the ADC range.

This project shares some similarities with [TapTDOA](https://hackaday.io/project/159821-taptdoa), and a modified version of that project's board was used with the following modifications:

1. Electret microphones were used and 10K bias resistors were added.
2. The analog filters were modified to pass a larger range of frequencies, with the 2nd low pass filter removed.
3. An 8x8 WS2812 / SK6812 8x8 LED matrix display was added and connected to the USB power supply.
4. Microphones are placed about 100mm apart along opposite ends of each axis, attached to the back of the display, facing in the opposite direction. This gives good time delta information up to about 1,700Hz where the delay hits 180 degrees and the direction can't be determined.

## Peripherals

* Timers generate triggers for ADCs (TIM3), and are used for microsecond timekeeping (TIM2).
* Four PGA peripherals are used to boost the microphone signal.
* Four ADCs sample microphone signals from the PGAs and trigger DMA.
* DMA copies data from the ADCs to circular buffers. DMA is also used for the console (ping-pong) and WS2812 driver (one-shot).
* The DAC is used to generate a programmable reference voltage used in combination with the PGA to keep the biased microphone signal within range after amplification.
* A UART is used to generate WS2812 / SK9822 compatible data streams for these addressable (smart) LEDs. A UART is also used for the serial console.


# Software Description

## Functional Description
The firmware takes simultaneous measurements of the ADCs and accumulates a buffer for each. A DC offset adjustment is applied, and incrementally adjusted every cycle. As the microphone signals are fairly small even after a 16X PGA gain, the signals are then scaled up.

Pairs of microphone data is then correlated with the mic opposite it. The location of peak correlation indicates the time delta between the 2 microphones. This is done for the X and Y axes.

When a sufficiently strong correlation is found, a pixel is painted to an internall display buffer. The time delta is used to plot a position within the 8x8 matrix. A position outside of the field of view is drawn at the limit of that axis. The strength is represented as a color from black to red to green to purple for the strongest signals. The dots will fade out, giving a little persistence as detection and display cycle progress.

The display buffer is then sent to the LED matrix.

A debug console is implemented that allows for capturing and displaying waveforms and correlation data for individual microphones or axis pairs. A scanning mode is also available that prints out detection information, pixel drawing coordinates, and the cycle execution time.

## Code Organization and Origins

STM32CubeIDE configuraiton tools were used to generate LL and HAL driver code, including peripheral initialization. This code is licensed by ST and/or Arm though various licenses including: a BSD 3-Clause license, the Apache License Version 2.0, or the Ultimate Liberty license SLA0044.

The STM32CubeIDE generated code is modified as little as possible, with most of the application bootstrapping done in `app.c`. Likewise `app.h` has various configurable settings.

Where possible HAL and LL drivers and constants are used, and direct modification of registers is avoided unless necessary.

Where possible CMSIS DSP library functions are used, and benefit from this MCU's DSP / SIMD instructions.

The console from [Elecia White's reusable repo](https://github.com/eleciawhite/reusable) was used with minor modification. It is in the public domain via the [unlicense](https://unlicense.org/) license.

A few lines of peripheral initialization code and the MCU's IOC configuration file were used from [TapTDOA](https://github.com/simap/taptdoa)'s code, which is licensed under the MIT licese.

The firmware is broken up into several modules. For the purposes of this report, I cover only the modules which I wrote or significatly contributed to:

#### app.c

Sets up the MCU for basic operation, initializes peripherals (when not delegated to another module) and initializes modules. Contains the main loop, which alternates between processing debug console commands and the display visualization process.

### displayVisualization.c

Coordinates sample aquisition and processing via AngleFinders. Draws the visual representation of the detection results to a display buffer, and passes this to the WS2812Driver.

### angleFinder.c

Uses a pair of MicData and correlates signals, finding the time delta and correlation strength.

### micData.c

Snapshots ADC data for processing by unwrapping a circular buffer fed via DMA, removes DC offsets, and scales the signal up. Adjusts the DC offset incrementally.

### ws2812Driver.c

The pixels in the display buffer are mapped according to the pixelmap. Color ordering configuration is applied. Uses a UART and DMA to stream out WS2812 pixel data from a buffer.

### pixelmap.c

Contains helper functions for calculating a pixel offset for both the display buffer and 8x8 matrix "zigzag" layout.

### consoleUart.c

Implements a buffered UART driver for low level syscalls support by implementing `__io_putchar` and `__io_getchar`, as well as a `consoleUartGetChNonBlocking` to support the debug console functions. DMA is used for transmit and receive. Reception is done via a circular buffer, and transmission is done via a ping pong buffer.

### consoleCommands.c

Added commands to perform these actions:

* echo - turn console echo of received characters on/off
* mic - capture, process, and display a microphone's data
* angle - capture, process, and display one axes' pair of microphone and correlation data.
* scan - toggle informational scanning messages for each cycle

# Architecural Overview / Block Diagram

This is also included in the project files as blockdiagram.txt and was made with [https://asciiflow.com/](https://asciiflow.com/)


```

MicDelta - visualize the direction of sounds in 2D
Given an array of microphones, the phase difference
of sound between a pair can indicate the source angle.


  ┌───┐
  │Mic│                                M3
  └───┤   ┌───────┐            ┌───────────────┐
      ├───►Angle X├──┐         │Display        │
  ┌───┤   └───────┘  ├────────►│       ▲Y      │
  │Mic│              │         │       │       │
  └───┘              │         │       │       │
                     │       M1│  ◄────┼────►  │M2
  ┌───┐              │         │       │    X  │
  │Mic│              │         │       │       │
  └───┤   ┌───────┐  │         │       ▼       │
      ├───►Angle Y├──┘         │               │
  ┌───┤   └───────┘            └───────────────┘
  │Mic│                                M4
  └───┘



┌─────────────────────────────────────────────────────────────────────────┐
│   Mic Data Pipeline                                                     │
│                                                     ┌────────────────┐  │
                 Optional bypass                      │Channel DC Value│
                 ┌────────────┐                       └───┬────────────┘
                 │            │                           │
┌───┐   ┌──────┐ │  ┌───┐   ┌─▼─┐   ┌───┐   ┌──────┐  ┌───▼─────────┐
│Mic├───►Filter├─┴──►PGA├───►ADC├───►DMA├───►Buffer├──►Remove offset│
└───┘   └──────┘    └───┘   └─▲─┘   └───┘   └──────┘  └─────┬───────┘
                    2-16x     │                             │
                  Filters too │ Trigger                     │ Out
                              │ Sample                      │
                            ┌─┴──┐                          ▼
                            │TIM3│
│                           └────┘                                       │
│  4X of these                                                           │
└────────────────────────────────────────────────────────────────────────┘

┌───────────────────────────────────────────────────────┐
│   Axis Angle Finder                                   │
│                                                       │
  ┌────────┐
  │Mic Data├────────┐
  └────────┘        │
                    │
               ┌────▼────┐   ┌────────┐    ┌──────────┐
               │Correlate├───►Find max├────►Calc Angle│
               └────▲────┘   └────────┘    └────┬─────┘
                    │                           │
  ┌────────┐        │                           │ Out
  │Mic Data├────────┘                           │
  └────────┘                                    │
│                                               ▼       │
│   2X pair                                             │
└───────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  Display Visualization                                          │
│                       ┌───────────────────┐                     │
  ┌───────┐             │Display Buffer     │
  │Angle X├───────┐     │                   │   ┌────────────┐
  └───────┘       │     │                   ├───►2D Pixel Map│
              ┌───▼─┐   │                   │   └──────┬─────┘
              │Paint├───►                   │          │
              └───▲─┘   │                   │          │
  ┌───────┐       │     │                   │   ┌──────▼──────┐
  │Angle Y├───────┘     │                   │   │WS2812 Driver│
│ └───────┘             └───────────────────┘   └─────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


┌────────────────────────────────────────────────────────────────┐
│  WS2812 Driver                                                 │
│                                                                │

  ┌──────┐   ┌────────────┐   ┌─────────────┐   ┌───┐  ┌────┐
  │Pixels├───►Pack to bits├───►Output Buffer├───►DMA├──►UART│
  └──────┘   └────────────┘   └─────────────┘   └───┘  └─┬──┘
                                                         │
                                                   ┌─────▼─────┐
                                                   │WS2812 LEDs│
│                                                  └───────────┘ │
│                                                                │
└────────────────────────────────────────────────────────────────┘
```

# Building

All STM32CubeIDE files are included. It should be possible to import this project to a workspace and compile using the STM32CubeIDE without any special setup or configuration. The MidDelta.ioc file can be used to generate projects for other build systems should the need arise. The generated source code can be regenerated from the .ioc file.

# Future

Additional detection accuracy work would be needed to make a useful / viable product. The project also lacks a bootloader or firmware update method.

The code has a number of FIXMEs and TODOs for improvement or enhancement, summarized here:

* Enhance visualization
  * A radial gradient using fractional pixel coordinates.
  * Change the shape of the dot/gradient based on the axis relative correlation strength.
  * Track multiple sound sources.
* Performance improvements
  * Phase detection via correlation only works well within a limited range of possible correlation values, and much of the correlation output is not usable. Right now `arm_correlate_q15` is used, but there exist Partial Convolution APIs in the CMSIS DSP library, as well as faster variants that may work under certain conditions. 
  * `q15_t` is used for all signal processing, but `q7_t` may suffice in some areas and could increase throughput via SIMD support on this MCU.
  * A large number of samples at high speed are used to get usable phase difference information, but I suspect with some math this could be significantly reduced by using curve fitting functions and slower sample rates.
  * The consoleUart module could implement `_write` and reduce overhead for multi-byte writes.
  * The MicData module might benefit slightly by using a Mem2Mem DMA transfer instead of memcpy when unwrapping the circular buffer. Alternatively, a one-shot or ping-pong buffer could be used and obviate the need to unwrapp/copy. 
* Analog / sample quality - supply spikes on the scope and phantom signals in ADC samples occurred when samples were taken while the system was busy. This is likely due to poor board analog design and could be fixed in hardware. A workaround in firmware is in place, but reduces throughput slightly.
* DC offset calculation improvements - the current implemenation tends to bounce around 1 LSB.
* Improve microphone hardware - pre amplificiation and blocking sounds behind the micriphone could improve the signal quality and device usefulness.

# Grading - Self Assessment

Note: I pivoted at the last minute so this project does not follow previous assignment deliverable descriptions of the final project. Life and paying client obligations took priority. Not having started on the monumental task of bringing up the impressive and feature laden STM32H745I-DISCO board, I instead opted for a simpler embedded project that I felt was doable within the time remaining.

Criteria  | Score | Explaination
------------- | ------------- | -------------
Project meets minimum project goals | 1 | A button was not added. This hardware does not have any exposed pins I could use, and the bodges in place were so fragile (and kept breaking) I was not willing to risk bodging directly to the MCU to add a button. I was tempted to get a really loud clicky button and trigger an ADC watchdog interrupt via a microphone, that would count, right? The state machines other than the console are not quite up to par. Other project goals were met.
Completeness of deliverables | 2 | The code, project files, report, and demo / walkthrough video include the requested deliverables. 
Clear intentions and working code | 2 | The system appears to work as described.
Reusing code | 3 | Student code is identified, 3rd party licenses documented, and version control shows an evolution of the written code. The code written for this project is release under the MIT license.
Originality and scope of goals | 1 or 3 | I believe this is the bare minimum, but I hope it has originality and novelty!

