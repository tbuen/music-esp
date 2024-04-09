#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>

#include "config.h"
#include "message.h"
#include "con.h"

// defines

// types

typedef struct {
    con_t con;
    con_mode_t mode;
    int sockfd;
} connection_t;

// function prototypes

// local variables

static const char *TAG = "audio.con";
static SemaphoreHandle_t mutex;
static connection_t connection[MAX_CLIENT_CONNECTIONS];
static con_t next_con = 1;

// public functions

void con_init(void) {
    mutex = xSemaphoreCreateMutex();
}

void con_create(con_mode_t mode, int sockfd) {
    bool created = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < sizeof(connection)/sizeof(connection_t); ++i) {
            if (!connection[i].con) {
                connection[i].con = next_con++;
                connection[i].mode = mode;
                connection[i].sockfd = sockfd;
                ESP_LOGI(TAG, "create con %d mode %d socket %d", connection[i].con, connection[i].mode, connection[i].sockfd);
                msg_send_value(MSG_CON, CON_CONNECTED);
                created = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    if (!created) {
        ESP_LOGE(TAG, "error creating connection");
    }
}

void con_delete(int sockfd) {
    bool deleted = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < sizeof(connection)/sizeof(connection_t); ++i) {
            if (connection[i].con && connection[i].sockfd == sockfd) {
                ESP_LOGI(TAG, "delete con %d socket %d", connection[i].con, connection[i].sockfd);
                memset(&connection[i], 0, sizeof(connection_t));
                msg_send_value(MSG_CON, CON_DISCONNECTED);
                deleted = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    if (!deleted) {
        ESP_LOGW(TAG, "error deleting connection");
    }
}

size_t con_count(void) {
    size_t count = 0;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < sizeof(connection)/sizeof(connection_t); ++i) {
            if (connection[i].con) {
                count++;
            }
        }
        xSemaphoreGive(mutex);
    }
    ESP_LOGI(TAG, "count %d", count);
    return count;
}

bool con_get_con(int sockfd, con_t *con) {
    bool found = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < sizeof(connection)/sizeof(connection_t); ++i) {
            if (connection[i].sockfd == sockfd) {
                *con = connection[i].con;
                found = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    return found;
}

bool con_get_sock(con_t con, int *sockfd) {
    bool found = false;
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        for (int i = 0; i < sizeof(connection)/sizeof(connection_t); ++i) {
            if (connection[i].con == con) {
                *sockfd = connection[i].sockfd;
                found = true;
                break;
            }
        }
        xSemaphoreGive(mutex);
    }
    return found;
}

// local functions
