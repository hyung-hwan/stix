/*
 * $Id$
 *
    Copyright (c) 2014-2015 Chung, Hyung-Hwan. All rights reserved.

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

#include "stix-prv.h"

static void cleanup_symbols_for_gc (stix_t* stix, stix_oop_t _nil)
{
	stix_oop_char_t symbol;
	stix_oow_t tally, index, i, x, y, z;
	stix_oow_t bucket_size;

#if defined(STIX_SUPPORT_GC_DURING_IGNITION)
	if (!stix->symtab) return; /* symbol table has not been created */
#endif

	bucket_size = STIX_OBJ_GET_SIZE(stix->symtab->bucket);

	tally = STIX_OOP_TO_SMINT(stix->symtab->tally);
	if (tally <= 0) return;

	for (index = 0; index < bucket_size; )
	{
		if (STIX_OBJ_GET_FLAGS_MOVED(stix->symtab->bucket->slot[index]))
		{
			index++;
			continue;
		}

		STIX_ASSERT (stix->symtab->bucket->slot[index] != _nil);

/*
{
sym = (stix_oop_char_t)buc->slot[index];
wprintf (L">> DISPOSING %d [%S] from the symbol table\n", (int)index, sym->slot);
}
*/

		for (i = 0, x = index, y = index; i < bucket_size; i++)
		{
			y = (y + 1) % bucket_size;

			/* done if the slot at the current hash index is _nil */
			if (stix->symtab->bucket->slot[y] == _nil) break;

			/* get the natural hash index for the data in the slot 
			 * at the current hash index */
			symbol = (stix_oop_char_t)stix->symtab->bucket->slot[y];

			STIX_ASSERT (STIX_CLASSOF(stix,symbol) == stix->_symbol);

			z = stix_hashchars(symbol->slot, STIX_OBJ_GET_SIZE(symbol)) % bucket_size;

			/* move an element if necessary */
			if ((y > x && (z <= x || z > y)) ||
			    (y < x && (z <= x && z > y)))
			{
				stix->symtab->bucket->slot[x] = stix->symtab->bucket->slot[y];
				x = y;
			}
		}

		stix->symtab->bucket->slot[x] = _nil;
		tally--;
	}

	stix->symtab->tally = STIX_OOP_FROM_SMINT(tally);
}

static stix_oop_t move_one (stix_t* stix, stix_oop_t oop)
{
#if defined(STIX_SUPPORT_GC_DURING_IGNITION)
	if (!oop) return oop;
#endif

	STIX_ASSERT (STIX_OOP_IS_POINTER(oop));

	if (STIX_OBJ_GET_FLAGS_MOVED(oop))
	{
		/* this object has migrated to the new heap. 
		 * the class field has been updated to the new object
		 * in the 'else' block below. i can simply return it
		 * without further migration. */
		return STIX_OBJ_GET_CLASS(oop);
	}
	else
	{
		stix_oow_t nbytes, nbytes_aligned;
		stix_oop_t tmp;

		/* calculate the payload size in bytes */
		nbytes = (oop->_size + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop);
		nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));

		/* allocate space in the new heap */
		tmp = stix_allocheapmem (stix, stix->newheap, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);

		/* allocation here must not fail because
		 * i'm allocating the new space in a new heap for 
		 * moving an existing object in the current heap. 
		 *
		 * assuming the new heap is as large as the old heap,
		 * and garbage collection doesn't allocate more objects
		 * than in the old heap, it must not fail. */
		STIX_ASSERT (tmp != STIX_NULL); 

		/* copy the payload to the new object */
		STIX_MEMCPY (tmp, oop, STIX_SIZEOF(stix_obj_t) + nbytes_aligned);

		/* mark the old object that it has migrated to the new heap */
		STIX_OBJ_SET_FLAGS_MOVED(oop, 1);

		/* let the class field of the old object point to the new 
		 * object allocated in the new heap. it is returned in 
		 * the 'if' block at the top of this function. */
		STIX_OBJ_SET_CLASS (oop, tmp);

		/* return the new object */
		return tmp;
	}
}

static stix_uint8_t* scan_new_heap (stix_t* stix, stix_uint8_t* ptr)
{
	while (ptr < stix->newheap->ptr)
	{
		stix_oow_t i;
		stix_oow_t nbytes, nbytes_aligned;
		stix_oop_t oop;

		oop = (stix_oop_t)ptr;
		nbytes = (STIX_OBJ_GET_SIZE(oop) + STIX_OBJ_GET_FLAGS_EXTRA(oop)) * STIX_OBJ_GET_FLAGS_UNIT(oop);
		nbytes_aligned = STIX_ALIGN (nbytes, STIX_SIZEOF(stix_oop_t));

		STIX_OBJ_SET_CLASS (oop, move_one(stix, STIX_OBJ_GET_CLASS(oop)));
		if (STIX_OBJ_GET_FLAGS_TYPE(oop) == STIX_OBJ_TYPE_OOP)
		{
			stix_obj_oop_t* xtmp;

			xtmp = (stix_oop_oop_t)oop;
			for (i = 0; i < oop->_size; i++)
			{
				if (STIX_OOP_IS_POINTER(xtmp->slot[i]))
					xtmp->slot[i] = move_one (stix, xtmp->slot[i]);
			}
		}

/*wprintf (L"ptr in gc => %p size => %d, aligned size => %d\n", ptr, (int)nbytes, (int)nbytes_aligned);*/
		ptr = ptr + STIX_SIZEOF(stix_obj_t) + nbytes_aligned;
	}

	/* return the pointer to the beginning of the free space in the heap */
	return ptr; 
}

