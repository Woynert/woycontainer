
static void *woytest_get_string(intmax_t count, bool reset)
{
    enum { BUFFER_CAP = 1024 };
    static char buffer[BUFFER_CAP];
    static struct { char *beg; char *end; } a = { 0 };
    if (a.beg == NULL || reset) {
        a.beg = buffer;
        a.end = buffer + BUFFER_CAP;
        if (reset) return NULL;
    }

    // Source https://nullprogram.com/blog/2023/09/27/

    const intmax_t size = sizeof(char); // We only "allocate" strings.
    const intmax_t align = _Alignof(char);

    ptrdiff_t padding = (ptrdiff_t)( -(uintptr_t)a.beg & (uintptr_t)(align - 1) );
    ptrdiff_t available = a.end - a.beg - padding;
    if (available <= 0 || count > available / size) {
        printf("OUT OF MEMORY.\n");
        abort();
    }
    void *p = a.beg + padding;
    a.beg += padding + count * size;
    return memset(p, 0, (size_t)(count * size));
}
