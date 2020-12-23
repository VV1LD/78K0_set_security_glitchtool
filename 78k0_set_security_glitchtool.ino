/*****************************************************************************************
*  Quick and dirty 78k0 security set cmd glitcher, used to unlock a chip for 
*  erase/program cmds
*  
*  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! DISCLAIMER !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*  ! HIGH BRICK RISK !
*  ! MAY CORRUPT THE 78K0R ROM, ACCEPT THE RISKS AND PROCEED WITH CAUTION !
*  ! MAY LUCK BE ON YOUR SIDE AND PULL A LUCKY 777 !
*  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*  
* Developed by Wildcard
*
* Compatible with a Teensy 4.0, using other teensy hardware will need pins changed
* and delay periods adjusted possibly.
*
* To glitch the chip, attach glitch pin to the regc pin.
* pinout can be seen on this repo.
*****************************************************************************************/

#define flmd_pin    2
#define reset_pin   3
#define vdd_pin     14
#define glitch_pin  17
#define glitch_pin2  18
#define detected_pin  22
#define trigger_pin 15
#define osc_clk_pin 19

#define ETX         3
#define SOH         1
#define STX         2

#define BLK_SIZE 0x400


// set to whatever orig sec byte is in the chip signature
#define SEC_BYTE 0x68

uint8_t boot0[BLK_SIZE];
int iii;
int slide;
int slide_us;
int len;
bool sec_set = true;
bool prot = false;
bool ackd = false;
bool reseting = true;
char outstr[0x20];
int detect_cnt = 0;
char c;
bool not_blank = false;


uint8_t checksum(int len, uint8_t* buf)
{
  
  uint8_t value = 0;
  
  for(int i = 0; i < len; i++)
  {
    value += buf[i];  
  }  
  
  value += len;  
  value = ~value+1;

  return value;
}



void enter_flash_mode(void)
{
  
  digitalWrite(flmd_pin, LOW);
  digitalWrite(reset_pin, LOW);  
  delay(1);
  digitalWrite(reset_pin, HIGH);  
  delay(2);  
  digitalWrite(flmd_pin, HIGH);  
  delay(15);  
  digitalWrite(reset_pin, LOW);  
  delayMicroseconds(55);  
  digitalWrite(reset_pin, HIGH);  
  delayMicroseconds(55);  
  digitalWrite(reset_pin, LOW);  
  delayMicroseconds(55);  
  digitalWrite(reset_pin, HIGH);  
  delayMicroseconds(55);  
  digitalWrite(reset_pin, LOW);  
  delayMicroseconds(55);  
  digitalWrite(reset_pin, HIGH); 
  delay(50);
  Serial1.write(0);
  delay(1);
  Serial1.write(0);

}



void cmd_init(void)
{
  uint8_t buf[] = {0};

  Serial1.write(SOH);
  Serial1.write(sizeof(buf));
  
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
  }
  
  Serial1.write(checksum(1, buf));
  Serial1.write(ETX);
}




void cmd_baud_set(void)
{
  // extclk = 8.33MHz for 1 div @ 115200 buad
  uint8_t buf[] = {0x90, 8, 3, 3, 4};
  
  Serial1.write(SOH);
  Serial1.write(sizeof(buf));
  
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
  }
  
  Serial1.write(checksum(sizeof(buf), buf));
  Serial1.write(ETX);

}



void cmd_signature(void)
{

  uint8_t buf[] = {0xc0};

  Serial1.write(SOH);
  Serial1.write(sizeof(buf));
  
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
  }
  
  Serial1.write(checksum(1, buf));
  Serial1.write(ETX);

}



void cmd_block_checksum(int block_index)
{
  uint8_t addr_upper_lo = (BLK_SIZE*(block_index+1))-1 & 0xff;
  uint8_t addr_upper_md = (BLK_SIZE*(block_index+1)-1 >> 8) & 0xff;
  uint8_t addr_upper_hi = (BLK_SIZE*(block_index+1)-1 >> 16) & 0xff;

  uint8_t addr_lower_lo = (BLK_SIZE*(block_index)) & 0xff;
  uint8_t addr_lower_md = ((BLK_SIZE*(block_index)) >> 8) & 0xff;
  uint8_t addr_lower_hi = ((BLK_SIZE*(block_index)) >> 16) & 0xff;


  uint8_t buf[] = {0xb0, addr_lower_hi, addr_lower_md, addr_lower_lo, addr_upper_hi, addr_upper_md, addr_upper_lo};

  Serial1.write(SOH);
  delayMicroseconds(20);
  Serial1.write(sizeof(buf));
  delayMicroseconds(20);


  
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(20);
  }

  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(2);
  Serial1.write(ETX);
}



