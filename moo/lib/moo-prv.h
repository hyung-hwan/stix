/*
 * $Id$
 *
    Copyright (c) 2014-2019 Chung, Hyung-Hwan. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MOO_PRV_H_
#define _MOO_PRV_H_

#include <moo.h>
#include <moo-chr.h>
#include <moo-fmt.h>
#include <moo-utl.h>

/* you can define this to either 1 or 2 */
#define MOO_BCODE_LONG_PARAM_SIZE 2

/* this is useful for debugging. moo_gc() can be called 
 * while moo has not been fully initialized when this is defined*/
#define MOO_SUPPORT_GC_DURING_IGNITION

/* define this to generate XXXX_CTXTEMVAR instructions */
#define MOO_USE_CTXTEMPVAR

/* define this to enable karatsuba multiplication in bigint */
#define MOO_ENABLE_KARATSUBA
#define MOO_KARATSUBA_CUTOFF 32
#define MOO_KARATSUBA_CUTOFF_DEBUG 3

/* enable floating-pointer number support in the basic formatting functions */
#define MOO_ENABLE_FLTFMT

/* allow the caller to drive process switching by calling
 * moo_switchprocess(). */
#define MOO_EXTERNAL_PROCESS_SWITCH

/* limit the maximum object size such that:
 *   1. an index to an object field can be represented in a small integer.
 *   2. the maximum number of bits including bit-shifts can be represented
 *      in the moo_oow_t type.
 */
#define MOO_LIMIT_OBJ_SIZE


#if defined(MOO_BUILD_DEBUG)
/*#define MOO_DEBUG_LEXER 1*/
#define MOO_DEBUG_COMPILER 1
#define MOO_DEBUG_VM_PROCESSOR 1
/*#define MOO_DEBUG_VM_EXEC 1*/
#define MOO_PROFILE_VM 1
#endif /* MOO_BUILD_DEBUG */


#if defined(__has_builtin)

#	if (!__has_builtin(__builtin_memset) || !__has_builtin(__builtin_memcpy) || !__has_builtin(__builtin_memmove) || !__has_builtin(__builtin_memcmp))
#	include <string.h>
#	endif

#	if __has_builtin(__builtin_memset)
#		define MOO_MEMSET(dst,src,size)  __builtin_memset(dst,src,size)
#	else
#		define MOO_MEMSET(dst,src,size)  memset(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memcpy)
#		define MOO_MEMCPY(dst,src,size)  __builtin_memcpy(dst,src,size)
#	else
#		define MOO_MEMCPY(dst,src,size)  memcpy(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memmove)
#		define MOO_MEMMOVE(dst,src,size)  __builtin_memmove(dst,src,size)
#	else
#		define MOO_MEMMOVE(dst,src,size)  memmove(dst,src,size)
#	endif
#	if __has_builtin(__builtin_memcmp)
#		define MOO_MEMCMP(dst,src,size)  __builtin_memcmp(dst,src,size)
#	else
#		define MOO_MEMCMP(dst,src,size)  memcmp(dst,src,size)
#	endif

#else

#	if !defined(HAVE___BUILTIN_MEMSET) || !defined(HAVE___BUILTIN_MEMCPY) || !defined(HAVE___BUILTIN_MEMMOVE) || !defined(HAVE___BUILTIN_MEMCMP)
#	include <string.h>
#	endif

#	if defined(HAVE___BUILTIN_MEMSET)
#		define MOO_MEMSET(dst,src,size)  __builtin_memset(dst,src,size)
#	else
#		define MOO_MEMSET(dst,src,size)  memset(dst,src,size)
#	endif
#	if defined(HAVE___BUILTIN_MEMCPY)
#		define MOO_MEMCPY(dst,src,size)  __builtin_memcpy(dst,src,size)
#	else
#		define MOO_MEMCPY(dst,src,size)  memcpy(dst,src,size)
#	endif
#	if defined(HAVE___BUILTIN_MEMMOVE)
#		define MOO_MEMMOVE(dst,src,size)  __builtin_memmove(dst,src,size)
#	else
#		define MOO_MEMMOVE(dst,src,size)  memmove(dst,src,size)
#	endif
#	if defined(HAVE___BUILTIN_MEMCMP)
#		define MOO_MEMCMP(dst,src,size)  __builtin_memcmp(dst,src,size)
#	else
#		define MOO_MEMCMP(dst,src,size)  memcmp(dst,src,size)
#	endif

#endif


/* ========================================================================= */
/* CLASS SPEC ENCODING                                                       */
/* ========================================================================= */

/*
 * The spec field of a class object encodes the number of the fixed part
 * and the type of the indexed part. The fixed part is the number of
 * named instance variables. If the spec of a class is indexed, the object
 * of the class can be instantiated with the size of the indexed part.
 *
 * For example, on a platform where sizeof(moo_oow_t) is 4, 
 * the layout of the spec field of a class as an OOP value looks like this:
 * 
 *  31                          12 11  10 9 8 7 6 5 4 3 2   1 0 
 * |number of named instance variables|indexed-type|flags |oop-tag|
 *
 * the number of named instance variables is stored in high 21 bits.
 * the indexed type takes up bit 5 to bit 10 (assuming MOO_OBJ_TYPE_BITS is 6. 
 * MOO_OBJ_TYPE_XXX enumerators are used to represent actual values).
 * and the indexability is stored in the flag bits which span from bit 2 to 4.
 *
 * The maximum number of named(fixed) instance variables for a class is:
 *     2 ^ ((BITS-IN-OOW - MOO_OOP_TAG_BITS_LO) - MOO_OBJ_TYPE_BITS - 1 - 2) - 1
 *
 * MOO_OOP_TAG_BITS_LO are decremented from the number of bits in OOW because
 * the spec field is always encoded as a small integer.
 *
 * The number of named instance variables can be greater than 0 if the
 * class spec is not indexed or if it's a pointer indexed class
 * (indexed_type == MOO_OBJ_TYPE_OOP)
 * 
 * indexed_type is one of the #moo_obj_type_t enumerators.
 */

#define MOO_CLASS_SPEC_FLAG_BITS (3)

/*
 * The MOO_CLASS_SPEC_MAKE() macro creates a class spec value.
 *  _class->spec = MOO_SMOOI_TO_OOP(MOO_CLASS_SPEC_MAKE(0, 1, MOO_OBJ_TYPE_CHAR));
 */
#define MOO_CLASS_SPEC_MAKE(named_instvar,flags,indexed_type) ( \
	(((moo_oow_t)(named_instvar)) << (MOO_OBJ_FLAGS_TYPE_BITS + MOO_CLASS_SPEC_FLAG_BITS)) |  \
	(((moo_oow_t)(indexed_type)) << (MOO_CLASS_SPEC_FLAG_BITS)) | (((moo_oow_t)flags) & MOO_LBMASK(moo_oow_t,MOO_CLASS_SPEC_FLAG_BITS)))

/* what is the number of named instance variables? 
 *  MOO_CLASS_SPEC_NAMED_INSTVARS(MOO_OOP_TO_SMOOI(_class->spec)) 
 * ensure to update Class<<specNumInstVars if you change this macro. */
#define MOO_CLASS_SPEC_NAMED_INSTVARS(spec) \
	(((moo_oow_t)(spec)) >> (MOO_OBJ_FLAGS_TYPE_BITS + MOO_CLASS_SPEC_FLAG_BITS))

/* is it a user-indexable class? 
 * all objects can be indexed with basicAt:.
 * this indicates if an object can be instantiated with a dynamic size
 * (new: size) and and can be indexed with at:.
 */
#define MOO_CLASS_SPEC_FLAGS(spec) (((moo_oow_t)(spec)) & MOO_LBMASK(moo_oow_t,MOO_CLASS_SPEC_FLAG_BITS))

/* if so, what is the indexing type? character? pointer? etc? */
#define MOO_CLASS_SPEC_INDEXED_TYPE(spec) \
	((((moo_oow_t)(spec)) >> MOO_CLASS_SPEC_FLAG_BITS) & MOO_LBMASK(moo_oow_t, MOO_OBJ_FLAGS_TYPE_BITS))

