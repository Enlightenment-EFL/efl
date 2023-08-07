/* EINA - EFL data type library
 * Copyright (C) 2016 ProFUSION embedded systems
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

EFL_START_TEST(eina_test_slice_init)
{
   Eina_Rw_Slice a = EINA_SLICE_STR_LITERAL("hello world");
   char buf[1024] = "";
   Eina_Rw_Slice b = EINA_SLICE_ARRAY(buf);
   Eina_Rw_Slice c = EINA_SLICE_STR("hi there");
   EINA_RW_SLICE_DECLARE(d, 512);


   ck_assert(a.len == sizeof("hello world") - 1);
   ck_assert(strcmp(a.mem, "hello world") == 0);

   ck_assert(b.len == sizeof(buf));

   ck_assert(c.len == strlen("hi there"));
   ck_assert(strcmp(c.mem, "hi there") == 0);

   ck_assert(d.len == 512);

}
EFL_END_TEST

EFL_START_TEST(eina_test_slice_ro)
{
   Eina_Slice slice = EINA_SLICE_STR_LITERAL("hi there");
   Eina_Slice a, needle;
   Eina_Rw_Slice dup;
   Eina_Bool r;
   const void *p;
   char *str;

   dup = eina_slice_dup(slice);
   ck_assert(dup.len == slice.len);
   ck_assert(dup.mem != slice.mem);
   ck_assert(eina_slice_compare(eina_rw_slice_slice_get(dup), slice) == 0);
   free(dup.mem);

   str = eina_slice_strdup(slice);
   ck_assert(str != NULL);
   ck_assert(strcmp(str, "hi there") == 0);
   free(str);

   a = eina_slice_seek(slice, strlen("hi "), SEEK_SET);
   ck_assert(a.mem == slice.bytes + strlen("hi "));
   ck_assert(a.len == slice.len - strlen("hi "));

   a = eina_slice_seek(slice, 0, SEEK_SET);
   ck_assert(a.mem == slice.bytes + 0);
   ck_assert(a.len == slice.len);

   a = eina_slice_seek(slice, -1, SEEK_END);
   ck_assert(a.mem == slice.bytes + slice.len - 1);
   ck_assert(a.len == 1);

   a = eina_slice_seek(slice, 0, SEEK_END);
   ck_assert(a.mem == eina_slice_end_get(slice));
   ck_assert(a.len == 0);

   p = eina_slice_end_get(slice);
   ck_assert(p == slice.bytes + slice.len);

   slice = (Eina_Slice)EINA_SLICE_STR_LITERAL("HELLO WORLD");
   slice.len = strlen("hi there"); /* crop... */

   p = eina_slice_strchr(slice, ' ');
   ck_assert(p == slice.bytes + 5); /* 5 = index of ' ' in HELLO WORLD.. */

   p = eina_slice_strchr(slice, '!');
   ck_assert(p == NULL);

   needle = (Eina_Slice)EINA_SLICE_STR_LITERAL(" W");
   p = eina_slice_find(slice, needle);
   ck_assert(p == slice.bytes + 5); /* 5 = index of ' W' in HELLO WORLD..*/

   needle = (Eina_Slice)EINA_SLICE_STR_LITERAL("LO");
   p = eina_slice_find(slice, needle);
   ck_assert(p == slice.bytes + 3); /* 3 = index of 'LO' in HELLO WORLD..*/

   needle = (Eina_Slice)EINA_SLICE_STR_LITERAL("HELLO");
   p = eina_slice_find(slice, needle);
   ck_assert(p == slice.mem);

   needle = (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"); /* would go out of boundaries */
   p = eina_slice_find(slice, needle);
   ck_assert(p == NULL);

   r = eina_slice_startswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("HEL"));
   ck_assert(r == EINA_TRUE);

   r = eina_slice_startswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"));
   ck_assert(r == EINA_FALSE);

   r = eina_slice_startswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL(""));
   ck_assert(r == EINA_FALSE);

   r = eina_slice_endswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WO"));
   ck_assert(r == EINA_TRUE);

   r = eina_slice_endswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"));
   ck_assert(r == EINA_FALSE);

   r = eina_slice_endswith(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL(""));
   ck_assert(r == EINA_FALSE);
}
EFL_END_TEST

