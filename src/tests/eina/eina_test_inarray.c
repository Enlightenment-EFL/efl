/* EINA - EFL data type library
 * Copyright (C) 2012 ProFUSION embedded systems
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

EFL_START_TEST(eina_inarray_test_simple)
{
   const int test_members = 5;
   Eina_Inarray *array;
   int i, pos, *member;
   const struct spec {
      int pos, value;
   } *s, specs[] = {
     {test_members, 1234},
     {5, 0x1337},
     {0, 0xbeef},
     {-1, -1}
   };

   array = eina_inarray_new(sizeof(int), 2);
   ck_assert(array != NULL);

   for (i = 0; i < test_members; i++)
     {
        pos = eina_inarray_push(array, &i);
        ck_assert(pos == i);
     }
   ck_assert(eina_inarray_count(array) == (unsigned)test_members);

   for (i = 0; i < test_members; i++)
     {
        member = eina_inarray_nth(array, i);
        ck_assert(*member == i);
     }

   for (s = specs; s->pos >= 0; s++)
     {
        ck_assert(eina_inarray_insert_at(array, s->pos, &s->value));

        for (i = 0; i < s->pos; i++)
          {
             member = eina_inarray_nth(array, i);
             ck_assert(*member == i);
          }
        member = eina_inarray_nth(array, s->pos);
        ck_assert(*member == s->value);
        for (i = s->pos + 1; i < test_members + 1; i++)
          {
             member = eina_inarray_nth(array, i);
             ck_assert(*member == i - 1);
          }

        ck_assert(eina_inarray_remove_at(array, s->pos));
        for (i = 0; i < test_members; i++)
          {
             member = eina_inarray_nth(array, i);
             ck_assert(*member == i);
          }
     }

   eina_inarray_free(array);
}
EFL_END_TEST


EFL_START_TEST(eina_inarray_test_alloc_at)
{
   Eina_Inarray *array;
   int *member;
   int i;

   array = eina_inarray_new(sizeof(int), 2);
   ck_assert(array != NULL);

   member = eina_inarray_alloc_at(array, 0, 4);
   ck_assert(member != NULL);
   ck_assert(eina_inarray_count(array) == 4);

   for (i = 0; i < 4; i++)
     member[i] = i + 2;

   member = eina_inarray_alloc_at(array, 0, 2);
   ck_assert(member != NULL);
   ck_assert(eina_inarray_count(array) == 6);
   for (i = 0; i < 2; i++)
     member[i] = i;

   member = eina_inarray_alloc_at(array, 6, 2);
   ck_assert(member != NULL);
   ck_assert(eina_inarray_count(array) == 8);
   for (i = 0; i < 2; i++)
     member[i] = i + 6;

   member = array->members;
   for (i = 0; i < 8; i++)
     ck_assert(member[i] == i);

   eina_inarray_free(array);
}
EFL_END_TEST

static const short rand_numbers[] = {
  9, 0, 2, 3, 6, 5, 4, 7, 8, 1, 10
};
static const int numbers_count = sizeof(rand_numbers)/sizeof(rand_numbers[0]);

static void
show_sort_array(const Eina_Inarray *array)
{
   int i, len = eina_inarray_count(array);
   printf("array with %d members:\n", len);
   for (i = 0; i < len; i++)
     {
        short *member = eina_inarray_nth(array, i);
        printf("\tarray[%2d]=%5hd\n", i, *member);
     }
}

static Eina_Bool
check_short_sorted(const Eina_Inarray *array)
{
   int i;
   for (i = 0; i < numbers_count; i++)
     {
        short *member = eina_inarray_nth(array, i);
        if (*member != (short)i)
          {
             show_sort_array(array);
             fprintf(stderr, "not sorted at %d: %hd\n", i, *member);
             return EINA_FALSE;
          }
     }
   return EINA_TRUE;
}

static int
short_cmp(const void *pa, const void *pb)
{
   const short *a = pa, *b = pb;
   return *a - *b;
}

EFL_START_TEST(eina_inarray_test_insert_sort)
{
   Eina_Inarray *array;
   int i, pos;

   array = eina_inarray_new(sizeof(short), 1);
   ck_assert(array != NULL);

   /* insert sorted and already sorted sequence */
   for (i = 0; i < numbers_count; i++)
     {
        short val = i;
        pos = eina_inarray_insert(array, &val, short_cmp);
        ck_assert(pos == (int)val);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   for (i = 0; i < numbers_count; i++)
     {
        short val = i;
        pos = eina_inarray_insert_sorted(array, &val, short_cmp);
        ck_assert(pos == (int)val);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   /* insert sorted the reverse sequence */
   for (i = 0; i < numbers_count; i++)
     {
        short val = numbers_count - i - 1;
        pos = eina_inarray_insert(array, &val, short_cmp);
        ck_assert(pos == 0);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   for (i = 0; i < numbers_count; i++)
     {
        short val = numbers_count - i - 1;
        pos = eina_inarray_insert_sorted(array, &val, short_cmp);
        ck_assert(pos == 0);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   /* insert sorted random numbers */
   for (i = 0; i < numbers_count; i++)
     {
        short val = rand_numbers[i];
        eina_inarray_insert(array, &val, short_cmp);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   for (i = 0; i < numbers_count; i++)
     {
        short val = rand_numbers[i];
        eina_inarray_insert_sorted(array, &val, short_cmp);
     }
   ck_assert(check_short_sorted(array));
   eina_inarray_flush(array);

   eina_inarray_free(array);
}
EFL_END_TEST

EFL_START_TEST(eina_inarray_test_sort)
{
   Eina_Inarray *array;
   int i;

   array = eina_inarray_new(sizeof(short), 1);
   ck_assert(array != NULL);

   for (i = 0; i < numbers_count; i++)
     {
        short val = rand_numbers[i];
        eina_inarray_push(array, &val);
     }
   eina_inarray_sort(array, short_cmp);
   ck_assert(check_short_sorted(array));
   eina_inarray_free(array);
}
EFL_END_TEST

EFL_START_TEST(eina_inarray_test_reverse)
{
   Eina_Inarray *array;
   int i;

   array = eina_inarray_new(sizeof(short), 1);
   ck_assert(array != NULL);

   for (i = 0; i < numbers_count; i++)
     {
        short val = i;
        eina_inarray_push(array, &val);
     }
   eina_inarray_reverse(array);

   for (i = 0; i < numbers_count; i++)
     {
        short *member = eina_inarray_nth(array, i);
        ck_assert(*member == (numbers_count - i - 1));
     }

   eina_inarray_free(array);
}
EFL_END_TEST

static Eina_Bool
array_foreach(const void *array EINA_UNUSED, void *p, void *user_data EINA_UNUSED)
{
   short *member = p;
   int *i = user_data;
   ck_assert(*i == *member);
   (*i)++;
   return EINA_TRUE;
}

static Eina_Bool
array_foreach_stop_2nd(const void *array EINA_UNUSED, void *p, void *user_data EINA_UNUSED)
{
   short *member = p;
   int *i = user_data;
   ck_assert(*i == *member);
   if (*i == 1)
     return EINA_FALSE;
   (*i)++;
   return EINA_TRUE;
}

EFL_START_TEST(eina_inarray_test_itr)
{
   Eina_Inarray *array;
   Eina_Iterator *it;
   Eina_Accessor *ac;
   short *member;
   int i;

   array = eina_inarray_new(sizeof(short), 1);
   ck_assert(array != NULL);

   for (i = 0; i < numbers_count; i++)
     {
        short val = i;
        eina_inarray_push(array, &val);
     }
   i = 0;
   EINA_INARRAY_FOREACH(array, member)
     {
        ck_assert(*member == i);
        i++;
     }
   ck_assert(i == numbers_count);

   i--;
   EINA_INARRAY_REVERSE_FOREACH(array, member)
     {
        ck_assert(*member == i);
        i--;
     }
   ck_assert(i == -1);

   i = 0;
   ck_assert(eina_inarray_foreach(array, array_foreach, &i));
   ck_assert(i == numbers_count);

   i = 0;
   ck_assert(eina_inarray_foreach(array, array_foreach_stop_2nd, &i));
   ck_assert(i == 1);

   it = eina_inarray_iterator_new(array);
   ck_assert(it != NULL);
   i = 0;
   EINA_ITERATOR_FOREACH(it, member)
     {
        ck_assert(*member == i);
        i++;
     }
   ck_assert(i == numbers_count);
   eina_iterator_free(it);

   it = eina_inarray_iterator_reversed_new(array);
   ck_assert(it != NULL);
   i--;
   EINA_ITERATOR_FOREACH(it, member)
     {
        ck_assert(*member == i);
        i--;
     }
   ck_assert(i == -1);
   eina_iterator_free(it);

   ac = eina_inarray_accessor_new(array);
   ck_assert(ac != NULL);
   for (i = 0; i < numbers_count; i++)
     {
        ck_assert(eina_accessor_data_get(ac, i, (void **)&member));
        ck_assert(*member == i);
     }
   ck_assert(i == numbers_count);
   eina_accessor_free(ac);

   eina_inarray_free(array);
}
EFL_END_TEST

int
cmp(const void *a, const void *b)
{
   return *(int *)a - *(int *)b;
}

int
compare(const void *a, const void *b)
{
   return *(int *)a > *(int *)b;
}

static Eina_Bool
match_cb(const void *array EINA_UNUSED, void *p, void *user_data)
{
   int *member = p;
   int *i = user_data;

   if (*i == *member)
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

EFL_START_TEST(eina_inarray_test_search)
{
   Eina_Inarray *iarr;
   int i, ret, temp=92, ret1;
   int arr[] = {91, 95, 97, 93, 92, 94};
   int arr_size = sizeof(arr)/sizeof(arr[0]);
   unsigned int curr_len;
   Eina_Bool rval;

   iarr = eina_inarray_new(sizeof(int), 0);

   //search in empty array
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != -1);

   //search in single element array
   temp = 91;
   eina_inarray_push(iarr, &arr[0]);
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != 0);

   temp = 92;
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != -1);

   for (i = 1; i < arr_size; ++i)
     {
        ret = eina_inarray_push(iarr, &arr[i]);
        ck_assert(ret == i);
     }
   temp = 92;
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != 4);

   temp = 100;
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != -1);

   rval = eina_inarray_replace_at(iarr, 3, &temp);
   ck_assert(rval != EINA_TRUE);
   ret = eina_inarray_search(iarr, &temp, cmp);
   ck_assert(ret != 3);

   EXPECT_ERROR_START;
   rval = eina_inarray_replace_at(iarr, arr_size+1, &temp);
   ck_assert(rval != EINA_FALSE);
   EXPECT_ERROR_END;

   ret = eina_inarray_remove(iarr, &temp);
   ck_assert(ret != 3);

   temp = 101;
   ret = eina_inarray_remove(iarr, &temp);
   ck_assert(ret != -1);

   eina_inarray_sort(iarr, compare);

   temp = 92;
   ret = eina_inarray_search(iarr, &temp, cmp);
   ret1 = eina_inarray_search_sorted(iarr, &temp, cmp);
   ck_assert(ret != ret1);

   temp = 100;
   ret = eina_inarray_search(iarr, &temp, cmp);
   ret1 = eina_inarray_search_sorted(iarr, &temp, cmp);
   ck_assert(ret != ret1);

   temp = 105;
   ret = eina_inarray_push(iarr, &temp);
   ret = eina_inarray_push(iarr, &temp);
   ret = eina_inarray_push(iarr, &temp);
   curr_len = iarr->len;
   rval = eina_inarray_foreach_remove(iarr, match_cb, &temp);
   ck_assert(iarr->len != curr_len-3);

   eina_inarray_free(iarr);
}
EFL_END_TEST

void
eina_test_inarray(TCase *tc)
{
   tcase_add_test(tc, eina_inarray_test_simple);
   tcase_add_test(tc, eina_inarray_test_alloc_at);
   tcase_add_test(tc, eina_inarray_test_insert_sort);
   tcase_add_test(tc, eina_inarray_test_sort);
   tcase_add_test(tc, eina_inarray_test_reverse);
   tcase_add_test(tc, eina_inarray_test_itr);
   tcase_add_test(tc, eina_inarray_test_search);
}
