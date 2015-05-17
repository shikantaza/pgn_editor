/**
  Copyright 2015 Rajesh Jayaprakash <rajesh.jayaprakash@gmail.com>

  This file is part of PGN Editor.

  PGN Editor is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  PGN Editor is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PGN Editor.  If not, see <http://www.gnu.org/licenses/>.
**/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include "pgn_editor.h"

//global variables
int selected_r, selected_c;

char ***new_pgn_fens;
int nof_plys;
int current_ply;

move_t *new_pgn_moves;

enum board_states state;

enum bool white_can_castle_o_o, black_can_castle_o_o;
enum bool white_can_castle_o_o_o, black_can_castle_o_o_o;

enum bool prev_white_can_castle_o_o, prev_black_can_castle_o_o;
enum bool prev_white_can_castle_o_o_o, prev_black_can_castle_o_o_o;
//end of global variables

//external variables
extern GtkWidget *window;
extern GtkWidget *grid;
extern GtkWidget *moves_text_view;
extern GtkWidget *annotation_text_view;
extern GtkWidget *annotation_window;
extern GtkWidget *comment_text_view;

extern GtkWidget *prom_win_white;
extern GtkWidget *prom_win_black;

extern char *convert_fen_to_algebraic_coords(int, int);

extern char event[100];
extern char site[100];
extern char date[100];
extern char round1[100]; //so named to avoid conflict with built in round() function
extern char white_player[100];
extern char black_player[100];
extern char result[100];

extern enum side promotion_side;
extern int promotion_index;
extern int promotion_d;
extern char *promotion_move_text;
extern char **promotion_new_fen;
extern char promotion_choice;

extern populate_moves_text_view();
extern void display_message(char *, GtkWidget *);

extern enum mode current_mode;
extern char **initial_pos_fen;
extern char pos_setup_selected_piece;

extern enum bool pos_setup_white_to_move;
extern enum bool pos_setup_white_o_o;
extern enum bool pos_setup_white_o_o_o;
extern enum bool pos_setup_black_o_o;
extern enum bool pos_setup_black_o_o_o;

extern enum bool position_setup_completed;
//end of external variables;

//forward declarations
enum bool does_piece_attack_square(int, int, 
				   int, int, 
				   char **);
enum bool is_valid_move(int, int,
			int, int,
			char **);
enum bool is_square_under_attack(enum side, int, int, char **);
enum bool is_white_piece(char);
enum bool is_black_piece(char);
char *convert_to_algebraic_notation(int, int, int, int, char **);
void append_ply_to_moves_list();
enum bool is_king_under_check(enum side, char **);
void free_new_pgn_data_structures();
void highlight_ply_new_pgn(enum bool, int);
void unhighlight_all_moves_new_pgn();
//end of forward declarations

void initialize_non_move_data()
{
  memset(event, '\0', 100);
  strcpy(event, "?");

  memset(site, '\0', 100);
  strcpy(site, "?");

  memset(date, '\0', 100);
  strcpy(date, "?");

  memset(round1, '\0', 100);
  strcpy(round1, "?");

  memset(white_player, '\0', 100);
  strcpy(white_player, "?");

  memset(black_player, '\0', 100);
  strcpy(black_player, "?");

  memset(result, '\0', 100);
  strcpy(result, "*");
}

char **create_new_fen()
{
  char **fen_array = (char **)malloc(8 * sizeof(char *));

  int i,j;
    
  for(i=0;i<8;i++)
    fen_array[i] = (char *)malloc(8 * sizeof(char));

  for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      fen_array[i][j] = 0;

  return fen_array;
}

void new_pgn()
{
  free_new_pgn_data_structures();

  current_mode = pgn_from_scratch;

  new_pgn_fens = (char ***)malloc(sizeof(char **));

  char **fen_array = create_new_fen();

  convert_fen_string_to_fen_array("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR ", fen_array);

  new_pgn_fens[0] = fen_array;

  state = white_to_move;

  nof_plys = 0;
  current_ply = 0;

  fill_grid(grid, fen_array);

  initialize_non_move_data();

  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)comment_text_view), "", -1);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)moves_text_view), "", -1);
}

void new_pgn_file(GtkWidget *widget, gpointer data)
{
  new_pgn();
}

