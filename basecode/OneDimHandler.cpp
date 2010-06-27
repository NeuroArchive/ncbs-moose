/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"

OneDimHandler::OneDimHandler( const DinfoBase* dinfo )
		: DataHandler( dinfo ), 
			data_( 0 ), size_( 0 ), 
			start_( 0 ), end_( 0 )
{;}

OneDimHandler::~OneDimHandler() {
	dinfo()->destroyData( data_ );
}

DataHandler* OneDimHandler::copy( unsigned int n, bool toGlobal )
	const
{
	if ( n > 1 ) {
		cout << Shell::myNode() << ": Error: OneDimHandler::copy: Cannot yet handle 2d arrays\n";
		exit( 0 );
	}
	if ( Shell::myNode() > 0 ) {
		cout << Shell::myNode() << ": Error: OneDimHandler::copy: Cannot yet handle multinode simulations\n";
		exit( 0 );
	}

	if ( toGlobal ) {
		if ( n <= 1 ) { // Don't need to boost dimension.
			OneDimGlobalHandler* ret = new OneDimGlobalHandler( dinfo() );
//			ret->setNumData1( size_ );
			ret->setData( dinfo()->copyData( data_, size_, 1 ), size_ );
			return ret;
		} else {
			OneDimGlobalHandler* ret = new OneDimGlobalHandler( dinfo() );
//			ret->setNumData1( n * size_ );
			ret->setData( dinfo()->copyData( data_, size_, n ), size_ * n );
			return ret;
		}
	} else {
		if ( n <= 1 ) { // do copy only on node 0.
			OneDimHandler* ret = new OneDimHandler( dinfo() );
//			ret->setNumData1( size_ );
			ret->setData( dinfo()->copyData( data_, size_, 1 ), size_ );
			return ret;
		} else {
			OneDimHandler* ret = new OneDimHandler( dinfo() );
			unsigned int size = ret->end() - ret->begin();
			if ( size > 0 ) {
//				ret->setNumData1( size_ * size );
				ret->setData( dinfo()->copyData( data_, size_, size ), 
					size_ * size );
			}
			return ret;
		}
	}
}


/**
 * Handles both the thread and node decomposition
 */
void OneDimHandler::process( const ProcInfo* p, Element* e, FuncId fid ) const
{
	/**
	 * This is the variant with interleaved threads.
	char* temp = data_ + p->threadIndexInGroup * dinfo()->size();
	unsigned int stride = dinfo()->size() * p->numThreadsInGroup;
	for ( unsigned int i = start_ + p->threadIndexInGroup; i < end_;
		i += p->numThreadsInGroup ) {
		reinterpret_cast< Data* >( temp )->process( p, Eref( e, i ) );
		temp += stride;
	}
	 */

	/**
	 * This is the variant with threads in a block.
	 */
	unsigned int startIndex = start_ + 
		( ( end_ - start_ ) * p->threadIndexInGroup + 
		p->numThreadsInGroup - 1 ) /
			p->numThreadsInGroup;
	unsigned int endIndex = start_ + 
		( ( end_ - start_ ) * ( 1 + p->threadIndexInGroup ) +
		p->numThreadsInGroup - 1 ) /
			p->numThreadsInGroup;
	
	char* temp = data_ + ( startIndex - start_ ) * dinfo()->size();
	/*
	for ( unsigned int i = startIndex; i != endIndex; ++i ) {
		reinterpret_cast< Data* >( temp )->process( p, Eref( e, i ) );
		temp += dinfo()->size();
	}
	*/

	const OpFunc* f = e->cinfo()->getOpFunc( fid );
	const ProcOpFuncBase* pf = dynamic_cast< const ProcOpFuncBase* >( f );
	assert( pf );
	for ( unsigned int i = startIndex; i != endIndex; ++i ) {
		pf->proc( temp, Eref( e, i ), p );
		temp += dinfo()->size();
	}
}


char* OneDimHandler::data( DataId index ) const {
	if ( isDataHere( index ) )
		return data_ + ( index.data() - start_ ) * dinfo()->size();
	return 0;
}

char* OneDimHandler::data1( DataId index ) const {
	if ( isDataHere( index ) )
		return data_ + ( index.data() - start_ ) * dinfo()->size();
	return 0;
}

/**
 * Assigns the size to use for the first (data) dimension
* If data is allocated, resizes that.
* If data is not allocated, does not touch it.
* For now: allocate it every time.
 */
void OneDimHandler::setNumData1( unsigned int size )
{
	size_ = size;
	unsigned int start =
		( size_ * Shell::myNode() ) / Shell::numNodes();
	unsigned int end = 
		( size_ * ( 1 + Shell::myNode() ) ) / Shell::numNodes();
	// if ( data_ ) {
		if ( start == start_ && end == end_ ) // already done
			return;
		// Otherwise reallocate.
		if ( data_ )
			dinfo()->destroyData( data_ );
		data_ = reinterpret_cast< char* >( 
			dinfo()->allocData( end - start ) );
	// }
	start_ = start;
	end_ = end;
}

/**
* Assigns the sizes of all array field entries at once.
* Ignore if 1 or 0 dimensions.
*/
void OneDimHandler::setNumData2( unsigned int start,
	const vector< unsigned int >& sizes )
{
	;
}

/**
 * Looks up the sizes of all array field entries at once.
 * Ignore in this case
 */
unsigned int OneDimHandler::getNumData2( vector< unsigned int >& sizes )
	const
{
	return 0;
}

/**
 * Returns true if the node decomposition has the data on the
 * current node
 */
bool OneDimHandler::isDataHere( DataId index ) const {
	return ( index.data() >= start_ && index.data() < end_ );
}

bool OneDimHandler::isAllocated() const {
	return data_ != 0;
}

void OneDimHandler::allocate() {
	if ( data_ )
		dinfo()->destroyData( data_ );
	data_ = reinterpret_cast< char* >( dinfo()->allocData( end_ - start_ ));
}

// Should really be 'start, end' rather than size. See setNumData1.
void OneDimHandler::setData( char* data, unsigned int size )
{
	data_ = data; // Data is preallocated, so we only want to set the size.
	size_ = size;
	start_ = 0;
	end_ = size;
}
