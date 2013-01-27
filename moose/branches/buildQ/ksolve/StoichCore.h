/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _STOICH_CORE_H
#define _STOICH_CORE_H

class StoichCore
{
	public: 
		StoichCore( bool isMaster = false );
		~StoichCore();

		//////////////////////////////////////////////////////////////////
		// Field assignment stuff
		//////////////////////////////////////////////////////////////////

		void setOneWay( bool v );
		bool getOneWay() const;

		/// Returns number of local pools that are updated by solver
		unsigned int getNumVarPools() const;

		/**
		 *  Returns total number of local pools. Leaves out the pools whose
		 *  actual calculations happen on another solver, but are given a
		 *  proxy here in order to handle cross-compartment reactions.
		 */
		unsigned int getNumAllPools() const;

		/**
		 * Returns number of proxy pools here for
		 * cross-compartment reactions
		 */
		unsigned int getNumProxyPools() const;

		void setPath( const Eref& e, SolverBase* sb, string v );
		string getPath( const Eref& e, const Qinfo* q ) const;

		// unsigned int getNumMeshEntries() const;
		double getEstimatedDt() const;

		/**
		 * Utility function to return # of rates_ entries. This includes
		 * the cross-solver reactions.
		 */
		unsigned int getNumRates() const;

		/**
		 * Utility function to return # of core rates for reacs which are
		 * entirely located on current compartment, including all reactants
		 */
		unsigned int getNumCoreRates() const;

		/// Utility function to return a rates_ entry
		const RateTerm* rates( unsigned int i ) const;

		//////////////////////////////////////////////////////////////////
		// Model traversal and building functions
		//////////////////////////////////////////////////////////////////
		/**
		 * Scans through elist to find any reactions that connect to
		 * pools not located on solver. Removes these reactions from the
		 * elist and maintains Ids of the affected reactions, and their 
		 * off-solver pools, in offSolverReacs_ and offSolverPools_.
		 */
		void locateOffSolverReacs( Id myCompt, vector< Id >& elist );

		/**
		 * Builds the objMap vector, which maps all Ids to 
		 * the internal indices for pools and reacs that are used in the
		 * solver. In addition to the elist, it also scans through the
		 * offSolverPools and offSolverReacs to build the map.
		 */
		void allocateObjMap( const vector< Id >& elist );

		/// Using the computed array sizes, now allocate space for them.
		void resizeArrays();
		/// Identifies and allocates objects in the Stoich.
		void allocateModelObject( 
				Id id, vector< Id >& bufPools, vector< Id >& funcPools );
		/// Calculate sizes of all arrays, and allocate them.
		void allocateModel( const vector< Id >& elist );

		/**
		 * zombifyModel marches through the specified id list and 
		 * converts all entries into zombies. The first arg e is the
		 * Eref of the Stoich itself.
		 */
		void zombifyModel( const Eref& e, const vector< Id >& elist );

		/**
		 * Converts back to ExpEuler type basic kinetic Elements.
		 */
		void unZombifyModel();

		/// unZombifies Pools. Helper for unZombifyModel.
		void unZombifyPools();
		/// unZombifies Funcs. Helper for unZombifyModel.
		void unZombifyFuncs();

		void zombifyChemMesh( Id compt );

		unsigned int convertIdToReacIndex( Id id ) const;
		unsigned int convertIdToPoolIndex( Id id ) const;
		unsigned int convertIdToFuncIndex( Id id ) const;
		// unsigned int convertIdToComptIndex( Id id ) const;

		/**
		 * This takes the specified forward and reverse half-reacs belonging
		 * to the specified Reac, and builds them into the Stoich.
		 */
		void installReaction( ZeroOrder* forward, ZeroOrder* reverse, Id reacId );
		/**
		 * This takes the baseclass for an MMEnzyme and builds the
		 * MMenz into the Stoich.
		 */
		void installMMenz( MMEnzymeBase* meb, unsigned int rateIndex,
			const vector< Id >& subs, const vector< Id >& prds );

		/**
		 * This takes the forward, backward and product formation half-reacs
		 * belonging to the specified Enzyme, and builds them into the
		 * Stoich
		 */
		void installEnzyme( ZeroOrder* r1, ZeroOrder* r2, ZeroOrder* r3,
			Id enzId, Id enzMolId, const vector< Id >& prds );

		/**
		 * This installs a funcTerm. Should be generic, that is, work
		 * for any form of func. The pool is the FuncPool being
		 * controlled.
		 */
		void installAndUnschedFunc( Id func, Id pool );

