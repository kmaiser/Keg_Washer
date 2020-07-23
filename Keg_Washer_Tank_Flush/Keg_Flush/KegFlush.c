/*KegFlush - Flushes all tanks
 *@Author - Kurt Maiser
 *Start Date 7/22/20
 */

#include <bcm2835.h>
#include <unistd.h>
#include <stdio.h>

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

    setOff(pump_san);
    setOff(pump_recycleRinse);
    setOff(pump_wash);
    setOff(sol_CO2Purge);
    setOff(sol_san);
    setOff(sol_wash);
    setOff(sol_drain);
    setOff(sol_recycleRinse);
    setOff(sol_hotRinse);

    fflush(stdin);
    char tankFlush = 'n';
    char tanksRefilled = 'n';
    printf("To purge tank please make sure coupler is pointed DOWN THE DRAIN\n");
    printf("Flush Tanks? Y/N: \n");
    fflush(stdin);
    scanf("%c", &tankFlush);

    if (tankFlush == 'Y' || tankFlush == 'y') {
        printf("Flushing Tanks\n");
        //PURGE TANKS CODE - tell operator to open ball-valve, open each pump in sequence until empty, tell to refill with water, run all pumps until empty again
        setOn(sol_drain);
        tankFlusher();    //For Testing - fill in with timed code (only wash for now)         //FIXME

        fflush(stdin);
        printf("Refilled Tanks With Water? Y/N: \n");
        fflush(stdin);
        scanf(" %c", &tanksRefilled);

        if (tanksRefilled == 'Y' || tanksRefilled == 'y') tankFlusher();
        printf("Tanks Flushed\n");
    }
    printf("Closing Program\n");
    setOff(pump_san);
    setOff(pump_recycleRinse);
    setOff(pump_wash);
    setOff(sol_CO2Purge);
    setOff(sol_san);
    setOff(sol_wash);
    setOff(sol_drain);
    setOff(sol_recycleRinse);
    setOff(sol_hotRinse);
    setOff(level_wash_L);
    setOff(level_san_L);
    bcm2835_close();
    return 0;
}

void setOn(uint8_t pin){
    //bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(pin, LOW);
}

void setOff(uint8_t pin){
    //bcm2835_gpio_fsel(pin, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(pin, HIGH);
}

//Flushes all 3 tanks
void tankFlusher() {
    setOn(pump_san);
    while(bcm2835_gpio_lev(level_san_L) != LOW) delay(1);
    sleep(2);
    setOff(pump_san);

    setOn(pump_wash);
    while(bcm2835_gpio_lev(level_wash_L) != HIGH) delay(1);
    sleep(2);
    setOff(pump_wash);

    setOn(pump_recycleRinse);
    sleep(40);
    setOff(pump_recycleRinse);
}
