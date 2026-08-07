/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr-side implementations of DiagVprintf() / DiagVprintfNano() for
 * AmebaSmart.
 *
 * The upstream AmebaSmart SDK ships these two functions only in the
 * lib_chipinfo.a prebuilt (an FreeRTOS-side blob), so log.c calls them
 * as external symbols. AmebaDPlus builds them inline in log.c using
 * DiagPutChar = LOGUART_PutChar. We mirror the AmebaDPlus implementation
 * here to satisfy the linker when using the Zephyr HAL glue instead of
 * the vendor prebuilts.
 */

#include "ameba_soc.h"
#include <string.h>

#define get_num_va_args(_args, _lcount)                                   \
	(((_lcount) > 1)  ? va_arg(_args, long long int) :                    \
	 (((_lcount) == 1) ? va_arg(_args, long int) :                        \
	  va_arg(_args, int)))

#define get_unum_va_args(_args, _lcount)                                  \
	(((_lcount) > 1)  ? va_arg(_args, unsigned long long int) :           \
	 (((_lcount) == 1) ? va_arg(_args, unsigned long int) :               \
	  va_arg(_args, unsigned int)))

#define get_char_upper_lower(_upper, _rem)                                \
	(((_rem) < 0xa)   ? ('0' + _rem) :                                    \
	 (((_upper) == 1) ? ('A' + (_rem - 0xa)) :                            \
	  ('a' + (_rem - 0xa))))

#define is_digit(c) ((c >= '0') && (c <= '9'))

static int print_string(const char *str)
{
	int count = 0;

	if (str == NULL) {
		return -1;
	}
	for (; *str != '\0'; str++) {
		DiagPutChar(*str);
		count++;
	}
	return count;
}

static int print_unsigned_num(unsigned long long int unum, unsigned int radix,
			      char padc, int padn, int upper, int sign)
{
	char num_buf[20];
	int i = 0, count = 0;
	unsigned int rem;

	do {
		rem = unum % radix;
		num_buf[i] = get_char_upper_lower(upper, rem);
		i++;
		unum /= radix;
	} while (unum > 0U);

	if (sign) {
		count++;
		padn--;
	}

	if (sign && (padc == '0')) {
		DiagPutChar('-');
	}
	if (padn > 0) {
		while (i < padn) {
			DiagPutChar(padc);
			count++;
			padn--;
		}
	}
	if (sign && (padc == ' ' || padc == '\0')) {
		DiagPutChar('-');
	}
	while (--i >= 0) {
		DiagPutChar(num_buf[i]);
		count++;
	}

	return count;
}

static inline int pad_char_control(int padn, char padc)
{
	int count = 0;
	while (padn > 0) {
		DiagPutChar(padc);
		padn--;
		count++;
	}
	return count;
}

