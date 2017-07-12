#ifndef RENDER_H
#define RENDER_H

#include <GL/glew.h>

#include "vecmath.h"

struct OSDState {
	int charY;
	int fontHeight;
};

inline void FillRectangle(const Vec2f &p, const Vec2f &size, const Vec4f &color) {
	glColor4fv(&color.m[0]);
	glBegin(GL_QUADS);
	glVertex2f(p.x + size.x, p.y + size.y);
	glVertex2f(p.x, p.y + size.y);
	glVertex2f(p.x, p.y);
	glVertex2f(p.x + size.x, p.y);
	glEnd();
}

inline void DrawLine(const Vec2f &a, const Vec2f &b, const Vec4f &color) {
	glColor4fv(&color.m[0]);
	glBegin(GL_LINES);
	glVertex2f(a.x, a.y);
	glVertex2f(b.x, b.y);
	glEnd();
}

inline void DrawCircle(const Vec2f &p, const float radius, const Vec4f &color) {
	const int segments = 16;
	const float segmentRad = ((float)M_PI * 2.0f) / (float)segments;
	glColor4fv(&color.m[0]);
	glBegin(GL_LINE_LOOP);
	float r = 0;
	for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex) {
		float x = p.x + cosf(r) * radius;
		float y = p.y + sinf(r) * radius;
		glVertex2f(x, y);
		r += segmentRad;
	}
	glEnd();
}

inline void FillCircle(const Vec2f &p, const float radius, const Vec4f &color) {
	const int segments = 16;
	const float segmentRad = ((float)M_PI * 2.0f) / (float)segments;
	glColor4fv(&color.m[0]);
	glBegin(GL_POLYGON);
	float r = 0;
	for (int segmentIndex = 0; segmentIndex < segments; ++segmentIndex) {
		float x = p.x + cosf(r) * radius;
		float y = p.y + sinf(r) * radius;
		glVertex2f(x, y);
		r += segmentRad;
	}
	glEnd();
}

inline int GetFontHeight() {
	int result = glutBitmapHeight(GLUT_BITMAP_8_BY_13);
	return(result);
}
inline int GetTextWidth(const char *text) {
	int result = 0;
	for (int i = 0; i < strlen(text); ++i) {
		result += glutBitmapWidth(GLUT_BITMAP_8_BY_13, text[i]);
	}
	return(result);
}

inline void RenderText(float x, float y, const char *text, float scale, const Vec4f &color) {
	glColor4fv(&color.m[0]);
	glRasterPos2f(x, y);
	glPushMatrix();
	glScalef(scale, scale, 1.0f);
	glutBitmapString(GLUT_BITMAP_8_BY_13, (const unsigned char *)text);
	glPopMatrix();
}

inline OSDState CreateOSD() {
	OSDState result = {};
	result.charY = 0;
	result.fontHeight = GetFontHeight();
	return(result);
}

inline void DrawOSDLine(OSDState *osdState, const char *text) {
	glRasterPos2i(0, osdState->charY);
	glutBitmapString(GLUT_BITMAP_8_BY_13, (const unsigned char *)text);
	osdState->charY -= osdState->fontHeight;
}

#endif
