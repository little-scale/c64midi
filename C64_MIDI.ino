/* C64MIDI
 
 by Sebastian Tomczak
 August 2009
 
 */

#include <math.h> 
#include <avr/pgmspace.h>



// SID setup
byte SID[24];
byte pitches[3];
double PAL = 985000;
double NTSC = 1023000;
double clock = PAL;
double frequency; // working byte for frequency calculations

int oct_div = 12;
double bend[] = {
  12, 12, 12};

byte previous_address; 

int delay_time = 180;
int transposition = 0; 


// Physical Pins
int wr = 9; 
int ad = 8; 


// Definitions
byte dataIn; // general working byte for serially-received data

byte channel;
byte pitch; 
byte velocity;
byte ccnumber; 
byte ccvalue; 
byte bendLSB;
byte bendMSB;
byte rstat;

int flag_previous = 0; // keeps track of the previus MIDI byte type received 

/* flag_previous meanings: 
 -1 = note off status
 -2 = note off pitch
 0 = no action / waiting
 1 = note on status
 2 = pitch
 3 = cc status
 4 = cc number
 */





// Setup
void setup() {
  Serial.begin(31250);
  DDRD = DDRD | B11000000;
  DDRC = DDRC | B00111111;
  pinMode(wr, OUTPUT);
  pinMode(ad, OUTPUT);

  digitalWrite(ad, LOW);
  digitalWrite(wr, LOW);

  doCC(0, 29, 127);

  for(int i = 0; i < 5; i ++) {

    doCC(i, 14, 127);
    doCC(i, 18, 0);
    doCC(i, 19, 0);
    doCC(i, 20, 80);

  }

  for(int i = 0; i < 4; i ++) {
    doNote(0, 24 + (i * 7), 127);
    doNote(1, 29 + (i * 7), 127);
    doNote(2, 34 + (i * 7), 127);
    delay(400);
    doNote(0, 24 + (i * 7), 0);
    doNote(1, 29 + (i * 7), 0);
    doNote(2, 34 + (i * 7), 0);
    delay(100);
  }

}


// Main Program
void loop() {





  

  // start main loop
  if(Serial.available() > 0) {
    dataIn = Serial.read();
    if(dataIn < 0x80 && flag_previous == 0) {
      doMidiIn(rstat);
    }

    doMidiIn(dataIn);
  }
  // end main loop
}



// Functions
void doNote(byte channel, byte pitch, byte velocity) {
  if(channel >= 0 && channel <= 2) {
    if(velocity > 0) {
      pitch = pitch + transposition; 
      
      // determine frequency
      pitches[channel] = pitch; //  save pitch data for later use with bends
      frequency = (440 * pow(2, ((pitch - 69 + bend[channel]) / oct_div)) ) / (clock / 16777216); // calculate frequency

      SID[(channel * 7) + 0]  = int(frequency) % 256; // save low frequency
      SID[(channel * 7) + 1] = int(frequency / 256); // save high frequency

      SID[(channel * 7) + 6] = (SID[(channel * 7) + 6] & B00001111) | ((velocity >> 3) << 4); // set and save sustain volume based on velocity
      SID[(channel * 7) + 4] = (SID[(channel * 7) + 4] & B11111110) | B00000001; // save note on event

      
      writeC64((channel * 7) + 0, SID[(channel * 7) + 0]); //  write frequency low
      writeC64((channel * 7) + 1, SID[(channel * 7) + 1]); // write frequency high
      writeC64((channel * 7) + 6, SID[(channel * 7) + 6]); // write sustain data
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]); // write note on event

      
    }

    else {
      SID[(channel * 7) + 4] = SID[(channel * 7) + 4] & B11111110; // save note off event
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]); // write note off event
    }

  }

}

void doNoteOff(byte channel, byte pitch, byte velocity) {
  if(channel >= 0 && channel <= 2) {
    SID[(channel * 7) + 4] = SID[(channel * 7) + 4] & B11111110; // save note off event
    writeC64((channel * 7) + 4, SID[(channel * 7) + 4]); // write note off event
  }
}

