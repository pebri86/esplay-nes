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
static void lv_tick_task(void);
static int get_menu_selected();
static lv_res_t list_release_action(lv_obj_t * btn);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_indev_t * indev;
static int selected = -1;
char * lines;

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
	//init_rom_list();
	lv_theme_t *th = lv_theme_material_init(200, NULL);
	lv_theme_set_current(th);

	lv_obj_t * scr = lv_page_create(NULL, NULL);
	lv_scr_load(scr);

	lv_obj_t * label = lv_label_create(scr, NULL); /*First parameters (scr) is the parent*/
	lv_label_set_text(label, "Choose ROM");  /*Set the text*/
	lv_obj_align(label, scr, LV_ALIGN_IN_TOP_MID, 0, 0);

	/****************
	* ADD A BATTERY ICON
	****************/
	lv_obj_t * battery = lv_label_create(scr, NULL); /*First parameters (scr) is the parent*/
	lv_label_set_text(battery, SYMBOL_BATTERY_3); /*Set the text*/
	lv_obj_align(battery, label, LV_ALIGN_IN_TOP_RIGHT, (LV_HOR_RES - lv_obj_get_width(label)-10)/2, 0);

	/****************
	* ADD A CLOCK
	****************/
	lv_obj_t * clock = lv_label_create(scr, NULL); /*First parameters (scr) is the parent*/
	lv_label_set_text(clock, "15:00"); /*Set the text*/
	lv_obj_align(clock, label, LV_ALIGN_IN_TOP_LEFT, ((lv_obj_get_width(label)-LV_HOR_RES)/2)+5, 0);

	/*Crate the list*/
	lv_obj_t * list1 = lv_list_create(scr, NULL);
	lv_obj_set_size(list1, LV_HOR_RES, (LV_VER_RES-lv_obj_get_height(label)));
	//lv_list_set_style(list1, LV_LIST_STYLE_BG, &lv_style_transp_tight);
    lv_list_set_style(list1, LV_LIST_STYLE_SCRL, &lv_style_transp_tight);
	lv_obj_align(list1, label, LV_ALIGN_IN_TOP_LEFT, ((lv_obj_get_width(label)-LV_HOR_RES)/2), 10);

	/*Add list elements*/
	lv_list_add(list1, NULL, "1942", list_release_action);
	lv_list_add(list1, NULL, "Contra", list_release_action);
	lv_list_add(list1, NULL, "Duck Tales", list_release_action);
	lv_list_add(list1, NULL, "Duck Tales 2", list_release_action);
	lv_list_add(list1, NULL, "Tetris", list_release_action);

	lv_group_t *group = lv_group_create();
	lv_group_add_obj(group, list1);

	lv_indev_set_group(indev, group);
}

int ui_choose_rom()
{
	ui_create();
	while(1)
	{
		vTaskDelay(10);
		lv_task_handler();
		if(get_menu_selected() != -1)
		{
			return get_menu_selected();
		}
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
	if (strcmp("1942",label) == 0)
		selected = 0;
	else if (strcmp("Contra",label) == 0)
		selected = 1;
	else if (strcmp("Duck Tales",label) == 0)
		selected = 2;
	else if (strcmp("Duck Tales 2",label) == 0)
		selected = 3;
	else if (strcmp("Tetris",label) == 0)
		selected = 4;
	else
		selected = -1;

	return LV_RES_OK;
}
static void init_rom_list() {
	char* romdata;
	const esp_partition_t* part;
	spi_flash_mmap_handle_t hrom;
	esp_err_t err;
	nvs_flash_init();
	part=esp_partition_find_first(0x40, 1, NULL);
	if(part==0) strcpy(lines, "No Rom List Found\n*");
	err=esp_partition_mmap(part, 0, 4*1024, SPI_FLASH_MMAP_DATA, (const void**)&romdata, &hrom);
	if (err!=ESP_OK) {
		strcpy(lines, "No Rom List Found\n*");
	}
	for(int i=0; i < 1000; i++){
		if(romdata[i] == '*'){
			lines=calloc(i+5, sizeof(char));
			break;
		}
	}

	for(int i=0; i < 1000; i++){
		lines[i]=romdata[i];
		//if(romdata[i]=='\n') lineCounter+=1;
		if(romdata[i] == '*'){
			break;
		}
	}
}

static void lv_tick_task(void)
{
	lv_tick_inc(portTICK_RATE_MS);
}

static int get_menu_selected()
{
	return selected;
}
