
/**
 * SlushEngine Model D: Periperal IO Controlller
 * 
 * The SlushEngine Model D uses and external controller (MSP430) to handle the IO lines. This allows the use of PWM, analog and digital IO. All
 * of these features are not conventionally avalible on a single I2C device.  
 * 
 * This firmware can be uploaded to the SlushEngine using the tst and rst test points on the board. 
 * 
 * Based on the infor from tinywire I2C, but uses Wire for the I2C. 
 * 
 * Register Map
 * Ouput_settings(0x00 - 0x07) > If zero will be digital. if 1 or greated will be PWM
 * Input_settings(0x08 - 0x0b) > If zero will be digital. if 1 or greater will be analog
 * Output_state(0x0c - 0x13)   > Write 1 for out put high, or 0-255 for pwm duty cycle
 * Input_state(0x14 - 0x17)    > read 1 or 0, or 0-255 for ADC reading
 * Frequency (0x18)            > PWM frequency for all channels (500Hz to 
 */
 
#define I2C_SLAVE_ADDRESS 0x17
#define TWI_RX_BUFFER_SIZE ( 24 )

#include <Wire.h>

char peripheral_pins[12] = {P1_2, P2_2, P2_1, P2_6, P2_7, P2_5, P2_4, P2_3, P1_0, P1_1, P1_3, P1_4};

volatile unsigned char i2c_regs[28] =
{
    0x00, //Output Setting 0 (0x00) 
    0x00, //Output Setting 1 (0x01)
    0x00, //Output Setting 2 (0x02)
    0x00, //Output Setting 3 (0x03)
    0x00, //Output Setting 4 (0x04) *NO PMW
    0x00, //Output Setting 5 (0x05)
    0x00, //Output Setting 6 (0x06)
    0x00, //Output Setting 7 (0x07) *NO PWM
    0x00, //Input Setting 0  (0x08)
    0x00, //Input Setting 1  (0x09)
    0x00, //Input Setting 2  (0x0a)
    0x00, //Input Setting 3  (0x0b)
    0x00, //Output value 0   (0x0c)
    0x00, //Output value 1   (0x0d)
    0x00, //Output value 2   (0x0e)
    0x00, //Output value 3   (0x0f)
    0x00, //Output value 4   (0x10)
    0x00, //Output value 5   (0x11)
    0x00, //Output value 6   (0x12)
    0x00, //Output value 7   (0x13)
    0x00, //Input value 0MSB    (0x14)
    0x00, //Input value 1MSB    (0x15)
    0x00, //Input value 2MSB    (0x16)
    0x00, //Input value 3MSB   (0x17)
    0x00, //Input value 0LSB    (0x18)
    0x00, //Input value 1LSB    (0x19)
    0x00, //Input value 2LSB    (0x1a)
    0x00, //Input value 3LSB   (0x1b)      
};

volatile unsigned char i2c_hist_regs[28];
volatile byte reg_position;
const byte reg_size = sizeof(i2c_regs);
volatile char new_data = 1;
volatile char input_lock = 0;
int update_counter = 0;

/**
 * This is called for each read request we receive, never put more than one byte of data (with Wire.send) to the 
 * send-buffer when using this callback
 */
void requestEvent()
{   
    Wire.write(i2c_regs[reg_position]);
    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
        reg_position = 0;
    }

}

/**
 * The I2C data received -handler
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly,
 */
void receiveEvent(int howMany)
{
    if (howMany < 1)
    {
        // Sanity-check
        return;
    }
    if (howMany > TWI_RX_BUFFER_SIZE)
    {
        // Also insane number
        return;
    }

    reg_position = Wire.read();
    howMany--;
    if (!howMany)
    {
        // This write was only to set the buffer for next read
        return;
    }
    while(howMany--)
    {
        i2c_regs[reg_position] = Wire.read();
        reg_position++;
        if (reg_position >= reg_size)
        {
            reg_position = 0;
        }
    }
    new_data = 1;
}


void setup()
{
    Wire.setModule(0);
    Wire.begin(I2C_SLAVE_ADDRESS);
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    for(int i = 0; i < 0x13; i++) i2c_hist_regs[i] = i2c_regs[i];
    
    //setup default pin states
    for(int i = 0; i < 8; i++){
      pinMode(peripheral_pins[i], OUTPUT);
      digitalWrite(peripheral_pins[i], LOW);
    }
    for(int i = 8; i < 12; i++) pinMode(peripheral_pins[i], INPUT);
}
/*
 * Main loop
 *  - will call update peripherals if new data is seen
 *  - always calls and update on any hardware inputs
 */
void loop()
{
    if(new_data) updatePeripherals();
    updateReadRegisters();

}

/*
 * Update 
 *  - Will update the state of an IO or its configuration
 *  - configures Digital IO
 */
void updatePeripherals(){
    //look for setup changes. If they have occured make the changes
    for(int i = 0x13; i >= 0; i--){
            if(i2c_regs[i] != i2c_hist_regs[i]){
                    //handle digital configuration
                    if(i <= 0x07){
                            if(i2c_regs[i] == 0){
                                    pinMode(peripheral_pins[i], OUTPUT);
                                    digitalWrite(peripheral_pins[i], LOW);
                            }
                            else{
                                    pinMode(peripheral_pins[i], OUTPUT);
                                    analogWrite(peripheral_pins[i], 0);
                            }
                    }
                    //handle analog configuration
                    if((i >= 0x08) && (i<= 0x0b)){
                            if(i2c_regs[i] == 0){
                                    pinMode(peripheral_pins[i], INPUT_PULLUP);
                                    i2c_regs[i + 12] == digitalRead(peripheral_pins[i]) ;
                                    updateReadRegisters();
                            }
                            else{
                                    pinMode(peripheral_pins[i], INPUT);
                                    updateReadRegisters();
                            }
                    }                
                    //handle digital outs
                    if((i >= 0x0c) && (i<= 0x13)){
                            if(i2c_regs[i-12] == 0x00){
                                    if(i2c_regs[i] == 1) digitalWrite(peripheral_pins[i-12], HIGH);
                                    if(i2c_regs[i] == 0) digitalWrite(peripheral_pins[i-12], LOW);                                
                            }
                            if(i2c_regs[i-12] == 0x01){
                                    analogWrite(peripheral_pins[i-12], i2c_regs[i]);
                            }                     
                    }
            }
    }
    //copy values over to the historical array 
    for(int i = 0x13; i > 0; i--) i2c_hist_regs[i] = i2c_regs[i]; 
    new_data = 0;  
}

/*
 * Update Read
 *  - reads hardware inputs and updates the registers
 */
void updateReadRegisters(){
  unsigned int temp_value = 0;
  for(int i = sizeof(i2c_regs); i >= 0; i--){
            if((i >= 0x14) && (i<= 0x17)){
                // read digital inputs if configured as such
                if(i2c_regs[i-12] == 0x00){
                    temp_value = digitalRead(peripheral_pins[i-12]);
                    i2c_regs[i] = temp_value;               
                }
                // read analog inputs
                else {
                    temp_value = analogRead(peripheral_pins[i-12]);
                    i2c_regs[i+4] = (temp_value & 0x03);
                    i2c_regs[i] = (temp_value >> 2);               
                  }              
            }
  }
}


