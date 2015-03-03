

/*
 * @file learn_letter_mode.c
 *
 * @brief code for helper functions "learning letter modes" (MD 2,7,8,12)
 * @author: Edward Cai
 */ 

// Standard libraries
#include <stdbool.h>
#include <stdio.h>

// Custom libraries
#include "audio.h"
#include "io.h"
#include "common.h"
#include "debug.h"
#include "script_common.h"
#include "learn_letter_mode.h"

// State variables
static char next_state = STATE_NULL;
static char user_response = NO_DOTS;
static char submode = SUBMODE_NULL;
static int index = 0;
static glyph_t* curr_glyph = NULL;
static glyph_t* user_glyph = NULL;
static char cell = 0;
static char cell_pattern = 0;
static char cell_control = 0;
static int incorrect_tries = 0;
static bool scrolled = false;

void learn_letter_reset(script_t* new_script, char* new_lang_fileset, char* new_mode_fileset) {
	set_mode_globals(new_script, new_lang_fileset, new_mode_fileset);
	next_state = STATE_MENU;
	user_response = NO_DOTS;
	submode = SUBMODE_NULL;
	index = 0;
	curr_glyph = NULL;
	user_glyph = NULL;
	cell = 0;
	cell_pattern = 0;
	cell_control = 0;
	incorrect_tries = 0;
	scrolled = false;
	PRINTF("[MD2] Mode reset\n\r");
}

