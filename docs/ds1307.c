int bcdtodec(uint8_t num)
{
    return ((num>>4)*10 + (num&0x0f));
}
int dectobcd(uint8_t num)
{
    return ((num/10)<<4 | (num%10));
}

/* main */
void app_main(void)
{
    

    // /* init i2C */
    ESP_ERROR_CHECK(i2c_master_init());

    // data_write[0]=0x07;
    // data_write[1]=0x10;
    // i2c_master_write_slave(I2C_MASTER_NUM, data_write, 2,0x68);

    // data_write[0]=0x00;
    // data_write[1]=dectobcd(0);
    // data_write[2]=dectobcd(46);
    // data_write[3]=dectobcd(15);
    // data_write[4]=dectobcd(3);
    // data_write[5]=dectobcd(28);
    // data_write[6]=dectobcd(5);
    // data_write[7]=dectobcd(20);
    // i2c_master_write_slave(I2C_MASTER_NUM, data_write, 8,0x68);

    while (1)
    {

    data_write[0]=0x00;
    i2c_master_write_slave(I2C_MASTER_NUM, data_write, 1,0x68);
    i2c_master_read_slave(I2C_MASTER_NUM, data_read, 7,0x68);

    data_read[0]=bcdtodec(data_read[0] & 0x7f);
    ESP_LOGI(TAG_I2C,"%d",data_read[0]);

    data_read[1]=bcdtodec(data_read[1]);
    ESP_LOGI(TAG_I2C,"%d",data_read[1]);

    data_read[2]=bcdtodec(data_read[2] & 0x3f);
    ESP_LOGI(TAG_I2C,"%d",data_read[2]);

    data_read[3]=bcdtodec(data_read[3]);
    ESP_LOGI(TAG_I2C,"%d",data_read[3]);

    data_read[4]=bcdtodec(data_read[4]);
    ESP_LOGI(TAG_I2C,"%d",data_read[4]);
 
    data_read[5]=bcdtodec(data_read[5]);
    ESP_LOGI(TAG_I2C,"%d",data_read[5]);

    data_read[6]=bcdtodec(data_read[6]);
    ESP_LOGI(TAG_I2C,"%d",data_read[6]+2000);
    ESP_LOGI(TAG_I2C,"------------------------------");

    vTaskDelay(1000/portTICK_PERIOD_MS);
    }

    /* start Freertos */ 
    //xTaskCreate(&postTask,"postTask",4096*4,NULL,3,NULL);
}
