/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2012 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/
#include "header.h"
#include "../shell/Shell.h"
#include "../shell/Wildcard.h"
#include "SimManager.h"

/*
static SrcFinfo1< Id >* plugin()
{
	static SrcFinfo1< Id > ret(
		"plugin", 
		"Sends out Stoich Id so that plugins can directly access fields and functions"
	);
	return &ret;
}
*/
static SrcFinfo0* requestMeshStats()
{
	static SrcFinfo0 requestMeshStats(
		"requestMeshStats", 
		"Asks for basic stats for mesh:"
		"Total # of entries, and a vector of unique volumes of voxels"
	);
	return &requestMeshStats;
}

static SrcFinfo2< unsigned int, unsigned int >* nodeInfo()
{
	static SrcFinfo2< unsigned int, unsigned int > nodeInfo(
		"nodeInfo", 
		"Sends out # of nodes to use for meshing, and # of threads to "
		"use on each node, to the ChemMesh. These numbers sometimes"
		"differ from the total # of nodes and threads, because the "
		"SimManager may have other portions of the model to allocate."
	);
	return &nodeInfo;
}

const Cinfo* SimManager::initCinfo()
{
		//////////////////////////////////////////////////////////////
		// Field Definitions
		//////////////////////////////////////////////////////////////
		static ValueFinfo< SimManager, double > syncTime(
			"syncTime",
			"SyncTime is the interval between synchornizing solvers"
			"5 msec is a typical value",
			&SimManager::setSyncTime,
			&SimManager::getSyncTime
		);

		static ValueFinfo< SimManager, bool > autoPlot(
			"autoPlot",
			"When the autoPlot flag is true, the simManager guesses which"
			"plots are of interest, and builds them.",
			&SimManager::setAutoPlot,
			&SimManager::getAutoPlot
		);

		static ValueFinfo< SimManager, double > plotDt(
			"plotDt",
			"plotDt is the timestep for plotting variables. As most will be"
			"chemical, a default of 1 sec is reasonable",
			&SimManager::setPlotDt,
			&SimManager::getPlotDt
		);

		//////////////////////////////////////////////////////////////
		// MsgDest Definitions
		//////////////////////////////////////////////////////////////
		static DestFinfo build( "build",
			"Sets up model, with the specified method. The method may be"
			"empty if the intention is that methods be set up through "
			"hints in the ChemMesh compartments.",
			new EpFunc1< SimManager, string >( &SimManager::build ) );

		static DestFinfo makeStandardElements( "makeStandardElements",
			"Sets up the usual infrastructure for a model, with the"
			"ChemMesh, Stoich, solver and suitable messaging."
			"The argument is the MeshClass to use.",
			new EpFunc1< SimManager, string >( &SimManager::makeStandardElements ) );

		static DestFinfo meshSplit( "meshSplit",
			"Handles message from ChemMesh that defines how"
			"meshEntries communicate between nodes."
			"First arg is list of other nodes, second arg is list number of"
			"meshEntries to be transferred for each of these nodes, "
			"third arg is catenated list of meshEntries indices on"
			"my node going to each of the other connected nodes, and"
			"fourth arg is matching list of meshEntries on other nodes",
			new EpFunc4< SimManager, vector< unsigned int >, vector< unsigned int>, vector<     unsigned int >, vector< unsigned int > >( &SimManager::meshSplit )
		);

		static DestFinfo meshStats( "meshStats",
			 "Basic statistics for mesh: Total # of entries, and a vector"
			 "of unique volumes of voxels",
			 new EpFunc2< SimManager, unsigned int, vector< double > >( 
			 	&SimManager::meshStats )
		);

		//////////////////////////////////////////////////////////////
		// Shared Finfos
		//////////////////////////////////////////////////////////////
		static Finfo* nodeMeshingShared[] = {
			&meshSplit, &meshStats, requestMeshStats(), nodeInfo()
		};  
		
		static SharedFinfo nodeMeshing( "nodeMeshing",
			"Connects to ChemMesh to coordinate meshing with parallel"
			"decomposition and with the Stoich",
			nodeMeshingShared, 
			sizeof( nodeMeshingShared ) / sizeof( const Finfo* )
		);

		//////////////////////////////////////////////////////////////

	static Finfo* simManagerFinfos[] = {
		&syncTime,		// Value
		&autoPlot,		// Value
		&plotDt,		// Value
		&build,			// DestFinfo
		&makeStandardElements,			// DestFinfo
		&nodeMeshing,	// SharedFinfo
	};

	static Cinfo simManagerCinfo (
		"SimManager",
		Neutral::initCinfo(),
		simManagerFinfos,
		sizeof( simManagerFinfos ) / sizeof ( Finfo* ),
		new Dinfo< SimManager >()
	);

	return &simManagerCinfo;
}

//////////////////////////////////////////////////////////////
// Class definitions
//////////////////////////////////////////////////////////////
static const Cinfo* simManagerCinfo = SimManager::initCinfo();

SimManager::SimManager()
	: 
		syncTime_( 0.005 ),
		autoPlot_( 1 ),
		plotdt_( 1 )
{;}

