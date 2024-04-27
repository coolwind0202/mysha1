#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef uint32_t word;
typedef uint8_t byte;

word circular_shift(word x, int n)
{
  return (x << n) | (x >> (32 - n));
}

void print_sha1(word *hash)
{
  for (size_t cur = 0; cur < 5; cur++)
  {
    printf("%08x ", hash[cur]);
  }
  puts("");
}

word f(size_t t, word b, word c, word d)
{
  if (t <= 19)
    return (b & c) | ((~b) & d);
  else if (t <= 39)
    return (b ^ c) ^ d;
  else if (t <= 59)
    return (b & c) | (b & d) | (c & d);
  else if (t <= 79)
    return b ^ c ^ d;

  fprintf(stderr, "Invalid t value %lu", t);
  exit(EXIT_FAILURE);
}

word k[80];

void initialize()
{
  size_t t = 0;
  for (t = 0; t <= 19; t++)
    k[t] = 0x5A827999;

  for (t = 20; t <= 39; t++)
    k[t] = 0x6ED9EBA1;

  for (t = 40; t <= 59; t++)
    k[t] = 0x8F1BBCDC;

  for (t = 60; t <= 79; t++)
    k[t] = 0xCA62C1D6;
}

word h[5] = {
    0x67452301,
    0xEFCDAB89,
    0x98BADCFE,
    0x10325476,
    0xC3D2E1F0};

byte padded_message[64] = {};

void process_padded_message()
{

  word w[80] = {};
  size_t t = 0;

  for (t = 0; t < 16; t++)
  {
    // Divide M[t] into 16 words (6.1.a)

    for (size_t j = 0; j < 4; j++)
    {
      w[t] |= padded_message[4 * t + j] << (8 * (3 - j));

      printf("%c", padded_message[4 * t + j]);
    }
    // w[t] = *(((word *)padded_message) + t);
    printf(" %ld %02x\n", t, w[t]);
  }

  // 6.1.b
  for (t = 16; t <= 79; t++)
  {
    w[t] = circular_shift(w[t - 3] ^ w[t - 8] ^ w[t - 14] ^ w[t - 16], 1);
  }

  // 6.1.c
  word a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];

  // 6.1.d
  for (t = 0; t <= 79; t++)
  {
    word temp = circular_shift(a, 5) + f(t, b, c, d) + e + w[t] + k[t];
    e = d;
    d = c;
    c = circular_shift(b, 30);
    b = a;
    a = temp;
  }

  // 6.1.e
  h[0] += a;
  h[1] += b;
  h[2] += c;
  h[3] += d;
  h[4] += e;

  memset(padded_message, 0, sizeof(padded_message));
}

void process(byte *message, size_t length)
{
  initialize();

  // cur: messageのうち、現在見ているバイトのindex。messageの参照に利用する。
  // l: RFCにて定義されている、ビット長を表す変数l。messageが8ビットバイトの列であることを仮定すると、8 * バイト長。
  size_t cur = 0, l = 0;

  // need_next_loop: cur == lengthを満たしていてもループを継続したい場合にtrueになる。
  // おもに、バイト長が64の倍数で、次のpadded_messageの先頭に1が格納されることが期待されるような状況で使用する。
  bool need_next_loop = true;

  while (cur < length || need_next_loop)
  {
    // i: padded_messageのうち、現在格納しようとしているindex。
    size_t i = 0;
    for (; cur < length && i < 64; cur++, i++)
    {
      padded_message[i] = message[cur];
      l += 8;
    }

    // もし63番目までに1を格納できるなら格納する。
    // いまの状態は、文字列をすべて格納した・またはiが64・またはその両方
    if (i <= 63)
    {
      // 63番目までに格納可能
      padded_message[i] = 0x80;
      i++;
    }
    else
    {
      // いま i == 64。パディングを終了し、次のpadded_messageでは1を格納できることを期待する
      process_padded_message();
      need_next_loop = true;
      continue;
    }

    // いま1の格納は完了している。
    // 次はlを格納したい。
    if (i >= 57)
    {
      // iはlの格納領域内に入っているため、64までゼロ埋めして次のメッセージにlを格納する。
      for (; i < 64; i++)
      {
        padded_message[i] = 0;
      }
      process_padded_message();
      for (; i < 56; i++)
      {
        padded_message[i] = 0;
      }
      for (size_t j = 0; j < 8; j++)
      {
        padded_message[63 - j] = l >> (j * 8);
      }
      process_padded_message();
      l = 0;
    }
    else
    {
      // 格納できるなら55番目までゼロ埋めしてlを格納。
      for (; i < 56; i++)
      {
        padded_message[i] = 0;
      }
      for (size_t j = 0; j < 8; j++)
      {
        padded_message[63 - j] = l >> (j * 8);
      }
      process_padded_message();
      l = 0;
    }

    need_next_loop = false;
  }

  print_sha1(h);
}

int main(void)
{
  char s[1024];
  puts("Message:");
  fgets(s, sizeof(s), stdin);
  // scanf("%s", s);

  for (int i = 1023; i >= 0; i--)
  {
    if (s[i] == '\n' || s[i] == '\r')
    {
      s[i] = 0;
    }
  }

  puts(s);

  process(s, strlen(s));

  return 0;
}