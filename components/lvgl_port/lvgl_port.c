/*********************
 *      INCLUDES
 *********************/
#include "lvgl_port.h"
#include <stdbool.h>
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "driver/gpio.h"

/*********************
 *      FONTS
 *********************/
// #include "bit_cell_8.h"
#include "public_pixel.h"

/*********************
 *      DEFINES
 *********************/
#define MY_DISP_HOR_RES    192
#define MY_DISP_VER_RES    96
#define LV_TICK_PERIOD_MS 10

#define BTN_GPIO GPIO_NUM_0

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
static lv_style_t base_btn_style;
static lv_style_t base_label_style;
static lv_style_t battery_voltage_style;
static lv_style_t battery_charge_style;
static lv_style_t scrollbar_style;
static lv_style_t selected_btn_style;

static lv_indev_drv_t indev_encoder;

static const int btn_width = 160;
static const int btn_height = 24;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void init_styles() {
    // scrollbar_style
    lv_style_init(&scrollbar_style);
    lv_style_set_radius(&scrollbar_style, 0);                     // Радиус скругления
    lv_style_set_width(&scrollbar_style, 8);
    lv_style_set_height(&scrollbar_style, 8);
    lv_style_set_pad_top(&scrollbar_style, 20); 
    lv_style_set_bg_color(&scrollbar_style, lv_color_white());
    // lv_obj_clear_flag(&scrollbar_style, LV_OBJ_FLAG_SCROLL_ONE);
    // scrollbar_style

    // base_btn_style
    lv_style_init(&base_btn_style);
    lv_style_set_text_color(&base_btn_style, lv_color_black());  // Цвет текста
    lv_style_set_radius(&base_btn_style, 0);                     // Радиус скругления
    lv_style_set_text_font(&base_btn_style, &lv_font_unscii_8);  // Шрифт текста
    lv_style_set_text_align(&base_btn_style, LV_TEXT_ALIGN_CENTER);
    // base_btn_style

    // selected_btn_style
    lv_style_set_border_color(&selected_btn_style, lv_color_black());
    lv_style_set_border_width(&selected_btn_style, 2);
    lv_style_init(&selected_btn_style);
    lv_style_set_text_color(&selected_btn_style, lv_color_black());  // Цвет текста
    lv_style_set_radius(&selected_btn_style, 0);                     // Радиус скругления
    lv_style_set_text_font(&selected_btn_style, &lv_font_unscii_8);  // Шрифт текста
    lv_style_set_text_align(&selected_btn_style, LV_TEXT_ALIGN_CENTER);
    // selected_btn_style

    // base_label_style
    lv_style_init(&base_label_style);
    lv_style_set_text_color(&base_label_style, lv_color_white());  // Цвет текста
    lv_style_set_radius(&base_label_style, 0);                     // Радиус скругления
    lv_style_set_text_font(&base_label_style, &lv_font_unscii_8);  // Шрифт текста
    lv_style_set_text_align(&base_label_style, LV_TEXT_ALIGN_CENTER);
    // base_label_style
    
    // battery_voltage_style
    lv_style_init(&battery_voltage_style);
    lv_style_set_text_color(&battery_voltage_style, lv_color_white());  // Цвет текста
    lv_style_set_radius(&battery_voltage_style, 0);                     // Радиус скругления
    lv_style_set_text_font(&battery_voltage_style, &lv_font_unscii_8);  // Шрифт текста
    lv_style_set_text_align(&battery_voltage_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_letter_space(&battery_charge_style, 2);
    // battery_voltage_style

    // battery_charge_style
    lv_style_init(&battery_charge_style);
    lv_style_set_text_color(&battery_charge_style, lv_color_white());  // Цвет текста
    lv_style_set_radius(&battery_charge_style, 0);                     // Радиус скругления
    lv_style_set_text_font(&battery_charge_style, &lv_font_unscii_8);  // Шрифт текста
    lv_style_set_text_align(&battery_charge_style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_letter_space(&battery_charge_style, 0);
    // battery_charge_style

    
    lv_style_set_width(&base_btn_style, btn_width);
    lv_style_set_height(&base_btn_style, btn_height);
}

void encoder_read(lv_indev_drv_t * drv, lv_indev_data_t * data){
  data->enc_diff = 0;

  if(gpio_get_level(BTN_GPIO) == 0) data->state = LV_INDEV_STATE_PRESSED;
  else data->state = LV_INDEV_STATE_RELEASED;
}

void init_ui_elements(void) {
    gpio_reset_pin(BTN_GPIO);
    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);

    lv_obj_set_scrollbar_mode(lv_scr_act(), LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), LV_STATE_DEFAULT);    // Установка цвета фона
    lv_obj_add_style(lv_scr_act(), &scrollbar_style, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    
    /**********************
     *    INPUT DEVICE
     **********************/
    
    lv_indev_drv_init(&indev_encoder);
    indev_encoder.read_cb = encoder_read;
    indev_encoder.type = LV_INDEV_TYPE_ENCODER;
    lv_indev_t * my_indev = lv_indev_drv_register(&indev_encoder);

    /**********************
     *      ELEMENTS
     **********************/
    lv_obj_t * bat_chrg = lv_label_create(lv_scr_act());
    lv_obj_t * bat_volt = lv_label_create(lv_scr_act());
    lv_obj_t * menu_mode = lv_label_create(lv_scr_act());

    lv_obj_t * btn1 = lv_btn_create(lv_scr_act());
    lv_obj_t * btn2 = lv_btn_create(lv_scr_act());
    lv_obj_t * btn3 = lv_btn_create(lv_scr_act());

    lv_obj_t * label1 = lv_label_create(btn1);
    lv_obj_t * label2 = lv_label_create(btn2);
    lv_obj_t * label3 = lv_label_create(btn3);

    lv_label_set_text(label1, "Типы БПЛА");
    lv_label_set_text(label2, "NOTIFICATIONS");
    // lv_label_set_text(label3, "DISPLAY SETTINGS");

    lv_label_set_text(bat_chrg, "###");
    lv_label_set_text(bat_volt, "3.72");
    lv_label_set_text(menu_mode, "Settings");   

    /**********************
     *      GROUPS
     **********************/
    lv_group_t * group0 = lv_group_create();
    lv_group_add_obj(group0, btn1);
    lv_group_add_obj(group0, btn2);
    lv_group_add_obj(group0, btn3);

    lv_indev_set_group(my_indev, group0);

    /**********************
     *      STYLES
     **********************/
    lv_obj_add_style(btn1, &base_btn_style, LV_STATE_DEFAULT);
    lv_obj_add_style(btn2, &base_btn_style, LV_STATE_DEFAULT);
    lv_obj_add_style(btn3, &base_btn_style, LV_STATE_DEFAULT);

    lv_obj_add_style(btn1, &selected_btn_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn2, &selected_btn_style, LV_STATE_FOCUSED);
    lv_obj_add_style(btn3, &selected_btn_style, LV_STATE_FOCUSED);
    
    // lv_obj_add_style(btn4, &base_btn_style, LV_STATE_DEFAULT);

    lv_obj_add_style(bat_chrg, &battery_charge_style, LV_STATE_DEFAULT);
    lv_obj_add_style(bat_volt, &battery_voltage_style, LV_STATE_DEFAULT);
    lv_obj_add_style(menu_mode, &base_label_style, LV_STATE_DEFAULT);

    /**********************
     *      LAYOUT
     **********************/

    // lv_obj_align(btn1, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_pos(btn1, 0, btn_height);
    lv_obj_set_pos(btn2, 0, btn_height * 2 + 1);
    lv_obj_set_pos(btn3, 0, btn_height * 3 + 2);

    lv_obj_set_pos(bat_chrg, MY_DISP_HOR_RES - 27, 0);
    lv_obj_set_pos(bat_volt, MY_DISP_HOR_RES - 60, 0);
    lv_obj_set_pos(menu_mode, 0, 0);
}

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

SemaphoreHandle_t xGuiSemaphore;

static void guiTask(void *pvParameter) {
    xGuiSemaphore = xSemaphoreCreateMutex();
    lv_init();
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    static lv_disp_draw_buf_t draw_buf_dsc_1;

    static lv_color_t buf_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];                          /*A buffer for 10 rows*/

    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * MY_DISP_VER_RES);   /*Initialize the display buffer*/

    printf("buffer init\n");

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

    /* Create and start a periodic timer interrupt to call lv_tick_inc */
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer; 
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000));

    init_styles();
    init_ui_elements();

    int cnt = 0;

    while (1) {
        /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
        vTaskDelay(pdMS_TO_TICKS(10));
        // ++cnt;
        // if(cnt == 250) lv_obj_clean(lv_scr_act());;

        // if(cnt == 600) init_ui_elements();
        /* Try to take the semaphore, call lvgl related function on success */
        if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY)) {
            lv_task_handler();
            xSemaphoreGive(xGuiSemaphore);
       }
    }
}

void lv_port_init(void)
{
    xTaskCreatePinnedToCore(guiTask, "gui", 4096*2, NULL, 0, NULL, 1);    
}

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

        bool string[MY_DISP_HOR_RES] = {false};
        bool even = false;

        printf("\u2554");
        for(int i = 0; i < MY_DISP_HOR_RES + 2; ++i) printf("\u2550");
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
        for(int i = 0; i < MY_DISP_HOR_RES + 2; ++i) printf("\u2550");
        printf("\u255D\n");
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_disp_flush_ready(disp_drv);
}