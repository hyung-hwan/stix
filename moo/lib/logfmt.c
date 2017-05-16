/*
 * $Id$
 *
    Copyright (c) 2014-2017 Chung, Hyung-Hwan. All rights reserved.

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

#include "moo-prv.h"

/*#include <stdio.h>*/ /* for snrintf(). used for floating-point number formatting */
#include <stdarg.h>

#if defined(_MSC_VER) || defined(__BORLANDC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1200))
#	define snprintf _snprintf 
#	if !defined(HAVE_SNPRINTF)
#		define HAVE_SNPRINTF
#	endif
#endif
#if defined(HAVE_QUADMATH_H)
#	include <quadmath.h> /* for quadmath_snprintf() */
#endif
/* TODO: remove stdio.h and quadmath.h once snprintf gets replaced by own 
floting-point conversion implementation*/

/* Max number conversion buffer length: 
 * moo_intmax_t in base 2, plus NUL byte. */
#define MAXNBUF (MOO_SIZEOF(moo_intmax_t) * 8 + 1)

enum
{
	/* integer */
	LF_C = (1 << 0),
	LF_H = (1 << 1),
	LF_J = (1 << 2),
	LF_L = (1 << 3),
	LF_Q = (1 << 4),
	LF_T = (1 << 5),
	LF_Z = (1 << 6),

	/* long double */
	LF_LD = (1 << 7),
	/* __float128 */
	LF_QD = (1 << 8)
};

static struct
{
	moo_uint8_t flag; /* for single occurrence */
	moo_uint8_t dflag; /* for double occurrence */
} lm_tab[26] = 
{
	{ 0,    0 }, /* a */
	{ 0,    0 }, /* b */
	{ 0,    0 }, /* c */
	{ 0,    0 }, /* d */
	{ 0,    0 }, /* e */
	{ 0,    0 }, /* f */
	{ 0,    0 }, /* g */
	{ LF_H, LF_C }, /* h */
	{ 0,    0 }, /* i */
	{ LF_J, 0 }, /* j */
	{ 0,    0 }, /* k */
	{ LF_L, LF_Q }, /* l */
	{ 0,    0 }, /* m */
	{ 0,    0 }, /* n */
	{ 0,    0 }, /* o */
	{ 0,    0 }, /* p */
	{ LF_Q, 0 }, /* q */
	{ 0,    0 }, /* r */
	{ 0,    0 }, /* s */
	{ LF_T, 0 }, /* t */
	{ 0,    0 }, /* u */
	{ 0,    0 }, /* v */
	{ 0,    0 }, /* w */
	{ 0,    0 }, /* z */
	{ 0,    0 }, /* y */
	{ LF_Z, 0 }, /* z */
};


enum 
{
	FLAGC_DOT       = (1 << 0),
	FLAGC_SPACE     = (1 << 1),
	FLAGC_SHARP     = (1 << 2),
	FLAGC_SIGN      = (1 << 3),
	FLAGC_LEFTADJ   = (1 << 4),
	FLAGC_ZEROPAD   = (1 << 5),
	FLAGC_WIDTH     = (1 << 6),
	FLAGC_PRECISION = (1 << 7),
	FLAGC_STAR1     = (1 << 8),
	FLAGC_STAR2     = (1 << 9),
	FLAGC_LENMOD    = (1 << 10) /* length modifier */
};

static const moo_bch_t hex2ascii_lower[] = 
{
	'0','1','2','3','4','5','6','7','8','9',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z'
};

static const moo_bch_t hex2ascii_upper[] = 
{
	'0','1','2','3','4','5','6','7','8','9',
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','H','Z'
};

static moo_uch_t uch_nullstr[] = { '(','n','u','l','l', ')','\0' };
static moo_bch_t bch_nullstr[] = { '(','n','u','l','l', ')','\0' };

typedef int (*moo_fmtout_putch_t) (
	moo_t*      moo,
	moo_oow_t   mask,
	moo_ooch_t  c,
	moo_oow_t   len
);

typedef int (*moo_fmtout_putcs_t) (
	moo_t*            moo,
	moo_oow_t         mask,
	const moo_ooch_t* ptr,
	moo_oow_t         len
);

typedef struct moo_fmtout_t moo_fmtout_t;
struct moo_fmtout_t
{
	moo_oow_t            count; /* out */
	moo_oow_t            mask;  /* in */
	moo_fmtout_putch_t   putch; /* in */
	moo_fmtout_putcs_t   putcs; /* in */
};

