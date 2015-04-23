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

%option noyywrap

%{
#include <string.h>

#include "pgn_editor.h"
#include "pgn_editor.tab.h"

enum bool in_move_text = false;

char* substring(const char* str, size_t begin, size_t len) 
{ 
  if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) 
    return 0; 

  return strndup(str + begin, len); 
}

%}

%%

\"(\\.|[^\\"])*\"                     { yylval.token_value = substring(yytext, 1, strlen(yytext)-2); BEGIN(INITIAL); return T_STRING_LITERAL; }

\[                                    { return T_LEFT_SQUARE_BRACKET; }
\]                                    { return T_RIGHT_SQUARE_BRACKET; }

[ \t]+                                /* ignore whitespace */
\n                                    /* ignore newlines */

\{([^\}\n]|\n)*\}                     { 
                                        if(in_move_text == true) {
                                          yylval.token_value = substring(yytext, 1, strlen(yytext)-2);
                                          return T_COMMENT;
                                        }
                                      }

^#(.)*                                { 
                                        if(in_move_text == true) {
                                          yylval.token_value = substring(yytext, 1, strlen(yytext)-2);
                                          return T_COMMENT;
                                        }
                                      }

[1-9][0-9]*                           { yylval.integer_value = atoi(yytext); return T_INTEGER; }

Event                                 { return T_EVENT;  }
Site                                  { return T_SITE;   }
Date                                  { return T_DATE;   }
Round                                 { return T_ROUND;  }
White                                 { return T_WHITE;  }
Black                                 { return T_BLACK;  }
Result                                { return T_RESULT; }

\.                                    { return T_PERIOD; }

O-O[+]{0,1}                           { yylval.token_value = strdup(yytext); return T_KING_SIDE_CASTLE; }
O-O-O[+]{0,1}                         { yylval.token_value = strdup(yytext); return T_QUEEN_SIDE_CASTLE; }
[a-h][1-8](=[QRBN])?(\+)?             { yylval.token_value = strdup(yytext); return T_PAWN_MOVE; }
[K|Q|R|B|N][a-h][1-8](\+)?            { yylval.token_value = strdup(yytext); return T_UNAMBIG_PIECE_MOVE; }
[R|B|N][a-h|1-8][a-h][1-8](\+)?       { yylval.token_value = strdup(yytext); return T_DISAMBIG_PIECE_MOVE; }
[a-h]x[a-h][1-8](\+)?                 { yylval.token_value = strdup(yytext); return T_PAWN_CAPTURE; }
[K|Q|R|B|N]x[a-h][1-8](\+)?           { yylval.token_value = strdup(yytext); return T_UNAMBIG_PIECE_CAPTURE; } 
[R|B|N][a-h|1-8]x[a-h][1-8](\+)?      { yylval.token_value = strdup(yytext); return T_DISAMBIG_PIECE_CAPTURE; } 

1-0                                   { return T_WHITE_WIN; }
0-1                                   { return T_BLACK_WIN; }
1\/2-1\/2                             { return T_DRAW; }
\*                                    { return T_OTHER; }

%%
