
EXPORT int test(int a, int b) {
	while(b != 0) {
		a *= b;
		b--;
	}
	//sinkptr(__builtin_return_address(0));
	return a == 1337;
}