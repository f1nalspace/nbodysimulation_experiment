#ifndef VECMATH_H
#define VECMATH_H

#define _USE_MATH_DEFINES
#include <math.h>

#include <algorithm>

const float kDeg2Rad = (float)M_PI / 180.0f;

union Vec2i {
	struct {
		int x, y;
	};
	int m[2];

	inline Vec2i() {
		x = y = 0;
	}
	inline Vec2i(const Vec2i &v) {
		x = v.x;
		y = v.y;
	}
	inline Vec2i(const int initX, const int initY) {
		x = initX;
		y = initY;
	}
};

union Vec2f {
	struct {
		float x, y;
	};
	struct {
		float w, h;
	};
	float m[2];

	inline Vec2f() {
		x = y = 0;
	}
	inline Vec2f(const Vec2f &v) {
		x = v.x;
		y = v.y;
	}
	inline Vec2f(const float value) {
		x = value;
		y = value;
	}
	inline Vec2f(const float initX, const float initY) {
		x = initX;
		y = initY;
	}
};

union Mat2f {
	struct {
		Vec2f col1;
		Vec2f col2;
	};
	float m[4];

	inline Mat2f() {
		col1 = Vec2f(1, 0);
		col2 = Vec2f(0, 1);
	}

	inline Mat2f(const Mat2f &other) {
		col1 = other.col1;
		col2 = other.col2;
	}
};

union Vec4f {
	struct {
		float x, y, z, w;
	};
	struct {
		float r, g, b, a;
	};
	float m[4];

	inline Vec4f() {
		x = y = z = 0;
		w = 1;
	}
	inline Vec4f(const Vec4f &other) {
		x = other.x;
		y = other.y;
		z = other.z;
		w = other.w;
	}
	inline Vec4f(const float initX, const float initY, const float initZ, const float initW) {
		x = initX;
		y = initY;
		z = initZ;
		w = initW;
	}
};

union Mat4f {
	struct {
		Vec4f col1;
		Vec4f col2;
		Vec4f col3;
		Vec4f col4;
	};
	float m[16];

	inline Mat4f() {
		col1 = Vec4f(1.0f, 0.0f, 0.0f, 0.0f);
		col2 = Vec4f(0.0f, 1.0f, 0.0f, 0.0f);
		col3 = Vec4f(0.0f, 0.0f, 1.0f, 0.0f);
		col4 = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	}

	inline Mat4f(const Mat4f &other) {
		col1 = other.col1;
		col2 = other.col2;
		col3 = other.col3;
		col4 = other.col4;
	}

	inline static Mat4f TransformationFromVec2(const Vec2f &p) {
		Mat4f result = Mat4f();
		result.col4.x = p.x;
		result.col4.y = p.y;
		result.col4.z = 0.0f;
		return (result);
	}

	inline static Mat4f ScaleFromVec2(const Vec2f &s) {
		Mat4f result = Mat4f();
		result.col1.x = s.x;
		result.col2.y = s.y;
		result.col3.z = 0.0f;
		return (result);
	}
};

union Pixel {
	struct {
		uint8_t r, g, b, a;
	};
	uint8_t m[4];
};

inline float ScalarLerp(float a, float t, float b) {
	float result = (1.0f - t) * a + t * b;
	return(result);
}

inline Vec2f operator*(const Vec2f &a, float b) {
	Vec2f result = Vec2f(a.x * b, a.y * b);
	return(result);
}
inline Vec2f& operator*=(Vec2f &a, float value) {
	a = a * value;
	return(a);
}
inline Vec2f operator-(const Vec2f &a) {
	Vec2f result = Vec2f(-a.x, -a.y);
	return(result);
}
inline Vec2f operator+(const Vec2f &a, const Vec2f &b) {
	Vec2f result = Vec2f(a.x + b.x, a.y + b.y);
	return(result);
}
inline Vec2f& operator+=(Vec2f &a, const Vec2f &b) {
	a = b + a;
	return(a);
}
inline Vec2f operator-(const Vec2f &a, const Vec2f &b) {
	Vec2f result = Vec2f(a.x - b.x, a.y - b.y);
	return(result);
}
inline Vec2f& operator-=(Vec2f &a, const Vec2f &b) {
	a = a - b;
	return(a);
}

inline float Vec2Dot(const Vec2f &a, const Vec2f &b) {
	float result = a.x * b.x + a.y * b.y;
	return(result);
}

inline float Vec2Length(const Vec2f &v) {
	float result = sqrtf(v.x * v.x + v.y * v.y);
	return(result);
}

inline Vec2f Vec2Normalize(const Vec2f &v) {
	float l = Vec2Length(v);
	if (l == 0) {
		l = 1;
	}
	float invL = 1.0f / l;
	Vec2f result = Vec2f(v) * invL;
	return(result);
}

inline Vec2f Vec2Hadamard(const Vec2f &a, const Vec2f &b) {
	Vec2f result = Vec2f(a.x * b.x, a.y * b.y);
	return(result);
}

