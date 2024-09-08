#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "lvgl_port.h"

#define BTN_GPIO GPIO_NUM_0

extern "C" void app_main(void)
{
    printf("Hello, Tester!\n");
    // init_gui();
    lv_port_disp_init();

    printf("lv_port_disp_init\n");


    

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
