#ifndef SHADERS_H_
#define SHADERS_H_

#include "SDL.h"
#include "Data.h"
#include <omp.h>
#include <iostream>

using namespace std;

class PixelShader {
    public:
        static const bool InterpolateZ = false;
        static const bool InterpolateW = false;
        static const int VarCount = 3;

        static SDL_Surface* surface;

        static void drawPixel(const PixelData &p) {
        int rint = (int)(p.r * 255);
        int gint = (int)(p.g * 255);
        int bint = (int)(p.b * 255);

        Uint32 color = rint << 16 | gint << 8 | bint;

        Uint32 *buffer = (Uint32*)((Uint8 *)surface->pixels + (int)p.y * surface->pitch + (int)p.x * 4);
        *buffer = color;
    }
};

#endif
