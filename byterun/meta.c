/**************************************************************************/
/*                                                                        */
/*                                 OCaml                                  */
/*                                                                        */
/*             Xavier Leroy, projet Cristal, INRIA Rocquencourt           */
/*                                                                        */
/*   Copyright 1996 Institut National de Recherche en Informatique et     */
/*     en Automatique.                                                    */
/*                                                                        */
/*   All rights reserved.  This file is distributed under the terms of    */
/*   the GNU Lesser General Public License version 2.1, with the          */
/*   special exception on linking described in the file LICENSE.          */
/*                                                                        */
/**************************************************************************/

#define CAML_INTERNALS

/* Primitives for the toplevel */

#include <string.h>
#include "caml/alloc.h"
#include "caml/config.h"
#include "caml/fail.h"
#include "caml/fix_code.h"
#include "caml/interp.h"
#include "caml/intext.h"
#include "caml/major_gc.h"
#include "caml/memory.h"
#include "caml/minor_gc.h"
#include "caml/misc.h"
#include "caml/mlvalues.h"
#include "caml/prims.h"
#include "caml/fiber.h"
#include "caml/startup_aux.h"
#include "caml/backtrace_prim.h"

#ifndef NATIVE_CODE

CAMLprim value caml_get_global_data(value unit)
{
  return caml_read_root(caml_global_data);
}

CAMLprim value caml_get_section_table(value unit)
{
  if (caml_params->section_table == NULL) caml_raise_not_found();
  return caml_input_value_from_block(caml_params->section_table,
                                     caml_params->section_table_size);
}

struct bytecode {
  code_t prog;
  asize_t len;
};
#define Bytecode_val(p) ((struct bytecode*)Data_abstract_val(p))

CAMLprim value caml_reify_bytecode(value ls_prog, value debuginfo)
{
  CAMLparam2(ls_prog, debuginfo);
  CAMLlocal4(clos, bytecode, retval, s);
  struct code_fragment * cf = caml_stat_alloc(sizeof(struct code_fragment));
  code_t prog;
  asize_t len, off;
  int i;

  /* Convert from LongString.t to contiguous buffer */
  len = 0;
  for (i = 0; i < Wosize_val(ls_prog); i++) {
    s = Field(ls_prog, i);
    len += caml_string_length(s);
  }

  prog = (code_t)caml_stat_alloc(len);
  off = 0;
  for (i = 0; i < Wosize_val(ls_prog); i++) {
    size_t s_len;
    s = Field(ls_prog, i);
    s_len = caml_string_length(s);
    memcpy((char*)prog + off, Bytes_val(s), s_len);
    off += s_len;
  }

  caml_add_debug_info(prog, Val_long(len), debuginfo);
  cf->code_start = (char *) prog;
  cf->code_end = (char *) prog + len;
  cf->digest_computed = 0;
  caml_ext_table_add(&caml_code_fragments_table, cf);

#ifdef ARCH_BIG_ENDIAN
  caml_fixup_endianness((code_t) prog, len);
#endif
#ifdef THREADED_CODE
  caml_thread_code((code_t) prog, len);
#endif
  clos = caml_alloc_1 (Closure_tag, Val_bytecode(prog));
  bytecode = caml_alloc_small (2, Abstract_tag);
  Bytecode_val(bytecode)->prog = prog;
  Bytecode_val(bytecode)->len = len;
  retval = caml_alloc_2 (0, bytecode, clos);
  CAMLreturn (retval);
}

/* signal to the interpreter machinery that a bytecode is no more
   needed (before freeing it) - this might be useful for a JIT
   implementation */

CAMLprim value caml_static_release_bytecode(value bc)
{
  code_t prog;
  asize_t len;
  struct code_fragment * cf = NULL, * cfi;
  int i;
  prog = Bytecode_val(bc)->prog;
  len = Bytecode_val(bc)->len;
  caml_remove_debug_info(prog);
  for (i = 0; i < caml_code_fragments_table.size; i++) {
    cfi = (struct code_fragment *) caml_code_fragments_table.contents[i];
    if (cfi->code_start == (char *) prog &&
        cfi->code_end == (char *) prog + len) {
      cf = cfi;
      break;
    }
  }

  if (!cf) {
      /* [cf] Not matched with a caml_reify_bytecode call; impossible. */
      CAMLassert (0);
  } else {
      caml_ext_table_remove(&caml_code_fragments_table, cf);
  }

#ifndef NATIVE_CODE
#else
  caml_failwith("Meta.static_release_bytecode impossible with native code");
#endif
  caml_stat_free(prog);
  return Val_unit;
}

