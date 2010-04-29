/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _MSG_H
#define _MSG_H

/**
 * Manages data flow between two elements. Is always many-to-many, with
 * assorted variants.
 */

class Msg
{
	public:
		Msg( Element* e1, Element* e2 );

		Msg( Element* e1, Element* e2, MsgId mid );
		virtual ~Msg();

		/**
		 * Deletes a message identified by its mid.
		 */
		static void deleteMsg( MsgId mid );

		/**
 		* Initialize the Null location in the Msg vector.
 		*/
		static void initNull();

		/**
		 * Calls Process on e1.
		 */
		virtual void process( const ProcInfo *p ) const;

		/**
		 * Report if the msg accepts input from the DataId specified
		 * in the Qinfo. Note that the direction of the message is also
		 * important to know in order to figure this out.
		 * Usually the answer is yes, regardless of DataId.
		 */
		virtual bool isMsgHere( const Qinfo& q ) const {
			return 1;
		}

		
		/**
		 * Execute func( arg ) on all relevant indices of target
		 */
		virtual void exec( const char* arg, const ProcInfo* p ) const = 0;

		/**
		 * Return the first element
		 */
		Element* e1() const {
			return e1_;
		}

		/**
		 * Return the second element
		 */
		Element* e2() const {
			return e2_;
		}

		/**
		 * return the Msg Id.
		 */
		MsgId mid() const {
			return mid_;
		}

		/**
		 * Looks up the message on the global vector of Msgs. No checks,
		 * except assertions in debug mode.
		 */
		static const Msg* getMsg( MsgId m );

		/**
		 * Returns the Msg if the MsgId is valid, otherwise returns 0.
		 * Utility function to check if Msg is OK. Used by diagnostic
		 * function Qinfo::reportQ. Normal code uses getMsg above.
		 */
		static const Msg* safeGetMsg( MsgId m );

		/**
		 * The zero MsgId, used as the error value.
		 */
		static const MsgId badMsg;

		/**
		 * A special MsgId used for Shell to get/set values
		 */
		static const MsgId setMsg;


	protected:
		Element* e1_;
		Element* e2_;
		MsgId mid_; // Index of this msg on the msg_ vector.

		/// Manages all Msgs in the system.
		static vector< Msg* > msg_;

		/**
		 * Manages garbage collection for deleted messages. Deleted
		 * MsgIds are kept here so that they can be reused.
		 */
		static vector< MsgId > garbageMsg_;
};

#endif // _MSG_H
