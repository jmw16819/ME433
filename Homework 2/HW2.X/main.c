#include<xc.h>           // processor SFR definitions
#include<sys/attribs.h>  // __ISR macro
#include<math.h>

#include "spi.h"

// DEVCFG0
#pragma config DEBUG = OFF // disable debugging
#pragma config JTAGEN = OFF // disable jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // disable flash write protect
#pragma config BWP = OFF // disable boot write protect
#pragma config CP = OFF // disable code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // disable secondary oscillator
#pragma config IESO = OFF // disable switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // disable clock output
#pragma config FPBDIV = DIV_1 // divide sysclk freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // disable clock switch and FSCM
#pragma config WDTPS = PS1 // use largest wdt
#pragma config WINDIS = OFF // use non-window mode wdt
#pragma config FWDTEN = OFF // wdt disabled
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the sysclk clock to 48MHz from the 8MHz crystal
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_24 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 48MHz

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = OFF // allow multiple reconfigurations
#pragma config IOL1WAY = OFF // allow multiple reconfigurations

int main() {

    __builtin_disable_interrupts(); // disable interrupts while initializing things

    // set the CP0 CONFIG register to indicate that kseg0 is cacheable (0x3)
    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);

    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;

    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;

    // disable JTAG to get pins back
    DDPCONbits.JTAGEN = 0;

    // do your TRIS and LAT commands here
    TRISAbits.TRISA4 = 0;
    TRISBbits.TRISB4 = 1;
    LATAbits.LATA4 = 0;
    
    initSPI();

    __builtin_enable_interrupts();
    
    int max = 100; // 100 data points per period
    unsigned short sin_points[max]; // will hold a period's worth of voltages
                                    // for the sine wave
    unsigned short tri_points[max]; // will hold a period's worth of voltages
                                    // for the triangle wave
    int sin_freq = 2; // 2Hz sine wave
    float pi = 3.14159265359; // my own definition of pi
    unsigned char chan_a = 0; // channel A
    unsigned char chan_b = 1; // channel B
    int i = 0; // counter for plotting the data points
    int j = 0; // counter for triangle while loop
    int k = 0; // counter for the sine while loop
    unsigned short s; // voltage from the sine array
    unsigned short t; // voltage from the triangle array
    unsigned short sw; // plotted sine point
    unsigned short tw; // plotted triangle point
    
    while (j!=max) { // while loop to make triangle wave array
        if (j=0) {
            tri_points[j] = 0;
        }
        else if (j>0 && j<max/2) {
            tri_points[j] = (unsigned short) (tri_points[j-1]+(4095/50));
        }
        else if (j==max/2) {
            tri_points[j] = 4095;
        }
        else if (j>0 && j>max/2) {
            tri_points[j] = (unsigned short) (tri_points[j-1]-(4095/50));
        }
        j++;
    }
    while (k!=max) { // while loop to make sine wave array
        sin_points[k] = (unsigned short) ((4095/2)+(4095/2)*sin(2*pi*sin_freq*(k/100)));
        k++;
    }
    
    while (1) {
        s = sin_points[i]; // voltage to be plotted at position i of array
        t = tri_points[i]; // voltage to be plotted at position i of array
        sw = chan_a<<15; // add channel A to the front of the sine value to 
                         // be plotted
        sw = sw|(0b111<<12); // add the control specs for the MCP4922 to the
                             // next three bits
        sw = sw|s; // add the voltage from the sine array to the last 12 bits
        tw = chan_b<<15; // add channel B to the front of the triangle value
                         // to be plotted
        tw = tw|(0b111<<12); // add the control specs for the MCP4922 to the
                             // next three bits
        tw = tw|t; // add the voltage from the triangle array to the last 12 bits
        // write the sine point over SPI to channel A
        LATAbits.LATA0 = 0; // bring CS low to write the data
        spi_io(sw>>8); // write first half of sine byte
        spi_io(sw&0xFF); // write second half of sine byte
        LATAbits.LATA0 = 1; // bring CS high to stop writing
        // write the triangle point over SPI to channel B
        LATAbits.LATA0 = 0;
        spi_io(tw>>8); // write first half of triangle byte
        spi_io(tw&0xFF); // write second half of triangle byte
        LATAbits.LATA0 = 1;
        
        i++;
        if (i==max){
            i = 0;
        }
        
        _CP0_SET_COUNT(0);
        while(_CP0_GET_COUNT() < 24000000) { // 1Hz
            ;
        }
    }
}