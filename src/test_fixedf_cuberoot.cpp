// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "fixed.h"
#include "Random.h"

void test_fixedf_cubic_root()
{
	typedef fixedf<16> fixed16;
	typedef fixedf<32> fixed32;
	typedef fixedf<48> fixed48;
	typedef fixedf<48> fixed58;
	Random rand(42);
	for (int i = 0; i < 10000000; ++i) {
		Uint32 first = rand.Int32();
		Uint32 second = rand.Int32();
		fixed16 f16(Sint64(first) << 32 | Sint64(second));
		fixed32 f32(Sint64(first) << 32 | Sint64(second));
		fixed48 f48(Sint64(first) << 32 | Sint64(second));
		fixed58 f58(Sint64(first) << 32 | Sint64(second));
		fixed16 f16_root = fixed16::CubeRootOf(f16);
		fixed32 f32_root = fixed32::CubeRootOf(f32);
		fixed48 f48_root = fixed48::CubeRootOf(f48);
		fixed58 f58_root = fixed48::CubeRootOf(f58);
	}
}
