#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// [Display Commands and Parameters]
   // system set commands and parameters
   #define C_SYS_SET 0b01000000     // system set command
      // VC (characters per line) for 8-pixel characters = 40
      // VC (characters per line) for 16-pixel characters = 20
      // DISP_HEIGHT = 240 (display height)
      // VD = 320 (display width)
      // character width = height?
      // parameter VC (characters per line) = 40 for small, 20 for large
      // parameter FX (character width in pixels) = 8 for small, 16 for large
      // C/R (characters per row) = 40 for small, 40 for large
      // TC/R (total characters per row >= C/R + 2 >= 42)
      // L/F = P_DISP_HEIGHT = 240
      // f_SYSCLK to be determined in-program (see page 102 of controller datasheet)
   #define P_SYS_SET_P1_SMALL 0b00100000  // Parameter 1 of System Set for 8-pixel characters (see pgs. 48, 71 of datasheet)
   #define P_SYS_SET_P1_LARGE 0b00100100  // Parameter 1 of System Set for 16-pixel characters (see pgs. 48, 71 of datasheet)
   #define P_SYS_SET_P2_SMALL 0b00000111  // Parameter 2 of System Set for 16-line AC drive and 8-pixel characters (see pgs. 50, 71 of datasheet)
   #define P_SYS_SET_P2_LARGE 0b00001111  // Parameter 2 of System Set for 16-line AC drive and 16-pixel characters (see pgs. 51, 71 of datasheet)
   #define P_SYS_SET_P3_SMALL 0b00000111  // Parameter 3 for 8-bit high characters (see pgs. 50, 71 of datasheet)
   #define P_SYS_SET_P3_LARGE 0b00001111  // Parameter 3 for 16-bit high characters (see pgs. 50, 71 of datasheet)
   #define P_SYS_SET_P4 0b01001111        // (C/R * bpp) - 1
   #define P_SYS_SET_P5 0b00101011        // TC/R + 1
   #define P_SYS_SET_P6 0b11101111        // Parameter 6 is the frame height in lines - 1 (frame height = screen height)
   #define P_SYS_SET_P7 0b00101000        // part 1 of Horizontal Address Range (set equal to C/R)
   #define P_SYS_SET_P8 0b00000000        // part 2 of Horizontal Address Range (set equal to C/R)
   
   // Display-On Command and Parameter
   #define C_DISP_ON 0b01011001              // turn display on
   #define P_DISP_ATTRIB_CURSOR 0b00000110   // set display attributes to have only 1 screen block on (no flashing) and cursor set to blink @ 1 Hz
   #define P_DISP_ATTRIB_NOCURSOR 0b00000100 // set display attributes to have only 1 screen block on (no flashing) and cursor off
   #define P_DISP_ATTRIB_DUAL_CURSOR 0b00010110   // set display attributes to have screen blocks 1-2 on (no flashing) and cursor set to blink @ 1 Hz
   #define P_DISP_ATTRIB__DUAL_NOCURSOR 0b00010100 // set display attributes to have screen blocks 1-2 on (no flashing) and cursor off 
   #define P_DISP_ATTRIB__TRIPLE_CURSOR 0b01010110   // set display attributes to have all 3 screen blocks on (no flashing) and cursor set to blink @ 1 Hz
   #define P_DISP_ATTRIB__TRIPLE_NOCURSOR 0b01010100 // set display attributes to have all 3 screen blocks on (no flashing) and cursor off 
   
   // Display-Off Command and Parameter
   #define C_DISP_OFF 0b01011000             // turn display off
      // use P_DISP_ATTRIB and P_DISP_ATTRIB_NOCURSOR in same way as for the DISP_ON command
      
   // Scroll Command
   #define C_SCROLL 0b01000100
   #define P_SCROLL_P1 0b00000000         // mono or tri mode screen block 1 start address = 0 (bits 7-0 of address)
   #define P_SCROLL_P2 0b00000000         // mono or tri mode screen block 1 start address = 0 (bits 15-8 of address)
   #define P_SCROLL_P1_DUAL 0b11110000    // dual mode screen block 1 start address = 9200 (bits 7-0 of address)
   #define P_SCROLL_P2_DUAL 0b00100011    // dual mode screen block 1 start address = 9200 (bits 15-8 of address)
   #define P_SCROLL_P3_MONO 0b11101111    // screen block 1 size for only 1 screen block active (240 lines)
   #define P_SCROLL_P3_MULTI 0b00001001   // screen block 1 size for 2 active screen blocks (10 lines)
   #define P_SCROLL_P3_TRI 0b11100101     // screen block 1 size for 3 active screen blocks (230 lines)
   #define P_SCROLL_P4_MONO 0b10000000    // mono mode screen block 2 start address = 9600 (bits 7-0 of address)
   #define P_SCROLL_P5_MONO 0b00100101    // mono mode screen block 2 start address = 9600 (bits 15-8 of address)
   #define P_SCROLL_P4_DUAL 0b00000000    // dual mode screen block 2 start adddress = 0 (bits 7-0 of address)
   #define P_SCROLL_P5_DUAL 0b00000000    // dual mode screen block 2 start adddress = 0 (bits 15-8 of address)
   #define P_SCROLL_P4_TRI 0b00000000     // tri mode screen block 2 start adddress = 0 (bits 7-0 of address)
   #define P_SCROLL_P5_TRI 0b00000000     // tri mode screen block 2 start adddress = 0 (bits 15-8 of address)
   #define P_SCROLL_P6_MONO 0b00000000    // screen block 2 size for 1 active screen blocks (0 lines)
   #define P_SCROLL_P6_DUAL 0b11100101    // screen block 2 size for 2 active screen blocks (230 lines)
   #define P_SCROLL_P6_TRI 0b11100101     // screen block 2 size for 3 active screen blocks (230 lines)
   #define P_SCROLL_P7_MONO 0b10000001    // mono mode screen block 3 start address = 9601 (bits 7-0 of address)
   #define P_SCROLL_P8_MONO 0b00100101    // mono mode screen block 3 start adddress = 9601 (bits 15-8 of address)
   #define P_SCROLL_P7_DUAL 0b10000000    // dual mode screen block 3 start address = 9600 (bits 7-0 of address)
   #define P_SCROLL_P8_DUAL 0b00100101    // dual mode screen block 3 start adddress = 9600 (bits 15-8 of address)
   #define P_SCROLL_P7_TRI 0b11110000     // tri mode screen block 3 start address = 9200 (bits 7-0 of address)
   #define P_SCROLL_P8_TRI 0b00100011     // tri mode screen block 3 start adddress = 9200 (bits 15-8 of address)
      // (parameters P9 and P10 are not used or set)
   
   // CSRFORM Command (Cursor Settings)
   #define C_CSRFORM 0b01011101
   #define P_CSRFORM_P1_SMALL 0b00000111  // cursor width for 8-pixel characters
   #define P_CSRFORM_P1_LARGE 0b00001111  // cursor width for 16-pixel characters
   #define P_CSRFORM_P2_SMALL 0b00000111  // cursor height for 8-pixel characters and underscore-type cursor
   #define P_CSRFORM_P2_LARGE 0b00001111  // cursor height for 16-pixel characters and underscore-type cursor
   #define P_CSRFORM_P2__BLK_LARGE 0b10001111  // cursor height for 16-pixel characters and block-type cursor
   #define P_CSRFORM_P2__BLK_SMALL 0b10000111  // cursor height for 8-pixel characters and block-type cursor
   
   
   // CSRDIR Command (Automatic Cursor Shift Direction)
   #define C_CSRDIR_RIGHT 0b01001100
   #define C_CSRDIR_LEFT 0b01001101
   #define C_CSRDIR_UP 0b01001110
   #define C_CSRDIR_DOWN 0b01001111
   
   // Overlay Command
   #define C_OVERLAY 0b01011011
   #define P_OVERLAY 0b00000000  // overlay parameter for mixed text/graphics with 2 layers
   
   // CGRAM skipped (CGRAM not used)
   
   // HDOT SCR Command (Sets Horizontal Scroll Position)
   #define C_HDOT_SCR 0b01011010
   #define P_HDOT_SCR 0b00000000 // horizontal scrolling off
   
   // CSRW Command (sets cursor address)
   #define C_CSRW 0b01000110
      // P_CSRW set in-program (address of cursor)
   
   // (CSRR command not used)
   
   // Grayscale Command
   #define C_GRAYSCALE 0b01100000
   #define P_GRAYSCALE 0b00000001   // (sets bits per pixel = 2)
   
   // MEMWRITE Command (writes to memory at cursor address)
   #define C_MEMWRITE 0b01000010
   
   // (Character Codes Defs not needed - just use ASCII codes)

