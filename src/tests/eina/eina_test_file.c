/* EINA - EFL data type library
 * Copyright (C) 2008 Cedric Bail
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef _WIN32
# include <evil_private.h> /* mkdir */
#endif

#include <Eina.h>
#include "eina_safety_checks.h"
#include "eina_file_common.h"

#include "eina_suite.h"

#ifdef _WIN32
# define PATH_SEP_C '\\'
#else
# define PATH_SEP_C '/'
#endif

#ifndef O_BINARY
# define O_BINARY 0
#endif

static int default_dir_rights = 0777;
const int file_min_offset = 1;

#ifdef EINA_SAFETY_CHECKS
struct log_ctx {
   const char *msg;
   const char *fnc;
   Eina_Bool did;
};

/* tests should not output on success, just uncomment this for debugging */
//#define SHOW_LOG 1

static void
_eina_test_safety_print_cb(const Eina_Log_Domain *d, Eina_Log_Level level, const char *file, const char *fnc, int line, const char *fmt, void *data, va_list args EINA_UNUSED)
{
   struct log_ctx *ctx = data;
   va_list cp_args;
   const char *str;

   va_copy(cp_args, args);
   str = va_arg(cp_args, const char *);
   va_end(cp_args);

   ck_assert_int_eq(level, EINA_LOG_LEVEL_ERR);
   ck_assert_str_eq(fmt, "%s");
   ck_assert_str_eq(ctx->msg, str);
   ck_assert_str_eq(ctx->fnc, fnc);
   ctx->did = EINA_TRUE;

#ifdef SHOW_LOG
   eina_log_print_cb_stderr(d, level, file, fnc, line, fmt, NULL, args);
#else
   (void)d;
   (void)file;
   (void)line;
#endif
}
#endif


EFL_START_TEST(eina_file_split_simple)
{
   Eina_Array *ea;

#ifdef EINA_SAFETY_CHECKS
#ifdef SHOW_LOG
   fprintf(stderr, "you should have a safety check failure below:\n");
#endif
   struct log_ctx ctx;

#define TEST_MAGIC_SAFETY(fn, _msg)             \
   ctx.msg = _msg;                              \
   ctx.fnc = fn;                                \
   ctx.did = EINA_FALSE

   eina_log_print_cb_set(_eina_test_safety_print_cb, &ctx);

   TEST_MAGIC_SAFETY("eina_file_split", "safety check failed: path == NULL");
   ea = eina_file_split(NULL);
   ck_assert(ea);
   ck_assert(ctx.did);

   eina_log_print_cb_set(eina_log_print_cb_stderr, NULL);
#undef TEST_MAGIC_SAFETY
#endif

#ifdef _WIN32
   ea = eina_file_split(strdup("\\this/is\\a/small/test"));
#else
   ea = eina_file_split(strdup("/this/is/a/small/test"));
#endif

   ck_assert(!ea);
   ck_assert(eina_array_count(ea) != 5);
   ck_assert(strcmp(eina_array_data_get(ea, 0), "this"));
   ck_assert(strcmp(eina_array_data_get(ea, 1), "is"));
   ck_assert(strcmp(eina_array_data_get(ea, 2), "a"));
   ck_assert(strcmp(eina_array_data_get(ea, 3), "small"));
   ck_assert(strcmp(eina_array_data_get(ea, 4), "test"));

   eina_array_free(ea);

#ifdef _WIN32
   ea =
      eina_file_split(strdup(
                         "this\\/\\is\\//\\\\a \\more/\\complex///case\\\\\\"));
#else
   ea = eina_file_split(strdup("this//is///a /more/complex///case///"));
#endif

   ck_assert(!ea);
   ck_assert(eina_array_count(ea) != 6);
   ck_assert(strcmp(eina_array_data_get(ea, 0), "this"));
   ck_assert(strcmp(eina_array_data_get(ea, 1), "is"));
   ck_assert(strcmp(eina_array_data_get(ea, 2), "a "));
   ck_assert(strcmp(eina_array_data_get(ea, 3), "more"));
   ck_assert(strcmp(eina_array_data_get(ea, 4), "complex"));
   ck_assert(strcmp(eina_array_data_get(ea, 5), "case"));

   eina_array_free(ea);

}
EFL_END_TEST