void board_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  int d = (int)data;

  if(current_mode == pgn_from_position && position_setup_completed == false)
  {
    //if a piece is already present in the
    //square, clear the square
    if(initial_pos_fen[d/8][d%8] != 0)
      initial_pos_fen[d/8][d%8] = 0;
    else if(pos_setup_selected_piece == 'P' && d/8 == 7 ||
	    pos_setup_selected_piece == 'p' && d/8 == 0)
    {
      display_message("Pawns cannot be placed in the first/eighth ranks!", window);
      return;
    }
    else if((pos_setup_selected_piece == 'K' || pos_setup_selected_piece == 'k'))
    {

      //if board already contains a king, remove that king
      int i, j;

      for(i=0;i<8;i++) {
	for(j=0;j<8;j++) {
	  if(initial_pos_fen[i][j] == pos_setup_selected_piece)
	  {
	    initial_pos_fen[i][j] = 0;
	    //need to put double break here, but no harm if no break
	  }
	}
      }

      initial_pos_fen[d/8][d%8] = pos_setup_selected_piece;
    }
    else
      initial_pos_fen[d/8][d%8] = pos_setup_selected_piece;

    fill_grid(grid, initial_pos_fen);
    return;
  }

  //don't process mouse clicks on the
  //board unless we're at the most
  //recent move
  if(current_ply != nof_plys)
    return;

  int index = nof_plys + 1;

  if(state == white_to_move)
  {
    char p = new_pgn_fens[index-1][d/8][d%8]; 
    if(p<65 || p>90)
    {
      display_message("Please select a white piece to move!", window);
      return;
    }

    selected_r = d/8;
    selected_c = d%8;
    state = piece_to_move_selected;
  }
  else if(state == black_to_move)
  {
    char p = new_pgn_fens[index-1][d/8][d%8]; 
    if(p<97 || p>122)
    {
      display_message("Please select a black piece to move!", window);
      return;
    }

    selected_r = d/8;
    selected_c = d%8;
    state = piece_to_move_selected;
  }
  else if(state == piece_to_move_selected)
  {
    //selecting another piece to move
    if((is_white_piece(new_pgn_fens[index-1][selected_r][selected_c]) == true &&
	is_white_piece(new_pgn_fens[index-1][d/8][d%8])               == true) ||
       (is_black_piece(new_pgn_fens[index-1][selected_r][selected_c]) == true &&
	is_black_piece(new_pgn_fens[index-1][d/8][d%8])               == true))
    {
      selected_r = d/8;
      selected_c = d%8;
      return;
    }

    if(is_valid_move(selected_r, selected_c, d/8, d%8, (char **)new_pgn_fens[index-1]) == false)
    {
      display_message("Please select a valid move!", window);
      return;
    }

    /* if(new_pgn_fens[index-1][selected_r][selected_c] == 'K') */
    /* { */
    /*   prev_white_can_castle_o_o = white_can_castle_o_o; */
    /*   prev_white_can_castle_o_o_o = white_can_castle_o_o_o; */

    /*   white_can_castle_o_o = false; */
    /*   white_can_castle_o_o_o = false; */
    /* } */
    /* else if(new_pgn_fens[index-1][selected_r][selected_c] == 'k') */
    /* { */
    /*   prev_black_can_castle_o_o = black_can_castle_o_o; */
    /*   prev_black_can_castle_o_o_o = black_can_castle_o_o_o; */

    /*   black_can_castle_o_o = false; */
    /*   black_can_castle_o_o_o = false; */
    /* } */

    if(new_pgn_fens[index-1][selected_r][selected_c] == 'R' && d == 63)
    {
      prev_white_can_castle_o_o = white_can_castle_o_o;
      white_can_castle_o_o = false;
    }
    else if(new_pgn_fens[index-1][selected_r][selected_c] == 'R' && d == 56)
    {
      prev_white_can_castle_o_o_o = white_can_castle_o_o_o;
      white_can_castle_o_o_o = false;
    }
    else if(new_pgn_fens[index-1][selected_r][selected_c] == 'r' && d == 0)
    {
      prev_black_can_castle_o_o = black_can_castle_o_o;
      black_can_castle_o_o = false;
    }
    else if(new_pgn_fens[index-1][selected_r][selected_c] == 'r' && d == 7)
    {
      prev_black_can_castle_o_o_o = black_can_castle_o_o_o;
      black_can_castle_o_o_o = false;
    }

    char **new_fen = create_new_fen();
    copy_fen(new_pgn_fens[index-1], new_fen);

    char piece = new_pgn_fens[index-1][selected_r][selected_c];

    char *move_text = (char *)malloc(10 * sizeof(char));
    memset(move_text, '\0', 10);

    //castling
    if(piece == 'K' && 
       selected_r == 7 &&
       selected_c == 4 &&
       (d/8) == 7 &&
       (d%8) == 6)
    {
      new_fen[7][7] = 0;
      new_fen[7][5] = 'R';
      prev_white_can_castle_o_o = white_can_castle_o_o;
      prev_white_can_castle_o_o_o = white_can_castle_o_o_o;
      white_can_castle_o_o = false;
      white_can_castle_o_o_o = false;
      strcpy(move_text, "O-O");
    }
    else if(piece == 'K' && 
	    selected_r == 7 &&
	    selected_c == 4 &&
	    (d/8) == 7 &&
	    (d%8) == 2)
    {
      new_fen[7][0] = 0;
      new_fen[7][3] = 'R';
      prev_white_can_castle_o_o = white_can_castle_o_o;
      prev_white_can_castle_o_o_o = white_can_castle_o_o_o;
      white_can_castle_o_o = false;
      white_can_castle_o_o_o = false;
      strcpy(move_text, "O-O-O");
    }
    else if(piece == 'k' && 
	    selected_r == 0 &&
	    selected_c == 4 &&
	    (d/8) == 0 &&
	    (d%8) == 6)
    {
      new_fen[0][7] = 0;
      new_fen[0][5] = 'r';
      prev_black_can_castle_o_o = black_can_castle_o_o;
      prev_black_can_castle_o_o_o = black_can_castle_o_o_o;
      black_can_castle_o_o = false;
      black_can_castle_o_o_o = false;
      strcpy(move_text, "O-O");
    }
    else if(piece == 'k' && 
	    selected_r == 0 &&
	    selected_c == 4 &&
	    (d/8) == 0 &&
	    (d%8) == 2)
    {
      new_fen[0][0] = 0;
      new_fen[0][3] = 'r';
      prev_black_can_castle_o_o = black_can_castle_o_o;
      prev_black_can_castle_o_o_o = black_can_castle_o_o_o;
      black_can_castle_o_o = false;
      black_can_castle_o_o_o = false;
      strcpy(move_text, "O-O-O");
    }

    enum bool promotion = false;
    
    if(new_pgn_fens[index-1][selected_r][selected_c] == 'P' &&
       d/8 == 0)
    {
      //new_fen[selected_r][selected_c] = 0;
      //new_fen[d/8][d%8] = 'Q'; //TODO: Ask user for choice of promotion piece
      //promotion = true;

      promotion_side       = white;
      promotion_index      = index;;
      promotion_d          = d;
      promotion_move_text  = move_text;
      promotion_new_fen    = new_fen;

      gtk_widget_show_all((GtkWidget *)prom_win_white);

      return;

    }
    else if(new_pgn_fens[index-1][selected_r][selected_c] == 'p' &&
       d/8 == 7)
    {
      //new_fen[selected_r][selected_c] = 0;
      //new_fen[d/8][d%8] = 'q'; //TODO: Ask user for choice of promotion piece
      //promotion = true;

      promotion_side       = black;
      promotion_index      = index;;
      promotion_d          = d;
      promotion_move_text  = move_text;
      promotion_new_fen    = new_fen;

      gtk_widget_show_all((GtkWidget *)prom_win_black);

      return;
    }
    else
    {
      new_fen[selected_r][selected_c] = 0;
      new_fen[d/8][d%8] = new_pgn_fens[index-1][selected_r][selected_c];
    }

    if(strlen(move_text) == 0)
    {
      char *temp = convert_to_algebraic_notation(selected_r, selected_c, d/8, d%8, new_pgn_fens[index-1]);
      strcpy(move_text, temp);
      free(temp);
    }
    
    if(promotion == true)
      strcat(move_text, "=Q");

    if(is_white_piece(new_fen[d/8][d%8]) == true && is_king_under_check(black, new_fen) == true ||
       is_black_piece(new_fen[d/8][d%8]) == true && is_king_under_check(white, new_fen) == true)
      strcat(move_text, "+");

    if(nof_plys % 2 == 0)
    {
      state = black_to_move;

      if(new_pgn_moves == NULL)
	new_pgn_moves = (move_t *)malloc(sizeof(move_t));
      else
	new_pgn_moves = (move_t *)realloc(new_pgn_moves, (nof_plys/2 + 1) * sizeof(move_t));

      memset(new_pgn_moves[nof_plys/2].white_move, '\0', 10);
      strcpy(new_pgn_moves[nof_plys/2].white_move, move_text);

      memset(new_pgn_moves[nof_plys/2].white_comment, '\0', 200);
      memset(new_pgn_moves[nof_plys/2].black_comment, '\0', 200);
    }
    else
    {
      state = white_to_move;

      memset(new_pgn_moves[nof_plys/2].black_move, '\0', 10);

      strcpy(new_pgn_moves[nof_plys/2].black_move, move_text);
    }

    free(move_text);

    append_ply_to_moves_list();

    nof_plys++;
    current_ply++;

    unhighlight_all_moves_new_pgn();
    highlight_ply_new_pgn(true, nof_plys);

    int new_size = nof_plys + 1;

    new_pgn_fens = (char ***)realloc(new_pgn_fens, new_size * sizeof(char **));

    new_pgn_fens[new_size - 1] = new_fen;

    fill_grid(grid, new_fen);
  }
  else
    assert(false);
}

