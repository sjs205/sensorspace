
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#ifdef RASPBERRYPI
#include <bcm2835.h>
#endif

#include "log.h"
#include "ssdisplay.h"



static void write_display_val(struct display *d, uint16_t val, bool cascaded) {
#ifdef RASPBERRYPI
  uint8_t i;

  bcm2835_gpio_write(d->rclk_pin, LOW);

  for (i = 0; i <= 15; i++) {
    bcm2835_gpio_write(d->sclk_pin, LOW);

    /* setup bit */
    bcm2835_gpio_write(d->dio_pin, 0x01 & (val >> i));

    /* clk change */
    bcm2835_gpio_write(d->sclk_pin, HIGH);

    bcm2835_gpio_write(d->dio_pin, LOW);
  }

  if (!cascaded) {
    bcm2835_gpio_write(d->rclk_pin, HIGH);
  }
#endif

  return;
}

void write_display(struct display *d) {
  uint8_t i, j;
  /* set pointer to first display */
  struct display *d_cas = d;
  if (d_cas) {
    for (i = 0; i <= 3; i++) {
      /* write digit i for each display */
      for (j = 0; j <= MAX_CASCADED_DISPLAY && d_cas; j++) {
        /* ensure cascade bit clear on last display */
        write_display_val(d_cas, d_cas->digit[i], (bool)(d_cas->cascaded));
        d_cas = d_cas->cascaded;
#if 0
        log_stderr(DEBUG, "display=%d digit=%d", j, i);
#endif
      }
      /* reset point to first display */
      d_cas = d;
    }

  }
  return;
}

int set_display_str(struct display *d, char *s) {
  uint8_t i;
  uint8_t idx = 0;

  for (i = 0; i < 4; i++) {
    /* set display digit */
    switch (s[idx++]) {
      case '0':
        d->digit[i] = DISP_0;
        break;

      case '1':
        d->digit[i] = DISP_1;
        break;

      case '2':
        d->digit[i] = DISP_2;
        break;

      case '3':
        d->digit[i] = DISP_3;
        break;

      case '4':
        d->digit[i] = DISP_4;
        break;

      case '5':
        d->digit[i] = DISP_5;
        break;

      case '6':
        d->digit[i] = DISP_6;
        break;

      case '7':
        d->digit[i] = DISP_7;
        break;

      case '8':
        d->digit[i] = DISP_8;
        break;

      case '9':
        d->digit[i] = DISP_9;
        break;

      case 'a':
      case 'A':
        d->digit[i] = DISP_A;
        break;

      case 'b':
      case 'B':
        d->digit[i] = DISP_B;
        break;

      case 'c':
      case 'C':
        d->digit[i] = DISP_C;
        break;

      case 'd':
      case 'D':
        d->digit[i] = DISP_D;
        break;

      case 'e':
      case 'E':
        d->digit[i] = DISP_E;
        break;

      case 'f':
      case 'F':
        d->digit[i] = DISP_F;
        break;

      case ' ':
        d->digit[i] = DISP_BLANK ;
        break;

      case '.':
        d->digit[--i] -= DISP_DP;
        break;
    }

    switch (i) {
      case 0:
        d->digit[i] |= DIGIT_1;
        break;
      case 1:
        d->digit[i] |= DIGIT_2;
        break;
      case 2:
        d->digit[i] |= DIGIT_3;
        break;
      case 3:
        d->digit[i] |= DIGIT_4;
        break;
      default:
        break;
    }

  }

  return 0;
}

/* brief Function to initialise memory shared with display process */
int init_display_mem(struct display **d, uint8_t cdisplays) {
  int i;

  struct display *disp = NULL;

  /* create initial display */
  disp = mmap(NULL, sizeof(struct display), PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
  if (!disp) {
    printf("ERROR: Failed to intialise shared memory.\n");
    return -1;
  }

  *d = disp;

  for (i = 0; i < cdisplays; i++) {
  /* initialise shared memory */
    disp->cascaded = mmap(NULL, sizeof(struct display), PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    disp = disp->cascaded;
    if (!disp) {
      printf("ERROR: Failed to intialise shared memory.\n");
      return -1;
    }
  }

  return 0;
}

void set_display_pins(struct display *d, uint8_t sclk, uint8_t rclk, uint8_t dio) {

  if (d->cascaded) {
    set_display_pins(d->cascaded, sclk, rclk, dio);
  }
  d->sclk_pin = sclk;
  d->rclk_pin = rclk;
  d->dio_pin = dio;

  return;
}

int display_init(struct display *d) {

#ifdef RASPBERRYPI
  if (!bcm2835_init()) {
    return 1;
  }

  /* set initial states */
  bcm2835_gpio_fsel(d->sclk_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(d->rclk_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_fsel(d->dio_pin, BCM2835_GPIO_FSEL_OUTP);
  bcm2835_gpio_write(d->sclk_pin, LOW);
  bcm2835_gpio_write(d->rclk_pin, LOW);
  bcm2835_gpio_write(d->dio_pin,  LOW);
#else
  log_stderr(LOG_WARN, "STUB 7-SEG DISPLAY: Initialising display");
#endif

  /* fork display process */
  if (fork() == 0) {
    /* parent */
    return 0;
  } else {
    while (1) {
      write_display(d);
    }
  }
  return 1;
}

void free_display(struct display *d) {
  if (d->cascaded) {
    /* free shared memory */
    free_display(d->cascaded);
  }
  munmap(d, sizeof(struct display));

  return;
}

