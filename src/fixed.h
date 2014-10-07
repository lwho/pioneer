// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _FIXED_H
#define _FIXED_H

#include <SDL_stdinc.h>
#include <cassert>
#include <algorithm>
#include "MathUtil.h"

template <int FRAC_BITS>
class fixedf {
public:
	static const int FRAC = FRAC_BITS;
	static const Uint64 MASK = (Uint64(1UL)<<FRAC_BITS)-1;

	fixedf(): v(0) {}
//	template <int bits>
//	fixedf(fixedf<bits> f) { *this = f; }
	fixedf(Sint64 raw): v(raw) {}
	fixedf(Sint64 num, Sint64 denom): v((num<<FRAC) / denom) {}
	// ^^ this is fucking shit

	fixedf Abs() const { return fixedf(v >= 0 ? v : -v); }
	friend fixedf operator+(const fixedf a, const Sint64 b) { return a+fixedf(b<<FRAC); }
	friend fixedf operator-(const fixedf a, const Sint64 b) { return a-fixedf(b<<FRAC); }
	friend fixedf operator*(const fixedf a, const Sint64 b) { return a*fixedf(b<<FRAC); }
	friend fixedf operator/(const fixedf a, const Sint64 b) { return a/fixedf(b<<FRAC); }
	friend fixedf operator+(const Sint64 a, const fixedf b) { return fixedf(a<<FRAC)+b; }
	friend fixedf operator-(const Sint64 a, const fixedf b) { return fixedf(a<<FRAC)-b; }
	friend fixedf operator*(const Sint64 a, const fixedf b) { return fixedf(a<<FRAC)*b; }
	friend fixedf operator/(const Sint64 a, const fixedf b) { return fixedf(a<<FRAC)/b; }
	friend bool operator==(const fixedf a, const Sint64 b) { return a == fixedf(b<<FRAC); }
	friend bool operator==(const Sint64 a, const fixedf b) { return b == fixedf(a<<FRAC); }
	friend bool operator!=(const fixedf a, const Sint64 b) { return a != fixedf(b<<FRAC); }
	friend bool operator!=(const Sint64 a, const fixedf b) { return b != fixedf(a<<FRAC); }
	friend bool operator>=(const fixedf a, const Sint64 b) { return a >= fixedf(b<<FRAC); }
	friend bool operator>=(const Sint64 a, const fixedf b) { return b >= fixedf(a<<FRAC); }
	friend bool operator<=(const fixedf a, const Sint64 b) { return a <= fixedf(b<<FRAC); }
	friend bool operator<=(const Sint64 a, const fixedf b) { return b <= fixedf(a<<FRAC); }
	friend bool operator>(const fixedf a, const Sint64 b) { return a > fixedf(b<<FRAC); }
	friend bool operator>(const Sint64 a, const fixedf b) { return b > fixedf(a<<FRAC); }
	friend bool operator<(const fixedf a, const Sint64 b) { return a < fixedf(b<<FRAC); }
	friend bool operator<(const Sint64 a, const fixedf b) { return b < fixedf(a<<FRAC); }
	friend fixedf operator>>(const fixedf a, const int b) { return fixedf(a.v >> b); }
	friend fixedf operator<<(const fixedf a, const int b) { return fixedf(a.v << b); }

	fixedf &operator*=(const fixedf a) { (*this) = (*this)*a; return (*this); }
	fixedf &operator*=(const Sint64 a) { (*this) = (*this)*a; return (*this); }
	fixedf &operator/=(const fixedf a) { (*this) = (*this)/a; return (*this); }
	fixedf &operator/=(const Sint64 a) { (*this) = (*this)/a; return (*this); }
	fixedf &operator+=(const fixedf a) { (*this) = (*this)+a; return (*this); }
	fixedf &operator+=(const Sint64 a) { (*this) = (*this)+a; return (*this); }
	fixedf &operator-=(const fixedf a) { (*this) = (*this)-a; return (*this); }
	fixedf &operator-=(const Sint64 a) { (*this) = (*this)-a; return (*this); }
	fixedf &operator>>=(const int a) { v >>= a; return (*this); }
	fixedf &operator<<=(const int a) { v <<= a; return (*this); }

