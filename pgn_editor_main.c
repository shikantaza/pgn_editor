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

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "pgn_editor.h"

//global variables
GtkWidget *annotation_window;
GtkWidget *window;
GtkWidget *grid;
GtkWidget *moves_text_view;
GtkWidget *comment_text_view;
GtkWidget *annotation_text_view;

char event[100];
char site[100];
char date[100];
char round1[100]; //so named to avoid conflict with built in round() function
char white_player[100];
char black_player[100];
char result[100];

unsigned int nof_moves;
unsigned int move_no;
unsigned int ply_no;

move_t *moves;
char ***fens;

char pgn_file_name[100];

enum bool changed = false;

//end of global variables

//extern variables
extern enum bool in_move_text;
extern FILE *yyin;

extern char *substring(char *, int, int);

extern void new_pgn_file(GtkWidget *, gpointer);
extern void new_pgn();
extern void board_clicked(GtkWidget *, GdkEventButton *, gpointer);
//end of extern variables

//forward declarations
void convert_fen_string_to_fen_array(char *, char **);
void fill_grid(GtkWidget *, char **);
void populate_moves_text_view();
void save();
void unhighlight_all_moves();
void set_comment_view_text(char *);
//end of forward declarations

inline int min(int a, int b) { return (a <= b) ? a : b; }
inline int max(int a, int b) { return (a >= b) ? a : b; }
inline abs(int a) { return (a >= 0) ? a : (-1)*a; }

int find_square(char piece, char **fen_array, int start_square)
{
  int i, j;

  int r = start_square/8;
  int c = start_square%8;

  for(j=c; j<8;j++)
    if(fen_array[r][j] == piece)
      return r*8+j;

  for(i=r+1; i<8; i++)
    for(j=0; j<8; j++)
      if(fen_array[i][j] == piece)
        return i*8 + j;

  return NOT_FOUND;
}

int convert_char(char c)
{
  if(c >= 'a' && c <= 'h')
    return (c - 96) - 1;
  else if(c >= '1' && c <= '8')
    return 8 - (c - '0');
  else
    assert(false);
}

char *convert_fen_to_algebraic_coords(int r, int c)
{
  char *s = (char *)malloc(3*sizeof(char));
  memset(s, '\0', 3);

  s[0] = c + 97;
  s[1] = 8 - r + '0';

  return s;
}

int convert_algebraic_to_fen_coords(char s[2])
{
  //colnum in FEN array = (colnum in algebraic notation) - 1
  //rownum in FEN array = 8 - (rownum in algebraic notation)
  int c = (s[0] - 96) - 1;   
  int r = 8 - (s[1] - '0');  

  return (r*8) + c ;
}

move_internal(char piece, char s[2], char **fen_array, char disambig_char)
{
  int index = convert_algebraic_to_fen_coords(s);
  int r = index / 8;
  int c = index % 8;

  if(piece == 'K' || piece == 'k' || piece == 'Q' || piece == 'q')
  {
    int sq = find_square(piece, fen_array, 0);
    fen_array[sq/8][sq%8] = 0;
    fen_array[r][c] = piece;    
  }
  else if(piece == 'R' || piece == 'r')
  {
    int sq = find_square(piece, fen_array, 0);
    int current_r = sq/8;
    int current_c = sq%8;

    enum bool match;

    if(disambig_char)
    {
      if(disambig_char >= 'a' && disambig_char <= 'h')
      {
        if(current_c == (disambig_char - 96) - 1)
          match = true;
        else
          match = false;
      }
      else if(disambig_char >= '1' && disambig_char <= '8')
      {
        if(current_r == 8 - (c - '0'))
          match = true;
        else
          match = false;
      }
    }
    else
    {
      match = true;

      if(current_r == r) //rook is on the same rank
      {
        int ii;

        int lower = min(current_c, c);
        int upper = max(current_c, c);

        for(ii=lower+1; ii<=upper-1; ii++)
          if(fen_array[r][ii] != 0) //intervening squares not empty
          {
            match = false;
            break;
          }
      }
      else if(current_c == c) //rook is on the same file
      {
        int ii;
        int lower = min(current_r, r);
        int upper = max(current_r, r);

        for(ii=lower+1; ii<=upper-1; ii++)
          if(fen_array[ii][c] != 0) //intervening squares not empty
          {
            match = false;
            break;
          }
      }
      else
        match = false;
    }

    if(match == true)
      fen_array[current_r][current_c] = 0;
    else //it has to be the other rook
    {
      int sq1 = find_square(piece, fen_array, sq+1);
      fen_array[sq1/8][sq1%8] = 0;
    }

    fen_array[r][c] = piece;
  }
  else if(piece == 'B' || piece == 'b')
  {
    int sq = find_square(piece, fen_array, 0);
    int current_r = sq/8;
    int current_c = sq%8;

    if((r+c) % 2 == (current_r+current_c) % 2) //bishop on same coloured square
      fen_array[current_r][current_c] = 0;
    else
    {
      int sq1 = find_square(piece, fen_array, sq+1);
      fen_array[sq1/8][sq1%8] = 0;
    }

    fen_array[r][c] = piece;
  }
  else if(piece == 'N' || piece == 'n')
  {
    int sq = find_square(piece, fen_array, 0);
    int current_r = sq/8;
    int current_c = sq%8;

    if(disambig_char)
    {
      if(disambig_char >= 'a' && disambig_char <= 'h')
      {
        if(current_c == (disambig_char - 96) - 1)
          fen_array[current_r][current_c] = 0;
        else
        {
          int sq1 = find_square(piece, fen_array, sq+1);
          fen_array[sq1/8][sq1%8] = 0;
        }
      }
      else if(disambig_char >= '1' && disambig_char <= '8')
      {
        if(current_r == 8 - (c - '0'))
          fen_array[current_r][current_c] = 0;
        else
        {
          int sq1 = find_square(piece, fen_array, sq+1);
          fen_array[sq1/8][sq1%8] = 0;
        }
      }
    }
    else
    {
      if((abs(current_r-r) == 1 && abs(current_c-c) == 2) ||
         (abs(current_r-r) == 2 && abs(current_c-c) == 1))
        fen_array[current_r][current_c] = 0;
      else
      {
       int sq1 = find_square(piece, fen_array, sq+1);
       fen_array[sq1/8][sq1%8] = 0;
      }
    }
    fen_array[r][c] = piece;
  }
}

