#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SEVEN_SEG_A  47
#define SEVEN_SEG_B  21
#define SEVEN_SEG_C  6
#define SEVEN_SEG_D  5
#define SEVEN_SEG_E  4
#define SEVEN_SEG_F  48
#define SEVEN_SEG_G  45
#define SEVEN_SEG_DOT  7

#define BAR_1   18
#define BAR_2   8
#define BAR_3   3
#define BAR_4   46
#define BAR_5   9
#define BAR_6   10
#define BAR_7   11
#define BAR_8   12
#define BAR_9   13
#define BAR_10  14

#define BUTTON  0

int digits [10][8] = {
    {0,1,1,1,1,1,1,0}, // digit 0
    {0,0,1,1,0,0,0,0}, // digit 1
    {0,1,1,0,1,1,0,1}, // digit 2
    {0,1,1,1,1,0,0,1}, // digit 3
    {0,0,1,1,0,0,1,1}, // digit 4
    {0,1,0,1,1,0,1,1}, // digit 5
    {0,1,0,1,1,1,1,1}, // digit 6
    {0,1,1,1,0,0,0,0}, // digit 7
    {0,1,1,1,1,1,1,1}, // digit 8
    {0,1,1,1,1,0,1,1}  // digit 9
    };

int bars [10] = {
    BAR_1,
    BAR_2,
    BAR_3,
    BAR_4,
    BAR_5,
    BAR_6,
    BAR_7,
    BAR_8,
    BAR_9,
    BAR_10,
};

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    int pinNumber = (int)args;
    //xQueueSendFromISR(interputQueue, &pinNumber, NULL);
    int level = gpio_get_level(pinNumber);
    gpio_set_level(SEVEN_SEG_DOT, level);
}

void app_main(void)
{
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON, GPIO_INTR_ANYEDGE);
    gpio_set_level(SEVEN_SEG_DOT, 1); // DOT off

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON, gpio_interrupt_handler, (void *)BUTTON);

    gpio_set_direction(SEVEN_SEG_DOT, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_G, GPIO_MODE_OUTPUT);

    for (int bar=0; bar<10; bar++) {
        gpio_set_direction(bars[bar], GPIO_MODE_OUTPUT);
    }

    int count = 0;
    while (true) {
        printf("%d\n", count);
        //if (digits[count][0]) gpio_set_level(SEVEN_SEG_DOT, 0); else gpio_set_level(SEVEN_SEG_DOT, 1);
        if (digits[count][1]) gpio_set_level(SEVEN_SEG_A, 0); else gpio_set_level(SEVEN_SEG_A, 1);
        if (digits[count][2]) gpio_set_level(SEVEN_SEG_B, 0); else gpio_set_level(SEVEN_SEG_B, 1);
        if (digits[count][3]) gpio_set_level(SEVEN_SEG_C, 0); else gpio_set_level(SEVEN_SEG_C, 1);
        if (digits[count][4]) gpio_set_level(SEVEN_SEG_D, 0); else gpio_set_level(SEVEN_SEG_D, 1);
        if (digits[count][5]) gpio_set_level(SEVEN_SEG_E, 0); else gpio_set_level(SEVEN_SEG_E, 1);
        if (digits[count][6]) gpio_set_level(SEVEN_SEG_F, 0); else gpio_set_level(SEVEN_SEG_F, 1);
        if (digits[count][7]) gpio_set_level(SEVEN_SEG_G, 0); else gpio_set_level(SEVEN_SEG_G, 1);

        for (int bar=0; bar<10; bar++) {
             gpio_set_level(bars[bar], (count==bar)? 0:1);
        }

        if (count++ >= 9)
            count = 0;

        vTaskDelay(1000/portTICK_PERIOD_MS);
    }
}