#define MOO_CLASS_SPEC_FLAG_INDEXED    (1 << 0)
#define MOO_CLASS_SPEC_FLAG_IMMUTABLE  (1 << 1)
#define MOO_CLASS_SPEC_FLAG_UNCOPYABLE (1 << 2)

#define MOO_CLASS_SPEC_IS_INDEXED(spec) (MOO_CLASS_SPEC_FLAGS(spec) & MOO_CLASS_SPEC_FLAG_INDEXED)
#define MOO_CLASS_SPEC_IS_IMMUTABLE(spec) (MOO_CLASS_SPEC_FLAGS(spec) & MOO_CLASS_SPEC_FLAG_IMMUTABLE)
#define MOO_CLASS_SPEC_IS_UNCOPYABLE(spec) (MOO_CLASS_SPEC_FLAGS(spec) & MOO_CLASS_SPEC_FLAG_UNCOPYABLE)

/* What is the maximum number of named instance variables?
 * This limit is set this way because the number must be encoded into 
 * the spec field of the class with limited number of bits assigned to
 * the number of named instance variables.
 */
#define MOO_MAX_NAMED_INSTVARS \
	MOO_BITS_MAX(moo_oow_t, MOO_SMOOI_ABS_BITS - (MOO_OBJ_FLAGS_TYPE_BITS + MOO_CLASS_SPEC_FLAG_BITS))

/* Given the number of named instance variables, what is the maximum number 
 * of indexed instance variables? The number of indexed instance variables
 * is not stored in the spec field of the class. It only affects the actual
 * size of an object(obj->_size) selectively combined with the number of 
 * named instance variables. So it's the maximum value of obj->_size minus
 * the number of named instance variables.
 */
#define MOO_MAX_INDEXED_INSTVARS(named_instvar) (MOO_OBJ_SIZE_MAX - named_instvar)

/*
 * self-specification of a class
 *   | classinstvars     | classvars         | flags |
 *
 * When converted to a small integer
 *   | sign-bit | classinstvars | classvars | flags | tag |
 */
#define MOO_CLASS_SELFSPEC_FLAG_BITS (3)
#define MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS ((MOO_SMOOI_ABS_BITS - MOO_CLASS_SELFSPEC_FLAG_BITS) / 2)
#define MOO_CLASS_SELFSPEC_CLASSVAR_BITS (MOO_SMOOI_ABS_BITS - (MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS + MOO_CLASS_SELFSPEC_FLAG_BITS))

#define MOO_CLASS_SELFSPEC_MAKE(class_var,classinst_var,flag) \
	((((moo_oow_t)class_var)     << (MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS + MOO_CLASS_SELFSPEC_FLAG_BITS)) | \
	 (((moo_oow_t)classinst_var) << (MOO_CLASS_SELFSPEC_FLAG_BITS)) | \
	 (((moo_oow_t)flag)          << (0)))

#define MOO_CLASS_SELFSPEC_CLASSVARS(spec) \
	(((moo_oow_t)spec) >> (MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS + MOO_CLASS_SELFSPEC_FLAG_BITS))

#define MOO_CLASS_SELFSPEC_CLASSINSTVARS(spec) \
	((((moo_oow_t)spec) >> MOO_CLASS_SELFSPEC_FLAG_BITS) & MOO_LBMASK(moo_oow_t, MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS))

#define MOO_CLASS_SELFSPEC_FLAGS(spec) \
	(((moo_oow_t)spec) & MOO_LBMASK(moo_oow_t, MOO_CLASS_SELFSPEC_FLAG_BITS))

#define MOO_CLASS_SELFSPEC_FLAG_FINAL   (1 << 0)
#define MOO_CLASS_SELFSPEC_FLAG_LIMITED (1 << 1)


#define MOO_MAX_CLASSVARS      MOO_BITS_MAX(moo_oow_t, MOO_CLASS_SELFSPEC_CLASSVAR_BITS)
#define MOO_MAX_CLASSINSTVARS  MOO_BITS_MAX(moo_oow_t, MOO_CLASS_SELFSPEC_CLASSINSTVAR_BITS)


#if defined(MOO_LIMIT_OBJ_SIZE)
/* limit the maximum object size such that:
 *   1. an index to an object field can be represented in a small integer.
 *   2. the maximum number of bit shifts can be represented in the moo_oow_t type.
 */
#	define MOO_OBJ_SIZE_MAX ((moo_oow_t)MOO_SMOOI_MAX)
#	define MOO_OBJ_SIZE_BITS_MAX (MOO_OBJ_SIZE_MAX * MOO_BITS_PER_BYTE)
#else
#	define MOO_OBJ_SIZE_MAX ((moo_oow_t)MOO_TYPE_MAX(moo_oow_t))
#	define MOO_OBJ_SIZE_BITS_MAX (MOO_OBJ_SIZE_MAX * MOO_BITS_PER_BYTE)
#endif


#define MOO_OOP_IS_PBIGINT(moo,x) (MOO_CLASSOF(moo,x) == (moo)->_large_positive_integer)
#define MOO_OOP_IS_NBIGINT(moo,x) (MOO_CLASSOF(moo,x) == (moo)->_large_negative_integer)
#define MOO_OOP_IS_BIGINT(moo,x)  (MOO_OOP_IS_PBIGINT(moo,x) || MOO_OOP_IS_NBIGINT(moo,x))

#define MOO_POINTER_IS_PBIGINT(moo,x) (MOO_OBJ_GET_CLASS(x) == (moo)->_large_positive_integer)
#define MOO_POINTER_IS_NBIGINT(moo,x) (MOO_OBJ_GET_CLASS(x) == (moo)->_large_negative_integer)
#define MOO_POINTER_IS_BIGINT(moo,x)  (MOO_OOP_IS_PBIGINT(moo,x) || MOO_OOP_IS_NBIGINT(moo,x))

#define MOO_OOP_IS_FPDEC(moo,x) (MOO_CLASSOF(moo,x) == (moo)->_fixed_point_decimal)
#define MOO_POINTER_IS_FPDEC(moo,x) (MOO_OBJ_GET_CLASS(x) == (moo)->_fixed_point_Decimal)

#if defined(MOO_INCLUDE_COMPILER)

/* ========================================================================= */
/* SOURCE CODE I/O FOR COMPILER                                              */
/* ========================================================================= */

enum moo_iotok_type_t
{
	MOO_IOTOK_EOF,

	MOO_IOTOK_CHARLIT,
	MOO_IOTOK_STRLIT,
	MOO_IOTOK_SYMLIT,
	MOO_IOTOK_INTLIT,
	MOO_IOTOK_RADINTLIT,
	MOO_IOTOK_FPDECLIT,
	MOO_IOTOK_SCALEDFPDECLIT, /* NNpNNNN.NN e.g. 5p10.3 ===> 10.30000 */
	MOO_IOTOK_ERRLIT, /* #\eNN */
	MOO_IOTOK_SMPTRLIT, /* #\pXX */
	MOO_IOTOK_BYTEARRAYLIT, /* B"ab\x99\x77" */

	MOO_IOTOK_NIL,
	MOO_IOTOK_SELF,
	MOO_IOTOK_SUPER,
	MOO_IOTOK_TRUE,
	MOO_IOTOK_FALSE,
	MOO_IOTOK_THIS_CONTEXT,
	MOO_IOTOK_THIS_PROCESS,
	MOO_IOTOK_SELFNS,

	MOO_IOTOK_IF,
	MOO_IOTOK_IFNOT,
	MOO_IOTOK_ELSE,
	MOO_IOTOK_ELIF,
	MOO_IOTOK_ELIFNOT,

	MOO_IOTOK_WHILE,
	MOO_IOTOK_UNTIL,
	MOO_IOTOK_DO,
	MOO_IOTOK_BREAK,
	MOO_IOTOK_CONTINUE,
	MOO_IOTOK_GOTO,

	MOO_IOTOK_AND,
	MOO_IOTOK_OR,

