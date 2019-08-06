#ifndef EQUATIONS_H_
#define EQUATIONS_H_

#include "SDL.h"
#include <omp.h>
#include <iostream>

using namespace std;

const int MaxVar = 16;

struct Vertex {
	float x; // The x component.
	float y; // The y component.
	float z; // The z component.
	float w; // The w component.

    float r;
    float g;
    float b;

    float var[MaxVar];
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
	ParameterEquation w;
	ParameterEquation var[MaxVar];

	TriangleEquations(const Vertex &v0, const Vertex &v1, const Vertex &v2)
	{
		e0.init(v0, v1);
		e1.init(v1, v2);
		e2.init(v2, v0);

		area = 0.5f * (e0.c + e1.c + e2.c);

		// Cull backfacing triangles.
		if (area < 0)
			return;

        r.init(v0.r, v1.r, v2.r, e0, e1, e2, area);
		g.init(v0.g, v1.g, v2.g, e0, e1, e2, area);
        b.init(v0.b, v1.b, v2.b, e0, e1, e2, area);
	}
};

#endif