void cmd_block_blank(int block_index)
{
  
  uint8_t addr_upper_lo = (BLK_SIZE*(block_index+1))-1 & 0xff;
  uint8_t addr_upper_md = (BLK_SIZE*(block_index+1)-1 >> 8) & 0xff;
  uint8_t addr_upper_hi = (BLK_SIZE*(block_index+1)-1 >> 16) & 0xff;

  uint8_t addr_lower_lo = (BLK_SIZE*(block_index)) & 0xff;
  uint8_t addr_lower_md = ((BLK_SIZE*(block_index)) >> 8) & 0xff;
  uint8_t addr_lower_hi = ((BLK_SIZE*(block_index)) >> 16) & 0xff;

  uint8_t buf[] = {0x32, addr_lower_hi, addr_lower_md, addr_lower_lo, addr_upper_hi, addr_upper_md, addr_upper_lo, 0x00};

  Serial1.write(SOH);
  delayMicroseconds(20);
  Serial1.write(sizeof(buf));
  delayMicroseconds(20);

 
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(20);
  }
  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(20);
  Serial1.write(ETX);

}



void cmd_program(int block_index)
{
  
  uint8_t addr_upper_lo = (BLK_SIZE*(block_index+1))-1 & 0xff;
  uint8_t addr_upper_md = (BLK_SIZE*(block_index+1)-1 >> 8) & 0xff;
  uint8_t addr_upper_hi = (BLK_SIZE*(block_index+1)-1 >> 16) & 0xff;

  uint8_t addr_lower_lo = (BLK_SIZE*(block_index)) & 0xff;
  uint8_t addr_lower_md = ((BLK_SIZE*(block_index)) >> 8) & 0xff;
  uint8_t addr_lower_hi = ((BLK_SIZE*(block_index)) >> 16) & 0xff;

  uint8_t buf[] = {0x40, addr_lower_hi, addr_lower_md, addr_lower_lo, addr_upper_hi, addr_upper_md, addr_upper_lo};

  Serial1.write(SOH);
  delayMicroseconds(10);
  Serial1.write(sizeof(buf));
  delayMicroseconds(10);

 
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(10);
  }
  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(10);
  Serial1.write(ETX);

}



void data_program(int len, int index)
{
  
  uint8_t buf[len];
  
  for(int p = 0; p<len; p++)
  {
    buf[p] = boot0[index + p];
  }

  Serial1.write(STX);
  delayMicroseconds(20);
  Serial1.write(0);
  delayMicroseconds(20);
    
  for(int i = 0; i < len; i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(20);
  }

  Serial1.write(checksum(len,buf));
  delayMicroseconds(20);
  Serial1.write(0x17);
    
}




void data_program_end(int len, int index)
{
  
  uint8_t buf[len];
  
  for(int p = 0; p<len; p++)
  {
    buf[p] = boot0[index + p];
  }

  Serial1.write(STX);
  delayMicroseconds(20);
  Serial1.write(0);
  delayMicroseconds(20);
    
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(20);
  }

  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(20);
  Serial1.write(ETX);
      
}




void cmd_block_erase(int block_index)
{

  uint8_t addr_upper_lo = (BLK_SIZE*(block_index+1))-1 & 0xff;
  uint8_t addr_upper_md = (BLK_SIZE*(block_index+1)-1 >> 8) & 0xff;
  uint8_t addr_upper_hi = (BLK_SIZE*(block_index+1)-1 >> 16) & 0xff;

  uint8_t addr_lower_lo = (BLK_SIZE*(block_index)) & 0xff;
  uint8_t addr_lower_md = ((BLK_SIZE*(block_index)) >> 8) & 0xff;
  uint8_t addr_lower_hi = ((BLK_SIZE*(block_index)) >> 16) & 0xff;


  uint8_t buf[] = {0x22, addr_lower_hi, addr_lower_md, addr_lower_lo, addr_upper_hi, addr_upper_md, addr_upper_lo};

  Serial1.write(SOH);
  delayMicroseconds(20);
  Serial1.write(sizeof(buf));
  delayMicroseconds(20);

  
  for(int i = 0; i < sizeof(buf); i++)
  {
    Serial1.write(buf[i]);
    delayMicroseconds(20);
  }

  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(20);
  Serial1.write(ETX);
}




