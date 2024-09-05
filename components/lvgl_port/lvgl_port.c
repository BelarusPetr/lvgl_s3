/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lvgl_port.h"
#include <stdbool.h>
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES    128
#define MY_DISP_VER_RES    64

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush_to_console(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p);
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//        const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_tick_task(void*) {
    while(1) {
        lv_tick_inc(10);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

SemaphoreHandle_t xGuiSemaphore;

void lv_port_disp_init(void)
{
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    static lv_disp_draw_buf_t draw_buf_dsc_1;

    static lv_color_t buf_1[MY_DISP_HOR_RES * 64];                          /*A buffer for 10 rows*/

    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * 64);   /*Initialize the display buffer*/

    printf("buffer init\n");

    // /* Example for 2) */
    // static lv_disp_draw_buf_t draw_buf_dsc_2;
    // static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];                        /*A buffer for 10 rows*/
    // static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];                        /*An other buffer for 10 rows*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * 10);   /*Initialize the display buffer*/

    // /* Example for 3) also set disp_drv.full_refresh = 1 below*/
    // static lv_disp_draw_buf_t draw_buf_dsc_3;
    // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
    // lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
    //                       MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/

    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv;                         /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);                    /*Basic initialization*/
    printf("drv init\n");
    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush_to_console;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_1;

    /*Required for Example 3)*/
    disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    //disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
    printf("drv reg\n");

    lv_style_t style;
    lv_style_init(&style);

    lv_style_set_text_color(&style, lv_color_white());
    lv_style_set_bg_color(&style, lv_color_black());

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_t * btn2 = lv_btn_create(lv_scr_act());

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_obj_t * label2 = lv_label_create(btn2);
        
    lv_label_set_text(label1, "Btn1");
    lv_label_set_text(label2, "Btn2");

    lv_obj_add_style(btn1, &style, LV_STATE_DEFAULT);
    lv_obj_add_style(btn2, &style, LV_STATE_DEFAULT);

    lv_obj_align(btn1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_align(btn2, LV_ALIGN_RIGHT_MID, 0, 0);

    printf("label set\n");

    xTaskCreatePinnedToCore(lv_tick_task, "lv_tick", 1024, NULL, 0, NULL, 1);

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_tick_inc(10);
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }
    
}

static const char *lt[] = {
    /* 0 */ "  ",
    /* 1 */ "\u2580 ",
    /* 2 */ " \u2580",
    /* 3 */ "\u2580\u2580",
    /* 4 */ "\u2584 ",
    /* 5 */ "\u2588 ",
    /* 6 */ "\u2584\u2580",
    /* 7 */ "\u2588\u2580",
    /* 8 */ " \u2584",
    /* 9 */ "\u2580\u2584",
    /* 10 */ " \u2588",
    /* 11 */ "\u2580\u2588",
    /* 12 */ "\u2584\u2584",
    /* 13 */ "\u2588\u2584",
    /* 14 */ "\u2584\u2588",
    /* 15 */ "\u2588\u2588",
};

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    /*You code here*/
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush_to_console(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p)
{
    if(disp_flush_enabled) {
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/

        int32_t x;
        int32_t y;

        bool string[128] = {false};
        bool even = false;

        printf("\u2554");
        for(int i = 0; i < 128 + 2; ++i) printf("\u2550");
        printf("\u2557\n");


        for(y = area->y1; y <= area->y2; y++) {
                if(!even) printf("\u2551 ");
            for(x = area->x1; x <= area->x2; x++) {
                if(even) string[x] = (bool)color_p->full;
                else {
                    if(string[x] == 0 && color_p->full == 0) printf(" ");
                    else if(string[x] == 1 && color_p->full == 0) printf("\u2580");
                    else if(string[x] == 0 && color_p->full == 1) printf("\u2584");
                    else if(string[x] == 1 && color_p->full == 1) printf("\u2588");
                    string[x] = (bool)color_p->full;
                }
                color_p++;
            }
            if(!even) printf(" \u2551\n");
            even = !even;
        }
        printf("\u255A");
        for(int i = 0; i < 128 + 2; ++i) printf("\u2550");
        printf("\u255D\n");
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
//static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                    const lv_area_t * fill_area, lv_color_t color)
//{
//    /*It's an example code which should be done by your GPU*/
//    int32_t x, y;
//    dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//    for(y = fill_area->y1; y <= fill_area->y2; y++) {
//        for(x = fill_area->x1; x <= fill_area->x2; x++) {
//            dest_buf[x] = color;
//        }
//        dest_buf+=dest_width;    /*Go to the next line*/
//    }
//}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
