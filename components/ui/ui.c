/**
 * @file ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_freertos_hooks.h"
#include "esp_event_loop.h"
#include "esp_partition.h"
#include "nvs_flash.h"
#include "ui.h"
#include "display.h"
#include "st7735r.h"
#include "gamepad.h"
#include "display.h"
#include "settings.h"
#include "string.h"

/*********************
 *      DEFINES
 *********************/
#define TAG_DEBUG "DEBUG"
#define TAG_INFO  "INFO"

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void init_rom_list();
static void IRAM_ATTR lv_tick_task(void);
static int get_menu_selected();
static lv_res_t list_release_action(lv_obj_t * btn);
static lv_obj_t * create_header();
static void create_list_page(lv_obj_t * parent);
static lv_res_t slider_action(lv_obj_t *slider);
static lv_res_t btn_click_action(lv_obj_t *btn);
static void create_settings_page(lv_obj_t *parent);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_t * indev;
static lv_group_t *group;
static int selected = -1;
static int brightness_value;

char title[14][100]; /*max rom = 14, max title char = 100*/
int line_max = 0;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void ui_init()
{
    lv_init();
    lv_disp_drv_t disp;
    lv_disp_drv_init(&disp);
    disp.disp_flush = st7735r_flush;
    lv_disp_drv_register(&disp);
    esp_register_freertos_tick_hook(lv_tick_task);

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read = lv_keypad_read;
    indev = lv_indev_drv_register(&indev_drv);
    lv_indev_init();

    ESP_LOGI(TAG_DEBUG, "(%s) RAM left %d", __func__ , esp_get_free_heap_size());
}

void ui_create(void)
{
    init_rom_list();

    lv_obj_t * scr = lv_page_create(NULL, NULL);
    lv_scr_load(scr);

    /*create group for keypad input*/
    group = lv_group_create();
    lv_indev_set_group(indev, group);

    lv_obj_t * header = create_header();

    static lv_style_t style_tv_btn_bg;
    lv_style_copy(&style_tv_btn_bg, &lv_style_plain);
    style_tv_btn_bg.body.main_color = LV_COLOR_HEX(0x487fb7);
    style_tv_btn_bg.body.grad_color = LV_COLOR_HEX(0x487fb7);
    style_tv_btn_bg.body.padding.ver = 0;

    static lv_style_t style_tv_btn_rel;
    lv_style_copy(&style_tv_btn_rel, &lv_style_btn_rel);
    style_tv_btn_rel.body.empty = 1;
    style_tv_btn_rel.body.border.width = 0;

    static lv_style_t style_tv_btn_pr;
    lv_style_copy(&style_tv_btn_pr, &lv_style_btn_pr);
    style_tv_btn_pr.body.radius = 0;
    style_tv_btn_pr.body.opa = LV_OPA_50;
    style_tv_btn_pr.body.main_color = LV_COLOR_WHITE;
    style_tv_btn_pr.body.grad_color = LV_COLOR_WHITE;
    style_tv_btn_pr.body.border.width = 0;
    style_tv_btn_pr.text.color = LV_COLOR_GRAY;

    lv_obj_t *tv = lv_tabview_create(scr, NULL);
    lv_obj_align(tv, header, LV_ALIGN_IN_TOP_LEFT, ((lv_obj_get_width(header)-LV_HOR_RES)/2), 10);

    lv_obj_t *tab1 = lv_tabview_add_tab(tv, "Play");
    lv_obj_t *tab2 = lv_tabview_add_tab(tv, "Settings");

    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BG, &style_tv_btn_bg);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_BG, &style_tv_btn_bg);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_INDIC, &lv_style_plain);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_REL, &style_tv_btn_rel);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_PR, &style_tv_btn_pr);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_REL, &style_tv_btn_rel);
    lv_tabview_set_style(tv, LV_TABVIEW_STYLE_BTN_TGL_PR, &style_tv_btn_pr);

    lv_group_add_obj(group, tv);

    create_list_page(tab1);
    create_settings_page(tab2);
}

int ui_choose_rom()
{
    ui_create();
    while(1)
    {
        lv_task_handler();
        if(get_menu_selected() != -1)
        {
            return get_menu_selected();
        }
        vTaskDelay(100/portTICK_PERIOD_MS);
    }

    return 0;
}

