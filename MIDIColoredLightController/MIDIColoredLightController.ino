/* Receive Incoming USB MIDI by reading data.  This approach
   gives you access to incoming MIDI message data, but requires
   more work to use that data.  For the simpler function-based
   approach, see InputFunctionsBasic and InputFunctionsComplete.

   Use the Arduino Serial Monitor to view the messages
   as Teensy receives them by USB MIDI

   You must select MIDI from the "Tools > USB Type" menu

   This example code is in the public domain.
*/
#define USE_OCTOWS2811
#include<OctoWS2811.h>
#include<FastLED.h>

#define NUM_LEDS_PER_STRIP 120
#define NUM_STRIPS 8
#define BRIGHTNESS 255

#define NUM_LEDS (NUM_LEDS_PER_STRIP * NUM_STRIPS)

#define NUM_NOTES 24
#define LOWEST_NOTE 12

#define MIN_STROBE_LENGTH 5
#define MAX_STROBE_LENGTH 800
#define MAX_PULSE_SCALE .6
#define MIN_PULSE_SCALE .0175
#define NO_STROBE_LENGTH 10
#define NOTE_RAINBOW_INC_MAX 35.0
#define CENTER_COUNTER_INC_MAX 6 // how fast the pulses move
#define PITCH_CENTER_VALUE 64
#define STANDING_WAVE_WIDTH 60 // led/s between 5 and 120

#define NUM_RAINBOW_MODES 10

#define ORANGE_WIDTH 10

boolean note_on_state[NUM_NOTES] = {0};
boolean white_note_on_state[NUM_NOTES] = {0};
boolean symmetric_mode_on = false;
enum manual_modes {
  NORMAL,
  NOTE_NEW_COLOR,
  NOTE_RAINBOW,
  CENTER_FILL,
  CENTER_FILL_NEW_COLOR
};

manual_modes curr_manual_mode = NORMAL;
boolean pulsing = false;
float pulsing_time_scale = 1;
float pulse_min_brightness = 0;
float pitch_counter = 0;
float last_pitch_command = 0;
byte last_velocity = 0;
const byte mode_note_ranges[5][2] = {{0, 23}, {24, 47}, {48, 71}, {72, 95}, {96, 119}};
byte curr_lowest_note = 0;
byte note_brightness[NUM_NOTES] = {0};
byte white_note_brightness[NUM_NOTES] = {0};
byte note_max_brightness[NUM_NOTES] = {0};
float note_hue[NUM_NOTES] = {0};
float note_center_counter[NUM_NOTES] = {0};
float center_counter_inc = 1;
const int blue_start_color[3] = {0, 20, 229};
//const int orange_end_color[3] = {248, 112, 18};
const int orange_end_color[3] = {0, 0, 0};
//const int orange_end_color[3] = {255, 255, 255};
// blue to black rain, purple to black rain, purple to blue rain, red to black rain, blue to red, blue to orange, blue to green
// First three are rain (discontinuous)
const int start_colors_hue[] = {169, 85, 0, 245, 128, 20, 245, 128, 20,0,245};  
const int end_colors_hue[] = {0, 0, 0, 169, 85, 128, 169, 85, 128,0,0};
const int mirror_note_num[] = {23, 13, 21, 15, 19, 17, 18, 16, 20, 14, 22, 12,11,1,9,3,7,5,6,4,8,2,10,0};
const int note_regions[][2] = {
  {0, 59}, // L1 idx 0

  {0, 59}, // idx 1 (diamond)
  {299, 359},
  {180, 239},
  {120, 178},
  
  {299, 359}, // L2 idx 5
  
  {240, 298}, // idx 6 (diamond)
  {180, 239},
  {418, 478},
  {360, 417},
  
  {240, 298}, // L3 idx 10
  
  {180, 239}, // L4 idx 11

  {0, 59}, // idx 12 (triangle)
  {299, 359},
  {240, 298},

  {120, 178}, // L5 idx 15

  {240, 298}, // idx 16 (triangle)
  {180, 239},
  {120, 178},
  
  {418, 478}, // L6 idx 19

  {120, 178}, // idx 20 (triangle)
  {418, 478},
  {360, 417},

  {360, 417}, // L7 idx 23

  {840, 898}, // R7 idx 24

  {480, 539}, // idx 25 (diamond)
  {779, 839},
  {659, 719},
  {599, 658},

  {899, 959}, // R6 idx 29

  {840, 898}, // idx 30 (diamond)
  {899, 959},
  {659, 719},
  {720, 778},

  {599, 658}, // R5 idx 34
  
  {659, 719}, // R4 idx 35

  {480, 539}, // idx 36 (triangle)
  {779, 839},
  {720, 778},

  {720, 778}, // R3 idx 39

  {720, 778}, // idx 40 (triangle)
  {659, 719},
  {599, 658},

  {779, 839}, // R2 idx 43

  {840, 898}, // idx 44 (triangle)
  {899, 959},
  {599, 658},

  {480, 539}, // R1 idx 47
  
  
  
};