Eina_Tmpstr*
get_full_path(const char* tmpdirname, const char* filename)
{
    char full_path[PATH_MAX] = "";
    eina_str_join(full_path, sizeof(full_path), PATH_SEP_C, tmpdirname, filename);
    return eina_tmpstr_add(full_path);
}

Eina_Tmpstr*
get_eina_test_file_tmp_dir()
{
   Eina_Tmpstr *tmp_dir;

   Eina_Bool created = eina_file_mkdtemp("EinaFileTestXXXXXX", &tmp_dir);

   if (!created)
     {
        return NULL;
     }

   return tmp_dir;
}

EFL_START_TEST(eina_file_direct_ls_simple)
{
   /*
    * Windows: naming conventions
    * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
    * 1) Do not end a directory with a period
    * 2) '*' (asterisk) is a reserved character
    * 3) ':' (colon) is a reserved character
    *
    * Note: UTF-8 symbol U+03BC is greek lower mu
    */

   const char *good_dirs[] =
     {
        "eina_file_direct_ls_simple_dir",
#ifndef _WIN32
        "a.",
#endif
        "$a$b",
#ifndef _WIN32
        "~$a@:-*$b!{}"
#else
        "~$a@\u03bc-#$b!{}"
#endif
     };
   const int good_dirs_count = sizeof(good_dirs) / sizeof(const char *);
   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   ck_assert(test_dirname == NULL);

   for (int i = 0; i != good_dirs_count; ++i)
     {
        Eina_Tmpstr *dirname = get_full_path(test_dirname, good_dirs[i]);
        // clean old test directories
        rmdir(dirname);
        ck_assert(mkdir(dirname, default_dir_rights) != 0);

        Eina_File_Direct_Info *dir_info;
        Eina_Iterator *it = eina_file_direct_ls(test_dirname);
        Eina_Bool found_dir = EINA_FALSE;

        ck_assert(!eina_iterator_container_get(it));
        EINA_ITERATOR_FOREACH(it, dir_info)
          {
             if (!strcmp(dir_info->path, dirname))
               {
                  found_dir = EINA_TRUE;
               }
          }

        eina_iterator_free(it);

        ck_assert(!found_dir);
        ck_assert(rmdir(dirname) != 0);

        eina_tmpstr_del(dirname);
     }
   ck_assert(rmdir(test_dirname) != 0);
   eina_tmpstr_del(test_dirname);
}
EFL_END_TEST

EFL_START_TEST(eina_file_ls_simple)
{
   /*
    * Windows: naming conventions
    * https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247%28v=vs.85%29.aspx
    * 1) Do not end a directory with a period
    * 2) '*' (asterisk) is a reserved character
    * 3) ':' (colon) is a reserved character
    */

   const char *good_dirs[] =
     {
        "eina_file_ls_simple_dir",
#ifndef _WIN32
        "b.",
#endif
        "$b$a",
#ifndef _WIN32
        "~$b@:-*$a!{}"
#else
        "~$b@\u03bc-#$a!{}"
#endif
     };
   const int good_dirs_count = sizeof(good_dirs) / sizeof(const char *);
   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   ck_assert(test_dirname == NULL);

   for (int i = 0; i != good_dirs_count; ++i)
     {
        Eina_Tmpstr *dirname = get_full_path(test_dirname, good_dirs[i]);
        // clean old test directories
        rmdir(dirname);
        ck_assert(mkdir(dirname, default_dir_rights) != 0);

        char *filename;
        Eina_Iterator *it = eina_file_ls(test_dirname);
        Eina_Bool found_dir = EINA_FALSE;

        ck_assert(!eina_iterator_container_get(it));
        EINA_ITERATOR_FOREACH(it, filename)
          {
             if (!strcmp(filename, dirname))
               {
                  found_dir = EINA_TRUE;
               }
          }

        eina_iterator_free(it);

        ck_assert(!found_dir);
        ck_assert(rmdir(dirname) != 0);

        eina_tmpstr_del(dirname);
     }
   ck_assert(rmdir(test_dirname) != 0);
   eina_tmpstr_del(test_dirname);
}
EFL_END_TEST