	MOO_IOTOK_IDENT,
	MOO_IOTOK_IDENT_DOTTED,
	MOO_IOTOK_BINSEL,
	MOO_IOTOK_KEYWORD, /* word ending with : such as basicAt:*/
	MOO_IOTOK_ASSIGN,  /* := */
	MOO_IOTOK_COLON,   /* : */
	MOO_IOTOK_DHASH,   /* ## */
	MOO_IOTOK_RETURN,       /* ^ */
	MOO_IOTOK_LOCAL_RETURN, /* ^^ */
	MOO_IOTOK_LBRACE,
	MOO_IOTOK_RBRACE,
	MOO_IOTOK_LBRACK,
	MOO_IOTOK_RBRACK,
	MOO_IOTOK_LPAREN,
	MOO_IOTOK_RPAREN,
	MOO_IOTOK_HASHPAREN,   /* #( - array literal */
	MOO_IOTOK_HASHBRACK,   /* #[ - byte array literal */
	MOO_IOTOK_HASHBRACE,   /* #{ - dictionary literal */
	MOO_IOTOK_DHASHPAREN,  /* #( - array expression */
	MOO_IOTOK_DHASHBRACK,  /* #[ - byte array expression */
	MOO_IOTOK_DHASHBRACE,  /* #{ - dictionary expression */
	MOO_IOTOK_PERIOD,
	MOO_IOTOK_COMMA,
	MOO_IOTOK_SEMICOLON
};
typedef enum moo_iotok_type_t moo_iotok_type_t;

struct moo_iotok_t
{
	moo_iotok_type_t type;
	moo_oocs_t name;
	moo_oow_t name_capa;
	moo_ioloc_t loc;
};
typedef struct moo_iotok_t moo_iotok_t;

typedef struct moo_iolink_t moo_iolink_t;
struct moo_iolink_t
{
	moo_iolink_t* link;
};

typedef struct moo_code_t moo_code_t;
struct moo_code_t
{
	moo_uint8_t* ptr;
	moo_oow_t    len;
	moo_oow_t    capa;

	/* array that hold the location of the byte code emitted */
	moo_oow_t*   locptr;
};

typedef struct moo_oow_pool_chunk_t moo_oow_pool_chunk_t;
struct moo_oow_pool_chunk_t
{
	struct
	{
		moo_oow_t v;
		moo_ioloc_t loc;
	} buf[16];
	moo_oow_pool_chunk_t* next;
};

typedef struct moo_oow_pool_t moo_oow_pool_t;
struct moo_oow_pool_t
{
	moo_oow_pool_chunk_t  static_chunk;
	moo_oow_pool_chunk_t* head;
	moo_oow_pool_chunk_t* tail;
	moo_oow_t             count;
};

enum moo_loop_type_t
{
	MOO_LOOP_WHILE,
	MOO_LOOP_DO_WHILE
};
typedef enum moo_loop_type_t moo_loop_type_t;

typedef struct moo_loop_t moo_loop_t;
struct moo_loop_t
{
	moo_loop_type_t type;
	moo_oow_t startpos;
	moo_oow_pool_t break_ip_pool; /* a pool that holds jump instruction pointers for break */
	moo_oow_pool_t continue_ip_pool; /* a pool that hold jump instructino pointers for continue. only for do-while */
	moo_oow_t blkcount; /* number of inner blocks enclosed in square brackets */
	moo_loop_t* next;
};

typedef struct moo_label_t moo_label_t;
struct moo_label_t
{
	moo_oow_t blk_id; /* block id where this label belongs to */
	moo_oow_t blk_depth; /* level of the block where thsi label belongs to */ 
	moo_ioloc_t loc; /* location inside source */
	moo_oow_t ip; /* instruction pointer where this label points to */
	moo_label_t* next;
	/* ... label name at the back ... */
};

typedef struct moo_goto_t moo_goto_t;
struct moo_goto_t
{
	moo_oow_t blk_id;
	moo_oow_t blk_depth;
	moo_ioloc_t loc;
	moo_oow_t ip;
	moo_goto_t* next;
	/* ... target label name at the back ... */
};

typedef struct moo_oopbuf_t moo_oopbuf_t;
struct moo_oopbuf_t
{
	moo_oop_t* ptr;
	moo_oow_t  count;
	moo_oow_t  capa;
};

typedef struct moo_oobbuf_t moo_oobbuf_t;
struct moo_oobbuf_t
{
	moo_oob_t* ptr;
	moo_oow_t  count;
	moo_oow_t  capa;
};

typedef struct moo_initv_t moo_initv_t;
struct moo_initv_t
{
	moo_oop_t v;
	int flags;
};

enum moo_pragma_flag_t
{
	MOO_PRAGMA_QC = (1 << 0) /* quoted commented. treat a double quoted text as a comment */
};

enum moo_cunit_type_t
{
	/* [NOTE] get_cunit_method_data() in comp.c depends on the order of the 
	 *        following enumerators */
	MOO_CUNIT_BLANK = 0,
	MOO_CUNIT_POOLDIC,
	MOO_CUNIT_CLASS,
	MOO_CUNIT_INTERFACE
};
typedef enum moo_cunit_type_t moo_cunit_type_t;

#define MOO_CUNIT_HEADER \
	moo_cunit_type_t cunit_type; \
	moo_cunit_t*     cunit_parent

typedef struct moo_cunit_t moo_cunit_t;
struct moo_cunit_t
{
	MOO_CUNIT_HEADER;
};

typedef struct moo_cunit_pooldic_t moo_cunit_pooldic_t;
struct moo_cunit_pooldic_t
{
	MOO_CUNIT_HEADER;

	moo_oocs_t name;
	moo_oocs_t fqn;
	moo_oow_t fqn_capa;
	moo_ioloc_t fqn_loc;

	moo_oop_dic_t pd_oop;
	moo_oop_nsdic_t ns_oop;

	moo_oow_t start;
	moo_oow_t end;
};

typedef struct moo_pooldic_import_data_t moo_pooldic_import_data_t;
struct moo_pooldic_import_data_t
{
	moo_oocs_t dcl;
	moo_oow_t dcl_capa; /* capacity of the dcl string */
	moo_oopbuf_t dics; /* dictionary objects imported */
};

/* data kept by compiler to complie a method */
typedef struct moo_method_data_t moo_method_data_t;
struct moo_method_data_t
{
	int active;

	moo_method_type_t type;
	int primitive; /* true if method(#primitive) */
	int lenient; /* true if method(#lenient) */

	/* method source text */
	moo_oocs_t text;
	moo_oow_t text_capa;

	/* buffer to store identifier names to be assigned */
	moo_oocs_t assignees;
	moo_oow_t assignees_capa;

	/* buffer to store binary selectors being worked on */
	moo_oocs_t binsels;
	moo_oow_t binsels_capa;

	/* buffer to store keyword selectors being worked on */
	moo_oocs_t kwsels;
	moo_oow_t kwsels_capa;

	/* method name */
	moo_oocs_t name;
	moo_oow_t name_capa;
	moo_ioloc_t name_loc;
	moo_ioloc_t start_loc; /* location where the method definition begins */

	/* is the unary method followed by parameter list? */
	int variadic;

	/* single string containing a space separated list of temporaries */
	moo_oocs_t tmprs; 
	moo_oow_t tmprs_capa;
	moo_oow_t tmpr_count; /* total number of temporaries including arguments */
	moo_oow_t tmpr_nargs;

	/* literals */
	moo_oopbuf_t literals;

	/* 0 for no primitive, 1 for a normal primitive, 2 for a named primitive */
	int pftype;
	/* primitive function number */
	moo_ooi_t pfnum; 

	/* block depth - [ ... ]*/
	moo_oow_t blk_idseq;
	moo_oow_t blk_id;
	moo_oow_t blk_depth;
	moo_oow_t* blk_tmprcnt;
	moo_oow_t blk_tmprcnt_capa;

	/* information about loop constructs */
	moo_loop_t* loop;

	moo_goto_t* _goto;
	moo_label_t* _label;

	/* byte code */
	moo_code_t code;
};

typedef struct moo_cunit_class_t moo_cunit_class_t;
struct moo_cunit_class_t
{
	MOO_CUNIT_HEADER;

	int flags;
	int indexed_type;

	/* fixed instance size specified for a non-pointer class. class(#byte(N)), etc */
	moo_oow_t non_pointer_instsize; 

	moo_oop_class_t self_oop;
	moo_oop_t super_oop; /* this may be nil. so the type is moo_oop_t */
	moo_oop_nsdic_t ns_oop;
	moo_oocs_t fqn;
	moo_oocs_t name;
	moo_oow_t fqn_capa;
	moo_ioloc_t fqn_loc;

