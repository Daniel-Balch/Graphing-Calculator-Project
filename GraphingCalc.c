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
   #define LED_PIN 0b00100000       // used for LED to indicate alt function use
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
volatile unsigned char* volatile inBuffer;   // buffer for keypad input
volatile unsigned char prevInput;            // used to ensure accurate keypress detection (always 1 character per button push/release)
volatile unsigned int nextBufferIndex;       // used to determine next index available to write to in buffer (unless buffer is full)

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
   unsigned char currentInput = getKeypadInput();
   char isValidInput = checkValidInput(currentInput, prevInput);
   if ((isValidInput == '1') && (inBuffer != NULL)) {
      if (nextBufferIndex < ((sizeOf(inBuffer))-1)) {
         inBuffer[nextBufferIndex] = currentInput;
         nextBufferIndex++;
      } else {
         removeFromString(inBuffer, 0);
         inBuffer[(sizeOf(inBuffer)-1)] = currentInput;
         nextBufferIndex = ((sizeOf(inBuffer))-1);
      }
   }
   prevInput = currentInput;
}

int main(void)
{
   byteToSend = 0b00000000;
   byteAwaitingTransmission = '0';
   isCommand = '0';
   inBuffer = (volatile unsigned char * ) malloc(IN_BUFFER_SIZE);
   fillWithNulls(inBuffer);
   prevInput = 0b00000000;
   nextBufferIndex = 0;
   DDRB = 0b00111111;
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
   char *equationA = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equA);
   char *equationB = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equB);
   char *equationC = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equC);
   char *equationD = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equD);
   char *equationE = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equE);
   char *equationF = (char * ) malloc(EQ_BUFFER_SIZE);
   fillWithNulls(equF);
   char *textBuffer = (char * ) malloc(TEXT_BUFFER_SIZE);
   fillWithNulls(textBuffer);
   char *functionTextSelBuffer = (char * ) malloc(FUNCTION_TEXT_SEL_BUFFER_SIZE);
   fillWithNulls(functionTextSelBuffer);
   int textBufferIndex = 0;
   int equationIndex = 0;
   int functionIndex = 0;
   double *windowBounds = (double * ) malloc((WINDOW_BOUNDS_SIZE*sizeOf(double)));
   char prevMode = 'c';
   char mode = 'c';
   char altFunction = '0';
   int textCursorPos = 0;
   char currentEquation = '0';
   char specialFunctionPasted = '0';
   int currentSpecFuncType = -1;
   textCursorPos = drawCommandLine(textBuffer, textCursorPos);
   while (1) {
      while (nextBufferIndex > 0) {
         unsigned char currentRawChar = inBuffer[0];
         char currentChar = decodeRawChar(currentRawChar, altFunction);
         char currentInputType = getInputType(currentChar, mode);
         switch (currentInputType) {
            case 'p':
               if (mode == 'c') {
                  if (textBufferIndex < (sizeOf(textBuffer)-1)) {
                     textBuffer[textBufferIndex] = currentChar;
                     textBufferIndex++;
                     textCursorPos = drawCharacter(currentChar, textCursorPos, '0');
                  }
               } else if (mode == 'e') {
                  switch (currentEquation) {
                     case 'a':
                        if (equationIndex < (sizeOf(equA)-1)) {
                           equA[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        }
                        break;
                     case 'b':
                        if (equationIndex < (sizeOf(equB)-1)) {
                           equB[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        } 
                        break;
                     case 'c':
                        if (equationIndex < (sizeOf(equC)-1)) {
                           equC[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        }
                        break;
                     case 'd':
                        if (equationIndex < (sizeOf(equD)-1)) {
                           equD[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        }
                        break;
                     case 'e':
                        if (equationIndex < (sizeOf(equE)-1)) {
                           equE[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        }
                        break;
                     case 'f':
                        if (equationIndex < (sizeOf(equF)-1)) {
                           equF[equationIndex] = currentChar;
                           equationIndex++;
                           textCursorPos = drawCharacter(currentChar, textCursorPos, '0');  
                        }
                        break;
                  }
               } else if (mode == 'f') {
                  if (functionIndex < (sizeOf(functionTextSelBuffer)-1)) {
                     functionTextSelBuffer(functionIndex) = currentChar;
                     functionIndex++;
                     textCursorPos = drawCharacter(currentChar, textCursorPos, '0');
                  }
               }      
               break;
            case 'a':
               PORTB ^= LED_PIN;
               if (altFunction == '0') {
                  altFunction = '1';
               } else {
                  altFunction = '0';
               }
               break;
            case 't':
               prevMode = mode;
               mode = getNextMode(currentChar);
               switch (mode) {
                  case 'c':
                     textCursorPos = drawCommandLine(textBuffer, textCursorPos);
                     break;
                  case 'g':
                     drawGraph(equA, equB, equC, equD, equE, equF, windowBounds);
                     break;
                  case 'e':
                     switch (currentChar) {
                        case 1:
                           currentEquation = 'a';
                           break;
                        case 2:
                           currentEquation = 'b';
                           break;
                        case 3:
                           currentEquation = 'c';
                           break;
                        case 4:
                           currentEquation = 'd';
                           break;
                        case 5:
                           currentEquation = 'e';
                           break;
                        case 6:
                           currentEquation = 'f';
                           break;
                     }      
                     textCursorPos = drawEquationScreen(equA, equB, equC, equD, equE, equF, currentEquation);
                     equationIndex = textCursorPos;
                     break;
                  case 'f':
                     textCursorPos = drawSpecialFunctionsScreen(prevMode, functionTextSelBuffer);
                     functionIndex = (sizeOf(SPEC_FUNC_TEXT) - 1 - textCursorPos);
                     break;
                  case 'm':
                     drawMenuScreen(prevMode);
                     break;
                  case 'q':
                     drawEquationsMenuScreen(equA, equB, equC, equD, equE, equF);
                     break;
               }         
               break;
            case 'e':
               if (mode == 'c') {
                  textCursorPos = printCmdOutput(textCursorPos, textBuffer);
                  if (specialFunctionPasted == '1') {
                     if (currentSpecFuncType == 1) {
                        textCursorPos = drawCommandLine(textBuffer, textCursorPos);
                     } else if (currentSpecFunctionType == 2) {
                        updateWindowBounds(windowBounds, textBuffer);
                     }   
                     specialFunctionPasted = '0';
                     currentSpecFunctionType = -1;
                  }      
                  textCursorPos = clearBuffer(textCursorPos, textBuffer);
                  textBufferIndex = 0;
               } else if (mode == 'e') {
                  switch (currentEquation) {
                     case 'a':
                        checkValidExpression(equA, '1');
                        break;
                     case 'b':
                        checkValidExpression(equB, '1');
                        break;
                     case 'c':
                        checkValidExpression(equC, '1');
                        break;
                     case 'd':
                        checkValidExpression(equD, '1');
                        break;
                     case 'e':
                        checkValidExpression(equE, '1');
                        break;
                     case 'f':
                        checkValidExpression(equF, '1');
                        break;
                  }
                  prevMode = mode;
                  mode = 'q';
                  drawEquationsMenuScreen(equA, equB, equC, equD, equE, equF);
               } else if (mode == 'f') {
                  int functionChoice = parseFunctionChoice(functionTextSelBuffer);
                  if (functionChoice >= 0) {
                     currentSpecFuncType = functionChoice;
                     mode = prevMode;
                     prevMode = 'f';
                     if (mode == 'c') {
                        textCursorPos = drawCommandLine(textBuffer, textCursorPos);
                        int prevTextCursorPos = textCursorPos;
                        textCursorPos = pasteSpecialFunction(functionChoice, textBuffer, textCursorPos);
                        textBufferIndex += (textCursorPos - prevTextCursorPos);
                     } else if (mode == 'e') {
                        textCursorPos = drawEquationScreen(equA, equB, equC, equD, equE, equF, currentEquation);
                        int prevTextCursorPos = textCursosPos;
                        switch (currentEquation) {
                           case 'a':
                              textCursorPos = pasteSpecialFunction(functionChoice, equA, textCursorPos);
                              break;
                           case 'b':
                              textCursorPos = pasteSpecialFunction(functionChoice, equB, textCursorPos);
                              break;
                           case 'c':
                              textCursorPos = pasteSpecialFunction(functionChoice, equC, textCursorPos);
                              break;
                           case 'd':
                              textCursorPos = pasteSpecialFunction(functionChoice, equD, textCursorPos);
                              break;
                           case 'e':
                              textCursorPos = pasteSpecialFunction(functionChoice, equE, textCursorPos);
                              break;
                           case 'f':
                              textCursorPos = pasteSpecialFunction(functionChoice, equF, textCursorPos);
                              break;
                        }
                        equationIndex += (textCursorPos - prevTextCursorPos);
                     }         
                     specialFunctionPasted = '1';
                  }
               }   
               break;
            case 'c':
               int offset = 0;
               if (currentChar == '<') {
                  if ((textBufferIndex > 0) && (mode == 'c')) {
                     offset = -1;
                     textBufferIndex--;
                  } else if ((equationIndex > 0) && (mode == 'e') {
                     offset = -1;
                     equationIndex--;
                  } else if ((functionIndex > 0) && (mode == 'f') {
                     offset = -1;
                     functionIndex--;
                  }      
               } else if (currentChar == '>') {
                  if ((textBufferIndex < (strlen(textBuffer)-1)) && (mode == 'c')) {
                     offset = 1;
                     textBufferIndex++;
                  } else if (mode == 'e') {
                     int currEqLength = 0;
                     switch (currentEquation) {
                        case 'a':
                           currEqLength = (strlen(equA)-1);
                           break;
                        case 'b':
                           currEqLength = (strlen(equB)-1);
                           break;
                        case 'c':
                           currEqLength = (strlen(eqC)-1);
                           break;
                        case 'd':
                           currEqLength = (strlen(equD)-1);
                           break;
                        case 'e':
                           currEqLength = (strlen(equE)-1);
                           break;
                        case 'f':
                           currEqLength = (strlen(equF)-1);
                           break;
                     }
                     if (equationIndex < currEqLength) {
                        offset = 1;
                        equationIndex++;
                     }         
                  } else if ((mode == 'f') && (functionIndex < (strlen(functionTextSelBuffer)-1))) {
                     offset = 1;
                     functionIndex++;
                  }   
               }    
               textCursorPos = moveTextCursor(textCursorPos, offset);
               updateScreenCursor(textCursorPos);   
               break;
            case 'd':
               if (mode == 'c') {
                  removeFromString(textBuffer, textBufferIndex); 
               } else if (mode == 'f') {
                  removeFromString(functionTextSelBuffer, functionIndex);
               } else if (mode == 'e') {
                  switch (currentEquation) {
                     case 'a':
                        removeFromString(equA, equationIndex);
                        break;
                     case 'b':
                        removeFromString(equB, equationIndex);
                        break;
                     case 'c':
                        removeFromString(equC, equationIndex);
                        break;
                     case 'd':
                        removeFromString(equD, equationIndex);
                        break;
                     case 'e':
                        removeFromString(equE, equationIndex);
                        break;
                     case 'f':
                        removeFromString(equF, equationIndex);
                        break;
                  }
               }           
               break;
         }
         removeFromString(inBuffer, 0);
         nextBufferIndex--;
      }
   }
}

char checkValidInput(unsigned char currentInput, unsigned char prevInput)
{
   if (currentInput == prevInput) {
      return '0';
   } else {
      int numInput = ((int) currentInput);
      if ((numInput >= 1) && (numInput <= 20)) {
         return '1';
      } else {
         return '0';
      }
   }
}            

void initDisplay()
{
   