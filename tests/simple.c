// clang: -O1 -mno-avx -target x86_64-pc-windows -march=nehalem -mno-avx -mno-avx2 -mno-avx512f -mllvm --use-retard-cc


/*
EXPORT void _fn_memcpy(void* dst, void* src, size_t count) {
    asm volatile("cld; rep movsb" : "+D"(dst), "+S"(src), "+c"(count) : : "memory");
}


EXPORT int _fn_0(int* a) {
	return *a * 2;
}
EXPORT int fn_1(int a, int b) {
	int* p = (int*)__builtin_alloca(a);
	asm volatile("" : "+m" (*p));
	asm volatile("" ::: "xmm7");

   while(b != 0) {
		a *= b;
		b--;
	}
	b += _fn_0(&a);
	return a * b; 
}
EXPORT int _fn_2(int a) {
	return fn_1(a, a);
}*/



EXPORT int cftest() {

	while(1){
		if (!short_marker("if(x)")) {
			marker("doshit1()");
			break;
		}
		if (!short_marker("if(y)")) {
			marker("doshit2()");
			break;
		}
		if (!short_marker("if(z)")) {
			return 0;
		}
		marker("rest-of-the-loop-body");
	}
	marker("long shit #1");
	marker("long shit #2");
	marker("long shit #3");
	return marker("long shit #4");

	/*if(!marker("if-cc")){
		marker("if-body");
	} else {
		marker("else-body");
	}

	while(!short_marker("while-cc")){
		marker("loop-body-begin");
		if(!short_marker("bif")) {
			marker("break");
			break;
		}
		if(!short_marker("cif")) {
			marker("continue");
			continue;
		}
		marker("loop-body-end");
	}

	return marker("end");*/
}

/*
OUTLINE int me_take_long(char x, ...) { asm volatile("" :: "r"(x)); return x;}

EXPORT int fn_2(int a, int b) {
	return a * b;
}

EXPORT int fn_1(int a) {
	return a * fn_2(a, a*56);
}

EXPORT int test(int a, int _0, int _1, int _2, int _3, int _4, int xx) {
	if(a == 12515) {
		int* p = (int*)__builtin_alloca(a);
		asm volatile("" : "+m" (*p));
		asm volatile("" ::: "xmm7");
		return *p;
	}


    int x = me_take_long(
        6,
        16,
        16,
        16,
        me_take_long(3) ? me_take_long(1) : 6,
        me_take_long(4) ? 1 : 5,
        a ? 9 : me_take_long(6)
    );
    int y = x * a + xx;

    //int x = 2*me_take_long(a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a);
    //int y = 2*me_take_long(a,a,a,a,a,a,a,a,a,a,a,a);
   // x = 2*me_take_big(a, {}, a);
   // y = 2*me_take_big(a, {}, a);

    return x+y;
}*/