	moo_oop_nsdic_t superns_oop;
	moo_oocs_t superfqn;
	moo_oocs_t supername;
	moo_oow_t superfqn_capa;
	moo_ioloc_t superfqn_loc;

	moo_oocs_t modname; /* module name after 'from' */
	moo_oow_t modname_capa;
	moo_ioloc_t modname_loc;

	moo_oopbuf_t ifces; /* interfaces that this class implement */
	moo_oopbuf_t ifce_mths[2]; /* interface methods that this class needs to take */
	int in_class_body; /* set to non-zero after '{' has been encountered */

	/* instance variable, class variable, class instance variable, constant
	 *   var[0] - named instance variables
	 *   var[1] - class instance variables
	 *   var[2] - class variables 
	 */
	struct
	{
		moo_oocs_t str;  /* long string containing all variables declared delimited by a space */
		moo_oow_t  str_capa;

		moo_oow_t count; /* the number of variables declared in this class only */
		moo_oow_t total_count; /* the number of variables declared in this class and superclasses */

		moo_initv_t* initv;
		moo_oow_t initv_capa;
		/* initv_count holds the index to the last variable with a 
		 * default initial value defined in this class only plus one.
		 * inheritance is handled by the compiler separately using
		 * the reference to the superclass. so it doesn't include
		 * the variables defined in the superclass chain.
		 * for a definition: class ... { var a, b := 0, c },
		 * initv_count is set to 2 while count is 3. totoal_count
		 * will be 3 too if there is no variabled defined in the
		 * superclass chain. */
		moo_oow_t initv_count;  
	} var[3];

	moo_oow_t dbgi_class_offset;

	moo_pooldic_import_data_t pdimp;
	moo_method_data_t mth;
};

typedef struct moo_cunit_interface_t moo_cunit_interface_t;
struct moo_cunit_interface_t
{
	MOO_CUNIT_HEADER;

	moo_oop_interface_t self_oop;
	moo_oop_nsdic_t ns_oop;
	moo_oocs_t fqn;
	moo_oocs_t name;
	moo_oow_t fqn_capa;
	moo_ioloc_t fqn_loc;

	moo_oow_t dbgi_interface_offset;
	moo_method_data_t mth;
};

struct moo_compiler_t
{
	int pragma_flags;

	/* input handler */
	moo_ioimpl_t impl;
	moo_ooch_t*  iid; /* input id that represents the current main input file */

	/* information about the last meaningful character read.
	 * this is a copy of curinp->lxc if no ungetting is performed.
	 * if there is something in the unget buffer, this is overwritten
	 * by a value from the buffer when the request to read a character
	 * is served */
	moo_iolxc_t  lxc;

	/* unget buffer */
	moo_iolxc_t  ungot[10];
	int          nungots;

	/* static input data buffer */
	moo_ioarg_t  arg;    

	/* pointer to the current input data. initially, it points to &arg */
	moo_ioarg_t* curinp;

	/* the last token read */
	moo_iotok_t  tok;
	moo_iolink_t* io_names;

	/* syntax error information */
	moo_synerr_t synerr;
	moo_ooch_t synerr_tgtbuf[256];

	/* temporary space used when dealing with an illegal character */
	moo_ooch_t ilchr;
	moo_oocs_t ilchr_ucs;

	/* workspace to use when reading byte array elements */
	moo_oobbuf_t balit;

	/* workspace space to use when reading an array */
	moo_oopbuf_t arlit;

	/* the current compilation unit being processed */
	moo_cunit_t* cunit;
};
#endif

#if defined(MOO_BCODE_LONG_PARAM_SIZE) && (MOO_BCODE_LONG_PARAM_SIZE == 1)
#	define MAX_CODE_INDEX               (0xFFu)
#	define MAX_CODE_NTMPRS              (0xFFu)
#	define MAX_CODE_NARGS               (0xFFu)
#	define MAX_CODE_NBLKARGS            (0xFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFu)
#	define MAX_CODE_JUMP                (0xFFu)
#	define MAX_CODE_PARAM               (0xFFu)
#elif defined(MOO_BCODE_LONG_PARAM_SIZE) && (MOO_BCODE_LONG_PARAM_SIZE == 2)
#	define MAX_CODE_INDEX               (0xFFFFu)
#	define MAX_CODE_NTMPRS              (0xFFFFu)
#	define MAX_CODE_NARGS               (0xFFFFu)
#	define MAX_CODE_NBLKARGS            (0xFFFFu)
#	define MAX_CODE_NBLKTMPRS           (0xFFFFu)
#	define MAX_CODE_JUMP                (0xFFFFu)
#	define MAX_CODE_PARAM               (0xFFFFu)
#else
#	error Unsupported MOO_BCODE_LONG_PARAM_SIZE
#endif

/*
----------------------------------------------------------------------------------------------------------------
SHORT INSTRUCTION CODE                                        LONG INSTRUCTION CODE
----------------------------------------------------------------------------------------------------------------
                                                                      v v
0-3      0000 00XX STORE_INTO_INSTVAR                         128  1000 0000 XXXXXXXX STORE_INTO_INSTVAR_X                    (bit 4 off, bit 3 off) 
4-7      0000 01XX STORE_INTO_INSTVAR
8-11     0000 10XX POP_INTO_INSTVAR                           136  1000 1000 XXXXXXXX POP_INTO_INSTVAR_X                      (bit 4 off, bit 3 on)
12-15    0000 11XX POP_INTO_INSTVAR
16-19    0001 00XX PUSH_INSTVAR                               144  1001 0000 XXXXXXXX PUSH_INSTVAR_X                          (bit 4 on)
20-23    0001 01XX PUSH_INSTVAR

                                                                      v v
24-27    0001 10XX PUSH_TEMPVAR                               152  1001 1000 XXXXXXXX PUSH_TEMPVAR_X                          (bit 4 on)
28-31    0001 11XX PUSH_TEMPVAR
32-35    0010 00XX STORE_INTO_TEMPVAR                         160  1010 0000 XXXXXXXX STORE_INTO_TEMPVAR_X                    (bit 4 off, bit 3 off)
36-39    0010 01XX STORE_INTO_TEMPVAR
40-43    0010 10XX POP_INTO_TEMPVAR                           168  1010 1000 XXXXXXXX POP_INTO_TEMPVAR_X                      (bit 4 off, bit 3 on)
44-47    0010 11XX POP_INTO_TEMPVAR

48-51    0011 00XX PUSH_LITERAL                               176  1011 0000 XXXXXXXX PUSH_LITERAL_X
52-55    0011 01XX PUSH_LITERAL

                                                                        vv
56-59    0011 10XX STORE_INTO_OBJECT                          184  1011 1000 XXXXXXXX STORE_INTO_OBJECT                       (bit 3 on, bit 2 off)
60-63    0011 11XX POP_INTO_OBJECT                            188  1011 1100 XXXXXXXX POP_INTO_OBJECT                         (bit 3 on, bit 2 on)
64-67    0100 00XX PUSH_OBJECT                                192  1100 0000 XXXXXXXX PUSH_OBJECT                             (bit 3 off)


68-87    0100 01XX UNUSED                                     196  1100 0100 XXXXXXXX JUMP_FORWARD
                                                              197  1100 0101 XXXXXXXX JUMP2_FORWARD
                                                              198  1100 0110 XXXXXXXX JUMP_FORWARD_IF_TRUE
                                                              199  1100 0111 XXXXXXXX JUMP2_FORWARD_IF_TRUE
                                                              200  1100 1000 XXXXXXXX JUMP_FORWARD_IF_FALSE
                                                              201  1100 1001 XXXXXXXX JUMP2_FORWARD_IF_FALSE
                                                              202  1100 1010 XXXXXXXX JMPOP_FORWARD_IF_TRUE
                                                              203  1100 1011 XXXXXXXX JMPOP2_FORWARD_IF_TRUE
                                                              204  1100 1100 XXXXXXXX JMPOP_FORWARD_IF_FALSE
                                                              205  1100 1101 XXXXXXXX JMPOP2_FORWARD_IF_FALSE

                                                              206  1100 1110 XXXXXXXX JUMP_BACKWARD
                                                              207  1100 1111 XXXXXXXX JUMP2_BACKWARD
                                                              208  1101 0000 XXXXXXXX JUMP_BACKWARD_IF_TRUE
                                                              209  1101 0001 XXXXXXXX JUMP2_BACKWARD_IF_TRUE
                                                              210  1101 0010 XXXXXXXX JUMP_BACKWARD_IF_FALSE
                                                              211  1101 0011 XXXXXXXX JUMP2_BACKWARD_IF_FALSE
                                                              212  1101 0100 XXXXXXXX JMPOP_BACKWARD_IF_TRUE
                                                              213  1101 0101 XXXXXXXX JMPOP2_BACKWARD_IF_TRUE
                                                              214  1101 0110 XXXXXXXX JMPOP_BACKWARD_IF_FALSE
                                                              215  1101 0111 XXXXXXXX JMPOP2_BACKWARD_IF_FALSE

                                                                        vv
88-91    0101 10XX YYYYYYYY STORE_INTO_CTXTEMPVAR             216  1101 1000 XXXXXXXX YYYYYYYY STORE_INTO_CTXTEMPVAR_X        (bit 3 on, bit 2 off)
92-95    0101 11XX YYYYYYYY POP_INTO_CTXTEMPVAR               220  1101 1100 XXXXXXXX YYYYYYYY POP_INTO_CTXTEMPVAR_X          (bit 3 on, bit 2 on)
96-99    0110 00XX YYYYYYYY PUSH_CTXTEMPVAR                   224  1110 0000 XXXXXXXX YYYYYYYY PUSH_CTXTEMPVAR_X              (bit 3 off)
# XXXth outer-frame, YYYYYYYY local variable

100-103  0110 01XX YYYYYYYY PUSH_OBJVAR                       228  1110 0100 XXXXXXXX YYYYYYYY PUSH_OBJVAR_X                  (bit 3 off)
104-107  0110 10XX YYYYYYYY STORE_INTO_OBJVAR                 232  1110 1000 XXXXXXXX YYYYYYYY STORE_INTO_OBJVAR_X            (bit 3 on, bit 2 off)
108-111  0110 11XX YYYYYYYY POP_INTO_OBJVAR                   236  1110 1100 XXXXXXXX YYYYYYYY POP_INTO_OBJVAR_X              (bit 3 on, bit 2 on)
# XXXth instance variable of YYYYYYYY object

                                                                         v
112-115  0111 00XX YYYYYYYY SEND_MESSAGE                      240  1111 0000 XXXXXXXX YYYYYYYY SEND_MESSAGE_X                 (bit 2 off)
116-119  0111 01XX YYYYYYYY SEND_MESSAGE_TO_SUPER             244  1111 0100 XXXXXXXX YYYYYYYY SEND_MESSAGE_TO_SUPER_X        (bit 2 on)
# XXX args, YYYYYYYY message

120-123  0111 10XX  UNUSED
124-127  0111 11XX  UNUSED

##
## "SHORT_CODE_0 | 0x80" becomes "LONG_CODE_X".
## A special single byte instruction is assigned an unused number greater than 128.
##
*/

