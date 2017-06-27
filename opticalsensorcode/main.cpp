#include "mbed.h"
#include "adns9800.h"

# define std_index 4

InterruptIn pulse(p8); // connects to FPGA and listens for start recording pulse
DigitalOut gndPin(p7);
DigitalOut ledPin(LED1);
ADNS9800 adns(p11,p12,p13,p10);
Serial pc(USBTX, USBRX);


int dx, dy;

int movementFlag, recFlag;
Ticker tmr;
uint32_t time_start, time_now, time_elapsed;
int arr [std_index];

unsigned int count(int i) {
 unsigned int ret=1;
 while (i/=10) ret++;
 return ret;
 }

void displayRegisters()
{
  int oreg[4] = {0x00,0x3F,0x2A,0x02};
  char* oregname[] = {"Product_ID:","Inverse_Product_ID:","SROM_Version:","Motion:"};
  uint8_t regres;
  
  int rctr;
  
  for(rctr=0; rctr<4; rctr++){
    pc.printf("---\r\n");
    regres = adns.read_reg(oreg[rctr]);
    pc.printf(oregname[rctr]);
    pc.printf("%02x\r\n", regres);
    wait_ms(1);
  }
}

void read_motion()
{
    int motion, obs;
    adns.read_burst(&motion, &obs, &dx, &dy);
   // if(recFlag == 1 && (dx > 0 || dy > 0))
   if(recFlag == 1)
    {
        time_now = us_ticker_read();
        time_elapsed = time_now - time_start;
        time_start = time_now;
        
        movementFlag = 1;
    }
}



void start_rec()
{
    recFlag = 1;
    ledPin = 1;
    time_start = us_ticker_read();
}

void stop_rec()
{
    ledPin = 0;
    recFlag = 0;
}

void format_and_send()
 {  
    // X direction. To comment a section add '/*' at the begining of the section and '*/' at the end of the section
   int outPkt_x =0;
    
    if(dx & 0x8000)
     {
         dx = (dx^0xFFFF) + 1; //2's complement
         outPkt_x= 0x40 | (dx & 0x3F); // negative
     }
     else
     {
         outPkt_x = dx & 0x3F;
     }
    pc.printf("%d\n", outPkt_x);
    
    //Y-direction to uncomment remove the '/*' and '*/'
    /*int outPkt_y =0;
    
    if(dy & 0x8000)
     {
         dy = (dy^0xFFFF) + 1; //2's complement
         outPkt_y= 0x40 | (dy & 0x3F); // negative
     }
     else
     {
         outPkt_y = dy & 0x3F;
     }
    pc.printf("%d\n", outPkt_y);
    */
    
      
    
 }

int main() {
    gndPin = 0; // lazy to connect to true ground pin
    movementFlag = 0;
     recFlag = 1;

    pc.baud(115200);
    pc.printf("ADNS9800 test\r\n");
    
    adns.initialize();
    
    // display registers
    displayRegisters();
    
    pc.printf("Sensor initialized\r\n");
    
    tmr.attach (&read_motion, 0.01);// replace it with 0.01 to execute 100 tasks in one second
    
    while(1) {
        
        if(movementFlag == 1)
        {
            format_and_send();
            movementFlag = 0;
        }
    }        
}
