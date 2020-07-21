/*Keg Washer Code - runs a wash cycle for keg washer.
* @Author - Kurt Maiser
* Start Date - 7/10/2020
*/

#include <bcm2835.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

//level sensors (High and Low)
#define level_wash_H RPI_BPLUS_GPIO_J8_07                   //UNUSED
#define level_wash_L RPI_BPLUS_GPIO_J8_11
#define level_san_H RPI_BPLUS_GPIO_J8_13                    //UNUSED
#define level_san_L RPI_BPLUS_GPIO_J8_15

//heating elements (RELAYS)
#define heat_san RPI_BPLUS_GPIO_J8_12                       //UNUSED
#define heat_wash RPI_BPLUS_GPIO_J8_16                      //UNUSED

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
#define sol_hotRinse RPI_BPLUS_GPIO_J8_40

//temperature sensors
#define temp_wash RPI_BPLUS_GPIO_J8_29                      //UNUSED
#define temp_san RPI_BPLUS_GPIO_J8_31                       //UNUSED

//lets you use BCM2835_GPIO_FSEL_OUTP for output and BCM2835_GPIO_FSEL_INPT for input

void setOn(uint8_t);
void setOff(uint8_t);
void purge();
void purgeSan();
void rinse();
void tankFlusher();

int main(int argc, char **argv) {
    //bcm2835_gpio_fsel(PIN, PIN_OUTP);         ----sets pin to output
    //bcm2835_gpio_fsel(PIN, PIN_INPT);         ----sets pin to input
    //bcm2835_gpio_write(PIN, HIGH);            ----turn pin ON
    //bcm2835_gpio_write(PIN, LOW);             ----turn pin off
    //bcm2835_gpio_lev(pin)                     ----READ

    if (!bcm2835_init()) return 1;
    //setup outputs
    bcm2835_gpio_fsel(pump_san, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(pump_recycleRinse, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(pump_wash, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_CO2Purge, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_san, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_wash, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_drain, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_recycleRinse, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(sol_hotRinse, BCM2835_GPIO_FSEL_OUTP);

    //setup inputs
    bcm2835_gpio_fsel(level_wash_L, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(level_san_L, BCM2835_GPIO_FSEL_INPT);
    char go = 'N';

    do {

        printf("Starting Wash Sequence");
        printf("\nDraining Old Beer");

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
        printf("\nRecycling Rinse Water");
        setOff(sol_drain);
        setOn(sol_recycleRinse);

        for (int i = 0; i < 2; i++) {
            rinse();
            purge();
            sleep(10);
        }
        sleep(10);

        //start wash cycle
        printf("\nStarting Wash Cycle");
        setOff(sol_recycleRinse);
        setOn(sol_wash);
        setOn(pump_wash);
        while(bcm2835_gpio_lev(level_wash_L) != HIGH);
        setOff(pump_wash);

        //Repeat 21-23 until 5 minuets have passed
        printf("\nStart 5 Min Wash Cycle Loop");
        clock_t startTime = clock();
        while (clock() < startTime + (5 * 60 * 1000)) {
            setOn(sol_CO2Purge);
            sleep(5);
            setOff(sol_CO2Purge);
            sleep(10);
            setOn(pump_wash);
            while(bcm2835_gpio_lev(level_wash_L) != HIGH);
            setOff(pump_wash);
        }
        setOff(pump_wash);
        sleep(30);
        setOff(sol_wash);

        //Rinse cycle
        printf("\nStarting Rinse Cycle");
        setOn(sol_recycleRinse);
        rinse();
        purge();
        sleep(10);
        rinse();
        purge();
        setOff(sol_recycleRinse);

        //sanitize cycle
        printf("\nStarting Sanitize Cycle");
        setOn(sol_san);
        setOn(pump_san);
        while(bcm2835_gpio_lev(level_san_L) != HIGH);
        setOff(pump_san);
        purgeSan();
        //repeat 40-43
        printf("\nStarting 1 Min San Loop");
        startTime = clock();
        while (clock() < startTime + (1 * 60 * 1000)) {
            setOn(pump_san);
            while(bcm2835_gpio_lev(level_san_L) != HIGH);
            setOff(pump_san);
            purgeSan();
        }
        setOff(pump_san);
        sleep(30);
        setOff(sol_san);

        //rinse cycle
        printf("\nStarting Rinse Cycle (Post-San)");
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
        printf("\nComplete");
        printf("\nAnother Keg to Wash? Y/N: ");
        scanf("%c", &go);

    } while (go == 'Y' || go == 'y');
    printf("\nEnding Processes");

    char tankFlush = 'n';
    char tanksRefilled = 'n';
    printf("To purge tank please make sure coupler is pointed DOWN THE DRAIN");
    printf(" - //FOR TESTING ONLY CHECKING WASH");                                          //FIXME
    printf("\nFlush Tanks? Y/N: ");
    fflush(stdin);
    scanf("%c", &tankFlush);

    if (tankFlush == 'Y' || tankFlush == 'y') {
        printf("\nFlushing Tanks");
        //PURGE TANKS CODE - tell operator to open ball-valve, open each pump in sequence until empty, tell to refill with water, run all pumps until empty again
        tankFlusher();    //For Testing - fill in with timed code (only wash for now)         //FIXME

        fflush(stdin);
        printf("\nRefilled Tanks With Water? Y/N: ");
        scanf("%c", &tanksRefilled);
        if (tanksRefilled == 'Y' || tanksRefilled == 'y') tankFlusher();
        printf("\nTanks Flushed");
    }
    printf("\nClosing Program");
    bcm2835_close();
    return 0;
}

void setOn(uint8_t pin){
    //bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(pin, HIGH);
}

void setOff(uint8_t pin){
    //bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(pin, LOW);
}

//Purges with CO2
void purge() {
    printf("\nPurging...");
    setOn(sol_CO2Purge);
    sleep(1);
    setOff(sol_CO2Purge);
    printf("Purged");
}

//purge w/ CO2 for 2 sec
void purgeSan(){
    printf("\nPurging San...");
    setOn(sol_CO2Purge);
    sleep(2);
    setOff(sol_CO2Purge);
    sleep(10);
    printf("Purged San");
}

//Rinses with hot water
void rinse() {
    printf("\nRinsing...");
    setOn(sol_hotRinse);
    sleep(5);
    setOff(sol_hotRinse);
    printf("Rinsed");
}
void tankFlusher() {
    setOn(pump_wash);
    bcm2835_gpio_fsel(level_wash_L, BCM2835_GPIO_FSEL_INPT);
    while(bcm2835_gpio_lev(level_wash_L) != HIGH);
    setOff(pump_wash);
    return 0;
}