enum bool is_king_under_check(enum side s, char **fen_array)
{
  //1. find the king's square
  //2. loop through the array, for
  //   each of the opposite colour's pieces
  //   (except for the king), check if the
  //   piece attacks the king's square

  int i, j;

  int r, c;

  char piece = (s == white) ? 'K' : 'k';

  enum bool king_square_found = false;

  for(i=0; i<8; i++)
  {
    if(king_square_found == true)
      break;

    for(j=0; j<8; j++)
      if(fen_array[i][j] == piece) {
	r = i; c = j; 
	king_square_found = true;
	break;
      }
  }

  if(piece == 'K')
    for(i=0; i<8;i++)
      for(j=0;j<8; j++)
    {
      char opposite_piece = fen_array[i][j];
      if((opposite_piece == 'q' ||
	  opposite_piece == 'r' ||
	  opposite_piece == 'b' ||
	  opposite_piece == 'n' ||
	  opposite_piece == 'p') &&
	 does_piece_attack_square(i,j, r,c, fen_array) == true)
	return true;
    }

  if(piece == 'k')
    for(i=0; i<8;i++)
      for(j=0;j<8; j++)
    {
      char opposite_piece = fen_array[i][j];
      if((opposite_piece == 'Q' ||
	  opposite_piece == 'R' ||
	  opposite_piece == 'B' ||
	  opposite_piece == 'N' ||
	  opposite_piece == 'P') &&
	 does_piece_attack_square(i,j, r,c, fen_array) == true)
	return true;
    }

  return false;
}

enum bool on_same_diagonal(int r1, int c1,
			   int r2, int c2)
{
  /* if(abs(r1*8+c1 - (r2*8+c2)) % 7 == 0 || */
  /*    abs(r1*8+c1 - (r2*8+c2)) % 9 == 0) */
  if(abs(r1-r2) == abs(c1-c2))
    return true;
  else
    return false;
}

