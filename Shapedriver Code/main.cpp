#include "mbed.h"
#include "MRF24J40.h"

#include <string>
#include <time.h>

#include "constants.h"
#include "shape_drawer.h"

// RF tranceiver to link with handheld.
MRF24J40 mrf(p11, p12, p13, p14, p26);

// LEDs you can treat these as variables (led2 = 1 will turn led2 on!)
DigitalOut led1(LED1);
DigitalOut led2(LED2);
DigitalOut led3(LED3);
DigitalOut led4(LED4);

// Timer
Timer timer;

// Serial port for showing RX data.
Serial pc(USBTX, USBRX);

// Used for sending and receiving
char txBuffer[128];
char rxBuffer[128];
int rxLen;

//***************** Do not change these methods (please) *****************//

/**
* Receive data from the MRF24J40.
*
* @param data A pointer to a char array to hold the data
* @param maxLength The max amount of data to read.
*/
int rf_receive(char *data, uint8_t maxLength)
{
    uint8_t len = mrf.Receive((uint8_t *)data, maxLength);
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};

    if(len > 10) {
        //Remove the header and footer of the message
        for(uint8_t i = 0; i < len-2; i++) {
            if(i<8) {
                //Make sure our header is valid first
                if(data[i] != header[i])
                    return 0;
            } else {
                data[i-8] = data[i];
            }
        }

        //pc.printf("Received: %s length:%d\r\n", data, ((int)len)-10);
    }
    return ((int)len)-10;
}

/**
* Send data to another MRF24J40.
*
* @param data The string to send
* @param maxLength The length of the data to send.
*                  If you are sending a null-terminated string you can pass strlen(data)+1
*/
void rf_send(char *data, uint8_t len)
{
    //We need to prepend the message with a valid ZigBee header
    uint8_t header[8]= {1, 8, 0, 0xA1, 0xB2, 0xC3, 0xD4, 0x00};
    uint8_t *send_buf = (uint8_t *) malloc( sizeof(uint8_t) * (len+8) );

    for(uint8_t i = 0; i < len+8; i++) {
        //prepend the 8-byte header
        send_buf[i] = (i<8) ? header[i] : data[i-8];
    }
    //pc.printf("Sent: %s\r\n", send_buf+8);

    mrf.Send(send_buf, len+8);
    free(send_buf);
}

/*******************************************************************/
// Display interrupt and drivers

int frame_id = 0;
char frame_buffer1[SLICES][WIDTH];
char frame_buffer2[SLICES][WIDTH];

int slice_i = 100;
BusOut blade(p15, p16, p17, p18, p19, p20, p21, p22);
DigitalOut clk(p23);
DigitalOut disp(p24);
Ticker pixel_ticker;

void push_pixels(){
    for(int j = 8; j < WIDTH; j++){
        if(frame_id == 0)
            blade = frame_buffer1[slice_i][j];
        else
            blade = frame_buffer2[slice_i][j];
            
        clk = 1;
        clk = 0;    
    }
    
    for(int j = 7; j >= 0; j--){
        if(frame_id == 0)
            blade = frame_buffer1[slice_i][j];
        else
            blade = frame_buffer2[slice_i][j];
        
        clk = 1;
        clk = 0;     
    }
    
    disp = 1;
    disp = 0;
    
    slice_i = (slice_i + 1)%SLICES;
}

//Hall sensor interupt
double rotate_time;
double slice_time;

Timer hall_timer;

void rotate_sense(){
    static bool firstTime = false;
    
    if (firstTime){
        hall_timer.reset();
        hall_timer.start();
        firstTime = false;
        return;
    }

    rotate_time = hall_timer.read_us();
    hall_timer.reset();    
    hall_timer.start();
    
    slice_time = (double) rotate_time/SLICES;
    slice_i = 0;
    pixel_ticker.attach_us(&push_pixels, slice_time);
}

/***************************************************************/
//code to adjust for offset blades and vertical alignment