EFL_START_TEST(eina_test_slice_rw)
{
   uint8_t buf[] = "hi there";
   Eina_Rw_Slice rw_slice = EINA_SLICE_ARRAY(buf);
   Eina_Slice ro_slice;
   Eina_Rw_Slice a;
   Eina_Bool r;
   const void *p;
   char *str;

   buf[sizeof(buf) - 1] = 0xff;
   rw_slice.len--; /* do not account \0 (now 0xff) */
   ro_slice = eina_rw_slice_slice_get(rw_slice);

   ck_assert(rw_slice.len == ro_slice.len);
   ck_assert(rw_slice.mem == ro_slice.mem);

   a = eina_slice_dup(ro_slice);
   ck_assert(a.len == ro_slice.len);
   ck_assert(a.mem != ro_slice.mem);
   ck_assert(eina_rw_slice_compare(a, rw_slice) == 0);
   free(a.mem);

   str = eina_rw_slice_strdup(rw_slice);
   ck_assert(str != NULL);
   ck_assert(strcmp(str, "hi there") == 0);
   free(str);

   a = eina_rw_slice_seek(rw_slice, strlen("hi "), SEEK_SET);
   ck_assert(a.mem == rw_slice.bytes + strlen("hi "));
   ck_assert(a.len == rw_slice.len - strlen("hi "));

   a = eina_rw_slice_seek(rw_slice, 0, SEEK_SET);
   ck_assert(a.mem == rw_slice.bytes + 0);
   ck_assert(a.len == rw_slice.len);

   a = eina_rw_slice_seek(rw_slice, -1, SEEK_END);
   ck_assert(a.mem == rw_slice.bytes + rw_slice.len - 1);
   ck_assert(a.len == 1);

   a = eina_rw_slice_seek(rw_slice, 0, SEEK_END);
   ck_assert(a.mem == eina_rw_slice_end_get(rw_slice));
   ck_assert(a.len == 0);

   p = eina_rw_slice_end_get(rw_slice);
   ck_assert(p == rw_slice.bytes + rw_slice.len);

   ro_slice = (Eina_Slice)EINA_SLICE_STR_LITERAL("HELLO WORLD, big string to be cropped");
   a = eina_rw_slice_copy(rw_slice, ro_slice);

   ck_assert(a.mem == rw_slice.mem);
   ck_assert(a.len == rw_slice.len);
   ck_assert(strncmp(a.mem, "HELLO WO", a.len) == 0);

   p = eina_rw_slice_strchr(rw_slice, ' ');
   ck_assert(p == rw_slice.bytes + 5); /* 5 = index of ' ' in HELLO WORLD.. */

   p = eina_rw_slice_strchr(rw_slice, '!');
   ck_assert(p == NULL);

   ro_slice = (Eina_Slice)EINA_SLICE_STR_LITERAL(" W");
   p = eina_rw_slice_find(rw_slice, ro_slice);
   ck_assert(p == rw_slice.bytes + 5); /* 5 = index of ' W' in HELLO WORLD..*/

   ro_slice = (Eina_Slice)EINA_SLICE_STR_LITERAL("LO");
   p = eina_rw_slice_find(rw_slice, ro_slice);
   ck_assert(p == rw_slice.bytes + 3); /* 3 = index of 'LO' in HELLO WORLD..*/

   ro_slice = (Eina_Slice)EINA_SLICE_STR_LITERAL("HELLO");
   p = eina_rw_slice_find(rw_slice, ro_slice);
   ck_assert(p == rw_slice.mem);

   ro_slice = (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"); /* would go out of boundaries */
   p = eina_rw_slice_find(rw_slice, ro_slice);
   ck_assert(p == NULL);

   ck_assert(buf[sizeof(buf) - 1] == 0xff);

   r = eina_rw_slice_startswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("HEL"));
   ck_assert(r == EINA_TRUE);

   r = eina_rw_slice_startswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"));
   ck_assert(r == EINA_FALSE);

   r = eina_rw_slice_startswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL(""));
   ck_assert(r == EINA_FALSE);

   r = eina_rw_slice_endswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WO"));
   ck_assert(r == EINA_TRUE);

   r = eina_rw_slice_endswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("WORLD"));
   ck_assert(r == EINA_FALSE);

   r = eina_rw_slice_endswith(rw_slice, (Eina_Slice)EINA_SLICE_STR_LITERAL(""));
   ck_assert(r == EINA_FALSE);
}
EFL_END_TEST

EFL_START_TEST(eina_test_slice_print)
{
   char buf[] = "Hello World";
   Eina_Slice slice = {.len = strlen("Hello"), .mem = buf};
   char a[128], b[128];

   snprintf(a, sizeof(a), EINA_SLICE_FMT, EINA_SLICE_PRINT(slice));
   snprintf(b, sizeof(b), "%p+%zu", slice.mem, slice.len);
   ck_assert(strcmp(a, b) == 0);

   snprintf(a, sizeof(a), EINA_SLICE_STR_FMT, EINA_SLICE_STR_PRINT(slice));
   ck_assert(strcmp(a, "Hello") == 0);
}
EFL_END_TEST

EFL_START_TEST(eina_test_slice_find)
{
    Eina_Slice slice = EINA_SLICE_STR_LITERAL("abcdef");
    const char *p;

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("a"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("ab"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("abc"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("f"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes + strlen("abcde"));

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("ef"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes + strlen("abcd"));

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("def"));
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes + strlen("abc"));

    p = eina_slice_find(slice, slice);
    ck_assert(p == NULL);
    ck_assert_str_eq(p, (const char *)slice.bytes);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("x"));
    ck_assert(p == NULL);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("xyz"));
    ck_assert(p == NULL);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("xa"));
    ck_assert(p == NULL);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("xb"));
    ck_assert(p == NULL);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL("abcdefgh"));
    ck_assert(p == NULL);

    p = eina_slice_find(slice, (Eina_Slice)EINA_SLICE_STR_LITERAL(""));
    ck_assert(p == NULL);
}
EFL_END_TEST

void
eina_test_slice(TCase *tc)
{
   tcase_add_test(tc, eina_test_slice_init);
   tcase_add_test(tc, eina_test_slice_ro);
   tcase_add_test(tc, eina_test_slice_rw);
   tcase_add_test(tc, eina_test_slice_print);
   tcase_add_test(tc, eina_test_slice_find);
}
