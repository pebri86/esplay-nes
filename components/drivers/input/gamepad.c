#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include <esp_log.h>
#include "sdkconfig.h"
#include "gamepad.h"

static volatile bool input_task_is_running = false;
static volatile input_gamepad_state gamepad_state;
static input_gamepad_state previous_gamepad_state;
static uint8_t debounce[GAMEPAD_INPUT_MAX];
static volatile bool input_gamepad_initialized = false;
static SemaphoreHandle_t xSemaphore;

input_gamepad_state gamepad_input_read_raw()
{
	input_gamepad_state state = {0};

	state.values[GAMEPAD_INPUT_UP] = !(gpio_get_level(UP));
    state.values[GAMEPAD_INPUT_DOWN] = !(gpio_get_level(DOWN));
    state.values[GAMEPAD_INPUT_LEFT] = !(gpio_get_level(LEFT));
    state.values[GAMEPAD_INPUT_RIGHT] = !(gpio_get_level(RIGHT));

	state.values[GAMEPAD_INPUT_SELECT] = !(gpio_get_level(SELECT));
    state.values[GAMEPAD_INPUT_START] = !(gpio_get_level(START));

    state.values[GAMEPAD_INPUT_A] = !(gpio_get_level(A));
    state.values[GAMEPAD_INPUT_B] = !(gpio_get_level(B));

    return state;
}

static void input_task(void *arg)
{
    input_task_is_running = true;

    // Initialize state
    for(int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
    {
        debounce[i] = 0xff;
    }

    while(input_task_is_running)
    {
        // Shift current values
        for(int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
		{
			debounce[i] <<= 1;
		}


        // Read hardware
#if 1

        input_gamepad_state state = gamepad_input_read_raw();

#else

        input_gamepad_state state = {0};

        state.values[GAMEPAD_INPUT_UP] = !(gpio_get_level(UP));
        state.values[GAMEPAD_INPUT_DOWN] = !(gpio_get_level(DOWN));
        state.values[GAMEPAD_INPUT_LEFT] = !(gpio_get_level(LEFT));
        state.values[GAMEPAD_INPUT_RIGHT] = !(gpio_get_level(RIGHT));

	    state.values[GAMEPAD_INPUT_SELECT] = !(gpio_get_level(SELECT));
        state.values[GAMEPAD_INPUT_START] = !(gpio_get_level(START));

        state.values[GAMEPAD_INPUT_A] = !(gpio_get_level(A));
        state.values[GAMEPAD_INPUT_B] = !(gpio_get_level(B));

#endif

        // Debounce
        xSemaphoreTake(xSemaphore, portMAX_DELAY);

        for(int i = 0; i < GAMEPAD_INPUT_MAX; ++i)
		{
            debounce[i] |= state.values[i] ? 1 : 0;
            uint8_t val = debounce[i] & 0x03; //0x0f;
            switch (val) {
                case 0x00:
                    gamepad_state.values[i] = 0;
                    break;

                case 0x03: //0x0f:
                    gamepad_state.values[i] = 1;
                    break;

                default:
                    // ignore
                    break;
            }

            //gamepad_state.values[i] = (debounce[i] == 0xff);
            //printf("odroid_input_task: %d=%d (raw=%d)\n", i, gamepad_state.values[i], state.values[i]);
		}

        previous_gamepad_state = gamepad_state;

        xSemaphoreGive(xSemaphore);


        // delay
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    input_gamepad_initialized = false;

    vSemaphoreDelete(xSemaphore);

    // Remove the task from scheduler
    vTaskDelete(NULL);

    // Never return
    while (1) { vTaskDelay(1);}
}

void gamepad_read(input_gamepad_state* out_state)
{
	if (!input_gamepad_initialized) abort();

    xSemaphoreTake(xSemaphore, portMAX_DELAY);

    *out_state = gamepad_state;

    xSemaphoreGive(xSemaphore);
}

void gamepad_init()
{
    xSemaphore = xSemaphoreCreateMutex();

    if(xSemaphore == NULL)
    {
        printf("xSemaphoreCreateMutex failed.\n");
        abort();
    }

	//Configure button
	gpio_config_t btn_config;
	btn_config.intr_type = GPIO_INTR_ANYEDGE;        //Enable interrupt on both rising and falling edges
	btn_config.mode = GPIO_MODE_INPUT;               //Set as Input
	btn_config.pin_bit_mask = (uint64_t)             //Bitmask
		((uint64_t) 1 << A) | ((uint64_t) 1 << B) | ((uint64_t) 1 << START) | ((uint64_t) 1 << SELECT) |
		((uint64_t) 1 << UP) | ((uint64_t) 1 << DOWN) | ((uint64_t) 1 << LEFT) | ((uint64_t) 1 << RIGHT);

	btn_config.pull_up_en = GPIO_PULLUP_ENABLE;      //Disable pullup
	btn_config.pull_down_en = GPIO_PULLDOWN_DISABLE; //Enable pulldown
	gpio_config(&btn_config);

    input_gamepad_initialized = true;

    // Start background polling
    xTaskCreatePinnedToCore(&input_task, "input_task", 1024 * 2, NULL, 5, NULL, 1);

  	printf("input_gamepad_init done.\n");
}

void input_gamepad_terminate()
{
    if (!input_gamepad_initialized) abort();

    input_task_is_running = false;
}