const int note_num_to_region_idx[NUM_NOTES] = {0, 1, 5, 6, 10, 11, 12, 15, 16, 19, 20, 23, 24, 25, 29, 30, 34, 35, 36, 39, 40, 43, 44, 47};

byte Hue = 1;
byte Saturation = 1;

#define TOTAL_REGIONS sizeof(note_regions)/(2*sizeof(int(2)))

byte decayRate = 255;
float rainbow_hue_inc = 1;
int color_scheme_index = 0;
int rainbow_saturation = 255;
int activeRegions[][2] = {{0}};
unsigned long last_time = millis();
unsigned long last_turn_on_time = millis();
int strobe_length_ms = NO_STROBE_LENGTH;
void note_rainbow(int region_start, int region_end, int note_num);
void center_fill(int region_start, int region_end, int note_num);
void rainbow();
void rainbowWithGlitter();

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, rainbowWithGlitter};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gradient_counter = 0;

CRGB leds[NUM_STRIPS * NUM_LEDS_PER_STRIP];

// Pin layouts on the teensy 3:
// OctoWS2811: 2,14,7,8,6,20,21,5

void setup() {
  Serial.begin(115200);
  LEDS.addLeds<OCTOWS2811>(leds, NUM_LEDS_PER_STRIP);
  LEDS.setBrightness(BRIGHTNESS);
}

void loop() {
  // usbMIDI.read() needs to be called rapidly from loop().  When
  // each MIDI messages arrives, it return true.  The message must
  // be fully processed before usbMIDI.read() is called again.
  if (usbMIDI.read()) {
    processMIDI();
  }
  if (millis() > (last_time+25)){
    last_time = millis();
  

    if (millis() > (last_turn_on_time + strobe_length_ms)){
      last_turn_on_time = millis();
    }
  
    // Everything will constantly be faded out, then immediately
    // reset to its full brightness for all regions that are on
    //fadeToBlackBy(leds,NUM_LEDS_PER_STRIP*NUM_STRIPS,decayRate);
    fade_all();
    command_on_notes();
    command_white_on_notes();
    digital_fade_after();
    LEDS.show();
  }
  //LEDS.delay(20);
}

boolean digital_fade_on = true;
void fade_all(void){
  if (digital_fade_on){
    for(int i=0; i<decayRate; i++){
      leds[random16(NUM_LEDS_PER_STRIP*NUM_STRIPS)] = CHSV(0,0,0);
    }
  }
  else{
    fadeToBlackBy(leds,NUM_LEDS_PER_STRIP*NUM_STRIPS,decayRate);
  }
}

boolean digital_fade_after_on = false;
void digital_fade_after(void){
   if (digital_fade_after_on){
    for(int i=0; i<decayRate*2; i++){
      leds[random16(NUM_LEDS_PER_STRIP*NUM_STRIPS)] = CHSV(0,0,0);
    }
  }
}

