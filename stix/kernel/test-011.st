
#include 'Stix.st'.

#################################################################
## MAIN
#################################################################

## TODO: use #define to define a class or use #class to define a class.
##       use #extend to extend a class
##       using #class for both feels confusing.

#extend Apex
{

}

#extend SmallInteger
{
	#method getTrue: anInteger
	{
		^anInteger + 9999.
	}

	#method inc
	{
		^self + 1.
	}
}

#class TestObject(Object)
{
	#dcl(#class) Q R.
	#dcl(#classinst) a1 a2.
}


#class MyObject(TestObject)
{

	#method(#class) main2
	{
		| k |
		k := ['this is test-011' dump. Exception signal. 8888 dump. ] 
			on: Exception do: [ :ex | 'Exception occurred' dump.  ex dump. 'Getting back to' dump. 80. ex pass ].

		k dump.
		'END OF test-011' dump.
	}

	#method(#class) main
	{
		'>>>>> BEGINNING OF MAIN' dump.
		
		[ self main2 ] on: Exception do: [ :ex | 'EXCEPTION CAUGHT IN MAIN....' dump ].

		'>>>>> END OF MAIN' dump.
	}
}