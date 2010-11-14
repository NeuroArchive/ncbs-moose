/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

// #include <unistd.h>
#include <pthread.h>
#ifdef USE_MPI
#include <mpi.h>
#endif
#include "FuncBarrier.h"

// #include <stdlib.h>
#include "header.h"
#include "../scheduling/Tick.h"
#include "../scheduling/TickMgr.h"
#include "../scheduling/TickPtr.h"
#include "../scheduling/Clock.h"

void* eventLoopForBcast( void* info )
{
	ProcInfo *p = reinterpret_cast< ProcInfo* >( info );
	cout << "eventLoop on " << p->nodeIndexInGroup << ":" << 
		p->threadIndexInGroup << endl;
	Clock* clock = reinterpret_cast< Clock* >( Id(1).eref().data() );

	while( clock->keepLooping() )
	// for( unsigned int i = 0; i < NLOOP; ++i )
	{
		// Phase 1. Here we carry out the Process calculations
		// on all simulated objects.
		clock->processPhase1( p );
		// This custom barrier also carries out the swap operation 
		// internally.
		p->barrier1->wait(); // Within this func, inQ and outQ are swapped.

		// Phase 2.
		// Then we clean up all the local node Msgs.
		// In parallel, the MPI data transfer begins by broadcasting
		// contents of inQ on node 0.
		clock->processPhase2( p ); // Do tick juggling for the clock.
		Qinfo::readQ( p ); // Deliver all local node Msgs.

		// Phase 3
		// The allgather approach is not going to scale well: 
		// For N nodes mpiQ needs to set aside N*sizeof(inQ). 
		// Instead, do N bcast calls and interleave with the processing
		// for the data received on the previous bcast call.
		// If we can permit slower internode data transfer then the #
		// of bcast calls goes down.
		for ( unsigned int j = 0; j < p->numNodesInGroup; ++j ) {
			p->barrier2->wait(); // This barrier swaps mpiInQ and mpiOutQ
			if ( j != p->nodeIndexInGroup )
				Qinfo::readMpiQ( p );
		}

		// Here we use the stock pthreads barrier, whose performance is
		// pretty dismal. Worth comparing with the Butenhof barrier. I
		// earlier wrote a nasty barrier that does a busy-loop but was
		// _much_ faster.
		p->barrier3->wait();
		// rc = pthread_barrier_wait( p->barrier3 );
		// assert( rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD );
	}
	pthread_exit( NULL );
}

/*
 * Happens on the one thread doing MPI stuff. This variant does an
 * MPI_Bcast to every node in the group. 
 */
void* mpiEventLoopForBcast( void* info )
{
	ProcInfo *p = reinterpret_cast< ProcInfo* >( info );
	cout << "mpiEventLoop on " << p->nodeIndexInGroup << ":" << 
		p->threadIndexInGroup << endl;

	Clock* clock = reinterpret_cast< Clock* >( Id( 1 ).eref().data() );
	while( clock->keepLooping() )
// 	for( unsigned int i = 0; i < NLOOP; ++i )
	{
		// Phase 1: do nothing. But we must wait for barrier 0 to clear,
		// because we need inQ to be settled before broadcasting it.
		p->barrier1->wait();

		// Phase 2, 3. Now we loop around barrier 2 till all nodes have
		// sent data and the data has been received and processed.
		// On the process threads the inQ/mpiInQ is busy being executed.
		for ( unsigned int j = 0; j < p->numNodesInGroup; ++j ) {
#ifdef USE_MPI
			if ( p->nodeIndexInGroup == j )
				MPI_Bcast( inQ, QSIZE * sizeof( Tracker ), MPI_CHAR, j, MPI_COMM_WORLD );
			else 
				MPI_Bcast( mpiRecvQ, QSIZE * sizeof( Tracker ), MPI_CHAR, j, MPI_COMM_WORLD );
#endif
			p->barrier2->wait(); // This barrier swaps mpiInQ and mpiRecvQ
		}

		// Phase 3: Read and execute the arrived MPI data on all threads 
		// except the one which just sent it out.
		// On this thread, we just wait till the final barrier.
		p->barrier3->wait();
		// int rc = pthread_barrier_wait( p->barrier3 );
		// assert( rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD );
	}
	pthread_exit( NULL );
}

/*
 * Happens on the one thread doing Shell stuff.
 */
