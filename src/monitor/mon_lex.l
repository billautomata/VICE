/* -*- C -*-
 *
 * mon_lex.l - Lexer for the VICE built-in monitor.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Andreas Boose <viceteam@t-online.de>
 *  Thomas Giesel <skoe@directbox.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

%{

/* Lexer for x64 monitor */

#include "vice.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "mon_command.h"
#include "montypes.h"
#include "mon_parse.h" /* FIXME ! */
#include "types.h"


#define YY_ALWAYS_INTERACTIVE 1
#define YY_NO_UNISTD_H

#undef min
#define min(a,b) ((a) < (b) ? (a) : (b))

int new_cmd = 1, quote = 0, opt_asm = 0, cur_len = 0, last_len = 0, dont_match_reg_a = 0;
int temp, push_back;
static int yywrap(void);
YY_BUFFER_STATE my_state;

#define YY_USER_ACTION { last_len = cur_len; cur_len += yyleng; }

%}

HDIGIT  [0-9a-fA-F]

%x FNAME CMD STR ROL ASM_MODE COND_MODE REG_ASGN LABEL_ASGN BNAME CTYPE

%%

%{
   if (new_cmd) {
      if (!(asm_mode && opt_asm))
         last_len = cur_len = 0;

      if (asm_mode) {
         BEGIN (ASM_MODE);
         opt_asm = 0;
      } else
         BEGIN (CMD);
      new_cmd = 0;
   }
%}

