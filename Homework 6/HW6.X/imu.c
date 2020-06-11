#include <stdio.h>
#include "imu.h"
#include "i2c_master_noint.h"
#include "ssd1306.h"

void imu_setup(){
    unsigned char who = 0;
    // read from IMU_WHOAMI
    i2c_master_start();
    i2c_master_send(IMU_ADDR);
    i2c_master_send(IMU_WHOAMI);
    i2c_master_restart();
    i2c_master_send(0b11010111);
    who = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    if (who != 0b01101001){
        while(1){
        LATAbits.LATA4 = !LATAbits.LATA4;
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 12000000){}
        }
    }

    // init IMU_CTRL1_XL
    unsigned char ctrl1_init = 0b10000010;
    i2c_master_start();
    i2c_master_send(IMU_ADDR);
    i2c_master_send(IMU_CTRL1_XL);
    i2c_master_send(ctrl1_init);
    i2c_master_stop();
    
    // init IMU_CTRL2_G
    unsigned char ctrl2_init = 0b10001000;
    i2c_master_start();
    i2c_master_send(IMU_ADDR);
    i2c_master_send(IMU_CTRL2_G);
    i2c_master_send(ctrl2_init);
    i2c_master_stop();
    
    // init IMU_CTRL3_C
    unsigned char ctrl3_init = 0b00000100;
    i2c_master_start();
    i2c_master_send(IMU_ADDR);
    i2c_master_send(IMU_CTRL3_C);
    i2c_master_send(ctrl3_init);
    i2c_master_stop();
}

void imu_read(unsigned char reg, signed short * data, int len){
    unsigned char char_data[len*2];
    // read multiple from the imu, each data takes 2 reads so you need len*2 chars
    i2c_read_multiple(IMU_ADDR, reg, char_data, 2*len);
    // turn the chars into the shorts
    unsigned short data1 = 0;
    unsigned short data2 = 0;
    int i = 0;
    while(i<len){
        data1 = char_data[2*i];
        data2 = char_data[2*i+1];
        *data = data1<<8;
        *data = *data|data2;
        i++;
        data++;
    }
}