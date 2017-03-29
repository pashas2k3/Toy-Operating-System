/*
 * profont.h
 * Font Header File
 *
 * ARM-based K70F120M microcontroller board
 *   for educational purposes only
 * CSCI E-92 Spring 2014, Professor James L. Frankel, Harvard Extension School
 *
 * Written by James L. Frankel (frankel@seas.harvard.edu)
 */

#ifndef _PROFONT_H
#define _PROFONT_H

#define PROFONT_CHARS_IN_FONT 256
#define PROFONT_FONT_HEIGHT 12
#define PROFONT_FONT_WIDTH 6

extern uint8_t profont[PROFONT_CHARS_IN_FONT][PROFONT_FONT_HEIGHT][PROFONT_FONT_WIDTH];

#endif /* ifndef _PROFONT_H */
