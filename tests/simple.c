

EXPORT int test(int a, int b) {

	asm volatile("" : "+m"(b));
	{
		int* p = (int*)__builtin_alloca(a);
		asm volatile("" : "+m" (*p));
	}
	asm volatile("" : "+m"(b));

	while(b != 0) {
		a *= b;
		b--;
	}
	return a == 1337;
}