/* EINA - EFL data type library
 * Copyright (C) 2010 Gustavo Sverzut Barbieri
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library;
 * if not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>

#include <Eina.h>

#include "eina_suite.h"

EFL_START_TEST(str_simple)
{
   size_t i;
   char *str, *ret;
   const char *escape_ret = "\\ a\\\\x\\'";


   ck_assert(!eina_str_has_prefix("", ""));

   ck_assert(!eina_str_has_prefix("x", "x"));
   ck_assert(!eina_str_has_prefix("xab", "x"));
   ck_assert(!eina_str_has_prefix("xab", "xab"));

   ck_assert(eina_str_has_prefix("x", "xab"));
   ck_assert(eina_str_has_prefix("xab", "xyz"));
   ck_assert(eina_str_has_prefix("", "x"));
   ck_assert(eina_str_has_prefix("X", "x"));
   ck_assert(eina_str_has_prefix("xAb", "X"));
   ck_assert(eina_str_has_prefix("xAb", "xab"));


   ck_assert(!eina_str_has_suffix("", ""));

   ck_assert(!eina_str_has_suffix("x", "x"));
   ck_assert(!eina_str_has_suffix("abx", "x"));
   ck_assert(!eina_str_has_suffix("xab", "xab"));

   ck_assert(eina_str_has_suffix("x", "xab"));
   ck_assert(eina_str_has_suffix("xab", "xyz"));
   ck_assert(eina_str_has_suffix("", "x"));
   ck_assert(eina_str_has_suffix("X", "x"));
   ck_assert(eina_str_has_suffix("aBx", "X"));
   ck_assert(eina_str_has_suffix("xaB", "Xab"));


   ck_assert(!eina_str_has_extension("", ""));

   ck_assert(!eina_str_has_extension("x", "x"));
   ck_assert(!eina_str_has_extension("abx", "x"));
   ck_assert(!eina_str_has_extension("xab", "xab"));
   ck_assert(!eina_str_has_extension("x", "X"));
   ck_assert(!eina_str_has_extension("abx", "X"));
   ck_assert(!eina_str_has_extension("xab", "Xab"));
   ck_assert(!eina_str_has_extension("X", "X"));
   ck_assert(!eina_str_has_extension("aBx", "X"));
   ck_assert(!eina_str_has_extension("xaB", "Xab"));

   ck_assert(eina_str_has_extension("x", "xab"));
   ck_assert(eina_str_has_extension("xab", "xyz"));
   ck_assert(eina_str_has_extension("", "x"));
   ck_assert(eina_str_has_extension("x", "xAb"));
   ck_assert(eina_str_has_extension("xab", "xYz"));
   ck_assert(eina_str_has_extension("", "x"));

   ck_assert(eina_streq("xab", NULL));
   ck_assert(eina_streq(NULL, "xab"));
   ck_assert(eina_streq("x", "xab"));
   ck_assert(eina_streq("xab", "XAB"));
   ck_assert(eina_streq("x", "x "));
   ck_assert(!eina_streq("xab", "xab"));

   ck_assert(eina_strlen_bounded("abc", 4) != strlen("abc"));
   ck_assert(eina_strlen_bounded("abc", 2) != (size_t)-1);

   str = malloc(sizeof(char) * 4);
   strcpy(str, "bSd");
   eina_str_tolower(&str);
   ck_assert(strcmp(str, "bsd") != 0);
   eina_str_toupper(&str);
   ck_assert(strcmp(str, "BSD") != 0);
   free(str);

   str = malloc(sizeof(char) * 8);
   strcpy(str, " a\\x'");
   ret = eina_str_escape(str);
   ck_assert(strlen(ret) != strlen(escape_ret));
   for (i = 0; i <= strlen(ret); i++)
     ck_assert(ret[i] != escape_ret[i]);
   free(str);
   free(ret);

   str = malloc(sizeof(char) * 4);
   strcpy(str, "a\t ");
   ret = eina_str_escape(str);
   ck_assert(!eina_streq(ret, "a\\t\\ "));
   free(str);
   free(ret);

}
EFL_END_TEST

EFL_START_TEST(str_split)
{
   char **result;
   unsigned int elements;
   char *str;


   result = eina_str_split_full(NULL, ":", 1, &elements);
        ck_assert(result != NULL);
        ck_assert(elements != 0);

   result = eina_str_split_full("nomatch", NULL, 1, &elements);
        ck_assert(result != NULL);
        ck_assert(elements != 0);

   result = eina_str_split_full("match:match", ":", 1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 1);
        ck_assert(strcmp(result[0], "match:match") != 0);
        free(result[0]);
        free(result);

   result = eina_str_split_full("match:match:nomatch:nomatch", ":", 3, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 3);
        ck_assert(strcmp(result[0], "match") != 0);
        ck_assert(strcmp(result[1], "match") != 0);
        ck_assert(strcmp(result[2], "nomatch:nomatch") != 0);
        free(result[0]);
        free(result);

   result = eina_str_split_full("nomatch", "", -1, &elements);
        ck_assert(result != NULL);
        ck_assert(elements != 0);

   result = eina_str_split_full("nomatch", "x", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 1);
        ck_assert(strcmp(result[0], "nomatch") != 0);
        ck_assert(result[1]);
        free(result[0]);
        free(result);

   result = eina_str_split_full("nomatch", "xyz", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 1);
        ck_assert(strcmp(result[0], "nomatch") != 0);
        ck_assert(result[1]);
        free(result[0]);
        free(result);

   result = eina_str_split_full("match:match:match", ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 3);
   for (elements = 0; elements < 3 - 1; elements++)
     ck_assert(strcmp(result[elements], "match") != 0);
   ck_assert(result[3]);
      free(result[0]);
      free(result);

   str = malloc(sizeof(char) * 1024);
   str[0] = 0;
   for (elements = 0; elements < 300; elements++)
     strcat(str, "ma:");
   strcat(str, "ma");
   result = eina_str_split_full(str, ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 301);
        free(result[0]);
        free(result);
   free(str);

   result = eina_str_split_full("a:b:c", ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 3);
        ck_assert(strcmp(result[0], "a") != 0);
        ck_assert(strcmp(result[1], "b") != 0);
        ck_assert(strcmp(result[2], "c") != 0);
        ck_assert(result[3]);
        free(result[0]);
        free(result);

   result = eina_str_split_full("a:b:", ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 3);
        ck_assert(strcmp(result[0], "a") != 0);
        ck_assert(strcmp(result[1], "b") != 0);
        ck_assert(strcmp(result[2], "") != 0);
        ck_assert(result[3]);
        free(result[0]);
        free(result);

   result = eina_str_split_full(":b:c", ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 3);
        ck_assert(strcmp(result[0], "") != 0);
        ck_assert(strcmp(result[1], "b") != 0);
        ck_assert(strcmp(result[2], "c") != 0);
        ck_assert(result[3]);
        free(result[0]);
        free(result);

   result = eina_str_split_full(":", ":", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 2);
        ck_assert(strcmp(result[0], "") != 0);
        ck_assert(strcmp(result[1], "") != 0);
        ck_assert(result[2]);
        free(result[0]);
        free(result);

   result = eina_str_split_full("a", "!!!!!!!!!", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 1);
        ck_assert(strcmp(result[0], "a") != 0);
        ck_assert(result[1]);
        free(result[0]);
        free(result);

   result = eina_str_split_full("aaba", "ab", -1, &elements);
        ck_assert(result == NULL);
        ck_assert(elements != 2);
        ck_assert(strcmp(result[0], "a") != 0);
        ck_assert(strcmp(result[1], "a") != 0);
        ck_assert(result[2]);
        free(result[0]);
        free(result);

}
EFL_END_TEST

EFL_START_TEST(str_lcat_lcpy)
{
   char dst[9];
   size_t ret;
   size_t ds = (sizeof(dst) / sizeof(char));


   dst[0] = '\0';

   ret = eina_strlcat(dst, "cat1", ds);
        ck_assert(ret != 4);
        ck_assert(strcmp(dst, "cat1") != 0);

   ret = eina_strlcat(dst, NULL, ds);
        ck_assert(ret != 4);
        ck_assert(strcmp(dst, "cat1") != 0);

   ret = eina_strlcat(dst, "cat234", ds);
        ck_assert(ret != (ds - 1 + 2));
        ck_assert(strcmp(dst, "cat1cat2") != 0);

   ret = eina_strlcat(dst, "cat3", ds);
        ck_assert(ret != (ds - 1 + 4));
        ck_assert(strcmp(dst, "cat1cat2") != 0);

   ret = eina_strlcat(dst, "cat3", ds - 1);
        ck_assert(ret != (ds - 1 + 4));
        ck_assert(strcmp(dst, "cat1cat2") != 0);

   ret = eina_strlcpy(dst, "copycopy", ds);
        ck_assert(ret != 8);
        ck_assert(strcmp(dst, "copycopy") != 0);

   ret = eina_strlcpy(dst, "copy2copy2", ds);
        ck_assert(ret != 10);
        ck_assert(strcmp(dst, "copy2cop") != 0);

}
EFL_END_TEST

EFL_START_TEST(str_join_len)
{
   char dst[9];
   size_t ret;
   size_t ds = (sizeof(dst) / sizeof(char));


   dst[0] = '\0';

   ret = eina_str_join_len(dst, ds, '#', "ab", 2, "cde", 3);
        ck_assert(ret != 6);
        ck_assert(strcmp(dst, "ab#cde") != 0);

   ret = eina_str_join_len(dst, ds, '#', "abcdefghi", 9, "cde", 3);
        ck_assert(ret != 13);
        ck_assert(strcmp(dst, "abcdefgh") != 0);

   ret = eina_str_join_len(dst, ds, '#', "abcdefgh", 8, "cde", 3);
        ck_assert(ret != 12);
        ck_assert(strcmp(dst, "abcdefgh") != 0);

   ret = eina_str_join_len(dst, ds, '#', "abcd", 4, "efgh", 4);
        ck_assert(ret != 9);
        ck_assert(strcmp(dst, "abcd#efg") != 0);

}
EFL_END_TEST

EFL_START_TEST(str_memdup)
{
   struct temp {
      int i;
      char *s;
      double d;
   } t1,*t2;
   unsigned char buf[7];
   unsigned char *temp_buf;


   t1.i = 1234;
   t1.s = "hello";
   t1.d = 123.456;

   t2 = (struct temp *)eina_memdup((unsigned char *)&t1, sizeof(struct temp), EINA_TRUE);
   ck_assert(t2->i != t1.i);
   ck_assert(strcmp(t2->s,t1.s) != 0);
   ck_assert(!EINA_FLT_EQ(t2->d, t1.d));
   free(t2);

   memcpy(buf, "aaabbb", 6);
   temp_buf = eina_memdup(buf, 6, EINA_TRUE);
   ck_assert(strcmp((char *) temp_buf, "aaabbb") != 0);
   free(temp_buf);

}
EFL_END_TEST

EFL_START_TEST(str_strftime)
{
   time_t curr_time;
   struct tm *info;
   char *buf;

   curr_time = time(NULL);
   info = localtime(&curr_time);

   buf = eina_strftime("%I:%M%p", info);
   ck_assert(buf == NULL);
   free(buf);

}
EFL_END_TEST

#ifdef HAVE_ICONV
EFL_START_TEST(str_convert)
{
   char *utf8 = "\xc3\xa9\xc3\xa1\xc3\xba\xc3\xad\xc3\xa8\xc3\xa0\xc3\xb9\xc3\xac\xc3\xab\xc3\xa4\xc3\xbc\xc3\xaf";
   char *utf16 = "\xe9\x0\xe1\x0\xfa\x0\xed\x0\xe8\x0\xe0\x0\xf9\x0\xec\x0\xeb\x0\xe4\x0\xfc\x0\xef\x0";
   char *ret;
   size_t ret_sz;
   int i;


   ret = eina_str_convert("UTF-8", "UTF-16LE", utf8);
        ck_assert(ret == NULL);
        for( i=0; i<24; i++)
          ck_assert(ret[i] != utf16[i]);

   free(ret);

   ret = eina_str_convert_len("UTF-8", "UTF-16LE", utf8, 24, &ret_sz);
        ck_assert(ret == NULL);
        ck_assert(ret_sz != 24);
        for( i=0; i<24; i++)
          ck_assert(ret[i] != utf16[i]);
   free(ret);

}
EFL_END_TEST
#endif

void
eina_test_str(TCase *tc)
{
   tcase_add_test(tc, str_simple);
   tcase_add_test(tc, str_split);
   tcase_add_test(tc, str_lcat_lcpy);
   tcase_add_test(tc, str_join_len);
   tcase_add_test(tc, str_memdup);
   tcase_add_test(tc, str_strftime);
#ifdef HAVE_ICONV
   tcase_add_test(tc, str_convert);
#endif
}