void ui_deinit()
{
    lv_obj_del(lv_scr_act());
    esp_deregister_freertos_tick_hook(lv_tick_task);
    ESP_LOGI(TAG_DEBUG, "(%s) RAM left %d", __func__ , esp_get_free_heap_size());
}
/**********************
 *   STATIC FUNCTIONS
 **********************/
static lv_res_t list_release_action(lv_obj_t * btn)
{
    const char * label;
    label = lv_list_get_btn_text(btn);
    for(int i=0; i < line_max; i++){
        if (strcmp(title[i],label) == 0)
            selected = i;
    }

    return LV_RES_OK;
}
static void init_rom_list() {
    char* romdata;
    const esp_partition_t* part;
    spi_flash_mmap_handle_t hrom;
    esp_err_t err;
    part=esp_partition_find_first(0x40, 1, NULL);
    printf("%s: Partition part not found\n", __func__);
    err=esp_partition_mmap(part, 0, 4*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
    if (err!=ESP_OK) {        
        printf("%s: ROM lists not found\n", __func__);
    }

    int line_counter = 0;
    int x = 0;
    for(int i=0; i < 1000; i++){
        if (romdata[i] == '\n')
        {
            line_counter++;
            x = 0;
        }
        else
        {
            if(romdata[i] == '*'){
                break;
            }

            title[line_counter][x] = romdata[i];
            x++;
        }

        line_max = line_counter;
    }
}

static void IRAM_ATTR lv_tick_task(void)
{
    lv_tick_inc(portTICK_RATE_MS);
}

static int get_menu_selected()
{
    return selected;
}

static lv_obj_t * create_header()
{
    lv_obj_t * header = lv_label_create(lv_scr_act(), NULL); /*First parameters (scr) is the parent*/
    lv_label_set_text(header, "MicroNES");  /*Set the text*/
    lv_obj_align(header, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 0);

    /****************
    * ADD A BATTERY ICON
    ****************/
    lv_obj_t * battery = lv_label_create(lv_scr_act(), NULL); /*First parameters (scr) is the parent*/
    lv_label_set_text(battery, SYMBOL_BATTERY_3); /*Set the text*/
    lv_obj_align(battery, header, LV_ALIGN_IN_TOP_RIGHT, (LV_HOR_RES - lv_obj_get_width(header)-10)/2, 0);

    /****************
    * ADD A CLOCK
    ****************/
    lv_obj_t * clock = lv_label_create(lv_scr_act(), NULL); /*First parameters (scr) is the parent*/
    lv_label_set_text(clock, "15:00"); /*Set the text*/
    lv_obj_align(clock, header, LV_ALIGN_IN_TOP_LEFT, ((lv_obj_get_width(header)-LV_HOR_RES)/2)+5, 0);

    return header;
}

static void create_list_page(lv_obj_t * parent)
{
    lv_page_set_style(parent, LV_PAGE_STYLE_BG, &lv_style_transp_fit);
    lv_page_set_style(parent, LV_PAGE_STYLE_SCRL, &lv_style_transp_fit);

    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
    lv_page_set_sb_mode(parent, LV_SB_MODE_OFF);

    /*Crate the list*/
    lv_obj_t * list1 = lv_list_create(parent, NULL);
    lv_obj_set_size(list1, LV_HOR_RES, LV_VER_RES - 30);
    lv_list_set_style(list1, LV_LIST_STYLE_BG, &lv_style_transp_tight);
    lv_list_set_style(list1, LV_LIST_STYLE_SCRL, &lv_style_transp_tight);
    lv_obj_align(list1, parent, LV_ALIGN_IN_TOP_LEFT, ((lv_obj_get_width(parent)-LV_HOR_RES)/2), 0);

    /*Add list elements*/
    for(int i=0; i < line_max; i++){
        lv_list_add(list1, NULL, title[i], list_release_action);
        printf("%s\n", title[i]);
    }

    lv_group_add_obj(group, list1);
}

static void create_settings_page(lv_obj_t *parent)
{
    lv_page_set_style(parent, LV_PAGE_STYLE_BG, &lv_style_transp_fit);
    lv_page_set_style(parent, LV_PAGE_STYLE_SCRL, &lv_style_transp_fit);

    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
    lv_page_set_sb_mode(parent, LV_SB_MODE_OFF);

    static lv_style_t style_txt;
    lv_style_copy(&style_txt, &lv_style_plain);
    style_txt.text.color = LV_COLOR_WHITE;

    /*Create slider brightness label*/
    lv_obj_t * label_brightness = lv_label_create(parent, NULL); /*First parameters (scr) is the parent*/
    lv_obj_set_style(label_brightness, &style_txt); 
    lv_label_set_text(label_brightness, "Brightness");  /*Set the text*/
    lv_obj_align(label_brightness, parent, LV_ALIGN_IN_TOP_MID, 0, 10);

    /*Create a bar, an indicator and a knob style*/
    static lv_style_t style_bar;
    static lv_style_t style_indic;
    static lv_style_t style_knob;

    lv_style_copy(&style_bar, &lv_style_pretty);
    style_bar.body.main_color =  LV_COLOR_BLACK;
    style_bar.body.grad_color =  LV_COLOR_GRAY;
    style_bar.body.radius = LV_RADIUS_CIRCLE;
    style_bar.body.border.color = LV_COLOR_WHITE;
    style_bar.body.opa = LV_OPA_60;
    style_bar.body.padding.hor = 0;
    style_bar.body.padding.ver = LV_DPI / 10;

    lv_style_copy(&style_indic, &lv_style_pretty);
    style_indic.body.grad_color =  LV_COLOR_RED;
    style_indic.body.main_color =  LV_COLOR_WHITE;
    style_indic.body.radius = LV_RADIUS_CIRCLE;
    style_indic.body.shadow.width = LV_DPI / 10;
    style_indic.body.shadow.color = LV_COLOR_RED;
    style_indic.body.padding.hor = LV_DPI / 30;
    style_indic.body.padding.ver = LV_DPI / 30;

    lv_style_copy(&style_knob, &lv_style_pretty);
    style_knob.body.radius = LV_RADIUS_CIRCLE;
    style_knob.body.opa = LV_OPA_70;

    /*Create a second slider*/
    lv_obj_t *slider = lv_slider_create(parent, NULL);
    lv_slider_set_style(slider, LV_SLIDER_STYLE_BG, &style_bar);
    lv_slider_set_style(slider, LV_SLIDER_STYLE_INDIC, &style_indic);
    lv_slider_set_style(slider, LV_SLIDER_STYLE_KNOB, &style_knob);
    lv_obj_set_size(slider, LV_HOR_RES-30, LV_DPI / 3);
    lv_obj_align(slider, label_brightness, LV_ALIGN_OUT_BOTTOM_MID, 0, 10); /*Align to below the chart*/
    lv_slider_set_action(slider, slider_action);
    lv_slider_set_range(slider, 1, 10);
    int16_t value = (int16_t) (get_backlight_settings() / 10);
    lv_slider_set_value(slider, value);
    slider_action(slider);          /*Simulate a user value set the refresh the chart*/

    /*Create a save button*/
    lv_obj_t * btn1 = lv_btn_create(parent, NULL);
    lv_cont_set_fit(btn1, true, true); /*Enable resizing horizontally and vertically*/
    lv_obj_align(btn1, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, (LV_VER_RES - slider->coords.y2 - lv_obj_get_height(btn1)) / 2);
    lv_obj_set_free_num(btn1, 1);   /*Set a unique number for the button*/
    lv_btn_set_action(btn1, LV_BTN_ACTION_CLICK, btn_click_action);

    /*Add a label to the button*/
    lv_obj_t * btn1_label = lv_label_create(btn1, NULL);
    lv_label_set_text(btn1_label, "Save");

    lv_group_add_obj(group, slider);
    lv_group_add_obj(group, btn1);
}

/**
 * Called when a new value on the slider on the Chart tab is set
 * @param slider pointer to the slider
 * @return LV_RES_OK because the slider is not deleted in the function
 */
static lv_res_t slider_action(lv_obj_t *slider)
{
    int16_t v = lv_slider_get_value(slider);
    brightness_value = (int)(v * 10);

    //set display brightness in percent;
    set_display_brightness(brightness_value);

    return LV_RES_OK;
}

static lv_res_t btn_click_action(lv_obj_t *btn)
{
    //save display brightness settings;
    set_backlight_settings((int32_t) brightness_value);

    lv_obj_t * mbox1 = lv_mbox_create(lv_scr_act(), NULL);
    lv_mbox_set_text(mbox1, "Settings saved!");
    lv_mbox_start_auto_close(mbox1, 1000);

    return LV_RES_OK;
}