enum bool can_pawn_move_to_square(int r1, int c1,
				  int r2, int c2,
				  char **fen_array)
{
  char piece = fen_array[r1][c1];

  assert(piece == 'P' || piece == 'p');

  if(piece == 'P')
  {
    if(r2 == 4 && r1 == 6 && c1 == c2 && fen_array[5][c1] == 0)
      return true;
    else if(r2 == r1-1 && c1 == c2 && fen_array[r2][c2] == 0)
      return true;
    else if(abs(c2-c1) == 1 && r2 == r1-1 && fen_array[r2][c2] != 0)
      return true;
    else
      return false;
  }
  else if(piece == 'p')
  {
    if(r2 == 3 && r1 == 1 && c1 == c2 && fen_array[2][c1] == 0)
      return true;
    else if(r2 == r1+1 && c1 == c2 && fen_array[r2][c2] == 0)
      return true;
    else if(abs(c2-c1) == 1 && r2 == r1+1 && fen_array[r2][c2] != 0)
      return true;
    else
      return false;
  }
  else
    assert(false);

  return false;
}

enum bool can_move_to_square(int r1, int c1,
			     int r2, int c2,
			     char **fen_array)
{
  char piece = fen_array[r1][c1];

  assert(piece != 0);

  if((piece == 'P' || piece == 'p') &&
     can_pawn_move_to_square(r1,c1, r2,c2, fen_array) == true)
    return true;

  if(piece == 'R' || piece == 'r')
    if(r1 == r2 || c1 == c2)
      return true;

  if(piece == 'B' || piece == 'b')
    if(on_same_diagonal(r1,c1, r2,c2) == true)
      return true;

  if(piece == 'N' || piece == 'n')
    if((abs(r1-r2) == 1 && abs(c1-c2) == 2) ||
       (abs(r1-r2) == 2 && abs(c1-c2) == 1))
      return true;

  if(piece == 'Q' || piece == 'q')
    if(r1 == r2 || c1 == c2 || on_same_diagonal(r1,c1, r2,c2) == true)
      return true;

  if(piece == 'K' || piece == 'k')
  {
    if((r1 == r2 || abs(r1-r2) == 1) &&
       (c1 == c2 || abs(c1-c2) == 1))
      return true;

    if(piece == 'K' &&
       r1 == 7 && c1 == 4 &&
       ((r2 == 7 && c2 == 6 &&
	 fen_array[7][5] == 0 &&
	 fen_array[7][6] == 0 && white_can_castle_o_o == true) ||
	(r2 == 7 && c2 == 2 &&
	 fen_array[7][1] == 0 &&
	 fen_array[7][2] == 0 &&
	 fen_array[7][3] == 0 && white_can_castle_o_o_o == true)) &&
       is_king_under_check(white, fen_array) == false)
      return true;
    else if(piece == 'k' &&
       r1 == 0 && c1 == 4 &&
       ((r2 == 0 && c2 == 6 &&
	 fen_array[0][5] == 0 &&
	 fen_array[0][6] == 0 && black_can_castle_o_o == true) ||
	(r2 == 0 && c2 == 2 &&
	 fen_array[0][1] == 0 &&
	 fen_array[0][2] == 0 &&
	 fen_array[0][3] == 0 && black_can_castle_o_o_o == true)) &&
       is_king_under_check(black, fen_array) == false)
      return true;
  }
  return false;
}

enum bool no_intervening_pieces_between(int r1, int c1,
					int r2, int c2,
					char **fen_array)
{
  char piece = fen_array[r1][c1];

  assert(piece == 'Q' || piece == 'q' ||
	 piece == 'R' || piece == 'r' ||
	 piece == 'B' || piece == 'b');

  assert(can_move_to_square(r1,c1, r2,c2, fen_array) == true);

  int lower_r = min(r1, r2);
  int upper_r = max(r1, r2);

  int lower_c = min(c1, c2);
  int upper_c = max(c1, c2);

  int i, j;

  if(piece == 'R' || piece == 'r')
  {
    if(r1 == r2)
    {
      for(j=lower_c+1; j<=upper_c-1; j++)
	if(fen_array[r1][j] != 0)
	  return false;
    }
    else if(c1 == c2)
    {
      for(i=lower_r+1; i<=upper_r-1; i++)
	if(fen_array[i][c1] != 0)
	  return false;
    }
    else
      assert(false);
  }
  else if(piece == 'Q' || piece == 'q')
  {
    if(r1 == r2)
    {
      for(j=lower_c+1; j<=upper_c-1; j++)
	if(fen_array[r1][j] != 0)
	  return false;
    }
    else if(c1 == c2)
    {
      for(i=lower_r+1; i<=upper_r-1; i++)
	if(fen_array[i][c1] != 0)
	  return false;
    }
    else
    {
      int index1 = r1*8 + c1;
      int index2 = r2*8 + c2;

      int min_index = min(index1, index2);
      int max_index = max(index1, index2);

      int i;
      int step_size;

      if(r1<r2 && c1<c2 || r1>r2 && c1<c2)
	step_size = 9;
      else if(r1<r2 && c1>c2 || r1>r2 && c1>c2)
	step_size = 7;

      for(i=min_index+step_size; i<=max_index-step_size; i += step_size)
	if(fen_array[i/8][i%8] != 0)
	  return false;
    }
  }
  else if(piece == 'B' || piece == 'b')
  {
    int index1 = r1*8 + c1;
    int index2 = r2*8 + c2;

    int min_index = min(index1, index2);
    int max_index = max(index1, index2);

    int i;
    int step_size;

    if(r1<r2 && c1<c2 || r1>r2 && c1>c2)
      step_size = 9;
    else if(r1<r2 && c1>c2 || r1>r2 && c1<c2)
      step_size = 7;

    for(i=min_index+step_size; i<=max_index-step_size; i += step_size)
      if(fen_array[i/8][i%8] != 0)
	return false;
  }

  return true;
}

