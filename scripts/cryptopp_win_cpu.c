/* Crypto++ 8.9 on Clang for Windows calls these two names from x64dll.asm. */
unsigned long long XGETBV64(unsigned int num) {
    unsigned int eax, edx;
    __asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(num));
    return ((unsigned long long)edx << 32) | eax;
}

unsigned long long CPUID64(unsigned int func, unsigned int sub, unsigned int *out) {
    unsigned int a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(func), "c"(sub));
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
    return a;
}