// [Port B Output Pin Definitions]
   #define BUTTON_PIN 0b00100000
   #define RESET_PIN 0b00010000
   #define CLOCK_PIN 0b00001000
   #define A0_PIN 0b00000100        // used to distinguish between commands and data/parameters
   #define DB7_PIN 0b00000010       // data pin 7
   #define DB6_PIN 0b00000001       // data pin 6

// (For Port D, display pins DB5-DB0 correspond to Port D Pins 7-2)

// [Program Constants]
   #define IN_BUFFER_SIZE 540            // size of inBuffer character array in bytes
   #define RESET_DELAY_DURATION 6        // duration of initial reset period in milliseconds
   #define EQ_BUFFER_SIZE 120            // size of an equation holder
   #define WINDOW_BOUNDS_SIZE 6          // size of array holding window bounds (in entries, not bytes)
   #define TEXT_BUFFER_SIZE 200          // size of temporal buffer holding command line text
   #define FUNCTION_TEXT_SEL_BUFFER_SIZE 3 // size of buffer for function selection

// global volatile variables for display output
//    > char byteToSend
//    > char byteSent
//    > keypad input buffer
//    > input buffer write index
//    > int numDClockIntervals

volatile unsigned char byteToSend;           // byte that is to be sent (or was last sent)
volatile char byteAwaitingTransmission;      // flag to tell interrupt it needs to send byteToSend
volatile char isCommand;                     // flag used to determine whether A0 will be low or high
volatile char buttonInput;
volatile char prevButtonInput;
volatile char buttonInputRead;
//volatile unsigned char* volatile inBuffer;   // buffer for keypad input
//volatile unsigned char prevInput;            // used to ensure accurate keypress detection (always 1 character per button push/release)
//volatile unsigned int nextBufferIndex;       // used to determine next index available to write to in buffer (unless buffer is full)

