/*
 * PolyCrossClock
 * A polyrythm cros-rhythm generating clock with drifting or strict divisions
 * Adam Tindale
 * 2020
 * For Cascadence
 * 
 * INPUT 1 - NOT IMPLEMENTED. Sync Planned.
 * OUTPUT 1 - Clock
 * OUTPUT 2 - Clock Division
 * KNOB 1 - Tempo 30 - 120
 * KNOB 2 - Length of Divisions 1-16 per beat
 * KNOB 3 - Cross 
 * KNOB 4 - Randomness per division step
 */


#include <tinySPI.h>

/// TEMPLATE
//DAC Definitions 
const int GAIN_1 = 0x1;
const int GAIN_2 = 0x0;
const int NO_SHTDWN = 1;
const int SHTDWN = 0;

const int A=0;  //Use A or B to write to DAC
const int B=1;  //This makes it easy to remember which output is which

//pin definitions
const int POTS[4]={0,1,2,3};  //the 4 pots from top to bottom
const int CLK_IN = 8;
const int SW = 7;
//DAC pins
const int MOSI = 6;
const int SCK = 4;
const int PIN_CS = 5;

unsigned int values[4]; //Global array to store potentiometer values

/// Clock Sketch Specific
// 1000 micro seconds in a millisecond => 1 000 000 micros in a second
unsigned long now;
unsigned long nextClockTick;
unsigned long tickDur = 1000000;
unsigned long pulseOff;
unsigned long pulseDur = 40000;
boolean didPulse = true;

unsigned long divisionTick;
unsigned long nextDivisionTick;
unsigned long divisionDur;
unsigned long divisionPulseOff;   
boolean divisionPulse = true;

// Adapted From EuclideanSequencer
#define MAXSTEPLENGTH 15  //+1 because 0 is a step = 16 is the step length
unsigned char cross = 1;
unsigned char divisions = 1;
unsigned char randomness = 0;
unsigned char stepnumber = 0;

void setup(){

  pinMode(POTS[0], INPUT);  //Set up pins as inputs
  pinMode(POTS[1], INPUT);
  pinMode(POTS[2], INPUT);
  pinMode(POTS[3], INPUT);
  pinMode(SW, INPUT);
  pinMode(CLK_IN,INPUT_PULLUP); //Pullup might not be necessary

  //mosi, sck, and pin_cs used for spi dac (mcp4822)
  pinMode(MOSI,OUTPUT);
  pinMode(SCK,OUTPUT);
 
  digitalWrite(PIN_CS,HIGH);  //prepare the dac CS line (active low)
  pinMode(PIN_CS, OUTPUT);  
}



void loop() {
  now = micros();
  
  if(digitalRead(CLK_IN) == LOW)  //we've received a clock pulse!
    {
      //setOutput(B, GAIN_2, NO_SHTDWN, 2000);
      //B or A to choose channels
      //GAIN_2 or GAIN_1 to choose 1x gain or 2x gain (I always use 2x)
      //NO_SHTDWN or SHTDWN to disable the DAC_CS
      //2000 - is the value out of 4096 to send out to the DAC
    }
    
    updatevalues();//subroutine to grab the 4 pot values

    // clock tick
    if ( now >= nextClockTick ){
      setOutput( A, GAIN_2, NO_SHTDWN, 0xFFF );   
      nextClockTick = now + tickDur;  
      pulseOff = now + pulseDur; 
      didPulse = false;
    } else if ( !didPulse ){
      if ( now >= pulseOff ){
       setOutput( 0, GAIN_2, NO_SHTDWN, 0 );
       didPulse = true;
      }
    }


    
     // division tick
    if ( now >= nextDivisionTick ){
      setOutput( B, GAIN_2, NO_SHTDWN, 0xFFF );   
      nextDivisionTick = now + divisionDur;
      divisionPulseOff = now + pulseDur;
      divisionPulse = false;
     } else if ( !divisionPulse ){
      if ( now >= divisionPulseOff ){
        setOutput( B, GAIN_2, NO_SHTDWN, 0 );
        divisionPulse = true;   
      } 
     }
     /*
      pulse=false;
      if(bitRead(seq[B],offset_stepnumber) == 1) //if there's a pulse
        pulse = true;
      if (random(4096) <= randomness ) // might be better to set this to analogReadResolution?
        pulse = !pulse;
      if(pulse == 1)
        SendPulse(A); //send one
  
      stepnumber++;
      if(stepnumber>MAXSTEPLENGTH)
      {
        stepnumber = 0;
      }

      /// stepnumber = ++stepnumber % MAXSTEPLENGTH;
     
     */
    
}


void updatevalues(void){
  
  /*
   * TODO: if switch is left quantized, otherwise quantized? 
  if (SW == HIGH){
  }else{
  }
  */
  
  unsigned char x;
  for( x=0; x < 4 ; x++ ){
    values[x] = analogRead( POTS[x] );  
  }

  tickDur = map( values[0], 0, 1023, 100000, 2000000 );
  
  //From EuclideanSequencer
  divisions = map( values[1],0,1023,1,MAXSTEPLENGTH+1 );
  cross  = map( values[2],0,1023,1,MAXSTEPLENGTH+1 );
  randomness = map( values[3],0,1023,0,MAXSTEPLENGTH );

  divisionDur = tickDur / divisions; 
  
}

/*
 * Not using in order to avoid blocking
void SendPulse (boolean chan) //Sends a 40ms trigger. This function blocks
{
  setOutput(chan, GAIN_2, NO_SHTDWN, 0xFFF);
  delay(40);
  setOutput(chan, GAIN_2, NO_SHTDWN, 0);
}
*/
void setOutput(byte channel, byte gain, byte shutdown, unsigned int val){
  
  byte lowByte = val & 0xff;
  byte highByte = ((val >> 8) & 0xff) | channel << 7 | gain << 5 | shutdown << 4;
  //bit level stuff to make the dac work.
  //channel can be 0 or 1 (A or B)
  //gain can be 0 or 1 (x1 or x2 corresponding to 2V max or 4V max)
  //shutdown is to disable the DAC output, 0 for shutdown, 1 for no shutdown
   
  digitalWrite(PIN_CS, LOW);
  shiftOut(MOSI,SCK,MSBFIRST,highByte);
  shiftOut(MOSI,SCK,MSBFIRST,lowByte);
  digitalWrite(PIN_CS, HIGH);
}


// Function to find the binary length of a number by counting bitwise 
int findlength(long int bnry){
  boolean lengthfound = false;
  int length=1; // no number can have a length of zero - single 0 has a length of one, but no 1s for the sytem to count
  for (int q=32;q>=0;q--){
    int r=bitRead(bnry,q);
    if(r==1 && lengthfound == false){
      length=q+1;
      lengthfound = true;
    }
  }
  return length;
}