void command_on_notes(void){
  /// Sends commands to all on notes
  pitch_counter += pulsing_time_scale;
  // Iterate through all notes
  gradient_counter += rainbow_hue_inc;
  
  for(int i=0; i< NUM_NOTES; i++){
    // We only care about notes that are currently on
    if (note_on_state[i]){ 
      // Get the starting/ending index for pulling region start/stop values
      // out of the note_regions array
      int region_start = note_num_to_region_idx[i];
      // Make sure we don't try to acess info from more notes than we have
      int region_end = note_num_to_region_idx[min(i+1,NUM_NOTES-1)];
      // If this is the last note, set region_end manually
      if (i == (NUM_NOTES-1)){
        region_end = TOTAL_REGIONS;
      }
      // Loop through all regions for that note and fill them
      
      for(int j = region_start; j<region_end;j++){
        // Sinusoidal pulsing
        if (pulsing){
          float sin_min = 255 * (1 - ((-last_pitch_command+63) / (PITCH_CENTER_VALUE - 1)));
          float sin_amp = (255 - sin_min) / 2;
          float sin_offset = (255 + sin_min) / 2;
          note_brightness[i] = sin_amp * sin(pitch_counter) + sin_offset;
        }
        // Only refill them if we're in the right block of time
        if (millis() < (last_turn_on_time + (float(strobe_length_ms) / 2))){
          if (curr_manual_mode == NOTE_RAINBOW){
            note_rainbow(note_regions[j][0], note_regions[j][1], i);
          }
          else if ((curr_manual_mode == CENTER_FILL) || (curr_manual_mode == CENTER_FILL_NEW_COLOR)){
            center_fill(note_regions[j][0], note_regions[j][1], i);
          }
          else{
            fill_gradient(leds, note_regions[j][0], CHSV(note_hue[i],Saturation,note_brightness[i]), note_regions[j][1], CHSV(note_hue[i],Saturation,note_brightness[i]), SHORTEST_HUES);
          }
        }
      }
    }
  }  
}

void command_white_on_notes(void){
  /// Sends commands to all on notes
  // Iterate through all notes
  for(int i=0; i< NUM_NOTES; i++){
    // We only care about notes that are currently on
    if (white_note_on_state[i]){ 
      // Get the starting/ending index for pulling region start/stop values
      // out of the note_regions array
      int region_start = note_num_to_region_idx[i];
      // Make sure we don't try to acess info from more notes than we have
      int region_end = note_num_to_region_idx[min(i+1,NUM_NOTES-1)];
      // If this is the last note, set region_end manually
      if (i == (NUM_NOTES-1)){
        region_end = TOTAL_REGIONS;
      }
      // Loop through all regions for that note and fill them
      for(int j = region_start; j<region_end;j++){
        // Only refill them if we're in the right block of time
        if (millis() < (last_turn_on_time + (float(strobe_length_ms) / 2))){ 
          fill_gradient(leds, note_regions[j][0], CHSV(0, 0, white_note_brightness[i]) , note_regions[j][1], CHSV(0, 0, white_note_brightness[i]), SHORTEST_HUES);
        
      }
    }
  }  
}
}

void update_modes(byte data1){
  // Updates mode based on note number/range
    if (data1 <= mode_note_ranges[0][1]){
      curr_manual_mode = NORMAL;
      curr_lowest_note = mode_note_ranges[0][0];
    }
    else if (data1 <= mode_note_ranges[1][1]){
      curr_manual_mode = NOTE_RAINBOW;
      curr_lowest_note = mode_note_ranges[1][0];
    }
    else if (data1 <= mode_note_ranges[2][1]){
      curr_manual_mode = NOTE_NEW_COLOR;
      curr_lowest_note = mode_note_ranges[2][0];
    }
    else if (data1 <= mode_note_ranges[3][1]){
      curr_manual_mode = CENTER_FILL;
      curr_lowest_note = mode_note_ranges[3][0];
    }
    else if (data1 <= mode_note_ranges[4][1]){
      curr_manual_mode = CENTER_FILL_NEW_COLOR;
      curr_lowest_note = mode_note_ranges[4][0];
    }
}

