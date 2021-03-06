class _FFI(Object) from 'ffi'
{
	method(#primitive) open(name).
	method(#primitive) close.
	method(#primitive) getsym(name).
	 
	/* TODO: make call variadic? method(#primitive,#variadic) call (func, sig). */
	method(#primitive) call(func, sig, args).
}

class FFIException(Exception)
{
}

class FFI(Object)
{
	var name, ffi, funcs.

	/*
	pooldic Type
	{
		UINT8  := 1,
		UINT16 := 2,
		UINT32 := 3,
		UINT64 := 4
	}
	*/
	/*
	const UINT8 := 1,
	const UINT16 := 2
	*/

	method(#class) new: aString
	{
		^self new open: aString.
	}

	method initialize 
	{
		self.funcs := Dictionary new.
		self.ffi := _FFI new.
		//self addToBeFinalized.
	}

	/*method finalize
	{
		self close
	}*/

	method open: name
	{
		| x |
		self.funcs removeAllKeys.
		self.name := name.

		x := self.ffi open(name).
		//(x isError) ifTrue: [^x].
		if (x isError) { FFIException signal: ('Unable to open %O' strfmt(name)) }.

		^self.
	}

	method close
	{
		self.ffi close.
	}

	method call: name signature: sig arguments: args
	{
		| f rc |

//self.Type.UINT8 dump.
//#[ self.Type.UINT8,   FFI.Type.UINT16 ] dump.

		/* f := self.funcs at: name ifAbsent: [ 
			f := self.ffi getsym(name).
			if (f isError) { FFIException signal: ('Unable to find %O' strfmt(name)) }.
			self.funcs at: name put: f.
			f. // need this as at:put: returns an association
		]. */

		f := self.funcs at: name ifAbsent: [ nil ].
		if (f isNil)
		{
			f := self.ffi getsym(name).
			if (f isError) { FFIException signal: ('Unable to find %O' strfmt(name)) }.
			self.funcs at: name put: f.
		}.

		rc := self.ffi call(f, sig, args).
		if (rc isError)	{ FFIException signal: ('Unable to call %O - %O' strfmt(name, rc)) }.

		^rc.
	}
}
