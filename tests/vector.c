
// clang: -O2

EXPORT void test(float* x) {
   for(int i = 0; i != 8; i++){
      x[i] *= 3.412f;
      *(long*)&x[i] |= 1;
   }
}