enum bool does_piece_attack_square(int r1, int c1, 
				   int r2, int c2, 
				   char **fen_array)
{
  //1. find the piece at the square (r1,c1) and hence its colour
  //2. if the piece is a pawn, if (r2,c2) is diagonally adjacent to
  //   (r1,c1), return true or false based on the colour of the piece
  //2. if (r2,c2) is not a square the piece can move to, return false
  //3. if the piece is a knight, return true
  //4. if there are no intervening pieces between (r1,c1) and (r2,c2)
  //   return true
  //5. return false

  char piece = fen_array[r1][c1];
  enum side s = (piece >= 65 && piece <= 90) ? white : black;

  if(piece == 'P')
    if(r2 == r1-1 && abs(c1-c2) == 1)
      return true;

  if(piece == 'p')
    if(r2 == r1+1 && abs(c1-c2) == 1)
      return true;

  if(piece != 'P' &&
     piece != 'p' &&
     can_move_to_square(r1,c1, r2,c2, fen_array) == false)
    return false;

  if(piece == 'N' || piece == 'n')
    return true;

  if((piece == 'Q' || piece == 'q' ||
      piece == 'R' || piece == 'r' ||
      piece == 'B' || piece == 'b') &&
     no_intervening_pieces_between(r1,c1, 
				   r2,c2, 
				   fen_array) == true)
    return true;

  return false;
}

enum bool is_valid_move(int r1, int c1,
			int r2, int c2,
			char **fen_array)
{
  /*
  1. if target square has piece of same colour, return false
  2. if target square cannot be reached by a legal move for the
     piece in question, return false
  3. if the piece is not a knight, if way to target square is blocked,
     return false
  4. if it's check and the move doesn't block the check, return false
  */

  char piece = fen_array[r1][c1];
  char p = fen_array[r2][c2];

  if(piece == 0)
    return false;

  if((piece >= 65 && piece <= 90 &&
      p     >= 65 && p     <= 90) ||
     (piece >= 97 && piece <= 122 &&
      p     >= 97 && p     <= 122))
    return false;
 
  if(can_move_to_square(r1,c1, r2,c2, fen_array) == false)
    return false;

  if(piece != 'N' && piece != 'n' &&
     piece != 'P' && piece != 'p' &&
     piece != 'K' && piece != 'k' &&
     no_intervening_pieces_between(r1,c1, r2,c2, fen_array) == false)
    return false;

  enum side s = (piece >= 65 && piece <= 90) ? white : black;

  if(s == white && piece == 'K' && 
     r1 == 7 && c1 == 4 &&
     r2 == 7 && c2 == 6)
  {
    if(is_square_under_attack(s, 7,5, fen_array) == true ||
       is_square_under_attack(s, 7,6, fen_array) == true)
      return false;
  }
  else if(s == white && piece == 'K' && 
	  r1 == 7 && c1 == 4 &&
	  r2 == 7 && c2 == 2)
  {
    if(is_square_under_attack(s, 7,1, fen_array) == true ||
       is_square_under_attack(s, 7,2, fen_array) == true ||
       is_square_under_attack(s, 7,3, fen_array) == true)
      return false;
  }
  else if(s == black && piece == 'k' && 
     r1 == 0 && c1 == 4 &&
     r2 == 0 && c2 == 6)
  {
    if(is_square_under_attack(s, 0,5, fen_array) == true ||
       is_square_under_attack(s, 0,6, fen_array) == true)
      return false;
  }
  else if(s == black && piece == 'k' && 
	  r1 == 0 && c1 == 4 &&
	  r2 == 0 && c2 == 2)
  {
    if(is_square_under_attack(s, 0,1, fen_array) == true ||
       is_square_under_attack(s, 0,2, fen_array) == true ||
       is_square_under_attack(s, 0,3, fen_array) == true)
      return false;
  }

  char **updated_fen_array = create_new_fen();

  copy_fen(fen_array, updated_fen_array);
    
  updated_fen_array[r1][c1] = 0;
  updated_fen_array[r2][c2] = piece;

  if(is_king_under_check(s, fen_array) == true &&
     is_king_under_check(s, updated_fen_array) == true)
  {
    free(updated_fen_array);
    return false;
  }

  free(updated_fen_array);

  return true;
}

enum bool is_black_piece(char p)
{
  if(p >= 97 && p <= 122)
    return true;
  else
    return false;
}

enum bool is_white_piece(char p)
{
  if(p >= 65 && p <= 90)
    return true;
  else
    return false;
}

enum bool is_square_under_attack(enum side s, int r, int c, char **fen_array)
{
  int i, j;