//converts a given PGN file into an array of move_t values.
//alse passes back the number of moves in nof_moves
move_t *convert_pgn(char *pgn_file_name, int *nof_moves)
{
  //TODO
  return NULL;
}

void update_fen(enum side s, char *move_str, char **fen_array)
{
  char piece_symbol, promotion_piece;
  enum bool check, mate, promotion;

  char mv1[10];
  memset(mv1, '\0', 10);

  char mv[10];
  memset(mv, '\0', 10);

  check = (move_str[strlen(move_str)-1] == '+') ? true : false;
  mate = (move_str[strlen(move_str)-1] == '#') ? true : false;

  if(check == true || mate == true)
    strncpy(mv1, move_str, strlen(move_str)-1);
  else
    strcpy(mv1, move_str);

  promotion = mv1[strlen(mv)-2] == '=' ? true : false;

  if(promotion == true)
  {
    promotion_piece = mv[strlen(mv)-1];
    strncpy(mv, mv1, strlen(mv)-3); 
  }
  else
    strcpy(mv, mv1);

  if(!strcmp(mv, "O-O"))
  {
    if(s == white)
    {
      fen_array[7][4] = 0;
      fen_array[7][7] = 0;
      fen_array[7][6] = 'K';
      fen_array[7][5] = 'R';
    }
    else
    {
      fen_array[0][4] = 0;
      fen_array[0][7] = 0;
      fen_array[0][6] = 'k';
      fen_array[0][5] = 'r';
    }
  }
  else if(!strcmp(mv, "O-O-O"))
  {
    if(s == white)
    {
      fen_array[7][0] = 0;
      fen_array[7][4] = 0;
      fen_array[7][2] = 'K';
      fen_array[7][3] = 'R';
    }
    else
    {
      fen_array[0][0] = 0;
      fen_array[0][4] = 0;
      fen_array[0][2] = 'k';
      fen_array[0][3] = 'r';
    }
  }

  if(strlen(mv) == 2) //pawn move
  {
    int index = convert_algebraic_to_fen_coords(mv);
    int r = index / 8;
    int c = index % 8;

    if(s == white)
    {
      if(r == 4)
      {
        //if it's a two-square move
        if(fen_array[r+1][c] == 0)
          fen_array[r+2][c] = 0;
        else
          fen_array[r+1][c] = 0;
      }
      else
        fen_array[r+1][c] = 0;

      if(promotion == true)
        fen_array[r][c] = promotion_piece;
      else
        fen_array[r][c] = 'P';
    }
    else
    {
      if(r == 3)
      {
        //if it's a two-square move
        if(fen_array[r-1][c] == 0)
          fen_array[r-2][c] = 0;
        else
          fen_array[r-1][c] = 0;
      }
      else
        fen_array[r-1][c] = 0;

      if(promotion == true)
        fen_array[r][c] = promotion_piece + 32; //convert to lowercase
      else
        fen_array[r][c] = 'p';
    }
  }
  else if(strlen(mv) == 3) // non-pawn move
  {
    move_internal((s == white) ? mv[0] : mv[0]+32, mv+1, fen_array, 0);
  }
  else if(strlen(mv) == 4) 
  {
    if(mv[1] == 'x') //capture
    {
      if(mv[0] == 'K' || mv[0] == 'Q' || mv[0] == 'R' ||
         mv[0] == 'B' || mv[0] == 'N')
      {
        move_internal((s == white) ? mv[0] : mv[0]+32, mv+2, fen_array, 0);
      }
      else //capture by pawn
      {
        int index = convert_algebraic_to_fen_coords(mv+2);
        int r = index / 8;
        int c = index % 8;

        if(s == white)
        {
          if(fen_array[r][c] == 0) //en passant
            fen_array[r+1][c] = 0;

          fen_array[r][c] = 'P';
          fen_array[r+1][(mv[0] - 96) - 1] = 0;
        }
        else
        {
          if(fen_array[r][c] == 0) //en passant
            fen_array[r-1][c] = 0;

          fen_array[r][c] = 'p';
          fen_array[r-1][(mv[0] - 96) - 1] = 0;
        }
      }
    }
    else //disambiguated move
    {
      move_internal((s == white) ? mv[0] : mv[0]+32, mv+2, fen_array, mv[1]);
    }
  }
  else //disambiguated capture
  {
    move_internal((s == white) ? mv[0] : mv[0]+32, mv+3, fen_array, mv[1]);
  }
}

