// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MATHUTIL_H
#define _MATHUTIL_H

#include "vector3.h"

namespace MathUtil {

	// random point on a sphere, distributed uniformly by area
	vector3d RandomPointOnSphere(double minRadius, double maxRadius);
	inline vector3d RandomPointOnSphere(double radius) { return RandomPointOnSphere(radius, radius); }

	vector3d RandomPointInCircle(double minRadius, double maxRadius);
	inline vector3d RandomPointInCircle(double radius) { return RandomPointInCircle(0.0, radius); }
	inline vector3d RandomPointOnCircle(double radius) { return RandomPointInCircle(radius, radius); }

	// interpolation, glsl style naming "mix"
	template< class T, class F >
	T mix(const T& v1, const T& v2, const F t){ 
		return t*v2 + (F(1.0)-t)*v1;
	}

	inline float Dot(const vector3f &a, const vector3f &b) { return a.x*b.x + a.y*b.y + a.z*b.z; }

	inline Uint32 ceil_pow2(Uint32 v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	inline void mult64to128(Uint64 u, Uint64 v, Uint64* out_hi, Uint64* out_lo)
	{
#if 0
		//#if (defined __GNUC__ || defined __clang__) && defined __x86_64__
		__asm__("mulq %3"
		    : "=d" (*out_hi), "=a" (*out_lo)
		    : "a" (u), "rm" (v)
		    : "cc" );

#else
		Uint64 u0, u1, v0, v1, k, t;
		Uint64 w1, w2, w3;

		u0 = u >> 32; u1 = u & 0xffffffff;
		v0 = v >> 32; v1 = v & 0xffffffff;

		t = u1*v1;
		w3 = t & 0xffffffff;
		k = t >> 32;

		// I was baffled why this can't overflow, but 0xffffffff**2 indeed leaves enough room in 64 bits to add another 32 bit value
		// maximum for u0*v1 is (2**32-1)**2 == 2**64-2**33+1
		t = u0*v1 + k;
		w2 = t & 0xffffffff;
		w1 = t >> 32;

		t = u1*v0 + w2; // same here, no overflow
		k = t >> 32;

		*out_hi = u0*v0 + w1 + k;
		*out_lo = (t << 32) + w3;
#endif
	}

	inline void square64to128(Uint64 a, Uint64* out_hi, Uint64* out_lo)
	{
	//#if (defined __GNUC__ || defined __clang__) && defined __x86_64__
#if 0
		__asm__("mulq %%rax"
		    : "=d" (*out_hi), "=a" (*out_lo)
		    : "a" (a)
		    : "cc" );

#else
		mult64to128(a, a, out_hi, out_lo);
#endif
	}

}

#endif
