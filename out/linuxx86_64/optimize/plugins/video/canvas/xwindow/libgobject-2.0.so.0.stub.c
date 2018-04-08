/* automatically generated: Sat Apr  7 15:05:49 CST 2018 by devbat@DESKTOP-JIVE2M0, do not edit
 *
 * Built by relaytool, a program for building delay-load jumptables
 * relaytool is (C) 2004 Mike Hearn <mike@navi.cx>
 * See http://autopackage.org/ for details.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/mman.h>

#ifdef __cplusplus
    extern "C" {
#endif

static void **ptrs;
static char *functions[] = {
    "g_array_get_type",
    "g_binding_flags_get_type",
    "g_binding_get_flags",
    "g_binding_get_source",
    "g_binding_get_source_property",
    "g_binding_get_target",
    "g_binding_get_target_property",
    "g_binding_get_type",
    "g_binding_unbind",
    "g_boxed_copy",
    "g_boxed_free",
    "g_boxed_type_register_static",
    "g_byte_array_get_type",
    "g_bytes_get_type",
    "g_cclosure_marshal_BOOLEAN__BOXED_BOXED",
    "g_cclosure_marshal_BOOLEAN__BOXED_BOXEDv",
    "g_cclosure_marshal_BOOLEAN__FLAGS",
    "g_cclosure_marshal_BOOLEAN__FLAGSv",
    "g_cclosure_marshal_STRING__OBJECT_POINTER",
    "g_cclosure_marshal_STRING__OBJECT_POINTERv",
    "g_cclosure_marshal_VOID__BOOLEAN",
    "g_cclosure_marshal_VOID__BOOLEANv",
    "g_cclosure_marshal_VOID__BOXED",
    "g_cclosure_marshal_VOID__BOXEDv",
    "g_cclosure_marshal_VOID__CHAR",
    "g_cclosure_marshal_VOID__CHARv",
    "g_cclosure_marshal_VOID__DOUBLE",
    "g_cclosure_marshal_VOID__DOUBLEv",
    "g_cclosure_marshal_VOID__ENUM",
    "g_cclosure_marshal_VOID__ENUMv",
    "g_cclosure_marshal_VOID__FLAGS",
    "g_cclosure_marshal_VOID__FLAGSv",
    "g_cclosure_marshal_VOID__FLOAT",
    "g_cclosure_marshal_VOID__FLOATv",
    "g_cclosure_marshal_VOID__INT",
    "g_cclosure_marshal_VOID__INTv",
    "g_cclosure_marshal_VOID__LONG",
    "g_cclosure_marshal_VOID__LONGv",
    "g_cclosure_marshal_VOID__OBJECT",
    "g_cclosure_marshal_VOID__OBJECTv",
    "g_cclosure_marshal_VOID__PARAM",
    "g_cclosure_marshal_VOID__PARAMv",
    "g_cclosure_marshal_VOID__POINTER",
    "g_cclosure_marshal_VOID__POINTERv",
    "g_cclosure_marshal_VOID__STRING",
    "g_cclosure_marshal_VOID__STRINGv",
    "g_cclosure_marshal_VOID__UCHAR",
    "g_cclosure_marshal_VOID__UCHARv",
    "g_cclosure_marshal_VOID__UINT",
    "g_cclosure_marshal_VOID__UINT_POINTER",
    "g_cclosure_marshal_VOID__UINT_POINTERv",
    "g_cclosure_marshal_VOID__UINTv",
    "g_cclosure_marshal_VOID__ULONG",
    "g_cclosure_marshal_VOID__ULONGv",
    "g_cclosure_marshal_VOID__VARIANT",
    "g_cclosure_marshal_VOID__VARIANTv",
    "g_cclosure_marshal_VOID__VOID",
    "g_cclosure_marshal_VOID__VOIDv",
    "g_cclosure_marshal_generic",
    "g_cclosure_marshal_generic_va",
    "g_cclosure_new",
    "g_cclosure_new_object",
    "g_cclosure_new_object_swap",
    "g_cclosure_new_swap",
    "g_checksum_get_type",
    "g_clear_object",
    "g_closure_add_finalize_notifier",
    "g_closure_add_invalidate_notifier",
    "g_closure_add_marshal_guards",
    "g_closure_get_type",
    "g_closure_invalidate",
    "g_closure_invoke",
    "g_closure_new_object",
    "g_closure_new_simple",
    "g_closure_ref",
    "g_closure_remove_finalize_notifier",
    "g_closure_remove_invalidate_notifier",
    "g_closure_set_marshal",
    "g_closure_set_meta_marshal",
    "g_closure_sink",
    "g_closure_unref",
    "g_date_get_type",
    "g_date_time_get_type",
    "g_enum_complete_type_info",
    "g_enum_get_value",
    "g_enum_get_value_by_name",
    "g_enum_get_value_by_nick",
    "g_enum_register_static",
    "g_enum_to_string",
    "g_error_get_type",
    "g_flags_complete_type_info",
    "g_flags_get_first_value",
    "g_flags_get_value_by_name",
    "g_flags_get_value_by_nick",
    "g_flags_register_static",
    "g_flags_to_string",
    "g_gstring_get_type",
    "g_gtype_get_type",
    "g_hash_table_get_type",
    "g_initially_unowned_get_type",
    "g_io_channel_get_type",
    "g_io_condition_get_type",
    "g_key_file_get_type",
    "g_main_context_get_type",
    "g_main_loop_get_type",
    "g_mapped_file_get_type",
    "g_markup_parse_context_get_type",
    "g_match_info_get_type",
    "g_object_add_toggle_ref",
    "g_object_add_weak_pointer",
    "g_object_bind_property",
    "g_object_bind_property_full",
    "g_object_bind_property_with_closures",
    "g_object_class_find_property",
    "g_object_class_install_properties",
    "g_object_class_install_property",
    "g_object_class_list_properties",
    "g_object_class_override_property",
    "g_object_compat_control",
    "g_object_connect",
    "g_object_disconnect",
    "g_object_dup_data",
    "g_object_dup_qdata",
    "g_object_force_floating",
    "g_object_freeze_notify",
    "g_object_get",
    "g_object_get_data",
    "g_object_get_property",
    "g_object_get_qdata",
    "g_object_get_type",
    "g_object_get_valist",
    "g_object_getv",
    "g_object_interface_find_property",
    "g_object_interface_install_property",
    "g_object_interface_list_properties",
    "g_object_is_floating",
    "g_object_new",
    "g_object_new_valist",
    "g_object_new_with_properties",
    "g_object_newv",
    "g_object_notify",
    "g_object_notify_by_pspec",
    "g_object_ref",
    "g_object_ref_sink",
    "g_object_remove_toggle_ref",
    "g_object_remove_weak_pointer",
    "g_object_replace_data",
    "g_object_replace_qdata",
    "g_object_run_dispose",
    "g_object_set",
    "g_object_set_data",
    "g_object_set_data_full",
    "g_object_set_property",
    "g_object_set_qdata",
    "g_object_set_qdata_full",
    "g_object_set_valist",
    "g_object_setv",
    "g_object_steal_data",
    "g_object_steal_qdata",
    "g_object_thaw_notify",
    "g_object_unref",
    "g_object_watch_closure",
    "g_object_weak_ref",
    "g_object_weak_unref",
    "g_option_group_get_type",
    "g_param_spec_boolean",
    "g_param_spec_boxed",
    "g_param_spec_char",
    "g_param_spec_double",
    "g_param_spec_enum",
    "g_param_spec_flags",
    "g_param_spec_float",
    "g_param_spec_get_blurb",
    "g_param_spec_get_default_value",
    "g_param_spec_get_name",
    "g_param_spec_get_name_quark",
    "g_param_spec_get_nick",
    "g_param_spec_get_qdata",
    "g_param_spec_get_redirect_target",
    "g_param_spec_gtype",
    "g_param_spec_int",
    "g_param_spec_int64",
    "g_param_spec_internal",
    "g_param_spec_long",
    "g_param_spec_object",
    "g_param_spec_override",
    "g_param_spec_param",
    "g_param_spec_pointer",
    "g_param_spec_pool_insert",
    "g_param_spec_pool_list",
    "g_param_spec_pool_list_owned",
    "g_param_spec_pool_lookup",
    "g_param_spec_pool_new",
    "g_param_spec_pool_remove",
    "g_param_spec_ref",
    "g_param_spec_ref_sink",
    "g_param_spec_set_qdata",
    "g_param_spec_set_qdata_full",
    "g_param_spec_sink",
    "g_param_spec_steal_qdata",
    "g_param_spec_string",
    "g_param_spec_uchar",
    "g_param_spec_uint",
    "g_param_spec_uint64",
    "g_param_spec_ulong",
    "g_param_spec_unichar",
    "g_param_spec_unref",
    "g_param_spec_value_array",
    "g_param_spec_variant",
    "g_param_type_register_static",
    "g_param_value_convert",
    "g_param_value_defaults",
    "g_param_value_set_default",
    "g_param_value_validate",
    "g_param_values_cmp",
    "g_pointer_type_register_static",
    "g_pollfd_get_type",
    "g_ptr_array_get_type",
    "g_regex_get_type",
    "g_signal_accumulator_first_wins",
    "g_signal_accumulator_true_handled",
    "g_signal_add_emission_hook",
    "g_signal_chain_from_overridden",
    "g_signal_chain_from_overridden_handler",
    "g_signal_connect_closure",
    "g_signal_connect_closure_by_id",
    "g_signal_connect_data",
    "g_signal_connect_object",
    "g_signal_emit",
    "g_signal_emit_by_name",
    "g_signal_emit_valist",
    "g_signal_emitv",
    "g_signal_get_invocation_hint",
    "g_signal_handler_block",
    "g_signal_handler_disconnect",
    "g_signal_handler_find",
    "g_signal_handler_is_connected",
    "g_signal_handler_unblock",
    "g_signal_handlers_block_matched",
    "g_signal_handlers_destroy",
    "g_signal_handlers_disconnect_matched",
    "g_signal_handlers_unblock_matched",
    "g_signal_has_handler_pending",
    "g_signal_list_ids",
    "g_signal_lookup",
    "g_signal_name",
    "g_signal_new",
    "g_signal_new_class_handler",
    "g_signal_new_valist",
    "g_signal_newv",
    "g_signal_override_class_closure",
    "g_signal_override_class_handler",
    "g_signal_parse_name",
    "g_signal_query",
    "g_signal_remove_emission_hook",
    "g_signal_set_va_marshaller",
    "g_signal_stop_emission",
    "g_signal_stop_emission_by_name",
    "g_signal_type_cclosure_new",
    "g_source_get_type",
    "g_source_set_closure",
    "g_source_set_dummy_callback",
    "g_strdup_value_contents",
    "g_strv_get_type",
    "g_thread_get_type",
    "g_time_zone_get_type",
    "g_type_add_class_cache_func",
    "g_type_add_class_private",
    "g_type_add_instance_private",
    "g_type_add_interface_check",
    "g_type_add_interface_dynamic",
    "g_type_add_interface_static",
    "g_type_check_class_cast",
    "g_type_check_class_is_a",
    "g_type_check_instance",
    "g_type_check_instance_cast",
    "g_type_check_instance_is_a",
    "g_type_check_instance_is_fundamentally_a",
    "g_type_check_is_value_type",
    "g_type_check_value",
    "g_type_check_value_holds",
    "g_type_children",
    "g_type_class_add_private",
    "g_type_class_adjust_private_offset",
    "g_type_class_get_instance_private_offset",
    "g_type_class_get_private",
    "g_type_class_peek",
    "g_type_class_peek_parent",
    "g_type_class_peek_static",
    "g_type_class_ref",
    "g_type_class_unref",
    "g_type_class_unref_uncached",
    "g_type_create_instance",
    "g_type_default_interface_peek",
    "g_type_default_interface_ref",
    "g_type_default_interface_unref",
    "g_type_depth",
    "g_type_ensure",
    "g_type_free_instance",
    "g_type_from_name",
    "g_type_fundamental",
    "g_type_fundamental_next",
    "g_type_get_instance_count",
    "g_type_get_plugin",
    "g_type_get_qdata",
    "g_type_get_type_registration_serial",
    "g_type_init",
    "g_type_init_with_debug_flags",
    "g_type_instance_get_private",
    "g_type_interface_add_prerequisite",
    "g_type_interface_get_plugin",
    "g_type_interface_peek",
    "g_type_interface_peek_parent",
    "g_type_interface_prerequisites",
    "g_type_interfaces",
    "g_type_is_a",
    "g_type_module_add_interface",
    "g_type_module_get_type",
    "g_type_module_register_enum",
    "g_type_module_register_flags",
    "g_type_module_register_type",
    "g_type_module_set_name",
    "g_type_module_unuse",
    "g_type_module_use",
    "g_type_name",
    "g_type_name_from_class",
    "g_type_name_from_instance",
    "g_type_next_base",
    "g_type_parent",
    "g_type_plugin_complete_interface_info",
    "g_type_plugin_complete_type_info",
    "g_type_plugin_get_type",
    "g_type_plugin_unuse",
    "g_type_plugin_use",
    "g_type_qname",
    "g_type_query",
    "g_type_register_dynamic",
    "g_type_register_fundamental",
    "g_type_register_static",
    "g_type_register_static_simple",
    "g_type_remove_class_cache_func",
    "g_type_remove_interface_check",
    "g_type_set_qdata",
    "g_type_test_flags",
    "g_type_value_table_peek",
    "g_value_array_append",
    "g_value_array_copy",
    "g_value_array_free",
    "g_value_array_get_nth",
    "g_value_array_get_type",
    "g_value_array_insert",
    "g_value_array_new",
    "g_value_array_prepend",
    "g_value_array_remove",
    "g_value_array_sort",
    "g_value_array_sort_with_data",
    "g_value_copy",
    "g_value_dup_boxed",
    "g_value_dup_object",
    "g_value_dup_param",
    "g_value_dup_string",
    "g_value_dup_variant",
    "g_value_fits_pointer",
    "g_value_get_boolean",
    "g_value_get_boxed",
    "g_value_get_char",
    "g_value_get_double",
    "g_value_get_enum",
    "g_value_get_flags",
    "g_value_get_float",
    "g_value_get_gtype",
    "g_value_get_int",
    "g_value_get_int64",
    "g_value_get_long",
    "g_value_get_object",
    "g_value_get_param",
    "g_value_get_pointer",
    "g_value_get_schar",
    "g_value_get_string",
    "g_value_get_type",
    "g_value_get_uchar",
    "g_value_get_uint",
    "g_value_get_uint64",
    "g_value_get_ulong",
    "g_value_get_variant",
    "g_value_init",
    "g_value_init_from_instance",
    "g_value_peek_pointer",
    "g_value_register_transform_func",
    "g_value_reset",
    "g_value_set_boolean",
    "g_value_set_boxed",
    "g_value_set_boxed_take_ownership",
    "g_value_set_char",
    "g_value_set_double",
    "g_value_set_enum",
    "g_value_set_flags",
    "g_value_set_float",
    "g_value_set_gtype",
    "g_value_set_instance",
    "g_value_set_int",
    "g_value_set_int64",
    "g_value_set_long",
    "g_value_set_object",
    "g_value_set_object_take_ownership",
    "g_value_set_param",
    "g_value_set_param_take_ownership",
    "g_value_set_pointer",
    "g_value_set_schar",
    "g_value_set_static_boxed",
    "g_value_set_static_string",
    "g_value_set_string",
    "g_value_set_string_take_ownership",
    "g_value_set_uchar",
    "g_value_set_uint",
    "g_value_set_uint64",
    "g_value_set_ulong",
    "g_value_set_variant",
    "g_value_take_boxed",
    "g_value_take_object",
    "g_value_take_param",
    "g_value_take_string",
    "g_value_take_variant",
    "g_value_transform",
    "g_value_type_compatible",
    "g_value_type_transformable",
    "g_value_unset",
    "g_variant_builder_get_type",
    "g_variant_dict_get_type",
    "g_variant_get_gtype",
    "g_variant_type_get_gtype",
    "g_weak_ref_clear",
    "g_weak_ref_get",
    "g_weak_ref_init",
    "g_weak_ref_set",
    0
};

static char *variables[] = {
    "g_param_spec_types",
    0
};

/* 1 if present, 0 if not */
int libgobject_2_0_is_present __attribute__((visibility("hidden"))) = 0;

