// clang: -O1 -m32

OUTLINE int __cdecl testc(int a, int b) {
   return a * b;
}
OUTLINE int __stdcall tests(int a, int b) {
   return a * b;
}
OUTLINE int __fastcall testf(int a, int b) {
   return a * b;
}
OUTLINE int __thiscall testt(int a, int b) {
   return a * b;
}

EXPORT int __cdecl test(int a, int b) {
   a = testc(a,b);
   b = tests(a,b);
   a = testf(a,b);
   b = testt(a,b);
   return a + b;
}