	friend fixedf operator-(const fixedf a) { return fixedf(-a.v); }
	friend fixedf operator+(const fixedf a, const fixedf b) { return fixedf(a.v+b.v); }
	friend fixedf operator-(const fixedf a, const fixedf b) { return fixedf(a.v-b.v); }
	friend fixedf operator*(fixedf a, fixedf b) {
		// 64*64 = (128bit>>FRAC) & ((1<<64)-1)
		//return fixedf(a.v*b.v >> FRAC);
		Uint64 hi;
		Uint64 lo;
		bool isneg = false;
		__int128_t check;
		if (a.v < 0) {
			a.v = -a.v;
			isneg = !isneg;
		}
		check = __int128_t(a.v);
		if (b.v < 0) {
			b.v = -b.v;
			isneg = !isneg;
		}
		check *= __int128_t(b.v);
		MathUtil::mult64to128(a.v, b.v, &hi, &lo);
		Sint64 out = (lo>>FRAC) + ((hi&MASK)<<(64-FRAC));
		check >>= FRAC;
		if (isneg) {
			check = -check;
			out = -out;
		}
		//assert(check <= INT64_MAX);
		assert(check > INT64_MAX || check < INT64_MIN || out == check);
		return out;
	}
	friend fixedf operator/(const fixedf a, const fixedf b) {
		// 128-bit divided by 64-bit, to make sure high bits are not lost
		Sint64 quotient_hi = a.v>>(64-FRAC);
		Uint64 quotient_lo = a.v<<FRAC;
		Sint64 d = b.v;
		int isneg = 0;
		Sint64 remainder=0;

		if (d < 0) {
			d = -d;
			isneg = 1;
		}

		for (int i=0; i<128; i++) {
			Uint64 sbit = (Uint64(1)<<63) & quotient_hi;
			remainder <<= 1;
			if (sbit) remainder |= 1;
			// shift quotient left 1
			{
				quotient_hi <<= 1;
				if (quotient_lo & (Uint64(1)<<63)) quotient_hi |= 1;
				quotient_lo <<= 1;
			}
			if (remainder >= d) {
				remainder -= d;
				quotient_lo |= 1;
			}
		}
		return (isneg ? -Sint64(quotient_lo) : quotient_lo);
	}
	friend bool operator==(const fixedf a, const fixedf b) { return a.v == b.v; }
	friend bool operator!=(const fixedf a, const fixedf b) { return a.v != b.v; }
	friend bool operator>(const fixedf a, const fixedf b) { return a.v > b.v; }
	friend bool operator<(const fixedf a, const fixedf b) { return a.v < b.v; }
	friend bool operator>=(const fixedf a, const fixedf b) { return a.v >= b.v; }
	friend bool operator<=(const fixedf a, const fixedf b) { return a.v <= b.v; }

	/* implicit operator float() bad */
	int ToInt32() const { return int(v>>FRAC); }
	Sint64 ToInt64() const { return v>>FRAC; }
	float ToFloat() const { return v/float(Sint64(1)<<FRAC); }
	double ToDouble() const { return v/double(Sint64(1)<<FRAC); }

	static fixedf FromDouble(const double val) { return fixedf(Sint64(((val) * double(Sint64(1)<<FRAC)))); }

	template <int NEW_FRAC_BITS>
	operator fixedf<NEW_FRAC_BITS>() const {
		int shift = NEW_FRAC_BITS - FRAC_BITS;
		if (shift > 0) return fixedf<NEW_FRAC_BITS>(v<<shift);
		else return fixedf<NEW_FRAC_BITS>(v>>(-shift));
	}

	static fixedf SqrtOf(fixedf a) {
		/* only works on even-numbered fractional bits */
		assert(!(FRAC & 1));
		Uint64 root, remHi, remLo, testDiv, count;
		root = 0;
		remHi = 0;
		remLo = a.v;
		count = 32+(FRAC>>1)-1;
		do {
			remHi = (remHi<<2) | (remLo>>62); remLo <<= 2;
			root <<= 1;
			testDiv = (root << 1) + 1;
			if (remHi >= testDiv) {
				remHi -= testDiv;
				root++;
			}
		} while (count-- != 0);

		return(fixedf(root));
	}

#if 0
	static fixedf CubeRootOf(fixedf a) {
		/* NR method. XXX very bad initial estimate (we get there in
		 * the end... XXX */
		fixedf x = a;
		for (int i=0; i<48; i++) x = fixedf(1,3) * ((a / (x*x)) + 2*x);
		return x;
	}
#endif

	static fixedf CubeRootOf(fixedf a);

	Sint64 v;
};

