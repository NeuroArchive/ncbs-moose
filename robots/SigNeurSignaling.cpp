/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2007 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "moose.h"
#include "../element/Neutral.h"
#include "../biophysics/Compartment.h"
#include "../kinetics/Molecule.h"
#include "../kinetics/Reaction.h"
#include "SigNeur.h"

static const double PI = 3.1415926535;
extern Element* findDiff( Element* pa );

/**
 * setAllVols traverses all signaling compartments  in the model and
 * assigns volumes.
 * This must be called before completeDiffusion because the vols
 * computed here are needed to compute diffusion rates.
 */
void SigNeur::setAllVols()
{
	for ( vector< TreeNode >::iterator i = tree_.begin();
		i != tree_.end(); ++i ) {
		for ( unsigned int j = i->sigStart; j < i->sigEnd; ++j ) {
			if ( i->category == SOMA ) {
				setComptVols( i->compt.eref(), 
					somaMap_, j, i->sigEnd - i->sigStart );
			} else if ( i->category == DEND ) {
				setComptVols( i->compt.eref(), 
					dendMap_, j - numSoma_, i->sigEnd - i->sigStart );
			} else if ( i->category == SPINE ) {
				setComptVols( i->compt.eref(), 
					spineMap_, j - ( numSoma_ + numDend_ ),
					i->sigEnd - i->sigStart );
			}
		}
	}
}

/**
 * This figures out dendritic segment dimensions. It assigns the 
 * volumeScale for each signaling compt, and puts Xarea / len into
 * each diffusion element for future use in setting up diffusion rates.
 */
void SigNeur::setComptVols( Eref compt, 
	map< string, Element* >& molMap,
	unsigned int index, unsigned int numSeg )
{
	static const Finfo* diaFinfo = 
		initCompartmentCinfo()->findFinfo( "diameter" );
	static const Finfo* lenFinfo = 
		initCompartmentCinfo()->findFinfo( "length" );
	static const Finfo* volFinfo = 
		initMoleculeCinfo()->findFinfo( "volumeScale" );
	static const Finfo* nInitFinfo = 
		initMoleculeCinfo()->findFinfo( "nInit" );
	static const Finfo* kfFinfo = 
		initReactionCinfo()->findFinfo( "kf" );
	static const Finfo* kbFinfo = 
		initReactionCinfo()->findFinfo( "kb" );
	assert( diaFinfo != 0 );
	assert( lenFinfo != 0 );
	assert( numSeg > 0 );
	double dia = 0.0;
	double len = 0.0;
	bool ret = get< double >( compt, diaFinfo, dia );
	assert( ret && dia > 0.0 );
	ret = get< double >( compt, lenFinfo, len );
	assert( ret && len > 0.0 );
	len /= numSeg;

	// Conversion factor from uM to #/m^3
	double volscale = len * dia * dia * ( PI / 4.0 ) * 6.0e20;
	double xByL = dia * dia * ( PI / 4.0 ) / len;
	
	// Set all the volumes. 
	for ( map< string, Element* >::iterator i = molMap.begin(); 
		i != molMap.end(); ++i ) {
		Element* de = findDiff( i->second );
		if ( !de )
			continue;
		Eref mol( i->second, index );
		Eref diff( de, index );
		double oldvol;
		double nInit;
		ret = get< double >( mol, volFinfo, oldvol );
		assert( ret != 0 && oldvol > 0.0 );
		ret = get< double >( mol, nInitFinfo, nInit );
		assert( ret );

		ret = set< double >( mol, volFinfo, volscale );
		assert( ret );
		ret = set< double >( mol, nInitFinfo, nInit * volscale / oldvol );
		assert( ret );
		ret = set< double >( diff, kfFinfo, xByL );
		assert( ret );
		ret = set< double >( diff, kbFinfo, xByL );
		assert( ret );
	}
}

/*
 * This function copies a signaling model. It first traverses the model and
 * inserts any required diffusion reactions into the model. These are
 * created as children of the molecule that diffuses, and are connected
 * up locally for one-half of the diffusion process. Subsequently the
 * system needs to connect up to the next compartment, to set up the 
 * other half of the diffusion. Also the last diffusion reaction
 * needs to have its rates nulled out.
 *
 * Returns the root element of the copy.
 * Kinid is destination of copy
 * proto is prototype
 * Name is the name to assign to the copy.
 * num is the number of duplicates needed.
 */
