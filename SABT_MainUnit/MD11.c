/**
 * @file md11.c
 * @brief Mode 11 code - Everyday Noises Game
 * @author Poornima Kaniarasu (pkaniara)
 */

#include "Globals.h"
#include "Modes.h"
#include "audio.h"
#include "common.h"
#include "letter_globals.h"
#include "script_common.h"
#include "script_english.h"

int md11_current_state, md11_prev_state = 0;
static int game_mode = 0;
char md11_last_dot, last_cell, expected_dot;
static int mistakes = 0;


char *noise_list[11] = {"aeroplane", "rain", "bell", "doorbell", "horn", "auto",
    "truck", "train", "siren", "phone", "clock"};

int noise_used_list[11] = {0,0,0,0,0,0,0,0,0,0,0};


/**
 * @brief Based off of the internal timer (TCNT1) - we generate
 *        a psuedo-random number. Turn that into a number from 1 - 11
 *        which corresponds to 1 of 11 different noises. Check which
 *        noises have been played already to be sure to play all of
 *        the different noises before repeating the list.
 * @return int - number between 1 - 11 corresponding to the noise file to play
 */
int choose_noise()
{
  int num = TCNT1;
  int i;

  num *= PRIME;
  num = (abs(num) % 11);

  char buf[11];
  sprintf(buf, "num=%i\r\n", num);
  PRINTF(buf);

  while(noises_used_list[num])
  {
    num = TCNT1;
    num *= PRIME;
    num = (abs(num) % 11);
  }

  noises_used_list[num] = 1;

  for(i = 0; i < 11; i ++)
  {
    sprintf(buf, "arr=%i, ",noises_used_list[i] );
    PRINTF(buf);
  }

  TX_NEWLINE_PC;
  sprintf(buf, "cnt=%i", noises_used+1);
  PRINTF(buf);
  TX_NEWLINE_PC;

  // increment noises_used until we've used all 11 noises then reset everything
  noises_used++;
  if (noises_used == 11)
  {
    noises_used = 0;
    for (i = 0; i < 11; i++)
      noises_used_list[i] = 0;
  }

  return num;
}

void md11_reset(void)
{
  set_mode_globals(&script_english, "ENG_", "MD11");
  md11_current_state = 0;
  md11_last_dot = 0;
  mistakes = 0;
}


/**
 * @brief  Step through the main stages in the code.
 *         Note : We need 2 STATE_REQUEST_INPUT states because request_to_play_mp3_file
 *         cannot handle 2 calls in quick succession.
 * @return Void
 * @TODO record md11INT.MP3
 */