		/**
		 * This extracts the map of all pools that diffuse.
		 */
		void buildDiffTerms( map< string, unsigned int >& diffTerms ) const;
		//////////////////////////////////////////////////////////////////
		/**
		 * Returns diffusion rate of specified pool
		 */
		double getDiffConst( unsigned int poolIndex ) const;

		/**
		 * Assigns diffusion rate of specified pool
		 */
		void setDiffConst( unsigned int poolIndex, double d );

		/**
		 * Returns SpeciesId of specified pool
		 */
		SpeciesId getSpecies( unsigned int poolIndex ) const;

		/**
		 * Assigns SpeciesId of specified pool
		 */
		void setSpecies( unsigned int poolIndex, SpeciesId s );

		/**
		 * Sets the forward rate v (given in millimoloar concentration units)
		 * for the specified reaction throughout the compartment in which the
		 * reaction lives. Internally the stoich uses #/voxel units so this 
		 * involves querying the volume subsystem about volumes for each
		 * voxel, and scaling accordingly.
		 */
		void setReacKf( const Eref& e, double v ) const;

		/**
		 * Sets the reverse rate v (given in millimoloar concentration units)
		 * for the specified reaction throughout the compartment in which the
		 * reaction lives. Internally the stoich uses #/voxel units so this 
		 * involves querying the volume subsystem about volumes for each
		 * voxel, and scaling accordingly.
		 */
		void setReacKb( const Eref& e, double v ) const;

		/**
		 * Sets the Km for MMenz, using appropriate volume conversion to
		 * go from the argument (in millimolar) to #/voxel.
		 * This may do the assignment among many voxels containing the enz
		 * in case there are different volumes.
		 */
		void setMMenzKm( const Eref& e, double v ) const;

		/**
		 * Sets the kcat for MMenz. No conversions needed.
		 */
		void setMMenzKcat( const Eref& e, double v ) const;

		/**
		 * Sets the rate v (given in millimoloar concentration units)
		 * for the forward enzyme reaction of binding substrate to enzyme.
		 * Does this throughout the compartment in which the
		 * enzyme lives. Internally the stoich uses #/voxel units so this 
		 * involves querying the volume subsystem about volumes for each
		 * voxel, and scaling accordingly.
		 */
		void setEnzK1( const Eref& e, double v ) const;

		/// Set rate k2 (1/sec) for enzyme
		void setEnzK2( const Eref& e, double v ) const;
		/// Set rate k3 (1/sec) for enzyme
		void setEnzK3( const Eref& e, double v ) const;

		/**
		 * Returns the internal rate in #/voxel, for R1, for the specified
		 * Eref.
		 */
		double getR1( const Eref& e ) const;
		/**
		 * Returns internal rate R1 in #/voxel, for the rate term 
		 * following the one directly referred to by the Eref e. Enzymes
		 * define multiple successive rate terms, so we look up the first,
		 * and then select one after it.
		 */
		double getR1offset1( const Eref& e ) const;
		/**
		 * Returns internal rate R1 in #/voxel, for the rate term 
		 * two after the one directly referred to by the Eref e. Enzymes
		 * define multiple successive rate terms, so we look up the first,
		 * and then select the second one after it.
		 */
		double getR1offset2( const Eref& e ) const;

		/**
		 * Returns the internal rate in #/voxel, for R2, for the specified
		 * reacIndex and voxel index. In some cases R2 is undefined, and it
		 * then returns 0.
		 */
		double getR2( const Eref& e ) const;

		/// Utility function, prints out N_, used for debugging
		void print() const;
		//////////////////////////////////////////////////////////////////
		// Utility funcs for numeric calculations
		//////////////////////////////////////////////////////////////////

		/// Updates the yprime array, rate of change of each molecule
		void updateRates( const double* s, double* yprime );

		/// Updates the function values, within s.
		void updateFuncs( double* s, double t );

		/// Updates the rates for cross-compartment reactions.
		void updateJunctionRates( const double* s,
			   const vector< unsigned int >& reacTerms, double* yprime );
		//////////////////////////////////////////////////////////////////
		// Access functions for cross-node reactions.
		//////////////////////////////////////////////////////////////////
		const vector< Id >& getOffSolverPools() const;

		/**
		 * List all the compartments to which the current compt is coupled,
		 * by cross-compartment reactions. Self not included.
		 */
		vector< Id > getOffSolverCompts() const;