/* ------------------------------------------------------------------------- */
/*
 * Put a NUL-terminated ASCII number (base <= 36) in a buffer in reverse
 * order; return an optional length and a pointer to the last character
 * written in the buffer (i.e., the first character of the string).
 * The buffer pointed to by `nbuf' must have length >= MAXNBUF.
 */

static moo_bch_t* sprintn_lower (moo_bch_t* nbuf, moo_uintmax_t num, int base, moo_ooi_t *lenp)
{
	moo_bch_t* p;

	p = nbuf;
	*p = '\0';
	do { *++p = hex2ascii_lower[num % base]; } while (num /= base);

	if (lenp) *lenp = p - nbuf;
	return p; /* returns the end */
}

static moo_bch_t* sprintn_upper (moo_bch_t* nbuf, moo_uintmax_t num, int base, moo_ooi_t *lenp)
{
	moo_bch_t* p;

	p = nbuf;
	*p = '\0';
	do { *++p = hex2ascii_upper[num % base]; } while (num /= base);

	if (lenp) *lenp = p - nbuf;
	return p; /* returns the end */
}

/* ------------------------------------------------------------------------- */
static int put_ooch (moo_t* moo, moo_oow_t mask, moo_ooch_t ch, moo_oow_t len)
{
	/* this is not equivalent to put_oocs(moo,mask,&ch, 1);
	 * this function is to emit a single character multiple times */
	moo_oow_t rem;

	if (len <= 0) return 1;

	if (moo->log.len > 0 && moo->log.last_mask != mask)
	{
		/* the mask has changed. commit the buffered text */

/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
		if (moo->log.ptr[moo->log.len - 1] != '\n')
		{
			/* no line ending - append a line terminator */
			moo->log.ptr[moo->log.len++] = '\n';
		}
		moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

redo:
	rem = 0;
	if (len > moo->log.capa - moo->log.len)
	{
		moo_oow_t newcapa, max;
		moo_ooch_t* tmp;

		max = MOO_TYPE_MAX(moo_oow_t) - moo->log.len;
		if (len > max)
		{
			/* data too big. */
			rem += len - max;
			len = max;
		}

		newcapa = MOO_ALIGN_POW2(moo->log.len + len, MOO_LOG_CAPA_ALIGN); /* TODO: adjust this capacity */
		if (newcapa > moo->option.log_maxcapa) 
		{
			/* [NOTE]
			 * it doesn't adjust newcapa to moo->option.log_maxcapa.
			 * nor does it cut the input to fit it into the adjusted capacity.
			 * if maxcapa set is not aligned to MOO_LOG_CAPA_ALIGN,
			 * the largest buffer capacity may be suboptimal */
			goto make_do;
		}

		/* +1 to handle line ending injection more easily */
		tmp = moo_reallocmem (moo, moo->log.ptr, (newcapa + 1) * MOO_SIZEOF(*tmp)); 
		if (!tmp) 
		{
		make_do:
			if (moo->log.len > 0)
			{
				/* can't expand the buffer. just flush the existing contents */
				/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
				if (moo->log.ptr[moo->log.len - 1] != '\n')
				{
					/* no line ending - append a line terminator */
					moo->log.ptr[moo->log.len++] = '\n';
				}
				moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
				moo->log.len = 0;
			}

			if (len > moo->log.capa)
			{
				rem += len - moo->log.capa;
				len = moo->log.capa;
			}

		}
		else
		{
			moo->log.ptr = tmp;
			moo->log.capa = newcapa; 
		}
	}

	while (len > 0)
	{
		moo->log.ptr[moo->log.len++] = ch;
		len--;
	}
	moo->log.last_mask = mask;

	if (rem > 0)
	{
		len = rem;
		goto redo;
	}

	
	return 1; /* success */
}

static int put_oocs (moo_t* moo, moo_oow_t mask, const moo_ooch_t* ptr, moo_oow_t len)
{
	moo_oow_t rem;

	if (len <= 0) return 1;

	if (moo->log.len > 0 && moo->log.last_mask != mask)
	{
		/* the mask has changed. commit the buffered text */
/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
		if (moo->log.ptr[moo->log.len - 1] != '\n')
		{
			/* no line ending - append a line terminator */
			moo->log.ptr[moo->log.len++] = '\n';
		}

		moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

redo:
	rem = 0;
	if (len > moo->log.capa - moo->log.len)
	{
		moo_oow_t newcapa, max;
		moo_ooch_t* tmp;

		max = MOO_TYPE_MAX(moo_oow_t) - moo->log.len;
		if (len > max) 
		{
			/* data too big. */
			rem += len - max;
			len = max;
		}

		newcapa = MOO_ALIGN_POW2(moo->log.len + len, 512); /* TODO: adjust this capacity */
		if (newcapa > moo->option.log_maxcapa)
		{
			/* [NOTE]
			 * it doesn't adjust newcapa to moo->option.log_maxcapa.
			 * nor does it cut the input to fit it into the adjusted capacity.
			 * if maxcapa set is not aligned to MOO_LOG_CAPA_ALIGN,
			 * the largest buffer capacity may be suboptimal */
			goto make_do;
		}

		/* +1 to handle line ending injection more easily */
		tmp = moo_reallocmem (moo, moo->log.ptr, (newcapa + 1) * MOO_SIZEOF(*tmp));
		if (!tmp) 
		{
		make_do:
			if (moo->log.len > 0)
			{
				/* can't expand the buffer. just flush the existing contents */
				/* TODO: HANDLE LINE ENDING CONVENTION BETTER... */
				if (moo->log.ptr[moo->log.len - 1] != '\n')
				{
					/* no line ending - append a line terminator */
					moo->log.ptr[moo->log.len++] = '\n';
				}
				moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
				moo->log.len = 0;
			}

			if (len > moo->log.capa)
			{
				rem += len - moo->log.capa;
				len = moo->log.capa;
			}
		}
		else
		{
			moo->log.ptr = tmp;
			moo->log.capa = newcapa;
		}
	}

	MOO_MEMCPY (&moo->log.ptr[moo->log.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->log.len += len;
	moo->log.last_mask = mask;

	if (rem > 0)
	{
		ptr += len;
		len = rem;
		goto redo;
	}

	return 1; /* success */
}

/* ------------------------------------------------------------------------- */

typedef moo_ooi_t (*outbfmt_t) (moo_t* moo, moo_oow_t mask, const moo_bch_t* fmt, ...);

static void print_object (moo_t* moo, moo_oow_t mask, moo_oop_t oop, outbfmt_t outbfmt)
{
	if (oop == moo->_nil)
	{
		outbfmt (moo, mask, "nil");
	}
	else if (oop == moo->_true)
	{
		outbfmt (moo, mask, "true");
	}
	else if (oop == moo->_false)
	{
		outbfmt (moo, mask, "false");
	}
	else if (MOO_OOP_IS_SMOOI(oop))
	{
		outbfmt (moo, mask, "%zd", MOO_OOP_TO_SMOOI(oop));
	}
	else if (MOO_OOP_IS_SMPTR(oop))
	{
		outbfmt (moo, mask, "%p", MOO_OOP_TO_SMPTR(oop));
	}
	else if (MOO_OOP_IS_CHAR(oop))
	{
		outbfmt (moo, mask, "$%.1C", MOO_OOP_TO_CHAR(oop));
	}
	else if (MOO_OOP_IS_ERROR(oop))
	{
		outbfmt (moo, mask, "error(%zd)", MOO_OOP_TO_ERROR(oop));
	}
	else
	{
		moo_oop_class_t c;
		moo_oow_t i;

		MOO_ASSERT (moo, MOO_OOP_IS_POINTER(oop));
		c = (moo_oop_class_t)MOO_OBJ_GET_CLASS(oop); /*MOO_CLASSOF(moo, oop);*/

		if (c == moo->_large_negative_integer)
		{
			moo_oow_t i;
			outbfmt (moo, mask, "-16r");
			for (i = MOO_OBJ_GET_SIZE(oop); i > 0;)
			{
				outbfmt (moo, mask, "%0*lX", (int)(MOO_SIZEOF(moo_liw_t) * 2), (unsigned long)((moo_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if (c == moo->_large_positive_integer)
		{
			moo_oow_t i;
			outbfmt (moo, mask, "16r");
			for (i = MOO_OBJ_GET_SIZE(oop); i > 0;)
			{
				outbfmt (moo, mask, "%0*lX", (int)(MOO_SIZEOF(moo_liw_t) * 2), (unsigned long)((moo_oop_liword_t)oop)->slot[--i]);
			}
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_CHAR)
		{
			if (c == moo->_symbol) 
			{
				outbfmt (moo, mask, "#%.*js", MOO_OBJ_GET_SIZE(oop), ((moo_oop_char_t)oop)->slot);
			}
			else /*if ((moo_oop_t)c == moo->_string)*/
			{
				moo_ooch_t ch;
				int escape = 0;

				for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
				{
					ch = ((moo_oop_char_t)oop)->slot[i];
					if (ch < ' ') 
					{
						escape = 1;
						break;
					}
				}

				if (escape)
				{
					moo_ooch_t escaped;

					outbfmt (moo, mask, "S'");
					for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
					{
						ch = ((moo_oop_char_t)oop)->slot[i];
						if (ch < ' ') 
						{
							switch (ch)
							{
								case '\0':
									escaped = '0';
									break;
								case '\n':
									escaped = 'n';
									break;
								case '\r':
									escaped = 'r';
									break;
								case '\t':
									escaped = 't';
									break;
								case '\f':
									escaped = 'f';
									break;
								case '\b':
									escaped = 'b';
									break;
								case '\v':
									escaped = 'v';
									break;
								case '\a':
									escaped = 'a';
									break;
								default:
									escaped = ch;
									break;
							}

							if (escaped == ch)
								outbfmt (moo, mask, "\\x%X", ch);
							else
								outbfmt (moo, mask, "\\%jc", escaped);
						}
						else
						{
							outbfmt (moo, mask, "%jc", ch);
						}
					}
					
					outbfmt (moo, mask, "'");
				}
				else
				{
					outbfmt (moo, mask, "'%.*js'", MOO_OBJ_GET_SIZE(oop), ((moo_oop_char_t)oop)->slot);
				}
			}
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_BYTE)
		{
			outbfmt (moo, mask, "#[");
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				outbfmt (moo, mask, " %d", ((moo_oop_byte_t)oop)->slot[i]);
			}
			outbfmt (moo, mask, "]");
		}
		
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_HALFWORD)
		{
			outbfmt (moo, mask, "#[["); /* TODO: fix this symbol/notation */
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				outbfmt (moo, mask, " %zX", (moo_oow_t)((moo_oop_halfword_t)oop)->slot[i]);
			}
			outbfmt (moo, mask, "]]");
		}
		else if (MOO_OBJ_GET_FLAGS_TYPE(oop) == MOO_OBJ_TYPE_WORD)
		{
			outbfmt (moo, mask, "#[[["); /* TODO: fix this symbol/notation */
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				outbfmt (moo, mask, " %zX", ((moo_oop_word_t)oop)->slot[i]);
			}
			outbfmt (moo, mask, "]]]");
		}
		else if (c == moo->_array)
		{
			outbfmt (moo, mask, "#(");
			for (i = 0; i < MOO_OBJ_GET_SIZE(oop); i++)
			{
				outbfmt (moo, mask, " ");
				print_object (moo, mask, ((moo_oop_oop_t)oop)->slot[i], outbfmt);
			}
			outbfmt (moo, mask, ")");
		}
		else if (c == moo->_class)
		{
			/* print the class name */
			outbfmt (moo, mask, "%.*js", MOO_OBJ_GET_SIZE(((moo_oop_class_t)oop)->name), ((moo_oop_class_t)oop)->name->slot);
		}
		else if (c == moo->_association)
		{
			outbfmt (moo, mask, "%O -> %O", ((moo_oop_association_t)oop)->key, ((moo_oop_association_t)oop)->value);
		}
		else
		{
			outbfmt (moo, mask, "instance of %.*js(%p)", MOO_OBJ_GET_SIZE(c->name), ((moo_oop_char_t)c->name)->slot, oop);
		}
	}
}

/* ------------------------------------------------------------------------- */

#undef FMTCHAR_IS_BCH
#undef FMTCHAR_IS_UCH
#undef FMTCHAR_IS_OOCH
#undef fmtchar_t
#undef logfmtv
#define fmtchar_t moo_bch_t
#define logfmtv __logbfmtv
#define FMTCHAR_IS_BCH
#if defined(MOO_OOCH_IS_BCH)
#	define FMTCHAR_IS_OOCH
#endif
#include "logfmtv.h"

#undef FMTCHAR_IS_BCH
#undef FMTCHAR_IS_UCH
#undef FMTCHAR_IS_OOCH
#undef fmtchar_t
#undef logfmtv
#define fmtchar_t moo_uch_t
#define logfmtv __logufmtv
#define FMTCHAR_IS_UCH
#if defined(MOO_OOCH_IS_UCH)
#	define FMTCHAR_IS_OOCH
#endif
#include "logfmtv.h" 


static int _logbfmtv (moo_t* moo, const moo_bch_t* fmt, moo_fmtout_t* data, va_list ap)
{
	return __logbfmtv (moo, fmt, data, ap, moo_logbfmt);
}

static int _logufmtv (moo_t* moo, const moo_uch_t* fmt, moo_fmtout_t* data, va_list ap)
{
	return __logufmtv (moo, fmt, data, ap, moo_logbfmt);
}


moo_ooi_t moo_logbfmt (moo_t* moo, moo_oow_t mask, const moo_bch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_t fo;

	fo.mask = mask;
	fo.putch = put_ooch;
	fo.putcs = put_oocs;

	va_start (ap, fmt);
	x = _logbfmtv (moo, fmt, &fo, ap);
	va_end (ap);

	if (moo->log.len > 0 && moo->log.ptr[moo->log.len - 1] == '\n')
	{
		moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}
	return (x <= -1)? -1: fo.count;
}

moo_ooi_t moo_logufmt (moo_t* moo, moo_oow_t mask, const moo_uch_t* fmt, ...)
{
	int x;
	va_list ap;
	moo_fmtout_t fo;

	fo.mask = mask;
	fo.putch = put_ooch;
	fo.putcs = put_oocs;

	va_start (ap, fmt);
	x = _logufmtv (moo, fmt, &fo, ap);
	va_end (ap);

	if (moo->log.len > 0 && moo->log.ptr[moo->log.len - 1] == '\n')
	{
		moo->vmprim.log_write (moo, moo->log.last_mask, moo->log.ptr, moo->log.len);
		moo->log.len = 0;
	}

	return (x <= -1)? -1: fo.count;
}


/* -------------------------------------------------------------------------- 
 * ERROR MESSAGE FORMATTING
 * -------------------------------------------------------------------------- */

static int put_errch (moo_t* moo, moo_oow_t mask, moo_ooch_t ch, moo_oow_t len)
{
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;
	if (len > max) len = max;

	if (len <= 0) return 1;

	while (len > 0)
	{
		moo->errmsg.buf[moo->errmsg.len++] = ch;
		len--;
	}
	moo->errmsg.buf[moo->errmsg.len] = '\0';

	return 1; /* success */
}

static int put_errcs (moo_t* moo, moo_oow_t mask, const moo_ooch_t* ptr, moo_oow_t len)
{
	moo_oow_t max;

	max = MOO_COUNTOF(moo->errmsg.buf) - moo->errmsg.len - 1;
	if (len > max) len = max;

	if (len <= 0) return 1;

	MOO_MEMCPY (&moo->errmsg.buf[moo->errmsg.len], ptr, len * MOO_SIZEOF(*ptr));
	moo->errmsg.len += len;
	moo->errmsg.buf[moo->errmsg.len] = '\0';

	return 1; /* success */
}


static moo_ooi_t __errbfmtv (moo_t* moo, moo_oow_t mask, const moo_bch_t* fmt, ...);

static int _errbfmtv (moo_t* moo, const moo_bch_t* fmt, moo_fmtout_t* data, va_list ap)
{
	return __logbfmtv (moo, fmt, data, ap, __errbfmtv);
}

static int _errufmtv (moo_t* moo, const moo_uch_t* fmt, moo_fmtout_t* data, va_list ap)
{
	return __logufmtv (moo, fmt, data, ap, __errbfmtv);
}

static moo_ooi_t __errbfmtv (moo_t* moo, moo_oow_t mask, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_t fo;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errbfmtv (moo, fmt, &fo, ap);
	va_end (ap);

	return fo.count;
}

void moo_seterrbfmt (moo_t* moo, moo_errnum_t errnum, const moo_bch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_t fo;

	moo->errnum = errnum;
	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errbfmtv (moo, fmt, &fo, ap);
	va_end (ap);
}

void moo_seterrufmt (moo_t* moo, moo_errnum_t errnum, const moo_uch_t* fmt, ...)
{
	va_list ap;
	moo_fmtout_t fo;

	moo->errnum = errnum;
	moo->errmsg.len = 0;

	fo.mask = 0; /* not used */
	fo.putch = put_errch;
	fo.putcs = put_errcs;

	va_start (ap, fmt);
	_errufmtv (moo, fmt, &fo, ap);
	va_end (ap);
}
