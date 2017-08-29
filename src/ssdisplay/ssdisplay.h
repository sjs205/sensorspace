#include <stdint.h>

#define DIGIT_1      0x1000
#define DIGIT_2      0x2000
#define DIGIT_3      0x4000
#define DIGIT_4      0x8000
#define DISP_DP      0x01
#define DISP_0       0b00000011
#define DISP_1       0b10011111
#define DISP_2       0b00100101
#define DISP_3       0b00001101
#define DISP_4       0b10011001
#define DISP_5       0b01001001
#define DISP_6       0b01000001
#define DISP_7       0b00011111
#define DISP_8       0b00000001
#define DISP_9       0b00001001
#define DISP_A       0b00010001
#define DISP_B       0b11000001
#define DISP_C       0b01100011
#define DISP_D       0b10000101
#define DISP_E       0b01100001
#define DISP_F       0b01110001
#define DISP_BLANK   0b11111111

#define MAX_CASCADED_DISPLAY    5

struct display {
  uint8_t sclk_pin;
  uint8_t rclk_pin;
  uint8_t dio_pin;
  uint16_t digit[4];

  struct display *cascaded;
};

void write_display(struct display *d);
int set_display_str(struct display *d, char *s);
int init_display_mem(struct display **d, uint8_t cdisplays);
void set_display_pins(struct display *d, uint8_t sclk, uint8_t rclk, uint8_t dio);
int display_init(struct display *d);
void free_display(struct display *d);