<CMD>{
        ~               { BEGIN(INITIAL);       return CONVERT_OP; }
        >               { BEGIN(INITIAL);       return CMD_ENTER_DATA; }
        @               { BEGIN(ROL);           return CMD_DISK; }
        a               { opt_asm = 1; BEGIN(INITIAL); return CMD_ASSEMBLE; }
        add_label|al    { BEGIN(INITIAL);       return CMD_ADD_LABEL; }
        attach          { BEGIN(FNAME);         return CMD_ATTACH; }
        bank            { BEGIN(BNAME);         return CMD_BANK; }
        bload|bl        { BEGIN(FNAME);         return CMD_BLOAD; }
        block_read|br   { BEGIN(INITIAL);       return CMD_BLOCK_READ; }
        break           { BEGIN(INITIAL);       return CMD_BREAK; }
        bsave|bs        { BEGIN(FNAME);         return CMD_BSAVE; }
        backtrace|bt    { BEGIN(INITIAL);       return CMD_BACKTRACE; }
        block_write|bw  { BEGIN(INITIAL);       return CMD_BLOCK_WRITE; }
        cartfreeze      { BEGIN(INITIAL);       return CMD_CARTFREEZE; }
        cd              { BEGIN(ROL);           return CMD_CHDIR; }
        command         { BEGIN(INITIAL);       return CMD_COMMAND; }
        compare|c       { BEGIN(INITIAL);       return CMD_COMPARE; }
        condition|cond  { BEGIN(INITIAL);       return CMD_CONDITION; }
        cpu             { BEGIN(CTYPE);         return CMD_CPU; }
        cpuhistory|chis { BEGIN(INITIAL);       return CMD_CPUHISTORY; }
        dir|ls          { BEGIN(ROL);           return CMD_DIR; }
        disass|d        { BEGIN(INITIAL);       return CMD_DISASSEMBLE; }
        delete|del      { BEGIN(INITIAL);       return CMD_DELETE; }
        delete_label|dl { BEGIN(INITIAL);       return CMD_DEL_LABEL; }
        device|dev      { BEGIN(INITIAL);       return CMD_DEVICE; }
        detach          { BEGIN(INITIAL);       return CMD_DETACH; }
        disable         { BEGIN(INITIAL);       return CMD_CHECKPT_OFF; }
        dump            { BEGIN(FNAME);         return CMD_DUMP; }
        enable          { BEGIN(INITIAL);       return CMD_CHECKPT_ON; }
        exit|x          { BEGIN(INITIAL);       return CMD_EXIT; }
        fill|f          { BEGIN(INITIAL);       return CMD_FILL; }
        goto|g          { BEGIN(INITIAL);       return CMD_GOTO; }
        help|"?"        { BEGIN(ROL);           return CMD_HELP; }
        hunt|h          { BEGIN(INITIAL);       return CMD_HUNT; }
        i               { BEGIN(INITIAL);       return CMD_TEXT_DISPLAY; }
        ii              { BEGIN(INITIAL);       return CMD_SCREENCODE_DISPLAY; }
        ignore          { BEGIN(INITIAL);       return CMD_IGNORE; }
        io              { BEGIN(INITIAL);       return CMD_IO; }
        keybuf          { BEGIN(ROL);           return CMD_KEYBUF; }
        load|l          { BEGIN(FNAME);         return CMD_LOAD; }
        load_labels|ll  { BEGIN(FNAME);         return CMD_LOAD_LABELS; }
        mem|m           { BEGIN(INITIAL);       return CMD_MEM_DISPLAY; }
        memchar|mc      { BEGIN(INITIAL);       return CMD_CHAR_DISPLAY; }
        memmapsave|mmsave { BEGIN(FNAME);       return CMD_MEMMAPSAVE; }
        memmapshow|mmsh { BEGIN(INITIAL);       return CMD_MEMMAPSHOW; }
        memmapzap|mmzap { BEGIN(INITIAL);       return CMD_MEMMAPZAP; }
        move|t          { BEGIN(INITIAL);       return CMD_MOVE; }
        memsprite|ms    { BEGIN(INITIAL);       return CMD_SPRITE_DISPLAY; }
        next|n          { BEGIN(INITIAL);       return CMD_NEXT; }
        playback|pb     { BEGIN(FNAME);         return CMD_PLAYBACK; }
        print|p         { BEGIN(INITIAL);       return CMD_PRINT; }
        pwd             { BEGIN(INITIAL);       return CMD_PWD; }
        quit            { BEGIN(INITIAL);       return CMD_QUIT; }
        radix|rad       { BEGIN(INITIAL);       return CMD_RADIX; }
        record|rec      { BEGIN(FNAME);         return CMD_RECORD; }
        registers|r     { BEGIN(REG_ASGN);      return CMD_REGISTERS; }
        reset           { BEGIN(INITIAL);       return CMD_RESET; }
        resourceget|resget { BEGIN(INITIAL);    return CMD_RESOURCE_GET; }
        resourceset|resset { BEGIN(INITIAL);    return CMD_RESOURCE_SET; }
        return|ret      { BEGIN(INITIAL);       return CMD_RETURN; }
        save|s          { BEGIN(FNAME);         return CMD_SAVE; }
        save_labels|sl  { BEGIN(FNAME);         return CMD_SAVE_LABELS; }
        screen|sc       { BEGIN(INITIAL);       return CMD_SCREEN; }
        screenshot|scrsh { BEGIN(FNAME);        return CMD_SCREENSHOT; }
        show_labels|shl { BEGIN(INITIAL);       return CMD_SHOW_LABELS; }
        sidefx|sfx      { BEGIN(INITIAL);       return CMD_SIDEFX; }
        step|z          { BEGIN(INITIAL);       return CMD_STEP; }
        stop            { BEGIN(INITIAL);       return CMD_STOP; }
        tapectrl        { BEGIN(INITIAL);       return CMD_TAPECTRL; }
        trace|tr        { BEGIN(INITIAL);       return CMD_TRACE; }
        until|un        { BEGIN(INITIAL);       return CMD_UNTIL; }
        undump          { BEGIN(FNAME);       return CMD_UNDUMP; }
        watch|w         { BEGIN(INITIAL);       return CMD_WATCH; }
        yydebug         { BEGIN(INITIAL);       return CMD_YYDEBUG; }
}
 /* this is not a real command, but could be a label name for a label assignment */
<CMD>[_a-zA-Z][_a-zA-Z0-9]* { BEGIN(LABEL_ASGN); yylval.str = lib_stralloc(yytext); return CMD_LABEL_ASGN; }

;		{ new_cmd = 1; return CMD_SEP; }

on  		{ yylval.i = e_ON; return TOGGLE; }
off 		{ yylval.i = e_OFF; return TOGGLE; }
toggle		{ yylval.i = e_TOGGLE; return TOGGLE; }

load 		{ yylval.i = e_load; return MEM_OP; }
store 		{ yylval.i = e_store; return MEM_OP; }

if		{ BEGIN (COND_MODE); return IF; }

\"		{ if (!quote) {
                     quote = 1;
                     BEGIN (STR);
                  }
                }
