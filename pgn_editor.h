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

#define NOT_FOUND -1

enum side {white, black};
enum piece {king, queen, rook, bishop, knight, pawn};
enum bool {true, false};

typedef struct
{
  char white_move[10];
  char white_comment[200];
  char black_move[10];
  char black_comment[200];
} move_t;