  if(s == white) {
    for(i=0;i<8;i++) {
      for(j=0;j<8;j++) {

	char piece = fen_array[i][j];
	if(is_black_piece(piece) == true) {

	  if(piece != 'p') {

	    if(piece == 'n') {
	      if(can_move_to_square(i,j, r,c, fen_array) == true)
		return true;
	    }
	    else {
	      if(can_move_to_square(i,j, r,c, fen_array) == true &&
		 no_intervening_pieces_between(i,j, r,c, fen_array) == true)
	      return true;
	    }
	  }
	  else {
	    if(i == r-1 && (j == c-1 || j == c+1))
	      return true;
	  }   
	}
      }
    }
  }
  else if(s == black) {
    for(i=0;i<8;i++) {
      for(j=0;j<8;j++) {

	char piece = fen_array[i][j];
	if(is_white_piece(piece) == true) {

	  if(piece != 'P') {

	    if(piece == 'N') {
	      if(can_move_to_square(i,j, r,c, fen_array) == true)
		return true;
	    }
	    else {
	      if(can_move_to_square(i,j, r,c, fen_array) == true &&
		 no_intervening_pieces_between(i,j, r,c, fen_array) == true)
	      return true;
	    }
	  }
	  else {
	    if(i == r+1 && (j == c-1 || j == c+1))
	      return true;
	  }   
	}
      }
    }
  }
  else
    assert(false);
}

char find_disambig_char(int r1, int c1, int r2, int c2, char **fen_array)
{
  int i, j;

  char p = fen_array[r1][c1];

  for(i=0;i<8;i++)
  {
    for(j=0;j<8;j++)
    {
      char p1 = fen_array[i][j];
      if(p1 == p &&
	 i != r1 && j != c1 &&
	 can_move_to_square(i,j, r2,c2, fen_array) == true &&
	 (p1 == 'n' || p1 == 'N' || p1 != 'n' && 
	  p1 != 'N' && no_intervening_pieces_between(i,j, r2,c2, fen_array) == true))
      {
	if(i == r1)
	  return c1 + 97;
	else if(j == c1)
	  return 8 - r1 + '0';
	else
	  return 0;
      }
    }
  }
  return 0;
}

char *convert_to_algebraic_notation(int r1, int c1, int r2, int c2, char **fen_array)
{
  char *s = (char *)malloc(10*sizeof(char));
  memset(s, '\0', 10);

  int len = 0;

  char piece = fen_array[r1][c1];

  if(piece == 'K' ||
     piece == 'Q' || 
     piece == 'R' || 
     piece == 'B' ||
     piece == 'N')
    len += sprintf(s+len, "%c", piece);
  else if(piece == 'k' ||
	  piece == 'q' || 
	  piece == 'r' || 
	  piece == 'b' ||
	  piece == 'n')
    len += sprintf(s+len, "%c", piece - 32);

  if(fen_array[r2][c2] != 0)
  {
    if(piece == 'P' || piece == 'p')
      len += sprintf(s+len, "%c", c1+97);
    len += sprintf(s+len, "x");
  }
    

  char *s1 = convert_fen_to_algebraic_coords(r2,c2);

  len += sprintf(s+len, "%s", s1);

  free(s1);

  return s;
}

void append_ply_to_moves_list()
{
  GtkTextBuffer *tbuffer;
  GtkTextIter ei;

  tbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(moves_text_view));
  gtk_text_buffer_get_end_iter(tbuffer, &ei);

  char buf[20];
  memset(buf, '\0', 20);

  if(nof_plys % 2 == 0)
    sprintf(buf, "%d. %s ", nof_plys/2 + 1, new_pgn_moves[nof_plys/2].white_move);
  else
    sprintf(buf, "%s\n", new_pgn_moves[nof_plys/2].black_move);

  gtk_text_buffer_insert(tbuffer, &ei, buf, -1);

  GtkTextIter si;
  gtk_text_buffer_get_start_iter(tbuffer, &si);
  gtk_text_buffer_get_end_iter(tbuffer, &ei);
}

void move_forward_new_pgn()
{
  if(current_ply == nof_plys)
    return;

  unhighlight_all_moves_new_pgn();
  current_ply++;
  highlight_ply_new_pgn(true, current_ply);
  fill_grid(grid, new_pgn_fens[current_ply]);
}

void move_backward_new_pgn()
{
  if(current_ply == 0)
    return;

  unhighlight_all_moves_new_pgn();
  current_ply--;
  if(current_ply>0)
    highlight_ply_new_pgn(true, current_ply);
  fill_grid(grid, new_pgn_fens[current_ply]);
}

void free_new_pgn_data_structures()
{
  if(new_pgn_moves)
  {
    free(new_pgn_moves);
    new_pgn_moves = NULL;
  }

  int i, j;
  
  if(new_pgn_fens)
  {
    for(i=0;i<nof_plys+1;i++)
    {
      for(j=0;j<8;j++)
	if(new_pgn_fens[i][j])
	{
	  free(new_pgn_fens[i][j]);
	  new_pgn_fens[i][j] = NULL;
	}

      free(new_pgn_fens[i]);
      new_pgn_fens[i] = NULL;
    }

    free(new_pgn_fens);
    new_pgn_fens = NULL;
  }  
}

