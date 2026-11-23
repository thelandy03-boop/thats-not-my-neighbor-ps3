#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

// Full HD 1080p native resolution (matching original Godot viewport)
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080

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
