#ifndef __LIBLCD_H__
#define __LIBLCD_H__

#include <stdbool.h>

#define MAX_LINES   (5)

#define LCD_MODE_6x12ASCII      (0x01)
#define LCD_MODE_11x12UNICODE   (0x02)
#define LCD_MODE_8x16ASCII      (0x03)
#define LCD_MODE_16x16UNICODE   (0x04)

#define LCD_DISP_NORMAL         (0)
#define LCD_DISP_INVERSE        (1)
#define LCD_DISP_FLASH          (2)

#define LCD_ROLL_NOROLL         (0)
#define LCD_ROLL_L2R            (1)
#define LCD_ROLL_R2L            (2)

#ifdef __cplusplus
extern "C" {
#endif
void lcd_init();
void lcd_term();

void lcd_set_roll_int(int ms);

void lcd_set_mode(char mode);
void lcd_set_grayscale(char level);
void lcd_set_backlight(bool onoff);
void lcd_show_cursor(bool onoff);

void lcd_cls(void);

void lcd_move_cursor(char cx, char cy);
void lcd_draw_string(bool inverse, char *string);
void lcd_draw_dot(bool onoff, char x,char y);
void lcd_draw_line(bool onoff, char x0, char y0, char x1, char y1);
void lcd_draw_circle(bool onoff, char ox, char oy, char rx);

void lcd_draw_linestr(char cy, char disp, char roll, char *string);
void lcd_erase_linestr(char cy);

int led_set_off(void);
int led_set_red(void);
int led_set_blue(void);
int led_set_red_and_blue(void);
void lcd_set_backlight_int(int ms);
#ifdef __cplusplus
}
#endif
#endif /* __LIBLCD_H__ */