void remove_all_children(GtkWidget *container)
{
  GList *children, *iter;

  children = gtk_container_get_children(GTK_CONTAINER(container));
  for(iter = children; iter != 0; iter = g_list_next(iter))
    gtk_widget_destroy(GTK_WIDGET(iter->data));
  g_list_free(children);
}

move_t create_move(char *white_move, char *white_comment, char *black_move, char *black_comment)
{
  move_t m;
  memset(m.white_move, '\0', 10);
  strcpy(m.white_move, white_move);

  memset(m.white_comment, '\0', 200);
  strcpy(m.white_comment, white_comment);

  memset(m.black_move, '\0', 10);
  strcpy(m.black_move, black_move);

  memset(m.black_comment, '\0', 200);
  strcpy(m.black_comment, black_comment);

  return m;
}

void quit_application()
{
  GtkWidget *dialog = gtk_message_dialog_new ((GtkWindow *)window,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Do you really want to quit?");

  if(gtk_dialog_run(GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
  {
    gtk_widget_destroy((GtkWidget *)dialog);

    if(changed == true)
    {
      GtkWidget *dialog1 = gtk_message_dialog_new ((GtkWindow *)window,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_QUESTION,
                                                   GTK_BUTTONS_YES_NO,
                                                   "File has been modified; save file?");

      gtk_widget_grab_focus(gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog1), GTK_RESPONSE_YES));

      if(gtk_dialog_run(GTK_DIALOG (dialog1)) == GTK_RESPONSE_YES)
      {
        gtk_widget_destroy((GtkWidget *)dialog1);
        save();
      }
    }

    free(moves);

    int i, j;

    if(fens)
    {
      for(i=0;i<2*nof_moves+1;i++)
      {
	for(j=0;j<8;j++)
	  if(fens[i][j])free(fens[i][j]);

	free(fens[i]);
      }
      free(fens);
    }

    gtk_main_quit();
    exit(0);
  }
  else
    gtk_widget_destroy((GtkWidget *)dialog);
}

void quit(GtkWidget *widget, gpointer data)
{
  quit_application();

}

void copy_fen(char **src, char **dest)
{
  int i,j;
  for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      dest[i][j] = src[i][j];
}

void load_from_file(char *file_name)
{
  in_move_text = false;

  yyin = fopen(file_name, "r");

  assert(yyin);

  yyparse();

  fclose(yyin);

  yyin = NULL;

  fens = (char ***)malloc((2*nof_moves + 1) * sizeof(char **));

  int i,j;

  for(i=0; i<2*nof_moves+1; i++)
  {
    fens[i] = (char **)malloc(8 * sizeof(char *));
    for(j=0;j<8;j++)
      fens[i][j] = (char *)malloc(8 * sizeof(char));
  }

  convert_fen_string_to_fen_array("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", fens[0]);

  for(i=0; i<nof_moves; i++)
  {
    copy_fen(fens[2*i], fens[2*i+1]);
    update_fen(white, moves[i].white_move, fens[2*i+1]);
    copy_fen(fens[2*i+1], fens[2*i+2]);
    update_fen(black, moves[i].black_move, fens[2*i+2]);
  }

  fill_grid(grid, fens[0]);

  populate_moves_text_view();

  gtk_widget_show_all((GtkWidget *)window);

  move_no = 0;
  ply_no = 0;
}

void load()
{
  if(changed == true)
  {
    GtkWidget *dialog1 = gtk_message_dialog_new ((GtkWindow *)window,
                                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_YES_NO,
                                                 "File has been modified; save file?");

    gtk_widget_grab_focus(gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog1), GTK_RESPONSE_YES));

    if(gtk_dialog_run(GTK_DIALOG (dialog1)) == GTK_RESPONSE_YES)
    {
      gtk_widget_destroy((GtkWidget *)dialog1);
      save();
    }
  }

  changed = false;

  GtkWidget *dialog;

  dialog = gtk_file_chooser_dialog_new ("Load PGN File",
                                        (GtkWindow *)window,
                                        GTK_FILE_CHOOSER_ACTION_OPEN,
                                        "Cancel", GTK_RESPONSE_CANCEL,
                                        "Open", GTK_RESPONSE_ACCEPT,
                                        NULL);

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
  {

    strcpy(pgn_file_name, gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)));
    load_from_file(pgn_file_name);
  }

  gtk_widget_destroy (dialog);
}