void processMIDI(void) {
  byte type, channel, data1, data2, cable;

  // fetch the MIDI message, defined by these 5 numbers (except SysEX)
  //
  type = usbMIDI.getType();       // which MIDI message, 128-255
  channel = usbMIDI.getChannel(); // which MIDI channel, 1-16
  data1 = usbMIDI.getData1();     // first data byte of message, 0-127
  data2 = usbMIDI.getData2();     // second data byte of message, 0-127
  cable = usbMIDI.getCable();     // which virtual cable with MIDIx8, 0-7
  int idx = 0;
  // uncomment if using multiple virtual cables
  ////Serial.print("cable ");
  ////Serial.print(cable, DEC);
  ////Serial.print(": ");

  // print info about the message
  //
  switch (type) {
    case usbMIDI.NoteOff: // 0x80
      //Serial.print("Note Off, ch=");
      //Serial.print(channel, DEC);
      //Serial.print(", note=");
      //Serial.print(data1, DEC);
      //Serial.print(", velocity=");
      //Serial.println(data2, DEC);
      update_modes(data1);
      // Toggle on state to off
      idx = min((data1-curr_lowest_note),NUM_NOTES-1),23;
      Serial.println(idx);
      if (channel != 1){
        note_on_state[idx] = 0;
        if (symmetric_mode_on){
          note_on_state[mirror_note_num[idx]] = 0;
          note_center_counter[mirror_note_num[idx]] = 0;
        }
        // Reset the note center counter to 0
        note_center_counter[idx] = 0;
      }
      else{
        white_note_on_state[idx] = 0;
      }
      break;

    case usbMIDI.NoteOn: // 0x90
      //Serial.print("Note On, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      //fill_gradient(leds, 10, CHSV(100,255,255), 60, CHSV(1,255,255), SHORTEST_HUES);
      update_modes(data1);
      idx = min((data1-curr_lowest_note),NUM_NOTES-1);
      Serial.println(idx);
      
      if (data1 == 120) {
        // Last note = 120, do rainbow toggle mode, swithc manual mode
        symmetric_mode_on = !symmetric_mode_on;
        //gCurrentPatternNumber = 0;
      }
      if (data1 < (curr_lowest_note + NUM_NOTES)){
        if (channel != 1){
          // Toggle on state to on
          note_on_state[idx] = 1;
          
          // Set brightness to scaled velocity value
          last_velocity = (map(data2,0,127,0,255));
          note_brightness[idx] = (map(data2,0,127,0,255));//data2;
          note_max_brightness[idx] = (map(data2,0,127,0,255));//data2;
          if (symmetric_mode_on){
            note_on_state[mirror_note_num[idx]] = 1;
            note_brightness[mirror_note_num[idx]] = (map(data2,0,127,0,255));//data2;
            note_max_brightness[mirror_note_num[idx]] = (map(data2,0,127,0,255));//data2;
          }
          
          // If we're in new color mode, increment the hue
          if ((curr_manual_mode == NOTE_NEW_COLOR) or (curr_manual_mode == CENTER_FILL_NEW_COLOR)){
            note_hue[idx] = Hue;
             
            if (symmetric_mode_on){
              note_hue[mirror_note_num[idx]] = Hue;
            }
            Hue = Hue + 30;   
          }    
        }
        else{
          white_note_on_state[min((data1-curr_lowest_note),NUM_NOTES-1)] = 1;
          white_note_brightness[min((data1-curr_lowest_note), NUM_NOTES-1)] = (map(data2,0,127,0,255));//data2;
        }
      }
      break;
      

    case usbMIDI.AfterTouchPoly: // 0xA0
      Serial.print("AfterTouch Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", note=");
      Serial.print(data1, DEC);
      Serial.print(", velocity=");
      Serial.println(data2, DEC);
      break;

    case usbMIDI.ControlChange: // 0xB0
      Serial.print("Control Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", control=");
      Serial.print(data1, DEC);
      Serial.print(", value=");
      Serial.println(data2, DEC);

      if (data1 == 10){ // data knob, control Hue
        Hue = map(data2,0,127,0,255);
        // Set color scheme based on knob value
        color_scheme_index = data2; //min(data2,10);
        for (int i = 0; i < NUM_NOTES; i++){
          note_hue[i] = Hue;
        }
      }
      else if (data1 == 7) { // volume slider 
        Saturation = map(data2,0,127,0,255);
        if (curr_manual_mode == NOTE_RAINBOW){
          rainbow_hue_inc = data2 * NOTE_RAINBOW_INC_MAX / 127.0;
        }
      }
      else if (data1 == 1){
        //decayRate = (255*127/126)/max(1,data2)-(255/126);
        decayRate = map(max(1,data2), 1,127, 255,2);
        pulsing_time_scale = float(max(0,data2) * (MAX_PULSE_SCALE-MIN_PULSE_SCALE) / 127.0 + MIN_PULSE_SCALE);
      }
      //decayRate = map(max(1,data2), 1,127, 255,2);
      //decayRate = map(127.0/max(1,data2), 127,1, 255,2); // should be between 255 (fast) and 1 (slow)
      break;

    case usbMIDI.ProgramChange: // 0xC0
      Serial.print("Program Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", program=");
      Serial.println(data1, DEC);
      break;

    case usbMIDI.AfterTouchChannel: // 0xD0
      Serial.print("After Touch, ch=");
      Serial.print(channel, DEC);
      Serial.print(", pressure=");
      Serial.println(data1, DEC);
      break;

    case usbMIDI.PitchBend: // 0xE0
      Serial.print("Pitch Change, ch=");
      Serial.print(channel, DEC);
      Serial.print(", pitch=");
      Serial.println(data1);
      Serial.println(data2); //+ data2 * 128, DEC);
      // Set strobe length to scaled pitch value
      strobe_length_ms = map(data2, 65, 128, MAX_STROBE_LENGTH, MIN_STROBE_LENGTH);
      // If the pitch wheel is below the default, no strobe
      if (data2 <= PITCH_CENTER_VALUE){
        strobe_length_ms = NO_STROBE_LENGTH;
        // Update timing for center fill
        center_counter_inc = data2 * CENTER_COUNTER_INC_MAX / 64.0;
      }
      if (data2 <= (PITCH_CENTER_VALUE - 1)){
        rainbow_saturation = int(map(data2,0,65,0,255)); // 
      }
        //pulsing = true;
        //last_pitch_command = data2;
        //LEDS.setBrightness(last_velocity);
//      }
//      else {
//        //pulsing = false;
//        //LEDS.setBrightness(BRIGHTNESS);
//      }
//      if (curr_manual_mode == NOTE_RAINBOW){
//        rainbow_saturation = int(map(data2,0,65,0,255)); // 
//      }

      break;

    case usbMIDI.SystemExclusive: // 0xF0
      // Messages larger than usbMIDI's internal buffer are truncated.
      // To receive large messages, you *must* use the 3-input function
      // handler.  See InputFunctionsComplete for details.
      Serial.print("SysEx Message: ");
      printBytes(usbMIDI.getSysExArray(), data1 + data2 * 256);
      Serial.println();
      break;

    case usbMIDI.TimeCodeQuarterFrame: // 0xF1
      Serial.print("TimeCode, index=");
      Serial.print(data1 >> 4, DEC);
      Serial.print(", digit=");
      Serial.println(data1 & 15, DEC);
      break;

    case usbMIDI.SongPosition: // 0xF2
      Serial.print("Song Position, beat=");
      Serial.println(data1 + data2 * 128);
      break;

    case usbMIDI.SongSelect: // 0xF3
      Serial.print("Sond Select, song=");
      Serial.println(data1, DEC);
      break;

    case usbMIDI.TuneRequest: // 0xF6
      Serial.println("Tune Request");
      break;

    case usbMIDI.Clock: // 0xF8
      Serial.println("Clock");
      break;

    case usbMIDI.Start: // 0xFA
      Serial.println("Start");
      break;

    case usbMIDI.Continue: // 0xFB
      Serial.println("Continue");
      break;

    case usbMIDI.Stop: // 0xFC
      Serial.println("Stop");
      break;

    case usbMIDI.ActiveSensing: // 0xFE
      Serial.println("Actvice Sensing");
      break;

    case usbMIDI.SystemReset: // 0xFF
      Serial.println("System Reset");
      break;

    default:
      Serial.println("Opps, an unknown MIDI message type!");
  }
}


void printBytes(const byte *data, unsigned int size) {
  while (size > 0) {
    byte b = *data++;
    if (b < 16) Serial.print('0');
    Serial.print(b, HEX);
    if (size > 1) Serial.print(' ');
    size = size - 1;
  }
}

void center_fill(int region_start, int region_end, int note_num){
  int region_center = int((region_end + region_start) / 2);
  int min_LED_turn_on = max((region_center - int(note_center_counter[note_num])), region_start);
  int max_LED_turn_on = min((region_center + int(note_center_counter[note_num])), (region_end - 1));
  // Does one iteration to fill in notes from the center
  for (int i=region_start; i < region_end; i++){
    if ((i >= min_LED_turn_on) && (i <= max_LED_turn_on)){
      leds[i] = CHSV(int(note_hue[note_num]), Saturation, note_brightness[note_num]);
    }
    else{
      leds[i] = CHSV(int(note_hue[note_num]), Saturation, 0);
    }
  }
  note_center_counter[note_num] += center_counter_inc;
}
void note_rainbow(int region_start, int region_end, int note_num){
  // Does one iteration of rainbow step for 
    if ((color_scheme_index >= 6) and (color_scheme_index<= 8)){ // standing wave functions to black
      //fill_standing_wave_to_black(i, color_scheme_index, region_start, region_end, note_num);
      fill_sine_wave_fucking_crazy(color_scheme_index, region_start, region_end, note_num);
    }
    else if ((color_scheme_index > 8) and (color_scheme_index < 10)){ // standing wave functions to second color 
      fill_standing_wave(color_scheme_index, region_start, region_end, note_num);
    }
    else if ((color_scheme_index >= 3) and (color_scheme_index <= 5)){
      fill_sine_wave(color_scheme_index, region_start, region_end, note_num);
    }
    else if(color_scheme_index < 3){
      fill_rain(color_scheme_index, region_start, region_end, note_num);
    }
    else if((color_scheme_index >=10) and (color_scheme_index <= 12)){
      fill_noise(color_scheme_index, region_start, region_end, note_num);
    }
    else{
      //fill_multicolor(region_start, region_end, note_num);
      fill_noise(color_scheme_index, region_start, region_end, note_num);
    }
  //gradient_counter += rainbow_hue_inc;
  note_hue[note_num] += rainbow_hue_inc;
}



void fill_noise(int color_scheme_index, int region_start, int region_end, int note_num){
    for (int i=0; i<(126-color_scheme_index); i++){
      leds[random16(region_start,region_end)] = CHSV(0,0,0);
    }
    for (int i=0; i<(color_scheme_index-10); i++){
      leds[random16(region_start,region_end)] = CHSV(0,0,255);
    }  
}

void fill_standing_wave(int color_scheme_index, int region_start, int region_end, int note_num){
  for (int i=region_start; i < region_end; i++){
    int r_comp, g_comp, b_comp;
    uint16_t standing_wave_val = triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start)))*triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start)));
    uint8_t brightness_multiplier = triwave8(uint8_t(gradient_counter));
    uint8_t how_bright = note_brightness[note_num]*standing_wave_val*brightness_multiplier/(255)/(255)/(255);
    CRGB start_rgb = CRGB(CHSV(start_colors_hue[color_scheme_index], rainbow_saturation, how_bright));
    
    //standing_wave_val = triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start+STANDING_WAVE_WIDTH/2)))*triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start+STANDING_WAVE_WIDTH/2)));
    standing_wave_val = triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start)))*triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start)));
    brightness_multiplier = triwave8(uint8_t(gradient_counter)+255/2);
    how_bright = note_brightness[note_num]*standing_wave_val*brightness_multiplier/(255)/(255)/(255);
    
    CRGB end_rgb = CRGB(CHSV(end_colors_hue[color_scheme_index], rainbow_saturation, how_bright));
    r_comp = (start_rgb.r+end_rgb.r)/2;
    g_comp = (start_rgb.g+end_rgb.g)/2;
    b_comp = (start_rgb.b+end_rgb.b)/2;
    leds[i] = CRGB(r_comp, g_comp, b_comp);   
  }
}