SimManager::~SimManager()
{;}

//////////////////////////////////////////////////////////////
// Field Definitions
//////////////////////////////////////////////////////////////

void SimManager::setAutoPlot( bool v )
{
	autoPlot_ = v;
}

bool SimManager::getAutoPlot() const
{
	return autoPlot_;
}

void SimManager::setSyncTime( double v )
{
	syncTime_ = v;
}

double SimManager::getSyncTime() const
{
	return syncTime_;
}

void SimManager::setPlotDt( double v )
{
	plotdt_ = v;
}

double SimManager::getPlotDt() const
{
	return plotdt_;
}
//////////////////////////////////////////////////////////////
// MsgDest Definitions
//////////////////////////////////////////////////////////////

Id SimManager::findChemMesh() const
{
	vector< Id > ret;
	string basePath = baseId_.path();

	int num = simpleWildcardFind( basePath + "/##[ISA=ChemMesh]", ret );
	if ( num == 0 )
		return Id();
	return ret[0];
}

double estimateChemLoad( Id mesh, Id stoich )
{

	unsigned int numMeshEntries = 
	 	Field< unsigned int >::get( mesh, "num_mesh" );
	unsigned int numPools = 
	 	Field< unsigned int >::get( stoich, "nVarPools" );
	double dt = Field< double >::get( stoich, "estimatedDt" );
	double load = numMeshEntries * numPools / dt;
	return load;
}

double estimateHsolveLoad( Id hsolver )
{
	// First check if solver exists.
	/*
	double dt = Field< double >::get( hsolver, "estimatedDt" );
	double mathDt = Field< double >::get( hsolver, "numHHChans" );
	*/
	return 0;
}

/**
 * This needs to be called only on the master node, and in shell
 * thread mode.
 */
void SimManager::build( const Eref& e, const Qinfo* q, string method )
{
	// First, check if the tree has a compartment/ChemMesh as the base
	// of the chemical system. If not, put in a single-voxel ChemMesh.
	baseId_ = e.id();
	Id mesh = findChemMesh();

	if ( mesh == Id() ) {
		 cout << "SimManager::build: No chem mesh found, still need to sort this out\n";
		 return;
	}
	buildFromKkitTree( e, q, method );

	// Apply heuristic for threads and nodes
	// Replicate pools as per node decomp. Shell::handleReMesh
	// Make the stoich, set up its path
	// Make the GslIntegrator
	// Set up GslIntegrator in the usual way.
	// ? Apply boundary conditions
	// Set up internode messages to and from stoichs
	// Set up and assign the clocks
	// Create the plots.
}

void SimManager::makeStandardElements( const Eref& e, const Qinfo* q, 
	string meshClass )
{
	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	vector< int > dims( 1, 1 );
	Id baseId_ = e.id();
	Id kinetics = 
		shell->doCreate( meshClass, baseId_, "kinetics", dims, true );
		SetGet2< double, unsigned int >::set( kinetics, "buildDefaultMesh", 1e-15, 1 );
	assert( kinetics != Id() );
	assert( kinetics.eref().element()->getName() == "kinetics" );

	Id graphs = Neutral::child( baseId_.eref(), "graphs" );
	if ( graphs == Id() ) {
		graphs = 
		shell->doCreate( "Neutral", baseId_, "graphs", dims, true );
	}
	assert( graphs != Id() );

	Id geometry = Neutral::child( baseId_.eref(), "geometry" );
	if ( geometry == Id() ) {

		geometry = 
		shell->doCreate( "Geometry", baseId_, "geometry", dims, true );
		// MsgId ret = shell->doAddMsg( "Single", geometry, "compt", kinetics, "reac" );
		// assert( ret != Msg::bad );
	}
	assert( geometry != Id() );

	Id groups = Neutral::child( baseId_.eref(), "groups" );
	if ( groups == Id() ) {
		groups = 
		shell->doCreate( "Neutral", baseId_, "groups", dims, true );
	}
	assert( groups != Id() );
}

void SimManager::meshSplit( const Eref& e, const Qinfo* q,
	vector< unsigned int > nodeList, 
	vector< unsigned int > numEntriesPerNode, 
	vector< unsigned int > outgoingEntries, 
	vector< unsigned int > incomingEntries
	)
{
	cout << "in SimManager::meshSplit"	;
	// buildFromKkitTree( "gsl" );
}

void SimManager::meshStats( const Eref& e, const Qinfo* q,
	unsigned int numMeshEntries, vector< double > voxelVols )
{
	cout << "in SimManager::meshStats"	;
}

//////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////

void SimManager::buildFromBareKineticTree( const string& method )
{
	;
}

 // Don't make any solvers.
void SimManager::buildEE( Shell* shell )
{
	string basePath = baseId_.path();
	shell->doUseClock( basePath + "/kinetics/##[TYPE=Pool]", "process", 0);
		// Normally we would simply say [ISA!=Pool] here. But that puts
		// a Process operation on the mesh, which should not be done in
		// this mode as diffusion isn't supported.
	shell->doUseClock( basePath + "/kinetics/##[ISA!=Pool]", "process", 1);
}