		/**
		 * Looks up the vector of pool Ids originating from the specified
		 * compartment, that are represented on this compartment as proxies.
		 */
		const vector< Id >& offSolverPoolMap( Id compt ) const;

		/**
		 * Spawns a near-clone of the current StoichCore, with the rates_
		 * vector trimmed to include only those reactions that apply to the
		 * specified grouping of compartments. Used to generate the correct
		 * reaction set for a voxel at a particular junction between 
		 * compartments.
		 */
		StoichCore* spawn( const vector< Id >& compts ) const;
		
		//////////////////////////////////////////////////////////////////
		static const unsigned int PoolIsNotOnSolver;
		static const Cinfo* initCinfo();
	protected:
		bool isMaster_;
		bool useOneWay_;
		string path_;
		Id stoichId_;

		/**
		 * vector of initial concentrations, one per Pool of any kind
		vector< double > initConcs_;
		 */

		/**
		 * Vector of diffusion constants, one per VarPool.
		 */
		vector< double > diffConst_;


		/**
		 * Lookup from each molecule to its Species identifer
		 * This will eventually be tied into an ontology reference.
		 */
		vector< SpeciesId > species_;

		/// The RateTerms handle the update operations for reaction rate v_
		vector< RateTerm* > rates_;

		/// The FuncTerms handle mathematical ops on mol levels.
		vector< FuncTerm* > funcs_;

		/// N_ is the stoichiometry matrix.
		KinSparseMatrix N_;

		/**
		 * Maps Ids to objects in the S_, RateTerm, and FuncTerm vectors.
		 * There will be holes in this map, but look up is very fast.
		 * The calling Id must know what it wants to find: all it
		 * gets back is an integer.
		 * The alternative is to have multiple maps, but that is slower.
		 * Assume no arrays. Each Pool/reac etc must be a unique
		 * Element. Later we'll deal with diffusion.
		 */
		vector< unsigned int > objMap_;
		/**
		 * Minor efficiency: We will usually have a set of objects that are
		 * nearly contiguous in the map. May as well start with the first of
		 * them.
		 */
		unsigned int objMapStart_;

		/**
		 * Tracks the reactions that go off the current solver.
		 */
		vector< Id > offSolverReacs_;

		/**
		 * Which compartment(s) does the off solver reac connect to?
		 * Usually it is just one, in which case the second id is Id()
		 */
		vector< pair< Id, Id > > offSolverReacCompts_;

		/**
		 * Offset of the first offSolverPool in the idMap_ vector.
		 */
		unsigned int offSolverPoolOffset_;

		/**
		 * These are pools that were not in the original scope of the 
		 * solver, but have to be brought in because they are reactants
		 * of one or more of the offSolverReacs.
		 */
		vector< Id > offSolverPools_;

		/**
		 * Map of vectors of Ids of offSolver pools. 
		 * Each map entry contains the vector of Ids of proxy pools 
		 * coming from the specified compartment.
		 * In use, the junction will copy the pool indices over for 
		 * data transfer.
		 */
		map< Id, vector< Id > > offSolverPoolMap_; 

		/**
		 * The number of core rates, that is, rates for reactions which
		 * are entirely contained within the compartment. This is also the
		 * offset of the first offSolverReac in the rates_ vector.
		 */
		unsigned int numCoreRates_;

		/**
		 * Map back from mol index to Id. Primarily for debugging.
		 */
		vector< Id > idMap_;

		/**
		 * Map back from reac index to Id. Needed to unzombify
		 */
		vector< Id > reacMap_;

		/**
		 * Map back from enz index to Id. Needed to unzombify
		 */
		vector< Id > enzMap_;

		/**
		 * Map back from enz index to Id. Needed to unzombify
		 */
		vector< Id > mmEnzMap_;
		
		/**
		 * Number of variable molecules that the solver deals with.
		 *
		 */
		unsigned int numVarPools_;
		unsigned int numVarPoolsBytes_;
		/**
		 * Number of buffered molecules
		 */
		unsigned int numBufPools_;
		/**
		 * Number of molecules whose values are computed by functions
		 */
		unsigned int numFuncPools_;

		/**
		 * Number of reactions in the solver model. This includes 
		 * conversion reactions A + B <---> C
		 * enzyme reactions E + S <---> E.S ---> E + P
		 * and MM enzyme reactions rate = E.S.kcat / ( Km + S )
		 * The enzyme reactions count as two reaction steps.
		 */
		unsigned int numReac_;
};

#endif	// _STOICH_CORE_H
