#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "../include/liblcd.h"
#include <pthread.h>


//#define sleep(A) do{usleep(A*1000*1000);}while(0);


int main()
{
    char buffer[1024];
    lcd_init();
    lcd_set_mode(LCD_MODE_11x12UNICODE);
    lcd_set_roll_int(500);

    lcd_show_cursor(0x00);
    lcd_set_backlight(01);
    lcd_set_backlight_int(5000);
#if 0
    lcd_cls();
    sleep(1);
    lcd_set_backlight(0x01);
    sleep(1);
    lcd_show_cursor(0x01);
    sleep(1);
    lcd_move_cursor(0, 1);
    sleep(1);
    lcd_move_cursor(8, 1);
    sleep(3);
    lcd_show_cursor(0x00);
    sleep(1);

    lcd_cls();
    sleep(1);
    lcd_draw_string(0,"draw a dot:");
    sleep(1);
    lcd_move_cursor(0, 1);
    sleep(1);

    lcd_cls();
    sleep(1);
    lcd_draw_dot(1,126,61);
    sleep(1);
    lcd_draw_dot(0,126,61);
    sleep(1);

    lcd_draw_string(0,"draw a line:");
    sleep(1);
    lcd_move_cursor(0, 1);
    sleep(1);
    lcd_cls();
    sleep(1);

    lcd_draw_line(1,10,5,50,60);
    sleep(1);
    lcd_draw_line(0,10,5,50,60);
    sleep(1);
    lcd_cls();
    sleep(1);

    lcd_draw_string(0,"draw a circle:");
    sleep(1);
    lcd_move_cursor(0, 1);
    sleep(1);
    lcd_cls();
    sleep(1);

    lcd_draw_circle(1,40,40,15);
    sleep(1);
    lcd_draw_circle(0,40,40,15);
    sleep(1);

    lcd_draw_string(0,"滚屏演示:");
    sleep(1);
    lcd_cls();
    sleep(1);

    lcd_draw_string(0, "button test!");
    sleep(1);
    lcd_set_backlight(0x00);
    sleep(3);

 #endif

 
 #if 1
    int cnt = 0;
    while(1) {
        int roll;
        for(int i=0; i<5; i++) {
        roll = rand()/(RAND_MAX/3);
        sprintf(buffer, "%d 第%d行: roll=%d, 测试good!", cnt, i+1, roll);
//       sprintf(buffer, "%d 第%d行: roll=%d", cnt, i+1, roll);
//        sprintf(buffer, "abcdefghijklmnopqr");
        lcd_draw_linestr(i+1, LCD_DISP_NORMAL, roll, buffer);
        }
        sleep(20);
        cnt++;
    }

    lcd_term();

#endif
}
