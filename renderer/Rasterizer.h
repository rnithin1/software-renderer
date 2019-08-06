#ifndef RASTERIZER_H_
#define RASTERIZER_H_
#include "SDL.h"
#include "Equations.h"
#include "Data.h"
#include "Shaders.h"
#include <iostream>

void drawPixel(PixelData);
void putpixel(SDL_Surface*, int, int, Uint32);
void drawTriangle(const Vertex&, const Vertex&, const Vertex&, SDL_Surface*);
template<bool>
void rasterizeBlock(const TriangleEquations&, float, float, SDL_Surface*);
template<bool>
void rasterizeBlock(const TriangleEquations&, float, float, SDL_Surface*, const Vertex&, const Vertex&, const Vertex&);

int blockSize = 4;

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
    auto start = chrono::steady_clock::now();

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

    int stepsX = (maxX - minX) / blockSize + 1;
    int stepsY = (maxY - minY) / blockSize + 1;
    float s = (float) blockSize - 1;

    // Add 0.5 to sample at pixel centers
    #pragma omp parallel for
    for (int i = 0; i < stepsX * stepsY; ++i) {
        int sx = i / stepsY;
        int sy = i % stepsY;

        // Add 0.5 to sample at pixel centers.
        float x = minX + sx * blockSize + 0.5f;
        float y = minY + sy * blockSize + 0.5f;

        // Test if block is inside or outside triangle or touches it.
        EdgeData e00; e00.init(eqn, x, y);
        EdgeData e01 = e00; e01.stepY(eqn, s);
        EdgeData e10 = e00; e10.stepX(eqn, s);
        EdgeData e11 = e01; e11.stepX(eqn, s);

        int result = e00.test(eqn) + e01.test(eqn) + e10.test(eqn) + e11.test(eqn);

        // All out.
        if (result == 0)
            continue;

        if (result == 4)
            // Fully Covered
            rasterizeBlock<false>(eqn, x, y, surface, v0, v1, v2);
        else
            // Partially Covered
            rasterizeBlock<true>(eqn, x, y, surface, v0, v1, v2);
    }
    auto end = chrono::steady_clock::now();
    cout << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
}

template <bool TestEdges>
void rasterizeBlock(const TriangleEquations &eqn, float x, float y, SDL_Surface* surface) {
		PixelData po;
		po.init(eqn, x, y);

		EdgeData eo;
		if (TestEdges) {
            eo.init(eqn, x, y);
        }

		for (float yy = y; yy < y + blockSize; yy += 1.0f) {
			PixelData pi = po;

			EdgeData ei;
			if (TestEdges)
				ei = eo;

			for (float xx = x; xx < x + blockSize; xx += 1.0f) {
				if (!TestEdges || (eqn.e0.test(ei.ev0) && eqn.e1.test(ei.ev1) && eqn.e2.test(ei.ev2))) {
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

template <bool TestEdges>
void rasterizeBlock(const TriangleEquations &eqn, float x, float y, SDL_Surface* surface, const Vertex& v0, const Vertex& v1, const Vertex& v2) { 
    PixelData po;
    po.init(eqn, x, y);

    EdgeData eo;
    if (TestEdges) {
        eo.init(eqn, x, y);
    }

    for (float yy = y; yy < y + blockSize; yy += 1.0f) {
        PixelData pi = po;

        EdgeData ei;
        if (TestEdges) {
            ei = eo;
        }

        for (float xx = x; xx < x + blockSize; xx += 1.0f) {
            if (!TestEdges || (eqn.e0.test(ei.ev0) && eqn.e1.test(ei.ev1) && eqn.e2.test(ei.ev2))) {
                int rint = (int)(pi.r * 255);
                int gint = (int)(pi.g * 255);
                int bint = (int)(pi.b * 255);
                Uint32 color = SDL_MapRGB(surface->format, rint, gint, bint);
                putpixel(surface, (int)xx, (int)yy, color);
//                    BaryCoords bc(v0, v1, v2, x, y);
//                    Uint32 color = SDL_MapRGB(surface->format, bc.r, bc.g, bc.b);
//					putpixel(surface, (int)xx, (int)yy, color);
            }

            pi.stepX(eqn);
            if (TestEdges) {
                ei.stepX(eqn);
            }
        }

        po.stepY(eqn);
        if (TestEdges) {
            eo.stepY(eqn);
        }
    }
}

#endif
