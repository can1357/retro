// clang: -O1 -m32

EXPORT int __cdecl _testc(int a, int b) {
   return a * b;
}
EXPORT int __stdcall _tests(int a, int b) {
   return a * b;
}
EXPORT int __fastcall _testf(int a, int b) {
   return a * b;
}
EXPORT int __thiscall _testt(int a, int b) {
   return a * b;
}

EXPORT int __cdecl test(int a, int b) {
   a = _testc(a,b);
   b = _tests(a,b);
   a = _testf(a,b);
   b = _testt(a,b);
   return a + b;
}