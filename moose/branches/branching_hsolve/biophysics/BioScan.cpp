/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**   copyright (C) 2003-2007 Upinder S. Bhalla, Niraj Dudani and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "moose.h"
#include "BioScan.h"

// Biophysics headers required for initialization of elements
#include "SpikeGen.h"
#include "SynChan.h"

int BioScan::adjacent( Id compartment, Id exclude, vector< Id >& ret )
{
	int size = ret.size();
	findAdjacent( compartment, ret );
	ret.erase(
		remove( ret.begin(), ret.end(), exclude ),
		ret.end()
	);
	return ret.size() - size;
}

int BioScan::adjacent( Id compartment, vector< Id >& ret )
{
	int size = 0;
	size += targets( compartment, "axial", ret );
	size += targets( compartment, "raxial", ret );
	return size;
}

int BioScan::children( Id compartment, vector< Id >& ret )
{
	return targets( compartment, "axial", ret );
}

int BioScan::channels( Id compartment, vector< Id >& ret )
{
	// Request for elements of type "HHChannel" only since
	// channel messages can lead to synchans as well.
	return targets( compartment, "channel", channel, "HHChannel" );
}

int BioScan::gates( Id channel, vector< Id >& ret )
{
	vector< Id > gate;
	targets( channel, "xGate", gate );
	targets( channel, "yGate", gate );
	targets( channel, "zGate", gate );
	ret.insert( ret.end(), gate.begin(), gate.end() );
	return gate.size();
}

int BioScan::spikegen( Id compartment, vector< Id >& ret )
{
	int size = targets( compartment, "VmSrc", ret );
	
	// Reinitialize
	ProcInfoBase p;
	vector< Id >::iterator ispike;
	for ( ispike = size + ret.begin(); ispike != ret.end(); ++ispike ) {
		SetConn c( ( *ispike )(), 0 );
		SpikeGen::reinitFunc( &c, &p );
	}
	
	return size;
}

int BioScan::synchan( Id compartment, vector< Id >& ret )
{
	// "channel" msgs lead to SynChans as well HHChannels, so request
	// explicitly for former.
	int size = targets( compartment, "channel", ret, "SynChan" );
	
	// Reinitialize
	ProcInfoBase p;
	p.dt_ = dt_;
	vector< Id >::iterator ispike;
	for ( isyn = size + ret.begin(); isyn != ret.end(); ++isyn ) {
		SetConn c( ( *ichan )(), 0 );
		SynChan::reinitFunc( &c, &p );
	}
	
	return size;
}

int BioScan::caTarget( Id channel, vector< Id >& ret )
{
	return targets( channel, "IkSrc", ret );
}

int BioScan::caDepend( Id channel, vector< Id >& ret )
{
	return targets( channel, "concen", ret );
}

void BioScan::synchanFields( Id synchan, SynChanStruct& scs )
{
	SetConn c( synchan(), 0 );
	ProcInfoBase p;
	p.dt_ = dt_;
	
	SynChan::reinitFunc( &c, &p );
	set< SynChanStruct* >( synchan(), "scan", &scs );
}

//~ Meant for small hack below. Temporary.
#include "../builtins/Interpol.h"
#include "HHGate.h"
void BioScan::rates(
	Id gate,
	const vector< double >& grid,
	vector< double >& A,
	vector< double >& B )
{
//~ Temporary
HHGate* h = static_cast< HHGate *>( gate()->data() );
	scanElm_.add( "gate", gate(), "gate" );
	
	A.resize( grid.size() );
	B.resize( grid.size() );
	
	//~ Uglier hack to access Interpol's tables directly.
	//~ Strictly for debugging purposes
	//~ const vector< double >& AA = h->A().table();
	//~ const vector< double >& BB = h->B().table();
	//~ for ( unsigned int i = 0; i < grid.size(); i++ ) {
		//~ A[ i ] = AA[ i ];
		//~ B[ i ] = BB[ i ];
	//~ }
	
	vector< double >::const_iterator igrid;
	vector< double >::iterator ia = A.begin();
	vector< double >::iterator ib = B.begin();
	for ( igrid = grid.begin(); igrid != grid.end(); ++igrid ) {
		//~ send1< double >( scanElm_, gateVmSlot, *igrid );
		//~ Temporary
		A_ = h->A().innerLookup( *igrid );
		B_ = h->B().innerLookup( *igrid );
		
		// locals A_ and B_ receive rate values from gate via a callback.
		*ia = A_;
		*ib = B_;
		++ia, ++ib;
	}
	
	scanElm_.dropAll( "gate" );
}

///////////////////////////////////////////////////
// Utility functions
///////////////////////////////////////////////////

int BioScan::targets(
	Id object,
	const string& msg,
	vector< Id >& target,
	const string& type ) const
{
	unsigned int oldSize = target.size();
	
	Id found;
	Conn* i = object()->targets( msg, 0 );
	for ( ; i->good(); i->increment() ) {
		found = i->target()->id();
		if ( type != "" && !isType( found, type ) )	// speed this up
			continue;
		
		target.push_back( found );
	}
	delete i;
	
	return target.size() - oldSize;
}

bool BioScan::isType( Id object, const string& type ) const
{
	return object()->cinfo()->isA( Cinfo::find( type ) );
}

