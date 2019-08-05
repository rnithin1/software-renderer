#include "SDL.h"
#include <iostream>

using namespace std;

const int MaxAVars = 16;
const int MaxPVars = 16;
const int aVarCount = 16;
const int pVarCount = 16;

int blockSize = 1;

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
  
    void init (const Vertex &v0, const Vertex &v1) {
        a = v0.y - v1.y;
        b = v1.x - v0.x;
        c = -(a * (v0.x + v1.x) + b * (v0.y + v1.y)) / 2;
        tie = a != 0 ? a > 0 : b > 0;
    }
  
    // Evaluate the edge equation for the given point.
    float evaluate(float x, float y) const {
        return a * x + b * y + c;
    }
  
    // Test if the given point is inside the edge.
    bool test(float x, float y) const {
        return test(evaluate(x, y));
    }
  
    // Test for a given evaluated value.
    bool test(float v) const {
        return (v > 0 || (v == 0 && tie));
    }

    float stepX(float v) const {
        return v + a;
    }

    // Step the equation value v to the x direction.
    float stepX(float v, float stepSize) const {
        return v + a * stepSize;
    }

    // Step the equation value v to the y direction.
    float stepY(float v) const {
        return v + b;
    }

    // Step the equation value vto the y direction.
    float stepY(float v, float stepSize) const {
        return v + b * stepSize;
    }
};

struct ParameterEquation {
    float a;
    float b;
    float c;
  
    void init (
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
  
    // Evaluate the parameter equation for the given point.
    float evaluate(float x, float y) const {
        return a * x + b * y + c;
    }

    float stepX(float v) const {
        return v + a;
    }

    // Step the equation value v to the x direction.
    float stepX(float v, float stepSize) const {
        return v + a * stepSize;
    }

    // Step the equation value v to the y direction.
    float stepY(float v) const {
        return v + b;
    }

    // Step the equation value vto the y direction.
    float stepY(float v, float stepSize) const {
        return v + b * stepSize;
    }
};

struct BaryCoords {
    float r;
    float g;
    float b;
    float wv0;
    float wv1;
    float wv2;

    BaryCoords(
            const Vertex& v0, 
            const Vertex& v1,
            const Vertex& v2,
            float px,
            float py) {
        
        float denom = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y); 
        wv0 = ((v1.y - v2.y) * (px - v2.x) + (v2.x - v1.x) * (py - v2.y)) / denom; 
        wv1 = ((v2.y - v0.y) * (px - v2.x) + (v0.x - v2.x) * (py - v2.y)) / denom;
        wv2 = 1 - wv0 - wv1;
        r = wv0 * v0.r + wv1 * v1.r + wv2 * v2.r;
        g = wv0 * v0.g + wv1 * v1.g + wv2 * v2.g;
        b = wv0 * v0.b + wv1 * v1.b + wv2 * v2.b;
    }

};

struct TriangleEquations {
    float area;

    EdgeEquation e0;
    EdgeEquation e1;
    EdgeEquation e2;

    ParameterEquation r;
    ParameterEquation g;
    ParameterEquation b;

    ParameterEquation z;
	ParameterEquation invw;
	ParameterEquation avar[MaxAVars];
    ParameterEquation pvar[MaxPVars];

    TriangleEquations(
            const Vertex &v0, 
            const Vertex &v1, 
            const Vertex &v2) {

        e0.init(v0, v1);
        e1.init(v1, v2);
        e2.init(v2, v0);

        area = 0.5f * (e0.c + e1.c + e2.c);
        float factor = 1.0f / area;
        z.init(v0.z, v1.z, v2.z, e0, e1, e2, factor);

        float invw0 = 1.0f / v0.w;
		float invw1 = 1.0f / v1.w;
        float invw2 = 1.0f / v2.w;

        // Cull backfacing triangles.
        if (area < 0) {
            return;
        }

        r.init(v0.r, v1.r, v2.r, e0, e1, e2, area);
        g.init(v0.g, v1.g, v2.g, e0, e1, e2, area);
        b.init(v0.b, v1.b, v2.b, e0, e1, e2, area);

        invw.init(invw0, invw1, invw2, e0, e1, e2, factor);

		for (int i = 0; i < aVarCount; ++i) {
			avar[i].init(v0.avar[i], v1.avar[i], v2.avar[i], e0, e1, e2, factor);
        }

		for (int i = 0; i < pVarCount; ++i) {
pvar[i].init(v0.pvar[i] * invw0, v1.pvar[i] * invw1, v2.pvar[i] * invw2, e0, e1, e2, factor);
        }
    }
};

struct PixelData {
    float r;
    float g;
    float b;

    float x;
    float y;

    /// Initialize pixel data for the given pixel coordinates.
    void init(const TriangleEquations &eqn, float xi, float yi) {
        x = xi;
        y = yi;

        r = eqn.r.evaluate(x, y);
        g = eqn.g.evaluate(x, y);
        b = eqn.b.evaluate(x, y);
    }
    /// Step all the pixel data in the x direction.
    void stepX(const TriangleEquations &eqn) {
        r = eqn.r.stepX(r);
        g = eqn.g.stepX(g);
        b = eqn.b.stepX(b);
    }

    /// Step all the pixel data in the y direction.
    void stepY(const TriangleEquations &eqn) {
        r = eqn.r.stepY(r);
        g = eqn.g.stepY(g);
        b = eqn.b.stepY(b);
    }
};

struct EdgeData {
    float ev0;
    float ev1;
    float ev2;

    /// Initialize the edge data values.
    void init(const TriangleEquations &eqn, float x, float y) {
        ev0 = eqn.e0.evaluate(x, y);
        ev1 = eqn.e1.evaluate(x, y);
        ev2 = eqn.e2.evaluate(x, y);
    }

