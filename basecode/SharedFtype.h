
/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2005 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU General Public License version 2
** See the file COPYING.LIB for the full notice.
**********************************************************************/
#ifndef _SHARED_FTYPE_H
#define _SHARED_FTYPE_H

typedef pair< const Ftype*, RecvFunc > TypeFuncPair;

/**
 * Handles typing for SharedFinfos. Basically it is a wrapper for
 * multiple Ftypes.
 * Here it has to do something a bit more sophisticated than just
 * matching up Ftypes. It needs to ensure that for every source Ftype
 * there is a matching destination Ftype. 
 *
 * When matching, it takes all the src Ftypes (in order) from one,
 * and all the dest Ftypes ( in order ) from the other, and checks.
 * Then vice versa.
 *
 * It does not worry if Src and Dest Ftypes are interleaved, through.
 * Each contributes to an independent sequence. This means that if
 * I have a single SharedFtype defined with the sequence
 * 			srcA destA srcB destB
 * it will be able to connect to itself. 
 *
 * However, if it were defined
 * 		srcA destB srcB destA, and type A != B
 * then it would not be able to connect to itself.
 *
 */

class SharedFtype: public Ftype
{
		public:
			SharedFtype( pair< const Ftype*, RecvFunc >*, unsigned int);

			unsigned int nValues() const {
				return nValues_;
			}

			bool isSameType( const Ftype* other ) const;

			size_t size() const {
				return size_;
			}

			RecvFunc recvFunc() const {
					return 0;
			}
			RecvFunc trigFunc() const {
					return 0;
			}

		private:
			vector< const Ftype* > destTypes_;
			vector< const Ftype* > srcTypes_;
			unsigned int nValues_;
			unsigned int size_;
};

#endif // _SHARED_FTYPE_H
