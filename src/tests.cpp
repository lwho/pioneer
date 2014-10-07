// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

void test_frames();
void test_stringf();
void test_random();
void test_datetime();
void test_fixedf_cubic_root();

int main(int argc, char *argv[])
{
	test_fixedf_cubic_root();
	test_frames();
	test_stringf();
	test_random();
	test_datetime();
	return 0;
}