void doCC(byte channel, byte ccnumber, byte ccvalue) {
  if(channel >= 0 && channel <= 2) {
    switch(ccnumber) {

    case 1: // set pulse width - coarse
      SID[(channel * 7) + 2] = (SID[(channel * 7) + 2] & B00011111) | (ccvalue % 8) << 5; // save lower 3 bits of value
      writeC64((channel * 7) + 2, SID[(channel * 7) + 2]); // write lower 3 bits of value
      writeC64((channel * 7) + 3, ccvalue >> 3); // write upper 4 bits of value

      break;


    case 12: // set pulse width - fine
      SID[(channel * 7) + 2] = (SID[(channel * 7) + 2] & B11100000) | ccvalue >> 2; // save lower 5 bits of value
      writeC64((channel * 7) + 2, SID[(channel * 7) + 2]); // write lower 5 bits of value

      break;


    case 14: // enable triangle
      SID[(channel * 7) + 4] = (SID[(channel * 7) + 4] & B11101111) | ((ccvalue >> 6) << 4);
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]);

      break; 


    case 15: // enable sawtooth
      SID[(channel * 7) + 4] = (SID[(channel * 7) + 4] & B11011111) | ((ccvalue >> 6) << 5);
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]);

      break; 


    case 16: // enable square wave
      SID[(channel * 7) + 4] = (SID[(channel * 7) + 4] & B10111111) | ((ccvalue >> 6) << 6);
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]);

      break;


    case 17: // set noise, turn others off
      SID[(channel * 7) + 4] = (SID[(channel * 7) + 4] & B00001111) | ((ccvalue >> 6) << 7);
      writeC64((channel * 7) + 4, SID[(channel * 7) + 4]);

      break;


    case 18: // set attack
      SID[(channel * 7) + 5] = (SID[(channel * 7) + 5] & B00001111) | ((ccvalue >> 3) << 4);
      writeC64((channel * 7) + 5, SID[(channel * 7) + 5]);

      break;


    case 19: // set decay
      SID[(channel * 7) + 5] = (SID[(channel * 7) + 5] & B11110000) | (ccvalue >> 3);
      writeC64((channel * 7) + 5, SID[(channel * 7) + 5]);

      break;


    case 20: // set release
      SID[(channel * 7) + 6] = (SID[(channel * 7) + 6] & B11110000) | (ccvalue >> 3);
      writeC64((channel * 7) + 6, SID[(channel * 7) + 6]);

      break;


    case 21: // enable filter for a voice
      if(channel == 0) {
        SID[0x17] = (SID[0x17] & B11111110) | ((ccvalue >> 6) << 0);
        writeC64(0x17, SID[0x17]);
      }

      else if(channel == 1) {
        SID[0x17] = (SID[0x17] & B11111101) | ((ccvalue >> 6) << 1);
        writeC64(0x17, SID[0x17]);
      }

      else if(channel == 2) {
        SID[0x17] = (SID[0x17] & B11111011) | ((ccvalue >> 6) << 2);
        writeC64(0x17, SID[0x17]);
      }
      break;



    case 22: // set filter cutoff - coarse
      SID[0x16] = (SID[0x16] & B00000001) | ccvalue << 1; 
      writeC64(0x16, SID[0x16]); 
      break;


    case 23: // set filter cutoff - fine
      SID[0x16] = (SID[0x16] & B11111110) | ccvalue >> 6; 
      writeC64(0x15, (ccvalue >> 3 & B00000111)); 
      writeC64(0x16, SID[0x16]); 
      break;


    case 24: // set filter resonance
      SID[0x17] = (SID[0x17] & B00001111) | ((ccvalue >> 3) << 4);
      writeC64(0x17, SID[0x17]);

      break;

    case 25: // enbale LPF
      SID[0x18] = (SID[0x18] & B11101111) | ((ccvalue >> 6) << 4);
      writeC64(0x18, SID[0x18]);

      break;


    case 26: // enbale BPF
      SID[0x18] = (SID[0x18] & B11011111) | ((ccvalue >> 6) << 5);
      writeC64(0x18, SID[0x18]);

      break;


    case 27: // enbale HPF
      SID[0x18] = (SID[0x18] & B10111111) | ((ccvalue >> 6) << 6);
      writeC64(0x18, SID[0x18]);

      break;  


    case 28: // enbale / disable voice 3
      SID[0x18] = (SID[0x18] & B01111111) | ((ccvalue >> 6) << 7);
      writeC64(0x18, SID[0x18]);

      break;     


    case 29: // global volume
      SID[0x18] = (SID[0x18] & B11110000) | (ccvalue >> 3);
      writeC64(0x18, SID[0x18]);

      break;   
      
    case 30: // transposition
      oct_div = ccvalue + 1; 
      
      break; 
      
    case 31: // transposition
      transposition = ccvalue; 
      
      break; 
    }
  }

}