// timer 0 is used to generate the display's clock signal
static inline void initTimer0(void)
{
   TCCR0A |=  (1 << WGM01);   // put timer in CTC mode
   TCCR0B |= (1 << CS01);     // timer counts every 8 clock cycles
   TIMSK0 |= (1 << OCIE0A);   // set to interrupt on compare
   OCR0A = 10; // interrupt triggered every 80 clock cycles to generate display clock (~200 kHz)
   TCNT0 = 0;
}

// timer 1 is used for polling the keypad once every ~80 milliseconds (doubles as a debouncer)
static inline void initTimer1(void)
{
   TCCR1B |= ((1 << WGM12) | (1 << CS12) | (1 << CS10));
   TIMSK1 |= (1 << OCIE1A);
   OCR1A = 1250;
   sei();
   TCNT1 = 0;
}

ISR(TIMER0_COMPA_vect)
{
   PORTB ^= CLOCK_PIN;
   if (((PORTB & CLOCK_PIN) == CLOCK_PIN) && (byteAwaitingTransmission == '1')) {
      if (isCommand == '1') {
         PORTB |= A0_PIN;
      } else if ((PORTB & A0_PIN) == A0_PIN) {
         PORTB ^= A0_PIN;
      }
      int i;
      for (i = 7; i >= 0; i--) {
         if (i >= 6) {
            unsigned char mask;
            if (i == 7) {
               mask = DB7_PIN;
            } else {
               mask = DB6_PIN;
            }
            if ((byteToSend & (1 << i)) == (1 << i)) {
               PORTB |= mask;
            } else if ((PORTB & mask) == mask) {
               PORTB ^= mask;
            }
         } else if ((byteToSend & (1 << i)) == (1 << i)) {
            PORTD |= (1 << (i+2));
         } else if ((PORTD & (1 << (i+2))) == (1 << (i+2))) {
            PORTD ^= (1 << (i+2));
         }
      }
      byteAwaitingTransmission = '0';
   } else if ((PORTB & CLOCK_PIN) == CLOCK_PIN) {
      PORTB = CLOCK_PIN;
      PORTD = 0b00000000;
   }/* else {
      PORTB = 0b00000000;
      PORTD = 0b00000000;
   }*/   
}

