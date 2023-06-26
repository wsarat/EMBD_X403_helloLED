#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_task_wdt.h>

// log
#include "esp_log.h"

// queue
#include "freertos/queue.h"
QueueHandle_t interruptQueue;

// sleep
#include "esp_pm.h"
#include "esp_sleep.h"

TaskHandle_t ledTaskHandle = NULL;
static const char *MAIN = "mainTesk";
static const char *LEDTAG = "ledTesk";

// define pin

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
    xQueueSendFromISR(interruptQueue, &pinNumber, NULL);
}

// enter deep sleep in microseconds
void sleep_enter( uint64_t sleep_ms ){

    if( sleep_ms < 100 * 1000 ){
        sleep_ms = 100 * 1000; // minimum 100ms sleep
    }
    
    // disable wifi and bluetoth, release the locks
    //deinit_wifi();
    // wait for shutdown of Wifi, necessary!
    vTaskDelay( 500 / portTICK_PERIOD_MS );
    // global in RTC memory
    //gettimeofday( &sleep_enter_time, NULL ); // best way to get system time

    printf("Entering sleep, %lld msec", sleep_ms/1000 );
    //uart_wait_tx_idle_polling( CONFIG_ESP_CONSOLE_UART_NUM ); // UART flush

    // NOW enter sleep mode
    esp_sleep_enable_timer_wakeup( sleep_ms );
    // allow ULP to wakeup processor
    esp_sleep_enable_ulp_wakeup();  
    esp_deep_sleep_start();    
    // does not return
}

// RTC RAM
RTC_NOINIT_ATTR uint32_t count;

void write7Seg(int number) {
    if (number > 9)
        number = 0;
        
    ESP_LOGI(LEDTAG, "7Segment: %d", (int)number);

    if (digits[number][1]) gpio_set_level(SEVEN_SEG_A, 0); else gpio_set_level(SEVEN_SEG_A, 1);
    if (digits[number][2]) gpio_set_level(SEVEN_SEG_B, 0); else gpio_set_level(SEVEN_SEG_B, 1);
    if (digits[number][3]) gpio_set_level(SEVEN_SEG_C, 0); else gpio_set_level(SEVEN_SEG_C, 1);
    if (digits[number][4]) gpio_set_level(SEVEN_SEG_D, 0); else gpio_set_level(SEVEN_SEG_D, 1);
    if (digits[number][5]) gpio_set_level(SEVEN_SEG_E, 0); else gpio_set_level(SEVEN_SEG_E, 1);
    if (digits[number][6]) gpio_set_level(SEVEN_SEG_F, 0); else gpio_set_level(SEVEN_SEG_F, 1);
    if (digits[number][7]) gpio_set_level(SEVEN_SEG_G, 0); else gpio_set_level(SEVEN_SEG_G, 1);    
}

void ledTask(void *args) {
    ESP_LOGI(LEDTAG, "Start");

    // init 7seg pin
    gpio_set_direction(SEVEN_SEG_DOT, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_A, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_B, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_C, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_D, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_E, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_F, GPIO_MODE_OUTPUT);
    gpio_set_direction(SEVEN_SEG_G, GPIO_MODE_OUTPUT);

    // init ledbar pin
    for (int bar=0; bar<10; bar++) {
        gpio_set_direction(bars[bar], GPIO_MODE_OUTPUT);
    }

    while (true) {
        if (count%10 == 0)
            write7Seg(count/10);

        for (int bar=0; bar<10; bar++) {
             gpio_set_level(bars[bar], ((count%10)>=bar)? 0:1);
        }

        if (count++ >= 99) 
            count = 0;

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    esp_err_t err;

    //
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    printf("Wakeup %d\n", wakeup_cause);
    if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER 
        || wakeup_cause == ESP_SLEEP_WAKEUP_EXT0 ) {
        printf("Wakeup cause => %d\n", wakeup_cause);
        printf("count from RAM = %d\n", (int)count);
    } else {
        // init
        count = 0;
    }

    // init sleep
    esp_pm_config_t pm_config = {
        .max_freq_mhz = 240, // Wifi min is 80MHz
        .min_freq_mhz = 10, // set to crystal frequency, 40MHz or divided by integer, min 10 MHz
        .light_sleep_enable = false, // deep sleep only
    };
    //pm_config.light_sleep_enable = true;

    if(( err = esp_pm_configure( &pm_config )) != ESP_OK ){
       printf("power management failed: %s", esp_err_to_name( err ));
    }       

    // allow deep sleep wakeup with button
    esp_sleep_enable_ext0_wakeup(BUTTON, false ); // false = wakeup on low
    //esp_sleep_enable_ext1_wakeup(BUTTON, false ); // false = wakeup on low
    // must then keep GPIO pin on
    esp_sleep_pd_config( ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON );

    // init pin 0 (button) interrupt
    gpio_set_direction(BUTTON, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(BUTTON, GPIO_INTR_ANYEDGE); //GPIO_INTR_ANYEDGE GPIO_INTR_LOW_LEVEL
    gpio_set_level(SEVEN_SEG_DOT, 1); // DOT off

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON, gpio_interrupt_handler, (void *)BUTTON);

    interruptQueue = xQueueCreate(10, sizeof(int));
    if (interruptQueue == 0) {
        ESP_LOGE(MAIN, "Failed to create interruptQueue = %p", interruptQueue);
    }

    xTaskCreate(ledTask, "LED_Task", 4096, NULL, 10, &ledTaskHandle);

    vTaskDelay(12000/portTICK_PERIOD_MS);
    ESP_LOGI(MAIN, "Deep sleep!");
        sleep_enter( 6000 * 1000 );

    int pinNumber, pinLevel;
    while (true){
        if (xQueueReceive(interruptQueue, &pinNumber, 0)) {
            pinLevel = gpio_get_level(pinNumber);
            gpio_set_level(SEVEN_SEG_DOT, pinLevel);
            ESP_LOGE(MAIN, "GPIO %d was %s", pinNumber, (pinLevel)?"Up":"Down");
        }

        vTaskDelay(100/portTICK_PERIOD_MS);
    }
}
