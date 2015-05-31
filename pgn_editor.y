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

%{

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "pgn_editor.h"

void yyerror (char *);
extern FILE *yyin;
extern char *yytext;
extern int yylineno;

extern char event[100];
extern char site[100];
extern char date[100];
extern char round1[100];
extern char white_player[100];
extern char black_player[100];
extern char result[100];

extern enum bool in_move_text;

extern unsigned int nof_moves;
extern move_t *moves;

extern move_t create_move(char *, char *, char *, char *);

extern char pos_setup_fen_str[1000];

extern enum bool pgn_file_contains_fen;
%}

%union {
  char *token_value;
  int integer_value;
  move_t move;
  move_t* moves;
}

%start	pgn

%token <token_value>             T_EVENT
%token <token_value>             T_SITE
%token <token_value>             T_DATE
%token <token_value>             T_ROUND
%token <token_value>             T_WHITE
%token <token_value>             T_BLACK
%token <token_value>             T_RESULT
%token <token_value>             T_FEN
%token <token_value>             T_SETUP
%token <integer_value>           T_INTEGER
%token                           T_LEFT_SQUARE_BRACKET 
%token                           T_RIGHT_SQUARE_BRACKET
%token <token_value>             T_STRING_LITERAL

%token <token_value>             T_KING_SIDE_CASTLE
%token <token_value>             T_QUEEN_SIDE_CASTLE
%token <token_value>             T_PAWN_MOVE
%token <token_value>             T_UNAMBIG_PIECE_MOVE
%token <token_value>             T_DISAMBIG_PIECE_MOVE
%token <token_value>             T_PAWN_CAPTURE
%token <token_value>             T_UNAMBIG_PIECE_CAPTURE
%token <token_value>             T_DISAMBIG_PIECE_CAPTURE 

%token                           T_WHITE_WIN
%token                           T_BLACK_WIN
%token                           T_DRAW
%token                           T_OTHER

%token <token_value>             T_COMMENT
%token                           T_PERIOD

%token                           T_ELLIPSIS_PLY

%type <moves>                    middle_moves
%type <token_value>              ply

%type <moves>                    moves
%type <move>                     special_first_move
%type <move>                     special_last_move
%type <move>                     first_move
%type <move>                     regular_move
%type <move>                     last_move

%%

pgn: event_tag site_tag date_tag round_tag white_tag black_tag result_tag moves result |
     event_tag site_tag date_tag round_tag white_tag black_tag result_tag fen_tags moves result;

event_tag:  T_LEFT_SQUARE_BRACKET T_EVENT  T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(event, '\0', 100);
         strcpy(event, $3);
       };

site_tag:   T_LEFT_SQUARE_BRACKET T_SITE   T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(site, '\0', 100);
         strcpy(site, $3);
       };

date_tag:   T_LEFT_SQUARE_BRACKET T_DATE   T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(date, '\0', 100);
         strcpy(date, $3);
       };

round_tag:  T_LEFT_SQUARE_BRACKET T_ROUND  T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(round1, '\0', 100);
         strcpy(round1, $3);
       };

white_tag:  T_LEFT_SQUARE_BRACKET T_WHITE  T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(white_player, '\0', 100);
         strcpy(white_player, $3);
       };

black_tag:  T_LEFT_SQUARE_BRACKET T_BLACK  T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(black_player, '\0', 100);
         strcpy(black_player, $3);
       };

result_tag: T_LEFT_SQUARE_BRACKET T_RESULT T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
         memset(result, '\0', 100);
         strcpy(result, $3);
       };

fen_tags: T_LEFT_SQUARE_BRACKET T_FEN T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
          T_LEFT_SQUARE_BRACKET T_SETUP T_STRING_LITERAL T_RIGHT_SQUARE_BRACKET
       {
	 pgn_file_contains_fen = true;
         memset(pos_setup_fen_str, '\0', 1000);
         strcpy(pos_setup_fen_str, $3);
       };

moves: first_move middle_moves last_move
       {
	 //if($1)
	 //{
	   in_move_text = true;

	   move_t *temp = moves;

	   moves = (move_t *)malloc((nof_moves + 1) * sizeof(move_t));

	   moves[0] = $1;

	   if(nof_moves > 0)
	   {
	     int i;
	     for(i=0; i<nof_moves; i++)
	       moves[i+1] = temp[i];

	     free(temp);
	   }

	   nof_moves++;
	   //}

	   //if($3)
	   //{
	   /* in_move_text = true; */

	   /* temp = moves; */

	   /* moves = (move_t *)malloc((nof_moves + 1) * sizeof(move_t)); */

	   /* if(nof_moves > 0) */
	   /* { */
	   /*   int i; */
	   /*   for(i=0; i<nof_moves; i++) */
	   /*     moves[i] = temp[i]; */

	   /*   free(temp); */
	   /* } */

	   /* moves[nof_moves] = $3; */

	   /* nof_moves++;	   */ 
	   //}
       }

first_move: /* empty */ | regular_move | special_first_move;

middle_moves:
       /* empty */
       {
	 in_move_text = true;
	 nof_moves = 0;
	 if(moves)
	   free(moves);
	 moves = NULL;
       }
       | middle_moves regular_move
       {
	 in_move_text = true;

         nof_moves++;

	 if(!moves)
	   moves = (move_t *)malloc(nof_moves * sizeof(move_t));
	 else
	   moves = (move_t *)realloc(moves, nof_moves * sizeof(move_t));

	 moves[nof_moves - 1] = $2;

         $$ = moves;
       };

last_move: /* empty */ | regular_move | special_last_move;

regular_move: T_INTEGER T_PERIOD ply ply
       {
         $$ = create_move($3, "", $4, "");
       }
       |
       T_INTEGER T_PERIOD ply T_COMMENT ply
       {
         $$ = create_move($3, $4, $5, "");
       }
       |
       T_INTEGER T_PERIOD ply ply T_COMMENT
       {
         $$ = create_move($3, "", $4, $5);
       }
       |
       T_INTEGER T_PERIOD ply T_COMMENT ply T_COMMENT
       {
         $$ = create_move($3, $4, $5, $6);
       }
       ;

special_first_move: T_INTEGER T_PERIOD T_ELLIPSIS_PLY ply
       {
         $$ = create_move("...", "", $4, "");
       }
       | 
       T_INTEGER T_PERIOD T_ELLIPSIS_PLY ply T_COMMENT
       {
         $$ = create_move("...", "", $4, $5);
       }
       ;

special_last_move: T_INTEGER T_PERIOD ply
       {
         $$ = create_move($3, "", "", "");
       }
       |
       T_INTEGER T_PERIOD ply T_COMMENT
       {
         $$ = create_move($3, $4, "", "");
       }
       ;

ply: T_KING_SIDE_CASTLE      | 
     T_QUEEN_SIDE_CASTLE     | 
     T_PAWN_MOVE             | 
     T_UNAMBIG_PIECE_MOVE    |
     T_DISAMBIG_PIECE_MOVE   |
     T_PAWN_CAPTURE          |
     T_UNAMBIG_PIECE_CAPTURE |
     T_DISAMBIG_PIECE_CAPTURE;

result: T_WHITE_WIN | T_BLACK_WIN | T_DRAW | T_OTHER;

%%

#ifdef TEST

int main(int argc, char **argv)
{
  yyin = fopen(argv[1], "r");
  assert(yyin);

  yyparse();

  fclose(yyin);

  return 0;
}
#endif

void yyerror (char *s)
{
  printf ("\n%d: %s at '%s'\n", yylineno, s, yytext);
}
