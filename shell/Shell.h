/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2007 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/
#ifndef _SHELL_H
#define _SHELL_H

// forward declaration
class SimDump;

class Shell
{
#ifdef DO_UNIT_TESTS
	friend void testShell();
#ifdef USE_MPI
	friend void testShellSetupAsyncParMsg();
#endif // USE_MPI
#endif // DO_UNIT_TESTS
	public:
		Shell();
	
////////////////////////////////////////////////////////////////////
// Some utility functions for path management.
// These need to be here in Shell for two reasons. First, the shell
// holds the current working element, cwe. Second, many functions are
// multinode and the Shell has to handle queries across nodes.
////////////////////////////////////////////////////////////////////
		// string expandPath( const std::string& path ) const;
		static string eid2path( Id eid );
		static Id path2eid( const string& path, const string& separator );
		Id innerPath2eid( const string& path, const string& separator ) const;
		static Id parent( Id eid );
		static Id traversePath( Id start, vector< string >& );
		static string head( const string& path, const string& separator );
		static string tail( const string& path, const string& separator );

		void digestPath( string& path );

//////////////////////////////////////////////////////////////////////
// Special low-level operations that Shell handles using raw
// serialized strings from PostMaster.
//////////////////////////////////////////////////////////////////////
		static void rawAddFunc( const Conn* c, string s );
		static void rawCopyFunc( const Conn* c, string s );
		static void rawTestFunc( const Conn* c, string s );
//////////////////////////////////////////////////////////////////////
// Infinite loop called on slave nodes to monitor commands from master.
//////////////////////////////////////////////////////////////////////
		static void pollFunc( const Conn* c );
////////////////////////////////////////////////////////////////////
// Local functions for implementing basic GENESIS/MOOSE command set.
////////////////////////////////////////////////////////////////////

		static void setCwe( const Conn*, Id id );
		static Id getCwe( const Element* );
		static void trigCwe( const Conn* );
		static void pushe( const Conn*, Id id );
		static void pope( const Conn* );

		static void trigLe( const Conn*, Id parent );

		bool create( const string& type, const string& name,
						Id parent, Id id );
		bool createArray( const string& type, const string& name,
						Id parent, Id id, int n );
		void destroy( Id victim );

		/**
		 * This function creates an object, generating its
		 * own Id. It may decide it should create the object on a
		 * remote node.
		 */
		static void staticCreate( const Conn*, string type,
						string name, Id parent );
		static void staticCreateArray1( const Conn*, string type,
						string name, Id parent, vector <double> parameter );
		static void staticCreateArray( const Conn*, string type,
						string name, Id parent, vector <double> parameter );
		static void planarconnect( const Conn* c, string source, string dest, double probability);
		static void planardelay(const Conn& c, string source, string destination, vector <double> parameter);
		static void planarweight(const Conn& c, string source, string  destination, vector <double> parameter);
		static void getSynCount2(const Conn* c, Id dest);
		
		static void staticDestroy( const Conn*, Id victim );

		static void getField( const Conn* c, Id id, string field );
		static void addField( const Conn* c, Id id, string fieldname );
		static void setField( const Conn* c, 
						Id id, string field, string value );
		static void setVecField( const Conn* c, 
				vector< Id > elist, string field, string value );

		static void setClock( const Conn* c, int clockNo, double dt,
				int stage );
		static void useClock( const Conn* c,
			Id tickId, vector< Id > path, string function );

		static void getWildcardList( const Conn* c,
						string path, bool ordered );


		/**
		 * addMessage creates a message between two Ids, which could
		 * also represent array elements.
		 */
		static void addMessage( const Conn* c,
			vector< Id >src, string srcField, vector< Id >dest, 
			string destField );
		static bool addSingleMessage( const Conn* c,
			Id src, string srcField, Id dest, string destField );
		/** 
		 * This one does the actual work for adding messages on the
		 * local node
		 */
		static bool addLocal(
			Id src, string srcField,
			Id dest, string destField );

		/**
		 * This is a more general form of the addMessage command,
		 * as it lets the user specify the connection type. Currently
		 * not available in the GENESIS parser.
		 */
		static void addEdge( const Conn* c,
			Fid src, Fid dest, int connType );

		/**
		 * deleteMessage gets rid of the message identified by the Fid and
		 * the integer lookup for it.
		 */
		static void deleteMessage( const Conn* c, Fid src, int msg );

		/**
		 * deleteEdge gets rid of messages specified as edges, that is,
		 * using the same src/field and dest/field info that was used
		 * to create the message.
		 */
		static void deleteMessageByDest( const Conn* c,
			Id src, string srcField, Id dest, string destField );
		static void deleteEdge( const Conn* c, Fid src, Fid dest );

		/**
 		* listMessages builds a list of messages associated with the 
 		* specified element on the named field, and sends it back to
 		* the calling parser. It extracts the
 		* target element from the connections, and puts this into a
 		* vector of unsigned ints.
 		*/
		static void listMessages( const Conn* c,
				Id id, string field, bool isIncoming );

