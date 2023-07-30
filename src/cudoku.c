#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include "clargs.h"

Flag flist[] = {
	{'f', "file", true},
	{'s', "steps", false},
	{'v', "verbose", false}
};

typedef struct sudoku_space {
	unsigned int nots: 9;
	unsigned int row: 4;
	unsigned int col: 4;
	unsigned int sqr: 4;
	unsigned int solution: 4;
	bool checked;
} Space;
Space spaces[9][9];

bool usefile;
char *filename;
bool usesteps;
bool usedebug;

int cx = 0;
int cy = 0;

int solved = 0;

void act();

void info(char type) {
	// TODO It would be better to do this with a window (?)
	char *infostr;
	bool printbits;
	switch (type) {
		case '0':
			infostr = "Input";
			break;
		case '1':
			infostr = "Check 1";
			break;
		case '2':
			infostr = "Check 2";
			break;
		case 's':
			infostr = "Saved!";
			break;
		case 'm':
			infostr = "Twin Win!";
			break;
		case 0:
			infostr = "";		
			break;
		case 'S':
			infostr = "";
			break;
	}
	char finfostr[10];
	if (!type && usedebug) {
		sprintf(finfostr, "%.9b", spaces[cx][cy].nots);
	} else if (type == 'S' && usedebug) {
		sprintf(finfostr, "%-9d", solved);
		act();
	} else {
		sprintf(finfostr, "%-9s", infostr);
	}
	mvaddstr(10, 0, finfostr);
	move(cy, cx);
	refresh();
}

void mywrite() {
	FILE *fp;
	fp = fopen(filename, "w");
	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 9; x++) {
			fputc(48 + spaces[x][y].solution, fp);
		}
		fputc('\n', fp);
	}
	info('s');
}

void htravel(bool forward) {
	int xcrement, limit, jump;
	if (forward) {
		xcrement = +1;
		limit = 8;
		jump = 0;
	} else {
		xcrement = -1;
		limit = 0;
		jump = 8;
	}
	if (cx == limit) {
		if (cy != limit) {
			cx = jump;
			cy+=xcrement;
		}
	} else {
		cx+=xcrement;
	}
	move(cy, cx);
}

void vtravel(bool upward) {
	int xcrement, limit;
	if (upward) {
		xcrement = -1;
		limit = 0;
	} else {
		xcrement = +1;
		limit = 8;
	}
	if (cy != limit) {
		cy+=xcrement;
	}
	move(cy, cx);
}

void setcspace(int solution) {
	if(!spaces[cx][cy].solution && solution) {
		solved++;
	} else if(spaces[cx][cy].solution && !solution) {
		solved--;
	}
	spaces[cx][cy].solution = solution;
	spaces[cx][cy].nots = 0;
	spaces[cx][cy].checked = false;
}

void act() {
	attrset(COLOR_PAIR(1));
	move(0, 0);
	cx = 0; cy = 0;
	int a;
	while((a = getch()) != '\n') {
		switch (a) {
			case 27:
			case 'q':
				endwin();
				exit(1);
				break;
			case 'S':
				if (usefile) mywrite();
				break;
			case 'X':
				if (usefile) mywrite();
				endwin();
				exit(1);
				break;
			case KEY_LEFT:
			case 'h':
				htravel(false);
				info(0);
				break;
			case KEY_RIGHT:
			case 'l':
				htravel(true);
				info(0);
				break;
			case KEY_DOWN:
			case 'j':
				vtravel(false);
				info(0);
				break;
			case KEY_UP:
			case 'k':
				vtravel(true);
				info(0);
				break;
			case KEY_BACKSPACE:
				htravel(false);
				addch(' ');
				move(cy, cx);
				setcspace(0);
				info(0);
				break;
			case '0':
			case ' ':
				addch(' ');
				setcspace(0);
				htravel(true);
				info(0);
				break;
			case 49 ... 57:
				addch(a);
				setcspace(a - 48);
				htravel(true);
				info(0);
				break;
			case KEY_MOUSE:
				MEVENT m;
				getmouse (&m);
				if (m.x < 9 && m.y < 9) {
					cx = m.x;
					cy = m.y;
					move(cy, cx);
				}
				info(0);
				break;
			case 9:
				do htravel(true);
				while (cx % 3);
				info(0);
				break;
			case KEY_BTAB:
				do htravel(false);
				while (cx % 3);
				info(0);
				break;
			default:
				break;
		}
	}
}

