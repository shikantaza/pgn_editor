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
enum mode current_mode;
char pos_setup_selected_piece;

char **initial_pos_fen;

enum bool pos_setup_white_to_move;
enum bool pos_setup_white_o_o;
enum bool pos_setup_white_o_o_o;
enum bool pos_setup_black_o_o;
enum bool pos_setup_black_o_o_o;

char pos_setup_fen_str[1000];

enum bool position_setup_completed;
//end of global variables

//external variables
extern GtkWidget *window;
extern GtkWidget *position_setup_window;
extern void new_pgn_from_pos();

extern enum board_states state;
extern display_message(char *, GtkWidget *);
extern GtkWidget *grid;
//end of external variables

//forward declarations
enum bool is_valid_pos(char **);
//end of forward declarations

void white_to_move_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    pos_setup_white_to_move = true;
  else
    pos_setup_white_to_move = false;
}

void clear()
{
  int i, j;

  for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      initial_pos_fen[i][j] = 0;

  fill_grid(grid, initial_pos_fen);
}

void clear_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  clear();
}

void close_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  if(is_valid_pos(initial_pos_fen) == false)
  {
    display_message("Invalid position!", position_setup_window);
    return;
  }

  gtk_widget_hide(position_setup_window);
  current_mode = pgn_from_position;
  position_setup_completed = true;

  if(pos_setup_white_to_move == true)
    state = white_to_move;
  else
    state = black_to_move;

  memset(pos_setup_fen_str, '\0', 1000);

  int i, j;
  int len = 0;

  for(i=0;i<8;i++)
  {
    for(j=0;j<8;j++)
    {
      if(initial_pos_fen[i][j] != 0)
	len += sprintf(pos_setup_fen_str+len, "%c", initial_pos_fen[i][j]);
      else
      {
	int nof_empty_squares = 0;

	while(initial_pos_fen[i][j] == 0 && j < 8)
	{
	  nof_empty_squares++;
	  j++;
	}
	len += sprintf(pos_setup_fen_str+len, "%d", nof_empty_squares);
	if(j<=7)
	  len += sprintf(pos_setup_fen_str+len, "%c", initial_pos_fen[i][j]);
      }
    }

    if(i != 7)
      len += sprintf(pos_setup_fen_str+len, "/");
    else
      len += sprintf(pos_setup_fen_str+len, " ");
  }

  if(pos_setup_white_to_move == true)
    len+= sprintf(pos_setup_fen_str+len, "w ");
  else
    len+= sprintf(pos_setup_fen_str+len, "b ");

  if(pos_setup_white_o_o == true)
    len+= sprintf(pos_setup_fen_str+len, "K");

  if(pos_setup_white_o_o_o == true)
    len+= sprintf(pos_setup_fen_str+len, "Q");

  if(pos_setup_black_o_o == true)
    len+= sprintf(pos_setup_fen_str+len, "k");

  if(pos_setup_black_o_o_o == true)
    len+= sprintf(pos_setup_fen_str+len, "q");

  if(pos_setup_white_o_o   == false &&
     pos_setup_white_o_o_o == false &&
     pos_setup_black_o_o   == false &&
     pos_setup_black_o_o_o == false)
    len+= sprintf(pos_setup_fen_str+len, "- ");
  else
    len+= sprintf(pos_setup_fen_str+len, " ");

  //TODO: en passant target square, halfmove clock
  //and fullmove number
  len+= sprintf(pos_setup_fen_str+len, "- 0 1");

  new_pgn_from_pos();
}

void position_setup_clicked(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  int d = (int)data;

  switch(d)
  {
    case 0:  pos_setup_selected_piece = 'K'; break;
    case 1:  pos_setup_selected_piece = 'Q'; break;
    case 2:  pos_setup_selected_piece = 'R'; break;
    case 3:  pos_setup_selected_piece = 'B'; break;
    case 4:  pos_setup_selected_piece = 'N'; break;
    case 5:  pos_setup_selected_piece = 'P'; break;
    case 6:  pos_setup_selected_piece = 'k'; break;
    case 7:  pos_setup_selected_piece = 'q'; break;
    case 8:  pos_setup_selected_piece = 'r'; break;
    case 9:  pos_setup_selected_piece = 'b'; break;
    case 10: pos_setup_selected_piece = 'n'; break;
    case 11: pos_setup_selected_piece = 'p'; break;
  }
}

