/**************************************************************************/
/*                                                                        */
/*                                 OCaml                                  */
/*                                                                        */
/*          Xavier Leroy and Damien Doligez, INRIA Rocquencourt           */
/*                                                                        */
/*   Copyright 1996 Institut National de Recherche en Informatique et     */
/*     en Automatique.                                                    */
/*                                                                        */
/*   All rights reserved.  This file is distributed under the terms of    */
/*   the GNU Lesser General Public License version 2.1, with the          */
/*   special exception on linking described in the file LICENSE.          */
/*                                                                        */
/**************************************************************************/

/* Miscellaneous macros and variables. */

#ifndef CAML_MISC_H
#define CAML_MISC_H

#ifndef CAML_NAME_SPACE
#include "compatibility.h"
#endif
#include "config.h"

/* Standard definitions */

#include <stddef.h>
#include <stdlib.h>

/* Basic types and constants */

typedef size_t asize_t;

#ifndef NULL
#define NULL 0
#endif

#ifdef CAML_INTERNALS
typedef char * addr;
#endif /* CAML_INTERNALS */

/* Noreturn is preserved for compatibility reasons.
   Instead of the legacy GCC/Clang-only
     foo Noreturn;
   you should prefer
     CAMLnoreturn_start foo CAMLnoreturn_end;
   which supports both GCC/Clang and MSVC.

   Note: CAMLnoreturn is a different macro defined in memory.h,
   to be used in function bodies rather than as a prototype attribute.
*/
#ifdef __GNUC__
  /* Works only in GCC 2.5 and later */
  #define CAMLnoreturn_start
  #define CAMLnoreturn_end __attribute__ ((noreturn))
  #define Noreturn __attribute__ ((noreturn))
#elif _MSC_VER >= 1500
  #define CAMLnoreturn_start __declspec(noreturn)
  #define CAMLnoreturn_end
  #define Noreturn
#else
  #define CAMLnoreturn_start
  #define CAMLnoreturn_end
  #define Noreturn
#endif



/* Export control (to mark primitives and to handle Windows DLL) */

#define CAMLexport
#define CAMLprim
#define CAMLextern extern

/* Weak function definitions that can be overridden by external libs */
/* Conservatively restricted to ELF and MacOSX platforms */
#if defined(__GNUC__) && (defined (__ELF__) || defined(__APPLE__))
#define CAMLweakdef __attribute__((weak))
#else
#define CAMLweakdef
#endif

/* Alignment */
#if defined(__GNUC__)
#define CAMLalign(n) __attribute__((aligned(n)))
#else
#error "How do I align values on this platform?"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* GC timing hooks. These can be assigned by the user.
   [caml_minor_gc_begin_hook] must not allocate nor change any heap value.
   The others can allocate and even call back to OCaml code.
*/
typedef void (*caml_timing_hook) (void);
extern caml_timing_hook caml_major_slice_begin_hook, caml_major_slice_end_hook;
extern caml_timing_hook caml_minor_gc_begin_hook, caml_minor_gc_end_hook;
extern caml_timing_hook caml_finalise_begin_hook, caml_finalise_end_hook;

/* Assertions */