void cascade(Space *mainspace);

// TODO jank method (?)
bool solvecheck(Space *checkspace) {
	int possibles = checkspace->nots^0777;
	for (int i = 0; i < 9; i++) {
		int check = 0001 << i;
		if (possibles == check) {
			checkspace->solution = i + 1;
			solved++;
			info('S');
			attrset(COLOR_PAIR(3));
			mvaddch(checkspace->row, checkspace->col, 49 + i);
			cascade(checkspace);
			return true;
		} else if (possibles % check) {
			return false;
		}
	}
}

bool solvecheck2(Space *checkspace) {
	int possibles = checkspace->nots^0777;
	for (int i = 0; i < 9; i++) {
		int check = 0001 << i;
		if (possibles == check) {
			checkspace->solution = i + 1;
			solved++;
			info('S');
			attrset(COLOR_PAIR(3));
			mvaddch(checkspace->row, checkspace->col, 49 + i);
			// cascade(checkspace);
			return true;
		} else if (possibles % check) {
			return false;
		}
	}
}


void check1(int solution, Space *checkspace) {
	if (checkspace->solution) return;
	checkspace->nots |= 0001 << (solution - 1);
	if (!solvecheck(checkspace) && usesteps) {
		attrset(COLOR_PAIR(2));
		mvaddch(checkspace->row, checkspace->col, '#');
	}
}

void cascade(Space *mainspace) {
	chtype oldch = 0;
	if (usesteps) {
		oldch = mvinch(mainspace->row, mainspace->col);
		mvaddch(mainspace->row, mainspace->col, oldch | A_REVERSE);
	}

	mainspace->checked = true;
	
	// TODO turn this entire 'for' block into it's own function
	int col_y = mainspace->row;
	int row_x = mainspace->col;
	int sqr_x = mainspace->sqr % 3 * 3;
	int sqr_y = mainspace->sqr / 3 * 3;
	for (int i = 0; i < 9; i++) {
		check1(mainspace->solution, &spaces[i][col_y]);
		check1(mainspace->solution, &spaces[row_x][i]);
		check1(mainspace->solution, 
			&spaces[sqr_x + i % 3][sqr_y + i / 3]);
	}
	if (usesteps) {
		act();
		mvaddch(mainspace->row, mainspace->col, oldch);
		// TODO better fix
		for (int i = 0; i < 9; i++) {
			attrset(COLOR_PAIR(2));
			if (!spaces[i][col_y].solution) {
				mvaddch(col_y, i, '.');
			}
			if (!spaces[row_x][i].solution) {
				mvaddch(i, row_x, '.');
			}
			if (!spaces[sqr_x + i % 3][sqr_y + i / 3].solution) {
				mvaddch(sqr_y + i / 3, sqr_x + i % 3, '.');
			}
		}
	}
}

void check2(Space *mainspace, Space *checkspace, int *twinc, Space *twins[9]) {
	if (checkspace->solution) return;
	if (!(mainspace->nots^checkspace->nots & mainspace->nots)) {
		twins[(*twinc)++] = checkspace;
		// optimize
	}
}

void collapse(Space *mainspace, Space *checkspace) {
	if (checkspace->solution) return;
	if (mainspace->nots^checkspace->nots & mainspace->nots) {
		checkspace->nots |= mainspace->nots^0777;
		solvecheck2(checkspace);
	}
}

