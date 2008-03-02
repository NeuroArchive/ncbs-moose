/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2008 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _CONN_TAINER_H
#define _CONN_TAINER_H

class ConnTainer
{
	public:
		ConnTainer( Element* e1, Element* e2,
			unsigned int msg1, unsigned int msg2 )
			:
			e1_( e1 ), e2_( e2 ),
			msg1_( msg1 ), msg2_( msg2 )
		{;}

		virtual ~ConnTainer()
		{;}

		/**
		 * Generates an iterator for this ConnTainer. The eIndex
		 * specifies the index of the originating element.
		 * The isReverse flag is passed in from the Msg and specifies
		 * if we are going forward or back along the direction of the Msg.
		 */
		virtual Conn* conn( unsigned int eIndex, bool isReverse ) const = 0;

		/**
		 * Generates an iterator for this ConnTainer. The eIndex
		 * specifies the index of the originating element.
		 * The isReverse flag is passed in from the Msg and specifies
		 * if we are going forward or back along the direction of the Msg.
		 * The connIndex specifies a conn within this ConnTainer.
		 */
		virtual Conn* conn( unsigned int eIndex, bool isReverse, 
			unsigned int connIndex ) const = 0;

		// virtual bool add( Element* e1, Element* e2 ) = 0;

		virtual Element* e1() const {
			return e1_;
		}

		virtual Element* e2() const {
			return e2_;
		}

		virtual unsigned int msg1() const {
			return msg1_;
		}

		virtual unsigned int msg2() const {
			return msg2_;
		}

		/**
		 * Returns the number of targets on this ConnTainer
		 */
		virtual unsigned int size() const = 0;

		/**
		 * Creates a duplicate ConnTainer for message(s) between 
		 * new elements e1 and e2, and adds this new container to the
		 * targets. It checks the original version for which msgs to put 
		 * the new one on.
		 * e1 must be the new source element.
		 * Returns true on success.
		 */
		virtual bool copy( Element* e1, Element* e2 ) const = 0;
		
	private:
		Element* e1_;
		Element* e2_;
		unsigned int msg1_;
		unsigned int msg2_;
};

#endif // _CONN_TAINER_H