#if !defined(CAML_NOASSERT) && defined(DEBUG)
#define CAMLassert(x) \
  ((x) ? (void) 0 : caml_failed_assert ( #x , __FILE__, __LINE__))
CAMLextern int caml_failed_assert (char *, char *, int);
#else
#define CAMLassert(x) ((void) 0)
#endif

#ifndef CAML_AVOID_CONFLICTS
#define Assert CAMLassert
#endif

#define CAML_STATIC_ASSERT_3(b, l) \
  typedef char static_assertion_failure_line_##l[(b) ? 1 : -1]
#define CAML_STATIC_ASSERT_2(b, l) CAML_STATIC_ASSERT_3(b, l)
#define CAML_STATIC_ASSERT(b) CAML_STATIC_ASSERT_2(b, __LINE__)

#ifdef __GNUC__
#define CAMLcheckresult __attribute__((warn_unused_result))
#else
#define CAMLcheckresult
#endif

#define Is_power_of_2(x) (((x) & ((x) - 1)) == 0)

CAMLextern void caml_fatal_error (const char *msg) Noreturn;
CAMLextern void caml_fatal_error_arg (const char *fmt, const char *arg) Noreturn;
CAMLextern void caml_fatal_error_arg2 (const char *fmt1, const char *arg1,
                                       const char *fmt2, const char *arg2) Noreturn;

/* Detection of available C built-in functions, the Clang way. */

#ifdef __has_builtin
#define Caml_has_builtin(x) __has_builtin(x)
#else
#define Caml_has_builtin(x) 0
#endif

/* Integer arithmetic with overflow detection.
   The functions return 0 if no overflow, 1 if overflow.
   The result of the operation is always stored at [*res].
   If no overflow is reported, this is the exact result.
   If overflow is reported, this is the exact result modulo 2 to the word size.
*/

static inline int caml_uadd_overflow(uintnat a, uintnat b, uintnat * res)
{
#if __GNUC__ >= 5 || Caml_has_builtin(__builtin_add_overflow)
  return __builtin_add_overflow(a, b, res);
#else
  uintnat c = a + b;
  *res = c;
  return c < a;
#endif
}

static inline int caml_usub_overflow(uintnat a, uintnat b, uintnat * res)
{
#if __GNUC__ >= 5 || Caml_has_builtin(__builtin_sub_overflow)
  return __builtin_sub_overflow(a, b, res);
#else
  uintnat c = a - b;
  *res = c;
  return a < b;
#endif
}

#if __GNUC__ >= 5 || Caml_has_builtin(__builtin_mul_overflow)
static inline int caml_umul_overflow(uintnat a, uintnat b, uintnat * res)
{
  return __builtin_mul_overflow(a, b, res);
}
#else
extern int caml_umul_overflow(uintnat a, uintnat b, uintnat * res);
#endif

/* Windows Unicode support */

#ifdef _WIN32

typedef wchar_t char_os;

#define _T(x) L ## x

#define access_os _waccess
#define open_os _wopen
#define stat_os _wstati64
#define unlink_os _wunlink
#define rename_os caml_win32_rename
#define chdir_os _wchdir
#define getcwd_os _wgetcwd
#define getenv_os _wgetenv
#define system_os _wsystem
#define rmdir_os _wrmdir
#define utime_os _wutime
#define putenv_os _wputenv
#define chmod_os _wchmod
#define execv_os _wexecv
#define execve_os _wexecve
#define execvp_os _wexecvp
#define execvpe_os _wexecvpe
#define strcmp_os wcscmp
#define strlen_os wcslen
#define sscanf_os swscanf

#define caml_stat_strdup_os caml_stat_wcsdup
#define caml_stat_strconcat_os caml_stat_wcsconcat

#define caml_stat_strdup_to_os caml_stat_strdup_to_utf16
#define caml_stat_strdup_of_os caml_stat_strdup_of_utf16
#define caml_copy_string_of_os caml_copy_string_of_utf16

#else /* _WIN32 */

typedef char char_os;

#define _T(x) x

#define access_os access
#define open_os open
#define stat_os stat
#define unlink_os unlink
#define rename_os rename
#define chdir_os chdir
#define getcwd_os getcwd
#define getenv_os getenv
#define system_os system
#define rmdir_os rmdir
#define utime_os utime
#define putenv_os putenv
#define chmod_os chmod
#define execv_os execv
#define execve_os execve
#define execvp_os execvp
#define execvpe_os execvpe
#define strcmp_os strcmp
#define strlen_os strlen
#define sscanf_os sscanf

#define caml_stat_strdup_os caml_stat_strdup
#define caml_stat_strconcat_os caml_stat_strconcat

#define caml_stat_strdup_to_os caml_stat_strdup
#define caml_stat_strdup_of_os caml_stat_strdup
#define caml_copy_string_of_os caml_copy_string

#endif /* _WIN32 */


/* Use macros for some system calls being called from OCaml itself.
  These calls can be either traced for security reasons, or changed to
  virtualize the program. */


#ifndef CAML_WITH_CPLUGINS

#define CAML_SYS_EXIT(retcode) exit(retcode)
#define CAML_SYS_OPEN(filename,flags,perm) open_os(filename,flags,perm)
#define CAML_SYS_CLOSE(fd) close(fd)
#define CAML_SYS_STAT(filename,st) stat_os(filename,st)
#define CAML_SYS_UNLINK(filename) unlink_os(filename)
#define CAML_SYS_RENAME(old_name,new_name) rename_os(old_name, new_name)
#define CAML_SYS_CHDIR(dirname) chdir_os(dirname)
#define CAML_SYS_GETENV(varname) getenv_os(varname)
#define CAML_SYS_SYSTEM(command) system_os(command)
#define CAML_SYS_READ_DIRECTORY(dirname,tbl) caml_read_directory(dirname,tbl)

#else


#define CAML_CPLUGINS_EXIT 0
#define CAML_CPLUGINS_OPEN 1
#define CAML_CPLUGINS_CLOSE 2
#define CAML_CPLUGINS_STAT 3
#define CAML_CPLUGINS_UNLINK 4
#define CAML_CPLUGINS_RENAME 5
#define CAML_CPLUGINS_CHDIR 6
#define CAML_CPLUGINS_GETENV 7
#define CAML_CPLUGINS_SYSTEM 8
#define CAML_CPLUGINS_READ_DIRECTORY 9
#define CAML_CPLUGINS_PRIMS_MAX 9

#define CAML_CPLUGINS_PRIMS_BITMAP  ((1 << CAML_CPLUGINS_PRIMS_MAX)-1)

extern intnat (*caml_cplugins_prim)(int,intnat,intnat,intnat);

#define CAML_SYS_PRIM_1(code,prim,arg1)               \
  (caml_cplugins_prim == NULL) ? prim(arg1) :    \
  caml_cplugins_prim(code,(intnat) (arg1),0,0)
#define CAML_SYS_STRING_PRIM_1(code,prim,arg1)               \
  (caml_cplugins_prim == NULL) ? prim(arg1) :    \
  (char_os*)caml_cplugins_prim(code,(intnat) (arg1),0,0)
#define CAML_SYS_VOID_PRIM_1(code,prim,arg1)               \
  (caml_cplugins_prim == NULL) ? prim(arg1) :    \
  (void)caml_cplugins_prim(code,(intnat) (arg1),0,0)
#define CAML_SYS_PRIM_2(code,prim,arg1,arg2)                         \
  (caml_cplugins_prim == NULL) ? prim(arg1,arg2) :              \
  caml_cplugins_prim(code,(intnat) (arg1), (intnat) (arg2),0)
#define CAML_SYS_PRIM_3(code,prim,arg1,arg2,arg3)                            \
  (caml_cplugins_prim == NULL) ? prim(arg1,arg2,arg3) :                 \
  caml_cplugins_prim(code,(intnat) (arg1), (intnat) (arg2),(intnat) (arg3))

#define CAML_SYS_EXIT(retcode) \
  CAML_SYS_VOID_PRIM_1(CAML_CPLUGINS_EXIT,exit,retcode)
#define CAML_SYS_OPEN(filename,flags,perm)                      \
  CAML_SYS_PRIM_3(CAML_CPLUGINS_OPEN,open_os,filename,flags,perm)
#define CAML_SYS_CLOSE(fd)                      \
  CAML_SYS_PRIM_1(CAML_CPLUGINS_CLOSE,close,fd)
#define CAML_SYS_STAT(filename,st)                      \
  CAML_SYS_PRIM_2(CAML_CPLUGINS_STAT,stat_os,filename,st)
#define CAML_SYS_UNLINK(filename)                       \
  CAML_SYS_PRIM_1(CAML_CPLUGINS_UNLINK,unlink_os,filename)
#define CAML_SYS_RENAME(old_name,new_name)                              \
  CAML_SYS_PRIM_2(CAML_CPLUGINS_RENAME,rename_os,old_name,new_name)
#define CAML_SYS_CHDIR(dirname)                         \
  CAML_SYS_PRIM_1(CAML_CPLUGINS_CHDIR,chdir_os,dirname)
#define CAML_SYS_GETENV(varname)                        \
  CAML_SYS_STRING_PRIM_1(CAML_CPLUGINS_GETENV,getenv_os,varname)
#define CAML_SYS_SYSTEM(command)                        \
  CAML_SYS_PRIM_1(CAML_CPLUGINS_SYSTEM,system_os,command)
#define CAML_SYS_READ_DIRECTORY(dirname,tbl)                            \
  CAML_SYS_PRIM_2(CAML_CPLUGINS_READ_DIRECTORY,caml_read_directory,     \
                  dirname,tbl)

#define CAML_CPLUGIN_CONTEXT_API 0

struct cplugin_context {
  int api_version;
  int prims_bitmap;
  char_os *exe_name;
  char_os** argv;
  char_os *plugin; /* absolute filename of plugin, do a copy if you need it ! */
  char *ocaml_version;
/* end of CAML_CPLUGIN_CONTEXT_API version 0 */
};

extern void caml_cplugins_init(char_os * exe_name, char_os **argv);

/* A plugin MUST define a symbol "caml_cplugin_init" with the prototype:

void caml_cplugin_init(struct cplugin_context *ctx)
*/

/* to write plugins for CAML_SYS_READ_DIRECTORY, we will need the
   definition of struct ext_table to be public. */

#endif /* CAML_WITH_CPLUGINS */

/* Data structures */

struct ext_table {
  int size;
  int capacity;
  void ** contents;
};

extern void caml_ext_table_init(struct ext_table * tbl, int init_capa);
extern int caml_ext_table_add(struct ext_table * tbl, void * data);
extern void caml_ext_table_free(struct ext_table * tbl, int free_entries);
extern void caml_ext_table_remove(struct ext_table * tbl, void * data);
extern void caml_ext_table_clear(struct ext_table * tbl, int free_entries);

CAMLextern int caml_read_directory(char_os * dirname, struct ext_table * contents);

/* Deprecated aliases */
#define caml_aligned_malloc caml_stat_alloc_aligned_noexc
#define caml_strdup caml_stat_strdup
#define caml_strconcat caml_stat_strconcat

#ifdef CAML_INTERNALS

/* GC flags and messages */

void caml_gc_log (char *, ...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 1, 2)))
#endif
;