CAMLprim value caml_register_code_fragment(value prog, value len, value digest)
{
  struct code_fragment * cf = caml_stat_alloc(sizeof(struct code_fragment));
  cf->code_start = (char *) prog;
  cf->code_end = (char *) prog + Long_val(len);
  memcpy(cf->digest, String_val(digest), 16);
  cf->digest_computed = 1;
  caml_ext_table_add(&caml_code_fragments_table, cf);
  return Val_unit;
}

CAMLprim value caml_realloc_global(value size)
{
  CAMLparam1(size);
  CAMLlocal2(old_global_data, new_global_data);
  mlsize_t requested_size, actual_size, i;
  old_global_data = caml_read_root(caml_global_data);

  requested_size = Long_val(size);
  actual_size = Wosize_val(old_global_data);
  if (requested_size >= actual_size) {
    requested_size = (requested_size + 0x100) & 0xFFFFFF00;
    caml_gc_message (0x08, "Growing global data to %"
                     ARCH_INTNAT_PRINTF_FORMAT "u entries",
                     requested_size);
    new_global_data = caml_alloc_shr(requested_size, 0);
    for (i = 0; i < actual_size; i++)
      caml_initialize_field(new_global_data, i, Field_imm(old_global_data, i));
    for (i = actual_size; i < requested_size; i++){
      caml_initialize_field(new_global_data, i, Val_long(0));
    }
    caml_modify_root(caml_global_data, new_global_data);
  }
  CAMLreturn (Val_unit);
}

CAMLprim value caml_get_current_environment(value unit)
{
  return *Caml_state->current_stack->sp;
}

CAMLprim value caml_invoke_traced_function(value codeptr, value env, value arg)
{
  caml_domain_state* domain_state = Caml_state;
  /* Stack layout on entry:
       return frame into instrument_closure function
       arg3 to call_original_code (arg)
       arg2 to call_original_code (env)
       arg1 to call_original_code (codeptr)
       arg3 to call_original_code (arg)
       arg2 to call_original_code (env)
       saved env */

  /* Stack layout on exit:
       return frame into instrument_closure function
       actual arg to code (arg)
       pseudo return frame into codeptr:
         extra_args = 0
         environment = env
         PC = codeptr
       arg3 to call_original_code (arg)                   same 6 bottom words as
       arg2 to call_original_code (env)                   on entrance, but
       arg1 to call_original_code (codeptr)               shifted down 4 words
       arg3 to call_original_code (arg)
       arg2 to call_original_code (env)
       saved env */

  value * osp, * nsp;
  int i;

  osp = domain_state->current_stack->sp;
  domain_state->current_stack->sp -= 4;
  nsp = domain_state->current_stack->sp;
  for (i = 0; i < 6; i++) nsp[i] = osp[i];
  nsp[6] = codeptr;
  nsp[7] = env;
  nsp[8] = Val_int(0);
  nsp[9] = arg;
  return Val_unit;
}

#else

/* Dummy definitions to support compilation of ocamlc.opt */

value caml_get_global_data(value unit)
{
  caml_invalid_argument("Meta.get_global_data");
  return Val_unit; /* not reached */
}

value caml_get_section_table(value unit)
{
  caml_invalid_argument("Meta.get_section_table");
  return Val_unit; /* not reached */
}

value caml_realloc_global(value size)
{
  caml_invalid_argument("Meta.realloc_global");
  return Val_unit; /* not reached */
}

value caml_invoke_traced_function(value codeptr, value env, value arg)
{
  caml_invalid_argument("Meta.invoke_traced_function");
  return Val_unit; /* not reached */
}

value caml_reify_bytecode(value prog, value len)
{
  caml_invalid_argument("Meta.reify_bytecode");
  return Val_unit; /* not reached */
}

value caml_static_release_bytecode(value prog, value len)
{
  caml_invalid_argument("Meta.static_release_bytecode");
  return Val_unit; /* not reached */
}

value * caml_stack_low;
value * caml_stack_high;
value * caml_stack_threshold;
value * caml_extern_sp;
value * caml_trapsp;
int caml_callback_depth;
int volatile caml_something_to_do;
void (* volatile caml_async_action_hook)(void);
struct longjmp_buffer * caml_external_raise;

#endif
