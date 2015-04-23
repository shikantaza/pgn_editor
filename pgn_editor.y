/**
  Copyright 2015 Rajesh Jayaprakash <rajesh.jayaprakash@gmail.com>

  This file is part of PGN Editor.

  pLisp is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  pLisp is distributed in the hope that it will be useful,
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

%type <moves>                    moves
%type <move>                     move
%type <move>                     half_move
%type <token_value>              ply

%%

pgn: event_tag site_tag date_tag round_tag white_tag black_tag result_tag move_text;

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

move_text: moves result
       |
       moves half_move result
       {
         nof_moves++;

         moves = (move_t *)realloc(moves, nof_moves * sizeof(move_t));

         moves[nof_moves - 1] = create_move($2.white_move, $2.white_comment, "", "");
       };

moves: /* empty */
       {
         nof_moves = 0;
       }
       | 
       moves move
       {
         nof_moves++;

         moves = (move_t *)realloc(moves, nof_moves * sizeof(move_t));

         moves[nof_moves - 1] = create_move($2.white_move, $2.white_comment, $2.black_move, $2.black_comment);

         $$ = moves;
       };

half_move: T_INTEGER T_PERIOD ply
       {
         in_move_text = true;
         $$ = create_move($3, "", "", "");
       } 
       |
       T_INTEGER T_PERIOD ply T_COMMENT
       {
         in_move_text = true;
         $$ = create_move($3, $4, "", "");
       };      

move: T_INTEGER T_PERIOD ply ply
       {
         in_move_text = true;
         $$ = create_move($3, "", $4, "");
       }
       |
       T_INTEGER T_PERIOD ply T_COMMENT ply
       {
         in_move_text = true;
         $$ = create_move($3, $4, $5, "");
       }
       |
       T_INTEGER T_PERIOD ply ply T_COMMENT
       {
         in_move_text = true;
         $$ = create_move($3, "", $4, $5);
       }
       |
       T_INTEGER T_PERIOD ply T_COMMENT ply T_COMMENT
       {
         in_move_text = true;
         $$ = create_move($3, $4, $5, $6);
       };

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
