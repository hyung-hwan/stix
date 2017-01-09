#include 'Apex.st'.
#include 'Context.st'.
#include 'Except.st'.
#include 'Class.st'.
#include 'Boolean.st'.
#include 'Magnitu.st'.
#include 'Collect.st'.
#include 'Process.st'.


class FFI(Object)
{
	dcl name handle funcs.

	method(#class) new: aString
	{
		^self new open: aString.
	}

	method open: aString
	{
		self.funcs := Dictionary new.
		self.name := aString.

		self.handle := self privateOpen: self.name.

		"[ self.handle := self privateOpen: self.name ] 
			on: Exception do: [
			]
			on: XException do: [
			]."

		^self.
	}

	method close
	{
		self privateClose: self.handle.
		self.handle := nil.
	}

	method call: aFunctionName withSig: aString withArgs: anArray
	{
		| f |

	##	f := self.funcs at: aFunctionName.
	##	f isNil ifTrue: [
	##		f := self privateGetSymbol: aFunctionName in: self.handle.
	##		f isNil ifTrue: [ self error: 'No such function' ].
	##		self.funcs at: aFunctionName put: f.
	##	].
f := self privateGetSymbol: aFunctionName in: self.handle.
f isNil ifTrue: [ self error: 'No such function' ].

		^self privateCall: f withSig: aString withArgs: anArray
	}

	method privateOpen: aString
	{
		<primitive: #_ffi_open>
		^nil. ## TODO: Error signal: 'can not open'
	}

	method privateClose: aHandle
	{
		<primitive: #_ffi_close>
	}

	method privateCall: aSymbol withSig: aString withArgs: anArray
	{
		<primitive: #_ffi_call>
	}

	method privateGetSymbol: aString in: aHandle
	{
		<primitive: #_ffi_getsym>
		^nil.
	}
}



#########################################################################################

#include 'Stdio.st'.
#include 'Console.st'.