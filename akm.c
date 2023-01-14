//###########################################################################
/*
 * This is a minimum working example which demonstrates usage of the SCI Module
 * for the MSP430F28379D
 *
 * Pinout
 *
 * I2C :            SDA (J1.10 / GPIO104)
 *                  SCL (J1.9 / GPIO105)
 * SCIA :           GPIO43 / GPIO42 (on board)
 * HEARTBEAT :      GPIO0 (J4.40)
 * ODINT-PIN :      GPIO1 (J4.39)
*/
//#############################################################################

//
// Included Files
//

#include <akm.h>
#include "driverlib.h"
#include "device.h"

#define TIMER_PERIOD_US 2000

uint8_t start_meas=0;
//test string for testing the serial comm
const char teststr[]="NNAABBCC\r\n";
uint16_t sendbuf[8];
uint16_t read_buf[8];

//circular buffer for storing H field samples
//#define CIRCULAR_BUFFER_SIZE 512
//int16_t bit5buffer[CIRCULAR_BUFFER_SIZE];

struct akm_meas{
    int16_t Bx;
    int16_t By;
    int16_t Bz;
    uint16_t status;
};

__interrupt void cpuTimer0ISR(void){
    start_meas=1;
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP1);
}


void serialize_akm(uint16_t* dest, const struct akm_meas* meas){
    //first two bytes are for framing
    dest[0]=0x0055;
    dest[1]=0x0055;
    //serialized measurements
    dest[2]=(meas->Bx);
    dest[3]=(meas->Bx)>>8;
    dest[4]=(meas->By);
    dest[5]=(meas->By)>>8;
    dest[6]=(meas->Bz);
    dest[7]=(meas->Bz)>>8;
}

void init_i2c(void){
    I2C_disableModule(I2CA_BASE);
    I2C_initMaster(I2CA_BASE, DEVICE_SYSCLK_FREQ, 200000, I2C_DUTYCYCLE_50);
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    I2C_setSlaveAddress(I2CA_BASE, 0);
    I2C_disableLoopback(I2CA_BASE);
    I2C_setBitCount(I2CA_BASE, I2C_BITCOUNT_8);
    I2C_setDataCount(I2CA_BASE, 1);
    I2C_setAddressMode(I2CA_BASE, I2C_ADDR_MODE_7BITS);
    I2C_enableFIFO(I2CA_BASE);
    I2C_setFIFOInterruptLevel(I2CA_BASE, I2C_FIFO_TXEMPTY, I2C_FIFO_RXEMPTY);
    I2C_setEmulationMode(I2CA_BASE, I2C_EMULATION_FREE_RUN);
    I2C_enableModule(I2CA_BASE);
}