		static void copy( const Conn* c, Id src, Id parent, string name );
		static void copyIntoArray( const Conn* c, Id src, Id parent, string name, vector <double> parameter );
		static void copyIntoArray1( const Conn* c, Id src, Id parent, string name, vector <double> parameter );
		static void move( const Conn* c, Id src, Id parent, string name );
		
		//////////////////////////////////////////////////////////
		// Some stuff for managing scheduling and simulation runs
		//////////////////////////////////////////////////////////
		static void resched( const Conn* c );
		static void reinit( const Conn* c );
		static void stop( const Conn* c );
		static void step( const Conn* c, double time );
		static void requestClocks( const Conn* c );
		static void requestCurrTime( const Conn* c );

		//////////////////////////////////////////////////////////
		// Major input functions.
		//////////////////////////////////////////////////////////
		static void readCell( 
					const Conn* c, string filename, string cellpath,
					vector< double > globalParms );

		//////////////////////////////////////////////////////////
		// Channel setup functions.
		//////////////////////////////////////////////////////////
		static void setupAlpha( const Conn* c, Id gate,
				vector< double > parms );
		static void setupTau( const Conn* c, Id gate,
				vector< double > parms );
		static void tweakAlpha( const Conn* c, Id gateId );
		static void tweakTau( const Conn* c, Id gateId );
		static void setupGate( const Conn* c, Id gate,
				vector< double > parms );

		//////////////////////////////////////////////////////////
		// SimDump functions
		//////////////////////////////////////////////////////////
		static void readDumpFile( const Conn* c, string filename );
		static void writeDumpFile( const Conn* c, 
			string filename, string path );
		static void simObjDump( const Conn* c, string fields );
		static void simUndump( const Conn* c, string args );
		static void openFile( const Conn* c, string filename, string mode );
		static void closeFile( const Conn* c, string filename );
		static void writeFile( const Conn* c, string filename, string text );
		static void readFile( const Conn* c, string filename, bool linemode );
		static void listFiles( const Conn* c );
		static void loadtab( const Conn* c, string data );
		void innerLoadTab( const string& data );

		//////////////////////////////////////////////////////////
		// Table special functions
		//////////////////////////////////////////////////////////
		static void tabop( const Conn* c, Id tab, char op, double min, 
			double max );
		static void file2tab( const Conn& c, 
				Id id, string filename, unsigned int skiplines );
			/*
		void add( const string& src, const string& dest );
		void drop( const string& src, const string& dest );
		void set( const string& field, const string& value );
		void move( const string& src, const string& dest );
		void copy( const string& src, const string& dest );
		void copyShallow( 
			const string& src, const string& dest );
		void copyHalo( const string& src, const string& dest );
		*/
		/*
		void listCommands( );
		void listClasses( );
		void echo( vector< string >& s, int options );
		void remoteCommand( string arglist );
		void command( int argc, const char** argv );
		*/

		////////////////////////////////////////////////////////////////////
		// functions for implementing inter-node shell-shell messaging
		////////////////////////////////////////////////////////////////////
		static void addParallelSrc( const Conn* c, unsigned int srcNode,
			Id src, string srcField, Id dest, string destField );
		static void addParallelDest( const Conn* c,
			Id src, string srcTypeStr, Id dest, string destField );

		static void parGetField( const Conn* c, Id id, string field );
		static void recvGetRequest( const Conn* c, string value );
		// setField is a regular command, no special stuff for it.

		static void parCreateFunc( const Conn* c,
			string objtype, string objname, 
			Id parentId, Id newObjId );
		
		static void parMsgErrorFunc( const Conn* c, 
			string errMsg, Id src, Id dest );
		static void parMsgOkFunc( const Conn* c, Id src, Id dest );

		/**
		 * Returns the Eref for the specified postmaster.
		 */
		Eref getPost( unsigned int node ) const;
	
	private:
		/// Current working element
		Id cwe_;
		/// Most recently created element
		Id recentElement_;
		vector< Id > workingElementStack_;
		// True if prompts etc are to be printed.
		///stores all the filehandles create by genesis style format.
		static map <string, FILE*> filehandler;
		// variable for file handling
		static vector <string> filenames;
		static vector <string> modes;
		static vector <FILE*> filehandles;
		
		int isInteractive_;
		string parser_;
		SimDump* simDump_;
		Id lastTab_; // Used for the loadtab -continue option, which 
			// contines loading numbers into the previously selected table.
		unsigned int node_;

		/**
		 * This keeps track of message requests sent off-node. 
		 * When the target node finishes its work it tells this one that
		 * things worked out. At reset time we examine this map to make
		 * sure everything has been cleared up.
		 * We can't do an immediate return because
		 * it is slow, and quite likely to give rise to race conditions.
		 */
		map< Id, Id > parMessagePending_;

		/**
		 * The postmaster element is used to set up all parallel models.
		 * In single-node simulations this is zero.
		 */
		Element* post_;
};

extern const Cinfo* initShellCinfo();

#endif // _SHELL_H
