#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lvgl_port.h"

extern "C" void app_main(void)
{
    printf("Hello, Tester!\n");
    // init_gui();
    lv_port_disp_init();

    printf("lv_port_disp_init\n");

    while (1)
    {
        vTaskDelay(portMAX_DELAY);
    }
}
