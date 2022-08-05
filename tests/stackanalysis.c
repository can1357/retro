// clang: -O1 -Xclang -cfguard


EXPORT int fn_stack_guard(int a, int b) {
	int* p = (int*)__builtin_alloca(a);
	asm volatile("" : "+m" (*p));
	return a * b; 
}

EXPORT int fn_control_guard(void(*f)()) {
    f();
    return 5;
}