void learn_letter_main(script_t* SCRIPT_ADDRESS, char* LANG_FILESET, char* MODE_FILESET) {
	switch (next_state) {
		case STATE_MENU:
		switch(create_dialog(MP3_MENU, DOT_1 | DOT_2 | ENTER_CANCEL)) {
			
			case NO_DOTS:
			break;

			case '1':
				PRINTF("[MD2] Submode: Learn\n\r");
				play_mp3(MODE_FILESET, MP3_INSTRUCTIONS);
				submode = SUBMODE_LEARN;
				next_state = STATE_GENQUES;
				break;

			case '2':
				PRINTF("[MD2] Submode: Play\n\r");
				play_mp3(MODE_FILESET, MP3_INSTRUCTIONS);
				submode = SUBMODE_PLAY;
				next_state = STATE_GENQUES;
				break;

			case CANCEL:
				PRINTF("[MD2] Quitting to main menu\n\r");
				quit_mode();
				break;

			case ENTER:
				PRINTF("[MD2] Re-issuing main menu prompt\n\r");
				next_state = STATE_MENU;
				break;

			default:
				break;
		}
		break;

		case STATE_GENQUES:
		switch (submode) {

			case SUBMODE_LEARN:
				curr_glyph = get_next_glyph(SCRIPT_ADDRESS);
				if (curr_glyph == NULL) {
					reset_script_indices(SCRIPT_ADDRESS);
					next_state = STATE_GENQUES;
					curr_glyph = get_next_glyph(SCRIPT_ADDRESS);
					break;
				}
				break;

			case SUBMODE_PLAY:
				curr_glyph = get_random_glyph(SCRIPT_ADDRESS);
				break;

			default:
				break;

		}
		sprintf(dbgstr, "[MD2] Next glyph: %s\n\r", curr_glyph->sound);
		PRINTF(dbgstr);
		play_mp3(LANG_FILESET, MP3_NEXT_LETTER);
		next_state = STATE_PROMPT;
		break;

		case STATE_PROMPT:
		switch(submode) {

			case SUBMODE_LEARN:
			play_glyph(curr_glyph);
			play_mp3(MODE_FILESET, MP3_FOR_X_PRESS_DOTS);
			play_dot_sequence(curr_glyph);
			break;

			case SUBMODE_PLAY:
			play_silence(500);
			play_glyph(curr_glyph);
			break;

			default:
			break;
		}
		next_state = STATE_INPUT;
		break;

		case STATE_INPUT:
		if (io_user_abort == true) {
			PRINTF("[MD2] User aborted input\n\r");
			next_state = STATE_REPROMPT;
			io_init();
		}
		cell = get_cell();
		if (cell == NO_DOTS) {
			break;
		}
		cell_pattern = GET_CELL_PATTERN(cell);
		cell_control = GET_CELL_CONTROL(cell);
		switch (cell_control) {
			case 0b11:
			user_glyph = search_script(SCRIPT_ADDRESS, cell_pattern);
			next_state = STATE_CHECK;
			PRINTF("[MD2] Checking answer\n\r");
			break;
			case 0b10:
			next_state = STATE_PROMPT;
			break;
			case 0b01:
			next_state = STATE_REPROMPT;
			break;
			case 0b00:
			break;
		}
		break;

		case STATE_CHECK:
		if (glyph_equals(curr_glyph, user_glyph)) {
			if(curr_glyph -> next == NULL) {
				incorrect_tries = 0;
				PRINTF("[MD7] User answered correctly\n\r");
				play_mp3(LANG_FILESET, MP3_CORRECT);
				play_mp3(SYS_FILESET, MP3_TADA);
				next_state = STATE_GENQUES;
				} else {
				curr_glyph = get_next(SCRIPT_ADDRESS, curr_glyph);
				play_mp3(LANG_FILESET, MP3_NEXT_CELL);
				play_dot_sequence(curr_glyph);
				next_state = STATE_INPUT;
			}
			} else {
			incorrect_tries++;
			PRINTF("[MD7] User answered incorrectly\n\r");
			play_mp3(LANG_FILESET, MP3_INCORRECT);
			play_mp3(LANG_FILESET, MP3_TRY_AGAIN);
			curr_glyph = get_root(SCRIPT_ADDRESS, curr_glyph);
			next_state = STATE_PROMPT;
			if (incorrect_tries >= MAX_INCORRECT_TRIES) {
				play_glyph(curr_glyph);
				play_mp3(MODE_FILESET, MP3_FOR_X_PRESS_DOTS);
				play_dot_sequence(curr_glyph);
				next_state = STATE_INPUT;
			}
		}
		break;

		case STATE_REPROMPT:
		switch(create_dialog(MP3_REPROMPT,
		ENTER_CANCEL | LEFT_RIGHT)) {
			case NO_DOTS:
			break;

			case CANCEL:
			PRINTF("[MD2] Reissuing prompt\n\r");
			next_state = STATE_PROMPT;
			scrolled = false;
			break;

			case ENTER:
			PRINTF("[MD2] Skipping character\n\r");
			if (scrolled)
			next_state = STATE_PROMPT;
			else
			next_state = STATE_GENQUES;
			scrolled = false;
			break;

			case LEFT:
			PRINTF("[MD2] Previous letter\n\r");
			switch (submode) {
				case SUBMODE_LEARN:
				curr_glyph = get_prev_glyph(SCRIPT_ADDRESS);
				if (curr_glyph == NULL) {
					curr_glyph = get_next_glyph(SCRIPT_ADDRESS);
				}
				break;
				case SUBMODE_PLAY:
				curr_glyph = get_random_glyph(SCRIPT_ADDRESS);
				break;
				default:
				break;
			}
			play_glyph(curr_glyph);
			scrolled = true;
			break;

			case RIGHT:
			PRINTF("[MD2] Next letter\n\r");
			switch (submode) {
				case SUBMODE_LEARN:
				curr_glyph = get_next_glyph(SCRIPT_ADDRESS);
				if (curr_glyph == NULL) {
					curr_glyph = get_prev_glyph(SCRIPT_ADDRESS);
				}
				break;
				case SUBMODE_PLAY:
				curr_glyph = get_random_glyph(SCRIPT_ADDRESS);
				break;
				default:
				break;
			}
			play_glyph(curr_glyph);
			scrolled = true;
			break;

			default:
			break;
		}
		break;

		default:
		break;
	}
}
