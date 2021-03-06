#include 'Moo.moo'.

class MyObject(Object)
{
	method(#class) main
	{
		| errmsgs synerrmsgs f |

		errmsgs := #( 
			"no error"
			"generic error"

			"not implemented"
			"subsystem error"
			"internal error that should never have happened"
			"insufficient system memory"
			"insufficient object memory"
			"invalid class/type"

			"invalid parameter or argument"
			"data not found"
			"existing/duplicate data"
			"busy"
			"access denied"
			"operation not permitted"
			"not directory"
			"interrupted"
			"pipe error"
			"resource temporarily unavailable"
			"bad system handle"

			"*** undefined error ***"
			"message receiver error"
			"message sending error"
			"wrong number of arguments"
			"range error"
			"byte-code full"
			"dictionary full"
			"processor full"
			"too many semaphores"
			"*** undefined error ***"
			"divide by zero"
			"I/O error"
			"encoding conversion error"
			"insufficient data for encoding conversion"
			"buffer full"
		).

		synerrmsgs := #(
			"no error"
			"illegal character"
			"comment not closed"
			"string not closed"
			"no character after $"
			"no valid character after #"
			"no valid character after #\\"
			"wrong character literal"
			"colon expected"
			"string expected"
			"invalid radix" 
			"invalid integer literal"
			"invalid fixed-point decimal scale"
			"invalid fixed-point decimal literal"
			"byte too small or too large"
			"wrong error literal"
			"wrong smptr literal"
			"{ expected"
			"} expected"
			"( expected"
			") expected"
			"] expected"
			". expected"
			", expected"
			"| expected"
			"> expected"
			":= expected"
			"identifier expected"
			"integer expected"
			"primitive: expected"
			"wrong directive"
			"wrong name"
			"duplicate name"
			"undefined name"
			"contradictory class definition"
			"class not conforming to interface"
			"invalid non-pointer instance size"
			"prohibited inheritance"
			"variable declaration not allowed"
			"modifier expected"
			"wrong modifier"
			"disallowed modifier"
			"duplicate modifier"
			"method name expected"
			"duplicate method name"
			"invalid variadic method definition"
			"variable name expected"
			"duplicate argument name"
			"duplicate temporary variable name"
			"duplicate variable name"
			"duplicate block argument name"
			"undeclared variable"
			"unusable variable in compiled code"
			"inaccessible variable"
			"ambiguous variable"
			"too many instance/class variables"
			"inaccessible super"
			"wrong expression primary"
			"too many temporaries"
			"too many arguments"
			"too many block temporaries"
			"too many block arguments"
			"array expression too large"
			"instruction data too large"
			"wrong primitive function number"
			"wrong primitive function identifier"
			"wrong primitive function argument definition"
			"wrong primitive value identifier"
			"primitive value load from module not allowed"
			"failed to import module"
			"#include error"
			"wrong pragma name"
			"wrong namespace name"
			"wrong pooldic import name"
			"duplicate pooldic import name"
			"literal expected"
			"break or continue not within a loop"
			"break or continue within a block"
			"while expected"
			"invalid goto target"
			"label at end"
		).

		f := Stdio open: "generr.out" for: "w".
		[ f isError ] ifTrue: [ System logNl: "Cannot open generr.out". thisProcess terminate. ].

		self emitMessages: errmsgs named: "errstr" on: f.
		self emitMessages: synerrmsgs named: "synerrstr" on: f.

		f close.
	}
	
	method(#class) emitMessages: errmsgs named: name on: f
	{
		| c prefix |
		prefix := name & '_'.
		
		c := errmsgs size - 1.
		0 to: c do: [:i |
			self printString: (errmsgs at: i) prefix: prefix index: i on: f.
		].

		
		f puts("static moo_ooch_t* ", name, "[] =\n{\n").
		0 to: c do: [:i |
			((i rem: 8) = 0) ifTrue: [ f putc(C"\t") ].
			f puts(prefix, (i asString)).
			(i = c) ifFalse: [f puts(",") ].
			(((i + 1) rem: 8) = 0) ifTrue: [ f putc(C"\n") ] ifFalse: [ f putc(C' ') ].
		].
		(((c + 1) rem: 8) = 0) ifFalse: [ f putc(C"\n") ].
		f puts("};\n").
	}

	method(#class) printString: s prefix: prefix index: index on: f
	{
		| c |
		c := s size - 1.

		f puts("static moo_ooch_t ", prefix, index asString, '[] = {').

		0 to: c do: [:i |
			| ch |
			ch := s at: i.
			if ((ch == $\) or (ch == $")) 
			{
				f putc($', $\, (s at: i), $').
			}
			else
			{
				f putc($', (s at: i), $').
			}.
			(i = c) ifFalse: [f putc($,) ].
		].

		f puts(",\'\\0\'};\n").
	}
}
