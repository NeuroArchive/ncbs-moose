#ifndef MOOSE_PROC_INFO_H
#define MOOSE_PROC_INFO_H
/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

class ProcInfo
{
	public:
		ProcInfo() 
			:       dt( 1.0 ), currTime( 0.0 ), 
                                threadId( 0 ),
				threadIndexInGroup( 0 ), 
                                numThreads( 1 ), 
				numThreadsInGroup( 1 ), 
				nodeIndexInGroup( 0 ),
				numNodesInGroup( 1 ), 
				groupId( 0 ),
#ifdef USE_CHARMPP
				procIndex( 0 )
#else
				procIndex( 0 ),
//				outQid( 0 ), 
				isMpiThread( 0 ),
				barrier1( 0 ),
				barrier2( 0 )
#endif
			{;}


		double dt;
		double currTime;
		unsigned int threadId;
		unsigned int threadIndexInGroup;
		unsigned int numThreads; // Includes the mpiThread, if any
//		Qid outQid;	// Index of outQ to use.
		unsigned int numThreadsInGroup; // compute threads
		unsigned int nodeIndexInGroup;
		unsigned int numNodesInGroup;
		unsigned int groupId;
		unsigned int procIndex; // Look up for this Proc on Shell::getProc
#ifndef USE_CHARMPP
		bool isMpiThread;
		FuncBarrier* barrier1;
		FuncBarrier* barrier2;
		FuncBarrier* barrier3;

		// void* barrier1;
		// void* barrier2;
		/**
		 * This utility function sets policy for which thread to use
		 * for Msg::exec, given any Id/DataId. 
		 * Returns true when the specified Id/DataId should be executed
		 * on current thread.
		 */
		inline bool execThread( Id id, unsigned int dataIndex ) const {
			return threadIndexInGroup == 
				( ( id.value() + dataIndex ) % numThreadsInGroup );
		}
#endif

#if !defined NDEBUG && !defined USE_CHARMPP
    // This is for debugging. Is not thread safe.
    friend ostream& operator <<( ostream& s, const ProcInfo p ){        
        s << "Instance of class ProcInfo"
          << "\nthreadId:\t" << p.threadId
          << "\nprocIndex:\t" << p.procIndex
          << "\nthreadIndexInGroup:\t" << p.threadIndexInGroup
          << "\nnumThreadsInGroup:\t" << p.numThreadsInGroup
          << "\nnodeIndexInGroup:\t" << p.nodeIndexInGroup
          << "\nnumNodesInGroup:\t" << p.numNodesInGroup
          << "\ngroupId:\t" << p.groupId
          << "\nisMpiThread:\t" << p.isMpiThread
          << endl;
        return s;
    }
#endif

};

typedef const ProcInfo* ProcPtr;
#endif // MOOSE_PROC_INFO_H
