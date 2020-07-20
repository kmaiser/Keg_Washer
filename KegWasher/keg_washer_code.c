/*Keg Washer Code - runs a wash cycle for keg washer.
* @Author - Kurt Maiser
* Start Date - 7/10/2020
*/

#include <bcm2835.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

//level sensors (High and Low)
#define level_wash_H RPI_BPLUS_GPIO_J8_07
#define level_wash_L RPI_BPLUS_GPIO_J8_11
#define level_san_H RPI_BPLUS_GPIO_J8_13
#define level_san_L RPI_BPLUS_GPIO_J8_15

//heating elements (RELAYS)
#define heat_san RPI_BPLUS_GPIO_J8_12
#define heat_wash RPI_BPLUS_GPIO_J8_16

//pumps (RELAYS)
#define pump_san RPI_BPLUS_GPIO_J8_18
#define pump_recycleRinse RPI_BPLUS_GPIO_J8_33
#define pump_wash RPI_BPLUS_GPIO_J8_35

//solenoids (RELAYS)
#define sol_CO2Purge RPI_BPLUS_GPIO_J8_22
#define sol_san RPI_BPLUS_GPIO_J8_32
#define sol_wash RPI_BPLUS_GPIO_J8_36
#define sol_drain RPI_BPLUS_GPIO_J8_37
#define sol_recycleRinse RPI_BPLUS_GPIO_J8_38
#define sol_hotRinse RPI_BPLUS_GPIO_J8_38

//temperature sensors
#define temp_wash RPI_BPLUS_GPIO_J8_29 // set to 130
#define temp_san RPI_BPLUS_GPIO_J8_31

//lets you use PIN_OUTP for output and PIN_INPT for input
#define PIN_OUTP BCM2835_GPIO_FSEL_OUTP
#define PIN_INPT BCM2835_GPIO_FSEL_INPT

void purge();
void purgeSan();
void rinse();
void setOn(uint8_t);
void setOff(uint8_t);

int main() {
    //bcm2835_gpio_fsel(PIN, PIN_OUTP);         ----sets pin to output
    //bcm2835_gpio_fsel(PIN, PIN_INPT);         ----sets pin to input
    //bcm2835_gpio_write(PIN, HIGH);            ----turn pin ON
    //bcm2835_gpio_write(PIN, LOW);             ----turn pin off
    //bcm2835_gpio_lev(pin)                     ----READ

    if (!bcm2835_init()) return 1;

    char go = 'N';

    do {

        printf("Starting Wash Sequence");
        printf("/nDraining Old Beer");

        //open drain
        setOn(sol_drain);
        purge();
        sleep(15);

        //steps 5-11
        for (int i = 0; i < 2; i++) {
            rinse();
            purge();
            sleep(10);
        }

        //switches from drain to recycle
        setOff(sol_drain);
        setOn(sol_recycleRinse);

        for (int i = 0; i < 2; i++) {
            rinse();
            purge();
            sleep(10);
        }
        sleep(10);

        //start wash cycle
        setOff(sol_recycleRinse);
        setOn(sol_wash);
        setOn(pump_wash);
        bcm2835_gpio_fsel(level_wash_L, PIN_INPT);
        while(bcm2835_gpio_lev(level_wash_L) != HIGH);
        setOff(pump_wash);

        //Repeat 21-23 until 5 minuets have passed
        clock_t startTime = clock();
        while (clock() < startTime + (5 * 60 * 1000)) {
            setOn(sol_CO2Purge);
            sleep(5);
            setOff(sol_CO2Purge);
            sleep(10);
            setOn(pump_wash);
            bcm2835_gpio_fsel(level_wash_L, PIN_INPT);
            while(bcm2835_gpio_lev(level_wash_L) != HIGH);
            setOff(pump_wash);
        }
        setOff(pump_wash);
        sleep(30);
        setOff(sol_wash);

        //Rinse cycle
        setOn(sol_recycleRinse);
        rinse();
        purge();
        sleep(10);
        rinse();
        purge();
        setOff(sol_recycleRinse);

        //sanitize cycle
        setOn(sol_san);
        setOn(pump_san);
        bcm2835_gpio_fsel(level_san_L, PIN_INPT);
        while(bcm2835_gpio_lev(level_san_L) != HIGH);
        setOff(pump_san);                                  //FIXME???
        purgeSan();
        //repeat 40-43
        startTime = clock();
        while (clock() < startTime + (1 * 60 * 1000)) {
            setOn(pump_san);
            bcm2835_gpio_fsel(level_san_L, PIN_INPT);
            while(bcm2835_gpio_lev(level_san_L) != HIGH);
            setOff(pump_san);
            // ^^^ not in steps 40-43 so unsure                             //FIXME???
            purgeSan();
        }
        setOff(pump_san);
        sleep(30);
        setOff(sol_san);

        //rinse cycle
        setOn(sol_recycleRinse);
        rinse();
        purge();
        sleep(10);
        rinse();
        purge();
        sleep(10);
        setOff(sol_recycleRinse);

        //cycle complete
        setOn(sol_drain);
                                                                    //light & buzzer?
        printf("/nComplete");
        printf("/nAnother Keg to Wash Y/N: ");
        scanf("%c", &go);

    } while (go == 'Y' || go == 'y');

        bcm2835_close();
        return 0;
}

void setOn(uint8_t pin){
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set(pin);

}

void setOff(uint8_t pin){
    bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr(pin));
}

//Purges with CO2
void purge() {
    printf("/nPurging...");
    bcm2835_gpio_fsel(sol_CO2Purge, PIN_OUTP);
    bcm2835_gpio_write(sol_CO2Purge, HIGH);
    sleep(1);
    bcm2835_gpio_write(sol_CO2Purge, LOW);
    printf("Purged");
    return 0;
}

//purge w/ CO2 for 2 sec
void purgeSan(){
    printf("/nPurging San...");
    bcm2835_gpio_fsel(sol_CO2Purge, PIN_OUTP);
    bcm2835_gpio_write(sol_CO2Purge, HIGH);
    sleep(2);
    bcm2835_gpio_write(sol_CO2Purge, LOW);
    sleep(10);
    printf("Purged San");
    return 0;
}

//Rinses with hot water
void rinse(){
    printf("/nRinsing...");
    bcm2835_gpio_fsel(sol_hotRinse, PIN_OUTP);
    bcm2835_gpio_write(sol_hotRinse, HIGH);
    sleep(5);
    bcm2835_gpio_write(sol_hotRinse, LOW);
    printf("Rinsed");
    return 0;
}
