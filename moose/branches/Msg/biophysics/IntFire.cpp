#include <queue>
#include "header.h"
#include "Synapse.h"
#include "IntFire.h"

const ConnId spikeSlot = 0;

static SrcFinfo1< double >* spike = 
	new SrcFinfo1< double >( 
		"spike", 
		"Sends out spike events",
		spikeSlot
	);

const Cinfo* IntFire::initCinfo()
{
	static Finfo* intFireFinfos[] = {
		new ValueFinfo< IntFire, double >(
			"Vm",
			"Membrane potential",
			&IntFire::setVm,
			&IntFire::getVm
		),

		new ValueFinfo< IntFire, double >(
			"tau",
			"charging time-course",
			&IntFire::setTau,
			&IntFire::getTau
		),

		new ValueFinfo< IntFire, double >(
			"thresh",
			"firing threshold",
			&IntFire::setThresh,
			&IntFire::getThresh
		),

		new ValueFinfo< IntFire, double >(
			"refractoryPeriod",
			"Minimum time between successive spikes",
			&IntFire::setRefractoryPeriod,
			&IntFire::getRefractoryPeriod
		),

		new ValueFinfo< IntFire, unsigned int >(
			"numSynapses",
			"Number of synapses on IntFire",
			&IntFire::setNumSynapses,
			&IntFire::getNumSynapses
		),
		//////////////////////////////////////////////////////////////
		// MsgDest Definitions
		//////////////////////////////////////////////////////////////
		new DestFinfo( "process",
			"Handles process call",
			new EpFunc1< IntFire, ProcPtr >( &IntFire::eprocess ) ),

		//////////////////////////////////////////////////////////////
		// MsgSrc Definitions
		//////////////////////////////////////////////////////////////

		spike,
	};

	static Cinfo intFireCinfo (
		"IntFire",
		0, // No base class, but eventually I guess it will be neutral.
		intFireFinfos,
		sizeof( intFireFinfos ) / sizeof ( Finfo* ),
		new Dinfo< IntFire >()
	);

	return &intFireCinfo;
}

static const Cinfo* intFireCinfo = IntFire::initCinfo();

IntFire::IntFire()
	: Vm_( 0.0 ), thresh_( 0.0 ), tau_( 1.0 ), refractoryPeriod_( 0.1 ), lastSpike_( -0.1 )
{
	;
}

IntFire::IntFire( double thresh, double tau )
	: Vm_( 0.0 ), thresh_( thresh ), tau_( tau ), refractoryPeriod_( 0.1 ), lastSpike_( -0.1 )
{
	;
}

void IntFire::process( const ProcInfo* p, const Eref& e )
{
	/*
	if ( e.index().data() % 900 == 0 ) {
	cout << "IntFire[" << e.index().data() << "]::process dt = " << p->dt << ", " << " t= " << p->currTime << ", #Pending= " << pendingEvents_.size() << " on thread " << p->threadId << ", " << p->threadIndexInGroup << ", Vm= " << Vm_ << endl << flush;
	*/
	}
	/*
	if ( e.index().data() == 1023 && pendingEvents_.size() > 0 && p->currTime > 0.9 ) {
		cout << "pending size on " << e.index() << " = " << pendingEvents_.size() << endl;
		*/
		/*
		while ( !pendingEvents_.empty() ) {
			double v = pendingEvents_.top().getWeight();
			double d = pendingEvents_.top().getDelay();
			cout << "(" << v << "," << d << ")	";
			pendingEvents_.pop();
		}
	}
	*/
	while ( !pendingEvents_.empty() &&
		pendingEvents_.top().getDelay() <= p->currTime ) {
			Vm_ += pendingEvents_.top().getWeight();
			pendingEvents_.pop();
	}
	if (  ( p->currTime - lastSpike_ ) < refractoryPeriod_ )
		Vm_ = 0.0;

	if ( Vm_ > thresh_ ) {
		spike->send( e, p, p->currTime );
		// e.sendSpike( spikeSlot, p->currTime );
		if ( e.index().data() % 100 == 0 ) {
			// cout << "IntFire[" << e.index().data() << "]::process, zeroing Vm= " << Vm_ << ", Ptr = " << this << endl;
		}
		Vm_ = -1.0e-7;
	} else {
		Vm_ *= ( 1.0 - p->dt / tau_ );
	}


