#include "WiringFrameworkDependencies.h"
#include "FakePgmSpace.h"

#ifdef ICACHE_FLASH

void* memcpy_P(void* dest, const void* src, size_t count) {
    const uint8_t* read = (const uint8_t*)(src);
    uint8_t* write = (uint8_t*)(dest);

    while (count)
    {
        *write++ = pgm_read_byte(read++);
        count--;
    }

    return dest;
}

size_t strlen_P(const char * src_P)
{
	char val = pgm_read_byte(src_P);
	int len = 0;

	while(val != 0)
	{
		++len; ++src_P;
		val = pgm_read_byte(src_P);
	}

	return len;
}

char *strcpy_P(char * dest, const char * src_P)
{
	for (char *p = dest; *p = pgm_read_byte(src_P++); p++) ;
	return dest;
}

char *strncpy_P(char * dest, size_t max_len, const char * src_P)
{
	int len = strlen_P(src_P);
	if(len >= max_len)
		len = max_len-1;
	memcpy_P(dest, src_P, len);
	dest[len] = 0;
	return dest;
}

int strcmp_P(const char *str1, const char *str2_P)
{
	for (; *str1 == pgm_read_byte(str2_P); str1++, str2_P++)
		if (*str1 == '\0')
			return 0;
	return *(unsigned char *)str1 < (unsigned char)pgm_read_byte(str2_P) ? -1 : 1;
}

char *strstr_P(char *haystack, const char *needle_P)
{
	const char *b = needle_P;
	if (pgm_read_byte(b) == 0)
		return haystack;

	for (; *haystack != 0; haystack++)
	{
		if (*haystack != pgm_read_byte(b))
			continue;

		char *a = haystack;
		while (1)
		{
			char c = pgm_read_byte(b);
			if (c == 0)
				return haystack;

			if (*a != c)
				break;

			a++;
			b++;
		}

		b = needle_P;
	}

	return 0;
}

#endif