Element* SigNeur::copySig( Id kinId, Id proto, 
	const string& name, unsigned int num )
{
	Element* ret = 0;
	if ( proto.good() ) {
		Id lib( "/library" );
		/*
		Element* temp = Neutral::create( "Neutral", "temp", libId, 
			Id::childId( libId ) );
		*/
		ret = proto()->copy( lib(), name + ".msgs" );
		assert( ret );
		insertDiffusion( ret ); // Scan through putting in diffs.

		if ( num == 1 ) {
			ret = ret->copy( kinId(), name );
		} else if ( num > 1 ) {
			ret = ret->copyIntoArray( kinId, name, num );
		}
	}
	return ret;
}

void SigNeur::makeSignalingModel( Eref me )
{
	// Make kinetic manager on sigNeur
	// Make array copy of soma model.
	// Make array copy of dend model.
	// Make array copy of spine model.
	// Traverse tree, set up diffusion messages.
	// If any are nonzero, activate kinetic manager
	
	Element* kin = Neutral::create( "KineticManager", "kinetics", 
		me.id(), Id::childId( me.id() ) );
 	Id kinId = kin->id();
	// Each of these protos should be a simple Element neutral.
	Element* soma = copySig( kinId, somaProto_, "soma", numSoma_ );
	Element* dend = copySig( kinId, dendProto_, "dend", numDend_ );
	Element* spine = copySig( kinId, spineProto_, "spine", numSpine_ );

	if ( soma )
		soma_ = soma->id();
	if ( dend )
		dend_ = dend->id();
	if ( spine )
		spine_ = spine->id();

	// first soma indices, then dend, then spines.
	vector< unsigned int > junctions( 
		numSoma_ + numDend_ + numSpine_, UINT_MAX );
	buildDiffusionJunctions( junctions );
	buildMoleculeNameMap( soma, somaMap_ );
	buildMoleculeNameMap( dend, dendMap_ );
	buildMoleculeNameMap( spine, spineMap_ );

	/*
	for ( unsigned int j = 0; j < junctions.size(); ++j )
		cout << " " << j << "," << junctions[j];
	cout << endl;
	*/

	setAllVols();

	completeSomaDiffusion( junctions );
	completeDendDiffusion( junctions );
	completeSpineDiffusion( junctions );

	// set< string >( kin, "method", dendMethod_ );
}

/**
 * Traverses the signaling tree to build a map of molecule Elements 
 * looked up by name.
 */
void SigNeur::buildMoleculeNameMap( Element* e,
	map< string, Element* >& molMap )
{
	static const Finfo* childSrcFinfo = 
		initNeutralCinfo()->findFinfo( "childSrc" );
	static const Finfo* parentFinfo = 
		initNeutralCinfo()->findFinfo( "parent" );
	if ( e == 0 )
		return;
	
	if ( e->cinfo()->isA( initMoleculeCinfo() ) ) {
		string ename = e->name();
		if ( ename == "kenz_cplx" ) {
			Id pa;
			get< Id >( e, parentFinfo, pa );
			Id grandpa;
			get< Id >( pa(), parentFinfo, grandpa );
			ename = grandpa()->name() + "__" + ename;
		}
		map< string, Element* >::iterator i = molMap.find( ename );
		if ( i != molMap.end() ) {
			cout << "buildMoleculeNameMap:: Warning: duplicate molecule: "
				<< i->second->id().path() << ", " << e->id().path() << endl;
		} else {
			molMap[ ename ] = e;
		}
	}
	// Traverse children.
	const Msg* m = e->msg( childSrcFinfo->msg() );
	for ( vector< ConnTainer* >::const_iterator i = m->begin();
		i != m->end(); ++i ) {
		if ( (*i)->e2() != e )
			buildMoleculeNameMap( (*i)->e2(), molMap );
		else
			buildMoleculeNameMap( (*i)->e1(), molMap );
	}
}