void load_pgn_file(GtkWidget *widget, gpointer data)
{
  load();
}

void save()
{
  if(strlen(pgn_file_name) == 0)
    return;

  FILE *out = fopen(pgn_file_name, "w");

  assert(out);

  fprintf(out, "[Event \"%s\"]\n",  event);
  fprintf(out, "[Site \"%s\"]\n",   site);
  fprintf(out, "[Date \"%s\"]\n",   date);
  fprintf(out, "[Round \"%s\"]\n",  round1);
  fprintf(out, "[White \"%s\"]\n",  white_player);
  fprintf(out, "[Black \"%s\"]\n",  black_player);
  fprintf(out, "[Result \"%s\"]\n", result);  

  int i;

  for(i=0; i<nof_moves; i++)
  {
    fprintf(out, "%d. %s ", i+1, moves[i].white_move);
    if(strlen(moves[i].white_comment) > 0)
      fprintf(out, "{%s} ", moves[i].white_comment);
    fprintf(out, "%s ", moves[i].black_move);
    if(strlen(moves[i].black_comment) > 0)
      fprintf(out, "{%s} ", moves[i].black_comment);
  }

  fprintf(out, "%s ", result);

  fprintf(out, "\n");

  fclose(out);

  changed = false;
}

void save_pgn_file(GtkWidget *widget, gpointer data)
{
  save();
}

void highlight_ply(enum bool highlight, int move_no, int ply_no)
{
  GtkTextIter line_start_iter, line_end_iter, match_start1, match_start2, match_end1, match_end2;

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
  gtk_text_iter_backward_search(&line_end_iter,
                                " ",
                                GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                &match_start2,
                                &match_end2,
                                NULL);

  if(highlight == true)
  {
    if(ply_no == 1)
      gtk_text_buffer_apply_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                        "cyan_bg",
                                        &match_end1,
                                        &match_start2);
    else if(ply_no == 2)
    {
      gtk_text_buffer_apply_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                        "cyan_bg",
                                        &match_end2,
                                        &line_end_iter);    
    }
    else
      assert(false);
  }
  else
  {
    if(ply_no == 1)
      gtk_text_buffer_remove_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                         "cyan_bg",
                                         &match_end1,
                                         &match_start2);
    else if(ply_no == 2)
    {
      gtk_text_buffer_remove_tag_by_name(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                                         "cyan_bg",
                                         &match_end2,
                                         &line_end_iter);    
    }
    else
      assert(false);
  }
}

void move_forward()
{
  if(strlen(pgn_file_name) == 0)
    return;

  if(move_no == -1)
  {
    move_no = 0;
    ply_no = 0;
  }

  if(move_no == nof_moves-1 && ply_no == 2)
    return;

  if(ply_no == 2)
  {
    move_no++;
    ply_no = 1;

    highlight_ply(false, move_no-1, 2);
  }
  else
  {
    if(ply_no == 0)ply_no = 1;
    else           ply_no = 2;

    highlight_ply(false, move_no, 1);
  }

  fill_grid(grid, fens[2*move_no + ply_no]);

  highlight_ply(true, move_no, ply_no);
}