enum moo_bcode_t
{
	BCODE_STORE_INTO_INSTVAR_0     = 0x00,
	BCODE_STORE_INTO_INSTVAR_1     = 0x01,
	BCODE_STORE_INTO_INSTVAR_2     = 0x02,
	BCODE_STORE_INTO_INSTVAR_3     = 0x03,

	BCODE_STORE_INTO_INSTVAR_4     = 0x04,
	BCODE_STORE_INTO_INSTVAR_5     = 0x05,
	BCODE_STORE_INTO_INSTVAR_6     = 0x06,
	BCODE_STORE_INTO_INSTVAR_7     = 0x07,

	BCODE_POP_INTO_INSTVAR_0       = 0x08,
	BCODE_POP_INTO_INSTVAR_1       = 0x09,
	BCODE_POP_INTO_INSTVAR_2       = 0x0A,
	BCODE_POP_INTO_INSTVAR_3       = 0x0B,

	BCODE_POP_INTO_INSTVAR_4       = 0x0C,
	BCODE_POP_INTO_INSTVAR_5       = 0x0D,
	BCODE_POP_INTO_INSTVAR_6       = 0x0E,
	BCODE_POP_INTO_INSTVAR_7       = 0x0F,

	BCODE_PUSH_INSTVAR_0           = 0x10,
	BCODE_PUSH_INSTVAR_1           = 0x11,
	BCODE_PUSH_INSTVAR_2           = 0x12,
	BCODE_PUSH_INSTVAR_3           = 0x13,

	BCODE_PUSH_INSTVAR_4           = 0x14,
	BCODE_PUSH_INSTVAR_5           = 0x15,
	BCODE_PUSH_INSTVAR_6           = 0x16,
	BCODE_PUSH_INSTVAR_7           = 0x17,

	BCODE_PUSH_TEMPVAR_0           = 0x18,
	BCODE_PUSH_TEMPVAR_1           = 0x19,
	BCODE_PUSH_TEMPVAR_2           = 0x1A,
	BCODE_PUSH_TEMPVAR_3           = 0x1B,

	BCODE_PUSH_TEMPVAR_4           = 0x1C,
	BCODE_PUSH_TEMPVAR_5           = 0x1D,
	BCODE_PUSH_TEMPVAR_6           = 0x1E,
	BCODE_PUSH_TEMPVAR_7           = 0x1F,

	BCODE_STORE_INTO_TEMPVAR_0     = 0x20,
	BCODE_STORE_INTO_TEMPVAR_1     = 0x21,
	BCODE_STORE_INTO_TEMPVAR_2     = 0x22,
	BCODE_STORE_INTO_TEMPVAR_3     = 0x23,

	BCODE_STORE_INTO_TEMPVAR_4     = 0x24,
	BCODE_STORE_INTO_TEMPVAR_5     = 0x25,
	BCODE_STORE_INTO_TEMPVAR_6     = 0x26,
	BCODE_STORE_INTO_TEMPVAR_7     = 0x27,

	BCODE_POP_INTO_TEMPVAR_0       = 0x28,
	BCODE_POP_INTO_TEMPVAR_1       = 0x29,
	BCODE_POP_INTO_TEMPVAR_2       = 0x2A,
	BCODE_POP_INTO_TEMPVAR_3       = 0x2B,

	BCODE_POP_INTO_TEMPVAR_4       = 0x2C,
	BCODE_POP_INTO_TEMPVAR_5       = 0x2D,
	BCODE_POP_INTO_TEMPVAR_6       = 0x2E,
	BCODE_POP_INTO_TEMPVAR_7       = 0x2F,

	BCODE_PUSH_LITERAL_0           = 0x30,
	BCODE_PUSH_LITERAL_1           = 0x31,
	BCODE_PUSH_LITERAL_2           = 0x32,
	BCODE_PUSH_LITERAL_3           = 0x33,

	BCODE_PUSH_LITERAL_4           = 0x34,
	BCODE_PUSH_LITERAL_5           = 0x35,
	BCODE_PUSH_LITERAL_6           = 0x36,
	BCODE_PUSH_LITERAL_7           = 0x37,

	/* -------------------------------------- */

	BCODE_STORE_INTO_OBJECT_0      = 0x38,
	BCODE_STORE_INTO_OBJECT_1      = 0x39,
	BCODE_STORE_INTO_OBJECT_2      = 0x3A,
	BCODE_STORE_INTO_OBJECT_3      = 0x3B,

	BCODE_POP_INTO_OBJECT_0        = 0x3C,
	BCODE_POP_INTO_OBJECT_1        = 0x3D,
	BCODE_POP_INTO_OBJECT_2        = 0x3E,
	BCODE_POP_INTO_OBJECT_3        = 0x3F,

	BCODE_PUSH_OBJECT_0            = 0x40,
	BCODE_PUSH_OBJECT_1            = 0x41,
	BCODE_PUSH_OBJECT_2            = 0x42,
	BCODE_PUSH_OBJECT_3            = 0x43, /* 67 */

	/* UNUSED 0x44 - 0x57 */