<STR>\" 	{ if (quote) {
                     quote = 0;
                     BEGIN (INITIAL);
                  }
                }

<STR>[^\"\n]*      { yylval.str = lib_stralloc(yytext); return STRING; }


\.\$		{ yylval.i = e_hexadecimal; return INPUT_SPEC; }
\.\+		{ yylval.i = e_decimal; return INPUT_SPEC; }
\.%		{ yylval.i = e_binary; return INPUT_SPEC; }
\.&		{ yylval.i = e_octal; return INPUT_SPEC; }
 /* \.TA		{ yylval.i = e_text_ascii; return INPUT_SPEC; } */
 /* \.TP		{ yylval.i = e_text_petscii; return INPUT_SPEC; } */
 /* \.\>		{ yylval.i = e_6502_asm; return INPUT_SPEC; } */
 /* \.S		{ yylval.i = e_sprite; return INPUT_SPEC; } */
 /* \.C		{ yylval.i = e_character; return INPUT_SPEC; } */

D|d		{ yylval.i = e_decimal; return RADIX_TYPE; }
H|h		{ yylval.i = e_hexadecimal; return RADIX_TYPE; }
B|b		{ yylval.i = e_binary; return RADIX_TYPE; }
O|o		{ yylval.i = e_octal; return RADIX_TYPE; }
 /* C		{ yylval.i = e_character; return RADIX_TYPE; } */
 /* S		{ yylval.i = e_sprite; return RADIX_TYPE; } */
 /* TA		{ yylval.i = e_text_ascii; return RADIX_TYPE; } */
 /* TP		{ yylval.i = e_text_petscii; return RADIX_TYPE; } */

<BNAME,INITIAL,COND_MODE>(c|C):          { return MEM_COMP; }
<BNAME,INITIAL,COND_MODE>8:              { return MEM_DISK8; }
<BNAME,INITIAL,COND_MODE>9:              { return MEM_DISK9; }
<BNAME,INITIAL,COND_MODE>10:             { return MEM_DISK10; }
<BNAME,INITIAL,COND_MODE>11:             { return MEM_DISK11; }

<CTYPE>[_a-zA-Z0-9]*      { yylval.str = lib_stralloc(yytext); return CPUTYPE; }

<*>[ \t]*
<*>\n          { dont_match_reg_a = 0;
                 new_cmd = 1;
                 opt_asm = 0;
                 quote = 0;
                 BEGIN(INITIAL);
                 return TRAIL;
               }

<FNAME>\".+\" { yytext[yyleng-1] = '\0';
                yylval.str = lib_stralloc(yytext+1);
                BEGIN (INITIAL); return FILENAME; }

<BNAME>[_a-zA-Z0-9]*      { yylval.str = lib_stralloc(yytext); return BANKNAME; }

<ROL>[^ \t\n][^\n]*          { yylval.str = lib_stralloc(yytext); return R_O_L; }

<REG_ASGN>{
	A|a 		{ yylval.i = e_A; return REGISTER; }
	X|x 		{ yylval.i = e_X; return REGISTER; }
	Y|y 		{ yylval.i = e_Y; return REGISTER; }
	[Pp][Cc]	{ yylval.i = e_PC; return REGISTER; }
	[Ss][Pp]	{ yylval.i = e_SP; return REGISTER; }
}

<COND_MODE>{
	== 		{ yylval.i = e_EQU; return COMPARE_OP; }
	!= 		{ yylval.i = e_NEQ; return COMPARE_OP; }
	\<=		{ yylval.i = e_LTE; return COMPARE_OP; }
	\<		{ yylval.i = e_LT;  return COMPARE_OP; }
	\>		{ yylval.i = e_GT;  return COMPARE_OP; }
	\>=		{ yylval.i = e_GTE; return COMPARE_OP; }
	&&		{ yylval.i = e_AND; return COMPARE_OP; }
	\|\|		{ yylval.i = e_OR;  return COMPARE_OP; }

	\.A 		{ yylval.i = e_A; return REGISTER; }
	\.X 		{ yylval.i = e_X; return REGISTER; }
	\.Y 		{ yylval.i = e_Y; return REGISTER; }
	\.PC 		{ yylval.i = e_PC; return REGISTER; }
	\.SP		{ yylval.i = e_SP; return REGISTER; }

	\(		{ return L_PAREN; }
	\)		{ return R_PAREN; }
}

