set(EINA_SAFETY_CHECKS 1)
set(EINA_DEBUG_THREADS OFF)
set(VALGRIND OFF)
if("${BUILD_PROFILE}" STREQUAL "dev")
  set(EINA_STRINGSHARE_USAGE ON)
  set(CHECK_VALGRIND ON)
elseif("${BUILD_PROFILE}" STREQUAL "debug")
  set(EINA_DEBUG_MALLOC ON)
  set(EINA_COW_MAGIC_ON ON)
  set(EINA_DEFAULT_MEMPOOL ON)
  set(EINA_DEBUG_MALLOC ON)
  set(EINA_DEBUG_THREADS ON)
  set(VALGRIND ON)
elseif("${BUILD_PROFILE}" STREQUAL "release")
  set(EINA_LOG_LEVEL_MAXIMUM 3)
endif()

EFL_OPTION(EINA_MAGIC_DEBUG "magic debug of eina structure" ON)
EFL_OPTION(EINA_DEBUG_THREADS "debugging of eina threads" ${EINA_DEBUG_THREADS})
EFL_OPTION(VALGRIND "valgrind support" ${VALGRIND})


#check for symbols in pthread
#TODO Make the definitions depending on the platform
set(CMAKE_REQUIRED_FLAGS "${CMAKE_THREAD_LIBS_INIT}")
set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE=1")
CHECK_SYMBOL_EXISTS(pthread_barrier_init pthread.h EINA_HAVE_PTHREAD_BARRIER)
CHECK_SYMBOL_EXISTS(pthread_attr_setaffinity_np pthread.h EINA_HAVE_PTHREAD_AFFINITY)
CHECK_SYMBOL_EXISTS(pthread_setname_np pthread.h EINA_HAVE_PTHREAD_SETNAME)
CHECK_SYMBOL_EXISTS(pthread_spin_init pthread.h EINA_HAVE_POSIX_SPINLOCK)

#check for eina header files that are required
CHECK_INCLUDE_FILE(alloca.h EINA_HAVE_ALLOCA_H)
CHECK_INCLUDE_FILE(byteswap.h EINA_HAVE_BYTESWAP_H)
CHECK_INCLUDE_FILE(fnmatch.h EINA_HAVE_FNMATCH_H)
#only enable log when fnmatch is there
if (EINA_HAVE_FNMATCH_H)
  set(EINA_ENABLE_LOG 1)
endif (EINA_HAVE_FNMATCH_H)
CHECK_INCLUDE_FILE(dirent.h EINA_HAVE_DIRENT_H)

CHECK_TYPE_SIZE("wchar_t" EINA_SIZEOF_WCHAR_T)
CHECK_TYPE_SIZE("uintptr_t" EINA_SIZEOF_UINTPTR_T)
CHECK_TYPE_SIZE("wchar_t" EINA_SIZEOF_WCHAR_T)

#check for swap16/32/64
CHECK_SYMBOL_EXISTS(bswap_16 byteswap.h EINA_HAVE_BSWAP16)
CHECK_SYMBOL_EXISTS(bswap_32 byteswap.h EINA_HAVE_BSWAP32)
CHECK_SYMBOL_EXISTS(bswap_64 byteswap.h EINA_HAVE_BSWAP64)

CHECK_SYMBOL_EXISTS(clock_gettime time.h HAVE_CLOCK_GETTIME)

if(VALGRIND)
  pkg_check_modules(VG REQUIRED valgrind)
  set(NVALGRIND OFF)
else()
  set(NVALGRIND ON)
endif()

#Check if there is the alloca header

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/lib/eina/eina_config.h.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/src/lib/eina/eina_config.h)
