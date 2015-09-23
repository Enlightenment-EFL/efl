//Compile with:
//gcc -g eina_tmpstr_01.c -o eina_tmpstr_01 `pkg-config --cflags --libs eina`

#include <stdio.h>
#include <Eina.h>

int
main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   const char *str, *str2;
   time_t curr_time;
   struct tm * info;
   const char *prologe = "The Cylons were created by man. They rebelled. They "
                         "evolved.";

   eina_init();

   str = eina_tmpstr_add_length(prologe, 31);
   printf("%s\n", str);
   printf("length: %d\n", eina_tmpstr_len(str));
   eina_tmpstr_del(str);

   curr_time = time(NULL);
   info = localtime(&curr_time);
   str2 = eina_tmpstr_strftime("%I:%M%p", info);
   printf("%s\n", str2);
   printf("length: %d\n", eina_tmpstr_len(str2));
   eina_tmpstr_del(str2);

   eina_shutdown();

   return 0;
}
