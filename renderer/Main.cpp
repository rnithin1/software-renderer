#include "SDL.h"
#include "Equations.h"
#include "Data.h"
#include "Shaders.h"
#include "Rasterizer.h"

#include <omp.h>
#include <chrono>
#include <iostream>

using namespace std;

//void drawPixel(PixelData);
//void putpixel(SDL_Surface*, int, int, Uint32);
//void drawTriangle(const Vertex&, const Vertex&, const Vertex&, SDL_Surface*);
//template<bool>
//void rasterizeBlock(const TriangleEquations&, float, float, SDL_Surface*);
//template<bool>
//void rasterizeBlock(const TriangleEquations&, float, float, SDL_Surface*, const Vertex&, const Vertex&, const Vertex&);

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
      "Render",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      640,
      639,
      0
    );

    SDL_Surface *surface = SDL_GetWindowSurface(window);
    SDL_PixelFormat *pixelFormat = surface->format;
    SDL_FillRect(surface, 0, 0);

//    for (int i = 0; i < 10000; i++) {
//        int x = random() % 640;
//        int y = random() % 480;
//        int r = random() % 255;
//        int g = random() % 255;
//        int b = random() % 255;
//
////        putpixel(surface, x, y, SDL_MapRGB(surface->format, r, g, b));
//    }

    const Vertex v0 = {500, 50, 0, 0, 0, 0, 200}; //random() % 255, random() % 255, random() % 255}; 
    const Vertex v1 = {250, 300, 0, 0, 0, 200, 0}; //random() % 255, random() % 255, random() % 255}; 
    const Vertex v2 = {10, 10, 0, 0, 200, 0, 0}; //random() % 255, random() % 255, random() % 255}; 

    const Vertex v3 = {500, 600, 0, 0, 0, 0, 200}; //random() % 255, random() % 255, random() % 255}; 
    const Vertex v4 = {250, 630, 0, 0, 0, 200, 0}; //random() % 255, random() % 255, random() % 255}; 
    const Vertex v5 = {10, 590, 0, 0, 200, 0, 0}; //random() % 255, random() % 255, random() % 255}; 

    drawTriangle(v0, v1, v2, surface);
    drawTriangle(v3, v4, v5, surface);

    SDL_UpdateWindowSurface(window);

    SDL_Delay(6000); //6000

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