void cascade2(Space *mainspace) {
	chtype oldch = 0;
	if (usesteps) {
		oldch = mvinch(mainspace->row, mainspace->col);
		mvaddch(mainspace->row, mainspace->col, oldch | A_REVERSE);
	}

	int possiblec = 0;
	int possibles = mainspace->nots^0777;
	for (int i = 0; i < 9; i ++) if (possibles & (0001 << i)) possiblec++;

	// TODO WARNING: AWFUL CODE INCOMING (i was sleepy)
	int row_x = mainspace->col;
	Space *twins_r[9];
	int twinc_r = 0;
	for (int i = 0; i < 9; i++)
		check2(mainspace, &spaces[row_x][i], &twinc_r, twins_r);
	if (twinc_r == possiblec) 
		for (int i = 0; i < 9; i++)
			collapse(mainspace, &spaces[row_x][i]);

	int col_y = mainspace->row;
	Space *twins_c[9];
	int twinc_c = 0;
	for (int i = 0; i < 9; i++)
		check2(mainspace, &spaces[i][col_y], &twinc_c, twins_c);
	if (twinc_c == possiblec) 
		for (int i = 0; i < 9; i++)
			collapse(mainspace, &spaces[i][col_y]);

	int sqr_x = mainspace->sqr % 3 * 3;
	int sqr_y = mainspace->sqr / 3 * 3;
	Space *twins_s[9];
	int twinc_s = 0;
	for (int i = 0; i < 9; i++)
		check2(mainspace, &spaces[sqr_x + i % 3][sqr_y + i / 3], 
			&twinc_s, twins_s);
	if (twinc_s == possiblec) 
		for (int i = 0; i < 9; i++)
			collapse(mainspace, 
				&spaces[sqr_x + i % 3][sqr_y + i / 3]);

	if (usesteps) {
		act();
		mvaddch(mainspace->row, mainspace->col, oldch);
	}
}

int main(int argc, char *argv[]) {
	SArgV *sargv;
	sargv = mksargs(argc, argv, flist, sizeof(flist)/sizeof(Flag));

	for (size_t i = 0; i < sargv->length; i++) {
		Arg *arg;
		arg = &(sargv->alist[i]);
		switch (arg->flag) {
			case 'f':
				usefile = true;
				filename = arg->str; break;
			case 's':
				usesteps = true;
				break;
			case 'v':
				usedebug = true;
				break;
			default:
				printf("Unexpected argument: %s\n", arg->str);
				return 1;
				break;
		}
	}
	
	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			spaces[x][y].nots = 0;
			spaces[x][y].row = y;
			spaces[x][y].col = x;
			spaces[x][y].sqr = x/3 + y/3 *3;
			spaces[x][y].solution = 0;
			spaces[x][y].checked = false;
		}
	}

	if (usefile) { 
		FILE *fp;
		if ((fp = fopen(filename, "r")) == NULL) {
			printf("%s: no such file\n", filename);
			return 1;
		}
		int fx = 0, fy = 0;
		int fc;
		while ((fc = fgetc(fp)) != EOF) {
			if (fc == '\n') {
				fy++;
				fx = 0;
			} else if (fx >= 9 || fy >= 9) {
				printf("%s L%dC%d: out of bounds\n", filename, 
					fy, fx);
				return 1;
			} else if (fc > 48 && fc < 58) {
				spaces[fx++][fy].solution = fc - 48;
			} else if (fc == '0' || fc == ' ') {
				spaces[fx++][fy].solution = 0;
			} else {
				printf("%s L%dC%d: unknown symbol: %c\n",
					filename, fy, fx, fc);
				return 1;
			}
		}
		fclose(fp);
	};

	initscr();
	keypad(stdscr, TRUE);
	noecho();
	mousemask(BUTTON1_PRESSED | BUTTON3_PRESSED | REPORT_MOUSE_POSITION,
		NULL);
	if (has_colors()) {
		start_color();
		init_pair(1, COLOR_CYAN, COLOR_BLACK); // start solution
		init_pair(2, COLOR_RED, COLOR_BLACK); // non-final check
		init_pair(3, COLOR_GREEN, COLOR_BLACK); // computer solved
		init_pair(4, COLOR_YELLOW, COLOR_BLACK); // computer solved
	}

	attrset(COLOR_PAIR(1));
	if (usefile) {
		for (int y = 0; y < 9; y++) {
			move(y, 0);
			for (int x = 0; x < 9; x++) {
				if (spaces[x][y].solution) {
					addch(48 + spaces[x][y].solution);
					solved++;
				} else {
					addch(' ');
				}
			}
		}
		move(0, 0);
		refresh();
	}
	
	while (solved < 81) {
		info('0');
		act();
		info('1');
		for (int x = 0; x < 9; x++) {
			for (int y = 0; y < 9; y++) {
				if (!spaces[x][y].checked && 
					spaces[x][y].solution)
					cascade(&spaces[x][y]);
			}
		}
		info('2');
		for (int x = 0; x < 9; x++) {
			for (int y = 0; y < 9; y++) {
				if (!spaces[x][y].solution) 
					cascade2(&spaces[x][y]);
			}
		}
	}

	int c;
	while ((c = getch()) != '\n' && c != 27 && c != 'q');

	endwin();

	return 0;
}