void create_position_setup_window()
{
  GtkWidget *win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(win), 300, 80);

  //gtk_window_set_modal(GTK_WINDOW(win), TRUE);
  gtk_window_set_resizable(GTK_WINDOW(win), FALSE);

  gtk_window_set_transient_for((GtkWindow *)win, (GtkWindow *)window);

  gtk_window_move((GtkWindow *)win, 830, 340);

  g_signal_connect (GTK_WIDGET(win), "delete_event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);  

  GtkWidget *pieces_grid = gtk_grid_new();

  GtkWidget *boxes[2][6];
  GtkWidget *images[2][6];

  int i,j;

  images[0][0] = gtk_image_new_from_file("icons/w_k_w.png");
  images[0][1] = gtk_image_new_from_file("icons/w_q_w.png");
  images[0][2] = gtk_image_new_from_file("icons/w_r_w.png");
  images[0][3] = gtk_image_new_from_file("icons/w_b_w.png");
  images[0][4] = gtk_image_new_from_file("icons/w_n_w.png");
  images[0][5] = gtk_image_new_from_file("icons/w_p_w.png");

  images[1][0] = gtk_image_new_from_file("icons/b_k_w.png");
  images[1][1] = gtk_image_new_from_file("icons/b_q_w.png");
  images[1][2] = gtk_image_new_from_file("icons/b_r_w.png");
  images[1][3] = gtk_image_new_from_file("icons/b_b_w.png");
  images[1][4] = gtk_image_new_from_file("icons/b_n_w.png");
  images[1][5] = gtk_image_new_from_file("icons/b_p_w.png");


  for(i=0; i<2; i++)
  {
    for(j=0;j<6;j++)
    {
      boxes[i][j] = gtk_event_box_new();
      gtk_container_add(GTK_CONTAINER(boxes[i][j]), images[i][j]);

      g_signal_connect(boxes[i][j], "button_press_event", G_CALLBACK(position_setup_clicked), (gpointer)(i*6+j)); 

      gtk_grid_attach (GTK_GRID (pieces_grid), boxes[i][j], j, i, 1, 1);
    }
  }

  GtkWidget *clear = gtk_button_new_with_label("Clear");
  GtkWidget *close = gtk_button_new_with_label("Close");

  g_signal_connect (GTK_WIDGET(close), "clicked", G_CALLBACK (close_clicked), NULL);
  g_signal_connect (GTK_WIDGET(clear), "clicked", G_CALLBACK (clear_clicked), NULL);

  GtkWidget *hbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  gtk_box_pack_start (GTK_BOX (hbox1), clear, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox1), close, TRUE, TRUE, 0);

  GtkWidget *vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox1), pieces_grid, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, FALSE, 0);

  GtkWidget *vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

  GtkWidget *white_to_move_check = gtk_check_button_new_with_label("White to move");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(white_to_move_check), TRUE);
  g_signal_connect (GTK_WIDGET(white_to_move_check), "clicked", G_CALLBACK (white_to_move_clicked), NULL);
  //GTK_WIDGET_UNSET_FLAGS(black_to_move_check, GTK_CAN_FOCUS);

  GtkWidget *white_O_O_check = gtk_check_button_new_with_label("White O-O");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(white_O_O_check), TRUE);
  //GTK_WIDGET_UNSET_FLAGS(white_O_O_check, GTK_CAN_FOCUS);

  GtkWidget *white_O_O_O_check = gtk_check_button_new_with_label("White O-O-O");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(white_O_O_O_check), TRUE);
  //GTK_WIDGET_UNSET_FLAGS(white_O_O_O_check, GTK_CAN_FOCUS);

  GtkWidget *black_O_O_check = gtk_check_button_new_with_label("Black O-O");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(black_O_O_check), TRUE);
  //GTK_WIDGET_UNSET_FLAGS(black_O_O_check, GTK_CAN_FOCUS);

  GtkWidget *black_O_O_O_check = gtk_check_button_new_with_label("Black O-O-O");
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(black_O_O_O_check), TRUE);
  //GTK_WIDGET_UNSET_FLAGS(black_O_O_O_check, GTK_CAN_FOCUS);

  gtk_box_pack_start (GTK_BOX (vbox2), white_to_move_check, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), white_O_O_check, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), white_O_O_O_check, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), black_O_O_check, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), black_O_O_O_check, TRUE, TRUE, 0);

  GtkWidget *hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

  gtk_box_pack_start (GTK_BOX (hbox2), vbox1, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox2, FALSE, FALSE, 0);

  GtkWidget *vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_box_pack_start (GTK_BOX (vbox3), hbox2, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (win), vbox3);

  position_setup_window = win;

  initial_pos_fen = (char **)malloc(8 * sizeof(char *));

  for(i=0;i<8;i++)
    initial_pos_fen[i] = (char *)malloc(8 * sizeof(char));

  for(i=0;i<8;i++)
    for(j=0;j<8;j++)
      initial_pos_fen[i][j] = 0;

  pos_setup_white_to_move = true;
}

enum bool is_valid_pos(char **fen)
{
  int i,j;

  enum bool white_king_found = false, black_king_found = false;

  for(i=0;i<8;i++)
  {
    if(white_king_found == true && black_king_found == true)
      return true;

    for(j=0;j<8;j++)
    {
      if(fen[i][j] == 'K')
	white_king_found = true;
      if(fen[i][j] == 'k')
	black_king_found = true;
    }
  }

  if(white_king_found == false || black_king_found == false)
    return false;

  return true;
}
