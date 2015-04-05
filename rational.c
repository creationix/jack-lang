#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef enum jackType {
  Nil,
  Boolean,
  Number
} JackType;

typedef struct jackValue {
  JackType type;
  intptr_t num;
  intptr_t dem;
} JackValue;

static intptr_t gcd(intptr_t a, intptr_t b) {
  a = a < 0 ? -a : a;
  b = b < 0 ? -b : b; 
  while (a != b) {
    if (a < b) b-= a; 
    else a -= b;
  }
  return a;
}

static JackValue jack_add(JackValue a, JackValue b) {
  JackValue r;
  r.type = Number;

  if (a.dem == 1 && b.dem == 1) {
    r.num = a.num + b.num;
    r.dem = 1;
    return r;
  }
  intptr_t n, d, g;
  g = gcd(a.dem, b.dem);
  n = a.num * (b.dem / g) +
      b.num * (a.dem / g);
  d = a.dem / g * b.dem;
  g = gcd(n, d);
  if (g > 1) {
    n /= g;
    d /= g;
  }
  r.num = n;
  r.dem = d;
  return r;
}
  
static JackValue jack_sub(JackValue a, JackValue b) {
  JackValue r;
  r.type = Number;

  if (a.dem == 1 && b.dem == 1) {
    r.num = a.num - b.num;
    r.dem = 1;
    return r;
  }
  intptr_t n, d, g;
  g = gcd(a.dem, b.dem);
  n = a.num * (b.dem / g) -
      b.num * (a.dem / g);
  d = a.dem / g * b.dem;
  g = gcd(n, d);
  if (g > 1) {
    n /= g;
    d /= g;
  }
  r.num = n;
  r.dem = d;
  return r;
}

static JackValue jack_mul(JackValue a, JackValue b) {
  JackValue r;
  r.type = Number;
  if (a.dem == 1 && b.dem == 1) {
    r.dem = 1;
    r.num = a.num * b.num;
    return r;
  }
  intptr_t n = a.num * b.num;
  intptr_t d = a.dem * b.dem;
  intptr_t g = gcd(n < 0 ? -n : n, d);
  r.num = n / g;
  r.dem = d / g;
  return r;
}

static JackValue jack_div(JackValue a, JackValue b) {
  JackValue r;
  r.type = Number;
    
  if (a.dem == 1 && b.num == 1) {
    r.dem = 1;
    r.num = a.num * b.dem;
    return r;
  }
  intptr_t n = a.num * b.dem;
  intptr_t d = a.dem * b.num;
  if (d < 0) {
    d = -d;
    n = -n;
  }
  intptr_t g = gcd(n < 0 ? -n : n, d);
  r.num = n / g;
  r.dem = d / g;
  return r;
}

static JackValue rat(intptr_t num, intptr_t dem) {
  JackValue r;
  r.type = Number;
  if (dem < 0) {
    dem = -dem;
    num = -num;
  }
  intptr_t g = gcd(num, dem);
  r.num = num / g;
  r.dem = dem / g;
  return r;
}

static void dump(JackValue v) {
  printf("%ld/%ld\n", v.num, v.dem);
}

typedef JackValue(binop)(JackValue, JackValue);
 
static void test(binop fn, const char* op) {
  intptr_t a,b,c,d;
  a = (rand() % 24) + 1;
  b = (rand() % 24) + 1;
  c = (rand() % 24) + 1;
  d = (rand() % 24) + 1;
  printf("%ld/%ld %s %ld/%ld = ", a, b, op, c, d);
  dump(fn(rat(a, b), rat(c, d)));
}

int main() {
  for (int i = 0; i < 10; i++) {
    test(jack_add, "+");
    test(jack_sub, "-");
    test(jack_mul, "*");
    test(jack_div, "/");
  }
  return 0;
}