void fill_standing_wave_to_black(int color_scheme_index, int region_start, int region_end, int note_num){
  for (int i=region_start; i < region_end; i++){
    int r_comp, g_comp, b_comp;
    uint16_t standing_wave_val = triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start))); //*triwave8(uint8_t((255/STANDING_WAVE_WIDTH)*(i - region_start)));
    uint8_t brightness_multiplier = triwave8(uint8_t(gradient_counter));
    
    uint8_t how_bright = min(255,note_brightness[note_num]*standing_wave_val*brightness_multiplier/(255)/(255)+40);
    CRGB start_rgb = CRGB(CHSV(start_colors_hue[color_scheme_index], rainbow_saturation, how_bright));
    CRGB end_rgb = CRGB(0,0,0);
    r_comp = (start_rgb.r+end_rgb.r);
    g_comp = (start_rgb.g+end_rgb.g);
    b_comp = (start_rgb.b+end_rgb.b);
    leds[i] = CRGB(r_comp, g_comp, b_comp);   
  }
}


void fill_sine_wave(int color_scheme_index, int region_start, int region_end, int note_num){
  for (int i=region_start; i < region_end; i++){
    uint8_t current_gradient_location = uint8_t(gradient_counter + 6*(i - region_start));
    int r_comp, g_comp, b_comp;
    // Map from color scheme index, hue, rainbow saturation to RGB start and stop values
    // Map from hsv to rgb
    CRGB start_rgb = CRGB(CHSV(start_colors_hue[color_scheme_index], rainbow_saturation, note_brightness[note_num]));
    CRGB end_rgb = CRGB(CHSV(end_colors_hue[color_scheme_index], rainbow_saturation, note_brightness[note_num]));
    // Map from hsv to rgb
    if (uint8_t(current_gradient_location+ORANGE_WIDTH) < ORANGE_WIDTH*2){
      r_comp = start_rgb.r;
      g_comp = start_rgb.g;
      b_comp = start_rgb.b;
    }
    else if (current_gradient_location <= 126){
      r_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.r, end_rgb.r);
      g_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.g, end_rgb.g);
      b_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.b, end_rgb.b);      
    }
    else{ // if (current_gradient_location < 255-ORANGE_WIDTH) {
      r_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.r, start_rgb.r);
      g_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.g, start_rgb.g);
      b_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.b, start_rgb.b);     
    }
  //  else{ 
  //    r_comp = start_rgb.r;
  //    g_comp = start_rgb.g;
  //    b_comp = start_rgb.b;
  //  }
    leds[i] = CRGB(r_comp, g_comp, b_comp); 
  }
}

