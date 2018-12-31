// Defines constant parameters related to note regions

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


const byte mode_note_ranges[5][2] = {{0, 23}, {24, 47}, {48, 71}, {72, 95}, {96, 119}};
const int blue_start_color[3] = {0, 20, 229};
const int orange_end_color[3] = {0, 0, 0};
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

#define TOTAL_REGIONS sizeof(note_regions)/(2*sizeof(int(2)))