void stix_gc (stix_t* stix)
{
	/* 
	 * move a referenced object to the new heap.
	 * inspect the fields of the moved object in the new heap.
	 * move objects pointed to by the fields to the new heap.
	 * finally perform some tricky symbol table clean-up.
	 */
	stix_uint8_t* ptr;
	stix_heap_t* tmp;
	stix_oop_t old_nil;
	stix_oow_t i;

	/* TODO: allocate common objects like _nil and the root dictionary 
	 *       in the permanant heap.  minimize moving around */
	old_nil = stix->_nil;

	/* move _nil and the root object table */
	stix->_nil               = move_one (stix, stix->_nil);
	stix->_true              = move_one (stix, stix->_true);
	stix->_false             = move_one (stix, stix->_false);

/*printf ("BEFORE GC = %p %p %p %p %p %p %p %p %p %p\n", stix->_array, stix->_class, stix->_nil_object, stix->_object, stix->_symbol, stix->_symbol_set, stix->_system_dictionary, stix->_association, stix->_character, stix->_small_integer);*/
	stix->_stix              = move_one (stix, stix->_stix);
	stix->_class             = move_one (stix, stix->_class);
	stix->_nil_object        = move_one (stix, stix->_nil_object);
	stix->_object            = move_one (stix, stix->_object);
	stix->_array             = move_one (stix, stix->_array);
	stix->_symbol            = move_one (stix, stix->_symbol);
	stix->_symbol_set        = move_one (stix, stix->_symbol_set);
	stix->_system_dictionary = move_one (stix, stix->_system_dictionary);
	stix->_association       = move_one (stix, stix->_association);
	stix->_character         = move_one (stix, stix->_character);
	stix->_small_integer     = move_one (stix, stix->_small_integer);

/*printf ("AFTER GC = %p %p %p %p %p %p %p %p %p %p\n", stix->_array, stix->_class, stix->_nil_object, stix->_object, stix->_symbol, stix->_symbol_set, stix->_system_dictionary, stix->_association, stix->_character, stix->_small_integer);*/
	stix->sysdic = (stix_oop_set_t) move_one (stix, (stix_oop_t)stix->sysdic);

	for (i = 0; i < stix->tmp_count; i++)
	{
		*stix->tmp_stack[i] = move_one (stix, *stix->tmp_stack[i]);
	}

	/* scan the new heap to move referenced objects */
	ptr = (stix_uint8_t*) STIX_ALIGN ((stix_uintptr_t)stix->newheap->base, STIX_SIZEOF(stix_oop_t));
	ptr = scan_new_heap (stix, ptr);

	/* traverse the symbol table for unreferenced symbols.
	 * if the symbol has not moved to the new heap, the symbol
	 * is not referenced by any other objects than the symbol 
	 * table itself */
	cleanup_symbols_for_gc (stix, old_nil);

	/* move the symbol table itself */
	stix->symtab = (stix_oop_set_t)move_one (stix, (stix_oop_t)stix->symtab);

	/* scan the new heap again from the end position of
	 * the previous scan to move referenced objects by 
	 * the symbol table. */
	ptr = scan_new_heap (stix, ptr);

	/* swap the current heap and old heap */
	tmp = stix->curheap;
	stix->curheap = stix->newheap;
	stix->newheap = tmp;

/*
{
stix_oow_t index;
stix_oop_oop_t buc;
printf ("=== SURVIVING SYMBOLS ===\n");
buc = (stix_oop_oop_t) stix->symtab->slot[STIX_SYMTAB_BUCKET];
for (index = 0; index < buc->size; index++)
{
	if ((stix_oop_t)buc->slot[index] != stix->_nil) 
	{
		const stix_oop_char_t* p = ((stix_oop_char_t)buc->slot[index])->slot;
		printf ("SYM [");
		while (*p) printf ("%c", *p++);
		printf ("]\n");
	}
}
printf ("===========================\n");
}
*/
}


void stix_pushtmp (stix_t* stix, stix_oop_t* oop_ptr)
{
	/* if you have too many temporaries pushed, something must be wrong.
	 * change your code not to exceede the stack limit */
	STIX_ASSERT (stix->tmp_count < STIX_COUNTOF(stix->tmp_stack));
	stix->tmp_stack[stix->tmp_count++] = oop_ptr;
}

void stix_poptmp (stix_t* stix)
{
	STIX_ASSERT (stix->tmp_count > 0);
	stix->tmp_count--;
}

void stix_poptmps (stix_t* stix, stix_oow_t count)
{
	STIX_ASSERT (stix->tmp_count >= count);
	stix->tmp_count -= count;
}