<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\,[a-zA-Z] {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\,\([a-zA-Z][a-zA-Z]\) {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ \([a-zA-Z][a-zA-Z]\)\,[a-zA-Z] {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ \([a-zA-Z][a-zA-Z]\)\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z][a-zA-Z]\ [0-7]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z][a-zA-Z][a-zA-Z]\ [a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>[a-zA-Z][a-zA-Z]\ [a-zA-Z][a-zA-Z]\, {
    yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE>af|AF         { return REG_AF; }
<ASM_MODE>bc|BC         { return REG_BC; }
<ASM_MODE>de|DE         { return REG_DE; }
<ASM_MODE>hl|HL         { return REG_HL; }
<ASM_MODE>ix|IX         { return REG_IX; }
<ASM_MODE>ixh|IXH       { return REG_IXH; }
<ASM_MODE>ixl|IXL       { return REG_IXL; }
<ASM_MODE>iy|IY         { return REG_IY; }
<ASM_MODE>iyh|IYH       { return REG_IYH; }
<ASM_MODE>iyl|IYL       { return REG_IYL; }
<ASM_MODE>[a-zA-Z]{2,4}                           { yylval.str = lib_stralloc(yytext); return OPCODE; }
<ASM_MODE,INITIAL>\.[_a-zA-Z][_a-zA-Z0-9]*        { yylval.str = lib_stralloc(yytext); return LABEL; }
<ASM_MODE,INITIAL>\.[_@\?:a-zA-Z][_@\?:a-zA-Z0-9]*        { yylval.str = lib_stralloc(yytext); return LABEL; }

<ASM_MODE>(a|A)/[ \t]*[:\n]           { if (!dont_match_reg_a) return REG_A;
                                        yylval.i = 0x0a; return H_NUMBER; }
<ASM_MODE>b|B           { return REG_B; }
<ASM_MODE>c|C           { return REG_C; }
<ASM_MODE>d|D           { return REG_D; }
<ASM_MODE>e|E           { return REG_E; }
<ASM_MODE>h|H           { return REG_H; }
<ASM_MODE>l|L           { return REG_L; }
<ASM_MODE>x|X           { return REG_X; }
<ASM_MODE>y|Y           { return REG_Y; }

<ASM_MODE>\(            { dont_match_reg_a = 1; return L_PAREN; }
<ASM_MODE>\)            { dont_match_reg_a = 0; return R_PAREN; }
<ASM_MODE>#             { dont_match_reg_a = 1; return ARG_IMMEDIATE; }
<ASM_MODE>:             { dont_match_reg_a = 0; return INST_SEP; }

<ASM_MODE,INITIAL,COND_MODE,REG_ASGN,LABEL_ASGN>{
	{HDIGIT}{8}             { yylval.str = lib_stralloc(yytext); return H_RANGE_GUESS; }
	1[01]*                  { yylval.str = lib_stralloc(yytext); return B_NUMBER_GUESS; }
	%[01]+                  { yylval.i = strtol(yytext+1, NULL, 2); return B_NUMBER; }
	&[0-7]+                 { yylval.i = strtol(yytext+1, NULL, 8); return O_NUMBER; }
	[0-7]*                  { yylval.str = lib_stralloc(yytext); return O_NUMBER_GUESS; }
	\+[0-9]+                { yylval.i = strtol(yytext+1, NULL, 10); return D_NUMBER; }
	[0-9]+                  { yylval.str = lib_stralloc(yytext); return D_NUMBER_GUESS; }
        xx|XX                   { yylval.i = 0x00; return MASK; }
	\${HDIGIT}+             { yylval.i = strtol(yytext+1, NULL, 16); return H_NUMBER; }
	{HDIGIT}+               { yylval.i = strtol(yytext, NULL, 16); return H_NUMBER; }
}

<*>=	{ return EQUALS; }
<*>,	{ return COMMA; }

 /* After a label assignment there may be a ; comment to EOL */
<LABEL_ASGN>;.*[^\n]	{ return LABEL_ASGN_COMMENT; }

<*>[^ \t]		{ return yytext[0]; }

%%

void free_buffer(void)
{
   yy_delete_buffer(my_state);
}

void make_buffer(char *str)
{
    my_state = yy_scan_buffer(str, strlen(str) + 2);
    assert(my_state);
}

static int yywrap(void)
{
    return 1;
}