void doBend(byte channel, byte bendLSB, byte bendMSB) {
  if(channel >= 0 && channel <= 2) {
    bend[channel] = bendMSB / 5.29166666667;

    pitch = pitches[channel];

    frequency = (440 * pow(2, ((pitch - 69 + bend[channel]) / oct_div)) ) / (clock / 16777216); // calculate frequency

    SID[(channel * 7) + 0]  = int(frequency) % 256; // save low frequency
    SID[(channel * 7) + 1] = int(frequency) / 256; // save high frequency
    writeC64((channel * 7) + 0, SID[(channel * 7) + 0]); //  write frequency low
    writeC64((channel * 7) + 1, SID[(channel * 7) + 1]); // write frequency high    
  }
}


void writeC64(byte address, byte data) {

  PORTD = address; 
  PORTC = address; 
  digitalWrite(ad, HIGH);
  delayMicroseconds(20);
  digitalWrite(wr, HIGH);
  delayMicroseconds(delay_time);
  digitalWrite(wr, LOW);

  delayMicroseconds(delay_time);
  delayMicroseconds(delay_time);




  PORTD = data; 
  PORTC = data; 
  digitalWrite(ad, LOW);
  delayMicroseconds(20);
  digitalWrite(wr, HIGH);
  delayMicroseconds(delay_time);
  digitalWrite(wr, LOW);

  delayMicroseconds(delay_time);
  delayMicroseconds(delay_time);


}



void doMidiIn(byte data) {
  // running status set

  if((data >= 0x80) && (data < 0xf0) && (flag_previous == 0)) {
    rstat = data;
  }


  // deal with note on
  if((data >= 0x90) && (data < 0xa0) && (flag_previous == 0)) {
    channel = data & B00001111;
    flag_previous = 1;
  }
  else if((data < 0x80) && (flag_previous == 1)) {
    pitch = data;
    flag_previous = 2;
  }
  else if((data < 0x80) && (flag_previous == 2)) {
    velocity = data;
    doNote(channel, pitch, velocity);
    flag_previous = 0;
  }
  // done with note on

  // deal with note off (as discrete status byte)
  else if((data >= 0x80) && (data < 0x90) && (flag_previous == 0)) {
    channel = data & B00001111;
    flag_previous = -1;
  }
  else if((data < 0x80) && (flag_previous == -1)) {
    pitch = data;
    flag_previous = -2;
  }
  else if((data < 0x80) && (flag_previous == -2)) {
    velocity = data;
    doNoteOff(channel, pitch, velocity);
    flag_previous = 0;
  }
  // done with note off (as discrete status byte)

  // deal with cc data
  else if((data >= 0xb0) && (data < 0xc0) && (flag_previous == 0)) {
    channel = data & B00001111;
    flag_previous = 3;
  }
  else if((data < 0x80) && (flag_previous == 3)) {
    ccnumber = data;
    flag_previous = 4;
  }
  else if((data < 0x80) && (flag_previous == 4)) {
    ccvalue = data;
    doCC(channel, ccnumber, ccvalue);
    flag_previous = 0;
  }
  // done with cc data

  // deal with bend data
  else if((data >= 0xe0) && (data < 0xf0) && (flag_previous == 0)) {
    channel = data & B00001111;
    flag_previous = 5;
  }
  else if((data < 0x80) && (flag_previous == 5)) {
    bendLSB = data;
    flag_previous = 6;
  }
  else if((data < 0x80) && (flag_previous == 6)) {
    bendMSB = data;
    doBend(channel, bendLSB, bendMSB);
    flag_previous = 0;
  }
  // done with bend data

}




