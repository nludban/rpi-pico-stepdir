# rpi-pico-stepdir
PIO stepper motor pulse engine for Raspberry Pi pico

## Summary

Two adjacent GPIOs are driven as step and direction,
output is a 50% duty cycle with the rising edge in
the center.
One command type efficiently supports fixed velocity
and allows frequent updates for more complex profiles.
Multiple SMs on the same pico will stay in synch for
coordinated motion control.
An external step clock can be used to keep SMs across
multiple picos in synch.

Project status is "Work In Progress" - PIO program
timings are verified, wrapper API is incomplete and
unstable.

## Command Word

Each command is a 32 bit word written to the TxFIFO:

31:30 second half step+dir
29:28 first half step+dir
27:8 delay count
7:0 repeat count

Note that step+dir order is application defined -
the value in the lower numbered bit will be output
to the lower numbered of the GPIO pins.

Pulse period is `delay+2` step clocks.

The pulse is output `repeat+1` times.

When idle time must be accurate to maintain
synchronization, supply commands where the step
bit does not change between halves.

For very long pulses, use two or more commands
with the step bit changing between commands.


## Performance

Two implementations are provided - one using the
internal clock and cycle counting, one synched to
an external clock source via `wait irq`.
Both are designed for a nominal
(3.125 MHz step clock)[https://github.com/nludban/rpi-pico-psst]
but count at 6.25 MHz - the same delay count twice
and producing a 50% duty cycle.

The internal clock variation requires the SM to
be clocked at 8x the step clock.
Up to four SMs in the same PIO can be used, but
there will be no synchronization between picos.

For an external step clock, the SM clock should be
at least 10x faster than the step clock.
One SM is needed to convert the GPIO edges to
6.25 MHz IRQs, leaving three SMs available to
control steppers.
This configuration can be expanded to synchronize
many picos.

Note that while shared clocks ensure all SMs
run at the same rate, additional care may be
needed to start them all at the same time
within tolerances needed by the application.

Minimum pulse period is less than a microsecond,
maximum period is over 335 milliseconds.
Two commands are sufficient to construct a pulse
with a period over 1 minute, or four commands
if rounding the period is not allowed.

Design goals were to support command rates from
10 Hz to 1 kHz, and step pulse rates up to 32 kHz
(ie, 300 rpm assuming 32 microsteps and 200
steps/rev).
The implementation far exceeds the goals as well
as the useful limits for stepper motor control.
Refer to the source code for details.


## Components and Connections

...