static void *handle = 0;

#if defined(PIC)
  #if defined( __i386__ )
    /* NOTE: Assumes GOT is in ebx. Probably the case since the function
	is unlikely to be called from outside the module. */
    #define ASM_VAR(x)      #x "@GOTOFF(%ebx)"
    #define ASM_FUNC(x)     #x "@PLT"
    #define ASM_FIXUPVAR(x) #x "@GOTOFF(%%ebx)"
  #elif defined( __x86_64__ )
    #define ASM_VAR(x)      #x "@GOTPCREL(%rip)"
    #define ASM_FUNC(x)     #x "@PLT"
    #define ASM_FIXUPVAR(x) #x "@GOTPCREL(%%rip)"
  #endif
#endif

#ifndef ASM_VAR
  #define ASM_VAR(x)    #x
#endif
#ifndef ASM_FUNC
  #define ASM_FUNC(x)   #x
#endif
#ifndef ASM_FIXUPVAR
  #define ASM_FIXUPVAR(x) #x
#endif

/* 1 if present, 0 if not, 0 with warning to stderr if lib not present or symbol not found */
int __attribute__((visibility("hidden"))) libgobject_2_0_symbol_is_present(char *s)
{
    int i;

    if( !libgobject_2_0_is_present ) {
        fprintf(stderr, "%s: relaytool: libgobject-2.0.so not present so cannot check for symbol %s.\n", getenv("_"), s);
        fprintf(stderr, "%s: relaytool: This probably indicates a bug in the application, please report.\n", getenv("_"));
        return 0;
    }

    i = 0;
    while (functions[i++]) if (!strcmp( functions[i - 1], s )) return ptrs[i - 1] > 0 ? 1 : 0;
    i = 0;
    while (variables[i++]) if (!strcmp( variables[i - 1], s )) return dlsym( handle, s ) > 0 ? 1 : 0;

    fprintf( stderr, "%s: relaytool: %s is an unknown symbol in libgobject-2.0.so.\n", getenv("_"), s );
    fprintf( stderr, "%s: relaytool: If you are the developer of this program, please correct the symbol name or rerun relaytool.\n", getenv("_") );
    return 0;
}