void SimManager::buildGssa( const Eref& e, const Qinfo* q, Shell* shell )
{
	vector< int > dims( 1, 1 );
	 // This is a placeholder for more sophisticated node-balancing info
	 // May also need to put in volscales here.
	stoich_ = shell->doCreate( "GssaStoich", baseId_, "stoich", dims );

	string basePath = baseId_.path();
	Id compt( basePath + "/kinetics" );
	assert( compt != Id() );

	Field< string >::set( stoich_, "path", basePath + "/kinetics/##");

	MsgId mid = shell->doAddMsg( "Single", compt, "meshSplit", 
		stoich_, "meshSplit" );
	assert( mid != Msg::bad );

	double chemLoad = estimateChemLoad( compt, stoich_ );
	// Here we would also estimate cell load
	Id hsolver;
	double hsolveLoad = estimateHsolveLoad( hsolver );

	numChemNodes_ = Shell::numNodes() * chemLoad / ( chemLoad + hsolveLoad);
	
	nodeInfo()->send( e, q->threadNum(), numChemNodes_,
		Shell::numProcessThreads() ); 
	Qinfo::waitProcCycles( 2 );

	string path0 = basePath + "/kinetics/mesh," + 
		basePath + "/kinetics/##[ISA=StimulusTable]";
	shell->doUseClock( path0, "process", 0);
	shell->doUseClock( basePath + "/stoich", "process", 1);
	/*
	Id meshEntry = Neutral::child( mesh.eref(), "mesh" );
	assert( meshEntry != Id() );
	*/
}

void SimManager::buildSmoldyn( Shell* shell )
{
}

void SimManager::buildGsl( const Eref& e, const Qinfo* q, 
	Shell* shell, const string& method )
{
	vector< int > dims( 1, 1 );
	 // This is a placeholder for more sophisticated node-balancing info
	 // May also need to put in volscales here.
	stoich_ = shell->doCreate( "Stoich", baseId_, "stoich", dims );
	Field< string >::set( stoich_, "path", baseId_.path() + "/kinetics/##");

	string basePath = baseId_.path();
	Id compt( basePath + "/kinetics" );
	assert( compt != Id() );

	MsgId mid = shell->doAddMsg( "Single", compt, "meshSplit", 
		stoich_, "meshSplit" );
	assert( mid != Msg::bad );

	double chemLoad = estimateChemLoad( compt, stoich_ );
	// Here we would also estimate cell load
	Id hsolver;
	double hsolveLoad = estimateHsolveLoad( hsolver );

	numChemNodes_ = Shell::numNodes() * chemLoad / ( chemLoad + hsolveLoad);
	
	nodeInfo()->send( e, q->threadNum(), numChemNodes_,
		Shell::numProcessThreads() ); 
	Qinfo::waitProcCycles( 2 );

	Id gsl = shell->doCreate( "GslIntegrator", stoich_, "gsl", dims );
	assert( gsl != Id() );
	bool ret = SetGet1< Id >::set( gsl, "stoich", stoich_ );
	assert( ret );
	ret = Field< bool >::get( gsl, "isInitialized" );
	assert( ret );
	ret = Field< string >::set( gsl, "method", method );
	assert( ret );
	string path0 = basePath + "/kinetics/mesh," + 
		basePath + "/kinetics/##[ISA=StimulusTable]";
	shell->doUseClock( path0, "process", 0);
	shell->doUseClock( basePath + "/stoich/gsl", "process", 1);

	Id meshEntry = Neutral::child( compt.eref(), "mesh" );
	assert( meshEntry != Id() );
	mid = shell->doAddMsg( "OneToOne", meshEntry, "remesh", gsl, "remesh" );
	assert( mid != Msg::bad );
}

void SimManager::buildFromKkitTree( const Eref& e, const Qinfo* q,
	const string& method )
{
	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	autoPlot_ = 0;
	vector< int > dims( 1, 1 );

	shell->doSetClock( 0, plotdt_ );
	shell->doSetClock( 1, plotdt_ );
	shell->doSetClock( 2, plotdt_ );
	shell->doSetClock( 3, 0 );

	string basePath = baseId_.path();
	if ( method == "Gillespie" || method == "gillespie" || 
		method == "GSSA" || method == "gssa" || method == "Gssa" ) {
		buildGssa( e, q, shell );
	} else if ( method == "Neutral" || method == "ee" || method == "EE" ) {
		buildEE( shell );
	} else if ( method == "Smoldyn" || method == "smoldyn" ) {
		buildSmoldyn( shell );
	} else {
		buildGsl( e, q, shell, method );
	}

	string plotpath = basePath + "/graphs/##[TYPE=Table]," + 
		basePath + "/moregraphs/##[TYPE=Table]";
	vector< Id > list;
	if ( wildcardFind( plotpath, list ) > 0 )
		shell->doUseClock( plotpath, "process", 2 );
	// shell->doReinit(); // Cannot use unless process is running.
}
