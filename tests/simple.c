


EXPORT int _fn_0(int* a) {
	return *a * 2;
}
EXPORT int fn_1(int a, int b) {
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