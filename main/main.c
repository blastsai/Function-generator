#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/dac_oneshot.h"
#include "driver/dac_types.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "soc/soc_caps.h"

#define PULSE_WAVE 0
#define COSINE_WAVE 1
#define TRIANGLE_WAVE 2
#define WAVE_TYPE TRIANGLE_WAVE

static void led_Task(void *args);
static void adc_Task(void *args);

#if WAVE_TYPE == PULSE_WAVE
static void pulse_Wave_Task(void *args);
#elif WAVE_TYPE == COSINE_WAVE
static void cosine_Wave_Task(void *args);
#elif WAVE_TYPE == TRIANGLE_WAVE
static void triangle_Wave_Task(void *args);
#endif

void app_main(void)
{
    // double frequency = configTICK_RATE_HZ;
    double frequency = 1;
#if WAVE_TYPE == PULSE_WAVE
    xTaskCreate(pulse_Wave_Task, "pulse_Wave_Task", 4096, &frequency, 5, NULL);
#elif WAVE_TYPE == COSINE_WAVE
    xTaskCreate(cosine_Wave_Task, "cosine_Wave_Task", 4096, &frequency, 5, NULL);
#elif WAVE_TYPE == TRIANGLE_WAVE
    xTaskCreate(triangle_Wave_Task, "triangle_Wave_Task", 4096, &frequency, 5, NULL);
#endif

    xTaskCreate(led_Task, "led_Task", 4096, &frequency, 5, NULL);

    frequency = 30;
    xTaskCreate(adc_Task, "adc_Task", 4096, &frequency, 5, NULL);
}

#pragma region DAC

#if WAVE_TYPE == PULSE_WAVE
static void pulse_Wave_Task(void *args)
{
    // Cấu hình, GPIO cho DAC
    dac_oneshot_handle_t dac_handle;
    dac_oneshot_config_t dac_config = {
        .chan_id = DAC_CHAN_0,
    };
    dac_oneshot_new_channel(&dac_config, &dac_handle);

    double frequency;
    memcpy(&frequency, args, sizeof(double));
    uint32_t delay = (uint32_t)(pdMS_TO_TICKS(1000.0 / frequency));

    uint8_t output = 0;

    while (1)
    {
        output = ~output;
        dac_oneshot_output_voltage(dac_handle, output);
        // ESP_LOGI("Output", "%d", output);
        vTaskDelay(delay);
    }
}
#endif

#if WAVE_TYPE == COSINE_WAVE
static void cosine_Wave_Task(void *args)
{
    // Cấu hình, GPIO cho DAC
    dac_oneshot_handle_t dac_handle;
    dac_oneshot_config_t dac_config = {
        .chan_id = DAC_CHAN_0,
    };
    dac_oneshot_new_channel(&dac_config, &dac_handle);

    double frequency;
    memcpy(&frequency, args, sizeof(double));
    double period = 1 / frequency;

    uint8_t output = 0;
    double time = 0;
    const float step = 1.0 / configTICK_RATE_HZ;

    while (1)
    {
        output = (uint8_t)(255.0 * sin(6.2832 * frequency * time));
        time += step;
        if (time >= period)
            time = 0;

        dac_oneshot_output_voltage(dac_handle, output);
        // ESP_LOGI("Output", "%d, time = %f", output, time);
        vTaskDelay(1);
    }
}
#endif

#if WAVE_TYPE == TRIANGLE_WAVE

double traingle(double value)
{
    if (value <= 0)
    {
        return 0;
    }
    if (value >= 1)
    {
        return 1;
    }

    if (value < 0.5)
    {
        return value;
    }
    else
    {
        return -2 * value + 2;
    }
}

void triangle_Wave_Task(void *args)
{
    // Cấu hình, GPIO cho DAC
    dac_oneshot_handle_t dac_handle;
    dac_oneshot_config_t dac_config = {
        .chan_id = DAC_CHAN_0,
    };
    dac_oneshot_new_channel(&dac_config, &dac_handle);

    double frequency;
    memcpy(&frequency, args, sizeof(double));
    double period = 1 / frequency;

    uint8_t output = 0;
    double time = 0;
    const double step = 1.0 / configTICK_RATE_HZ;

    while (1)
    {
        output = (uint8_t)(255.0 * traingle(time / period));
        time += step;
        if (time >= period)
            time = 0;

        dac_oneshot_output_voltage(dac_handle, output);
        // ESP_LOGI("Output", "%d, time = %f", output, time);
        vTaskDelay(1);
    }
}

#endif

#pragma endregion DAC

#pragma region LED

static void led_Task(void *args)
{
    gpio_config_t led_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = 1 << GPIO_NUM_2};

    gpio_config(&led_gpio_config);

    double frequency;
    memcpy(&frequency, args, sizeof(double));

    uint32_t delay = (uint32_t)(pdMS_TO_TICKS(1000.0 / frequency));
    uint32_t value = 0;
    while (1)
    {
        value = !value;
        gpio_set_level(GPIO_NUM_2, value);
        // ESP_LOGI("LED", "%ld", value);
        vTaskDelay(delay);
    }
}

#pragma endregion LED

#pragma region ADC

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
void writeDataToFile(int *arr, int length);

static void adc_Task(void *args)
{
    // Cấu hình, GPIO cho chân 9
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_2,
        .ulp_mode = false,
    };
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    double frequency;
    memcpy(&frequency, args, sizeof(double));
    uint32_t delay = (uint32_t)(pdMS_TO_TICKS(1000.0 / frequency));
    ESP_LOGI("ADC", "DelayTick = %ld", delay);

    adc_oneshot_new_unit(&adc_cfg, &adc_handle);
    adc_oneshot_config_channel(adc_handle, ADC_CHANNEL_9, &chan_cfg);

    // Hàm có sẵn để loại bỏ nhiễu, xác định hiệu điện thế
    adc_cali_handle_t adc_cali_handle = NULL;
    adc_calibration_init(ADC_UNIT_2, ADC_CHANNEL_9, ADC_ATTEN_DB_12, &adc_cali_handle);

    int adcValue = 0;
    int adcVoltage = 0;

    while (1)
    {
        adc_oneshot_read(adc_handle, ADC_CHANNEL_9, &adcValue);
        adc_cali_raw_to_voltage(adc_cali_handle, adcValue, &adcVoltage);
        //ESP_LOGI("ADC Voltage = ", "%d", adcVoltage);
        printf("%d \n", adcVoltage);
        vTaskDelay(delay);
    }
}

bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated)
    {
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI("ADC CALI INIT", "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW("ADC CALI INIT", "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE("ADC CALI INIT", "Invalid arg or no memory");
    }

    return calibrated;
}

#pragma endregion ADC