ISR(TIMER1_COMPA_vect)
{
   sei();   // allow ISR for timer0 to interrupt this ISR
   char rawButtonInput;
   if ((PORTB & BUTTON_PIN) == BUTTON_PIN) {
      rawButtonInput = '1';
      if ((prevButtonInput == '0')) {
         buttonInput = '1';
         buttonRead = '0';
      } else if (buttonRead == '0') {
         buttonInput = '1';
      } else {
         buttonInput = '0';
      }      
   } else {
      rawButtonInput = '0';
         if ((prevButtonInput == '1') && (buttonRead == '0')) {
            buttonInput = '1';
         } else {
            buttonInput = '0';
         }      
   }            
   prevButtonInput = rawButtonInput;
}


int main(void)
{
   byteToSend = 0b00000000;
   byteAwaitingTransmission = '0';
   isCommand = '0';
   prevButtonInput = '0';
   rawButtonInput = '0';
   buttonInput = '0';
   //inBuffer = (volatile unsigned char * ) malloc(IN_BUFFER_SIZE);
   //fillWithNulls(inBuffer);
   //prevInput = 0b00000000;
   //nextBufferIndex = 0;
   DDRB = 0b00011111;
   DDRD = 0b11111110;
   PORTB = 0b00000000;
   PORTD = 0b00000000;
   PORTB |= RESET_PIN;
   _delay_ms(RESET_DELAY_DURATION);
   PORTB = 0b00000000;
   initTimer0();
   initTimer1();
   sei();
   initDisplay();
//    char prevMode = 'c';
   int mode = 0;
//    char altFunction = '0';
//    int textCursorPos = 0;
//    char currentEquation = '0';
//    char specialFunctionPasted = '0';
//    int currentSpecFuncType = -1;
   //textCursorPos = drawCommandLine(textBuffer, textCursorPos);
   while (1) {
      if ((buttonInput == '1') && (buttonRead == '0')) {
         buttonRead = '1';
         if (mode < 2) {
            mode++;
         } else {
            mode = 0;
         }   
         switch (mode) {
            case 0:
               drawPosLine();
               break;
            case 1:
               drawNegLine();
               break;
            case 2:
               drawParabola();
               break;
         }
      }
   }
}

