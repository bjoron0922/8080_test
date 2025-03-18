#pragma once

#include <stdio.h>
#include <sys/lock.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_interface.h"
#include "esp_lcd_panel_ops.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "lvgl_demo_ui.c"
#include "lvgl.h"

static const char *TAG = "example";

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     CONFIG_EXAMPLE_LCD_PIXEL_CLOCK_HZ
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_DATA0          CONFIG_EXAMPLE_PIN_NUM_DATA0
#define EXAMPLE_PIN_NUM_DATA1          CONFIG_EXAMPLE_PIN_NUM_DATA1
#define EXAMPLE_PIN_NUM_DATA2          CONFIG_EXAMPLE_PIN_NUM_DATA2
#define EXAMPLE_PIN_NUM_DATA3          CONFIG_EXAMPLE_PIN_NUM_DATA3
#define EXAMPLE_PIN_NUM_DATA4          CONFIG_EXAMPLE_PIN_NUM_DATA4
#define EXAMPLE_PIN_NUM_DATA5          CONFIG_EXAMPLE_PIN_NUM_DATA5
#define EXAMPLE_PIN_NUM_DATA6          CONFIG_EXAMPLE_PIN_NUM_DATA6
#define EXAMPLE_PIN_NUM_DATA7          CONFIG_EXAMPLE_PIN_NUM_DATA7
#if CONFIG_EXAMPLE_LCD_I80_BUS_WIDTH > 8
#define EXAMPLE_PIN_NUM_DATA8          CONFIG_EXAMPLE_PIN_NUM_DATA8
#define EXAMPLE_PIN_NUM_DATA9          CONFIG_EXAMPLE_PIN_NUM_DATA9
#define EXAMPLE_PIN_NUM_DATA10         CONFIG_EXAMPLE_PIN_NUM_DATA10
#define EXAMPLE_PIN_NUM_DATA11         CONFIG_EXAMPLE_PIN_NUM_DATA11
#define EXAMPLE_PIN_NUM_DATA12         CONFIG_EXAMPLE_PIN_NUM_DATA12
#define EXAMPLE_PIN_NUM_DATA13         CONFIG_EXAMPLE_PIN_NUM_DATA13
#define EXAMPLE_PIN_NUM_DATA14         CONFIG_EXAMPLE_PIN_NUM_DATA14
#define EXAMPLE_PIN_NUM_DATA15         CONFIG_EXAMPLE_PIN_NUM_DATA15
#endif
#define EXAMPLE_PIN_NUM_WR             CONFIG_EXAMPLE_PIN_NUM_PCLK
#define EXAMPLE_PIN_NUM_RD             5
#define EXAMPLE_PIN_NUM_CS             CONFIG_EXAMPLE_PIN_NUM_CS
#define EXAMPLE_PIN_NUM_DC             CONFIG_EXAMPLE_PIN_NUM_DC
#define EXAMPLE_PIN_NUM_RST            CONFIG_EXAMPLE_PIN_NUM_RST
#define EXAMPLE_PIN_NUM_BK_LIGHT       CONFIG_EXAMPLE_PIN_NUM_BK_LIGHT
#define EXAMPLE_PIN_NUM_WAIT           9
#define EXAMPLE_LCD_H_RES              800
#define EXAMPLE_LCD_V_RES              480
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_MAX_DELAY_MS 500
#define EXAMPLE_LVGL_TASK_MIN_DELAY_MS 1
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2
#define EXAMPLE_LVGL_DRAW_BUF_LINES    100
#define EXAMPLE_DMA_BURST_SIZE         64 // 16, 32, 64. Higher size improves the performance when the buffer comes from PSRAM

/*Bus configuration struct, required for operating with the 8080 interface*/
esp_lcd_i80_bus_config_t bus_config = {
    .clk_src = LCD_CLK_SRC_DEFAULT,
    .dc_gpio_num = EXAMPLE_PIN_NUM_DC,
    .wr_gpio_num = EXAMPLE_PIN_NUM_WR,
    .data_gpio_nums = {
        EXAMPLE_PIN_NUM_DATA0,
        EXAMPLE_PIN_NUM_DATA1,
        EXAMPLE_PIN_NUM_DATA2,
        EXAMPLE_PIN_NUM_DATA3,
        EXAMPLE_PIN_NUM_DATA4,
        EXAMPLE_PIN_NUM_DATA5,
        EXAMPLE_PIN_NUM_DATA6,
        EXAMPLE_PIN_NUM_DATA7,
#if CONFIG_EXAMPLE_LCD_I80_BUS_WIDTH > 8
        EXAMPLE_PIN_NUM_DATA8,
        EXAMPLE_PIN_NUM_DATA9,
        EXAMPLE_PIN_NUM_DATA10,
        EXAMPLE_PIN_NUM_DATA11,
        EXAMPLE_PIN_NUM_DATA12,
        EXAMPLE_PIN_NUM_DATA13,
        EXAMPLE_PIN_NUM_DATA14,
        EXAMPLE_PIN_NUM_DATA15
#endif
    },
    .bus_width = CONFIG_EXAMPLE_LCD_I80_BUS_WIDTH,
    .max_transfer_bytes = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * sizeof(uint16_t),
    .dma_burst_size = EXAMPLE_DMA_BURST_SIZE
};

/*IO configuration struct, also required for operating with 8080 interface*/
esp_lcd_panel_io_i80_config_t io_config = {
    .cs_gpio_num = EXAMPLE_PIN_NUM_CS,
    .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    .trans_queue_depth = 10,
    .on_color_trans_done = NULL,
    .user_ctx = NULL,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
    .dc_levels = {
        .dc_idle_level = 0,
        .dc_cmd_level = 0,
        .dc_dummy_level = 0,
        .dc_data_level = 1
    },
    .flags = {
        .cs_active_high = 0,
        .reverse_color_bits = 0,
        .swap_color_bytes = 0,
        .pclk_active_neg = 0,
        .pclk_idle_low = 0
    }
};

