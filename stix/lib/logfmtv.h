/*
 * $Id$
 *
    Copyright (c) 2014-2016 Chung, Hyung-Hwan. All rights reserved.

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

/*
 * This file contains a formatted output routine derived from kvprintf() 
 * of FreeBSD. It has been heavily modified and bug-fixed.
 */

/*
 * Copyright (c) 1986, 1988, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


/* NOTE: data output is aborted if the data limit is reached or 
 *       I/O error occurs  */

#undef PUT_OOCH
#undef PUT_OOCS

#define PUT_OOCH(c,n) do { \
	int xx; \
	if ((xx = data->putch (stix, data->mask, c, n)) <= -1) goto oops; \
	if (xx == 0) goto done; \
	data->count += n; \
} while (0)

#define PUT_OOCS(ptr,len) do { \
	int xx; \
	if ((xx = data->putcs (stix, data->mask, ptr, len)) <= -1) goto oops; \
	if (xx == 0) goto done; \
	data->count += len; \
} while (0)

int logfmtv (stix_t* stix, const fmtchar_t* fmt, stix_fmtout_t* data, va_list ap)
{
	const fmtchar_t* percent;
#if defined(FMTCHAR_IS_OOCH)
	const fmtchar_t* checkpoint;
#endif
	stix_bch_t nbuf[MAXNBUF], bch;
	const stix_bch_t* nbufp;
	int n, base, neg, sign;
	stix_ooi_t tmp, width, precision;
	stix_ooch_t ch, padc;
	int lm_flag, lm_dflag, flagc, numlen;
	stix_uintmax_t num = 0;
	int stop = 0;

#if 0
	stix_bchbuf_t* fltfmt;
	stix_oochbuf_t* fltout;
#endif
	stix_bch_t* (*sprintn) (stix_bch_t* nbuf, stix_uintmax_t num, int base, stix_ooi_t* lenp);

	data->count = 0;

#if 0
	fltfmt = &stix->d->fltfmt;
	fltout = &stix->d->fltout;

	fltfmt->ptr  = fltfmt->sbuf;
	fltfmt->capa = STIX_COUNTOF(fltfmt->sbuf) - 1;

	fltout->ptr  = fltout->sbuf;
	fltout->capa = STIX_COUNTOF(fltout->sbuf) - 1;
#endif

	while (1)
	{
	#if defined(FMTCHAR_IS_OOCH)
		checkpoint = fmt;
		while ((ch = *fmt++) != '%' || stop) 
		{
			if (ch == '\0') 
			{
				PUT_OOCS (checkpoint, fmt - checkpoint - 1);
				goto done;
			}
		}
		PUT_OOCS (checkpoint, fmt - checkpoint - 1);
	#else
		while ((ch = *fmt++) != '%' || stop) 
		{
			if (ch == '\0') goto done;
			PUT_OOCH (ch, 1);
		}
	#endif
		percent = fmt - 1;

		padc = ' '; 
		width = 0; precision = 0;
		neg = 0; sign = 0;

		lm_flag = 0; lm_dflag = 0; flagc = 0; 
		sprintn = sprintn_lower;

reswitch:
		switch (ch = *fmt++) 
		{
		case '%': /* %% */
			bch = ch;
			goto print_lowercase_c;

		/* flag characters */
		case '.':
			if (flagc & FLAGC_DOT) goto invalid_format;
			flagc |= FLAGC_DOT;
			goto reswitch;

		case '#': 
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SHARP;
			goto reswitch;

		case ' ':
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SPACE;
			goto reswitch;

		case '+': /* place sign for signed conversion */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			flagc |= FLAGC_SIGN;
			goto reswitch;

		case '-': /* left adjusted */
			if (flagc & (FLAGC_WIDTH | FLAGC_DOT | FLAGC_LENMOD)) goto invalid_format;
			if (flagc & FLAGC_DOT)
			{
				goto invalid_format;
			}
			else
			{
				flagc |= FLAGC_LEFTADJ;
				if (flagc & FLAGC_ZEROPAD)
				{
					padc = ' ';
					flagc &= ~FLAGC_ZEROPAD;
				}
			}
			
			goto reswitch;

		case '*': /* take the length from the parameter */
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & (FLAGC_STAR2 | FLAGC_PRECISION)) goto invalid_format;
				flagc |= FLAGC_STAR2;

				precision = va_arg(ap, stix_ooi_t); /* this deviates from the standard printf that accepts 'int' */
				if (precision < 0) 
				{
					/* if precision is less than 0, 
					 * treat it as if no .precision is specified */
					flagc &= ~FLAGC_DOT;
					precision = 0;
				}
			} 
			else 
			{
				if (flagc & (FLAGC_STAR1 | FLAGC_WIDTH)) goto invalid_format;
				flagc |= FLAGC_STAR1;

				width = va_arg(ap, stix_ooi_t); /* it deviates from the standard printf that accepts 'int' */
				if (width < 0) 
				{
					/*
					if (flagc & FLAGC_LEFTADJ) 
						flagc  &= ~FLAGC_LEFTADJ;
					else
					*/
						flagc |= FLAGC_LEFTADJ;
					width = -width;
				}
			}
			goto reswitch;

		case '0': /* zero pad */
			if (flagc & FLAGC_LENMOD) goto invalid_format;
			if (!(flagc & (FLAGC_DOT | FLAGC_LEFTADJ)))
			{
				padc = '0';
				flagc |= FLAGC_ZEROPAD;
				goto reswitch;
			}
		/* end of flags characters */

		case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			for (n = 0;; ++fmt) 
			{
				n = n * 10 + ch - '0';
				ch = *fmt;
				if (ch < '0' || ch > '9') break;
			}
			if (flagc & FLAGC_DOT) 
			{
				if (flagc & FLAGC_STAR2) goto invalid_format;
				precision = n;
				flagc |= FLAGC_PRECISION;
			}
			else 
			{
				if (flagc & FLAGC_STAR1) goto invalid_format;
				width = n;
				flagc |= FLAGC_WIDTH;
			}
			goto reswitch;

		/* length modifiers */
		case 'h': /* short int */
		case 'l': /* long int */
		case 'q': /* long long int */
		case 'j': /* stix_intmax_t/stix_uintmax_t */
		case 'z': /* stix_ooi_t/stix_oow_t */
		case 't': /* ptrdiff_t */
			if (lm_flag & (LF_LD | LF_QD)) goto invalid_format;

			flagc |= FLAGC_LENMOD;
			if (lm_dflag)
			{
				/* error */
				goto invalid_format;
			}
			else if (lm_flag)
			{
				if (lm_tab[ch - 'a'].dflag && lm_flag == lm_tab[ch - 'a'].flag)
				{
					lm_flag &= ~lm_tab[ch - 'a'].flag;
					lm_flag |= lm_tab[ch - 'a'].dflag;
					lm_dflag |= lm_flag;
					goto reswitch;
				}
				else
				{
					/* error */
					goto invalid_format;
				}
			}
			else 
			{
				lm_flag |= lm_tab[ch - 'a'].flag;
				goto reswitch;
			}
			break;

		case 'L': /* long double */
			if (flagc & FLAGC_LENMOD) 
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_LD;
			goto reswitch;

		case 'Q': /* __float128 */
			if (flagc & FLAGC_LENMOD)
			{
				/* conflict with other length modifier */
				goto invalid_format; 
			}
			flagc |= FLAGC_LENMOD;
			lm_flag |= LF_QD;
			goto reswitch;
		/* end of length modifiers */

		case 'n':
			if (lm_flag & LF_J) /* j */
				*(va_arg(ap, stix_intmax_t*)) = data->count;
			else if (lm_flag & LF_Z) /* z */
				*(va_arg(ap, stix_ooi_t*)) = data->count;
		#if (STIX_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q) /* ll */
				*(va_arg(ap, long long int*)) = data->count;
		#endif
			else if (lm_flag & LF_L) /* l */
				*(va_arg(ap, long int*)) = data->count;
			else if (lm_flag & LF_H) /* h */
				*(va_arg(ap, short int*)) = data->count;
			else if (lm_flag & LF_C) /* hh */
				*(va_arg(ap, char*)) = data->count;
			else if (flagc & FLAGC_LENMOD)
			{
				stix->errnum = STIX_EINVAL;
				goto oops;
			}
			else
				*(va_arg(ap, int*)) = data->count;
			break;
 
		/* signed integer conversions */
		case 'd':
		case 'i': /* signed conversion */
			base = 10;
			sign = 1;
			goto handle_sign;
		/* end of signed integer conversions */

		/* unsigned integer conversions */
		case 'o': 
			base = 8;
			goto handle_nosign;
		case 'u':
			base = 10;
			goto handle_nosign;
		case 'X':
			sprintn = sprintn_upper;
		case 'x':
			base = 16;
			goto handle_nosign;
		/* end of unsigned integer conversions */

		case 'p': /* pointer */
			base = 16;

			if (width == 0) flagc |= FLAGC_SHARP;
			else flagc &= ~FLAGC_SHARP;

			num = (stix_uintptr_t)va_arg(ap, void*);
			goto number;

		case 'c':
		{
			/* zeropad must not take effect for 'c' */
			if (flagc & FLAGC_ZEROPAD) padc = ' '; 
			if (lm_flag & LF_L) goto uppercase_c;

		lowercase_c:
			bch = STIX_SIZEOF(stix_bch_t) < STIX_SIZEOF(int)? va_arg(ap, int): va_arg(ap, stix_bch_t);

		print_lowercase_c:
			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			PUT_OOCH (bch, 1);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			break;
		}

		case 'C':
		{
			stix_ooch_t ooch;

			/* zeropad must not take effect for 'C' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_H) goto lowercase_c;
		uppercase_c:
			ooch = STIX_SIZEOF(stix_ooch_t) < STIX_SIZEOF(int)? va_arg(ap, int): va_arg(ap, stix_ooch_t);

			/* precision 0 doesn't kill the letter */
			width--;
			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			PUT_OOCH (ooch, 1);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			break;
		}

		case 's':
		{
			const stix_bch_t* bsp;
			stix_oow_t bslen, slen;

			/* zeropad must not take effect for 'S' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_L) goto uppercase_s;
		lowercase_s:

			bsp = va_arg (ap, stix_bch_t*);
			if (bsp == STIX_NULL) bsp = bch_nullstr;

			/* get the length */
			for (bslen = 0; bsp[bslen]; bslen++);

			if (stix_utf8toucs (bsp, &bslen, STIX_NULL, &slen) <= -1)
			{ 
				/* conversion error */
				stix->errnum = STIX_EECERR;
				goto oops;
			}

			/* slen holds the length after conversion */
			n = slen;
			if ((flagc & FLAGC_DOT) && precision < slen) n = precision;
			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);

			{
				stix_ooch_t conv_buf[32]; 
				stix_oow_t conv_len, src_len, tot_len = 0;
				while (n > 0)
				{
					STIX_ASSERT (bslen > tot_len);

					src_len = bslen - tot_len;
					conv_len = STIX_COUNTOF(conv_buf);

					/* this must not fail since the dry-run above was successful */
					stix_utf8toucs (&bsp[tot_len], &src_len, conv_buf, &conv_len);
					tot_len += src_len;

					if (conv_len > n) conv_len = n;
					PUT_OOCS (conv_buf, conv_len);

					n -= conv_len;
				}
			}
			
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			break;
		}

		case 'S':
		{
			const stix_ooch_t* sp;

			/* zeropad must not take effect for 's' */
			if (flagc & FLAGC_ZEROPAD) padc = ' ';
			if (lm_flag & LF_H) goto lowercase_s;
		uppercase_s:
			sp = va_arg (ap, stix_ooch_t*);
			if (sp == STIX_NULL) sp = ooch_nullstr;

			if (flagc & FLAGC_DOT)
			{
				for (n = 0; n < precision && sp[n]; n++);
			}
			else
			{
				for (n = 0; sp[n]; n++);
			}

			width -= n;

			if (!(flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			PUT_OOCS (sp, n);
			if ((flagc & FLAGC_LEFTADJ) && width > 0) PUT_OOCH (padc, width);
			break;
		}

		case 'O': /* object - ignore precision, width, adjustment */
			print_object (stix, data->mask, va_arg (ap, stix_oop_t));
			break;

#if 0
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		/*
		case 'a':
		case 'A':
		*/
		{
			/* let me rely on snprintf until i implement float-point to string conversion */
			int q;
			stix_oow_t fmtlen;
		#if (STIX_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			__float128 v_qd;
		#endif
			long double v_ld;
			double v_d;
			int dtype = 0;
			stix_oow_t newcapa;

			if (lm_flag & LF_J)
			{
			#if (STIX_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF) && (STIX_SIZEOF_FLTMAX_T == STIX_SIZEOF___FLOAT128)
				v_qd = va_arg (ap, stix_fltmax_t);
				dtype = LF_QD;
			#elif STIX_SIZEOF_FLTMAX_T == STIX_SIZEOF_DOUBLE
				v_d = va_arg (ap, stix_fltmax_t);
			#elif STIX_SIZEOF_FLTMAX_T == STIX_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, stix_fltmax_t);
				dtype = LF_LD;
			#else
				#error Unsupported stix_flt_t
			#endif
			}
			else if (lm_flag & LF_Z)
			{
				/* stix_flt_t is limited to double or long double */

				/* precedence goes to double if sizeof(double) == sizeof(long double) 
				 * for example, %Lf didn't work on some old platforms.
				 * so i prefer the format specifier with no modifier.
				 */
			#if STIX_SIZEOF_FLT_T == STIX_SIZEOF_DOUBLE
				v_d = va_arg (ap, stix_flt_t);
			#elif STIX_SIZEOF_FLT_T == STIX_SIZEOF_LONG_DOUBLE
				v_ld = va_arg (ap, stix_flt_t);
				dtype = LF_LD;
			#else
				#error Unsupported stix_flt_t
			#endif
			}
			else if (lm_flag & (LF_LD | LF_L))
			{
				v_ld = va_arg (ap, long double);
				dtype = LF_LD;
			}
		#if (STIX_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
			else if (lm_flag & (LF_QD | LF_Q))
			{
				v_qd = va_arg (ap, __float128);
				dtype = LF_QD;
			}
		#endif
			else if (flagc & FLAGC_LENMOD)
			{
				stix->errnum = STIX_EINVAL;
				goto oops;
			}
			else
			{
				v_d = va_arg (ap, double);
			}

			fmtlen = fmt - percent;
			if (fmtlen > fltfmt->capa)
			{
				if (fltfmt->ptr == fltfmt->sbuf)
				{
					fltfmt->ptr = STIX_MMGR_ALLOC (STIX_MMGR_GETDFL(), STIX_SIZEOF(*fltfmt->ptr) * (fmtlen + 1));
					if (fltfmt->ptr == STIX_NULL) goto oops;
				}
				else
				{
					stix_mchar_t* tmpptr;

					tmpptr = STIX_MMGR_REALLOC (STIX_MMGR_GETDFL(), fltfmt->ptr, STIX_SIZEOF(*fltfmt->ptr) * (fmtlen + 1));
					if (tmpptr == STIX_NULL) goto oops;
					fltfmt->ptr = tmpptr;
				}

				fltfmt->capa = fmtlen;
			}

			/* compose back the format specifier */
			fmtlen = 0;
			fltfmt->ptr[fmtlen++] = '%';
			if (flagc & FLAGC_SPACE) fltfmt->ptr[fmtlen++] = ' ';
			if (flagc & FLAGC_SHARP) fltfmt->ptr[fmtlen++] = '#';
			if (flagc & FLAGC_SIGN) fltfmt->ptr[fmtlen++] = '+';
			if (flagc & FLAGC_LEFTADJ) fltfmt->ptr[fmtlen++] = '-';
			if (flagc & FLAGC_ZEROPAD) fltfmt->ptr[fmtlen++] = '0';

			if (flagc & FLAGC_STAR1) fltfmt->ptr[fmtlen++] = '*';
			else if (flagc & FLAGC_WIDTH) 
			{
				fmtlen += stix_fmtuintmaxtombs (
					&fltfmt->ptr[fmtlen], fltfmt->capa - fmtlen, 
					width, 10, -1, '\0', STIX_NULL);
			}
			if (flagc & FLAGC_DOT) fltfmt->ptr[fmtlen++] = '.';
			if (flagc & FLAGC_STAR2) fltfmt->ptr[fmtlen++] = '*';
			else if (flagc & FLAGC_PRECISION) 
			{
				fmtlen += stix_fmtuintmaxtombs (
					&fltfmt->ptr[fmtlen], fltfmt->capa - fmtlen, 
					precision, 10, -1, '\0', STIX_NULL);
			}

			if (dtype == LF_LD)
				fltfmt->ptr[fmtlen++] = 'L';
		#if (STIX_SIZEOF___FLOAT128 > 0)
			else if (dtype == LF_QD)
				fltfmt->ptr[fmtlen++] = 'Q';
		#endif

			fltfmt->ptr[fmtlen++] = ch;
			fltfmt->ptr[fmtlen] = '\0';

		#if defined(HAVE_SNPRINTF)
			/* nothing special here */
		#else
			/* best effort to avoid buffer overflow when no snprintf is available. 
			 * i really can't do much if it happens. */
			newcapa = precision + width + 32;
			if (fltout->capa < newcapa)
			{
				STIX_ASSERT (fltout->ptr == fltout->sbuf);

				fltout->ptr = STIX_MMGR_ALLOC (STIX_MMGR_GETDFL(), STIX_SIZEOF(char_t) * (newcapa + 1));
				if (fltout->ptr == STIX_NULL) goto oops;
				fltout->capa = newcapa;
			}
		#endif

			while (1)
			{

				if (dtype == LF_LD)
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((stix_mchar_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_ld);
				#else
					q = sprintf ((stix_mchar_t*)fltout->ptr, fltfmt->ptr, v_ld);
				#endif
				}
			#if (STIX_SIZEOF___FLOAT128 > 0) && defined(HAVE_QUADMATH_SNPRINTF)
				else if (dtype == LF_QD)
				{
					q = quadmath_snprintf ((stix_mchar_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_qd);
				}
			#endif
				else
				{
				#if defined(HAVE_SNPRINTF)
					q = snprintf ((stix_mchar_t*)fltout->ptr, fltout->capa + 1, fltfmt->ptr, v_d);
				#else
					q = sprintf ((stix_mchar_t*)fltout->ptr, fltfmt->ptr, v_d);
				#endif
				}
				if (q <= -1) goto oops;
				if (q <= fltout->capa) break;

				newcapa = fltout->capa * 2;
				if (newcapa < q) newcapa = q;

				if (fltout->ptr == fltout->sbuf)
				{
					fltout->ptr = STIX_MMGR_ALLOC (STIX_MMGR_GETDFL(), STIX_SIZEOF(char_t) * (newcapa + 1));
					if (fltout->ptr == STIX_NULL) goto oops;
				}
				else
				{
					char_t* tmpptr;

					tmpptr = STIX_MMGR_REALLOC (STIX_MMGR_GETDFL(), fltout->ptr, STIX_SIZEOF(char_t) * (newcapa + 1));
					if (tmpptr == STIX_NULL) goto oops;
					fltout->ptr = tmpptr;
				}
				fltout->capa = newcapa;
			}

			if (STIX_SIZEOF(char_t) != STIX_SIZEOF(stix_mchar_t))
			{
				fltout->ptr[q] = '\0';
				while (q > 0)
				{
					q--;
					fltout->ptr[q] = ((stix_mchar_t*)fltout->ptr)[q];
				}
			}

			sp = fltout->ptr;
			flagc &= ~FLAGC_DOT;
			width = 0;
			precision = 0;
			goto print_lowercase_s;
		}
#endif


handle_nosign:
			sign = 0;
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (STIX_SIZEOF_UINTMAX_T > STIX_SIZEOF_OOW_T) && \
			    (STIX_SIZEOF_UINTMAX_T != STIX_SIZEOF_LONG_LONG) && \
			    (STIX_SIZEOF_UINTMAX_T != STIX_SIZEOF_LONG)
				/* GCC-compiled binaries crashed when getting stix_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < STIX_SIZEOF(stix_uintmax_t) / STIX_SIZEOF(stix_oow_t); i++)
				{	
				#if defined(STIX_ENDIAN_BIG)
					num = num << (8 * STIX_SIZEOF(stix_oow_t)) | (va_arg (ap, stix_oow_t));
				#else
					register int shift = i * STIX_SIZEOF(stix_oow_t);
					stix_oow_t x = va_arg (ap, stix_oow_t);
					num |= (stix_uintmax_t)x << (shift * 8);
				#endif
				}
			#else
				num = va_arg (ap, stix_uintmax_t);
			#endif
			}
#if 0
			else if (lm_flag & LF_T)
				num = va_arg (ap, stix_ptrdiff_t);
#endif
			else if (lm_flag & LF_Z)
				num = va_arg (ap, stix_oow_t);
			#if (STIX_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg (ap, unsigned long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg (ap, unsigned long int);
			else if (lm_flag & LF_H)
				num = (unsigned short int)va_arg (ap, int);
			else if (lm_flag & LF_C)
				num = (unsigned char)va_arg (ap, int);
			else
				num = va_arg (ap, unsigned int);
			goto number;

handle_sign:
			if (lm_flag & LF_J)
			{
			#if defined(__GNUC__) && \
			    (STIX_SIZEOF_INTMAX_T > STIX_SIZEOF_OOI_T) && \
			    (STIX_SIZEOF_UINTMAX_T != STIX_SIZEOF_LONG_LONG) && \
			    (STIX_SIZEOF_UINTMAX_T != STIX_SIZEOF_LONG)
				/* GCC-compiled binraries crashed when getting stix_uintmax_t with va_arg.
				 * This is just a work-around for it */
				int i;
				for (i = 0, num = 0; i < STIX_SIZEOF(stix_intmax_t) / STIX_SIZEOF(stix_oow_t); i++)
				{
				#if defined(STIX_ENDIAN_BIG)
					num = num << (8 * STIX_SIZEOF(stix_oow_t)) | (va_arg (ap, stix_oow_t));
				#else
					register int shift = i * STIX_SIZEOF(stix_oow_t);
					stix_oow_t x = va_arg (ap, stix_oow_t);
					num |= (stix_uintmax_t)x << (shift * 8);
				#endif
				}
			#else
				num = va_arg (ap, stix_intmax_t);
			#endif
			}

#if 0
			else if (lm_flag & LF_T)
				num = va_arg(ap, stix_ptrdiff_t);
#endif
			else if (lm_flag & LF_Z)
				num = va_arg (ap, stix_ooi_t);
			#if (STIX_SIZEOF_LONG_LONG > 0)
			else if (lm_flag & LF_Q)
				num = va_arg (ap, long long int);
			#endif
			else if (lm_flag & (LF_L | LF_LD))
				num = va_arg (ap, long int);
			else if (lm_flag & LF_H)
				num = (short int)va_arg (ap, int);
			else if (lm_flag & LF_C)
				num = (char)va_arg (ap, int);
			else
				num = va_arg (ap, int);

number:
			if (sign && (stix_intmax_t)num < 0) 
			{
				neg = 1;
				num = -(stix_intmax_t)num;
			}

			nbufp = sprintn (nbuf, num, base, &tmp);
			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 8) tmp++;
				else if (base == 16) tmp += 2;
			}
			if (neg) tmp++;
			else if (flagc & FLAGC_SIGN) tmp++;
			else if (flagc & FLAGC_SPACE) tmp++;

			numlen = nbufp - nbuf;
			if ((flagc & FLAGC_DOT) && precision > numlen) 
			{
				/* extra zeros for precision specified */
				tmp += (precision - numlen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && !(flagc & FLAGC_ZEROPAD) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (padc, width);
				width = 0;
			}

			if (neg) PUT_OOCH ('-', 1);
			else if (flagc & FLAGC_SIGN) PUT_OOCH ('+', 1);
			else if (flagc & FLAGC_SPACE) PUT_OOCH (' ', 1);

			if ((flagc & FLAGC_SHARP) && num != 0) 
			{
				if (base == 8) 
				{
					PUT_OOCH ('0', 1);
				} 
				else if (base == 16) 
				{
					PUT_OOCH ('0', 1);
					PUT_OOCH ('x', 1);
				}
			}

			if ((flagc & FLAGC_DOT) && precision > numlen)
			{
				/* extra zeros for precision specified */
				PUT_OOCH ('0', precision - numlen);
			}

			if (!(flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (padc, width);
			}

			while (*nbufp) PUT_OOCH (*nbufp--, 1); /* output actual digits */

			if ((flagc & FLAGC_LEFTADJ) && width > 0 && (width -= tmp) > 0)
			{
				PUT_OOCH (padc, width);
			}
			break;

invalid_format:
		#if defined(FMTCHAR_IS_OOCH)
			PUT_OOCS (percent, fmt - percent);
		#else
			while (percent < fmt) PUT_OOCH (*percent++, 1);
		#endif
			break;

		default:
		#if defined(FMTCHAR_IS_OOCH)
			PUT_OOCS (percent, fmt - percent);
		#else
			while (percent < fmt) PUT_OOCH (*percent++, 1);
		#endif
			/*
			 * Since we ignore an formatting argument it is no
			 * longer safe to obey the remaining formatting
			 * arguments as the arguments will no longer match
			 * the format specs.
			 */
			stop = 1;
			break;
		}
	}

done:
	return 0;

oops:
	return -1;
}
#undef PUT_OOCH