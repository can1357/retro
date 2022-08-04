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

struct meme {
   int(__cdecl*testc)(int, int);
   int(__stdcall*tests)(int, int);
   int(__fastcall*testf)(int, int);
   int(__thiscall*testt)(int, int);
};

EXPORT int __cdecl test(struct meme* m, int a, int b) {
   a = m->testc(a,b);
   b = m->tests(a,b);
   a = m->testf(a,b);
   b = m->testt(a,b);
   return a + b;
}