/*Panel configuration struct, also required for operating with 8080 interface*/
esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
    .flags.reset_active_high = false
};

/*Handles for sending into different functions when using 8080 interface*/
esp_lcd_i80_bus_handle_t bus_handle = NULL;
esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

/*Configuration struct of WAIT pin. The WAIT pin signals
  when the screen is ready to receive more information*/
gpio_config_t wait_config = {
    .pin_bit_mask = (1ULL << EXAMPLE_PIN_NUM_WAIT),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};

/*Configuration struct of PWM timer for controlling backlight level*/
ledc_timer_config_t bk_pwm_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_10_BIT,
    .timer_num = 0,
    .freq_hz = 50000,
    .clk_cfg = LEDC_AUTO_CLK
};

/*Configuration struct of PWM channel for controlling backlight level*/
ledc_channel_config_t bk_pwm_ch_config = {
    .gpio_num = EXAMPLE_PIN_NUM_BK_LIGHT,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 1023
};

/*Parameters for transmitting 8080 interfaced information*/
uint8_t cmd, param = 0;

/*Parameeter to keep track of the current brightness of the screen*/
uint16_t current_brightness = 1023;

/*Function for changing the backlight level of screen*/
void bright(uint16_t target) {
    if (target < 1) {
        target = 1;
    }
    else if (target > 1023) {
        target = 1023;
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, target);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

/*Function for smoothly changing the backlight level of screen*/
void bright_smooth(uint16_t target) {
    if (target < 1) {
        target = 1;
    }
    else if (target > 1023) {
        target = 1023;
    }
    if (target < current_brightness) {
        for (uint16_t i = current_brightness; i > target; i--) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    } 
    else if (target > current_brightness) {
        for (uint16_t i = current_brightness; i < target; i++) {
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, i);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
    } else {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, target);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    }
    current_brightness = target;
}

/*Call on this to display test pattern on ER-TFTMC050-3*/
void test_pattern(uint8_t on_off) {
    if (on_off == 0) {
        cmd = 0x12;
        param = 0b01000000;
        while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            printf("wait.....\n");
        }
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    } else {
        cmd = 0x12;
        param = 0b01100000;
        while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
            vTaskDelay(pdMS_TO_TICKS(1));
            printf("wait.....\n");
        }
        ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    }
}

/*Initialisation of GPIOs*/
void gpio_init() {
    gpio_config(&wait_config);
    ledc_timer_config(&bk_pwm_config);
    ledc_channel_config(&bk_pwm_ch_config);
}

/*Initialisation of 8080 interface*/
void i80_init() {
    esp_lcd_new_i80_bus(&bus_config, &bus_handle);
    esp_lcd_new_panel_io_i80(bus_handle, &io_config, &io_handle);
    esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
}

/*Initialisation of ER-TFTMC050-3*/
void screen_init(esp_lcd_i80_bus_handle_t bus_handle, esp_lcd_panel_dev_config_t *panel_config, esp_lcd_panel_handle_t *panel) {

    /*Espressifs reset function taken from ST7789 driver, seems to be working fine*/
    esp_lcd_panel_reset(*panel);

    //esp_lcd_panel_init(*panel); //The program seems to be working fine without this, custom init starts below.

    /*Sets up 8 bit bus and WAIT pin*/
    cmd = 0x01;
    param = 0b00010000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Clocks*/
    cmd = 0x05;
    param = 0b11010100;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x06;
    param = 0b01100100;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x07;
    param = 0b11010100;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x08;
    param = 0b11001000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x09;
    param = 0b11010100;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x0A;
    param = 0b11001000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x00;
    param = 0b10000000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*SDRAM organisation*/
    cmd = 0xE0;
    param = 0x20;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0xE1;
    param = 0x03;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0xE2;
    param = 0x1A;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0xE3;
    param = 0x06;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0xE4;
    param = 0b00000101;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*"Canvas" and "active window" settings*/
    cmd = 0x50;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x51;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x52;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x53;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x54;
    param = 0x20;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x55;
    param = 0x03;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x56;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x57;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x58;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x59;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x5A;
    param = 0x20;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x5B;
    param = 0x03;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x5C;
    param = 0xE0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x5D;
    param = 0x01;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x5E;
    param = 0x01;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Setting for using DisplayRAM for image data*/
    cmd = 0x03;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Picture size settings*/
    cmd = 0x20;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x21;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x22;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x23;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x24;
    param = 0b00100000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x25;
    param = 0b00000011;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x26;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x27;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x28;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x29;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Color depth setting*/
    cmd = 0x10;
    param = 0b00000100;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Display off for setting up following picture setting*/
    cmd = 0x12;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    
    /*Mandatory registers for picture settings*/
    cmd = 0x13;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x14;
    param = 0x63;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x15;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x16;
    param = 0x03;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x17;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x18;
    param = 0x1F;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x19;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1A;
    param = 0xDF;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1B;
    param = 0x01;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1C;
    param = 0x15;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1D;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1E;
    param = 0x0B;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));
    cmd = 0x1F;
    param = 0;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Display on for next set of settings according to data sheet*/
    cmd = 0x12;
    param = 0b01000000;
    while (gpio_get_level(EXAMPLE_PIN_NUM_WAIT) == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        printf("wait.....\n");
    }
    ESP_ERROR_CHECK(esp_lcd_panel_io_tx_param(io_handle, cmd, &param, 1));

    /*Full screen brightness*/
    bright(1023);

    *panel = panel_handle;
}