/* This is what we would do for a conductance  channel.
	X_ = activation * xconst1_ + X_ * xconst2_;
	Y_ = X_ * yconst1_ + Y_ * yconst2_;
	*/
	 
/*
	unsigned int synSize = sizeof( SynInfo );
	for( char* i = e.processQ.begin(); i != e.processQ.end(); i += synSize )
	{
		SynInfo* si = static_cast< SynInfo* >( i );
		insertQ( si );
	}
	
	SynInfo* si = processQ.top();
	double current = 0.0;
	while ( si->time < p->time && si != processQ.end() ) {
		current += si->weight;
	}

	v_ += current * Gm_ + Em_ - tau_ * v_;
	if ( v_ > vThresh ) {
		v_ = Em_;
		sendWithId< double >( e, spikeSlot, p->t );
	}
*/
}

/**
 * This variant is called from the regular messaging.
 */
void IntFire::eprocess( Eref e, const Qinfo* q, ProcPtr p )
{
	process( p, e );
}

/**
 * Inserts an event into the pendingEvents queue for spikes.
 * Note that this function lives on the Element managing the Synapses,
 * and gets redirected to the IntFire.
 * This is called by UpFunc1< double >
 */
void IntFire::addSpike( DataId index, const double time )
{
	assert( index.field() < synapses_.size() );
	Synapse s( synapses_[ index.field() ], time );
	// cout << index << "	";
	pendingEvents_.push( s );
}

void IntFire::reinit( Eref& e )
{
	// pendingEvents_.resize( 0 );
	while( !pendingEvents_.empty() )
		pendingEvents_.pop();
	Vm_ = 0.0;
}

/*
void IntFire::clearQ( Eref e )
{
	const char* i = e.generalQ.begin();
	while i != e.generalQ.end() {
		// FuncId* fi = static_cast< FuncId* >( i );
		// i += sizeof( FuncId );
		// i += fi->doOperation( e, i );
		// i += doOperation( *fi, e, i );
		unsigned int op = *static_cast< const unsigned int* >( i );
		i += sizeof( unsigned int );
		i += this->opVec_[ op ]( e, i );
			// opVec is set up statically, has the function ptrs.
			// All are of the form f( Eref e, const char* i ).
	}
}
*/

/*
unsigned int FuncId::doOperation( Eref e, char* i )
{
	unsigned int op = *static_cast< unsigned int* >( i );
	i += sizeof( unsigned int );
	return opVec_[ op ]( i ) + sizeof( unsigned int );
}
*/


void IntFire::setVm( const double v )
{
	Vm_ = v;
}

void IntFire::setTau( const double v )
{
	tau_ = v;
}

void IntFire::setThresh( const double v )
{
	thresh_ = v;
}

void IntFire::setRefractoryPeriod( const double v )
{
	refractoryPeriod_ = v;
	lastSpike_ = -v;
}

void IntFire::setNumSynapses( const unsigned int v )
{
	assert( v < 10000000 );
	synapses_.resize( v );
}

double IntFire::getVm() const
{
	return Vm_;
}

double IntFire::getTau() const
{
	return tau_;
}

double IntFire::getThresh() const
{
	return thresh_;
}

double IntFire::getRefractoryPeriod() const
{
	return refractoryPeriod_;
}

unsigned int IntFire::getNumSynapses() const
{
	return synapses_.size();
}

Synapse* IntFire::synapse( unsigned int i )
{
	return &synapses_[i];
}