inline Vec2f Vec2MultMat2(const Mat2f &A, const Vec2f &v) {
	Vec2f result = Vec2f(A.col1.x * v.x + A.col2.x * v.y, A.col1.y * v.x + A.col2.y * v.y);
	return(result);
}

inline float Vec2DistanceSquared(const Vec2f &a, const Vec2f &b) {
	float f = (b.x - a.x) * (b.y - a.y);
	float result = f * f;
	return(result);
}

/* Returns the right perpendicular vector */
inline Vec2f Vec2Cross(const Vec2f &a, float s) {
	return Vec2f(s * a.y, -s * a.x);
}

/* Returns the left perpendicular vector */
inline Vec2f Vec2Cross(float s, const Vec2f &a) {
	return Vec2f(-s * a.y, s * a.x);
}

inline float Vec2Cross(const Vec2f &a, const Vec2f &b) {
	return a.x * b.y - a.y * b.x;
}

inline float Vec2AxisToAngle(const Vec2f &axis) {
	float result = atan2f(axis.y, axis.x);
	return(result);
}

inline Vec2f Vec2RandomDirection() {
	float d = rand() / (float)RAND_MAX;
	float angle = d * ((float)M_PI * 2.0f);
	Vec2f result = Vec2f(cosf(angle), sinf(angle));
	return(result);
}

inline Vec2f Vec2Lerp(const Vec2f &a, float t, const Vec2f &b) {
	Vec2f result;
	result.x = ScalarLerp(a.x, t, b.x);
	result.y = ScalarLerp(a.y, t, b.y);
	return(result);
}

//
// Mat2f
//

inline Mat2f Mat2Identity() {
	Mat2f result = Mat2f();
	return (result);
}

inline Mat2f Mat2FromAngle(float angle) {
	float s = sinf(angle);
	float c = cosf(angle);
	Mat2f result;
	result.col1 = Vec2f(c, s);
	result.col2 = Vec2f(-s, c);
	return(result);
}

inline Mat2f Mat2FromAxis(const Vec2f &axis) {
	Mat2f result;
	result.col1 = axis;
	result.col2 = Vec2Cross(1.0f, axis);
	return(result);
}

inline Mat2f Mat2Transpose(const Mat2f &m) {
	Mat2f result;
	result.col1 = Vec2f(m.col1.x, m.col2.x);
	result.col2 = Vec2f(m.col1.y, m.col2.y);
	return(result);
}

inline Mat2f Mat2Mult(const Mat2f &a, const Mat2f &b) {
	Mat2f result;
	result.col1 = Vec2MultMat2(a, b.col1);
	result.col2 = Vec2MultMat2(a, b.col2);
	return(result);
}

inline float Mat2ToAngle(const Mat2f &mat) {
	float result = Vec2AxisToAngle(mat.col1);
	return(result);
}

/* Generates a 2x2 matrix for doing B to A conversion */
inline Mat2f Mat2MultTranspose(const Mat2f &a, const Mat2f &b) {
	Mat2f result;
	result.col1 = Vec2f(Vec2Dot(a.col1, b.col1), Vec2Dot(a.col2, b.col1));
	result.col2 = Vec2f(Vec2Dot(a.col1, b.col2), Vec2Dot(a.col2, b.col2));
	return(result);
}



inline Mat4f &operator *(const Mat4f &a, const Mat4f &b) {
	Mat4f result;
	for (int i = 0; i < 16; i += 4) {
		for (int j = 0; j < 4; ++j) {
			result.m[i + j] =
				(b.m[i + 0] * a.m[j + 0])
				+ (b.m[i + 1] * a.m[j + 4])
				+ (b.m[i + 2] * a.m[j + 8])
				+ (b.m[i + 3] * a.m[j + 12]);
		}
	}
	return(result);
}

const static Vec4f ColorWhite = Vec4f(1.0f, 1.0f, 1.0f, 1.0f);
const static Vec4f ColorRed = Vec4f(1.0f, 0.0f, 0.0f, 1.0f);
const static Vec4f ColorGreen = Vec4f(0.0f, 1.0f, 0.0f, 1.0f);
const static Vec4f ColorBlue = Vec4f(0.0f, 0.0f, 1.0f, 1.0f);
const static Vec4f ColorLightGray = Vec4f(0.3f, 0.3f, 0.3f, 1.0f);
const static Vec4f ColorDarkGray = Vec4f(0.2f, 0.2f, 0.2f, 1.0f);

#define RGBAToPixel(rgba) {((rgba) >> 0) & 0xFF, ((rgba) >> 8) & 0xFF, ((rgba) >> 16) & 0xFF, ((rgba) >> 24) & 0xFF}
const static float INV255 = 1.0f / 255.0f;

inline Vec4f RGBAToLinear(uint32_t rgba) {
	Pixel pixel = RGBAToPixel(rgba);
	Vec4f result = Vec4f(pixel.r * INV255, pixel.g * INV255, pixel.b * INV255, pixel.a * INV255);
	return(result);
}

#endif