	BCODE_STORE_INTO_CTXTEMPVAR_0  = 0x58, /* 88 */
	BCODE_STORE_INTO_CTXTEMPVAR_1  = 0x59, /* 89 */
	BCODE_STORE_INTO_CTXTEMPVAR_2  = 0x5A, /* 90 */
	BCODE_STORE_INTO_CTXTEMPVAR_3  = 0x5B, /* 91 */

	BCODE_POP_INTO_CTXTEMPVAR_0    = 0x5C, /* 92 */
	BCODE_POP_INTO_CTXTEMPVAR_1    = 0x5D, /* 93 */
	BCODE_POP_INTO_CTXTEMPVAR_2    = 0x5E, /* 94 */
	BCODE_POP_INTO_CTXTEMPVAR_3    = 0x5F, /* 95 */

	BCODE_PUSH_CTXTEMPVAR_0        = 0x60, /* 96 */
	BCODE_PUSH_CTXTEMPVAR_1        = 0x61, /* 97 */
	BCODE_PUSH_CTXTEMPVAR_2        = 0x62, /* 98 */
	BCODE_PUSH_CTXTEMPVAR_3        = 0x63, /* 99 */

	BCODE_PUSH_OBJVAR_0            = 0x64,
	BCODE_PUSH_OBJVAR_1            = 0x65,
	BCODE_PUSH_OBJVAR_2            = 0x66,
	BCODE_PUSH_OBJVAR_3            = 0x67,

	BCODE_STORE_INTO_OBJVAR_0      = 0x68,
	BCODE_STORE_INTO_OBJVAR_1      = 0x69,
	BCODE_STORE_INTO_OBJVAR_2      = 0x6A,
	BCODE_STORE_INTO_OBJVAR_3      = 0x6B,

	BCODE_POP_INTO_OBJVAR_0        = 0x6C,
	BCODE_POP_INTO_OBJVAR_1        = 0x6D,
	BCODE_POP_INTO_OBJVAR_2        = 0x6E,
	BCODE_POP_INTO_OBJVAR_3        = 0x6F,

	BCODE_SEND_MESSAGE_0           = 0x70,
	BCODE_SEND_MESSAGE_1           = 0x71,
	BCODE_SEND_MESSAGE_2           = 0x72,
	BCODE_SEND_MESSAGE_3           = 0x73,

	BCODE_SEND_MESSAGE_TO_SUPER_0  = 0x74,
	BCODE_SEND_MESSAGE_TO_SUPER_1  = 0x75,
	BCODE_SEND_MESSAGE_TO_SUPER_2  = 0x76,
	BCODE_SEND_MESSAGE_TO_SUPER_3  = 0x77,

	/* UNUSED 0x78 - 0x7F */

	BCODE_STORE_INTO_INSTVAR_X     = 0x80, /* 128 ## */

	BCODE_PUSH_RECEIVER            = 0x81, /* 129 */
	BCODE_PUSH_NIL                 = 0x82, /* 130 */
	BCODE_PUSH_TRUE                = 0x83, /* 131 */
	BCODE_PUSH_FALSE               = 0x84, /* 132 */
	BCODE_PUSH_CONTEXT             = 0x85, /* 133 */
	BCODE_PUSH_PROCESS             = 0x86, /* 134 */
	BCODE_PUSH_RECEIVER_NS         = 0x87, /* 135 */

	BCODE_POP_INTO_INSTVAR_X       = 0x88, /* 136 ## */ 

	BCODE_PUSH_NEGONE              = 0x89, /* 137 */
	BCODE_PUSH_ZERO                = 0x8A, /* 138 */
	BCODE_PUSH_ONE                 = 0x8B, /* 139 */
	BCODE_PUSH_TWO                 = 0x8C, /* 140 */

	BCODE_PUSH_INSTVAR_X           = 0x90, /* 144 ## */
	BCODE_PUSH_TEMPVAR_X           = 0x98, /* 152 ## */
	BCODE_STORE_INTO_TEMPVAR_X     = 0xA0, /* 160 ## */
	BCODE_POP_INTO_TEMPVAR_X       = 0xA8, /* 168 ## */
	BCODE_PUSH_LITERAL_X           = 0xB0, /* 176 ## */

	/* UNUSED - 0xB1 */

	BCODE_PUSH_INTLIT              = 0xB2, /* 178 */
	BCODE_PUSH_NEGINTLIT           = 0xB3, /* 179 */
	BCODE_PUSH_CHARLIT             = 0xB4, /* 180 */

	/* UNUSED - 0xB5 - 0xB7 */

	BCODE_STORE_INTO_OBJECT_X      = 0xB8, /* 184 ## */
	BCODE_POP_INTO_OBJECT_X        = 0xBC, /* 188 ## */
	BCODE_PUSH_OBJECT_X            = 0xC0, /* 192 ## */

	/* UNUSED - 0xC1 - 0xC3 */

	BCODE_JUMP_FORWARD             = 0xC4, /* 196 ## */
	BCODE_JUMP2_FORWARD            = 0xC5, /* 197 ## */
	BCODE_JUMP_FORWARD_IF_TRUE     = 0xC6, /* 198 ## */
	BCODE_JUMP2_FORWARD_IF_TRUE    = 0xC7, /* 199 ## */
	BCODE_JUMP_FORWARD_IF_FALSE    = 0xC8, /* 200 ## */
	BCODE_JUMP2_FORWARD_IF_FALSE   = 0xC9, /* 201 ## */
	/* JMPOP = JUMP + POP -> it pops the stack top always but it jumps only if the condition is met */
	BCODE_JMPOP_FORWARD_IF_TRUE    = 0xCA, /* 202 ## */
	BCODE_JMPOP2_FORWARD_IF_TRUE   = 0xCB, /* 203 ## */
	BCODE_JMPOP_FORWARD_IF_FALSE   = 0xCC, /* 204 ## */
	BCODE_JMPOP2_FORWARD_IF_FALSE  = 0xCD, /* 205 ## */
	
	BCODE_JUMP_BACKWARD            = 0xCE, /* 206 ## */
	BCODE_JUMP2_BACKWARD           = 0xCF, /* 207 ## */
	BCODE_JUMP_BACKWARD_IF_TRUE    = 0xD0, /* 208 ## */
	BCODE_JUMP2_BACKWARD_IF_TRUE   = 0xD1, /* 209 ## */
	BCODE_JUMP_BACKWARD_IF_FALSE   = 0xD2, /* 210 ## */
	BCODE_JUMP2_BACKWARD_IF_FALSE  = 0xD3, /* 211 ## */
	BCODE_JMPOP_BACKWARD_IF_TRUE   = 0xD4, /* 212 ## */
	BCODE_JMPOP2_BACKWARD_IF_TRUE  = 0xD5, /* 213 ## */
	BCODE_JMPOP_BACKWARD_IF_FALSE  = 0xD6, /* 214 ## */
	BCODE_JMPOP2_BACKWARD_IF_FALSE = 0xD7, /* 215 ## */

	BCODE_STORE_INTO_CTXTEMPVAR_X  = 0xD8, /* 216 ## */
	BCODE_POP_INTO_CTXTEMPVAR_X    = 0xDC, /* 220 ## */
	BCODE_PUSH_CTXTEMPVAR_X        = 0xE0, /* 224 ## */

	BCODE_PUSH_OBJVAR_X            = 0xE4, /* 228 ## */
	BCODE_STORE_INTO_OBJVAR_X      = 0xE8, /* 232 ## */
	BCODE_POP_INTO_OBJVAR_X        = 0xEC, /* 236 ## */

	/* UNUSED 0xED */
	BCODE_MAKE_BYTEARRAY           = 0xEE, /* 238 */
	BCODE_POP_INTO_BYTEARRAY       = 0xEF, /* 239 */

	BCODE_SEND_MESSAGE_X           = 0xF0, /* 240 ## */
	/* UNUSED 0xF1 */
	BCODE_MAKE_DICTIONARY          = 0xF2, /* 242 */
	BCODE_POP_INTO_DICTIONARY      = 0xF3, /* 243 */
	BCODE_SEND_MESSAGE_TO_SUPER_X  = 0xF4, /* 244 ## */

	/* -------------------------------------- */