void* shellEventLoop( void* info )
{
	ProcInfo *p = reinterpret_cast< ProcInfo* >( info );
	cout << "shellEventLoop on " << p->nodeIndexInGroup << ":" << 
		p->threadIndexInGroup << endl;

	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	Clock* clock = reinterpret_cast< Clock* >( Id(1).eref().data() );
	while( clock->keepLooping() )
	// for( unsigned int i = 0; i < NLOOP; ++i )
	{
		// Phase 1: Protect the barrier (actually, the swap call)
		// with a mutex so that the Shell doesn't insert data into outQ
		// while things are changing. Note that this outQ is in the
		// Shell group and thus is safe from the other threads.
		pthread_mutex_lock( shell->parserMutex() );
			p->barrier1->wait();
			if ( shell->inBlockingParserCall() && !shell->isAckPending() ) {
				pthread_cond_signal( shell->parserBlockCond() );
			}
		pthread_mutex_unlock( shell->parserMutex() );

		// Phase 2. Here we simply ignore barrier 2 as it
		// does not matter for the Shell. This takes a little
		// care when initializing the threads, but saves time.

		// Phase 3: We need to block here to ensure that the endit
		// call is encapsulated within the current cycle.
		p->barrier3->wait();
		// int rc = pthread_barrier_wait( p->barrier3 );
		// assert( rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD );
	}
	pthread_exit( NULL );
}




void* reportGraphics( void* info )
{
	ProcInfo *p = reinterpret_cast< ProcInfo* >( info );
	cout << "reportGraphics on " << p->nodeIndexInGroup << ":" << 
		p->threadIndexInGroup << endl;
	pthread_exit( NULL );
}

void Shell::launchThreads()
{
	attr_ = new pthread_attr_t;
	pthread_attr_init( attr_ );
	pthread_attr_setdetachstate( attr_, PTHREAD_CREATE_JOINABLE );
	unsigned int numThreads = numCores_ + 1; // Add one for the MPI thread.

	// Extra thread on barrier 1 for parser control on node 0 
	// (the main thread here).
	unsigned int numBarrier1Threads = numThreads + ( myNode_ == 0 );

	barrier1_ = new FuncBarrier( numBarrier1Threads, &Qinfo::swapQ );
	barrier2_ = new FuncBarrier( numThreads, &Qinfo::swapMpiQ );
	barrier3_ = new FuncBarrier( numThreads );
	// barrier3 = new pthread_barrier_t;
	int ret;
	pthread_t gThread;

	parserMutex_ = new pthread_mutex_t; // Assign the Shell variables.
	parserBlockCond_ = new pthread_cond_t;

	ret = pthread_mutex_init( parserMutex_, NULL );
	assert( ret == 0 );

	ret = pthread_cond_init( parserBlockCond_, NULL );
	assert( ret == 0 );

	// ret = pthread_barrier_init( barrier3, NULL, numBarrier1Threads );
	// assert( ret == 0 );

	if ( myNode_ == 0 ) { // Launch graphics thread only on node 0.
		ProcInfo p;
		// pthread_barrier_t barrier1;
		int rc = pthread_create(&gThread, NULL, reportGraphics, 
			reinterpret_cast< void* >( &p ) );
		if ( rc )
			cout << "Error: return code from pthread_create: " << rc << endl;
	}
	threadProcs_.resize( numBarrier1Threads );
	vector< ProcInfo >& p = threadProcs_;
	// An extra thread is used by MPI, and on node 0, yet another for Shell
	// pthread_t* threads = new pthread_t[ numBarrier1Threads ];
	threads_ = new pthread_t[ numBarrier1Threads ];

	for ( unsigned int i = 0; i < numBarrier1Threads; ++i ) {
		// Note that here we put # of compute cores, not total threads.
		p[i].numThreadsInGroup = numCores_; 

		p[i].threadIndexInGroup = i;
		p[i].nodeIndexInGroup = myNode_;
		p[i].numNodesInGroup = numNodes_;
		p[i].barrier1 = barrier1_;
		p[i].barrier2 = barrier2_;
		p[i].barrier3 = barrier3_;

		// These are the compute threads
		if ( i < numCores_ ) { 
			int rc = pthread_create( threads_ + i, NULL, eventLoopForBcast, 
				(void *)&p[i] );
			assert( rc == 0 );
		} else if ( i == numCores_ ) { // mpiThread stufff.
			int rc = pthread_create( 
				threads_ + i, NULL, mpiEventLoopForBcast, (void *)&p[i] );
			assert( rc == 0 );
		} else if ( i == numThreads ) { // shellThread stuff.
			int rc = pthread_create( 
				threads_ + i, NULL, shellEventLoop, (void *)&p[i] );
			assert( rc == 0 );
		}
	}
	cout << "Calling thread\n";
}

void Shell::joinThreads()
{
	int numThreads = numCores_ + 1; // Add one for the MPI thread.
	int numBarrier1Threads = numThreads + ( myNode_ == 0 );
	int ret;

	for ( int i = 0; i < numBarrier1Threads; ++i ) {
		void* status;
		ret = pthread_join( threads_[i], &status );
		if ( ret )
			cout << "Error: Unable to join threads\n";
	}

	if ( myNode_ == 0 ) { // clean up graphics thread only on node 0.
		void* status;
		ret = pthread_join( *gThread_, &status );
		if ( ret )
			cout << "Error: Unable to join threads\n";
	}

	delete[] threads_;
	pthread_attr_destroy( attr_ );
	delete attr_;
	ret = pthread_mutex_destroy( parserMutex_ );
	ret = pthread_cond_destroy( parserBlockCond_ );
	assert( ret == 0 );
}

