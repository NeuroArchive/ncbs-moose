/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "Message.h"
#include "SparseMatrix.h"
#include "SparseMsg.h"
#include "../randnum/randnum.h"
#include "../biophysics/Synapse.h"

SparseMsg::SparseMsg( Element* e1, Element* e2, Id managerId )
	: Msg( e1, e2, managerId ), 
	matrix_( e1->dataHandler()->numData1(), e2->dataHandler()->numData1() )
{
	assert( e1->dataHandler()->numDimensions() == 1  && 
		e2->dataHandler()->numDimensions() >= 1 );
}

SparseMsg::SparseMsg( Element* e1, Element* e2 )
	: Msg( e1, e2, id_ ), 
	matrix_( e1->dataHandler()->numData1(), e2->dataHandler()->numData1() )
{
	assert( e1->dataHandler()->numDimensions() == 1  && 
		e2->dataHandler()->numDimensions() >= 1 );
}

SparseMsg::~SparseMsg()
{
	if ( safeGetMsg( mid() ) != 0 ) // this may be called by ~PsparseMsg
		MsgManager::dropMsg( mid() );
}

unsigned int rowIndex( const Element* e, const DataId& d )
{
	if ( e->dataHandler()->numDimensions() == 1 ) {
		return d.data();
	} else if ( e->dataHandler()->numDimensions() == 2 ) {
		// This is a nasty case, hopefully very rare.
		unsigned int row = 0;
		for ( unsigned int i = 0; i < d.data(); ++i )
			row += e->dataHandler()->numData2( i );
		return ( row + d.field() );
	}
	return 0;
}

void SparseMsg::exec( const char* arg, const ProcInfo *p ) const
{
	const Qinfo *q = ( reinterpret_cast < const Qinfo * >( arg ) );
	// arg += sizeof( Qinfo );

	/**
	 * The system is really optimized for data from e1 to e2,
	 * where e2 is a 2-D array or an array nested in another array.
	 */
	if ( q->isForward() ) {
		const OpFunc* f = e2_->cinfo()->getOpFunc( q->fid() );
		unsigned int row = rowIndex( e1_, q->srcIndex() );
		const unsigned int* fieldIndex;
		const unsigned int* colIndex;
		unsigned int n = matrix_.getRow( row, &fieldIndex, &colIndex );
		for ( unsigned int j = 0; j < n; ++j ) {
			// Eref tgt( target, DataId( *colIndex++, *fieldIndex++ )
			Eref tgt( e2_, DataId( colIndex[j], fieldIndex[j] ) );
			if ( tgt.isDataHere() )
				f->op( tgt, arg );
		}
	} else { // Avoid using this back operation!
		// Note that we do NOT use the fieldIndex going backward. It is
		// assumed that e1 does not have fieldArrays.
		const OpFunc* f = e1_->cinfo()->getOpFunc( q->fid() );
		unsigned int column = rowIndex( e2_, q->srcIndex() );
		vector< unsigned int > fieldIndex;
		vector< unsigned int > rowIndex;
		unsigned int n = matrix_.getColumn( column, fieldIndex, rowIndex );
		for ( unsigned int j = 0; j < n; ++j ) {
			Eref tgt( e1_, DataId( rowIndex[j] ) );
			if ( tgt.isDataHere() )
				f->op( tgt, arg );
		}
	}
}

bool SparseMsg::add( Element* e1, const string& srcField, 
			Element* e2, const string& destField, double probability )
{
	FuncId funcId;
	const SrcFinfo* srcFinfo = validateMsg( e1, srcField,
		e2, destField, funcId );

	if ( srcFinfo ) {
		SparseMsg* m = new SparseMsg( e1, e2 );
		e1->addMsgAndFunc( m->mid(), funcId, srcFinfo->getBindIndex() );
		m->randomConnect( probability );
		return 1;
	}
	return 0;
}

/**
 * Should really have a seed argument
 */
unsigned int SparseMsg::randomConnect( double probability )
{
	unsigned int nRows = matrix_.nRows(); // Sources
	unsigned int nCols = matrix_.nColumns();	// Destinations
	// matrix_.setSize( 0, nRows ); // we will transpose this later.
	matrix_.clear();
	unsigned int totalSynapses = 0;
	unsigned int startSynapse = 0;
	vector< unsigned int > sizes;
	bool isFirstRound = 1;
	unsigned int totSynNum = 0;

	// SynElement* syn = dynamic_cast< SynElement* >( e2_ );
	Element* syn = e2_;
	syn->dataHandler()->getNumData2( sizes );
	assert( sizes.size() == nCols );

	for ( unsigned int i = 0; i < nCols; ++i ) {
		// Check if synapse is on local node
		bool isSynOnMyNode = syn->dataHandler()->isDataHere( i );
		vector< unsigned int > synIndex;
		// This needs to be obtained from current size of syn array.
		// unsigned int synNum = sizes[ i ];
		unsigned int synNum = 0;
		for ( unsigned int j = 0; j < nRows; ++j ) {
			double r = mtrand(); // Want to ensure it is called each time round the loop.
			if ( isSynOnMyNode ) {
				if ( isFirstRound ) {
					startSynapse = totSynNum;
					isFirstRound = 0;
				}
			}
			if ( r < probability && isSynOnMyNode ) {
				synIndex.push_back( synNum );
				++synNum;
			} else {
				synIndex.push_back( ~0 );
			}
			if ( r < probability )
				++totSynNum;
		}
		sizes[ i ] = synNum;
		totalSynapses += synNum;

		matrix_.addRow( i, synIndex );
	}
	/*
	cout << Shell::myNode() << ": sizes.size() = " << sizes.size() << ", ncols = " << nCols << endl;
	for ( unsigned int i = 0; i < sizes.size(); ++i ) {
		cout << Shell::myNode() << ": sizes[" << i << "] = " << sizes[i] << endl;
	}
	*/
	syn->dataHandler()->setNumData2( startSynapse, sizes );
	cout << Shell::myNode() << ": sizes.size() = " << sizes.size() << ", ncols = " << nCols << ", startSynapse = " << startSynapse << endl;
	matrix_.transpose();
	return totalSynapses;
}

void SparseMsg::setMatrix( const SparseMatrix< unsigned int >& m )
{
	matrix_ = m;
}

Id SparseMsg::id() const
{
	return id_;
}

SparseMatrix< unsigned int >& SparseMsg::matrix() 
{
	return matrix_;
}