void pinmux_init(void){
    // GPIO11 -> HEARTBEAT Pinmux
    GPIO_setPinConfig(GPIO_0_GPIO0);
    GPIO_setDirectionMode(HEARTBEAT_PIN, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(HEARTBEAT_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setMasterCore(HEARTBEAT_PIN, GPIO_CORE_CPU1);
    GPIO_setQualificationMode(HEARTBEAT_PIN, GPIO_QUAL_SYNC);
    // SDA & SCL lines for I2C
    GPIO_setPinConfig(GPIO_105_SCLA);
    GPIO_setPadConfig(105, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(105, GPIO_PIN_TYPE_OD);
    GPIO_setQualificationMode(105, GPIO_QUAL_ASYNC);
    GPIO_setPinConfig(GPIO_104_SDAA);
    GPIO_setPadConfig(104, GPIO_PIN_TYPE_PULLUP);
    GPIO_setPadConfig(104, GPIO_PIN_TYPE_OD);
    GPIO_setQualificationMode(104, GPIO_QUAL_ASYNC);
    // ODINT Pin
    GPIO_setDirectionMode(ODINT,GPIO_DIR_MODE_IN);
    GPIO_setPinConfig(GPIO_1_GPIO1);
    // SCIA
    GPIO_setPinConfig(GPIO_43_SCIRXDA);
    GPIO_setPinConfig(GPIO_42_SCITXDA);
}

void gpio_init(void){
    //HEARTBEAT initialization
    GPIO_setDirectionMode(HEARTBEAT_PIN, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(HEARTBEAT_PIN, GPIO_PIN_TYPE_STD);
    GPIO_setMasterCore(HEARTBEAT_PIN, GPIO_CORE_CPU1);
    GPIO_setQualificationMode(HEARTBEAT_PIN, GPIO_QUAL_SYNC);

    //W2BW_SUPPLY initialization
//    GPIO_setDirectionMode(ODINT, GPIO_DIR_MODE_OUT);
//    GPIO_setPadConfig(ODINT, GPIO_PIN_TYPE_STD);
//    GPIO_setMasterCore(ODINT, GPIO_CORE_CPU1);
//    GPIO_setQualificationMode(ODINT, GPIO_QUAL_SYNC);
}

void timer_init(void){
    const uint32_t timer_perios_us=TIMER_PERIOD_US;
    CPUTimer_setPeriod(CPUTIMER0_BASE,DEVICE_SYSCLK_FREQ/1000000*timer_perios_us);
    CPUTimer_setEmulationMode(CPUTIMER0_BASE,
                              CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT);
    CPUTimer_setPreScaler(CPUTIMER0_BASE, 0);
    CPUTimer_stopTimer(CPUTIMER0_BASE);
    CPUTimer_reloadTimerCounter(CPUTIMER0_BASE);
    CPUTimer_startTimer(CPUTIMER0_BASE);
    //register & enable CPU Timer0 interrupt
    Interrupt_register(INT_TIMER0, &cpuTimer0ISR);
    CPUTimer_enableInterrupt(CPUTIMER0_BASE);
    Interrupt_enable(INT_TIMER0);
}

void sci_comm(void){
    //
    // Configuration for the SCI Rx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCIRXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCIRXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_DIR_MODE_IN);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCIRXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCIRXDA, GPIO_QUAL_ASYNC);

    //
    // Configuration for the SCI Tx pin.
    //
    GPIO_setMasterCore(DEVICE_GPIO_PIN_SCITXDA, GPIO_CORE_CPU1);
    GPIO_setPinConfig(DEVICE_GPIO_CFG_SCITXDA);
    GPIO_setDirectionMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(DEVICE_GPIO_PIN_SCITXDA, GPIO_PIN_TYPE_STD);
    GPIO_setQualificationMode(DEVICE_GPIO_PIN_SCITXDA, GPIO_QUAL_ASYNC);

    //
    // Initialize SCIA and its FIFO.
    //
    SCI_performSoftwareReset(SCIA_BASE);


    //
    // Configure SCIA for echoback.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 460800, (SCI_CONFIG_WLEN_8 |
                                                        SCI_CONFIG_STOP_ONE |
                                                        SCI_CONFIG_PAR_NONE));
    SCI_resetChannels(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
    SCI_resetTxFIFO(SCIA_BASE);
    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_TXFF | SCI_INT_RXFF);
    SCI_enableFIFO(SCIA_BASE);
    SCI_enableModule(SCIA_BASE);
    SCI_performSoftwareReset(SCIA_BASE);
}

uint8_t i2c_write(uint8_t addr,const uint8_t* data,uint8_t count){
    //wait until the bus is not busy anymore (e.g. STOP condition has been sent)
    while(I2C_getStopConditionStatus(I2CA_BASE))
        ;

    //set the I2C slave address to the write address
    I2C_setSlaveAddress(I2CA_BASE,I2C_W2BW_ADDRESS);
    //set the number of bytes to be sent
    I2C_setDataCount(I2CA_BASE, count);
    //push the data to the I2C TX FIFO
    uint8_t i=0;
    for(i=0; i<count; i++){
        I2C_putData(I2CA_BASE,data[i]);
    }
    //set the I2C mode to master
    I2C_setConfig(I2CA_BASE, I2C_MASTER_SEND_MODE);
    //send the start condition
    I2C_sendStartCondition(I2CA_BASE);
    //send the stop condition
    I2C_sendStopCondition(I2CA_BASE);
    return 0;
}

uint8_t i2c_read(uint8_t addr,uint16_t* data,uint8_t count){
    //wait until the bus is not busy anymore (e.g. STOP condition has been sent)
    while(I2C_getStopConditionStatus(I2CA_BASE))
        ;

    uint8_t i=0;
    //set the I2C slave address to the write address
    I2C_setSlaveAddress(I2CA_BASE,I2C_W2BW_ADDRESS);
    //set the I2C mode to master & repeat mode (data is read until STOP condition)
    I2C_setConfig(I2CA_BASE, I2C_MASTER_RECEIVE_MODE|I2C_REPEAT_MODE);
    //send the start condition
    I2C_sendStartCondition(I2CA_BASE);


    uint8_t bytes_read=0;
    while((count-bytes_read)>16){
        //wait until 16 bytes have been read
        while(I2C_getRxFIFOStatus(I2CA_BASE)!=I2C_FIFO_RX16)
            ;
        //copy the 16 bytes to the data buffer
        for(i=0; i<16; i++)
            data[bytes_read+i]=I2C_getData(I2CA_BASE);
        bytes_read+=16;
    }
    uint8_t remaining_bytes=count-bytes_read;
    //wait for the remaining bytes to be read into the RX FIFO
    while(I2C_getRxFIFOStatus(I2CA_BASE)!=remaining_bytes)
        ;
    I2C_RxFIFOLevel level=I2C_getRxFIFOStatus(I2CA_BASE);
    //send the stop condition
    I2C_sendStopCondition(I2CA_BASE);
    //copy the remaining bytes
    //retreive the latest bit (e.g. the bit that was received most recently)
    //data[remaining_bytes-1]=I2C_getData(I2CA_BASE);
    //copy the remaining received bits from the FIFO
    for(i=0; i<remaining_bytes; i++)
        data[bytes_read+i]=I2C_getData(I2CA_BASE);

    return 0;
}

void parse_akm_meas(const uint8_t* bytes,struct akm_meas* meas){
    //read the status byte
    meas->status=bytes[0];
    //parse Bz value
    uint16_t Bz=0;
    Bz=bytes[1];
    Bz=Bz<<8;
    Bz=Bz|bytes[2];
    //parse By value
    uint16_t By=0;
    By=bytes[3];
    By=By<<8;
    By=By|bytes[4];
    //parse Bx value
    uint16_t Bx=0;
    Bx=bytes[5];
    Bx=Bx<<8;
    Bx=Bx|bytes[6];
    //fill in structure
    meas->Bx=(int16_t)Bx;
    meas->By=(int16_t)By;
    meas->Bz=(int16_t)Bz;
}

//definitions for AKM comm
//----CNTL1 register (16bit)
const uint8_t ADDR_CNTL1=0x20;
const uint8_t CNTL1_LOW_DRDY_EVENT_EN=0b00000001;
// upper 8 bits are all zero for this register
//----CNTL2 register (8bit)
const uint8_t ADDR_CNTL2=0x21;
const uint8_t CNTL2_MEASMODE6=0x0C;     //500Hz continuous measurement mode
const uint8_t CNTL2_SINGLEMEAS=0x01;    //single measurement mode
//----additional addresses
const uint8_t ADDR_STATUS=0x10;
const uint8_t ADDR_STATUS_AND_FIELDS=0x17;

//global variables
struct akm_meas current_meas;

//global memores of last N measurements
#define N_MEM 1024
int16_t Bxmem[N_MEM]={0};
int16_t Bymem[N_MEM]={0};
int16_t Bzmem[N_MEM]={0};

//
// Main
//
void main(void)
{
    //
    // Initialize device clock and peripherals
    //
    Device_init();
    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();
    //
    // Initialize PIE and clear PIE registers. Disable CPU interrupts.
    //
    Interrupt_initModule();
    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //init the I2C interface
    pinmux_init();
    gpio_init();
    init_i2c();

    //enable / disable the I2C module
    I2C_disableModule(I2CA_BASE);
    DEVICE_DELAY_US(200000u);
    I2C_enableModule(I2CA_BASE);


    //enable event on data ready (e.g. odint pulled low when data ready)
    const uint8_t WRITE_CONFIG_CNTL1[]={ADDR_CNTL1,CNTL1_LOW_DRDY_EVENT_EN,0x00};
    //set measurement mode to MODE6 (e.g. single measurement mode)
    const uint8_t WRITE_CONFIG_CNTL2[]={ADDR_CNTL2,CNTL2_SINGLEMEAS};
    //write the address to be read to the sensor using I2C write command
    const uint8_t WRITE_STATUS_AND_FIELDS_ADDR[]={ADDR_STATUS_AND_FIELDS};

    //initialize AKM sensor
    i2c_write(0,WRITE_CONFIG_CNTL1,3);
    i2c_write(0,WRITE_CONFIG_CNTL2,2);
    DEVICE_DELAY_US(2000000u);

    /* read status register for debugging purposes, not needed
    uint8_t read_buf[6];
    //first, write the random address to be read to the sensor using a I2C write transaction
    const uint8_t WRITE_RAND_ADDR[]={ADDR_STATUS};
    i2c_write(0,WRITE_RAND_ADDR,1);
    i2c_read(0,read_buf,1);
    */

    //initialize the SCI comm interface to the PC
    sci_comm();

    //set up timer & enable interrupts
    timer_init();
    Interrupt_enableMaster();
    EINT;
    ERTM;

    uint16_t loop_counter=0;
    for(;;){
        if(start_meas){
            //toggle the heartbeat pin
            GPIO_togglePin(HEARTBEAT_PIN);

            //set the mode to single measurement mode via I2C, this triggers a new measurement
            i2c_write(0,WRITE_CONFIG_CNTL2,2);
            //DEVICE_DELAY_US(100u);

            i2c_write(0,WRITE_STATUS_AND_FIELDS_ADDR,1);
            //DEVICE_DELAY_US(100u);

            //read 7 bytes from the address
            // for some reason, with the current `i2c_read` function, we need to read 8 bytes instead of 7. the first byte that is read seems to be random
            i2c_read(0,read_buf,7+1);

            //parse magnetic data from sensor
            parse_akm_meas(read_buf+1,&current_meas);

            //serialize last measurement and send over UART
            serialize_akm(sendbuf,&current_meas);
            SCI_writeCharArray(SCIA_BASE,sendbuf,sizeof(sendbuf));


            start_meas=0;
            loop_counter+=1;
        }
    }
}

//
// End of File
//