__attribute__((noreturn)) __attribute__((visibility("hidden"))) void _relaytool_stubcall_libgobject_2_0(int offset)
{
    fprintf( stderr, "%s: relaytool: stub call to libgobject-2.0.so:%s, aborting.\n", getenv("_"),
             functions[offset / sizeof(void*)] );
    exit( 1 );
}

#if defined( __i386__ )
    #define FIXUP_GOT_RELOC(sym, addr) \
        asm("\tmovl %0, %%eax\n" \
            "\tmovl %%eax, " ASM_FIXUPVAR (sym) "\n" : : "r" (addr));
#elif defined( __powerpc__ )

    /* The PowerPC ELF ABI is a twisted nightmare. Until I figure it out,
       for now we don't support GOT fixup on this architecture */
        
    #error Variables are not currently supported on PowerPC
        
#elif defined( __x86_64__ )
    #define FIXUP_GOT_RELOC(sym, addr) \
        asm("\tmovq %0, %%rax\n" \
            "\tmovq %%rax, " ASM_FIXUPVAR (sym) "\n" : : "r" (addr));
#else        
    #error Please define FIXUP_GOT_RELOC for your architecture
#endif

void __attribute__((constructor)) __attribute__((visibility("hidden"))) _relaytool_init_libgobject_2_0()
{
    int i = 0;

    ptrs = malloc( sizeof(functions) );
    memset( ptrs, 0, sizeof(functions) );
handle = dlopen( "libgobject-2.0.so.0", RTLD_LAZY );
    if (!handle) return;
    
    libgobject_2_0_is_present = 1;

    /* build function jumptable */
    while (functions[i++]) ptrs[i - 1] = dlsym( handle, functions[i - 1] );
    
    /* now fixup the global offset table for variable imports */
    void* symptr;
    symptr = dlsym(handle, "g_param_spec_types");
    FIXUP_GOT_RELOC( g_param_spec_types, symptr );
}