void move_backward()
{
  if(strlen(pgn_file_name) == 0)
    return;

  if(move_no == 0 && ply_no == 0)
    return;

  if(move_no == 0 &&  ply_no == 1)
  {
    highlight_ply(false, 0, 1);
    fill_grid(grid, fens[0]);
    ply_no = 0;
    return;
  }

  if(ply_no == 2)
  {
    ply_no = 1;

    highlight_ply(false, move_no, 2);
  }
  else
  {
    move_no--;
    ply_no = 2;

    highlight_ply(false, move_no+1, 1);
  }

  fill_grid(grid, fens[2*move_no + ply_no]);

  if(move_no != -1)
    highlight_ply(true, move_no, ply_no);
}

void forward(GtkWidget *widget, gpointer data)
{
  move_forward();
}

void backward(GtkWidget *widget, gpointer data)
{
  move_backward();
}

void accept_comment(GtkWidget *widget, gpointer data)
{
  GtkTextIter start, end;
  GtkTextBuffer *buf = gtk_text_view_get_buffer((GtkTextView *)annotation_text_view);

  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);

  gchar *txt = gtk_text_buffer_get_text(buf, &start, &end, FALSE);

  if(ply_no == 1)
  {
    memset(moves[move_no].white_comment, '\0', 200);
    strcpy(moves[move_no].white_comment, txt);
  }
  else if(ply_no == 2)
  {
    memset(moves[move_no].black_comment, '\0', 200);
    strcpy(moves[move_no].black_comment, txt);
  }
  else
    assert(false);

  set_comment_view_text(txt);

  changed = true;

  gtk_widget_hide((GtkWidget *)annotation_window);
}

void cancel_comment(GtkWidget *widget, gpointer data)
{
  gtk_widget_hide((GtkWidget *)annotation_window);
}

void annot()
{
  if(strlen(pgn_file_name) == 0)
    return;

  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)annotation_text_view),
                           ply_no == 1 ? moves[move_no].white_comment : moves[move_no].black_comment, 
                           -1);

  gtk_widget_show_all((GtkWidget *)annotation_window);
  gtk_widget_grab_focus(annotation_text_view);
}

void annotate(GtkWidget *widget, gpointer data)
{
  annot();
}

void clear_board(GtkWidget *widget, gpointer data)
{
  fill_grid(grid, fens[0]);
}

void move_to_start_pos()
{
  unhighlight_all_moves();
  move_no = 0;
  ply_no = 0;
  fill_grid(grid, fens[0]);
}

void move_to_end()
{
  move_no = nof_moves-1;

  if(strlen(moves[move_no].black_move) > 0)
    ply_no = 2;
  else
    ply_no = 1;

  unhighlight_all_moves();
  highlight_ply(true, move_no, ply_no);

  fill_grid(grid, fens[2*move_no + ply_no]);
}

gboolean handle_key_press_events(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  if(widget == (GtkWidget *)window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_n)
    new_pgn();
  else if(widget == (GtkWidget *)window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_l)
    load();
  else if(widget == (GtkWidget *)window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_s)
    save();
  else if(widget == (GtkWidget *)window && event->keyval == GDK_KEY_Left)
    move_backward();
  else if(widget == (GtkWidget *)window && event->keyval == GDK_KEY_Right)
    move_forward();
  else if(widget == (GtkWidget *)window && event->keyval == GDK_KEY_Home)
    move_to_start_pos();
  else if(widget == (GtkWidget *)window && event->keyval == GDK_KEY_End)
    move_to_end();
  else if(widget == (GtkWidget *)window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_a)
    annot();
  else if(widget == (GtkWidget *)window && (event->state & GDK_CONTROL_MASK) && event->keyval == GDK_KEY_q)
    quit_application();
}

int is_piece(char c)
{
  return c == 'K' || c == 'Q' || c == 'R' || c == 'B' || c == 'N' || c == 'P' ||
    c == 'k' || c == 'q' || c == 'r' || c == 'b' || c == 'n' || c == 'p';
}

void convert_fen_string_to_fen_array(char *str, char **fen_array)
{
  int i = 0;
  int ii;
  int r = 0, c = 0;

  while(str[i] != ' ') //we're only interested in the positions
  {
    if(is_piece(str[i]))
    {
      fen_array[r][c] = str[i];
      c++;
    }
    else if(str[i] == '/')
    {
      r++;
      c = 0;
    }
    else //if it's not a piece, it's a number indicating how many empty squares
    {
      for(ii=0; ii<str[i] - '0'; ii++)
      {
        fen_array[r][c] = 0;
        c++;
      }
    }

    i++;
  }
}

