

EXPORT int test(int a, int b) {
	while(b != 0) {
		a *= b;
		b--;
	}
	return a;
}