    /// Step the edge values in the x direction.
    void stepX(const TriangleEquations &eqn) {
        ev0 = eqn.e0.stepX(ev0);
        ev1 = eqn.e1.stepX(ev1);
        ev2 = eqn.e2.stepX(ev2);
    }

    /// Step the edge values in the x direction.
    void stepX(const TriangleEquations &eqn, float stepSize) {
        ev0 = eqn.e0.stepX(ev0, stepSize);
        ev1 = eqn.e1.stepX(ev1, stepSize);
        ev2 = eqn.e2.stepX(ev2, stepSize);
    }

    /// Step the edge values in the y direction.
    void stepY(const TriangleEquations &eqn) {
        ev0 = eqn.e0.stepY(ev0);
        ev1 = eqn.e1.stepY(ev1);
        ev2 = eqn.e2.stepY(ev2);
    }

    /// Step the edge values in the y direction.
    void stepY(const TriangleEquations &eqn, float stepSize) {
        ev0 = eqn.e0.stepY(ev0, stepSize);
        ev1 = eqn.e1.stepY(ev1, stepSize);
        ev2 = eqn.e2.stepY(ev2, stepSize);
    }

    /// Test for triangle containment.
    bool test(const TriangleEquations &eqn) {
        return eqn.e0.test(ev0) && eqn.e1.test(ev1) && eqn.e2.test(ev2);
    }
};

template <class Derived>
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

void drawPixel(PixelData);
void putpixel(SDL_Surface*, int, int, Uint32);
void drawTriangle(const Vertex&, const Vertex&, const Vertex&, SDL_Surface*);
template<bool>
void rasterizeBlock(const TriangleEquations&, float, float, SDL_Surface*);

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

    SDL_Delay(6000);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

void drawPixel(PixelData p, SDL_Surface *surface) {
    putpixel(surface, p.x, p.y, SDL_MapRGB(surface->format, p.r, p.g, p.b));
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

    TriangleEquations eqn(v0, v1, v2);

    // Check if triangle is back-facing.
    if (eqn.area < 0) {
        return;
    }

    // Compute triangle bounding box.
    int minX = min(min(v0.x, v1.x), v2.x);
    int maxX = max(max(v0.x, v1.x), v2.x);
    int minY = min(min(v0.y, v1.y), v2.y);
    int maxY = max(max(v0.y, v1.y), v2.y);
  
    minX = minX & ~(blockSize - 1);
    maxX = maxX & ~(blockSize - 1);
    minY = minY & ~(blockSize - 1);
    maxY = maxY & ~(blockSize - 1);


  // Add 0.5 to sample at pixel centers
  for (float x = minX + 0.5f, xm = maxX + 0.5f; x <= xm; x += blockSize) {
      for (float y = minY + 0.5f, ym = maxY + 0.5f; y <= ym; y += blockSize){
            float s = (float) blockSize - 1;

			float e0_00 = eqn.e0.evaluate(x, y);
			float e0_10 = eqn.e0.stepX(e0_00, s);
			float e0_01 = eqn.e0.stepY(e0_00, s);
			float e0_11 = eqn.e0.stepX(e0_01, s);

			float e1_00 = eqn.e1.evaluate(x, y);
			float e1_10 = eqn.e1.stepX(e1_00, s);
			float e1_01 = eqn.e1.stepY(e1_00, s);
			float e1_11 = eqn.e1.stepX(e1_01, s);

			float e2_00 = eqn.e2.evaluate(x, y);
			float e2_10 = eqn.e2.stepX(e2_00, s);
			float e2_01 = eqn.e2.stepY(e2_00, s);
			float e2_11 = eqn.e2.stepX(e2_01, s);

			int in0 = eqn.e0.test(e0_00) && eqn.e1.test(e1_00) && eqn.e2.test(e2_00);
			int in1 = eqn.e0.test(e0_10) && eqn.e1.test(e1_10) && eqn.e2.test(e2_10);
			int in2 = eqn.e0.test(e0_01) && eqn.e1.test(e1_01) && eqn.e2.test(e2_01);
			int in3 = eqn.e0.test(e0_11) && eqn.e1.test(e1_11) && eqn.e2.test(e2_11);
			int sum = in0 + in1 + in2 + in3;

			// All out.
			if (sum == 0)
				continue;

		    // Fully Covered
			if (sum == 4)
				rasterizeBlock<false>(eqn, x, y, surface);
		    
            // Partially Covered
			else
				rasterizeBlock<true>(eqn, x, y, surface);
        }
  }
}

template <bool TestEdges>
void rasterizeBlock(const TriangleEquations &eqn, float x, float y, SDL_Surface* surface)
	{
		PixelData po;
		po.init(eqn, x, y);

		EdgeData eo;
		if (TestEdges)
			eo.init(eqn, x, y);

		for (float yy = y; yy < y + blockSize; yy += 1.0f)
		{
			PixelData pi = po;

			EdgeData ei;
			if (TestEdges)
				ei = eo;

			for (float xx = x; xx < x + blockSize; xx += 1.0f)
			{
				if (!TestEdges || (eqn.e0.test(ei.ev0) && eqn.e1.test(ei.ev1) && eqn.e2.test(ei.ev2)))
				{
					int rint = (int)(pi.r * 255);
					int gint = (int)(pi.g * 255);
					int bint = (int)(pi.b * 255);
					Uint32 color = SDL_MapRGB(surface->format, rint, gint, bint);
					putpixel(surface, (int)xx, (int)yy, color);
				}

				pi.stepX(eqn);
				if (TestEdges)
					ei.stepX(eqn);
			}

			po.stepY(eqn);
			if (TestEdges)
				eo.stepY(eqn);
		}
}