#if 0
inline Uin64 CubeRootOf(const Uint64 a)
{
	/* http://en.wikipedia.org/wiki/Shifting_nth_root_algorithm with B=2, n=3
	 * NR may overflow even though a cube root is never larger than the operand */
	Uint64 y = 0;
	Uint64 r = 0;
	Uint64 av = a;
	for (int i = 0; i < 64; ++i) {
		const unsigned alpha = (av & 0xe000000000000000) >> 61;
		av <<= 3;
		const Uint64 det1 = ((12 * y + 6) * y) + 1; // This is (2*y + 1)^3 - 2^3 * y^3
		const Uint64 det2 = 8*r + alpha;
		if (det1 <= det2) {	// beta = 1
			y = 2*y + 1;
			r = det2 - det1;
		} else {			// beta = 0
			y = 2*y;
			r = det2;
		}
	}
	const fixedf<FRAC_BITS> result = fixedf<FRAC_BITS>(y);
	assert(result * result * result <= a);
	const fixedf<FRAC_BITS> next = fixedf<FRAC_BITS>(y+1);
	assert(next * next * next > a);
	return isneg ? -result : result;
}
#endif

template <int FRAC_BITS>
inline fixedf<FRAC_BITS> fixedf<FRAC_BITS>::CubeRootOf(fixedf<FRAC_BITS> a)
{
	static_assert(FRAC_BITS <= 58, "CubeRootOf currently only supports at most 58 FRAC_BITS");
	/* http://en.wikipedia.org/wiki/Shifting_nth_root_algorithm with B=2, n=3
	 * NR may overflow even though a cube root is never larger than the operand */
	bool isneg = false;
	if (a.v < 0) {
		a.v = -a.v;
		isneg = true;
	}
	Uint64 av = FRAC_BITS % 3 == 0 ? Uint64(a.v) << 1 :
				FRAC_BITS % 3 == 1 ? Uint64(a.v) :
				/* else */           Uint64(a.v) >> 1;
	Uint64 y = 0;
	Uint64 r = 0;
	Uint64 x = 0;
	// Up to 13 FRAC_BITS this can not overflow 64 bit unsigned
	__int128_t x1 = 0;
	const int end = std::min(FRAC_BITS - FRAC_BITS/3, 9);
	for (int i = 0; i < 63/3 + end; ++i) {
		const unsigned alpha = (av & 0xe000000000000000) >> 61;
		x = x << 3 | alpha;
		x1 = x1 << 3 | alpha;
		av <<= 3;
		const Uint64 det1 = ((12 * y + 6) * y) + 1; // This is (2*y + 1)^3 - 2^3 * y^3
		assert(r < 0x2000000000000000); // FIXME: FAILS for i == 30
		const Uint64 det2 = 8*r + alpha;
		if (det1 <= det2) {	// beta = 1
			y = 2*y + 1;
			r = det2 - det1;
		} else {			// beta = 0
			y = 2*y;
			r = det2;
		}
		//assert(y*y*y + r == x);
		//assert((y+1)*(y+1)*(y+1) > x);
		assert(__int128_t(y)*__int128_t(y)*__int128_t(y) + __int128_t(r) == x1);
		assert(__int128_t(y+1)*__int128_t(y+1)*__int128_t(y+1) > x1);
	}
	Uint64 r1 = 0;
	for (int i = 9; i < FRAC_BITS - FRAC_BITS/3; ++i) {
#if 1
		 // calculate ((12 * y + 6) * y) + 1
		Uint64 det1hi, det1lo;
		det1lo = 12 * y + 6; // Overflow-free for FRAC_BITS <= 58 (each iteration adds 1 bit to y: 63/3 + 58 - 58/3 = 60)
		MathUtil::mult64to128(y, det1lo, &det1hi, &det1lo);
		++det1lo; det1hi += (det1lo == 0);
		Uint64 det2hi = (r1 << 3) | (r >> 61);
		Uint64 det2lo = r << 3;
		if (det1hi < det2hi || (det1hi == det2hi && det1lo <= det2lo)) {	// beta = 1
			y = 2*y + 1;
			r = det2lo - det1lo;
			r1 = det2hi - det1hi - Uint64(r > det2lo);
		} else {															// beta = 0
			y = 2*y;
			r = det2lo;
			r1 = det2hi;
		}
#else
		const __int128_t det1 = ((12 * __int128_t(y) + 6) * y) + 1; // This is (2*y + 1)^3 - 2^3 * y^3
		const __int128_t det2 = 8*r1;
		if (det1 <= det2) {	// beta = 1
			y = 2*y + 1;
			r1 = det2 - det1;
		} else {			// beta = 0
			y = 2*y;
			r1 = det2;
		}
#endif
		assert(i > 21 || __int128_t(y)*__int128_t(y)*__int128_t(y) + (__int128_t(r1) << 64) + r == (x1 << (i*3 - 24)));
		assert(i > 21 || __int128_t(y+1)*__int128_t(y+1)*__int128_t(y+1) > (x1 << (i*3 - 24)));
	}
	return isneg ? -fixedf<FRAC_BITS>(y) : fixedf<FRAC_BITS>(y);
}

typedef fixedf<32> fixed;

#endif /* _FIXED_H */