#if defined( __i386__ )

#define JUMP_SLOT(name, index)  \
    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n"  \
        name ":\n" \
        "        movl " ASM_VAR (ptrs) ", %eax\n" \
        "        movl " #index "(%eax), %eax\n" \
        "        test %eax, %eax\n" \
        "        jnz  JS" name "\n" \
        "        push $" #index "\n" \
        "        call " ASM_FUNC (_relaytool_stubcall_libgobject_2_0) "\n" \
        "JS" name ":    jmp *%eax\n");


#elif defined( __x86_64__ )

#define JUMP_SLOT(name, index)  \
    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n"  \
        name ":\n" \
        "        movq " ASM_VAR (ptrs) ", %r11\n" \
        "        movq (%r11), %r11\n" \
        "        movq " #index "(%r11), %r11\n" \
        "        test %r11, %r11\n" \
        "        jnz  JS" name "\n" \
        "        push $" #index "\n" \
        "        call " ASM_FUNC (_relaytool_stubcall_libgobject_2_0) "\n" \
        "JS" name ":    jmp *%r11\n");
#elif defined( __powerpc__ )

#define JUMP_SLOT(name, index) \                                    asm(".section .text." name ", \"ax\", @progbits\n" \
        ".globl  " name "\n" \
        ".hidden " name "\n" \
        "        .type " name ", @function\n" \
        name ":\n" \
        "        lis r11, ptrs@ha\n" \
        "        lwz r11, " #index "(r11)\n" \
        "        cmpi cr0,r11,0\n" \
        "        beq- 1f\n" \
        "        mtctr r11\n" \
        "        bctr\n" \
        "1:      li r3, " #index "\n" \
        "        b _relaytool_stubcall_libgobject_2_0\n" \
        );
        
#else        
    #error Please define JUMP_SLOT for your architecture
#endif

/* define placeholders for the variable imports: their type doesn't matter,
   however we must restrict ELF symbol scope to prevent the definition in the imported
   shared library being bound to this dummy symbol (not all libs are compiled -Bsymbolic)
 */
int g_param_spec_types __attribute__(( visibility("hidden") )) = -1;

/* define each jump slot in its own section. this increases generated code
   size, but it means unused slots can be deleted by the linker when
   --gc-sections is used.
 */