void fill_sine_wave_fucking_crazy(int color_scheme_index, int region_start, int region_end, int note_num){
  for (int i=region_start; i < region_end; i++){
    uint8_t current_gradient_location = uint8_t(gradient_counter + 6*(i - region_start));
    int r_comp, g_comp, b_comp;
    // Map from color scheme index, hue, rainbow saturation to RGB start and stop values
    // Map from hsv to rgb
    CRGB start_rgb = CRGB(CHSV(start_colors_hue[color_scheme_index], rainbow_saturation, note_brightness[note_num]));
    CRGB end_rgb = CRGB(CHSV(end_colors_hue[color_scheme_index], rainbow_saturation, note_brightness[note_num]));
    // Map from hsv to rgb
  //  if (current_gradient_location < ORANGE_WIDTH){
  //    r_comp = start_rgb.r;
  //    g_comp = start_rgb.g;
  //    b_comp = start_rgb.b;
  //  }
    if (current_gradient_location <= 126){
      r_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.r, end_rgb.r);
      g_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.g, end_rgb.g);
      b_comp = map(current_gradient_location, ORANGE_WIDTH, 126, start_rgb.b, end_rgb.b);      
    }
    else {
      r_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.r, start_rgb.r);
      g_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.g, start_rgb.g);
      b_comp = map(current_gradient_location, 127, 255-ORANGE_WIDTH, end_rgb.b, start_rgb.b);     
    }
  //  else{ (current_gradient_location < 255-ORANGE_WIDTH)
  //    r_comp = start_rgb.r;
  //    g_comp = start_rgb.g;
  //    b_comp = start_rgb.b;
  //  }
    leds[i] = CRGB(r_comp, g_comp, b_comp); 
  }
}
void fill_rain( int color_scheme_index, int region_start, int region_end, int note_num){
  for (int i=region_start; i < region_end; i++){
    //leds[i] = CHSV(int(note_hue[note_num])+i,192,note_brightness[note_num]);
    uint8_t current_gradient_location = uint8_t(gradient_counter + 6*(i - region_start));
    int r_comp, g_comp, b_comp;
    // Map from color scheme index, hue, rainbow saturation to RGB start and stop values
    // Map from hsv to rgb
    CRGB start_rgb = CRGB(CHSV(start_colors_hue[color_scheme_index], rainbow_saturation, note_brightness[note_num]));
    // Get black for rain, since black is not a hue
    CRGB end_rgb = CRGB(0, 0, 0);
    r_comp = uint8_t(max(0, current_gradient_location * (end_rgb.r - start_rgb.r) / 245 + start_rgb.r));
    g_comp = uint8_t(max(0, current_gradient_location * (end_rgb.g - start_rgb.g) / 245 + start_rgb.g));
    b_comp = uint8_t(max(0, current_gradient_location * (end_rgb.b - start_rgb.b) / 245 + start_rgb.b)); 
    
    r_comp = r_comp*r_comp/255;
    g_comp = g_comp*g_comp/255;
    b_comp = b_comp*b_comp/255;
    leds[i] = CRGB(r_comp, g_comp, b_comp);
  }
}

void fill_multicolor(int region_start, int region_end, int note_num){
  // Does one iteration of rainbow step for 
  for (int i=region_start; i < region_end; i++){
    leds[i] = CHSV(int(note_hue[note_num])+i,192,note_brightness[note_num]);
  }
  //note_hue[note_num] += rainbow_hue_inc;
}
  
void rainbow() 
{
  static uint8_t hue = 0;
  for(int i = 0; i < NUM_STRIPS; i++) {
    for(int j = 0; j < NUM_LEDS_PER_STRIP; j++) {
      leds[(i*NUM_LEDS_PER_STRIP) + j] = CHSV((32*i) + hue+j,192,255);
    }
  }
  hue = hue + 1;
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(95);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}