void fill_grid(GtkWidget *grid, char **fen_array) 
{
  int i, j;

  GtkWidget *boxes[8][8];
  GtkWidget *images[8][8];

  remove_all_children(grid);

  for (i=0; i<8; i++)
  {
    for (j=0; j<8; j++)
    {
      if(fen_array[i][j] == 'K')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_k_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_k_w.png");
      }
      else if(fen_array[i][j] == 'k')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_k_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_k_w.png");
      }
      else if(fen_array[i][j] == 'Q')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_q_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_q_w.png");
      }
      else if(fen_array[i][j] == 'q')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_q_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_q_w.png");
      }
      else if(fen_array[i][j] == 'R')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_r_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_r_w.png");
      }
      else if(fen_array[i][j] == 'r')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_r_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_r_w.png");
      }
      else if(fen_array[i][j] == 'N')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_n_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_n_w.png");
      }
      else if(fen_array[i][j] == 'n')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_n_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_n_w.png");
      }      
      else if(fen_array[i][j] == 'B')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_b_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_b_w.png");
      }
      else if(fen_array[i][j] == 'b')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_b_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_b_w.png");
      }
      else if(fen_array[i][j] == 'P')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/w_p_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/w_p_w.png");
      }
      else if(fen_array[i][j] == 'p')
      {
        if((i + j) % 2)   images[i][j] = gtk_image_new_from_file("icons/b_p_b.png");
        else              images[i][j] = gtk_image_new_from_file("icons/b_p_w.png");
      }        
      else //if(fen_array[i][j] == 0)
      {
        if((i + j) % 2)
          images[i][j] = gtk_image_new_from_file("icons/black_square.png");
        else
          images[i][j] = gtk_image_new_from_file("icons/white_square.png");
      }

      boxes[i][j] = gtk_event_box_new();
      gtk_container_add(GTK_CONTAINER(boxes[i][j]), images[i][j]);

      g_signal_connect(boxes[i][j], "button_press_event", G_CALLBACK(board_clicked), (gpointer)(i*8+j)); 

      //gtk_grid_attach (GTK_GRID (grid), images[i][j], j, i, 1, 1);
      gtk_grid_attach (GTK_GRID (grid), boxes[i][j], j, i, 1, 1);
    }
  }

  if(moves)
  {
    if(ply_no == 1 && strlen(moves[move_no].white_comment) > 0)
      set_comment_view_text(moves[move_no].white_comment);
    else if(ply_no == 2 && strlen(moves[move_no].black_comment) > 0)
      set_comment_view_text(moves[move_no].black_comment);
    else
      set_comment_view_text("");
  }

  gtk_widget_show_all((GtkWidget *)window);
}

void populate_moves_text_view()
{
  char buf[1000];
  memset(buf, '\0', 1000);

  int len = 0;

  int i;

  for(i=0; i<nof_moves; i++)
    len += sprintf(buf+len, "%d. %s %s\n", i+1, moves[i].white_move, moves[i].black_move);

  gtk_text_buffer_set_text(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                             buf, 
                             -1);

}

GtkToolbar *create_toolbar()
{
  GtkWidget *toolbar;

  GtkWidget *new_icon = gtk_image_new_from_file ("icons/new32x32.png");
  GtkWidget *load_icon = gtk_image_new_from_file ("icons/load32x32.png");
  GtkWidget *save_icon = gtk_image_new_from_file ("icons/save32x32.png");
  GtkWidget *backward_icon = gtk_image_new_from_file ("icons/backward.png");
  GtkWidget *forward_icon = gtk_image_new_from_file ("icons/forward.png");
  GtkWidget *annotate_icon = gtk_image_new_from_file ("icons/annotate.png");
  GtkWidget *clear_icon = gtk_image_new_from_file ("icons/clear.png");
  GtkWidget *exit_icon = gtk_image_new_from_file ("icons/exit.png");

  toolbar = gtk_toolbar_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (toolbar), GTK_ORIENTATION_HORIZONTAL);
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_container_set_border_width (GTK_CONTAINER (toolbar), 5);

  GtkToolItem *new_button = gtk_tool_button_new(new_icon, NULL);
  gtk_tool_item_set_tooltip_text(new_button, "New PGN (Ctrl-N)");
  g_signal_connect (new_button, "clicked", G_CALLBACK (new_pgn_file), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, new_button, 0);

  GtkToolItem *load_button = gtk_tool_button_new(load_icon, NULL);
  gtk_tool_item_set_tooltip_text(load_button, "Load PGN (Ctrl-L)");
  g_signal_connect (load_button, "clicked", G_CALLBACK (load_pgn_file), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, load_button, 1);

  GtkToolItem *save_button = gtk_tool_button_new(save_icon, NULL);
  gtk_tool_item_set_tooltip_text(save_button, "Save PGN (Ctrl-S)");
  g_signal_connect (save_button, "clicked", G_CALLBACK (save_pgn_file), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, save_button, 2);

  GtkToolItem *backward_button = gtk_tool_button_new(backward_icon, NULL);
  gtk_tool_item_set_tooltip_text(backward_button, "Back (Left Arrow)");
  g_signal_connect (backward_button, "clicked", G_CALLBACK (backward), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, backward_button, 3);

  GtkToolItem *forward_button = gtk_tool_button_new(forward_icon, NULL);
  gtk_tool_item_set_tooltip_text(forward_button, "Forward (Right Arrow)");
  g_signal_connect (forward_button, "clicked", G_CALLBACK (forward), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, forward_button, 4);

  GtkToolItem *annotate_button = gtk_tool_button_new(annotate_icon, NULL);
  gtk_tool_item_set_tooltip_text(annotate_button, "Annotate (Ctrl-A)");
  g_signal_connect (annotate_button, "clicked", G_CALLBACK (annotate), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, annotate_button, 5);

  /* GtkToolItem *clear_button = gtk_tool_button_new(clear_icon, NULL); */
  /* gtk_tool_item_set_tooltip_text(clear_button, "Clear board"); */
  /* g_signal_connect (clear_button, "clicked", G_CALLBACK (clear_board), NULL); */
  /* gtk_toolbar_insert((GtkToolbar *)toolbar, clear_button, 5); */

  GtkToolItem *exit_button = gtk_tool_button_new(exit_icon, NULL);
  gtk_tool_item_set_tooltip_text(exit_button, "Exit (Ctrl-Q)");
  g_signal_connect (exit_button, "clicked", G_CALLBACK (quit), NULL);
  gtk_toolbar_insert((GtkToolbar *)toolbar, exit_button, 6);

  return (GtkToolbar *)toolbar;
}

