
// clang: -O2

EXPORT void test(float* x) {
   for(int i = 0; i != 4; i++){
      x[i] *= 3.412f;
   }
}