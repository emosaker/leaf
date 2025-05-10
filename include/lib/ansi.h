/*
 * This file is part of the leaf programming language
 */

#ifndef LEAF_ANSI_H
#define LEAF_ANSI_H

#define ANSI(X) "\033[" #X "m"

/* control */
#define RESET      ANSI(0)
#define BOLD       ANSI(1)
#define FAINT      ANSI(2)
#define ITALIC     ANSI(3)
#define UNDERLINE  ANSI(4)
#define BLINK      ANSI(5)
#define FAST_BLINK ANSI(6)
#define REVERSE    ANSI(7)
#define CONCEAL    ANSI(8)
#define CROSSED    ANSI(9)

#define FRAMED    ANSI(51)
#define CIRCLED   ANSI(52)
#define OVERLINED ANSI(53)

/* colors */
#define FG_BLACK   ANSI(30)
#define FG_RED     ANSI(31)
#define FG_GREEN   ANSI(32)
#define FG_YELLOW  ANSI(33)
#define FG_BLUE    ANSI(34)
#define FG_MAGENTA ANSI(35)
#define FG_CYAN    ANSI(36)
#define FG_GRAY    ANSI(37)

#define FG_BRIGHT_BLACK   ANSI(90)
#define FG_BRIGHT_RED     ANSI(91)
#define FG_BRIGHT_GREEN   ANSI(92)
#define FG_BRIGHT_YELLOW  ANSI(93)
#define FG_BRIGHT_BLUE    ANSI(94)
#define FG_BRIGHT_MAGENTA ANSI(95)
#define FG_BRIGHT_CYAN    ANSI(96)
#define FG_BRIGHT_GRAY    ANSI(97)

#define BG_BLACK   ANSI(40)
#define BG_RED     ANSI(41)
#define BG_GREEN   ANSI(42)
#define BG_YELLOW  ANSI(43)
#define BG_BLUE    ANSI(44)
#define BG_MAGENTA ANSI(45)
#define BG_CYAN    ANSI(46)
#define BG_GRAY    ANSI(47)

#define BG_BRIGHT_BLACK   ANSI(100)
#define BG_BRIGHT_RED     ANSI(101)
#define BG_BRIGHT_GREEN   ANSI(102)
#define BG_BRIGHT_YELLOW  ANSI(103)
#define BG_BRIGHT_BLUE    ANSI(104)
#define BG_BRIGHT_MAGENTA ANSI(105)
#define BG_BRIGHT_CYAN    ANSI(106)
#define BG_BRIGHT_GRAY    ANSI(107)

#define FG_RGB(R, G, B) ANSI(38;2;R;G;B)
#define BG_RGB(R, G, B) ANSI(48;2;R;G;B)

#endif /* LEAF_ANSI_H */
