#ifndef RENDER_H
#define RENDER_H

#include <GL/glew.h>
#include <GL/freeglut.h>

#include "vecmath.h"

struct OSDState {
	int charY;
	int fontHeight;
	void *font;
};

inline void DrawRectangle(const Vec2f &p, const Vec2f &size, const Vec4f &color, const float lineWidth) {
	glLineWidth(lineWidth);
	glColor4fv(&color.m[0]);
	glBegin(GL_LINE_LOOP);
	glVertex2f(p.x + size.x, p.y + size.y);
	glVertex2f(p.x, p.y + size.y);
	glVertex2f(p.x, p.y);
	glVertex2f(p.x + size.x, p.y);
	glEnd();
	glLineWidth(1.0f);
}

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

inline int GetFontHeight(void *font) {
	int result = glutBitmapHeight(font);
	return(result);
}
inline int GetTextWidth(const char *text, void *font) {
	int result = 0;
	for (int i = 0; i < strlen(text); ++i) {
		result += glutBitmapWidth(font, text[i]);
	}
	return(result);
}

inline void RenderText(float x, float y, const char *text, const Vec4f &color, void *font) {
	glColor4fv(&color.m[0]);
	glRasterPos2f(x, y + 2.0f);
	glPushMatrix();
	glutBitmapString(font, (const unsigned char *)text);
	glPopMatrix();
}

inline float GetStrokeTextWidth(const char *text, float size) {
	float w = glutStrokeLengthf(GLUT_STROKE_ROMAN, (const unsigned char *)text);
	float h = glutStrokeHeight(GLUT_STROKE_ROMAN);
	float result = size * (w / h);
	return(result);
}

inline void RenderStrokeText(float x, float y, const char *text, const Vec4f &color, float size, float lineWidth) {
	float h = glutStrokeHeight(GLUT_STROKE_ROMAN);
	glLineWidth(lineWidth);
	glColor4fv(&color.m[0]);
	glPushMatrix();
	glTranslatef(x, y, 0.0f);
	glScalef(size / h, size / h, 1.0f);
	glutStrokeString(GLUT_STROKE_ROMAN, (const unsigned char *)text);
	glPopMatrix();
	glLineWidth(1.0f);
}

inline OSDState CreateOSD(void *font) {
	OSDState result = {};
	result.charY = 0;
	result.fontHeight = GetFontHeight(font);
	result.font = font;
	return(result);
}

inline void DrawOSDLine(OSDState *osdState, const char *text) {
	glRasterPos2f(0.0f, (float)osdState->charY + 2.0f);
	glutBitmapString(osdState->font, (const unsigned char *)text);
	osdState->charY -= osdState->fontHeight;
}

#endif