void highlight_ply_new_pgn(enum bool highlight, int ply_no)
{
  //assert(ply_no > 0 && ply_no <= nof_plys);

  int move_no = (ply_no % 2 == 1) ? ply_no / 2 : (ply_no - 2) / 2;

  GtkTextIter line_start_iter, line_end_iter;
  GtkTextIter match_start1, match_start2, match_start3, match_end1, match_end2, match_end3;

  gtk_text_buffer_get_iter_at_line(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                   &line_start_iter,
                                   move_no);

  gtk_text_buffer_get_iter_at_line(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                   &line_end_iter,
                                   move_no+1);

  //find the first space ("1. e4")
  gtk_text_iter_forward_search(&line_start_iter,
                               " ",
                               GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                               &match_start1,
                               &match_end1,
                               NULL);

  //find the second space ("1. e4 c5")
  gtk_text_iter_forward_search(&match_end1,
                                " ",
                                GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                &match_start2,
                                &match_end2,
                                NULL);

  if(ply_no % 2 == 0)
    //find the newline ("1. e4 c5\n")
    gtk_text_iter_forward_search(&match_end1,
				 "\n",
				 GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
				 &match_start3,
				 &match_end3,
				 NULL);

  if(highlight == true)
  {
    if(ply_no % 2 == 1)
      gtk_text_buffer_apply_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                        "cyan_bg",
                                        &match_end1,
                                        &match_start2);
    else if(ply_no % 2 == 0)
    {
      gtk_text_buffer_apply_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                        "cyan_bg",
                                        &match_end2,
                                        &match_start3);    
    }
    else
      assert(false);
  }
  else
  {
    if(ply_no % 2 == 1)
      gtk_text_buffer_remove_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                         "cyan_bg",
                                         &match_end1,
                                         &match_start2);
    else if(ply_no %2 == 0)
    {
      gtk_text_buffer_remove_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                         "cyan_bg",
                                         &match_end2,
                                         &match_start3);    
    }
    else
      assert(false);
  }  
}

void unhighlight_all_moves_new_pgn()
{
  int i;
  for(i=1;i<=nof_plys; i++)
  {
    highlight_ply_new_pgn(false, i);
    //highlight_ply_new_pgn(false, i);
  }
}

void display_comment_new_pgn()
{
  if(current_ply == 0)
    return;

  if(new_pgn_moves)
  {
    int move_no = (current_ply % 2 == 1) ? current_ply / 2 : (current_ply - 2) / 2;

    if(current_ply % 2 == 1 && strlen(new_pgn_moves[move_no].white_comment) > 0)
      set_comment_view_text(new_pgn_moves[move_no].white_comment);
    else if(current_ply % 2 == 0 && strlen(new_pgn_moves[move_no].black_comment) > 0)
      set_comment_view_text(new_pgn_moves[move_no].black_comment);
    else
      set_comment_view_text("");
  }
}

void annot_new_pgn()
{
  if(current_ply == 0)
    return;

  int move_no = (current_ply % 2 == 1) ? current_ply / 2 : (current_ply - 2) / 2;

  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)annotation_text_view),
                           current_ply % 2 == 1 ? new_pgn_moves[move_no].white_comment : new_pgn_moves[move_no].black_comment, 
                           -1);

  gtk_widget_show_all((GtkWidget *)annotation_window);
  gtk_widget_grab_focus(annotation_text_view);
}

void accept_comment_new_pgn(GtkWidget *widget, gpointer data)
{
  if(current_ply == 0)
    return;

  GtkTextIter start, end;
  GtkTextBuffer *buf = gtk_text_view_get_buffer((GtkTextView *)annotation_text_view);

  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);

  gchar *txt = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

  int move_no = (current_ply % 2 == 1) ? current_ply / 2 : (current_ply - 2) / 2;

  if(current_ply % 2 == 1)
  {
    memset(new_pgn_moves[move_no].white_comment, '\0', 200);
    strcpy(new_pgn_moves[move_no].white_comment, txt);
  }
  else if(current_ply % 2 == 0)
  {
    memset(new_pgn_moves[move_no].black_comment, '\0', 200);
    strcpy(new_pgn_moves[move_no].black_comment, txt);
  }
  else
    assert(false);

  set_comment_view_text(txt);

  gtk_widget_hide((GtkWidget *)annotation_window);
}

void process_promotion()
{
  promotion_new_fen[selected_r][selected_c] = 0;
  promotion_new_fen[promotion_d/8][promotion_d%8] = promotion_choice;

  if(strlen(promotion_move_text) == 0)
  {
    char *temp = convert_to_algebraic_notation(selected_r, selected_c, promotion_d/8, promotion_d%8, new_pgn_fens[promotion_index-1]);
    strcpy(promotion_move_text, temp);
    free(temp);
  }
  
  if(promotion_side == white)
  {
    if(promotion_choice == 'Q')
      strcat(promotion_move_text, "=Q");
    else if(promotion_choice == 'R')
      strcat(promotion_move_text, "=R");
    else if(promotion_choice == 'B')
      strcat(promotion_move_text, "=B");
    else if(promotion_choice == 'N')
      strcat(promotion_move_text, "=N");
    else
      assert(false);
  }
  else if(promotion_side == black)
  {
    if(promotion_choice == 'q')
      strcat(promotion_move_text, "=Q");
    else if(promotion_choice == 'r')
      strcat(promotion_move_text, "=R");
    else if(promotion_choice == 'b')
      strcat(promotion_move_text, "=B");
    else if(promotion_choice == 'n')
      strcat(promotion_move_text, "=N");
    else
      assert(false);
  }
  else
    assert(false);

  if(is_white_piece(promotion_new_fen[promotion_d/8][promotion_d%8]) == true && is_king_under_check(black, promotion_new_fen) == true ||
     is_black_piece(promotion_new_fen[promotion_d/8][promotion_d%8]) == true && is_king_under_check(white, promotion_new_fen) == true)
    strcat(promotion_move_text, "+");

  if(nof_plys % 2 == 0)
  {
    state = black_to_move;

    if(new_pgn_moves == NULL)
      new_pgn_moves = (move_t *)malloc(sizeof(move_t));
    else
      new_pgn_moves = (move_t *)realloc(new_pgn_moves, (nof_plys/2 + 1) * sizeof(move_t));

    memset(new_pgn_moves[nof_plys/2].white_move, '\0', 10);
    strcpy(new_pgn_moves[nof_plys/2].white_move, promotion_move_text);

    memset(new_pgn_moves[nof_plys/2].white_comment, '\0', 200);
    memset(new_pgn_moves[nof_plys/2].black_comment, '\0', 200);
  }
  else
  {
    state = white_to_move;

    memset(new_pgn_moves[nof_plys/2].black_move, '\0', 10);

    strcpy(new_pgn_moves[nof_plys/2].black_move, promotion_move_text);
  }

  free(promotion_move_text);

  append_ply_to_moves_list();

  nof_plys++;
  current_ply++;

  unhighlight_all_moves_new_pgn();
  highlight_ply_new_pgn(true, nof_plys);

  int new_size = nof_plys + 1;

  new_pgn_fens = (char ***)realloc(new_pgn_fens, new_size * sizeof(char **));

  new_pgn_fens[new_size - 1] = promotion_new_fen;

  fill_grid(grid, promotion_new_fen);
}

