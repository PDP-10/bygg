#include <stdio.h>

void x_foo(void) { /* ... */ }
void x_bar(void) { /* ... */ }

/******** first way: */

typedef void (func)(void);

typedef struct kw_1 {
  char* key;
  int flags;
  union {
    void* p;
    func* f;
    int i;
  } data;
} keyword_1;

static keyword_1 k1[] = {
  { "foo", 0, {.f=x_foo} },
  { "bar", 0, {.f=x_bar} },
  { "urk", 1, {.p="urrrk"} },
  { "xyz", 2, {.i=4711} },
};

/******** second way: */

typedef struct kw_2 {
  char* key;
  int flags;
  void* data;
} keyword_2;

#define ANY(x) ((void*) ((unsigned long long) (x)))

static keyword_2 k2[] = {
  { "foo", 0, ANY(x_foo) },
  { "bar", 0, ANY(x_bar) },
  { "urk", 1, ANY("urrrk") },
  { "xyz", 2, ANY(4711) },
};

/******** hybrid way: */

#if __STDC_VERSION__ < 199901L
#  define KEYDATA(f, x) ((void*) ((unsigned long long) (x)))
#else
#  define KEYDATA(f, x) {.f=x}
#endif

static keyword_1 hybr[] = {
  { "foo", 0, KEYDATA(f, x_foo) },
  { "bar", 0, KEYDATA(f, x_bar) },
  { "urk", 1, KEYDATA(p, "urrrk") },
  { "xyz", 2, KEYDATA(i, 4711) },
};

int main(int argc, char* argv[])
{
  printf("short: %d\n", sizeof(short));
  printf("int:   %d\n", sizeof(int));
  printf("long:  %d\n", sizeof(long));
  printf("llong: %d\n", sizeof(long long));
  printf("void*: %d\n", sizeof(void*));
  printf("func*: %d\n", sizeof(&main));
}