int drawParabola() {
   clearDisplay();
   double xStepSize = ((X_MAX-X_MIN) / SCREEN_WIDTH);
   double x = X_MIN;
   while (x < X_XMAX) {
      double y = (x*x);
      double invertedY = (YMAX - y);
      if ((invertedY <= YMAX) && (invertedY >= YMIN)) {
         int scaledY = ((int) (invertedY * SCREEN_HEIGHT)));
         int scaledX = ((int) ((x - X_MIN);
         drawPoint(scaledX, scaledY);
      }
      x += xStepSize;
   }
   return 0;
}

int drawNegLine() {
   clearDisplay();
   double xStepSize = ((X_MAX-X_MIN) / SCREEN_WIDTH);
   double x = X_MIN;
   while (x < X_XMAX) {
      double y = (-1.0 * x);
      double invertedY = (YMAX - y);
      if ((invertedY <= YMAX) && (invertedY >= YMIN)) {
         int scaledY = ((int) (invertedY * SCREEN_HEIGHT)));
         int scaledX = ((int) ((x - X_MIN);
         drawPoint(scaledX, scaledY);
      }
      x += xStepSize;
   }
   return 0;
}

int drawPosLine() {
   clearDisplay();
   double xStepSize = ((X_MAX-X_MIN) / SCREEN_WIDTH);
   double x = X_MIN;
   while (x < X_XMAX) {
      double y = x;
      double invertedY = (YMAX - y);
      if ((invertedY <= YMAX) && (invertedY >= YMIN)) {
         int scaledY = ((int) (invertedY * SCREEN_HEIGHT)));
         int scaledX = ((int) ((x - X_MIN);
         drawPoint(scaledX, scaledY);
      }
      x += xStepSize;
   }
   return 0;
}

int drawPoint(scaledX, scaledY)
{
   

int initDisplay()
{
   systemSet();
   _delay_ms(5);
   setScroll();
   _delay_ms(5);
   setHDOT_SCR();
   _delay_ms(5);
   setOverlay();
   _delay_ms(5);
   setDispState('0');
   _delay_ms(5);
   clearAllDisplayMemory();
   _delay_ms(5);
   setCSRW();
   _delay_ms(5);
   setCSRForm();
   _delay_ms(5);
   setDispState('1');
   _delay_ms(5);
   return 0;
}

int systemSet()
{
   sendByteToDisplay(C_SYS_SET, '1');
   sendByteToDisplay(P_SYS_SET_P1_SMALL, '0');
   sendByteToDisplay(P_SYS_SET_P2_SMALL, '0');
   sendByteToDisplay(P_SYS_SET_P3_SMALL, '0');
   sendByteToDisplay(P_SYS_SET_P4, '0');
   sendByteToDisplay(P_SYS_SET_P5, '0');
   sendByteToDisplay(P_SYS_SET_P6, '0');
   sendByteToDisplay(P_SYS_SET_P7, '0');
   sendByteToDisplay(P_SYS_SET_P8, '0');
   return 0;
}

int setScroll()
{
   sendByteToDisplay(C_SCROLL, '1');
   sendByteToDisplay(P_SCROLL_P1, '0');
   sendByteToDisplay(P_SCROLL_P2, '0');
   sendByteToDisplay(P_SCROLL_P3_MONO, '0');
   sendByteToDisplay(P_SCROLL_P4_MONO, '0');
   sendByteToDisplay(P_SCROLL_P5_MONO, '0');
   sendByteToDisplay(P_SCROLL_P6_MONO, '0');
   sendByteToDisplay(P_SCROLL_P7_MONO, '0');
   sendByteToDisplay(P_SCROLL_P8_MONO, '0');
   return 0;
}

int setHDOT_SCR()
{
   sendByteToDisplay(C_HDOT_SCR, '1');
   sendByteToDisplay(P_HDOT_SCR, '0');
   return 0;
}

int setOverlay()
{
   sendByteToDisplay(C_OVERLAY, '1');
   sendByteToDisplay(P_OVERLAY, '0'); 
   return 0; 
}

int setDispState(char on)
{
   if (on == '1') {
      sendByteToDisplay(C_DISP_ON, '1');
   } else {
      sendByteToDisplay(C_DISP_OFF, '1');
   }      
   sendByteToDisplay(P_DISP_ATTRIB_NOCURSOR, '0');
   return 0;
}

int clearAllDisplayMemory()
{
   unsigned char i;
   unsigned char j;
   for (i = 0; i < 0b11111111; i++) {
      for (j = 0; j < 0b01111111; j++) {
         sendByteToDisplay(C_CSRW, '1');
         sendByteToDisplay(i, '0');
         sendByteToDisplay(j, '0');
         sendByteToDisplay(C_MEMWRITE, '1');
         sendByteToDisplay(0b00000000, '0');
      }
   }
   return 0;
}

int setCSRW()
{
   sendByteToDisplay(C_CSRW, '1');
   sendByteToDisplay(0b00000000, '0');
   sendByteToDispay(0b00000000, '0');
   return 0;
}

int setCSRForm()
{
   sendByteToDisplay(C_CSRFORM, '1');
   sendByteToDisplay(P_CSRFORM_P1_SMALL, '0');
   sendByteToDisplay(P_CSRFORM_P2_SMALL, '0');
   return 0;
}

int sendByteToDisplay(unsigned char currentByte, char currentIsCommand)
{
   isCommand = currentIsCommand;
   byteToSend = currentByte;
   byteAwaitingTransmission = '1';
   while (byteAwaitingTransmission == '1') {
      _delay_us(1);
   }
   return 0;
}      
   
   