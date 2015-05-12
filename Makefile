#  Copyright 2015 Rajesh Jayaprakash <rajesh.jayaprakash@gmail.com>

#  This file is part of PGN Editor.

#  PGN Editor is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.

#  PGN Editor is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.

#  You should have received a copy of the GNU General Public License
#  along with PGN Editor.  If not, see <http://www.gnu.org/licenses/>.

OBJS	= pgn_editor_main.o pgn_creation.o pgn_position_setup.o bison.o lex.o

CC	= gcc
CFLAGS	= -g `pkg-config --cflags gtk+-3.0` -I/usr/local/include -L/usr/local/lib

all:	pgn_editor

pgn_editor:	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o pgn_editor `pkg-config --libs gtk+-3.0`

lex.o:	lex.yy.c
	$(CC) $(CFLAGS) -c lex.yy.c -o lex.o

lex.yy.c:	pgn_editor.lex 
		flex -o lex.yy.c pgn_editor.lex

bison.o:	pgn_editor.tab.c
		$(CC) $(CFLAGS) -c pgn_editor.tab.c -o bison.o

pgn_editor.tab.c:	pgn_editor.y
		bison -d -v pgn_editor.y -o pgn_editor.tab.c

pgn_editor_main.o:	pgn_editor_main.c
		$(CC) $(CFLAGS) -c pgn_editor_main.c -o pgn_editor_main.o

pgn_creation.o:	pgn_creation.c
		$(CC) $(CFLAGS) -c pgn_creation.c -o pgn_creation.o

pgn_position_setup.o:	pgn_position_setup.c
		$(CC) $(CFLAGS) -c pgn_position_setup.c -o pgn_position_setup.o

bison.o			: pgn_editor.tab.c pgn_editor.h
lex.o			: pgn_editor.tab.h pgn_editor.h
pgn_editor_main.o	: pgn_editor.h
pgn_creation.o		: pgn_editor.h
pgn_position_setup.o	: pgn_editor.h

clean:
	rm -f *.o *~ lex.yy.c pgn_editor.tab.c pgn_editor.tab.h pgn_editor.output pgn_editor *.stackdump core


