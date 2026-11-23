#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

typedef struct {
    float x;
    float y;
} Vector2D;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} Rect2D;

#endif // COMMON_H