void create_annotation_window()
{
  GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(win),
                               500, 200);

  gtk_window_set_transient_for((GtkWindow *)win, (GtkWindow *)window);

  g_signal_connect (GTK_WIDGET(win), "delete_event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);

  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  GtkWidget *scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  annotation_text_view = gtk_text_view_new();

  gtk_container_add (GTK_CONTAINER (scrolled_win), annotation_text_view);

  GtkWidget *ok = gtk_button_new_with_label("OK");
  GtkWidget *cancel = gtk_button_new_with_label("Cancel");

  g_signal_connect (GTK_WIDGET(ok), "clicked", G_CALLBACK (accept_comment), NULL);
  g_signal_connect (GTK_WIDGET(cancel), "clicked", G_CALLBACK (cancel_comment), NULL);

  gtk_box_pack_start (GTK_BOX (hbox), ok, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), cancel, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (win), vbox);

  annotation_window = win;
}

void unhighlight_all_moves()
{
  int i;
  for(i=0;i<nof_moves; i++)
  {
    highlight_ply(false, i, 1);
    highlight_ply(false, i, 2);
  }
}

void select_move(GtkTextView *view, GdkEventButton *event, gpointer data)
{
  gint x, y;
  GtkTextIter iter, match_start1, match_end1, match_start2, match_end2;

  gtk_text_view_window_to_buffer_coords (view, 
                                         GTK_TEXT_WINDOW_TEXT, 
                                         event->x, event->y,
                                         &x, &y);

  gtk_text_view_get_iter_at_location(view, &iter, x, y);

  char *str;

  enum bool contains_newline = false;

  enum bool white_move = false;

  int i, loc = 0;

  gboolean found = gtk_text_iter_backward_search(&iter,
                                                 " ",
                                                 GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                                 &match_start1,
                                                 &match_end1,
                                                 NULL);

  if(found == FALSE)
    return;

  str = gtk_text_buffer_get_text(gtk_text_view_get_buffer((GtkTextView *)view),
                                 &match_end1,
                                 &iter,
                                 FALSE);

  contains_newline = false;
  loc = 0;

  for(i=0; i<strlen(str); i++)
  {
    if(str[i] == '\n')
    {
      contains_newline = true;
      loc = i;
      break;
    }
  }

  if(contains_newline == true)
  {
    //printf("loc = %d; str = %s\n", loc, str);
    return;
  }

  found = gtk_text_iter_forward_search(&iter,
                                       " ",
                                       GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                       &match_start2,
                                       &match_end2,
                                       NULL);

  if(found == TRUE)
  {
    str = gtk_text_buffer_get_text(gtk_text_view_get_buffer((GtkTextView *)view),
                                   &match_end1,
                                   &match_start2,
                                   FALSE);

    contains_newline = false;
    loc = 0;

    for(i=0; i<strlen(str); i++)
    {
      if(str[i] == '\n')
      {
          contains_newline = true;
          loc = i;
          break;
      }
    }

    if(contains_newline == true)
      ;//printf("%s\n", substring(str, 0, loc));
    else
    {
      white_move = true;
      //printf("%s\n", str);
    }
  }
  else //this is the last move
  {
    gtk_text_iter_forward_search(&iter,
                                 "\n",
                                 GTK_TEXT_SEARCH_TEXT_ONLY | GTK_TEXT_SEARCH_VISIBLE_ONLY,
                                 &match_start2,
                                 &match_end2,
                                 NULL);

    str = gtk_text_buffer_get_text(gtk_text_view_get_buffer((GtkTextView *)view),
                                   &match_end1,
                                   &match_start2,
                                   FALSE);
    //printf("%s\n", str);
  }

  unhighlight_all_moves();

  move_no = gtk_text_iter_get_line(&iter);
  ply_no = (white_move == true) ? 1 : 2;

  fill_grid(grid, fens[2*move_no + ply_no]);
  highlight_ply(true, move_no, ply_no);
}