void caml_gc_message (int, char *, ...)
#ifdef __GNUC__
  __attribute__ ((format (printf, 2, 3)))
#endif
;

/* Runtime warnings */
extern uintnat caml_runtime_warnings;
int caml_runtime_warnings_active(void);

#ifdef DEBUG
#ifdef ARCH_SIXTYFOUR
#define Debug_tag(x) (INT64_LITERAL(0xD700D7D7D700D6D7u) \
                      | ((uintnat) (x) << 16) \
                      | ((uintnat) (x) << 48))
#define Is_debug_tag(x) \
  (((x) & INT64_LITERAL(0xff00ffffff00ffffu)) == INT64_LITERAL(0xD700D7D7D700D6D7u))
#else
#define Debug_tag(x) (0xD700D6D7ul | ((uintnat) (x) << 16))
#define Is_debug_tag(x) \
  (((x) & 0xff00fffful) == 0xD700D6D7ul)
#endif /* ARCH_SIXTYFOUR */

/*
  00 -> free words in minor heap
  01 -> fields of free list blocks in major heap
  03 -> heap chunks deallocated by heap shrinking
  04 -> fields deallocated by [caml_obj_truncate]
  10 -> uninitialised fields of minor objects
  11 -> uninitialised fields of major objects
  15 -> uninitialised words of [caml_stat_alloc_aligned] blocks
  85 -> filler bytes of [caml_stat_alloc_aligned]
  99 -> the magic prefix of a memory block allocated by [caml_stat_alloc]

  special case (byte by byte):
  D7 -> uninitialised words of [caml_stat_alloc] blocks
*/
#define Debug_free_minor     Debug_tag (0x00)
#define Debug_free_major     Debug_tag (0x01)
#define Debug_free_shrink    Debug_tag (0x03)
#define Debug_free_truncate  Debug_tag (0x04)
#define Debug_uninit_minor   Debug_tag (0x10)
#define Debug_uninit_major   Debug_tag (0x11)
#define Debug_uninit_align   Debug_tag (0x15)
#define Debug_filler_align   Debug_tag (0x85)
#define Debug_pool_magic     Debug_tag (0x99)

#define Debug_uninit_stat    0xD7

#endif /* DEBUG */


/* snprintf emulation for Win32 */

#ifdef _WIN32
extern int caml_snprintf(char * buf, size_t size, const char * format, ...);
#define snprintf caml_snprintf
#endif

#define CAML_INSTR_DECLARE(t) /**/
#define CAML_INSTR_ALLOC(t) /**/
#define CAML_INSTR_START(t, name) /**/
#define CAML_INSTR_SETUP(t, name) /**/
#define CAML_INSTR_TIME(t, msg) /**/
#define CAML_INSTR_INT(msg, c) /**/
#define CAML_INSTR_INIT() /**/
#define CAML_INSTR_ATEXIT() /**/

#endif /* CAML_INTERNALS */

#ifdef __cplusplus
}
#endif

#endif /* CAML_MISC_H */
