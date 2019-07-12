#include "SDL.h"
#include <iostream>

using namespace std;

const int MaxAVars = 16;
const int MaxPVars = 16;

struct Vertex {
	float x; // The x component.
	float y; // The y component.
	float z; // The z component.
	float w; // The w component.

    float r;
    float g;
    float b;

	/// Affine variables.
	float avar[MaxAVars];

	/// Perspective variables.
	float pvar[MaxPVars];
};

struct EdgeEquation {
    float a;
    float b;
    float c;
    bool tie;
  
    EdgeEquation(const Vertex &v0, const Vertex &v1) {
        a = v0.y - v1.y;
        b = v1.x - v0.x;
        c = -(a * (v0.x + v1.x) + b * (v0.y + v1.y)) / 2;
        tie = a != 0 ? a > 0 : b > 0;
    }
  
    /// Evaluate the edge equation for the given point.
    float evaluate(float x, float y) {
        return a * x + b * y + c;
    }
  
    /// Test if the given point is inside the edge.
    bool test(float x, float y) {
        return test(evaluate(x, y));
    }
  
    /// Test for a given evaluated value.
    bool test(float v) {
        return (v > 0 || (v == 0 && tie));
    }
};

struct ParameterEquation {
    float a;
    float b;
    float c;
  
    ParameterEquation(
        float p0,
        float p1,
        float p2,
        const EdgeEquation &e0,
        const EdgeEquation &e1,
        const EdgeEquation &e2,
        float area) {
        float factor = 1.0f / (2.0f * area);
  
        a = factor * (p0 * e0.a + p1 * e1.a + p2 * e2.a);
        b = factor * (p0 * e0.b + p1 * e1.b + p2 * e2.b);
        c = factor * (p0 * e0.c + p1 * e1.c + p2 * e2.c);
    }
  
    /// Evaluate the parameter equation for the given point.
    float evaluate(float x, float y) {
        return a * x + b * y + c;
    }
};

struct BaryCoords {
    float r;
    float g;
    float b;
};

void putpixel(SDL_Surface*, int, int, Uint32);
void drawTriangle(const Vertex&, const Vertex&, const Vertex&, SDL_Surface*);
BaryCoords* getBarycentricCoordinates(const Vertex&, const Vertex&, const Vertex&, int, int);

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *window = SDL_CreateWindow(
      "SDL2Test",
      SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED,
      640,
      480,
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

    const Vertex v0 = {500, 50, 0, 0, random() % 255, random() % 255, random() % 255}; 
    const Vertex v1 = {250, 300, 0, 0, random() % 255, random() % 255, random() % 255}; 
    const Vertex v2 = {10, 10, 0, 0, random() % 255, random() % 255, random() % 255}; 

    drawTriangle(v0, v1, v2, surface);

    SDL_UpdateWindowSurface(window);

    SDL_Delay(6000);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void putpixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8*) surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
        case 1:
            *p = pixel;
            break;

        case 2:
            *(Uint16*) p = pixel;
            break;

        case 3:
            if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                p[0] = (pixel >> 16) & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = pixel & 0xff;
            } else {
                p[0] = pixel & 0xff;
                p[1] = (pixel >> 8) & 0xff;
                p[2] = (pixel >> 16) & 0xff;
            }
            break;

        case 4:
            *(Uint32*) p = pixel;
            break;
    }
}

void drawTriangle(const Vertex& v0, const Vertex& v1, const Vertex& v2, SDL_Surface* surface) {

    // Compute triangle bounding box.
    int minX = min(min(v0.x, v1.x), v2.x);
    int maxX = max(max(v0.x, v1.x), v2.x);
    int minY = min(min(v0.y, v1.y), v2.y);
    int maxY = max(max(v0.y, v1.y), v2.y);
  
    // Clip to scissor rect.
  //  minX = max(minX, m_minX);
  //  maxX = min(maxX, m_maxX);
  //  minY = max(minY, m_minY);
  //  maxY = min(maxY, m_maxY);
  
    // Compute edge equations.
    EdgeEquation e0(v0, v1);
    EdgeEquation e1(v1, v2);
    EdgeEquation e2(v2, v0);
  
    float area = 0.5 * (e0.c + e1.c + e2.c);
  
    ParameterEquation r(v0.r, v1.r, v2.r, e0, e1, e2, area);
    ParameterEquation g(v0.g, v1.g, v2.g, e0, e1, e2, area);
    ParameterEquation b(v0.b, v1.b, v2.b, e0, e1, e2, area);
  
    // Check if triangle is backfacing.
    if (area < 0) {
        return;
    }
  
    // Add 0.5 to sample at pixel centers.
    for (float x = minX + 0.5f, xm = maxX + 0.5f; x <= xm; x += 1.0f) {
        for (float y = minY + 0.5f, ym = maxY + 0.5f; y <= ym; y += 1.0f) {
            if (e0.test(x, y) && e1.test(x, y) && e2.test(x, y)) {
                //int rint = r.evaluate(x, y) * 255;
                //int gint = g.evaluate(x, y) * 255;
                //int bint = b.evaluate(x, y) * 255;
                auto bc = getBarycentricCoordinates(v0, v1, v2, x, y);
                Uint32 color = SDL_MapRGB(surface->format, bc->r, bc->g, bc->b);
                putpixel(surface, x, y, color);
            }
        }
    }
}

BaryCoords* getBarycentricCoordinates(const Vertex& v0, const Vertex& v1, const Vertex& v2, int px, int py) {
    float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y); 
    float wv0 = ((v1.y - v2.y) * (px - v2.x) + (v2.x - v1.x) * (py - v2.y)) / denom; 
    float wv1 = ((v2.y - v0.y) * (px - v2.x) + (v0.x - v2.x) * (py - v2.y)) / denom;
    float wv2 = 1 - wv0 - wv1;
    float r = wv0 * v0.r + wv1 * v1.r + wv2 * v2.r;
    float g = wv0 * v0.g + wv1 * v1.g + wv2 * v2.g;
    float b = wv0 * v0.b + wv1 * v1.b + wv2 * v2.b;
    BaryCoords* bc;
    bc->r = r;
    bc->g = g;
    bc->b = b;
    return bc;
}