void convert_array(){
    if(frame_id == 0){
        move_buffer(&frame_buffer2);   
        frame_id = 1; 
    } else {
        move_buffer(&frame_buffer1);  
        frame_id = 0;  
    }
}

int display_mode = 0;
Ticker animate_ticker;

int height(float d){
    return 0;    
}

float freq = 4;
float freq_disp = 0.0;
int wave_height(float d){
    float h = 3.5*sin(2.0 * M_PI * (d + freq_disp) * freq) + 3.5;   
    return (int)rint(h);
}

bool init = false;
void switch_mode(){
    display_mode ++;
    if(display_mode == 4)
        display_mode = 1;
        
    init = true;
}

void animate(){
    
    if(display_mode == 0)
    {
        if(init){
            freq_disp = 0;
            freq = 4;
            init = false;   
            set_hfunc(&wave_height); 
        }
        
        for(int i = 3; i < WIDTH; i++){
            draw_circle(0, 0, i, 4, false);    
        } 
        
        freq_disp += 0.1;
        if(freq_disp >= 1.0){
            freq_disp = 0.0;    
        }
    } 
    else if(display_mode == 1)
    {
        set_hfunc(NULL);
        draw_line(10, 10, 0, 10, -10, 0);  
        draw_line(10, -10, 0, -10, -10, 0); 
        draw_line(-10, -10, 0, -10, 10, 0); 
        draw_line(-10, 10, 0, 10, 10, 0); 

        draw_line(10, 10, 7, 10, -10, 7);  
        draw_line(10, -10, 7, -10, -10, 7); 
        draw_line(-10, -10, 7, -10, 10, 7); 
        draw_line(-10, 10, 7, 10, 10, 7);
        
        draw_line(10, 10, 0, 10, 10, 7); 
        draw_line(10, -10, 0, 10, -10, 7); 
        draw_line(-10, -10, 0, -10, -10, 7); 
        draw_line(-10, 10, 0, -10, 10, 7); 
        
        draw_line(5, 5, 2, 5, -5, 2);  
        draw_line(5, -5, 2, -5, -5, 2); 
        draw_line(-5, -5, 2, -5, 5, 2); 
        draw_line(-5, 5, 2, 5, 5, 2); 

        draw_line(5, 5, 5, 5, -5, 5);  
        draw_line(5, -5, 5, -5, -5, 5); 
        draw_line(-5, -5, 5, -5, 5, 5); 
        draw_line(-5, 5, 5, 5, 5, 5);
        
        draw_line(5, 5, 2, 5, 5, 5); 
        draw_line(5, -5, 2, 5, -5, 5); 
        draw_line(-5, -5, 2, -5, -5, 5); 
        draw_line(-5, 5, 2, -5, 5, 5);
   
    } 

    else if(display_mode == 2)
    {
        static float radius = 1;
        
        radius ++;
        if(radius == 16)
            radius = 0;
            
        draw_circle(0, 0, radius, 4, true);
          
    } else if(display_mode == 3){
        if(init){
            freq_disp = 0.0;
            freq = 2.0;
            set_hfunc(&wave_height);
            init = false;    
        }
        
        freq_disp += 0.1;
        if(freq_disp >= 1.0){
            freq_disp = 0.0;    
        }
        
        for(int i = -16; i < 16; i += 2){
            draw_line(i, 16, 0, i, -16, 0);
            //draw_line(16, i, 0, -16, i, 0);
        }
    }
    
    convert_array(); 
}

bool animate_i = false;
void animate_int(){
    animate_i = true;
}

Ticker switch_ticker;

void switch_mode(){
    display_mode = (display_mode + 1) % 3; 
}

int main (void)
{
    display_mode = 4;
        
    InterruptIn hall_pin(p25);
    hall_pin.fall(&rotate_sense);
    animate_ticker.attach(&animate_int, 1.0);
    switch_ticker.attach(&switch_mode, 15.0);
    
    uint8_t channel = 2;

    //Set the Channel. 0 is default, 15 is max
    mrf.SetChannel(channel);

    while(true) {
        if(animate_i){
            animate();    
        }
    }
}