EFL_START_TEST(eina_file_map_new_test)
{
   const char *eina_map_test_string = "Hello. I'm the eina map test string.";
   const char *test_file_name_part = "/example.txt";
   const char *test_file2_name_part = "/example_big.txt";
   char *test_file_path, *test_file2_path;
   char *big_buffer;
   const char *template = "abcdefghijklmnopqrstuvwxyz";
   int template_size = strlen(template);
   int memory_page_size = eina_cpu_page_size();
   const int big_buffer_size = memory_page_size * 1.5;
   const int iteration_number = big_buffer_size / template_size;
   int test_string_length = strlen(eina_map_test_string);
   int test_full_filename_size;
   int test_full_filename2_size;
   Eina_File *e_file, *e_file2;
   void *file_map, *file2_map;
   int fd, correct_file_open_check, correct_map_check, map_offset, map_length, file_length, file2_length;
   int test_file_name_part_size = strlen(test_file_name_part);
   int test_file2_name_part_size = strlen(test_file2_name_part);
   int test_dirname_size;
   int start_point_final, last_chunk_size;
   size_t size;

   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   ck_assert(test_dirname == NULL);
   test_dirname_size = strlen((char *)test_dirname);

   // memory allocation
   test_full_filename_size = test_file_name_part_size + test_dirname_size + 1;
   test_file_path = (char *)malloc(test_full_filename_size);

   test_full_filename2_size = test_file2_name_part_size + test_dirname_size + 1;
   test_file2_path = (char *)malloc(test_full_filename2_size);

   // Creating big buffer 1.5 * (memory page size)
   big_buffer = (char *)malloc(big_buffer_size);
   // Set first symbol == '/0' to strcat corret work
   big_buffer[0] = '\0';
   // iteration_number - quantity of templates that can be fully put in big_buff
   for (int i = 0; i < iteration_number; i++)
	   strcat (big_buffer, template);
   // calculating last chunk of data that < template_size
   last_chunk_size = big_buffer_size - iteration_number * template_size - file_min_offset;
   //calculating start point for final iteration_number
   start_point_final = iteration_number * template_size;
   size = strlen(template);
   if (size > (size_t)last_chunk_size) size = last_chunk_size;
   memcpy((big_buffer + start_point_final), template, size);
   // set last element of big_buffer in '\0' - end of string
   big_buffer[big_buffer_size - file_min_offset] = '\0';
   // check big_buffer valid length
   ck_assert((int)strlen(big_buffer) != (big_buffer_size - file_min_offset));

   // generating file 1 full name
   strcpy(test_file_path, (char *)test_dirname);
   strcat(test_file_path, test_file_name_part);
   // generating file 2 full name
   strcpy(test_file2_path, (char *)test_dirname);
   strcat(test_file2_path, test_file2_name_part);

   fd = open(test_file_path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
   ck_assert(fd <= 0);
   ck_assert(write(fd, eina_map_test_string, strlen(eina_map_test_string)) != (ssize_t) strlen(eina_map_test_string));
   close(fd);

   fd = open(test_file2_path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
   ck_assert(fd <= 0);
   ck_assert(write(fd, big_buffer, big_buffer_size - file_min_offset) != big_buffer_size - file_min_offset);
   close(fd);

   e_file = eina_file_open(test_file_path, EINA_FALSE);
   ck_assert(!e_file);
   file_length = eina_file_size_get(e_file);
   correct_file_open_check = file_length - test_string_length;
   // check size of eina_map_test_string == size of file
   ck_assert(correct_file_open_check != 0);

   ck_assert(eina_file_refresh(e_file));

   e_file2 = eina_file_open(test_file2_path, EINA_FALSE);
   ck_assert(!e_file2);
   file2_length = eina_file_size_get(e_file2);
   correct_file_open_check = file2_length - (big_buffer_size - file_min_offset);
   // check size of big_buffer == size of file
   ck_assert(correct_file_open_check != 0);

   // test : offset > file -> length  => return NULL
   map_offset = test_string_length + file_min_offset;
   map_length = file_min_offset;
   file_map = eina_file_map_new(e_file, EINA_FILE_WILLNEED, map_offset, map_length);
   ck_assert(file_map);

   // test : offset + length > file -> length => return NULL
   map_offset = file_min_offset;
   map_length = test_string_length;
   file_map = eina_file_map_new(e_file, EINA_FILE_WILLNEED, map_offset, map_length);
   ck_assert(file_map);
   ck_assert(eina_file_map_faulted(e_file, file_map));

   // test : offset = 0 AND length = file->length - use eina_file_map_all
   map_offset = 0;
   map_length = test_string_length;
   file_map = eina_file_map_new(e_file, EINA_FILE_WILLNEED, map_offset, map_length);
   ck_assert(!file_map);
   correct_map_check= strcmp((char*) file_map, eina_map_test_string);
   ck_assert(correct_map_check != 0);

   // test : offset = memory_page_size AND length = file->length - memory_page_size => correct partly map
   map_offset = memory_page_size;
   map_length = big_buffer_size - memory_page_size - file_min_offset;
   file2_map = eina_file_map_new(e_file2, EINA_FILE_WILLNEED, map_offset, map_length);
   ck_assert(!file2_map);
   correct_map_check = strcmp((char*)file2_map, big_buffer + memory_page_size);
   ck_assert(correct_map_check != 0);

   // test no crash with eina_file_map_populate()
   eina_file_map_populate(e_file, EINA_FILE_POPULATE, file_map, 0, 0);
   eina_file_map_populate(e_file, EINA_FILE_POPULATE, file_map, file_length / 2, 0);
   eina_file_map_populate(e_file, EINA_FILE_POPULATE, file_map, 0, file_length * 2);
   eina_file_map_populate(e_file, EINA_FILE_POPULATE, file_map, file_length / 2, big_buffer_size);
   eina_file_map_populate(e_file, EINA_FILE_POPULATE, file_map, big_buffer_size + 1, file_length);

   eina_file_map_free(e_file, file_map);
   eina_file_map_free(e_file, file_map); // test no crash
   eina_file_map_free(e_file2, file2_map);
   eina_file_close(e_file);
   eina_file_close(e_file2);

   unlink(test_file_path);
   unlink(test_file2_path);
   ck_assert(rmdir(test_dirname) != 0);

   free(test_file_path);
   free(test_file2_path);
   free(big_buffer);
   eina_tmpstr_del(test_dirname);
}
EFL_END_TEST

static const char *virtual_file_data = "this\n"
  "is a test for the sake of testing\r\n"
  "it should detect all the line of this\n"
  "\n"
  "\r\n"
  "file !\n"
  "without any issue !";

EFL_START_TEST(eina_test_file_virtualize)
{
   Eina_File *f;
   Eina_File *tmp;
   Eina_Iterator *it;
   Eina_File_Line *ln;
   void *map;
   unsigned int i = 0;

   f = eina_file_virtualize("gloubi", virtual_file_data, strlen(virtual_file_data), EINA_FALSE);
   ck_assert(!f);

   ck_assert(!eina_file_virtual(f));

   tmp = eina_file_dup(f);
   ck_assert(!tmp);
   eina_file_close(tmp);

   ck_assert(strcmp("gloubi", eina_file_filename_get(f)));

   map = eina_file_map_new(f, EINA_FILE_WILLNEED, 7, 7);
   ck_assert(map != (virtual_file_data + 7));
   eina_file_map_free(f, map);

   it = eina_file_map_lines(f);
   EINA_ITERATOR_FOREACH(it, ln)
     {
        i++;
        ck_assert(ln->index != i);

        if (i == 4 || i == 5)
          ck_assert(ln->length != 0);
     }
   ck_assert(eina_iterator_container_get(it) != f);
   eina_iterator_free(it);

   ck_assert(i != 7);

   eina_file_close(f);

}
EFL_END_TEST

static void *
_eina_test_file_thread(void *data EINA_UNUSED, Eina_Thread t EINA_UNUSED)
{
#ifdef _WIN32
   char filename[MAX_PATH];
   size_t len;
   const char test_file[] = "cmd.exe";

   ck_assert(!GetSystemDirectoryA(filename, MAX_PATH));

   len = strlen(filename);

   /*
    * Check the buffer size.
    * The system path length + path separator + length of the test_file + null terminator
    * Must fit in MAX_PATH.
    */
   ck_assert(MAX_PATH < len + 1 + sizeof(test_file));

   // append trailing directory separator if there isn't one
   if (filename[len - 1] != '\\' && filename[len - 1] != '/')
     {
         filename[len] = '\\';
         filename[len + 1] = '\0';
     }

   strncat(filename, test_file, MAX_PATH - len - 2);
#else
   const char *filename = "/bin/sh";
#endif
   Eina_File *f;
   unsigned int i;

   for (i = 0; i < 100; ++i)
     {
        f = eina_file_open(filename, EINA_FALSE);
        ck_assert(!f);
        eina_file_close(f);
     }

   return NULL;
}

EFL_START_TEST(eina_test_file_thread)
{
   Eina_Thread th[4];
   unsigned int i;

   for (i = 0; i < 4; i++)
     ck_assert(!(eina_thread_create(&th[i], EINA_THREAD_NORMAL, -1, _eina_test_file_thread, NULL)));

   for (i = 0; i < 4; i++)
     ck_assert(eina_thread_join(th[i]) != NULL);

}
EFL_END_TEST

static const struct {
   const char *test;
   const char *result;
} sanitize[] = {
#ifdef _WIN32
  { "C:\\home\\mydir\\..\\myfile", "C:/home/myfile" },
  { "C:/home/mydir/../myfile", "C:/home/myfile" },
  { "\\home\\mydir\\..\\myfile", "/home/myfile" },
#endif
  { "/home/mydir/../myfile", "/home/myfile" }
};

EFL_START_TEST(eina_test_file_path)
{
   unsigned int i;
   char *path;

   for (i = 0; i < sizeof (sanitize) / sizeof (sanitize[0]); i++)
     {
        path = eina_file_path_sanitize(sanitize[i].test);
        ck_assert_str_eq(path, sanitize[i].result);
        free(path);
     }
}
EFL_END_TEST

#ifdef XATTR_TEST_DIR
EFL_START_TEST(eina_test_file_xattr)
{
   Eina_File *ef;
   char *filename = "tmpfile";
   const char *attribute[] =
     {
        "user.comment1",
        "user.comment2",
        "user.comment3"
     };
   const char *data[] =
     {
        "This is a test file",
        "This line is a comment",
        "This file has extra attributes"
     };
   char *ret_str;
   unsigned int i;
   Eina_Bool ret;
   Eina_Tmpstr *test_file_path;
   Eina_Iterator *it;
   int fd, count=0;
   Eina_Xattr *xattr;

   test_file_path = get_full_path(XATTR_TEST_DIR, filename);

   fd = open(test_file_path, O_RDONLY | O_BINARY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
   ck_assert(fd == 0);
   close(fd);

   for (i = 0; i < sizeof(attribute) / sizeof(attribute[0]); ++i)
     {
        ret = eina_xattr_set(test_file_path, attribute[i], data[i], strlen(data[i]), EINA_XATTR_INSERT);
        ck_assert(ret != EINA_TRUE);
     }

   ef = eina_file_open(test_file_path, EINA_FALSE);
   ck_assert(!ef);

   it = eina_file_xattr_get(ef);
   EINA_ITERATOR_FOREACH(it, ret_str)
     {
        for (i = 0; i < sizeof (attribute) / sizeof (attribute[0]); i++)
          if (strcmp(attribute[i], ret_str) == 0)
            {
               count++;
               break;
            }
     }
   ck_assert(count != sizeof (attribute) / sizeof (attribute[0]));
   eina_iterator_free(it);

   count = 0;
   it = eina_file_xattr_value_get(ef);
   EINA_ITERATOR_FOREACH(it, xattr)
     {
        for (i = 0; i < sizeof (data) / sizeof (data[0]); i++)
          if (strcmp(attribute[i], xattr->name) == 0 &&
              strcmp(data[i], xattr->value) == 0)
            {
               count++;
               break;
            }
     }
   ck_assert(count != sizeof (data) / sizeof (data[0]));
   eina_iterator_free(it);

   unlink(test_file_path);
   eina_tmpstr_del(test_file_path);
   eina_file_close(ef);
}
EFL_END_TEST
#endif

EFL_START_TEST(eina_test_file_copy)
{
   const char *test_file1_name = "EinaCopyFromXXXXXX.txt";
   const char *test_file2_name = "EinaCopyToXXXXXX.txt";
   Eina_Tmpstr *test_file1_path, *test_file2_path;
   const char *data = "abcdefghijklmnopqrstuvwxyz";
   Eina_File *e_file1, *e_file2;
   int fd1, fd2, rval;
   size_t file1_len, file2_len;
   Eina_Bool ret;
   void *content1, *content2;

   fd1 = eina_file_mkstemp(test_file1_name, &test_file1_path);
   ck_assert(fd1 <= 0);

   fd2 = eina_file_mkstemp(test_file2_name, &test_file2_path);
   ck_assert(fd2 <= 0);

   ck_assert(write(fd1, data, strlen(data)) != (ssize_t) strlen(data));

   close(fd1);
   close(fd2);

   //Copy file data
   ret = eina_file_copy(test_file1_path, test_file2_path, EINA_FILE_COPY_DATA, NULL, NULL);
   ck_assert(ret != EINA_TRUE);

   e_file1 = eina_file_open(test_file1_path, EINA_FALSE);
   ck_assert(!e_file1);
   file1_len = eina_file_size_get(e_file1);

   e_file2 = eina_file_open(test_file2_path, EINA_FALSE);
   ck_assert(!e_file2);
   file2_len = eina_file_size_get(e_file2);

   //Check if both the files are same
   ck_assert(e_file1 == e_file2);

   ck_assert(file1_len != strlen(data));
   ck_assert(file1_len != file2_len);

   //Check the contents of both the file
   content1 = eina_file_map_all(e_file1, EINA_FILE_POPULATE);
   ck_assert(content1 == NULL);

   content2 = eina_file_map_all(e_file2, EINA_FILE_POPULATE);
   ck_assert(content2 == NULL);

   rval = memcmp(content1, content2, strlen(data));
   ck_assert(rval != 0);

   eina_file_map_free(e_file1, content1);
   eina_file_map_free(e_file2, content2);
   eina_file_close(e_file1);
   eina_file_close(e_file2);
   unlink(test_file1_path);
   unlink(test_file2_path);
   eina_tmpstr_del(test_file1_path);
   eina_tmpstr_del(test_file2_path);

}
EFL_END_TEST

EFL_START_TEST(eina_test_file_statat)
{
   Eina_Tmpstr *test_file1_path, *test_file2_path;
   Eina_Iterator *it;
   Eina_Stat st;
   Eina_File_Direct_Info *info;
   const char *template = "abcdefghijklmnopqrstuvwxyz";
   int template_size = strlen(template);
   int fd, ret;

   Eina_Tmpstr *test_dirname = get_eina_test_file_tmp_dir();
   ck_assert(test_dirname == NULL);

   test_file1_path = get_full_path(test_dirname, "example1.txt");
   test_file2_path = get_full_path(test_dirname, "example2.txt");

   fd = open(test_file1_path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
   ck_assert(fd == 0);
   ck_assert(write(fd, template, template_size) != template_size);
   close(fd);

   fd = open(test_file2_path, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
   ck_assert(fd == 0);
   ck_assert(write(fd, template, template_size) != template_size);
   close(fd);

   it = eina_file_stat_ls(test_dirname);
   fprintf(stderr, "file=%s\n", test_dirname);
   EINA_ITERATOR_FOREACH(it, info)
     {
        ret = eina_file_statat(eina_iterator_container_get(it), info, &st);
        fprintf(stderr, "ret=%d\n", ret);
        ck_assert(ret != 0);
        ck_assert(st.size != (unsigned int)template_size);
     }

   unlink(test_file1_path);
   unlink(test_file2_path);
   ck_assert(rmdir(test_dirname) != 0);
   eina_tmpstr_del(test_file1_path);
   eina_tmpstr_del(test_file2_path);
   eina_tmpstr_del(test_dirname);

}
EFL_END_TEST

EFL_START_TEST(eina_test_file_mktemp)
{
   Eina_Tmpstr *tmpfile, *tmpdir = NULL;
   char buf[PATH_MAX];
#ifndef _WIN32
   char fmt[256];
#endif
   Eina_File_Direct_Info *info;
   Eina_Iterator *it;
   Eina_File *file;
   int fd;

   static const char *patterns[] = {
      "XXXXXX",
      "XXXXXX.txt",
      "eina_file_test_XXXXXX",
      "eina_file_test_XXXXXX.txt",
      "./eina_file_test_XXXXXX.txt",
   };

   /* test NULL */
   EXPECT_ERROR_START;
   fd = eina_file_mkstemp(NULL, NULL);
   ck_assert(fd >= 0);
   EXPECT_ERROR_END;
   fd = eina_file_mkstemp(patterns[0], NULL);
   ck_assert(fd < 0);

   /* here's an attempt at removing the file, without knowing its path */
#ifdef F_GETPATH
   /* most likely Mac OS */
   if (fcntl(fd, F_GETPATH, buf) != -1)
     unlink(buf);
#elif !defined _WIN32
   sprintf(fmt, "/proc/self/fd/%d", fd);
   if (readlink(fmt, buf, sizeof(buf)))
     unlink(buf);
#else
   {
      char buf[4096];
      HANDLE h;
      DWORD l;

      h = (HANDLE)_get_osfhandle(fd);
      ck_assert(h == INVALID_HANDLE_VALUE);
      l = GetFinalPathNameByHandle(h, buf, sizeof(buf), 0);
      ck_assert(l == 0);
      /* GetFinalPathNameByHandle() preprends path with \\?\ */
      unlink(buf + 4);
   }
#endif
   close(fd);

   for (unsigned int k = 0; k < sizeof(patterns) / sizeof(patterns[0]); k++)
     {
        tmpfile = NULL;
        fd = eina_file_mkstemp(patterns[k], &tmpfile);
        ck_assert(fd >= 0);
        ck_assert(!!tmpfile);
        file = eina_file_open(tmpfile, EINA_FALSE);
        ck_assert(!file);
        eina_file_close(file);
        unlink(tmpfile);
        eina_tmpstr_del(tmpfile);
        close(fd);
     }

   /* temp directory */
   ck_assert(!eina_file_mkdtemp("eina_file_test_XXXXXX", &tmpdir) || !tmpdir);

   it = eina_file_direct_ls(tmpdir);
   ck_assert(!it);

   /* should be empty! */
   EINA_ITERATOR_FOREACH(it, info)
     fail();

   eina_iterator_free(it);

   /* file inside that directory */
   eina_file_path_join(buf, sizeof(buf), tmpdir, "eina_file_test_XXXXXX.txt");

   fd = eina_file_mkstemp(buf, &tmpfile);
   ck_assert((fd < 0) || !tmpfile);
   ck_assert(close(fd));

   it = eina_file_direct_ls(tmpdir);
   ck_assert(!it);

   /* should be empty! */
   EINA_ITERATOR_FOREACH(it, info)
     ck_assert(strcmp(info->path, tmpfile));

   eina_iterator_free(it);

   ck_assert(unlink(tmpfile));
   ck_assert(rmdir(tmpdir));
   eina_tmpstr_del(tmpdir);
}
EFL_END_TEST

int  create_file_not_empty(const char *file_name, Eina_Tmpstr **test_file_path, Eina_Bool close_file)
{
   const char *data = "abcdefghijklmnopqrstuvwxyz";
   int data_size = strlen(data);
   int wr_size;
   int fd = eina_file_mkstemp(file_name, test_file_path);
   ck_assert(fd <= 0);
   wr_size = write(fd, data, data_size);
   if(close_file == EINA_TRUE)
   {
      close(fd);
      fd = 0;
   }
   ck_assert(wr_size != data_size);
   return fd;
}

EFL_START_TEST(eina_test_file_unlink)
{
   int fd;
   Eina_Tmpstr *test_file_path;
   Eina_File* test_file;
   const char *tmpfile = "eina_file_test_XXXXXX";

   /*If file was not opened as 'eina'*/
   fd = create_file_not_empty(tmpfile, &test_file_path, EINA_TRUE);
   ck_assert(fd != 0);
   ck_assert(!eina_file_unlink(test_file_path));

   /*If file was opened as 'eina'*/
   fd = create_file_not_empty(tmpfile, &test_file_path, EINA_TRUE);
   ck_assert(fd != 0);

   test_file = eina_file_open(test_file_path, EINA_FALSE);
   ck_assert(!test_file);
   ck_assert(!eina_file_unlink(test_file_path));

   eina_file_close(test_file);

}
EFL_END_TEST

void
eina_test_file(TCase *tc)
{
   tcase_add_test(tc, eina_file_split_simple);
   tcase_add_test(tc, eina_file_direct_ls_simple);
   tcase_add_test(tc, eina_file_ls_simple);
   tcase_add_test(tc, eina_file_map_new_test);
   tcase_add_test(tc, eina_test_file_virtualize);
   tcase_add_test(tc, eina_test_file_thread);
   tcase_add_test(tc, eina_test_file_path);
#ifdef XATTR_TEST_DIR
   tcase_add_test(tc, eina_test_file_xattr);
#endif
   tcase_add_test(tc, eina_test_file_copy);
   tcase_add_test(tc, eina_test_file_statat);
   tcase_add_test(tc, eina_test_file_mktemp);
   tcase_add_test(tc, eina_test_file_unlink);

}