static inline int pad_num_control(unsigned long long int unum, unsigned int radix, char padc,
				  int padn, int upper, int sign, int pad_on_right)
{
	int count = 0;

	if (pad_on_right) {
		int width = print_unsigned_num(unum, radix, padc, 0, upper, sign);
		count += width;
		if ((padn > width) && (padc == ' ')) {
			count += pad_char_control(padn - width, padc);
		}
	} else {
		count += print_unsigned_num(unum, radix, padc, padn, upper, sign);
	}
	return count;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
int DiagVprintf(const char *fmt, va_list args)
{
	int l_count;
	long long int num;
	unsigned long long int unum;
	char *str;

	int count = 0;
	int upper = 0;

	char padc = '\0';
	int padn;
	int pad_on_right;
	int sign = 0;

	char c = '\0';
	int percent_flag = 0;
	char *tempstr = NULL;
	int tempcount = 0;

	while (*fmt != '\0') {
		l_count = 0;
		percent_flag = pad_on_right = padn = sign = 0;
		padc = '\0';

		if (*fmt == '%') {
			percent_flag = 1;
			fmt++;
loop:
			switch (*fmt) {
			case 'i':
			case 'd':
				num = get_num_va_args(args, l_count);
				if (num < 0) {
					sign = 1;
					unum = (unsigned long long int) - num;
				} else {
					unum = (unsigned long long int)num;
				}
				count += pad_num_control(unum, 10, padc, padn, upper, sign, pad_on_right);
				break;
			case 'u':
				unum = get_unum_va_args(args, l_count);
				count += pad_num_control(unum, 10, padc, padn, upper, sign, pad_on_right);
				break;
			case 'B':
			case 'b':
				unum = get_unum_va_args(args, l_count);
				count += pad_num_control(unum, 2, padc, padn, upper, sign, pad_on_right);
				break;
			case 'O':
			case 'o':
				unum = get_unum_va_args(args, l_count);
				count += pad_num_control(unum, 8, padc, padn, upper, sign, pad_on_right);
				break;
			case 'X':
				upper = 1;
			case 'x':
				unum = get_unum_va_args(args, l_count);
				count += pad_num_control(unum, 16, padc, padn, upper, sign, pad_on_right);
				break;
			case 'p':
				upper = 1;
				unum = (uintptr_t)va_arg(args, void *);
				if (sizeof(uintptr_t) == 4U) {
					padc = '0';
					padn = 8;
				} else {
					padc = '0';
					padn = 16;
				}
				count += pad_num_control(unum, 16, padc, padn, upper, sign, pad_on_right);
				break;
			case 's':
				str = va_arg(args, char *);
				tempstr = str;
				tempcount = 0;
				while (*tempstr != '\0') {
					tempstr++;
					tempcount++;
				}
				if (!pad_on_right && (padn - tempcount > 0)) {
					count += pad_char_control(padn - tempcount, padc);
				}
				count += print_string(str);
				if (pad_on_right && (padn - tempcount > 0)) {
					count += pad_char_control(padn - tempcount, padc);
				}
				break;
			case 'c':
			case 'C':
				c = va_arg(args, int);
				if (!pad_on_right && (padn > 1)) {
					count += pad_char_control(padn - 1, padc);
				}
				DiagPutChar(c);
				count++;
				if (pad_on_right && (padn > 1)) {
					count += pad_char_control(padn - 1, padc);
				}
				break;
			case 'z':
				if (sizeof(size_t) == 8U) {
					goto not_support;
				}
				fmt++;
				goto loop;
			case 'L':
			case 'l':
				l_count++;
				fmt++;
				goto loop;
			case '-':
				fmt++;
				pad_on_right++;
				goto loop;
			case '%':
				if (percent_flag) {
					DiagPutChar('%');
				}
				count++;
				break;
			case '0':
				padc = '0';
				padn = 0;
				fmt++;
				goto pad_count;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				padc = ' ';
				padn = 0;
				goto pad_count;
			case ' ':
				padc = ' ';
				padn = 0;
				fmt++;
pad_count:
				for (;;) {
					char ch = *fmt;
					if (ch == '\0') {
						goto exit;
					} else if (!is_digit(ch)) {
						goto loop;
					}
					padn = (padn * 10) + (ch - '0');
					fmt++;
				}
			case '\0':
				break;
			default:
not_support:
				return -1;
			}
			fmt++;
			continue;
		}
		DiagPutChar(*fmt);
		fmt++;
		count++;
	}
exit:
	return count;
}

static inline int print_decimal_num(unsigned int unum, int radix, char padc, int padn, int sign)
{
	char num_buf[10];
	int i = 0, count = 0;
	unsigned int rem;

	do {
		rem = unum % radix;
		num_buf[i] = get_char_upper_lower(0, rem);
		i++;
		unum /= radix;
	} while (unum > 0U);

	if (sign) {
		count++;
		padn--;
	}

	if (sign && (padc == '0')) {
		DiagPutChar('-');
	}
	if (padn > 0) {
		while (i < padn) {
			DiagPutChar(padc);
			count++;
			padn--;
		}
	}
	if (sign && (padc == ' ' || padc == '\0')) {
		DiagPutChar('-');
	}
	while (--i >= 0) {
		DiagPutChar(num_buf[i]);
		count++;
	}

	return count;
}

int DiagVprintfNano(const char *fmt, va_list args)
{
	char *str;

	int count = 0;
	int sign = 0, num = 0;

	char padc = '\0';
	int padn;
	int pad_on_right;

	char c = '\0';
	int percent_flag = 0;
	char *tempstr = NULL;
	int tempcount = 0;

	unsigned int unum;
	while (*fmt != '\0') {
		percent_flag = pad_on_right = padn = 0;
		padc = '\0';
		sign = 0;

		if (*fmt == '%') {
			percent_flag = 1;
			fmt++;
loop:
			switch (*fmt) {
			case 'i':
			case 'd':
				num = get_num_va_args(args, 0);
				if (num < 0) {
					sign = 1;
					unum = (unsigned int) - num;
				} else {
					unum = (unsigned int)num;
				}
				count += print_decimal_num(unum, 10, padc, padn, sign);
				break;
			case 'u':
				unum = (unsigned int)get_unum_va_args(args, 0);
				count += print_decimal_num(unum, 10, padc, padn, 0);
				break;
			case 'x':
				unum = (unsigned int)get_unum_va_args(args, 0);
				count += print_decimal_num(unum, 16, padc, padn, 0);
				break;
			case 's':
				str = va_arg(args, char *);
				tempstr = str;
				tempcount = 0;
				while (*tempstr != '\0') {
					tempstr++;
					tempcount++;
				}
				if (!pad_on_right && (padn - tempcount > 0)) {
					count += pad_char_control(padn - tempcount, padc);
				}
				count += print_string(str);
				if (pad_on_right && (padn - tempcount > 0)) {
					count += pad_char_control(padn - tempcount, padc);
				}
				break;
			case 'c':
			case 'C':
				c = va_arg(args, int);
				if (!pad_on_right && (padn > 1)) {
					count += pad_char_control(padn - 1, padc);
				}
				DiagPutChar(c);
				count++;
				if (pad_on_right && (padn > 1)) {
					count += pad_char_control(padn - 1, padc);
				}
				break;
			case '%':
				if (percent_flag) {
					DiagPutChar('%');
				}
				count++;
				break;
			case '0':
				padc = '0';
				padn = 0;
				fmt++;
				goto pad_count;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				padc = ' ';
				padn = 0;
				goto pad_count;
			case ' ':
				padc = ' ';
				padn = 0;
				fmt++;
pad_count:
				for (;;) {
					char ch = *fmt;
					if (ch == '\0') {
						return count;
					} else if (!is_digit(ch)) {
						goto loop;
					}
					padn = (padn * 10) + (ch - '0');
					fmt++;
				}
			case '\0':
				break;
			default:
				return -1;
			}
			fmt++;
			continue;
		}
		DiagPutChar(*fmt);
		fmt++;
		count++;
	}

	return count;
}
#pragma GCC diagnostic pop
