// I2C Master utilities, using polling rather than interrupts
// The functions must be called in the correct order as per the I2C protocol
// I2C pins need pull-up resistors, 2k-10k
#include "i2c_master_noint.h"

void i2c_master_setup(void) {
    // using a large BRG to see it on the nScope, make it smaller after verifying that code works
    // look up TPGD in the datasheet
    I2C1BRG = 1000; // I2CBRG = [1/(2*Fsck) - TPGD]*Pblck - 2 (TPGD is the Pulse Gobbler Delay)
    I2C1CONbits.ON = 1; // turn on the I2C1 module
}

void i2c_master_start(void) {
    I2C1CONbits.SEN = 1; // send the start bit
    while (I2C1CONbits.SEN) {
        ;
    } // wait for the start bit to be sent
}

void i2c_master_restart(void) {
    I2C1CONbits.RSEN = 1; // send a restart 
    while (I2C1CONbits.RSEN) {
        ;
    } // wait for the restart to clear
}

void i2c_master_send(unsigned char byte) { // send a byte to slave
    I2C1TRN = byte; // if an address, bit 0 = 0 for write, 1 for read
    while (I2C1STATbits.TRSTAT) {
        ;
    } // wait for the transmission to finish
    if (I2C1STATbits.ACKSTAT) { // if this is high, slave has not acknowledged
        // ("I2C1 Master: failed to receive ACK\r\n");
        while(1){} // get stuck here if the chip does not ACK back
    }
}

unsigned char i2c_master_recv(void) { // receive a byte from the slave
    I2C1CONbits.RCEN = 1; // start receiving data
    while (!I2C1STATbits.RBF) {
        ;
    } // wait to receive the data
    return I2C1RCV; // read and return the data
}

void i2c_master_ack(int val) { // sends ACK = 0 (slave should send another byte)
    // or NACK = 1 (no more bytes requested from slave)
    I2C1CONbits.ACKDT = val; // store ACK/NACK in ACKDT
    I2C1CONbits.ACKEN = 1; // send ACKDT
    while (I2C1CONbits.ACKEN) {
        ;
    } // wait for ACK/NACK to be sent
}

void i2c_master_stop(void) { // send a STOP:
    I2C1CONbits.PEN = 1; // comm is complete and master relinquishes bus
    while (I2C1CONbits.PEN) {
        ;
    } // wait for STOP to complete
}

void i2c_write(unsigned char reg, unsigned char data){// write to a pin
    unsigned char add = 0b01000000;
    i2c_master_start();
    i2c_master_send(add);
    i2c_master_send(reg);
    i2c_master_send(data);
    i2c_master_stop();
}

unsigned char i2c_read(unsigned char reg){ // reads a pin
    unsigned char data;
    unsigned char write_add = 0b01000000;
    unsigned char read_add = 0b01000001;
    i2c_master_start();
    i2c_master_send(write_add);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(read_add);
    data = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
    return data;
} 

void set_pin(unsigned char pin, unsigned char val){ // sets an output pin
    i2c_write(pin, val);
} 

void i2c_init(void){ // initializes IODIR
    i2c_write(0x00, 0x00);
    i2c_write(0x01, 0xFF);
    set_pin(0x14, 0x00);
}

void i2c_read_multiple(unsigned char address, unsigned char reg, unsigned char *data, int length){
    int i = 0;
    i2c_master_start();
    i2c_master_send(address);
    i2c_master_send(reg);
    i2c_master_restart();
    i2c_master_send(address+1);
    while(i<length-1){
        *data = i2c_master_recv();
        i2c_master_ack(0);
        data++;
        i++;
    }
    *data = i2c_master_recv();
    i2c_master_ack(1);
    i2c_master_stop();
}