int main(int argc, char *argv[])
{
  GtkWidget *vbox;

  GtkWidget *vbox2, *vbox3, *hbox;

  gtk_init (&argc, &argv);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window),
                               520, 500);

  g_signal_connect (GTK_WIDGET(window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect(GTK_WIDGET(window), 
                   "key_press_event", 
                   G_CALLBACK (handle_key_press_events), 
                   NULL);

  //moves list

  GtkWidget *scrolled_win1 = gtk_scrolled_window_new(NULL, NULL);
  moves_text_view = gtk_text_view_new();
  gtk_text_view_set_editable((GtkTextView *)moves_text_view, FALSE);
  gtk_text_view_set_cursor_visible((GtkTextView *)moves_text_view, FALSE);

  g_signal_connect (moves_text_view, "button_press_event", G_CALLBACK (select_move), NULL);

  gtk_container_add (GTK_CONTAINER (scrolled_win1), moves_text_view);  

  //end of moves list

  //comment text area

  GtkWidget *scrolled_win2 = gtk_scrolled_window_new (NULL, NULL);
  comment_text_view = gtk_text_view_new();

  gtk_text_view_set_editable((GtkTextView *)comment_text_view, FALSE);
  gtk_text_view_set_cursor_visible((GtkTextView *)comment_text_view, FALSE);

  g_signal_connect (comment_text_view, "button_press_event", G_CALLBACK (annotate), NULL);

  gtk_text_buffer_create_tag(gtk_text_view_get_buffer((GtkTextView *)comment_text_view),
                             "red_fg",
                             "foreground",
                             "red",
                             NULL);

  gtk_container_add (GTK_CONTAINER (scrolled_win2), comment_text_view);

  gtk_text_buffer_create_tag(gtk_text_view_get_buffer((GtkTextView *)moves_text_view),
                             "cyan_bg",
                             "background",
                             "cyan",
                             NULL);

  //end of comment text area

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  gtk_box_pack_start (GTK_BOX (vbox), (GtkWidget *)create_toolbar(), FALSE, FALSE, 0);

  grid = gtk_grid_new();

  if(argc == 1)
  {
    char **fen_array = (char **)malloc(8 * sizeof(char *));
    int i;
    
    for(i=0;i<8;i++)
      fen_array[i] = (char *)malloc(8 * sizeof(char));

    convert_fen_string_to_fen_array("8/8/8/8/8/8/8/8 ", fen_array);

    fill_grid(grid, fen_array);

    for(i=0;i<8;i++)
      free(fen_array[i]);
    free(fen_array);

    memset(pgn_file_name, '\0', 100);
  }
  else
  {
    strcpy(pgn_file_name, argv[1]);
    load_from_file(pgn_file_name);
  }

  gtk_box_pack_start (GTK_BOX (hbox), grid, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win1, TRUE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win2, TRUE, TRUE, 0);
  
  gtk_container_add (GTK_CONTAINER (window), vbox);

  gtk_widget_show_all((GtkWidget *)window);

  create_annotation_window();

  gtk_main ();

  return(0);
}

void set_comment_view_text(char *txt)
{
  GtkTextBuffer *buf = gtk_text_view_get_buffer((GtkTextView *)comment_text_view);

  GtkTextIter start, end;

  gtk_text_buffer_set_text(buf,
                           txt, 
                           -1);

  gtk_text_buffer_get_start_iter(buf, &start);
  gtk_text_buffer_get_end_iter(buf, &end);

  gtk_text_buffer_apply_tag_by_name(buf,
				    "red_fg",
				    &start,
				    &end);
}