JUMP_SLOT("g_array_get_type", 0);
JUMP_SLOT("g_binding_flags_get_type", 8);
JUMP_SLOT("g_binding_get_flags", 16);
JUMP_SLOT("g_binding_get_source", 24);
JUMP_SLOT("g_binding_get_source_property", 32);
JUMP_SLOT("g_binding_get_target", 40);
JUMP_SLOT("g_binding_get_target_property", 48);
JUMP_SLOT("g_binding_get_type", 56);
JUMP_SLOT("g_binding_unbind", 64);
JUMP_SLOT("g_boxed_copy", 72);
JUMP_SLOT("g_boxed_free", 80);
JUMP_SLOT("g_boxed_type_register_static", 88);
JUMP_SLOT("g_byte_array_get_type", 96);
JUMP_SLOT("g_bytes_get_type", 104);
JUMP_SLOT("g_cclosure_marshal_BOOLEAN__BOXED_BOXED", 112);
JUMP_SLOT("g_cclosure_marshal_BOOLEAN__BOXED_BOXEDv", 120);
JUMP_SLOT("g_cclosure_marshal_BOOLEAN__FLAGS", 128);
JUMP_SLOT("g_cclosure_marshal_BOOLEAN__FLAGSv", 136);
JUMP_SLOT("g_cclosure_marshal_STRING__OBJECT_POINTER", 144);
JUMP_SLOT("g_cclosure_marshal_STRING__OBJECT_POINTERv", 152);
JUMP_SLOT("g_cclosure_marshal_VOID__BOOLEAN", 160);
JUMP_SLOT("g_cclosure_marshal_VOID__BOOLEANv", 168);
JUMP_SLOT("g_cclosure_marshal_VOID__BOXED", 176);
JUMP_SLOT("g_cclosure_marshal_VOID__BOXEDv", 184);
JUMP_SLOT("g_cclosure_marshal_VOID__CHAR", 192);
JUMP_SLOT("g_cclosure_marshal_VOID__CHARv", 200);
JUMP_SLOT("g_cclosure_marshal_VOID__DOUBLE", 208);
JUMP_SLOT("g_cclosure_marshal_VOID__DOUBLEv", 216);
JUMP_SLOT("g_cclosure_marshal_VOID__ENUM", 224);
JUMP_SLOT("g_cclosure_marshal_VOID__ENUMv", 232);
JUMP_SLOT("g_cclosure_marshal_VOID__FLAGS", 240);
JUMP_SLOT("g_cclosure_marshal_VOID__FLAGSv", 248);
JUMP_SLOT("g_cclosure_marshal_VOID__FLOAT", 256);
JUMP_SLOT("g_cclosure_marshal_VOID__FLOATv", 264);
JUMP_SLOT("g_cclosure_marshal_VOID__INT", 272);
JUMP_SLOT("g_cclosure_marshal_VOID__INTv", 280);
JUMP_SLOT("g_cclosure_marshal_VOID__LONG", 288);
JUMP_SLOT("g_cclosure_marshal_VOID__LONGv", 296);
JUMP_SLOT("g_cclosure_marshal_VOID__OBJECT", 304);
JUMP_SLOT("g_cclosure_marshal_VOID__OBJECTv", 312);
JUMP_SLOT("g_cclosure_marshal_VOID__PARAM", 320);
JUMP_SLOT("g_cclosure_marshal_VOID__PARAMv", 328);
JUMP_SLOT("g_cclosure_marshal_VOID__POINTER", 336);
JUMP_SLOT("g_cclosure_marshal_VOID__POINTERv", 344);
JUMP_SLOT("g_cclosure_marshal_VOID__STRING", 352);
JUMP_SLOT("g_cclosure_marshal_VOID__STRINGv", 360);
JUMP_SLOT("g_cclosure_marshal_VOID__UCHAR", 368);
JUMP_SLOT("g_cclosure_marshal_VOID__UCHARv", 376);
JUMP_SLOT("g_cclosure_marshal_VOID__UINT", 384);
JUMP_SLOT("g_cclosure_marshal_VOID__UINT_POINTER", 392);
JUMP_SLOT("g_cclosure_marshal_VOID__UINT_POINTERv", 400);
JUMP_SLOT("g_cclosure_marshal_VOID__UINTv", 408);
JUMP_SLOT("g_cclosure_marshal_VOID__ULONG", 416);
JUMP_SLOT("g_cclosure_marshal_VOID__ULONGv", 424);
JUMP_SLOT("g_cclosure_marshal_VOID__VARIANT", 432);
JUMP_SLOT("g_cclosure_marshal_VOID__VARIANTv", 440);
JUMP_SLOT("g_cclosure_marshal_VOID__VOID", 448);
JUMP_SLOT("g_cclosure_marshal_VOID__VOIDv", 456);
JUMP_SLOT("g_cclosure_marshal_generic", 464);
JUMP_SLOT("g_cclosure_marshal_generic_va", 472);
JUMP_SLOT("g_cclosure_new", 480);
JUMP_SLOT("g_cclosure_new_object", 488);
JUMP_SLOT("g_cclosure_new_object_swap", 496);
JUMP_SLOT("g_cclosure_new_swap", 504);
JUMP_SLOT("g_checksum_get_type", 512);
JUMP_SLOT("g_clear_object", 520);
JUMP_SLOT("g_closure_add_finalize_notifier", 528);
JUMP_SLOT("g_closure_add_invalidate_notifier", 536);
JUMP_SLOT("g_closure_add_marshal_guards", 544);
JUMP_SLOT("g_closure_get_type", 552);
JUMP_SLOT("g_closure_invalidate", 560);
JUMP_SLOT("g_closure_invoke", 568);
JUMP_SLOT("g_closure_new_object", 576);
JUMP_SLOT("g_closure_new_simple", 584);
JUMP_SLOT("g_closure_ref", 592);
JUMP_SLOT("g_closure_remove_finalize_notifier", 600);
JUMP_SLOT("g_closure_remove_invalidate_notifier", 608);
JUMP_SLOT("g_closure_set_marshal", 616);
JUMP_SLOT("g_closure_set_meta_marshal", 624);
JUMP_SLOT("g_closure_sink", 632);
JUMP_SLOT("g_closure_unref", 640);
JUMP_SLOT("g_date_get_type", 648);
JUMP_SLOT("g_date_time_get_type", 656);
JUMP_SLOT("g_enum_complete_type_info", 664);
JUMP_SLOT("g_enum_get_value", 672);
JUMP_SLOT("g_enum_get_value_by_name", 680);
JUMP_SLOT("g_enum_get_value_by_nick", 688);
JUMP_SLOT("g_enum_register_static", 696);
JUMP_SLOT("g_enum_to_string", 704);
JUMP_SLOT("g_error_get_type", 712);
JUMP_SLOT("g_flags_complete_type_info", 720);
JUMP_SLOT("g_flags_get_first_value", 728);
JUMP_SLOT("g_flags_get_value_by_name", 736);
JUMP_SLOT("g_flags_get_value_by_nick", 744);
JUMP_SLOT("g_flags_register_static", 752);
JUMP_SLOT("g_flags_to_string", 760);
JUMP_SLOT("g_gstring_get_type", 768);
JUMP_SLOT("g_gtype_get_type", 776);
JUMP_SLOT("g_hash_table_get_type", 784);
JUMP_SLOT("g_initially_unowned_get_type", 792);
JUMP_SLOT("g_io_channel_get_type", 800);
JUMP_SLOT("g_io_condition_get_type", 808);
JUMP_SLOT("g_key_file_get_type", 816);
JUMP_SLOT("g_main_context_get_type", 824);
JUMP_SLOT("g_main_loop_get_type", 832);
JUMP_SLOT("g_mapped_file_get_type", 840);
JUMP_SLOT("g_markup_parse_context_get_type", 848);
JUMP_SLOT("g_match_info_get_type", 856);
JUMP_SLOT("g_object_add_toggle_ref", 864);
JUMP_SLOT("g_object_add_weak_pointer", 872);
JUMP_SLOT("g_object_bind_property", 880);
JUMP_SLOT("g_object_bind_property_full", 888);
JUMP_SLOT("g_object_bind_property_with_closures", 896);
JUMP_SLOT("g_object_class_find_property", 904);
JUMP_SLOT("g_object_class_install_properties", 912);
JUMP_SLOT("g_object_class_install_property", 920);
JUMP_SLOT("g_object_class_list_properties", 928);
JUMP_SLOT("g_object_class_override_property", 936);
JUMP_SLOT("g_object_compat_control", 944);
JUMP_SLOT("g_object_connect", 952);
JUMP_SLOT("g_object_disconnect", 960);
JUMP_SLOT("g_object_dup_data", 968);
JUMP_SLOT("g_object_dup_qdata", 976);
JUMP_SLOT("g_object_force_floating", 984);
JUMP_SLOT("g_object_freeze_notify", 992);
JUMP_SLOT("g_object_get", 1000);
JUMP_SLOT("g_object_get_data", 1008);
JUMP_SLOT("g_object_get_property", 1016);
JUMP_SLOT("g_object_get_qdata", 1024);
JUMP_SLOT("g_object_get_type", 1032);
JUMP_SLOT("g_object_get_valist", 1040);
JUMP_SLOT("g_object_getv", 1048);
JUMP_SLOT("g_object_interface_find_property", 1056);
JUMP_SLOT("g_object_interface_install_property", 1064);
JUMP_SLOT("g_object_interface_list_properties", 1072);
JUMP_SLOT("g_object_is_floating", 1080);
JUMP_SLOT("g_object_new", 1088);
JUMP_SLOT("g_object_new_valist", 1096);
JUMP_SLOT("g_object_new_with_properties", 1104);
JUMP_SLOT("g_object_newv", 1112);
JUMP_SLOT("g_object_notify", 1120);
JUMP_SLOT("g_object_notify_by_pspec", 1128);
JUMP_SLOT("g_object_ref", 1136);
JUMP_SLOT("g_object_ref_sink", 1144);
JUMP_SLOT("g_object_remove_toggle_ref", 1152);
JUMP_SLOT("g_object_remove_weak_pointer", 1160);
JUMP_SLOT("g_object_replace_data", 1168);
JUMP_SLOT("g_object_replace_qdata", 1176);
JUMP_SLOT("g_object_run_dispose", 1184);
JUMP_SLOT("g_object_set", 1192);
JUMP_SLOT("g_object_set_data", 1200);
JUMP_SLOT("g_object_set_data_full", 1208);
JUMP_SLOT("g_object_set_property", 1216);
JUMP_SLOT("g_object_set_qdata", 1224);
JUMP_SLOT("g_object_set_qdata_full", 1232);
JUMP_SLOT("g_object_set_valist", 1240);
JUMP_SLOT("g_object_setv", 1248);
JUMP_SLOT("g_object_steal_data", 1256);
JUMP_SLOT("g_object_steal_qdata", 1264);
JUMP_SLOT("g_object_thaw_notify", 1272);
JUMP_SLOT("g_object_unref", 1280);
JUMP_SLOT("g_object_watch_closure", 1288);
JUMP_SLOT("g_object_weak_ref", 1296);
JUMP_SLOT("g_object_weak_unref", 1304);
JUMP_SLOT("g_option_group_get_type", 1312);
JUMP_SLOT("g_param_spec_boolean", 1320);
JUMP_SLOT("g_param_spec_boxed", 1328);
JUMP_SLOT("g_param_spec_char", 1336);
JUMP_SLOT("g_param_spec_double", 1344);
JUMP_SLOT("g_param_spec_enum", 1352);
JUMP_SLOT("g_param_spec_flags", 1360);
JUMP_SLOT("g_param_spec_float", 1368);
JUMP_SLOT("g_param_spec_get_blurb", 1376);
JUMP_SLOT("g_param_spec_get_default_value", 1384);
JUMP_SLOT("g_param_spec_get_name", 1392);
JUMP_SLOT("g_param_spec_get_name_quark", 1400);
JUMP_SLOT("g_param_spec_get_nick", 1408);
JUMP_SLOT("g_param_spec_get_qdata", 1416);
JUMP_SLOT("g_param_spec_get_redirect_target", 1424);
JUMP_SLOT("g_param_spec_gtype", 1432);
JUMP_SLOT("g_param_spec_int", 1440);
JUMP_SLOT("g_param_spec_int64", 1448);
JUMP_SLOT("g_param_spec_internal", 1456);
JUMP_SLOT("g_param_spec_long", 1464);
JUMP_SLOT("g_param_spec_object", 1472);
JUMP_SLOT("g_param_spec_override", 1480);
JUMP_SLOT("g_param_spec_param", 1488);
JUMP_SLOT("g_param_spec_pointer", 1496);
JUMP_SLOT("g_param_spec_pool_insert", 1504);
JUMP_SLOT("g_param_spec_pool_list", 1512);
JUMP_SLOT("g_param_spec_pool_list_owned", 1520);
JUMP_SLOT("g_param_spec_pool_lookup", 1528);
JUMP_SLOT("g_param_spec_pool_new", 1536);
JUMP_SLOT("g_param_spec_pool_remove", 1544);
JUMP_SLOT("g_param_spec_ref", 1552);
JUMP_SLOT("g_param_spec_ref_sink", 1560);
JUMP_SLOT("g_param_spec_set_qdata", 1568);
JUMP_SLOT("g_param_spec_set_qdata_full", 1576);
JUMP_SLOT("g_param_spec_sink", 1584);
JUMP_SLOT("g_param_spec_steal_qdata", 1592);
JUMP_SLOT("g_param_spec_string", 1600);
JUMP_SLOT("g_param_spec_uchar", 1608);
JUMP_SLOT("g_param_spec_uint", 1616);
JUMP_SLOT("g_param_spec_uint64", 1624);
JUMP_SLOT("g_param_spec_ulong", 1632);
JUMP_SLOT("g_param_spec_unichar", 1640);
JUMP_SLOT("g_param_spec_unref", 1648);
JUMP_SLOT("g_param_spec_value_array", 1656);
JUMP_SLOT("g_param_spec_variant", 1664);
JUMP_SLOT("g_param_type_register_static", 1672);
JUMP_SLOT("g_param_value_convert", 1680);
JUMP_SLOT("g_param_value_defaults", 1688);
JUMP_SLOT("g_param_value_set_default", 1696);
JUMP_SLOT("g_param_value_validate", 1704);
JUMP_SLOT("g_param_values_cmp", 1712);
JUMP_SLOT("g_pointer_type_register_static", 1720);
JUMP_SLOT("g_pollfd_get_type", 1728);
JUMP_SLOT("g_ptr_array_get_type", 1736);
JUMP_SLOT("g_regex_get_type", 1744);
JUMP_SLOT("g_signal_accumulator_first_wins", 1752);
JUMP_SLOT("g_signal_accumulator_true_handled", 1760);
JUMP_SLOT("g_signal_add_emission_hook", 1768);
JUMP_SLOT("g_signal_chain_from_overridden", 1776);
JUMP_SLOT("g_signal_chain_from_overridden_handler", 1784);
JUMP_SLOT("g_signal_connect_closure", 1792);
JUMP_SLOT("g_signal_connect_closure_by_id", 1800);
JUMP_SLOT("g_signal_connect_data", 1808);
JUMP_SLOT("g_signal_connect_object", 1816);
JUMP_SLOT("g_signal_emit", 1824);
JUMP_SLOT("g_signal_emit_by_name", 1832);
JUMP_SLOT("g_signal_emit_valist", 1840);
JUMP_SLOT("g_signal_emitv", 1848);
JUMP_SLOT("g_signal_get_invocation_hint", 1856);
JUMP_SLOT("g_signal_handler_block", 1864);
JUMP_SLOT("g_signal_handler_disconnect", 1872);
JUMP_SLOT("g_signal_handler_find", 1880);
JUMP_SLOT("g_signal_handler_is_connected", 1888);
JUMP_SLOT("g_signal_handler_unblock", 1896);
JUMP_SLOT("g_signal_handlers_block_matched", 1904);
JUMP_SLOT("g_signal_handlers_destroy", 1912);
JUMP_SLOT("g_signal_handlers_disconnect_matched", 1920);
JUMP_SLOT("g_signal_handlers_unblock_matched", 1928);
JUMP_SLOT("g_signal_has_handler_pending", 1936);
JUMP_SLOT("g_signal_list_ids", 1944);
JUMP_SLOT("g_signal_lookup", 1952);
JUMP_SLOT("g_signal_name", 1960);
JUMP_SLOT("g_signal_new", 1968);
JUMP_SLOT("g_signal_new_class_handler", 1976);
JUMP_SLOT("g_signal_new_valist", 1984);
JUMP_SLOT("g_signal_newv", 1992);
JUMP_SLOT("g_signal_override_class_closure", 2000);
JUMP_SLOT("g_signal_override_class_handler", 2008);
JUMP_SLOT("g_signal_parse_name", 2016);
JUMP_SLOT("g_signal_query", 2024);
JUMP_SLOT("g_signal_remove_emission_hook", 2032);
JUMP_SLOT("g_signal_set_va_marshaller", 2040);
JUMP_SLOT("g_signal_stop_emission", 2048);
JUMP_SLOT("g_signal_stop_emission_by_name", 2056);
JUMP_SLOT("g_signal_type_cclosure_new", 2064);
JUMP_SLOT("g_source_get_type", 2072);
JUMP_SLOT("g_source_set_closure", 2080);
JUMP_SLOT("g_source_set_dummy_callback", 2088);
JUMP_SLOT("g_strdup_value_contents", 2096);
JUMP_SLOT("g_strv_get_type", 2104);
JUMP_SLOT("g_thread_get_type", 2112);
JUMP_SLOT("g_time_zone_get_type", 2120);
JUMP_SLOT("g_type_add_class_cache_func", 2128);
JUMP_SLOT("g_type_add_class_private", 2136);
JUMP_SLOT("g_type_add_instance_private", 2144);
JUMP_SLOT("g_type_add_interface_check", 2152);
JUMP_SLOT("g_type_add_interface_dynamic", 2160);
JUMP_SLOT("g_type_add_interface_static", 2168);
JUMP_SLOT("g_type_check_class_cast", 2176);
JUMP_SLOT("g_type_check_class_is_a", 2184);
JUMP_SLOT("g_type_check_instance", 2192);
JUMP_SLOT("g_type_check_instance_cast", 2200);
JUMP_SLOT("g_type_check_instance_is_a", 2208);
JUMP_SLOT("g_type_check_instance_is_fundamentally_a", 2216);
JUMP_SLOT("g_type_check_is_value_type", 2224);
JUMP_SLOT("g_type_check_value", 2232);
JUMP_SLOT("g_type_check_value_holds", 2240);
JUMP_SLOT("g_type_children", 2248);
JUMP_SLOT("g_type_class_add_private", 2256);
JUMP_SLOT("g_type_class_adjust_private_offset", 2264);
JUMP_SLOT("g_type_class_get_instance_private_offset", 2272);
JUMP_SLOT("g_type_class_get_private", 2280);
JUMP_SLOT("g_type_class_peek", 2288);
JUMP_SLOT("g_type_class_peek_parent", 2296);
JUMP_SLOT("g_type_class_peek_static", 2304);
JUMP_SLOT("g_type_class_ref", 2312);
JUMP_SLOT("g_type_class_unref", 2320);
JUMP_SLOT("g_type_class_unref_uncached", 2328);
JUMP_SLOT("g_type_create_instance", 2336);
JUMP_SLOT("g_type_default_interface_peek", 2344);
JUMP_SLOT("g_type_default_interface_ref", 2352);
JUMP_SLOT("g_type_default_interface_unref", 2360);
JUMP_SLOT("g_type_depth", 2368);
JUMP_SLOT("g_type_ensure", 2376);
JUMP_SLOT("g_type_free_instance", 2384);
JUMP_SLOT("g_type_from_name", 2392);
JUMP_SLOT("g_type_fundamental", 2400);
JUMP_SLOT("g_type_fundamental_next", 2408);
JUMP_SLOT("g_type_get_instance_count", 2416);
JUMP_SLOT("g_type_get_plugin", 2424);
JUMP_SLOT("g_type_get_qdata", 2432);
JUMP_SLOT("g_type_get_type_registration_serial", 2440);
JUMP_SLOT("g_type_init", 2448);
JUMP_SLOT("g_type_init_with_debug_flags", 2456);
JUMP_SLOT("g_type_instance_get_private", 2464);
JUMP_SLOT("g_type_interface_add_prerequisite", 2472);
JUMP_SLOT("g_type_interface_get_plugin", 2480);
JUMP_SLOT("g_type_interface_peek", 2488);
JUMP_SLOT("g_type_interface_peek_parent", 2496);
JUMP_SLOT("g_type_interface_prerequisites", 2504);
JUMP_SLOT("g_type_interfaces", 2512);
JUMP_SLOT("g_type_is_a", 2520);
JUMP_SLOT("g_type_module_add_interface", 2528);
JUMP_SLOT("g_type_module_get_type", 2536);
JUMP_SLOT("g_type_module_register_enum", 2544);
JUMP_SLOT("g_type_module_register_flags", 2552);
JUMP_SLOT("g_type_module_register_type", 2560);
JUMP_SLOT("g_type_module_set_name", 2568);
JUMP_SLOT("g_type_module_unuse", 2576);
JUMP_SLOT("g_type_module_use", 2584);
JUMP_SLOT("g_type_name", 2592);
JUMP_SLOT("g_type_name_from_class", 2600);
JUMP_SLOT("g_type_name_from_instance", 2608);
JUMP_SLOT("g_type_next_base", 2616);
JUMP_SLOT("g_type_parent", 2624);
JUMP_SLOT("g_type_plugin_complete_interface_info", 2632);
JUMP_SLOT("g_type_plugin_complete_type_info", 2640);
JUMP_SLOT("g_type_plugin_get_type", 2648);
JUMP_SLOT("g_type_plugin_unuse", 2656);
JUMP_SLOT("g_type_plugin_use", 2664);
JUMP_SLOT("g_type_qname", 2672);
JUMP_SLOT("g_type_query", 2680);
JUMP_SLOT("g_type_register_dynamic", 2688);
JUMP_SLOT("g_type_register_fundamental", 2696);
JUMP_SLOT("g_type_register_static", 2704);
JUMP_SLOT("g_type_register_static_simple", 2712);
JUMP_SLOT("g_type_remove_class_cache_func", 2720);
JUMP_SLOT("g_type_remove_interface_check", 2728);
JUMP_SLOT("g_type_set_qdata", 2736);
JUMP_SLOT("g_type_test_flags", 2744);
JUMP_SLOT("g_type_value_table_peek", 2752);
JUMP_SLOT("g_value_array_append", 2760);
JUMP_SLOT("g_value_array_copy", 2768);
JUMP_SLOT("g_value_array_free", 2776);
JUMP_SLOT("g_value_array_get_nth", 2784);
JUMP_SLOT("g_value_array_get_type", 2792);
JUMP_SLOT("g_value_array_insert", 2800);
JUMP_SLOT("g_value_array_new", 2808);
JUMP_SLOT("g_value_array_prepend", 2816);
JUMP_SLOT("g_value_array_remove", 2824);
JUMP_SLOT("g_value_array_sort", 2832);
JUMP_SLOT("g_value_array_sort_with_data", 2840);
JUMP_SLOT("g_value_copy", 2848);
JUMP_SLOT("g_value_dup_boxed", 2856);
JUMP_SLOT("g_value_dup_object", 2864);
JUMP_SLOT("g_value_dup_param", 2872);
JUMP_SLOT("g_value_dup_string", 2880);
JUMP_SLOT("g_value_dup_variant", 2888);
JUMP_SLOT("g_value_fits_pointer", 2896);
JUMP_SLOT("g_value_get_boolean", 2904);
JUMP_SLOT("g_value_get_boxed", 2912);
JUMP_SLOT("g_value_get_char", 2920);
JUMP_SLOT("g_value_get_double", 2928);
JUMP_SLOT("g_value_get_enum", 2936);
JUMP_SLOT("g_value_get_flags", 2944);
JUMP_SLOT("g_value_get_float", 2952);
JUMP_SLOT("g_value_get_gtype", 2960);
JUMP_SLOT("g_value_get_int", 2968);
JUMP_SLOT("g_value_get_int64", 2976);
JUMP_SLOT("g_value_get_long", 2984);
JUMP_SLOT("g_value_get_object", 2992);
JUMP_SLOT("g_value_get_param", 3000);
JUMP_SLOT("g_value_get_pointer", 3008);
JUMP_SLOT("g_value_get_schar", 3016);
JUMP_SLOT("g_value_get_string", 3024);
JUMP_SLOT("g_value_get_type", 3032);
JUMP_SLOT("g_value_get_uchar", 3040);
JUMP_SLOT("g_value_get_uint", 3048);
JUMP_SLOT("g_value_get_uint64", 3056);
JUMP_SLOT("g_value_get_ulong", 3064);
JUMP_SLOT("g_value_get_variant", 3072);
JUMP_SLOT("g_value_init", 3080);
JUMP_SLOT("g_value_init_from_instance", 3088);
JUMP_SLOT("g_value_peek_pointer", 3096);
JUMP_SLOT("g_value_register_transform_func", 3104);
JUMP_SLOT("g_value_reset", 3112);
JUMP_SLOT("g_value_set_boolean", 3120);
JUMP_SLOT("g_value_set_boxed", 3128);
JUMP_SLOT("g_value_set_boxed_take_ownership", 3136);
JUMP_SLOT("g_value_set_char", 3144);
JUMP_SLOT("g_value_set_double", 3152);
JUMP_SLOT("g_value_set_enum", 3160);
JUMP_SLOT("g_value_set_flags", 3168);
JUMP_SLOT("g_value_set_float", 3176);
JUMP_SLOT("g_value_set_gtype", 3184);
JUMP_SLOT("g_value_set_instance", 3192);
JUMP_SLOT("g_value_set_int", 3200);
JUMP_SLOT("g_value_set_int64", 3208);
JUMP_SLOT("g_value_set_long", 3216);
JUMP_SLOT("g_value_set_object", 3224);
JUMP_SLOT("g_value_set_object_take_ownership", 3232);
JUMP_SLOT("g_value_set_param", 3240);
JUMP_SLOT("g_value_set_param_take_ownership", 3248);
JUMP_SLOT("g_value_set_pointer", 3256);
JUMP_SLOT("g_value_set_schar", 3264);
JUMP_SLOT("g_value_set_static_boxed", 3272);
JUMP_SLOT("g_value_set_static_string", 3280);
JUMP_SLOT("g_value_set_string", 3288);
JUMP_SLOT("g_value_set_string_take_ownership", 3296);
JUMP_SLOT("g_value_set_uchar", 3304);
JUMP_SLOT("g_value_set_uint", 3312);
JUMP_SLOT("g_value_set_uint64", 3320);
JUMP_SLOT("g_value_set_ulong", 3328);
JUMP_SLOT("g_value_set_variant", 3336);
JUMP_SLOT("g_value_take_boxed", 3344);
JUMP_SLOT("g_value_take_object", 3352);
JUMP_SLOT("g_value_take_param", 3360);
JUMP_SLOT("g_value_take_string", 3368);
JUMP_SLOT("g_value_take_variant", 3376);
JUMP_SLOT("g_value_transform", 3384);
JUMP_SLOT("g_value_type_compatible", 3392);
JUMP_SLOT("g_value_type_transformable", 3400);
JUMP_SLOT("g_value_unset", 3408);
JUMP_SLOT("g_variant_builder_get_type", 3416);
JUMP_SLOT("g_variant_dict_get_type", 3424);
JUMP_SLOT("g_variant_get_gtype", 3432);
JUMP_SLOT("g_variant_type_get_gtype", 3440);
JUMP_SLOT("g_weak_ref_clear", 3448);
JUMP_SLOT("g_weak_ref_get", 3456);
JUMP_SLOT("g_weak_ref_init", 3464);
JUMP_SLOT("g_weak_ref_set", 3472);


#ifdef __cplusplus
    }
#endif
