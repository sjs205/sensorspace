#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <bcm2835.h>

#include "ssdisplay.h"

#define PIN_SCLK    RPI_V2_GPIO_P1_36
#define PIN_RCLK    RPI_V2_GPIO_P1_38
#define PIN_DIO     RPI_V2_GPIO_P1_37

int main(int argc, char **argv)
{
  struct display *d;

  /* initialisation */
  init_display_mem(&d, 2);

  set_display_pins(d, PIN_SCLK, PIN_RCLK, PIN_DIO);
  //cd = d->cascaded;

  if (!display_init(d)) {
    /* parent */
    for (;;) {
      delay(1000);
      set_display_str(d, (char *)"23.15");
      set_display_str(d->cascaded, (char *)"100.9");
      set_display_str(d->cascaded->cascaded, (char *)"135.9");

      /*d->cascaded->digit[0] = DIGIT_1 | DISP_8 | DISP_DP;
      d->cascaded->digit[1] = DIGIT_2 | DISP_E | DISP_DP;
      d->cascaded->digit[2] = DIGIT_3 | DISP_E | DISP_DP;
      d->cascaded->digit[3] = DIGIT_4 | DISP_A | DISP_DP;
      */

      delay(1000);
    }
    free_display(d);
  }

  return 0;
}

