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
  int num;
  int dem;
} JackValue;

static int gcd(int a, int b) {
  int c;
  while (a != 0) {
    c = a;
    a = b % a;
    b = c;
  }
  return b;
}

static JackValue jack_add(JackValue a, JackValue b) {
  JackValue r;
  r.type = Number;

  if (a.dem == b.dem) {
    r.num = a.num + b.num;
    r.dem = a.dem;
    return r;
  }

  int n, d, g;
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

  if (a.dem == b.dem) {
    r.num = a.num - b.num;
    r.dem = a.dem;
    return r;
  }

  int n, d, g;
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
  int n = a.num * b.num;
  int d = a.dem * b.dem;
  int g = gcd(n < 0 ? -n : n, d);
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
  int n = a.num * b.dem;
  int d = a.dem * b.num;
  if (d < 0) {
    d = -d;
    n = -n;
  }
  int g = gcd(n < 0 ? -n : n, d);
  r.num = n / g;
  r.dem = d / g;
  return r;
}

static JackValue rat(int num, int dem) {
  JackValue r;
  r.type = Number;
  if (num == 0) {
    r.num = 0;
    r.dem = 1;
    return r;
  }
  if (dem < 0) {
    dem = -dem;
    num = -num;
  }
  int g = gcd(num, dem);
  r.num = num / g;
  r.dem = dem / g;
  return r;
}

static void dump(JackValue v) {
  printf("%d/%d\n", v.num, v.dem);
}

typedef JackValue(binop)(JackValue, JackValue);

static void test(binop fn, const char* op) {
  int a,b,c,d;
  a = (rand() % 24) - 12;
  b = (rand() % 24) + 1;
  c = (rand() % 24) - 12;
  d = (rand() % 24) + 1;
  printf("%d/%d %s %d/%d = ", a, b, op, c, d);
  dump(fn(rat(a, b), rat(c, d)));
}

int main() {
  for (int i = 0; i < 100; i++) {
    test(jack_add, "+");
    test(jack_sub, "-");
    test(jack_mul, "*");
    test(jack_div, "/");
  }
  return 0;
}