void undo()
{
  if((current_mode == pgn_from_position || current_mode == pgn_from_scratch) && nof_plys > 0)
  {
    int move_no = (current_ply % 2 == 1) ? current_ply / 2 : (current_ply - 2) / 2;

    /* if(state == white_to_move && !strcmp(new_pgn_moves[move_no].black_move, "O-O")) */
    /*   black_can_castle_o_o = true; */
    /* else if(state == white_to_move && !strcmp(new_pgn_moves[move_no].black_move, "O-O-O")) */
    /*   black_can_castle_o_o_o = true; */
    /* else if(state == black_to_move && !strcmp(new_pgn_moves[move_no].white_move, "O-O")) */
    /*   white_can_castle_o_o = true; */
    /* else if(state == black_to_move && !strcmp(new_pgn_moves[move_no].white_move, "O-O-O")) */
    /*   white_can_castle_o_o_o = true; */
    if(state == white_to_move && 
       (!strcmp(new_pgn_moves[move_no].black_move, "O-O") ||
	!strcmp(new_pgn_moves[move_no].black_move, "O-O-O") ||
	new_pgn_moves[move_no].black_move[0] == 'K' ||
	new_pgn_moves[move_no].black_move[0] == 'R'))
    {
      black_can_castle_o_o = prev_black_can_castle_o_o;
      black_can_castle_o_o_o = prev_black_can_castle_o_o_o;
    }
    else if(state == black_to_move && 
	    (!strcmp(new_pgn_moves[move_no].white_move, "O-O") ||
	     !strcmp(new_pgn_moves[move_no].white_move, "O-O-O") ||
	     new_pgn_moves[move_no].white_move[0] == 'K' ||
	     new_pgn_moves[move_no].white_move[0] == 'R'))
    {
      white_can_castle_o_o = prev_white_can_castle_o_o;
      white_can_castle_o_o_o = prev_white_can_castle_o_o_o;
    }

    if(current_ply == nof_plys)
      current_ply--;

    nof_plys--;

    if(nof_plys % 2 == 0)
      new_pgn_moves = (move_t *)realloc(new_pgn_moves, (nof_plys/2) * sizeof(move_t));

    new_pgn_fens = (char ***)realloc(new_pgn_fens, (nof_plys+1) * sizeof(char **));
  
    populate_moves_text_view();

    unhighlight_all_moves_new_pgn();

    if(current_ply > 0)
      highlight_ply_new_pgn(true, current_ply);

    fill_grid(grid, new_pgn_fens[nof_plys]);

    if(state == white_to_move)
      state = black_to_move;
    else if(state == black_to_move)
      state = white_to_move;
    else
      assert(false);
  }
}

void undo_clicked(GtkWidget *widget, gpointer data)
{
  undo();
}

void new_pgn_from_pos()
{
  free_new_pgn_data_structures();

  if(pos_setup_white_to_move == true)
  {
    state = white_to_move;

    new_pgn_fens = (char ***)malloc(sizeof(char **));

    new_pgn_fens[0] = initial_pos_fen;

    nof_plys = 0;
    current_ply = 0;

    fill_grid(grid, initial_pos_fen);

    gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)moves_text_view), "", -1);
  }
  else
  {
    state = black_to_move;

    new_pgn_fens = (char ***)malloc(2 * sizeof(char **));
    new_pgn_fens[0] = initial_pos_fen;

    char **fen = create_new_fen();
    copy_fen(initial_pos_fen, fen);
    new_pgn_fens[1] = fen;

    nof_plys = 1;
    current_ply = 1;

    new_pgn_moves = (move_t *)malloc(sizeof(move_t));

    memset(new_pgn_moves[0].white_move, '\0', 10);
    strcpy(new_pgn_moves[0].white_move, "...");

    memset(new_pgn_moves[0].white_comment, '\0', 100);

    fill_grid(grid, new_pgn_fens[1]);

    gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)moves_text_view), "1. ... ", -1);
  }

  initialize_non_move_data();

  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)comment_text_view), "", -1);
}
