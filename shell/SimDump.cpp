/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2007 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "moose.h"
#include "../element/Neutral.h"
#include "Shell.h"
#include "SimDump.h"

//////////////////////////////////////////////////////////////////
// SimDumpInfo functions
//////////////////////////////////////////////////////////////////

SimDumpInfo::SimDumpInfo(
	const string& oldObject, const string& newObject,
			const string& oldFields, const string& newFields)
			: oldObject_( oldObject ), newObject_( newObject )
{
	vector< string > oldList;
	vector< string > newList;

	separateString( oldFields, oldList, " " );
	separateString( newFields, newList, " " );

	if ( oldList.size() != newList.size() ) {
		cout << "Error: SimDumpInfo::SimDumpInfo: field list length diffs:\n" << oldFields << "\n" << newFields << "\n";
		return;
	}
	for ( unsigned int i = 0; i < oldList.size(); i++ )
		fields_[ oldList[ i ] ] = newList[ i ];
}

// Takes info from simobjdump
void SimDumpInfo::setFieldSequence( int argc, const char** argv )
{
	string blank = "";
	fieldSequence_.resize( 0 );
	for ( int i = 0; i < argc; i++ ) {
		string temp( argv[ i ] );
		map< string, string >::iterator j = fields_.find( temp );
		if ( j != fields_.end() )
			fieldSequence_.push_back( j->second );
		else 
			fieldSequence_.push_back( blank );
	}
}

bool SimDumpInfo::setFields( Element* e, int argc, const char** argv )
{
	if ( static_cast< unsigned int >(argc) != fieldSequence_.size() ) {
		cout << "Error: SimDumpInfo::setFields:: Number of argument mismatch\n";
		return 0;
	}
	for ( int i = 0; i < argc; i++ ) {
		if ( fieldSequence_[ i ].length() > 0 )
		{
			const Finfo* f = e->findFinfo( fieldSequence_[ i ] );
			if ( !f ) {
				cout << "Error: SimDumpInfo::setFields:: Failed to find field '" << fieldSequence_[i] << "'\n";
				return 0;
			}
			
			bool ret = f->strSet( e, argv[ i ] );
			if ( ret == 0 )
			{
				cout << "Error: SimDumpInfo::setFields:: Failed to set '";
				cout << e->name() << "/" << 
					fieldSequence_[ i ] << " to " << argv[ i ] << "'\n";
				return 0;
			}
		}
	}
	return 1;
}

//////////////////////////////////////////////////////////////////
// SimDump functions
//////////////////////////////////////////////////////////////////

SimDump::SimDump()
{
	string className = "molecule";
	vector< SimDumpInfo *> sid;
	// Here we initialize some simdump conversions. Several things here
	// are for backward compatibility. Need to think about how to
	// eventually evolve out of these. Perhaps SBML.
	sid.push_back( new SimDumpInfo(
		"kpool", "Molecule", 
		"n nInit vol slave_enable", 
		"n nInit volumeScale slaveEnable") );
	sid.push_back( new SimDumpInfo(
		"kreac", "Reaction", "kf kb", "kf kb") );
	sid.push_back( new SimDumpInfo( "kenz", "Enzyme",
		"k1 k2 k3 usecomplex",
		"k1 k2 k3 mode") );
	sid.push_back( new SimDumpInfo( "xtab", "Table",
	"input output step_mode stepsize",
	"input output mode stepsize" ) );

	sid.push_back( new SimDumpInfo( "group", "Neutral", "", "" ) );
	sid.push_back( new SimDumpInfo( "xgraph", "Neutral", "", "" ) );
	sid.push_back( new SimDumpInfo( "xplot", "Plot", "", "" ) );

	sid.push_back( new SimDumpInfo( "geometry", "Neutral", "", "" ) );
	sid.push_back( new SimDumpInfo( "xcoredraw", "Neutral", "", "" ) );
	sid.push_back( new SimDumpInfo( "xtree", "Neutral", "", "" ) );
	sid.push_back( new SimDumpInfo( "xtext", "Neutral", "", "" ) );

	sid.push_back( new SimDumpInfo( "kchan", "ConcChan",
		"perm Vm",
		"permeability Vm" ) );

	for (unsigned int i = 0 ; i < sid.size(); i++ ) {
		dumpConverter_[ sid[ i ]->oldObject() ] = sid[ i ];
	}
}

void SimDump::simObjDump( int argc, const char** argv )
{
	if ( argc < 3 )
		return;
	string name = argv[ 1 ];
	map< string, SimDumpInfo* >::iterator i = 
		dumpConverter_.find( name );
	if ( i != dumpConverter_.end() ) {
		i->second->setFieldSequence( argc - 2, argv + 2 );
	}
}

void SimDump::simUndump( int argc, const char** argv )
{
	// use a map to associate class with sequence of fields, 
	// as set up in default and also with simobjdump
	if (argc < 4 ) {
		cout << string("usage: ") + argv[ 0 ] +
			" class path clock [fields...]\n";
		return;
	}
	string oldClassName = argv[ 1 ];
	string path = argv[ 2 ];
	map< string, SimDumpInfo*  >::iterator i;

	i = dumpConverter_.find( oldClassName );
	if ( i == dumpConverter_.end() ) {
		cout << string("simundumpFunc: old class name '") + 
			oldClassName + "' not entered into simobjdump\n";
		return;
	}

	// Case 1: The element already exists.
	unsigned int id = Shell::path2eid( path, "/" );
	Element* e = 0;
	if ( id != BAD_ID ) {
		e = Element::element( id );
		assert( e != 0 );
		i->second->setFields( e, argc - 4, argv + 4 );
		return;
	}

	// Case 2: Element does not exist but parent element does.
	string epath = Shell::head( path, "/" );
	id = Shell::path2eid( path, "/" );
	if ( id == BAD_ID ) {
		cout << "simundumpFunc: bad parent path" << endl;
		return;
	}
	Element* parent = Element::element( id );
	assert( parent != 0 );

	string newClassName = i->second->newObject();

	e = Neutral::create( newClassName, epath, parent );
	if ( !e ) {
		cout << "Error: simundumpFunc: Failed to create " << newClassName <<
			" " << epath << endl;
			return;
	}
	i->second->setFields( e, argc - 4, argv + 4 );
}
