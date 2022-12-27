#include "driver/gpio.h"
#include "esp_log.h"

#include "message.h"
#include "button.h"

#define TASK_CORE     1
#define TASK_PRIO     1
#define STACK_SIZE 4096

#define BUTTON_PIN     GPIO_NUM_22
#define BUTTON_PRESSED 0
#define BUTTON_DELAY_S 1

static void button_task(void *param);

static const char *TAG = "button";

static TaskHandle_t handle;
static QueueHandle_t queue;

void button_init(QueueHandle_t q) {
    if (handle) return;

    queue = q;

    gpio_config_t io_conf = {};

    io_conf.pin_bit_mask = BIT64(BUTTON_PIN);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    if (xTaskCreatePinnedToCore(&button_task, "button-task", STACK_SIZE, NULL, TASK_PRIO, &handle, TASK_CORE) != pdPASS) {
        ESP_LOGE(TAG, "could not create task");
    }
}

static void button_task(void *param) {
    uint8_t button_cnt = 0;

    for (;;) {
        if (gpio_get_level(BUTTON_PIN) == BUTTON_PRESSED) {
            if (button_cnt < BUTTON_DELAY_S * 100) {
                button_cnt++;
                if (button_cnt == BUTTON_DELAY_S * 100) {
                    ESP_LOGI(TAG, "BUTTON pressed!");
                    message_t msg = { BASE_BUTTON, EVENT_BUTTON_PRESSED, NULL };
                    xQueueSendToBack(queue, &msg, 0);
                }
            }
        } else {
            button_cnt = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
