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

#include "_x11.h"
#include <moo-utl.h>

#include <errno.h>
#include <xcb/xcb.h>
#include <stdlib.h>

#define C MOO_METHOD_CLASS
#define I MOO_METHOD_INSTANCE


/**
 * @brief Bit mask to find event type regardless of event source.
 *
 * Each event in the X11 protocol contains an 8-bit type code.
 * The most-significant bit in this code is set if the event was
 * generated from a SendEvent request. This mask can be used to
 * determine the type of event regardless of how the event was
 * generated. See the X11R6 protocol specification for details.
 */
#define XCB_EVENT_RESPONSE_TYPE_MASK (0x7f)
#define XCB_EVENT_RESPONSE_TYPE(e)   (e->response_type &  XCB_EVENT_RESPONSE_TYPE_MASK)
#define XCB_EVENT_SENT(e)            (e->response_type & ~XCB_EVENT_RESPONSE_TYPE_MASK)


typedef struct x11_modctx_t x11_modctx_t;
struct x11_modctx_t
{
	moo_oop_class_t x11_class;
};

typedef struct x11_t x11_t;
struct x11_t
{
	xcb_connection_t* c;
	xcb_screen_t* screen;
	xcb_generic_event_t* curevt; /* most recent event received */
};

typedef struct x11_win_t x11_win_t;
struct x11_win_t
{
	xcb_window_t id;
	xcb_intern_atom_reply_t* dwar;
	xcb_connection_t* c;
};

