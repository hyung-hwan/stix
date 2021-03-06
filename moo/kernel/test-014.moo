
#include 'Moo.moo'.

////////////////////////////////////////////////////////////////#
// MAIN
////////////////////////////////////////////////////////////////#

extend Apex
{

}

extend SmallInteger
{
	method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	method inc
	{
		^self + 1.
	}
}

class TestObject(Object)
{
	var(#class) Q, R.
	var(#classinst) a1, a2.

	method test999
	{
		^self.Q
	}
}

class B.TestObject(Object)
{
	var(#class) Q, R.
	var(#classinst) a1, a2.

	method test000
	{
		^self.Q
	}
}

pooldic ABC 
{
	KKK := 20
}

pooldic SRX.ABC
{
	JJJ := 1000
}
class MyConsole(Console)
{
	method box: origin corner: corner
	{
		| tmp |
		self setCursor: origin.
		self write: '+'.
		(corner x - origin x - 1) timesRepeat: [self write: '-'].
		self write: '+'.

		tmp := Point new.

		(origin y + 1) to: (corner y - 1) by: 1 do: [ :i |
			tmp x: origin x y: i.
			self setCursor: tmp.
			self write: '|'.

			tmp x: corner x.
			self setCursor: tmp.
			self write: '|'.
		].

		tmp x: origin x y: corner y.
		self setCursor: tmp.
		self write: '+'.
		(corner x - origin x - 1) timesRepeat: [self write: '-'].
		self write: '+'.
	}
}

class MyObject(TestObject)
{
	//import(#pooldic) ABC, SRX.ABC.
	import ABC, SRX.ABC.
	
	method(#class) main
	{
		| v1 v2 |

		v2 := 'have fun'.

		v2 at: 0 put: $H.

		System logNl: ('START OF MAIN - ' & v2).

		v1 := MyConsole output.
		v1 clear.
		v1 box: 0@0 corner: 80@20.
		v1 write: "hello, 월드 이거 좋지 않니\n".
		v1 write: "하하하하하하하하 좋아좋아 可愛くってしょうがない(^o^) ほのかちゃん、しおりちゃん元気そうだね！ 久しぶりに見た。しおりちゃんどうしたのかな？좋아 하라하하\n".
		v1 close.

		self main2.

		System logNl: (9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999
		             * 8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888) asString.
		System logNl: (9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999 
		             - 8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888) asString.
		System logNl: (8888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888 
		             - 9999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999999) asString.
		System logNl:(820186817651640487320479808367534510238779540102526006236474836166734016865205999870833760242352512045225158774173869894826877890589130978987229877889333678492731896878236182891224254464936050871086340438798130266913122427332418216677813151305680453358955006355665628938266331979307689540884269372365762883678113227136498054422414501840232090872158915536978847443767922315217311444711397048331496139248250188991402851129033493732164230227458717486395514436574417275149404197774547389507462779807727615
		             * 765507696474864454832447821143032209556194237429024272487376513755618415740858933212778176226195677908876814855895611901838419364549855580388081219363378099926549770419687104031809304167273647479680584409544921582452247598843590335565958941218635089801691339265287920342381909847353843571491984747541378691432905678660731517460920201717549951480681654501180257614183394160869490681730637245109396396631700176391975994387097927483353281545628136320635813474136122790139443917922910896873631927820545774) asString.
		System logNl:(-820186817651640487320479808367534510238779540102526006236474836166734016865205999870833760242352512045225158774173869894826877890589130978987229877889333678492731896878236182891224254464936050871086340438798130266913122427332418216677813151305680453358955006355665628938266331979307689540884269372365762883678113227136498054422414501840232090872158915536978847443767922315217311444711397048331496139248250188991402851129033493732164230227458717486395514436574417275149404197774547389507462779807727615
		             * 765507696474864454832447821143032209556194237429024272487376513755618415740858933212778176226195677908876814855895611901838419364549855580388081219363378099926549770419687104031809304167273647479680584409544921582452247598843590335565958941218635089801691339265287920342381909847353843571491984747541378691432905678660731517460920201717549951480681654501180257614183394160869490681730637245109396396631700176391975994387097927483353281545628136320635813474136122790139443917922910896873631927820545774) asString.
		System logNl: "\0\0\0END OF MAIN\0AB\0\0\0C\0\0\0".


		//v1 := Stdio2 open: '/tmp/1.txt' for: 'w+'.
		v1 := Stdio2 new open('/tmp/1.txt', 'w+').
		(v1 isError) 
			ifTrue: [
				System logNl: ('Error in opening a file....' & v1 asString). 
			]
			ifFalse: [
				// v1 puts: 'hello'.
				v1 puts ('hello', 'world', 'good', C'\n', C'\t', 'under my umbrella 123.', C'\n').
				v1 close.

				/*v1 format(10, 20) isNil ifFalse: [
				 	'Beautiful life' dump.
				].*/
			].
		nil isNil ifTrue: [ 'NIL NIL NIL' dump. ].
		(Apex new) notNil ifTrue: [ 'APEX NIL NIL NIL' dump. ].

		self varg_test (10, 20, 30, 40, 50) dump.
		self varg_test2 (10, 20, 30, 40, 50) dump.
		self varg_test3 (10, 20, 30, 40, 50) dump.
		self varg_test3 (10, 20, 30, 40, 50, 60, 70, 80, 90, 100) dump.
		thisContext vargCount dump.
		thisContext vargCount dump.
		
		((2305843009213693951 bitAt: 61) = 0) ifFalse: [ 
			System logNl: 'Test 1 failed'.
			thisProcess terminate
		].
		
		((-2305843009213693951 bitAt: 62) = 1) ifFalse: [
			System logNl: 'Test 2 failed'.
			thisProcess terminate
		].

		((2r1000000000000000000000000000100000000000000000000000000000000000000000000000 bitAt: 120) = 0) ifFalse: [
			System logNl: 'Test 3 failed'.
			thisProcess terminate
		].

		((-2r1000000000000000000000000000100000000000000000000000000000000000000000000000 bitAt: 16rFFFFFFFFFFFFFFFF0) = 1) ifFalse: [
			System logNl: 'Test 4 failed'.
			thisProcess terminate
		].

		0 priorTo: 200 do: [:i |
			| k |
			k := 1 bitShift: i.
			// (k printStringRadix: 2) dump.
			((k bitAt: i) = 1) ifFalse: [
				System logNl: 'Test 5 failed'.
				thisProcess terminate.
			].
			((k bitAt: i - 1) = 0) ifFalse: [
				System logNl: 'Test 6 failed'.
				thisProcess terminate.
			].
		].
		
		2r100000000_10001111_01010000 dump.
		16rFFFFFFFF_12345678 dump.
		
		(v1 := self t001()) isError ifTrue: [('t001 Error 111....' & v1 asInteger asString) dump].
		(v1 := self t001(10)) isError ifTrue: [('t001 Error 222....' & v1 asInteger asString) dump].
		(v1 := self t001(20)) isError ifTrue: [('t001 Error 333....' & v1 asInteger asString) dump].
		#\e9999 dump.
		#\e9999 asInteger dump.

		v2 := (16rFFFFFFFF_FFFFFFFF_FFFFFFFF_FFFFFFFF_FFFFFFFF_FFFFFFFF) basicAt: 1 put: 1; yourself.
		v2 dump.
	}

	method(#class,#variadic) varg_test()
	{
		0 to: (thisContext vargCount - 1) do: [:k |
			(thisContext vargAt: k) dump.
		].
		^999
	}
	method(#class,#variadic) varg_test2(a,b,c)
	{
		'varg_test2 start .....' dump.
		a dump.
		b dump.
		c dump.
		'varg_test2 varg .....' dump.
		0 priorTo: (thisContext vargCount) do: [:k |
			(thisContext vargAt: k) dump.
		].
		'varg_test2 end .....' dump.
		^a
	}
	method(#class,#liberal) varg_test3(a,b,c,d,e,f)
	{
		'varg_test3 start .....' dump.
		0 priorTo: (thisContext vargCount) do: [:k |
			(thisContext vargAt: k) dump.
		].
		'varg_test3 end .....' dump.
		// ^b * 100
		^f
	}
	
	method(#class,#liberal) t001(a)
	{
		a isNil ifTrue: [^#\e10].
		(a = 20) ifTrue: [^#\e0 ].
		(a = 10) ifTrue: [^123 asError].
		^a.

		#! a := error(10).
		#! [ a = error(10) ] ifTrue: [....].
		
	#!	self t001 (error:10).
	#!	self t001: (error:10)
	}
}

extend MyObject
{
	method(#class) main2
	{
		System logNl: KKK.
		System logNl: SRX.ABC.JJJ.
		System logNl: JJJ.
		System logNl: -200 asString.
	}
}
