/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "DiagonalMsg.h"
#include "../scheduling/Tick.h"
#include "../scheduling/TickPtr.h"
#include "../scheduling/Clock.h"
#include "Arith.h"

void testArith()
{
	Id a1id = Id::nextId();
	vector< unsigned int > dims( 1, 10 );
	Element* a1 = new Element( a1id, Arith::initCinfo(), "a1", dims, 1 );

	Eref a1_0( a1, 0 );
	Eref a1_1( a1, 1 );

	Arith* data1_0 = reinterpret_cast< Arith* >( a1->dataHandler()->data1( 0 ) );
//	Arith* data1_1 = reinterpret_cast< Arith* >( a1->data1( 1 ) );

	data1_0->arg1( 1 );
	data1_0->arg2( 0 );

	ProcInfo p;
	data1_0->process( &p, a1_0 );

	assert( data1_0->getOutput() == 1 );

	data1_0->arg1( 1 );
	data1_0->arg2( 2 );

	data1_0->process( &p, a1_0 );

	assert( data1_0->getOutput() == 3 );

	a1id.destroy();

	cout << "." << flush;
}

/** 
 * This test uses the Diagonal Msg and summing in the Arith element to
 * generate a Fibonacci series.
 */
void testFibonacci()
{
	unsigned int numFib = 20;
	vector< unsigned int > dims( 1, numFib );

	Id a1id = Id::nextId();
	Element* a1 = new Element( a1id, Arith::initCinfo(), "a1", dims );

	Arith* data = reinterpret_cast< Arith* >( a1->dataHandler()->data1( 0 ) );
	if ( data ) {
		data->arg1( 0 );
		data->arg2( 1 );
	}

	bool ret = DiagonalMsg::add( a1, "output", a1, "arg1", 1 );
	assert( ret );
	ret = DiagonalMsg::add( a1, "output", a1, "arg2", 2 );
	assert( ret );


	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	shell->setclock( 0, 1.0, 0 );
	Eref ticker = Id( 2 ).eref();
	ret = OneToAllMsg::add( ticker, "process0", a1, "process" );
	assert( ret );

	/*
	Eref clocker = Id(1).eref();
	Clock* clock = reinterpret_cast< Clock* >( clocker.data() );
	Qinfo dummyQ; // Not actually used in the Clock::start function
	clock->setNumThreads( 0 );
	clock->setBarrier( 0 );
	clock->start( clocker, &dummyQ, numFib );
	*/
	// clock->tStart( clocker, ti )
	if ( Shell::myNode() == 0 ) {
		shell->doStart( numFib );
	} else {
		Element* shelle = Id().eref().element();
		Eref clocker = Id( 1 ).eref();
		double currentTime = 0.0;
		while ( currentTime < 1 ) {
			shell->passThroughMsgQs( shelle );
			currentTime = ( reinterpret_cast< Clock* >( clocker.data() ) )->getCurrentTime();
		}
		shell->passThroughMsgQs( shelle );
	}

	unsigned int f1 = 1;
	unsigned int f2 = 0;
	for ( unsigned int i = 0; i < numFib; ++i ) {
		if ( a1->dataHandler()->isDataHere( i ) ) {
			Arith* data = reinterpret_cast< Arith* >( a1->dataHandler()->data1( i ) );
			cout << Shell::myNode() << ": i = " << i << ", " << data->getOutput() << ", " << f1 << endl;
			assert( data->getOutput() == f1 );
		}
		unsigned int temp = f1;
		f1 = temp + f2;
		f2 = temp;
	}

	a1id.destroy();
	cout << "." << flush;
}

void testBuiltins( bool useMPI )
{
	testArith();
	// if ( !useMPI )
		testFibonacci();
}