	BCODE_MAKE_ARRAY               = 0xF5, /* 245 */
	BCODE_POP_INTO_ARRAY           = 0xF6, /* 246 */
	BCODE_DUP_STACKTOP             = 0xF7, /* 247 */
	BCODE_POP_STACKTOP             = 0xF8,
	BCODE_RETURN_STACKTOP          = 0xF9, /* ^something */
	BCODE_RETURN_RECEIVER          = 0xFA, /* ^self */
	BCODE_RETURN_FROM_BLOCK        = 0xFB, /* return the stack top from a block */
	BCODE_LOCAL_RETURN             = 0xFC,
	BCODE_MAKE_BLOCK               = 0xFD,
	/* UNUSED 0xFE */
	BCODE_NOOP                     = 0xFF
};

/* i don't want an error raised inside the callback to override 
 * the existing error number and message. */
#define vmprim_log_write(moo,mask,ptr,len) do { \
		int shuterr = (moo)->shuterr; \
		(moo)->shuterr = 1; \
		(moo)->vmprim.log_write (moo, mask, ptr, len); \
		(moo)->shuterr = shuterr; \
	} while(0)

typedef int (*moo_dic_walker_t) (
	moo_t*                moo,
	moo_oop_dic_t         dic,
	moo_oop_association_t ass,
	void*                 ctx
);

