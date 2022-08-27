#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"


static const char *TAG = "i2c-example";

#define I2C_MASTER_SCL_IO GPIO_NUM_23 // GPIO number for i2c master
#define I2C_MASTER_SDA_IO GPIO_NUM_22
#define I2C_MASTER_NUM 1              // I2C port number for master
#define I2C_MASTER_FREQ_HZ 100000     // I2C master clock frequency
#define I2C_MASTER_TX_BUF_DISABLE 0   // I2c master doesn't need buffer
#define I2C_MASTER_RX_BUF_DISABLE 0

#define WRITE_BIT I2C_MASTER_WRITE    // I2C master write
#define READ_BIT I2C_MASTER_READ      //            read
#define ACK_CHECK_EN 0x1              // I2C master will check ack from slave
#define ACK_CHECK_DIS 0x0             // I2C master will not check ack from slave
#define ACK_VAL 0x0                   // I2C ack value
#define NACK_VAL 0x1                  // I2C nack value

#define BH1750_SLAVE_ADDR   0x23      // Slave address
#define BH1750_PWR_DOWN     0x00      // Close module
#define BH1750_PWR_ON       0x01      // Open the module and wait for the measurement command
#define BH1750_RST          0x07      // Reset data register value valid in 'PowerOn' mode
#define BH1750_CON_H        0x10      // Continuous high-resolution mode, 1lx, 120ms
#define BH1750_CON_H2       0x11      // Continuous high-resolution mode, 0.5lx, 120ms
#define BH1750_CON_L        0x13      // Continuous low-resolution mode, 4lx, 16ms
#define BH1750_ONE_H        0x20      // Once in high-resolution mode, 11x, 120ms, the module goes to 'PowerDown' mode after measurement
#define BH1750_ONE_H2       0x21      // Once in high-resolution mode, 0.51x, 120ms, the module goes to 'PowerDown' mode after measurement
#define BH1750_ONE_L        0x23      // Once in low-resolution mode, 4lx, 16ms, the module goes to 'PowerDown' mode after measurement

SemaphoreHandle_t print_mux = NULL;

int I2C_Init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

int I2C_WriteData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    if (regAddr != NULL) {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_write(cmd, pData, dataLen, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(1, cmd, 1000/portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

int I2C_ReadData(uint8_t slaveAddr, uint8_t regAddr, uint8_t *pData, uint16_t dataLen)
{
    int ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slaveAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if (regAddr != NULL) {
        i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    }
    i2c_master_read(cmd, pData, dataLen, ACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(1, cmd, 1000/portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void BH1750_Init(void)
{
    uint8_t data;
    data = BH1750_PWR_ON; // Phát sóng khởi động mệnh lệnh
    I2C_WriteData(BH1750_SLAVE_ADDR, NULL, &data, 1);
    data = BH1750_CON_H;  // Đặt chế độ độ phân giải cao liên tục, 1lx, 120ms
    I2C_WriteData(BH1750_SLAVE_ADDR, NULL, &data, 1);
}

float BH1750_ReadLightIntensity(void)
{
    float lux = 0.0;
    uint8_t sensorData[2] = {0};
    I2C_ReadData(BH1750_SLAVE_ADDR, NULL, sensorData, 2);
    lux = (sensorData[0] << 8 | sensorData[1]) / 1.2;
    return lux;
}

static void i2c_test_task(void *arg)
{
    int cnt = 0;
    float lux;

    BH1750_Init();
    vTaskDelay(180 / portTICK_RATE_MS);

    while(1) {
        ESP_LOGI(TAG, "test cnt: %d", cnt++);
        lux = BH1750_ReadLightIntensity();
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("sensor value: %.02f [Lux]\n", lux);
        xSemaphoreGive(print_mux);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    vSemaphoreDelete(print_mux);
    vTaskDelete(NULL);
}

void app_main() {
    print_mux = xSemaphoreCreateMutex();
    ESP_ERROR_CHECK(I2C_Init());
    xTaskCreate(i2c_test_task, "i2c_test_task_0", 1024*2, NULL, 10, NULL);
}