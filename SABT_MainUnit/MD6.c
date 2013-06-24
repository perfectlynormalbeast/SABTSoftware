/**
 * @file MD6.c
 * @brief  Mode 6 code - Free Play
 * @author Vivek Nair (viveknair@cmu.edu)
 */

#include "Globals.h"
#include "audio.h"
#include "common.h"

/* Change this script header for new script */
#include "script_english.h"

// State definitions
#define STATE_INITIAL   0b00000000  // Initial state
#define STATE_INPUT     0b00000001  // Waits for dot input from user
#define STATE_CHECK     0b00000010  // Verifies input, gives feedback,
                                    // returns to input wait loop

#define MAX_INCORRECT_TRIES 3

// Change these script pointers for new script
static script_t* this_script = &script_english;
static char* lang_fileset = script_english.fileset;
static char mode_fileset[5] = "MD6_";

// State variables
static char next_state = STATE_INITIAL;
static char button_bits = 0b00000000;
static char last_dot = 0;
static char last_cell = 0;
static alphabet_t *this_alpha = NULL;
static char incorrect_tries = 0;

void md6_reset_vars() {
  button_bits = 0x00;
  last_dot = 0;
  last_cell = 0;
  this_alpha = NULL;
  incorrect_tries = 0;
  PRINTF("State reset\n\r");
}

void md6_main(void) {
  switch (next_state) {
    
    // Initialises mode
    case STATE_INITIAL:
      PRINTF("*** MD6 Free Play ***\n\r");
      md6_reset_vars();
      play_mp3(mode_fileset, "INT");
      next_state = STATE_INPUT;
      break;

    // If last button pressed is ENTER, check the dots input so far
    // otherwise continue to accept more dots
    case STATE_INPUT:
      if (last_dot != 0) { 
        switch (last_dot) {
          case ENTER:
            next_state = STATE_CHECK;
            break;

          case CANCEL:
            play_mp3(lang_fileset, "CANC");
            button_bits = 0x00;
            break;

          case '1': case '2': case '3': case '4': case '5': case '6':
            button_bits = add_dot(button_bits, last_dot);
            play_dot(lang_fileset, last_dot);
            break;

          default:
            incorrect_tries++;
            play_mp3(lang_fileset, "INVP");
            if (incorrect_tries >= MAX_INCORRECT_TRIES) {
              incorrect_tries = 0;
              next_state = STATE_INITIAL;
            }
        }
        last_dot = 0;
      }
      break;

    // If user presses ENTER, then check dot sequence for valid letter
    // and provide feedback
    case STATE_CHECK:
      this_alpha = get_alphabet_by_bits(button_bits, this_script);
      play_alphabet(lang_fileset, this_alpha);
      next_state = STATE_INPUT;
      button_bits = 0x00;
      break;
  }
}

void md6_reset(void) {
  next_state = STATE_INITIAL;
}

void md6_call_mode_yes_answer(void) {
  last_dot = ENTER;
}

void md6_call_mode_no_answer(void) {
  last_dot = CANCEL;
}

void md6_input_dot(char this_dot) {
  last_dot = this_dot;
}

void md6_input_cell(char this_cell) {
  if (last_dot != 0) {
    last_cell = this_cell;
  }
}

void md6_call_mode_left(void) {
  last_dot = LEFT;
}

void md6_call_mode_right(void) {
  last_dot = RIGHT;
}