
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
	#method(#class) main111
	{
		| s3 |
		s3 := Semaphore new.
		Processor signal: s3 after: 1 and: 50.
		s3 wait.
		'END OF MAIN' dump.
	}

	#method(#class) main
	{
		|t1 t2 s1 s2 s3|

		'START OF MAIN' dump.

		s1 := Semaphore new.
		s2 := Semaphore new.
		s3 := Semaphore new.

		t1 := [ 
			10 timesRepeat: ['BLOCK #1' dump. Processor activeProcess sleep: 1.].
			s1 signal
		] newProcess.
		t2 := [ 5 timesRepeat: ['BLOCK #2' dump]. s2 signal. ] newProcess.

		t1 resume.
		t2 resume.

		Processor signal: s3 after: 10.

		'STARTED t1 and t2' dump.
		s1 wait.
		s2 wait.

		##Processor unsignal: s3.
		s3 wait.

		10 timesRepeat: ['WAITED t1 and t2' dump].

		
		'END OF MAIN' dump.
	}

	#method(#class) test_semaphore_heap
	{
		| sempq a |
		sempq := SemaphoreHeap new.

		'--------------------------' dump.

		1 to: 200 by: 1 do: [ :i |  
			| sem |
			sem := Semaphore new. 
			sem fireTime: (200 - i).
			sempq insert: sem
		].

		'--------------------------' dump.
		sempq deleteAt: 40.
		sempq deleteAt: 50.

		[sempq size > 0] whileTrue: [
			| sem |
			sem := sempq popTop.
			sem fireTime dump.
		]
	}
}