void cmd_set_security(void)
{
  
  uint8_t buf[] = {0xa0, 0, 0};

  Serial1.write(SOH);
  delayMicroseconds(100);
  Serial1.write(sizeof(buf));
  delayMicroseconds(100);
  
  for(int i = 0; i < sizeof(buf); i++)
  {  
    Serial1.write(buf[i]);
    delayMicroseconds(100);
  }
  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(100);
  Serial1.write(ETX);
}




void data_set_security(void)
{

  uint8_t buf[] = {0xff,0x03}; // should result in 0x7f on 78k0 cause its signed char

  Serial1.write(STX);
  delayMicroseconds(100);
  Serial1.write(sizeof(buf));
  delayMicroseconds(100);
  
  for(int i = 0; i < sizeof(buf); i++)
  {  
    Serial1.write(buf[i]);
    delayMicroseconds(100);
  }
  
  Serial1.write(checksum(sizeof(buf),buf));
  delayMicroseconds(100);
  Serial1.write(ETX);
}





void log_bytes(void)
{
  
  detect_cnt = 0;
  sec_set = true;


  while(Serial1.available())
  {
    c = Serial1.read();

    if(c == SEC_BYTE)
    {
      sec_set = false;
    }

    if(c == 6)
    {
      reseting = false;
      ackd = true;
      prot = false;
    }

    if(c == 0x10)
    {
      prot = true;
      ackd = false;
      reseting = false;
    }
  
      
    sprintf(outstr, "%02X ",c);       
    Serial.printf(outstr);
  
  }

  if(not ackd and not prot)
    ackd = true;
  
  Serial.printf("\n");
}



void clear_serial_buffer(void)
{
  Serial1.flush();
  while(Serial1.available() > 0)
  Serial1.read(); 
}



void init_flasher_comms(void)
{
  
  Serial1.begin(9600);
  
  enter_flash_mode();
  
  delay(4);
  
  cmd_init();
  
  delay(13);
  
  cmd_baud_set();
  
  delay(10);
  
  Serial1.begin(115200);
  
  delayMicroseconds(5000);
  
  cmd_init();
    
  delayMicroseconds(2000);

  cmd_signature();
  
  delayMicroseconds(1700);

}



void setup() {

  Serial.begin(1000000);  
  Serial1.begin(9600);
  Serial2.begin(115200);

  Serial.print("starting\n");
  
  pinMode(flmd_pin, OUTPUT);
  pinMode(reset_pin, OUTPUT);
  pinMode(glitch_pin, OUTPUT);
  pinMode(detected_pin, OUTPUT);

  digitalWrite(glitch_pin,HIGH);

}



void loop() 
{
  
  init_flasher_comms();

  delayMicroseconds(2000);
  
  cmd_set_security();
  
  delay(2);

  data_set_security();

  // slide around in the glitch window
  slide_us =  100 + (iii % 200);
  slide = random() % 10000;
  len = random() % 400;
  
  delayMicroseconds(slide_us);
  delayNanoseconds(slide);
  
  // glitch regc pin
  digitalWrite(glitch_pin, LOW);
  delayNanoseconds(len);
  digitalWrite(glitch_pin, HIGH);

  // wait a bit for a reply
  delay(40);

  // screen reply
  log_bytes();

  // success
  if(sec_set)
  {
  
    // used for logic analyzer so ignore
    digitalWriteFast(detected_pin, HIGH);
    delayNanoseconds(100);
    digitalWriteFast(detected_pin, LOW);

    
    Serial.print("security set!\n\n");
  
    Serial.print("slide us:");
    Serial.print(slide_us);
    Serial.print("\n");
    Serial.print("slide ns:");
    Serial.print(slide);
    Serial.print("\n");
    Serial.print("length:");
    Serial.print(len);
    Serial.print("\n");
    Serial.print("\n");
    
  }
  while(sec_set);
  
  iii += 1;
}
