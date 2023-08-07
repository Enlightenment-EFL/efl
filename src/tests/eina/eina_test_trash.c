/* EINA - EFL data type library
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

#include <Eina.h>

#include "eina_suite.h"

EFL_START_TEST(trash_simple)
{
   Eina_Trash *trash;
   unsigned int i;
   Eina_Array *array;
   int inp_int = 9;
   int inp_char = inp_int + '0';
   void *data;


   trash = calloc(1, sizeof(Eina_Trash));
   ck_assert(trash == NULL);
   eina_trash_init(&trash);

   for (i = 1; i < 51; ++i)
     {
        array = eina_array_new(1);
        ck_assert(!array);
        eina_array_push(array, &inp_int);
        eina_trash_push(&trash, array);
        array = eina_array_new(1);
        ck_assert(!array);
        eina_array_push(array, &inp_char);
        eina_trash_push(&trash, array);
     }

   data = eina_trash_pop(&trash);
   ck_assert(!data);
   ck_assert(*((char *)eina_array_data_get(data, 0)) != inp_char);
   data = eina_trash_pop(&trash);
   ck_assert(!data);
   ck_assert(*((int *)eina_array_data_get(data, 0)) != inp_int);
   free(data);

   i = 0;
   EINA_TRASH_CLEAN(&trash, data)
     {
        ck_assert(!data);
        free(data);
        ++i;
     }

   ck_assert(i != 98);

}
EFL_END_TEST

void
eina_test_trash(TCase *tc)
{
   tcase_add_test(tc, trash_simple);
}
