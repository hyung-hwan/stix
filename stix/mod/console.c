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


#include "console.h"
#include <stix-utl.h>

#include <unistd.h>
#include <fcntl.h> 

#include <stdlib.h> /* getenv */

#include <curses.h>
#include <term.h>

#include <sys/ioctl.h>

typedef struct console_t console_t;
struct console_t
{
	int fd;
	int fd_opened;
	char* cup;
	char* clear;
};
/* ------------------------------------------------------------------------ */

static stix_pfrc_t pf_open (stix_t* stix, stix_ooi_t nargs)
{
#if defined(_WIN32)
	HANDLE h;
	stix_ooi_t immv;

	h = GetStdHandle(STD_INPUT_HANDLE);
	if (h == INVALID_HANDLE_VALUE) return 0;
	if (h == NULL)
	{
	}

	imm = stix_makeimm (stix, h);
	if (imm <= -1) 
	{
		/* error */
	}


#else

	console_t* con;
	int err;
	char* term;

	con = stix_callocmem (stix, STIX_SIZEOF(*con));
	if (!con) return 0;

	if (isatty(1))
	{
		con->fd = 1;
	}
	else
	{
		con->fd = open ("/dev/tty", O_RDWR, 0);
		if (con->fd == -1)
		{
			/* TODO: failed to open /dev/stdout */
			stix_freemem (stix, con);
			return 0;
		}

		con->fd_opened = 1;
	}

	term = getenv ("TERM");
	if (term && setupterm (term, con->fd, &err) == OK)
	{
	}

	con->cup = tigetstr ("cup"); /* TODO: error check */
	con->clear = tigetstr ("clear");

#if 0
	{
	const char* cup, * clear;
	struct winsize wsz;

	cup = tigetstr ("cup");
	clear = tigetstr ("clear");
	ioctl (fd, TIOCGWINSZ, &wsz);

	write (fd, clear, strlen(clear));
	/*write (fd, tparm (cup, wsz.ws_row - 1, 0), strlen(tparm (cup, wsz.ws_row - 1, 0)));*/
	write (fd, tiparm (cup, 0, 0), strlen(tiparm (cup, 0, 0)));
	}
#endif

#endif

	STIX_STACK_SETRET (stix, nargs, STIX_SMOOI_TO_OOP((stix_oow_t)con));
	return STIX_PF_SUCCESS;
}

static stix_pfrc_t pf_close (stix_t* stix, stix_ooi_t nargs)
{
#if defined(_WIN32)
	HANDLE h;

	h = STIX_STACK_GETARG (stix, nargs, 0);
#else
#endif
	console_t* con;

	con = STIX_OOP_TO_SMOOI(STIX_STACK_GETARG (stix, nargs, 0));
	/* TODO: sanity check */

	if (con->fd_opened) close (con->fd);

	stix_freemem (stix, con);
	STIX_STACK_SETRETTORCV (stix, nargs);
	return STIX_PF_SUCCESS;
}

static stix_pfrc_t pf_write (stix_t* stix, stix_ooi_t nargs)
{
	console_t* con;
	stix_oop_char_t oomsg;

	stix_oow_t ucspos, ucsrem, ucslen, bcslen;
	stix_bch_t bcs[1024];
	int n;

	con = STIX_OOP_TO_SMOOI(STIX_STACK_GETARG (stix, nargs, 0));
	oomsg = (stix_oop_char_t)STIX_STACK_GETARG (stix, nargs, 1);

	if (STIX_CLASSOF(stix,oomsg) != stix->_string)
	{
/* TODO: invalid message */
		return STIX_PF_FAILURE;
	}

	ucspos = 0;
	ucsrem = STIX_OBJ_GET_SIZE(oomsg);
	while (ucsrem > 0)
	{
		ucslen = ucsrem;
		bcslen = STIX_COUNTOF(bcs);
		if ((n = stix_convootobchars (stix, &oomsg->slot[ucspos], &ucslen, bcs, &bcslen)) <= -1)
		{
			if (n != -2 || ucslen <= 0) 
			{
				stix_seterrnum (stix, STIX_EECERR);
				return STIX_PF_HARD_FAILURE;
			}
		}

		write (con->fd, bcs, bcslen); /* TODO: error handling */
/* TODO: abort looping for async processing???? */
		ucspos += ucslen;
		ucsrem -= ucslen;
	}

	STIX_STACK_SETRETTORCV (stix, nargs); /* TODO: change return code */
	return STIX_PF_SUCCESS;
}

static stix_pfrc_t pf_clear (stix_t* stix, stix_ooi_t nargs)
{
	console_t* con;

	con = STIX_OOP_TO_SMOOI(STIX_STACK_GETARG(stix, nargs, 0));

	write (con->fd, con->clear, strlen(con->clear));

	STIX_STACK_SETRETTORCV (stix, nargs);
	return STIX_PF_SUCCESS;
}

static stix_pfrc_t pf_setcursor (stix_t* stix, stix_ooi_t nargs)
{
	console_t* con;
	stix_oop_oop_t point;
	char* cup;

	con = STIX_OOP_TO_SMOOI(STIX_STACK_GETARG(stix, nargs, 0));
	point = STIX_STACK_GETARG(stix, nargs, 1);

/* TODO: error check, class check, size check.. */
	if (STIX_OBJ_GET_SIZE(point) != 2)
	{
		return STIX_PF_FAILURE;
	}

	cup = tiparm (con->cup, STIX_OOP_TO_SMOOI(point->slot[1]), STIX_OOP_TO_SMOOI(point->slot[0]));
	write (con->fd, cup, strlen(cup)); /* TODO: error check */

	STIX_STACK_SETRETTORCV (stix, nargs);
	return STIX_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

typedef struct fnctab_t fnctab_t;
struct fnctab_t
{
	const stix_bch_t* name;
	stix_pfimpl_t handler;
};

static fnctab_t fnctab[] =
{
	{ "clear",      pf_clear     },
	{ "close",      pf_close     },
	{ "open",       pf_open      },
	{ "setcursor",  pf_setcursor },
	{ "write",      pf_write     },
};

/* ------------------------------------------------------------------------ */

static stix_pfimpl_t query (stix_t* stix, stix_mod_t* mod, const stix_ooch_t* name)
{
	int left, right, mid, n;

	left = 0; right = STIX_COUNTOF(fnctab) - 1;

	while (left <= right)
	{
		mid = (left + right) / 2;

		n = stix_compoocbcstr (name, fnctab[mid].name);
		if (n < 0) right = mid - 1; 
		else if (n > 0) left = mid + 1;
		else
		{
			return fnctab[mid].handler;
		}
	}

	stix->errnum = STIX_ENOENT;
	return STIX_NULL;
}


static void unload (stix_t* stix, stix_mod_t* mod)
{
	/* TODO: close all open handle?? */
}


int stix_mod_console (stix_t* stix, stix_mod_t* mod)
{
	mod->query = query;
	mod->unload = unload; 
	mod->ctx = STIX_NULL;
	return 0;
}