typedef struct x11_gc_t x11_gc_t;
struct x11_gc_t
{
	xcb_gcontext_t id;
	xcb_window_t wid;
	xcb_connection_t* c;
};

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_connect (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	xcb_connection_t* c;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

/* TODO: accept display target as a parameter */
	if (nargs != 0)
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	if (x11->c)
	{
		MOO_DEBUG0 (moo, "<x11.connect> Unable to connect multiple times\n");
		goto softfail;
	}

	/*
	name = MOO_STACK_GETARG(moo, nargs, 0);

	if (!MOO_OBJ_IS_CHAR_POINTER(name)) 
	{
		moo_seterrnum (moo, MOO_EINVAL);
		goto softfail;
	}

	MOO_DEBUG3 (moo, "<x11.connect> %.*js => %p\n", MOO_OBJ_GET_SIZE(name), ((moo_oop_char_t)name)->slot, rcv->handle);
	*/
	c = xcb_connect (MOO_NULL, MOO_NULL);
	if (!c || xcb_connection_has_error(c))
	{
		MOO_DEBUG0 (moo, "<x11.connect> Cannot connect to X11 server\n");
		goto softfail;
	}

	x11->screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	x11->c = c;

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

softfail:
	MOO_STACK_SETRETTOERROR (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_disconnect (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	MOO_DEBUG1 (moo, "<x11.disconnect> %p\n", x11->c);

	if (x11->curevt) 
	{
		free (x11->curevt); 
		x11->curevt = MOO_NULL;
	}
	if (x11->c)
	{
		xcb_disconnect (x11->c);
		x11->c = MOO_NULL;
	}

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_get_fd (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	MOO_DEBUG1 (moo, "<x11.get_fd> %p\n", x11->c);

	if (x11->c)
	{
		int c;

		c = xcb_get_file_descriptor(x11->c);
		if (!MOO_IN_SMOOI_RANGE(c)) goto error;
		MOO_STACK_SETRET (moo, nargs, MOO_SMOOI_TO_OOP(c));
	}
	else
	{
	error:
		MOO_STACK_SETRETTOERROR (moo, nargs); /* TODO: be more specific about error code */
	}

	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_getevent (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	xcb_generic_event_t* evt;
	int e;

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	MOO_DEBUG1 (moo, "<x11.getevent> %p\n", x11->c);

	evt = xcb_poll_for_event(x11->c);
	if (x11->curevt) free (x11->curevt); 
	x11->curevt = evt;

	if (evt)
	{
		moo_oop_t llevt;
		/*uint8_t evttype;

		evttype = evt->response_type & 0x7F;*/
		x11->curevt = evt;

		llevt = moo_oowtoint (moo, (moo_oow_t)evt);
		if (!llevt) 
		{
			llevt = MOO_ERROR_TO_OOP(moo->errnum);
			free (evt);
			x11->curevt = MOO_NULL;
		}

		MOO_STACK_SETRET (moo, nargs, llevt);
	}
	else if ((e = xcb_connection_has_error(x11->c)))
	{
		/* TODO: to be specific about the error */
MOO_DEBUG1 (moo, "XCB CONNECTION ERROR %d\n", e);
		MOO_STACK_SETRETTOERROR (moo, nargs);
	}
	else
	{
		/* nil if there is no event */
		MOO_STACK_SETRET (moo, nargs, moo->_nil);
	}

	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfrc_t pf_gc_draw_line (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;
	xcb_point_t pt[2];
	moo_oop_t a[4];

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	a[0] = MOO_STACK_GETARG(moo, nargs, 0);
	a[1] = MOO_STACK_GETARG(moo, nargs, 1);
	a[2] = MOO_STACK_GETARG(moo, nargs, 2);
	a[3] = MOO_STACK_GETARG(moo, nargs, 3);
	if (!MOO_OOP_IS_SMOOI(a[0]) || !MOO_OOP_IS_SMOOI(a[1]) ||
	    !MOO_OOP_IS_SMOOI(a[2]) || !MOO_OOP_IS_SMOOI(a[0])) goto reterr;

	pt[0].x = MOO_OOP_TO_SMOOI(a[0]);
	pt[0].y = MOO_OOP_TO_SMOOI(a[1]);
	pt[1].x = MOO_OOP_TO_SMOOI(a[2]);
	pt[1].y = MOO_OOP_TO_SMOOI(a[3]);

	xcb_poly_line (gc->c, XCB_COORD_MODE_ORIGIN, gc->wid, gc->id, 2, pt);
	xcb_flush (gc->c);

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs); /* More specific error code*/
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gc_draw_rect (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;
	xcb_rectangle_t r;
	moo_oop_t a[4];

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	a[0] = MOO_STACK_GETARG(moo, nargs, 0);
	a[1] = MOO_STACK_GETARG(moo, nargs, 1);
	a[2] = MOO_STACK_GETARG(moo, nargs, 2);
	a[3] = MOO_STACK_GETARG(moo, nargs, 3);
	if (!MOO_OOP_IS_SMOOI(a[0]) || !MOO_OOP_IS_SMOOI(a[1]) ||
	    !MOO_OOP_IS_SMOOI(a[2]) || !MOO_OOP_IS_SMOOI(a[0])) goto reterr;

	r.x = MOO_OOP_TO_SMOOI(a[0]);
	r.y = MOO_OOP_TO_SMOOI(a[1]);
	r.width = MOO_OOP_TO_SMOOI(a[2]);
	r.height = MOO_OOP_TO_SMOOI(a[3]);

	xcb_poly_rectangle (gc->c, gc->wid, gc->id, 1, &r);
	xcb_flush (gc->c);

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs); /* More specific error code*/
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gc_set_foreground (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;
	moo_oop_t t;
	uint32_t value;

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	t = MOO_STACK_GETARG(moo, nargs, 0);
	if (MOO_OOP_IS_SMOOI(t)) goto reterr;
	value = MOO_OOP_TO_SMOOI(t);

	xcb_change_gc (gc->c, gc->id, XCB_GC_FOREGROUND, &value);
	xcb_flush (gc->c);

	MOO_STACK_SETRETTORCV (moo, nargs);
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs); /* More specific error code*/
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gc_get_id (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;
	moo_oop_t x;

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	x = moo_oowtoint (moo, gc->id);
	if (!x) return MOO_PF_HARD_FAILURE;

	MOO_STACK_SETRET (moo, nargs, x);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gc_kill (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

/* TODO: check on gc id */
	xcb_free_gc (gc->c, gc->id);
	xcb_flush (gc->c);

	MOO_STACK_SETRETTORCV (moo, nargs); 
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_gc_make (moo_t* moo, moo_ooi_t nargs)
{
	x11_gc_t* gc;
	x11_win_t* win;
	moo_oop_t id;
	xcb_gcontext_t gcid;

	gc = (x11_gc_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);
	win = (x11_win_t*)moo_getobjtrailer(moo, MOO_STACK_GETARG(moo, nargs, 0), MOO_NULL);

	gcid = xcb_generate_id (win->c);

	id = moo_oowtoint (moo, gcid);
	if (!id) return MOO_PF_HARD_FAILURE;

	xcb_create_gc (win->c, gcid, win->id, 0, 0);

/*TODO: use xcb_request_check() for error handling. xcb_create_gc_checked??? xxx_checked()... */
	xcb_flush (win->c);

	gc->id = gcid;
	gc->wid = win->id;
	gc->c = win->c;

	MOO_STACK_SETRET (moo, nargs, id); 
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs); /* TODO: be more specific about error */
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */
static moo_pfrc_t pf_win_get_dwatom (moo_t* moo, moo_ooi_t nargs)
{
	x11_win_t* win;
	moo_oop_t x;

	win = (x11_win_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	x = moo_oowtoint (moo, win->dwar->atom);
	if (!x) return MOO_PF_HARD_FAILURE;

	MOO_STACK_SETRET (moo, nargs, x);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_win_get_id (moo_t* moo, moo_ooi_t nargs)
{
	x11_win_t* win;
	moo_oop_t x;

	win = (x11_win_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	x = moo_oowtoint (moo, win->id);
	if (!x) return MOO_PF_HARD_FAILURE;

	MOO_STACK_SETRET (moo, nargs, x);
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_win_kill (moo_t* moo, moo_ooi_t nargs)
{
	x11_win_t* win;

	win = (x11_win_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

/* TODO: check on windows id */

	xcb_unmap_window (win->c, win->id); /* TODO: check error code */
	xcb_destroy_window (win->c, win->id);
	xcb_flush (win->c);

	MOO_STACK_SETRETTORCV (moo, nargs); 
	return MOO_PF_SUCCESS;
}

static moo_pfrc_t pf_win_make (moo_t* moo, moo_ooi_t nargs)
{
	x11_t* x11;
	x11_win_t* win;
	moo_oop_t id;
	xcb_window_t wid;

	uint32_t mask;
	uint32_t values[2];
	xcb_intern_atom_cookie_t cookie;
	xcb_intern_atom_reply_t* reply;

MOO_DEBUG0 (moo, "<x11.win._make_on:> %p\n");

	win = (x11_win_t*)moo_getobjtrailer(moo, MOO_STACK_GETRCV(moo, nargs), MOO_NULL);

	x11 = (x11_t*)moo_getobjtrailer(moo, MOO_STACK_GETARG(moo, nargs, 0), MOO_NULL);
	if (!x11 || !x11->c) goto reterr;

	wid = xcb_generate_id (x11->c);

	id = moo_oowtoint (moo, wid);
	if (!id) return MOO_PF_HARD_FAILURE;

	mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	values[0] = x11->screen->white_pixel;
	values[1] = XCB_EVENT_MASK_KEY_RELEASE |
	            XCB_EVENT_MASK_BUTTON_PRESS |
	            XCB_EVENT_MASK_BUTTON_RELEASE |
	            /*XCB_EVENT_MASK_BUTTON_MOTION |*/
	            XCB_EVENT_MASK_EXPOSURE |
	            /*XCB_EVENT_MASK_POINTER_MOTION |*/
	            XCB_EVENT_MASK_ENTER_WINDOW |
	            XCB_EVENT_MASK_LEAVE_WINDOW |
	            XCB_EVENT_MASK_VISIBILITY_CHANGE;
	xcb_create_window (
		x11->c, 
		XCB_COPY_FROM_PARENT,
		wid,
		x11->screen->root,
		0, 0, 300, 300, 10,
		XCB_WINDOW_CLASS_INPUT_OUTPUT,
		x11->screen->root_visual,
		mask, values);

	cookie = xcb_intern_atom(x11->c, 1, 12, "WM_PROTOCOLS");
	reply = xcb_intern_atom_reply(x11->c, cookie, 0);

	cookie = xcb_intern_atom(x11->c, 0, 16, "WM_DELETE_WINDOW");
	win->dwar = xcb_intern_atom_reply(x11->c, cookie, 0);

	xcb_change_property(x11->c, XCB_PROP_MODE_REPLACE, wid, reply->atom, 4, 32, 1, &win->dwar->atom);

	xcb_map_window (x11->c, wid);
	xcb_flush (x11->c);

	win->id = wid;
	win->c = x11->c;

	MOO_STACK_SETRET (moo, nargs, id); 
	return MOO_PF_SUCCESS;

reterr:
	MOO_STACK_SETRETTOERROR (moo, nargs); /* TODO: be more specific about error */
	return MOO_PF_SUCCESS;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_pfinfo[] =
{
	{ I, { '_','c','o','n','n','e','c','t','\0' },                     0, { pf_connect,    0, 0 } },
	{ I, { '_','d','i','s','c','o','n','n','e','c','t','\0' },         0, { pf_disconnect, 0, 0 } },
	{ I, { '_','g','e','t','_','e','v','e','n','t','\0'},              0, { pf_getevent,   0, 0 } },
	{ I, { '_','g','e','t','_','f','d','\0' },                         0, { pf_get_fd,     0, 0 } }
};

static int x11_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_t)) <= -1) return -1;
	/*return moo_genpfmethods(moo, mod, _class, x11_pfinfo, MOO_COUNTOF(x11_pfinfo));*/
	return 0;
}

static moo_pfbase_t* x11_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase (moo, x11_pfinfo, MOO_COUNTOF(x11_pfinfo), name, namelen);
}

static void x11_unload (moo_t* moo, moo_mod_t* mod)
{
	/* TODO: anything else? close all open dll handles? For that, pf_open must store the value it returns to mod->ctx or somewhere..*/
	moo_freemem (moo, mod->ctx);
}

static void x11_gc (moo_t* moo, moo_mod_t* mod)
{
	x11_modctx_t* ctx = mod->ctx;

	MOO_ASSERT (moo, ctx != MOO_NULL);
	ctx->x11_class = (moo_oop_class_t)moo_moveoop (moo, (moo_oop_t)ctx->x11_class);
}

int moo_mod_x11 (moo_t* moo, moo_mod_t* mod)
{
	if (mod->hints & MOO_MOD_LOAD_FOR_IMPORT)
	{
		mod->gc = MOO_NULL;
		mod->ctx = MOO_NULL;
	}
	else
	{
		x11_modctx_t* ctx;

		static moo_ooch_t name_x11[] = { 'X','1','1','\0' };

		ctx = moo_callocmem (moo, MOO_SIZEOF(*ctx));
		if (!ctx) return -1;

		ctx->x11_class = (moo_oop_class_t)moo_findclass (moo, moo->sysdic, name_x11);
		if (!ctx->x11_class)
		{
			/* Not a single X11.XXX has been defined. */
			MOO_DEBUG0 (moo, "X11 class not found\n");
			moo_freemem (moo, ctx);
			return -1;
		}

		mod->gc = x11_gc;
		mod->ctx = ctx;
	}

	mod->import = x11_import;
	mod->query = x11_query;
	mod->unload = x11_unload; 

	return 0;
}

/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_gc_pfinfo[] =
{
	{ I, { '_','d','r','a','w','L','i','n','e' },                              0, { pf_gc_draw_line,      4, 4 } },
	{ I, { '_','d','r','a','w','R','e','c','t' },                              0, { pf_gc_draw_rect,      4, 4 } },
	{ I, { '_','f','o','r','e','g','r','o','u','n','d',':','\0' },             0, { pf_gc_set_foreground, 1, 1 } },
	{ I, { '_','g','e','t','_','i','d','\0' },                                 0, { pf_gc_get_id,         0, 0 } },
	{ I, { '_','k','i','l','l','\0' },                                         0, { pf_gc_kill,           0, 0 } },
	{ I, { '_','m','a','k','e','_','o','n',':','\0' },                         0, { pf_gc_make,           1, 1 } }
	
};

static int x11_gc_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_gc_t)) <= -1) return -1;
	return 0;
}

static moo_pfbase_t* x11_gc_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase(moo, x11_gc_pfinfo, MOO_COUNTOF(x11_gc_pfinfo), name, namelen);
}

static void x11_gc_unload (moo_t* moo, moo_mod_t* mod)
{
	/* anything? */
}

int moo_mod_x11_gc (moo_t* moo, moo_mod_t* mod)
{
	mod->import = x11_gc_import;
	mod->query = x11_gc_query;
	mod->unload = x11_gc_unload; 
	mod->gc = MOO_NULL;
	mod->ctx = MOO_NULL;

	return 0;
}


/* ------------------------------------------------------------------------ */

static moo_pfinfo_t x11_win_pfinfo[] =
{
	{ I, { '_','g','e','t','_','d','w','a','t','o','m','\0'}, 0, { pf_win_get_dwatom,   0, 0 } },
	{ I, { '_','g','e','t','_','i','d','\0' },                0, { pf_win_get_id,       0, 0 } },

	{ I, { '_','k','i','l','l','\0' },                        0, { pf_win_kill,         0, 0 } },
	{ I, { '_','m','a','k','e','\0' },                        0, { pf_win_make,         1, 1 } },
	{ I, { '_','m','a','k','e','_','o','n',':','\0' },        0, { pf_win_make,         1, 1 } }
};

static int x11_win_import (moo_t* moo, moo_mod_t* mod, moo_oop_class_t _class)
{
	if (moo_setclasstrsize(moo, _class, MOO_SIZEOF(x11_win_t)) <= -1) return -1;
	/*return moo_genpfmethods(moo, mod, _class, x11_win_pfinfo, MOO_COUNTOF(x11_win_pfinfo));*/
	return 0;
}

static moo_pfbase_t* x11_win_query (moo_t* moo, moo_mod_t* mod, const moo_ooch_t* name, moo_oow_t namelen)
{
	return moo_findpfbase(moo, x11_win_pfinfo, MOO_COUNTOF(x11_win_pfinfo), name, namelen);
}

static void x11_win_unload (moo_t* moo, moo_mod_t* mod)
{
	/* anything? */
}

int moo_mod_x11_win (moo_t* moo, moo_mod_t* mod)
{
	mod->import = x11_win_import;
	mod->query = x11_win_query;
	mod->unload = x11_win_unload; 
	mod->gc = MOO_NULL;
	mod->ctx = MOO_NULL;

	return 0;
}