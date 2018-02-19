#ifndef __FAKE_PGMSPACE_H_
#define __FAKE_PGMSPACE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "m_printf.h"

#define PGM_P  const char *
#define PRIPSTR "%s"

typedef void prog_void;
typedef char prog_char;
typedef unsigned char prog_uchar;
typedef int8_t prog_int8_t;
typedef uint8_t prog_uint8_t;
typedef int16_t prog_int16_t;
typedef uint16_t prog_uint16_t;
typedef int32_t prog_int32_t;
typedef uint32_t prog_uint32_t;

#define strcat_P(dest, src) strcat((dest), (src))

#ifdef ICACHE_FLASH

#ifndef PROGMEM
#define PROGMEM __attribute__((aligned(4))) __attribute__((section(".irom.text")))
#endif

#define PSTR(s) (__extension__({static const char __c[] PROGMEM = (s); &__c[0];}))

#define pgm_read_with_offset(addr, res) \
  asm("extui    %0, %1, 0, 2\n"     /* Extract offset within word (in bytes) */ \
      "sub      %1, %1, %0\n"       /* Subtract offset from addr, yielding an aligned address */ \
      "l32i.n   %1, %1, 0x0\n"      /* Load word from aligned address */ \
      "slli     %0, %0, 3\n"        /* Multiply offset by 8, yielding an offset in bits */ \
      "ssr      %0\n"               /* Prepare to shift by offset (in bits) */ \
      "srl      %0, %1\n"           /* Shift right; now the requested byte is the first one */ \
      :"=r"(res), "=r"(addr) \
      :"1"(addr) \
      :);

static inline uint8_t pgm_read_byte_inlined(const void* addr) {
  register uint32_t res;
  pgm_read_with_offset(addr, res);
  return (uint8_t) res;     /* This masks the lower byte from the returned word */
}

/* Although this says "word", it's actually 16 bit, i.e. half word on Xtensa */
static inline uint16_t pgm_read_word_inlined(const void* addr) {
  register uint32_t res;
  pgm_read_with_offset(addr, res);
  return (uint16_t) res;    /* This masks the lower half-word from the returned word */
}

// Make sure, that libraries checking existence of this macro are not failing
#define pgm_read_byte(addr) pgm_read_byte_inlined(addr)
#define pgm_read_word(addr) pgm_read_word_inlined(addr)

#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))

void *memcpy_P(void *dest, const void *src_P, size_t length);
size_t strlen_P(const char * src_P);
char *strcpy_P(char * dest, const char * src_P);
char *strncpy_P(char * dest, size_t size, const char * src_P);
int strcmp_P(const char *str1, const char *str2_P);
char *strstr_P(char *haystack, const char *needle_P);
#define sprintf_P(s, f_P, ...) \
	({ \
		int len_P = strlen_P(f_P); \
		int __result=0;char *__localF = (char *)malloc(len_P + 1); \
		if(__localF) { strcpy_P(__localF, f_P); __localF[len_P] = '\0'; \
		__result = m_snprintf(s, len_P, __localF, ##__VA_ARGS__); \
		free(__localF);} \
		__result; \
	})
#define printf_P_heap(f_P, ...) \
	({ \
		char *__localF = (char *)malloc(strlen_P(f_P) + 1); \
		strcpy_P(__localF, (f_P)); \
		int __result = os_printf_plus(__localF, ##__VA_ARGS__); \
		free(__localF); \
		__result; \
	})
#define printf_P_stack(f_P, ...) \
	({ \
		char __localF[256]; \
		/*memset(__localF, 0, sizeof(__localF));*/ \
		m_printf(strncpy_P(__localF, sizeof(__localF), (f_P)), ##__VA_ARGS__); \
	})
#define printf_P printf_P_heap

#else /* ICACHE_FLASH */

#define PROGMEM

#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#define pgm_read_float(addr) (*(const float *)(addr))

#define memcpy_P(dest, src, num) memcpy((dest), (src), (num))
#define strlen_P(a) strlen((a))
#define strcpy_P(dest, src) strcpy((dest), (src))
#define strcmp_P(a, b) strcmp((a), (b))
#define strstr_P(a, b) strstr((a), (b))
#define sprintf_P(s, f, ...) m_sprintf((s), (f), ##__VA_ARGS__)
#define printf_P(f, ...) m_printf((f), ##__VA_ARGS__)

#endif /* ICACHE_FLASH */

#define pgm_read_byte_near(addr) pgm_read_byte(addr)
#define pgm_read_word_near(addr) pgm_read_word(addr)
#define pgm_read_dword_near(addr) pgm_read_dword(addr)
#define pgm_read_float_near(addr) pgm_read_float(addr)
#define pgm_read_byte_far(addr) pgm_read_byte(addr)
#define pgm_read_word_far(addr) pgm_read_word(addr)
#define pgm_read_dword_far(addr) pgm_read_dword(addr)
#define pgm_read_float_far(addr) pgm_read_float(addr)

#ifdef __cplusplus
}
#endif

#endif /* __FAKE_PGMSPACE_H_ */
