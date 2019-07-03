#ifndef XMALLOC_HEADER
#define XMALLOC_HEADER 1

void *xmalloc(size_t size);
void *xcalloc(size_t count, size_t size);
void *xrealloc(void *ptr, size_t size);
char *xstrdup(const char *s);

#endif