void md11_main(void)
{
  char noise_file[16];
  char noise_sound[16];
  char spell_letter[8];
  switch(md11_current_state)
  {
    case STATE_INITIAL:
      play_mp3("MD11","INT"); // Welcomes and asks to choose a mode A or B
	  game_mode = 0;
      md11_current_state = STATE_SELECT_MODE; //STATE_REQUEST_INPUT1;
      noises_used = 0;
      got_input = false;
      break;

    case STATE_REQUEST_INPUT1:
	  if (game_mode == 1) play_mp3("MD11","PLSA");
	  else if (game_mode == 2) play_mp3("MD11","PLSB");
      length_entered_word = 0;
      current_word_index = 0;
      noise = noise_list[choose_noise()];
      md11_current_state = STATE_REQUEST_INPUT2;
      break;

    case STATE_REQUEST_INPUT2:

      if (game_mode == 1) play_mp3(NULL,noise);
      else if (game_mode == 2) { sprintf(noise_file, "N%s", noise);
	  play_mp3(NULL,noise_file);}

      md11_current_state = STATE_WAIT_INPUT;
      break;

    case STATE_WAIT_INPUT:
	 
      if(got_input)
      {
        got_input = false;		
        md11_current_state = STATE_PROC_INPUT;
      }
      break;

    case STATE_PROC_INPUT:
      // set entered_letter in valid_letter(), but return 1 or 0
	  
      if (last_cell == 0)
      {
        md11_current_state = STATE_READ_ENTERED_LETTERS;
      } else if (valid_letter(last_cell))
      {
        char buf[16];
        sprintf(buf, "ENG_%c", entered_letter);
        md11_current_state = STATE_CHECK_IF_CORRECT;
		if (!game_mode)
	    {
		if (entered_letter == 'a') game_mode = 1;
	    else if (entered_letter == 'b') game_mode = 2;
		else {
		    md11_current_state = STATE_WAIT_INPUT;
			break;
		}
		md11_current_state = STATE_REQUEST_INPUT1;
	    }
		play_mp3(NULL,buf);
      } else 
      {
	    mistakes = mistakes+1;
		PRINTF("mistake_inv");
		 play_mp3("ENG_","INVP");
		if (mistakes == 3){
			md11_current_state = STATE_WORD_HINT;
		}
		else if (mistakes >= 6){
			md11_current_state = STATE_LETTER_HINT;
		}
        else md11_current_state = STATE_READ_ENTERED_LETTERS;
      }
      break;

    case STATE_READ_ENTERED_LETTERS:
      if(length_entered_word > 0)
      {
        char buf[16];
        sprintf(buf, "ENG_%c", noise[current_word_index]);
        play_mp3(NULL,buf);
        current_word_index++;
      }

      if (current_word_index == length_entered_word)
      {
        md11_current_state = STATE_WAIT_INPUT;
        current_word_index = 0; 
      }
      break;

    case STATE_CHECK_IF_CORRECT:
      if (noise[length_entered_word] == entered_letter)
      {
        length_entered_word++;

        if (length_entered_word != strlen(noise))
          md11_current_state = STATE_CORRECT_INPUT;
        else
          md11_current_state = STATE_DONE_WITH_CURRENT_NOISE;
      } else 
      {
        md11_current_state = STATE_WRONG_INPUT;
      }
      break;

    case STATE_WRONG_INPUT:
	  
      play_mp3("ENG_","NO");
	  mistakes = mistakes + 1;
	  PRINTF("mistakes");

	  if (mistakes == 3){
			md11_current_state = STATE_WORD_HINT;
	  }
	  else if (mistakes >= 6){
			md11_current_state = STATE_LETTER_HINT;
	  }
      else md11_current_state = STATE_READ_ENTERED_LETTERS;
      break;

    case STATE_CORRECT_INPUT:
	  mistakes = 3;
      play_mp3("ENG_","GOOD");	  
      md11_current_state = STATE_WAIT_INPUT;
      break;

    case STATE_DONE_WITH_CURRENT_NOISE:
	  mistakes = 0;
	  play_mp3("ENG_","GOOD");
	  play_mp3("ENG_","NCWK");
	  if (game_mode == 1) {
      	for (int count = 0; count < strlen(noise); count++) {
			sprintf(spell_letter,"ENG_%c",noise[count]);
			play_mp3(NULL,spell_letter);
		}  	  	
	  }
	  md11_current_state = STATE_PLAY_SOUND; 
      break;

    case STATE_SELECT_MODE:
	  play_mp3("MD11","MSEL");
	  md11_current_state = STATE_WAIT_INPUT;
	  break;
 
    case STATE_PLAY_SOUND:
	  sprintf(noise_sound, "N%s", noise);
	  play_mp3(NULL,noise_sound);
	  if (game_mode == 2){
		  play_mp3("MD11","LIKE");
		  play_mp3(NULL,noise);
		  }
	  md11_current_state = STATE_REQUEST_INPUT1;
	  break;
	case STATE_PROMPT:
	  break;

    case STATE_WORD_HINT:
	  play_mp3("MD11","PLWR");
	  sprintf(noise_sound, "N%s", noise);
	  play_mp3(NULL,noise_sound);
	  for (int count = 0; count < strlen(noise); count++) {
			sprintf(spell_letter,"ENG_%c",noise[count]);
			play_mp3(NULL,spell_letter);
	  }
	  md11_current_state = STATE_WAIT_INPUT;  	  
	  break;

	case STATE_LETTER_HINT:
	  play_mp3("MD11","PLWR");
	  char let[8];
      sprintf(let, "ENG_%c", noise[length_entered_word]);
	  PRINTF(let);
      play_mp3(NULL,let);
	  play_mp3("MD11","PRSS");	  
	  md11_current_state = STATE_BUTTON_HINT;
	  break;

	case STATE_BUTTON_HINT:
      play_pattern(get_bits_from_letter(noise[length_entered_word]));
      md11_current_state = STATE_WAIT_INPUT;
      break;
      break;
  }
}
/**
 * @brief Handle left scroll button presses in mode 3
 * @return Void
 */

void md11_call_mode_left(void)
{
 	//replays the noise name or sound in accordance with submode
	md11_current_state = STATE_REQUEST_INPUT2;
}
/**
 * @brief Handle right scroll button presses in mode 3
 * @return Void
 */

void md11_call_mode_right(void)
{
// skips the noise 
 if (md11_current_state != STATE_PROMPT) md11_prev_state =  md11_current_state;
 play_mp3("MD11","SKIP");
 md11_current_state = STATE_PROMPT;
}
/**
 * @brief Handle enter button presses in mode 3
 * @return Void
 */
void md11_call_mode_yes_answer(void)
{
  if (md11_current_state == STATE_PROMPT) md11_current_state = STATE_REQUEST_INPUT1;	
}

/**
 * @brief Handle exit buton pressed in this mode
 * @param Void 
 * @return Void
 */
void md11_call_mode_no_answer(void)
{  
   if (md11_current_state == STATE_PROMPT)
   {
	  md11_current_state = md11_prev_state;    
   }  
   else 
   {  
      play_mp3("MD11","INT"); // Welcomes and asks to choose a mode A or B
	  game_mode = 0;
      md11_current_state = STATE_SELECT_MODE; 
      noises_used = 0;
	  mistakes = 0;
      got_input = false;
   }
}

/**
 * @brief Set the dot the from input
 * @param this_dot the entered dot
 * @return Void
 */
void md11_input_dot(char this_dot)
{
  md11_last_dot = this_dot;
  play_dot(md11_last_dot);
}

/**
 * @brief handle cell input
 * @param this_cell the entered cell
 * @return Void
 */
void md11_input_cell(char this_cell)
{
  if(md11_last_dot != 0)
  {
    last_cell = this_cell;
    got_input = true;
  }
}