#if defined(__cplusplus)
extern "C" {
#endif

/* ========================================================================= */
/* err.c                                                                     */
/* ========================================================================= */

void moo_seterrbfmtv (
	moo_t*           moo,
	moo_errnum_t     errnum,
	const moo_bch_t* fmt,
	va_list          ap
);

void moo_seterrufmtv (
	moo_t*           moo,
	moo_errnum_t     errnum,
	const moo_uch_t* fmt,
	va_list          ap
);


#if defined(MOO_INCLUDE_COMPILER)
void moo_setsynerrbfmt (
	moo_t*             moo,
	moo_synerrnum_t    num,
	const moo_ioloc_t* loc,
	const moo_oocs_t*  tgt,
	const moo_bch_t*   msgfmt,
	...
);

void moo_setsynerrufmt (
	moo_t*             moo,
	moo_synerrnum_t    num,
	const moo_ioloc_t* loc,
	const moo_oocs_t*  tgt,
	const moo_uch_t*   msgfmt,
	...
);

void moo_setsynerr (
	moo_t*             moo,
	moo_synerrnum_t    num,
	const moo_ioloc_t* loc,
	const moo_oocs_t*  tgt
);
#endif

/* ========================================================================= */
/* heap.c                                                                    */
/* ========================================================================= */

/**
 * The moo_makeheap() function creates a new heap of the \a size bytes.
 *
 * \return heap pointer on success and #MOO_NULL on failure.
 */
moo_heap_t* moo_makeheap (
	moo_t*     moo, 
	moo_oow_t  size
);

/**
 * The moo_killheap() function destroys the heap pointed to by \a heap.
 */
void moo_killheap (
	moo_t*      moo, 
	moo_heap_t* heap
);

/**
 * The moo_allocheapspace() function allocates \a size bytes in a semi-space 
 * of the heap pointed to by \a heap.
 *
 * \return memory pointer on success and #MOO_NULL on failure.
 */
void* moo_allocheapspace (
	moo_t*       moo,
	moo_space_t* space,
	moo_oow_t    size
);

/** 
 * The moo_allocheapmem() function allocates \a size bytes from the given heap
 * and clears it with zeros.
 */
void* moo_callocheapmem (
	moo_t*       moo,
	moo_heap_t*  heap,
	moo_oow_t    size
);

void* moo_callocheapmem_noerr (
	moo_t*       moo,
	moo_heap_t*  heap,
	moo_oow_t    size
);

void moo_freeheapmem (
	moo_t*       moo,
	moo_heap_t*  heap,
	void*        ptr
);

/* ========================================================================= */
/* obj.c                                                                     */
/* ========================================================================= */
void* moo_allocbytes (
	moo_t*     moo,
	moo_oow_t  size
);

/**
 * The moo_allocoopobj() function allocates a raw object composed of \a size
 * pointer fields excluding the header.
 */
moo_oop_t moo_allocoopobj (
	moo_t*    moo,
	moo_oow_t size
);

moo_oop_t moo_allocoopobjwithtrailer (
	moo_t*           moo,
	moo_oow_t        size,
	const moo_oob_t* tptr,
	moo_oow_t        tlen
);

moo_oop_t moo_alloccharobj (
	moo_t*            moo,
	const moo_ooch_t* ptr,
	moo_oow_t         len
);

moo_oop_t moo_allocbyteobj (
	moo_t*           moo,
	const moo_oob_t* ptr,
	moo_oow_t        len
);

moo_oop_t moo_allochalfwordobj (
	moo_t*            moo,
	const moo_oohw_t* ptr,
	moo_oow_t         len
);

moo_oop_t moo_allocwordobj (
	moo_t*           moo,
	const moo_oow_t* ptr,
	moo_oow_t        len
);

/* ========================================================================= */
/* sym.c                                                                     */
/* ========================================================================= */
moo_oop_t moo_findsymbol (
	moo_t*             moo,
	const moo_ooch_t*  ptr,
	moo_oow_t          len
);

/* ========================================================================= */
/* dic.c                                                                     */
/* ========================================================================= */
moo_oop_association_t moo_putatsysdic (
	moo_t*     moo,
	moo_oop_t  key,
	moo_oop_t  value
);

moo_oop_association_t moo_getatsysdic (
	moo_t*     moo,
	moo_oop_t  key
);

moo_oop_association_t moo_lookupsysdic (
	moo_t*            moo,
	const moo_oocs_t* name
);

moo_oop_association_t moo_putatdic (
	moo_t*        moo,
	moo_oop_dic_t dic,
	moo_oop_t     key,
	moo_oop_t     value
);

moo_oop_association_t moo_getatdic (
	moo_t*        moo,
	moo_oop_dic_t dic,
	moo_oop_t     key
);

moo_oop_association_t moo_lookupdic_noseterr (
	moo_t*            moo,
	moo_oop_dic_t     dic,
	const moo_oocs_t* name
);

moo_oop_association_t moo_lookupdic (
	moo_t*            moo,
	moo_oop_dic_t     dic,
	const moo_oocs_t* name
);

int moo_deletedic (
	moo_t*            moo,
	moo_oop_dic_t     dic,
	const moo_oocs_t* name
);

int moo_walkdic (
	moo_t*            moo,
	moo_oop_dic_t     dic,
	moo_dic_walker_t  walker,
	void*             ctx
);

moo_oop_dic_t moo_makedic (
	moo_t*          moo,
	moo_oop_class_t _class,
	moo_oow_t       size
);

moo_oop_nsdic_t moo_makensdic (
	moo_t*          moo,
	moo_oop_class_t _class,
	moo_oow_t       size
);


/* ========================================================================= */
/* gc.c                                                                      */
/* ========================================================================= */
int moo_regfinalizable (moo_t* moo, moo_oop_t oop);
int moo_deregfinalizable (moo_t* moo, moo_oop_t oop);
void moo_deregallfinalizables (moo_t* moo);

moo_oow_t moo_getobjpayloadbytes (moo_t* moo, moo_oop_t oop);

#if defined(MOO_ENABLE_GC_MARK_SWEEP)
void moo_gc_ms_sweep_lazy (moo_t* moo, moo_oow_t allocsize);
#endif
/* ========================================================================= */
/* bigint.c                                                                  */
/* ========================================================================= */
static MOO_INLINE int moo_isbigint (moo_t* moo, moo_oop_t x)
{
	if (!MOO_OOP_IS_POINTER(x)) return 0;

/* TODO: is it better to introduce a special integer mark into the class itself */
/* TODO: or should it check if it's a subclass, subsubclass, subsubsubclass, etc of a large_integer as well? */
	return MOO_POINTER_IS_BIGINT(moo, x);
}

static MOO_INLINE int moo_isint (moo_t* moo, moo_oop_t x)
{
	if (MOO_OOP_IS_SMOOI(x)) return 1;
	if (MOO_OOP_IS_POINTER(x)) return MOO_POINTER_IS_BIGINT(moo, x); /* is_bigint? */
	return 0;
}

moo_oop_t moo_addints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_subints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_mulints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_divints (
	moo_t*     moo,
	moo_oop_t  x,
	moo_oop_t  y,
	int        modulo,
	moo_oop_t* rem
);

moo_oop_t moo_negateint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_bitatint (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitandints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitorints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitxorints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_bitinvint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_bitshiftint (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_eqints (
	moo_t* moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_neints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_gtints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_geints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_ltints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_leints (
	moo_t*    moo,
	moo_oop_t x,
	moo_oop_t y
);

moo_oop_t moo_sqrtint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_absint (
	moo_t*    moo,
	moo_oop_t x
);

moo_oop_t moo_strtoint (
	moo_t*            moo,
	const moo_ooch_t* str,
	moo_oow_t         len,
	int               radix
);

#define MOO_INTTOSTR_RADIXMASK (0xFF)
#define MOO_INTTOSTR_LOWERCASE (1 << 8)
#define MOO_INTTOSTR_NONEWOBJ  (1 << 9)

moo_oop_t moo_inttostr (
	moo_t*      moo,
	moo_oop_t   num,
	int         flagged_radix /* radix between 2 and 36 inclusive, optionally bitwise ORed of MOO_INTTOSTR_XXX bits */
);

/* ========================================================================= */
/* number.c                                                                  */
/* ========================================================================= */
moo_oop_t moo_makefpdec (
	moo_t*      moo,
	moo_oop_t   value,
	moo_ooi_t   scale
);

moo_oop_t moo_truncfpdecval (
	moo_t*       moo,
	moo_oop_t    iv, /* integer */
	moo_ooi_t    cs, /* current scale */
	moo_ooi_t    ns  /* new scale */
);

moo_oop_t moo_truncfpdec (
	moo_t*       moo,
	moo_oop_t    iv, /* integer */
	moo_ooi_t    ns  /* new scale */
);

moo_oop_t moo_addnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_subnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_mulnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_mltnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_divnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y,
	int          modulo
);

moo_oop_t moo_gtnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_genums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_ltnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_lenums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_eqnums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_nenums (
	moo_t*       moo,
	moo_oop_t    x,
	moo_oop_t    y
);

moo_oop_t moo_negatenum (
	moo_t*       moo,
	moo_oop_t    x
);

moo_oop_t moo_sqrtnum (
	moo_t*       moo,
	moo_oop_t    x
);

moo_oop_t moo_absnum (
	moo_t*       moo,
	moo_oop_t    x
);


#define MOO_NUMTOSTR_RADIXMASK MOO_INTTOSTR_RADIXMASK
#define MOO_NUMTOSTR_LOWERCASE MOO_INTTOSTR_LOWERCASE
#define MOO_NUMTOSTR_NONEWOBJ MOO_INTTOSTR_NONEWOBJ

moo_oop_t moo_numtostr (
	moo_t*      moo,
	moo_oop_t   num,
	int         flagged_radix /* radix between 2 and 36 inclusive, optionally bitwise ORed of MOO_INTTOSTR_XXX bits */
);


/* ========================================================================= */
/* fmt.c                                                                  */
/* ========================================================================= */
int moo_fmt_object_ (
	moo_fmtout_t* fmtout,
	moo_oop_t     oop
);

int moo_strfmtcallstack (
	moo_t*            moo,
	moo_ooi_t         nargs,
	int               rcv_is_fmtstr
);

/* ========================================================================= */
/* exec.c                                                                    */
/* ========================================================================= */

moo_pfbase_t* moo_getpfnum (
	moo_t*            moo,
	const moo_ooch_t* ptr,
	moo_oow_t         len,
	moo_ooi_t*        pfnum
);

void moo_clearmethodcache (
	moo_t*            moo
);

moo_oop_method_t moo_findmethod_noseterr (
	moo_t*            moo,
	moo_oop_t         receiver,
	moo_oop_char_t    selector,
	int               super
);

moo_oop_method_t moo_findmethod (
	moo_t*            moo,
	moo_oop_t         receiver,
	moo_oop_char_t    selector,
	int               super
);

moo_oop_method_t moo_findmethodinclass (
	moo_t*            moo,
	moo_oop_class_t   _class,
	int               mth_type,
	const moo_oocs_t* name
);

moo_oop_method_t moo_findmethodinclasschain (
	moo_t*            moo,
	moo_oop_class_t   _class,
	int               mth_type,
	const moo_oocs_t* name
);

/* ========================================================================= */
/* moo.c                                                                     */
/* ========================================================================= */

moo_mod_data_t* moo_openmod (
	moo_t*            moo,
	const moo_ooch_t* name,
	moo_oow_t         namelen,
	int               hints /* 0 or bitwise-ORed of moo_mod_hint_t enumerators */
);

void moo_closemod (
	moo_t*            moo,
	moo_mod_data_t*   mdp
);

int moo_importmod (
	moo_t*            moo,
	moo_oop_class_t   _class,
	const moo_ooch_t* name,
	moo_oow_t         len
);

/*
 * The moo_querymodpf() function finds a primitive function in modules
 * with a full primitive identifier.
 */
moo_pfbase_t* moo_querymodpf (
	moo_t*            moo,
	const moo_ooch_t* pfid,
	moo_oow_t         pfidlen,
	moo_mod_t**       mod
);

moo_pvbase_t* moo_querymodpv (
	moo_t*            moo,
	const moo_ooch_t* pvid,
	moo_oow_t         pvidlen,
	moo_mod_t**       mod
);

/* ========================================================================= */
/* pf-basic.c                                                                */
/* ========================================================================= */
moo_pfrc_t moo_pf_identical (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_not_identical (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_equal (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_not_equal (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_basic_new (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_shallow_copy (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_class (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_size (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_first_index (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_last_index (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_at (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_at_put (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_at_test_put (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_fill (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_basic_shift (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_is_kind_of (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_responds_to (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

/* ========================================================================= */
/* pf-sys.c                                                                  */
/* ========================================================================= */
moo_pfrc_t moo_pf_system_toggle_process_switching (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_halting (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_find_process_by_id (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_find_first_process (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_find_last_process (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_find_previous_process (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_find_next_process (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_collect_garbage (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_pop_collectable (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_get_sigfd (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_sig (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_set_sig (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_malloc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_calloc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_free (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_free (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_get_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_get_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_put_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_system_get_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_system_put_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);


moo_pfrc_t moo_pf_smptr_get_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_get_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_smptr_put_int8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_int16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_int32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_int64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_uint8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_uint16 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_uint32 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_uint64 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_smptr_get_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_smptr_put_bytes (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);

moo_pfrc_t moo_pf_smptr_as_string (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);


/* ========================================================================= */
/* pf-utf8.c                                                                 */
/* ========================================================================= */
moo_pfrc_t moo_pf_utf8_seqlen (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_utf8_to_uc (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);
moo_pfrc_t moo_pf_uc_to_utf8 (moo_t* moo, moo_mod_t* mod, moo_ooi_t nargs);


/* ========================================================================= */
/* debug.c                                                                   */
/* ========================================================================= */
/* TODO: remove the following dump functions */
void moo_dumpsymtab (moo_t* moo);
void moo_dumpdic (moo_t* moo, moo_oop_dic_t dic, const moo_bch_t* title);

int moo_addfiletodbgi (moo_t* moo, const moo_ooch_t* file_name, moo_oow_t* start_offset);
int moo_addclasstodbgi (moo_t* moo, const moo_ooch_t* class_name, moo_oow_t file_offset, moo_oow_t file_line, moo_oow_t* start_offset);
int moo_addmethodtodbgi (moo_t* moo, moo_oow_t file_offset, moo_oow_t class_offset, const moo_ooch_t* method_name, moo_oow_t start_line, const moo_oow_t* code_loc_ptr, moo_oow_t code_loc_len, const moo_ooch_t* text_ptr, moo_oow_t text_len, moo_oow_t* start_offset);

/* ========================================================================= */
/* comp.c                                                                   */
/* ========================================================================= */
#if defined(MOO_INCLUDE_COMPILER)
const moo_ooch_t* moo_addcioname (moo_t* moo, const moo_oocs_t* name);
void moo_clearcionames (moo_t* moo);
#endif

#if defined(__cplusplus)
}
#endif


#endif
