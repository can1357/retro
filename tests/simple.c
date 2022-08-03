// clang: -O1

EXPORT int _fn_0(int* a) {
	return *a * 2;
}

EXPORT void _fn_memcpy(void* dst, void* src, size_t count) {
    asm volatile("cld; rep movsb" : "+D"(dst), "+S"(src), "+c"(count) : : "memory");
}

EXPORT int fn_1(int a, int b) {
	int* p = (int*)__builtin_alloca(a);
	asm volatile("" : "+m" (*p));

   while(b != 0) {
		a *= b;
		b--;
	}
	b += _fn_0(&a);
	return a * b; 
}
EXPORT int _fn_2(int a) {
	return fn_1(a, a);
}