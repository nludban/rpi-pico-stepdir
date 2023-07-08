//#include <boards/adafruit_itsybitsy_rp2040.h>

#include "stepdir.pio.h"
#include "psst.pio.h"

#include <stdio.h>
#include <stdlib.h>

#include <hardware/gpio.h>

#include <hardware/pio.h>
#include <hardware/clocks.h>

#include <pico/stdlib.h>

static inline void put_pixel(uint32_t pixel_grb) {
   pio_sm_put_blocking(pio0, /*sm=*/1, pixel_grb << 8u);
}

#if 0
// GRBL for UNO R3 pin assignments
typedef m4x::board_pins::D13	GRBL_A_DirPin;	// (spindle en)
typedef m4x::board_pins::D12	GRBL_A_StepPin;	// (spindle dir)
typedef m4x::board_pins::D11	GRBL_Z_LimitPin;
typedef m4x::board_pins::D10	GRBL_Y_LimitPin;
typedef m4x::board_pins::D9	GRBL_X_LimitPin;
typedef m4x::board_pins::D8	GRBL_nEnablePin;

typedef m4x::board_pins::D7	GRBL_Z_DirPin;
typedef m4x::board_pins::D6	GRBL_Y_DirPin;
typedef m4x::board_pins::D5	GRBL_X_DirPin;
typedef m4x::board_pins::D4	GRBL_Z_StepPin;
typedef m4x::board_pins::D3	GRBL_Y_StepPin;
typedef m4x::board_pins::D2	GRBL_X_StepPin;
#endif

int
main()
{
   setup_default_uart();
   //stdio_init_all();

   PIO pio = pio0;

   // Pins for watchdog:
   int clock_pin = 6;		// GP6  ->  9 (SPI0 SCK)
   int nerror_pin = 7;		// GP7  -> 10 (SPI0 TX)

   // Pins for receive:
   int pulse_step_dir_pins = 14; // and 15.

   int wdog_sm = 0;

   int clock_sm = 2;
   int pulse_sm = 3;

   // 8 instrs
   uint wdog_offset = pio_add_program(pio, &psst_wdog_program);
   psst_wdog_program_init(pio, wdog_sm, wdog_offset,
			  clock_pin,
			  nerror_pin);

   // 4 instrs
   uint clock_offset = pio_add_program(
      pio, &stepdir_clock_to_irq_program);
   stepdir_clock_to_irq_program_init(pio, clock_sm, clock_offset,
				     clock_pin);

   // ~17 instrs
   uint pulse_offset = pio_add_program(
      pio, &stepdir_ext_clock_program);
   stepdir_ext_clock_program_init(pio, pulse_sm, pulse_offset,
				  pulse_step_dir_pins);

# if 0
   const int STEP_FWD_1ms_x10 = ((0x4 << 28)	// 0100
				 | (3125 - 2) << 8
				 | (10 - 1));
   const int STEP_REV_2ms_x05 = ((0xe << 28)	// 1110
				 | (3125 * 2 - 2) << 8
				 | (5 - 1));

   for (;;) {
      psst_wdog_pet(pio, wdog_sm, 8, 100);

      pio_sm_put_blocking(pio, pulse_sm, STEP_FWD_1ms_x10);
      pio_sm_put_blocking(pio, pulse_sm, STEP_REV_2ms_x05);
   }

# else
   const int Hz = 200;		// velocity update rate
   const int T = 1000 / Hz;
   const int VMAX = 32 * 1000;	// pulses/sec
   const int VMIN = -VMAX;
   const int K_ACC = 1000 / T;	// updates from 0 to VMAX
   const int ACC = VMAX / K_ACC;
   const int HOLD = 5000 / T;	// updates to hold VMAX

   // 100us to 5ms.

   int v = 0;
   int a = ACC;
   int h = 0;
   for (;;) {
      psst_wdog_pet(pio, wdog_sm, 8, 100);

      if ((h > 0) && (h == HOLD)) {
	 a = -ACC;
	 h = 0;
      }
      v += a;
      if (v >= VMAX) {
	 a = 0;
	 h++;
      }
      if (v <= VMIN)
	 a = +ACC;

      int repeat = abs(v) * T / 1000;
      int delay = 3125 * T;
      if (repeat > 0)
	 delay /= repeat;

      int flags = 0x0;
      if (v < 0)
	 flags |= 0xa;		// 1010 = dir bits
      if (repeat > 0)
	 flags |= 0x4;		// 0100 = step bit

      if (repeat < 1)
	 repeat = 1;
      int command = ((flags << 28)
		     | ((delay - 2) << 8)
		     | (repeat - 1));

      pio_sm_put_blocking(pio, pulse_sm, command);
   }
# endif

}

/**/
