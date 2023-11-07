#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_timer.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "cJSON.h"


#define CONFIG_BROKER_URL "mqtt://192.168.1.76:1883"
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_NUM_26)

static QueueHandle_t mqtt_evt_queue = NULL;

static const char *TAG = "mqtt";
volatile bool state = 0;
volatile bool connection = 0;


static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        while (!connection && !xQueueSend(mqtt_evt_queue, &client, 0)) {}
        msg_id = esp_mqtt_client_subscribe(client, "shellyplus1-a8032abc70c4/status/switch:0", 2);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        esp_mqtt_client_publish(client, "shellyplus1-a8032abc70c4/command/switch:0", "status_update", 0, 2, 0);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        cJSON *json = cJSON_Parse(event->data);
        if (json != NULL)
        {
            char *rendered = cJSON_PrintUnformatted(json);
            printf("DATA_json=%.*s\r\n", event->data_len, rendered);
            cJSON *output = cJSON_GetObjectItem(json, "output");
            if (output != NULL && cJSON_IsBool(output))
            {
                printf("output=%d\r\n", output->valueint);
                state = output->valueint;
            }
        }
        else
        {
            printf("DATA=%.*s\r\n", event->data_len, event->data);
        }
        cJSON_Delete(json);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;

    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void gpio_task(void *arg)
{
    bool current = 0;
    esp_mqtt_client_handle_t client;
    while (!xQueueReceive(mqtt_evt_queue, &client, 0)) {}
    connection = true;
    for (;;)
    {
        if (!gpio_get_level(GPIO_NUM_26))
        {
            if (!current)
            {
                esp_mqtt_client_publish(client, "shellyplus1-a8032abc70c4/command/switch:0", state ? "off" : "on", 0, 2, 0);
            }
            current = 1;
        }
        else
        {
            current = 0;
        }
        /*
            Debounce with the FreeRTOS delay, as there is no difference with millis(),
            in both cases this task will execute in a time slice set by the scheduler.
            You can also use interrupts, but you will need hardware debouncing (RC filter),
            without using the pullup mode on the input (capacitors do not work at ground).
        */
        vTaskDelay(150 / portTICK_PERIOD_MS);
    }
}

static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_BROKER_URL,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    mqtt_evt_queue = xQueueCreate(10, sizeof(client));
    esp_mqtt_client_start(client);
}

void app_main(void)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
    esp_log_level_set("mqtt_shelly", ESP_LOG_VERBOSE);
    esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("transport", ESP_LOG_VERBOSE);
    esp_log_level_set("outbox", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);
}