/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2011 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include <fstream>
#include <sstream>
#include "header.h"
#include "Interpol2D.h"

const unsigned int Interpol2D::MAX_DIVS = 100000;

static SrcFinfo1< double > trig( "trig",
	"respond to a request for a value lookup" );

const Cinfo* Interpol2D::initCinfo()
{
	///////////////////////////////////////////////////////
	// Shared message definitions
	///////////////////////////////////////////////////////

	static DestFinfo lookup( "lookup", 
		"Looks up table value based on indices v1 and v2, and sends"
		"value back using the 'trig' message",
		new EpFunc2< Interpol2D, double, double >( &Interpol2D::lookupReturn )
	);
	static Finfo* lookupReturnShared[] =
	{
		&trig, &lookup
	};

	static SharedFinfo lookupReturn2D( "lookupReturn2D",
		"This is a shared message for doing lookups on the table. "
		"Receives 2 doubles: x, y. "
		"Sends back a double with the looked-up z value.",
		lookupReturnShared, sizeof( lookupReturnShared ) / sizeof( Finfo * )
	);
	
	///////////////////////////////////////////////////////
	// Field definitions
	///////////////////////////////////////////////////////
	static ValueFinfo< Interpol2D, double > xmin( "xmin",
		"Minimum value for x axis of lookup table",
			&Interpol2D::setXmin,
			&Interpol2D::getXmin
		);
	static ValueFinfo< Interpol2D, double > xmax( "xmax",
		"Maximum value for x axis of lookup table",
			&Interpol2D::setXmax,
			&Interpol2D::getXmax
		);
	static ValueFinfo< Interpol2D, unsigned int > xdivs( "xdivs",
		"# of divisions on x axis of lookup table",
			&Interpol2D::setXdivs,
			&Interpol2D::getXdivs
		);
	static ValueFinfo< Interpol2D, double > dx( "dx",
		"Increment on x axis of lookup table",
			&Interpol2D::setDx,
			&Interpol2D::getDx
		);
	static ValueFinfo< Interpol2D, double > ymin( "ymin",
		"Minimum value for y axis of lookup table",
			&Interpol2D::setYmin,
			&Interpol2D::getYmin
		);
	static ValueFinfo< Interpol2D, double > ymax( "ymax",
		"Maximum value for y axis of lookup table",
			&Interpol2D::setYmax,
			&Interpol2D::getYmax
		);
	static ValueFinfo< Interpol2D, unsigned int > ydivs( "ydivs",
		"# of divisions on y axis of lookup table",
			&Interpol2D::setYdivs,
			&Interpol2D::getYdivs
		);
	static ValueFinfo< Interpol2D, double > dy( "dy",
		"Increment on y axis of lookup table",
			&Interpol2D::setDy,
			&Interpol2D::getDy
		);
	static LookupValueFinfo< Interpol2D, vector< unsigned int >, double > 
		table( "table",
		"Lookup an entry on the table",
			&Interpol2D::setTableValue,
			&Interpol2D::getTableValue
		);		
	static LookupValueFinfo< Interpol2D, vector< unsigned int >, double > 
		table2D( "table2D",
		"Lookup an entry on the table",
			&Interpol2D::setTableValue,
			&Interpol2D::getTableValue
		);
	static ValueFinfo< Interpol2D, vector< vector< double > > >
		tableVector2D( "tableVector2D",
		"Get the entire table.",
			&Interpol2D::setTableVector,
			&Interpol2D::getTableVector
		);
		
	///////////////////////////////////////////////////////
	// MsgSrc definitions
	///////////////////////////////////////////////////////

	///////////////////////////////////////////////////////
	// MsgDest definitions
	///////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////
	static Finfo* interpol2DFinfos[] =
	{
		&lookupReturn2D,	// Shared
		&xmin,				// Value
		&xmax,				// Value
		&xdivs,				// Value
		&dx,				// Value
		&ymin,				// Value
		&ymax,				// Value
		&ydivs,				// Value
		&dy,				// Value
		&table,				// Lookup
		&table2D,			// Lookup
		&tableVector2D,		// Value

	};

	static string doc[] =
	{
		"Name", "Interpol2D",
		"Author", "Niraj Dudani, 2009, NCBS",
		"Description", "Interpol2D: Interpolation class. "
				"Handles lookup from a 2-dimensional grid of real-numbered values. "
				"Returns 'z' value based on given 'x' and 'y' values. "
				"Can either use interpolation or roundoff to the nearest index.",
	};
	
	static Cinfo interpol2DCinfo(
		"Niraj Dudani NCBS",
		Neutral::initCinfo(),
		interpol2DFinfos, sizeof( interpol2DFinfos ) / sizeof( Finfo * ),
		new Dinfo< Interpol2D >()
	);

	return &interpol2DCinfo;
}

static const Cinfo* interpol2DCinfo = Interpol2D::initCinfo();

// static const Slot lookupReturnSlot = initInterpol2DCinfo()->getSlot( "lookupReturn2D.trig" );
////////////////////////////////////////////////////////////////////
// Constructors
////////////////////////////////////////////////////////////////////

Interpol2D::Interpol2D()
	: 	xmin_( 0.0 ), xmax_( 1.0 ), invDx_( 1.0 ),
		ymin_( 0.0 ), ymax_( 1.0 ), invDy_( 1.0 ),
		sy_( 1.0 )
{
	table_.resize( 2 );
	table_[ 0 ].resize( 2, 0.0 );
	table_[ 1 ].resize( 2, 0.0 );
}

Interpol2D::Interpol2D(
	unsigned int xdivs, double xmin, double xmax,
	unsigned int ydivs, double ymin, double ymax )
	: 	xmin_( xmin ), xmax_( xmax ),
		ymin_( ymin ), ymax_( ymax ),
		sy_( 1.0 )
{
	resize( xdivs+1, ydivs+1 );

	if ( !doubleEq( xmax_, xmin ) )
		invDx_ = xdivs / ( xmax_ - xmin_);
	else
		invDx_ = 1.0;
	
	if ( !doubleEq( ymax_, ymin ) )
		invDy_ = ydivs / ( ymax_ - ymin_);
	else
		invDy_ = 1.0;
}

////////////////////////////////////////////////////////////////////
// Here we set up Interpol2D value fields
////////////////////////////////////////////////////////////////////

/**
 * Resizes 2-D vector. If either argument is zero, it remains unchanged
 */
void Interpol2D::resize( unsigned int xsize, unsigned int ysize, double init )
{
	unsigned int oldx = table_.size();
	unsigned int oldy = 0;
	if ( oldx > 0 )
		oldy = table_[0].size();
	if ( xsize == 0 ) xsize = oldx;
	if ( ysize == 0 ) ysize = oldy;

	if ( xsize != oldx ) {
		table_.resize( xsize );
		if ( xsize > oldx ) {
			for ( unsigned int i = oldx; i < xsize; ++i )
				table_[i].resize( ysize, init );
		}
	}

	if ( ysize != oldy ) {
		for ( unsigned int i = 0; i < xsize; ++i )
			table_[i].resize( ysize, init );
	}
	invDx_ = xdivs() / ( xmax_ - xmin_ );
	invDy_ = ydivs() / ( ymax_ - ymin_ );
}

///////////////////////////////////////////////////////////////
// X dimension access funcs
///////////////////////////////////////////////////////////////

void Interpol2D::setXmin( double value ) {
	if ( !doubleApprox( xmax_, value ) ) {
		xmin_ = value;
		invDx_ = xdivs() / ( xmax_ - xmin_ );
	} else {
		cerr << "Error: Interpol2D::setXmin: Xmin ~= Xmax : Assignment failed\n";
	}
}

double Interpol2D::getXmin() const
{
	return xmin_;
}

void Interpol2D::setXmax( double value ) {
	if ( !doubleApprox( xmin_, value ) ) {
		xmax_ = value;
		invDx_ = xdivs() / ( xmax_ - xmin_ );
	} else {
		cerr << "Error: Interpol2D::setXmax: Xmin ~= Xmax : Assignment failed\n";
	}
}

double Interpol2D::getXmax() const
{
	return xmax_;
}

void Interpol2D::setXdivs( unsigned int value )
{
		resize( value + 1, 0 );
}

unsigned int Interpol2D::getXdivs( ) const {
	if ( table_.empty() )
		return 0;
	return table_.size() - 1;
}

void Interpol2D::setDx( double value ) {
	if ( !doubleEq( value, 0.0 ) ) {
		unsigned int xdivs = static_cast< unsigned int >( 
			0.5 + fabs( xmax_ - xmin_ ) / value );
		if ( xdivs < 1 || xdivs > MAX_DIVS ) {
			cerr <<
				"Error: Interpol2D::localSetDx Out of range:" <<
				xdivs + 1 << " entries in table.\n";
				return;
		}
		resize( xdivs + 1, 0 );
	}
}

double Interpol2D::getDx() const {
	if ( xdivs() == 0 )
		return 0.0;
	else
		return ( xmax_ - xmin_ ) / xdivs();
}

///////////////////////////////////////////////////////////////
// Y dimension access funcs
///////////////////////////////////////////////////////////////

void Interpol2D::setYmin( double value ) {
	if ( !doubleApprox( ymax_, value ) ) {
		ymin_ = value;
		invDy_ = ydivs() / ( ymax_ - ymin_ );
	} else {
		cerr << "Error: Interpol2D::setYmin: Ymin ~= Ymax : Assignment failed\n";
	}
}

double Interpol2D::getYmin() const
{
	return ymin_;
}

void Interpol2D::setYmax( double value ) {
	if ( !doubleApprox( ymin_, value ) ) {
		ymax_ = value;
		invDy_ = ydivs() / ( ymax_ - ymin_ );
	} else {
		cerr << "Error: Interpol2D::setYmax: Ymin ~= Ymax : Assignment failed\n";
	}
}

double Interpol2D::getYmax() const
{
	return ymax_;
}

void Interpol2D::setYdivs( unsigned int value ) {
	resize( 0, value + 1 );
}

unsigned int Interpol2D::getYdivs( ) const {
	if ( table_.size() > 0 )
		if ( table_[0].size() > 0 )
			return table_[0].size() - 1;
	return 0;
}

/**
 * \todo Later do interpolation etc to preserve contents.
 * \todo Later also check that it is OK for xmax_ < xmin_
 */
void Interpol2D::setDy( double value ) {
	if ( !doubleEq( value, 0.0 ) ) {
		unsigned int ydivs = static_cast< unsigned int >( 
			0.5 + fabs( ymax_ - ymin_ ) / value );
		if ( ydivs < 1 || ydivs > MAX_DIVS ) {
			cerr <<
				"Error: Interpol2D::localSetDy Out of range:" <<
				ydivs + 1 << " entries in table.\n";
				return;
		}
		
		setYdivs( ydivs );
		invDy_ = ydivs / ( ymax_ - ymin_ );
	}
}

double Interpol2D::getDy() const {
	if ( ydivs() == 0 )
		return 0.0;
	else
		return ( ymax_ - ymin_ ) / ydivs();
}

double Interpol2D::getSy() const 
{
	return sy_;
}

void Interpol2D::setSy( double value ) {
	if ( !doubleEq( value, 0.0 ) ) {
		double ratio = value / sy_;
		vector< vector< double > >::iterator i;
		vector< double >::iterator j;
		for ( i = table_.begin(); i != table_.end(); i++ )
			for ( j = i->begin(); j != i->end(); j++ )
				*j *= ratio;
		sy_ = value;
	} else {
		cerr << "Error: Interpol2D::localSetSy: sy too small:" <<
			value << "\n";
	}
}

vector< vector< double > > Interpol2D::getTableVector() const
{
	return table_;
}

void Interpol2D::setTableValue( vector< unsigned int > index, double value )
{
	assert( index.size() == 2 );
	unsigned int i0 = index[ 0 ];
	unsigned int i1 = index[ 1 ];
	
	if ( i0 < table_.size() && i1 < table_[ 0 ].size() )
		table_[ i0 ][ i1 ] = value;
	else
		cerr << "Error: Interpol2D::setTableValue: Index out of bounds!\n";
}

double Interpol2D::getTableValue( vector< unsigned int > index ) const
{
	assert( index.size() == 2 );
	unsigned int i0 = index[ 0 ];
	unsigned int i1 = index[ 1 ];
	
	if ( i0 < table_.size() && i1 < table_[ 0 ].size() )
		return table_[ i0 ][ i1 ];
	else {
		cerr << "Error: Interpol2D::getTableValue: Index out of bounds!\n";
		return 0.0;
	}
}

// This sets the whole thing up: values, xdivs, dx and so on. Only xmin
// and xmax are unknown to the input vector.
void Interpol2D::setTableVector( vector< vector< double > > value ) 
{
	table_ = value;
	invDx_ = ( table_.size() + 1) / ( xmax_ - xmin_ );
	invDy_ = ydivs() / ( ymax_ - ymin_ );
}

unsigned int Interpol2D::xdivs() const
{
	if ( table_.empty() )
		return 0;
	return table_.size() - 1;
}

unsigned int Interpol2D::ydivs() const
{
	if ( table_.empty() || table_[0].empty() )
		return 0;
	return table_[0].size() - 1;
}


////////////////////////////////////////////////////////////////////
// Here we set up Interpol2D Destination functions
////////////////////////////////////////////////////////////////////

/**
 * lookupReturn uses its argument to do an interpolating lookup of the
 * table. It sends a return message to the
 * originating object with the looked up value.
 */
void Interpol2D::lookupReturn( const Eref& e, const Qinfo*q, 
	double v1, double v2 )
{
	double ret = innerLookup( v1, v2 );
	trig.send( e, q->getProcInfo(), ret );
}

////////////////////////////////////////////////////////////////////
// Here we set up private Interpol2D class functions.
////////////////////////////////////////////////////////////////////


double Interpol2D::indexWithoutCheck( double x, double y ) const
{
	assert( table_.size() > 1 );
	
	unsigned long xInteger = static_cast< unsigned long >( ( x - xmin_ ) * invDx_ );
	assert( xInteger < table_.size() );
	
	unsigned long yInteger = static_cast< unsigned long >( ( y - ymin_ ) * invDy_ );
	assert( yInteger < table_[ 0 ].size() );
	
	return table_[ xInteger ][ yInteger ];
}

/**
 * Performs bi-linear interpolation, without bounds-checking.
 */
double Interpol2D::interpolateWithoutCheck( double x, double y ) const
{
	assert( table_.size() > 1 );
	
	double xv = ( x - xmin_ ) * invDx_;
	unsigned long xInteger = static_cast< unsigned long >( xv );
	double xFraction = xv - xInteger;
	assert( xInteger < table_.size() - 1 );
	
	double yv = ( y - ymin_ ) * invDy_;
	unsigned long yInteger = static_cast< unsigned long >( yv );
	double yFraction = yv - yInteger;
	assert( yInteger < table_[ 0 ].size() - 1 );
	
	/* The following is the same as:
			double z00 = table_[ xInteger ][ yInteger ];
			double z01 = table_[ xInteger ][ yInteger + 1 ];
			double z10 = table_[ xInteger + 1 ][ yInteger ];
			double z11 = table_[ xInteger + 1 ][ yInteger + 1 ];
	*/
	vector< vector< double > >::const_iterator iz0 = table_.begin() + xInteger;
	vector< double >::const_iterator iz00 = iz0->begin() + yInteger;
	vector< double >::const_iterator iz10 = ( iz0 + 1 )->begin() + yInteger;
	double z00 = *iz00;
	double z01 = *( iz00 + 1 );
	double z10 = *iz10;
	double z11 = *( iz10 + 1 );
	
	/* The following is the same as:
			return (
				z00 * ( 1 - xFraction ) * ( 1 - yFraction ) +
				z10 * xFraction * ( 1 - yFraction ) +
				z01 * ( 1 - xFraction ) * yFraction +
				z11 * xFraction * yFraction );
	*/
	double xFyF = xFraction * yFraction;
	return (
		z00 * ( 1 - xFraction - yFraction + xFyF ) +
		z10 * ( xFraction - xFyF ) +
		z01 * ( yFraction - xFyF ) +
		z11 * xFyF );
}

double Interpol2D::innerLookup( double x, double y ) const
{
	bool isOutOfBounds = false;
	
	if ( table_.size() == 0 )
		return 0.0;
	
	if ( x < xmin_ ) {
		x = xmin_;
		isOutOfBounds = true;
	}
	if ( x > xmax_ ) {
		x = xmax_;
		isOutOfBounds = true;
	}
	if ( y < ymin_ ) {
		y = ymin_;
		isOutOfBounds = true;
	}
	if ( y > ymax_ ) {
		y = ymax_;
		isOutOfBounds = true;
	}
	
	if ( isOutOfBounds )
		return indexWithoutCheck( x, y );
	else 
		return interpolateWithoutCheck( x, y );
}

bool Interpol2D::operator==( const Interpol2D& other ) const
{
	return (
		xmin_ == other.xmin_ &&
		xmax_ == other.xmax_ &&
		ymin_ == other.ymin_ &&
		ymax_ == other.ymax_ &&
		table_ == other.table_ );
}

bool Interpol2D::operator<( const Interpol2D& other ) const
{
	if ( table_.size() < other.table_.size() )
		return 1;
	
	if ( table_.size() > other.table_.size() )
		return 0;
	
	for ( size_t i = 0; i < table_.size(); i++ ) {
		for ( size_t j = 0; j < table_[ i ].size(); j++ ) {
			if ( table_[ i ][ j ] < other.table_[ i ][ j ] )
				return 1;
			if ( table_[ i ][ j ] > other.table_[ i ][ j ] )
				return 0;
		}
	}
	
	return 0;
}
// This sets the whole thing up: values, xdivs, dx and so on. Only xmin
// and xmax are unknown to the input vector.
void Interpol2D::appendTableVector(
	const vector< vector< double > >& value ) 
{
	if ( value.empty() )
		return;
	
	unsigned int ysize = value[ 0 ].size();
	vector< vector< double > >::const_iterator i;
	for ( i = value.begin() + 1; i != value.end(); i++ )
		if ( i->size() != ysize ) {
			ysize = ~0u;
			break;
		}
	
	if ( ysize == ~0u ) {
		cerr <<
			"Error: Interpol2D::localAppendTableVector: All rows should have a "
			"uniform width. Not changing anything.\n";
		return;
	}
	
	if ( ! table_.empty() && ysize != table_[ 0 ].size() ) {
		cerr <<
			"Error: Interpol2D: localAppendTableVector: Table widths must match. "
			"Not changing anything.\n";
		return;
	}
	
	table_.insert( table_.end(), value.begin(), value.end() );
	invDx_ = xdivs() / ( xmax_ - xmin_ );
}

void Interpol2D::print( const string& fname, bool appendFlag ) const
{
	std::ofstream fout;
	if ( appendFlag )
		fout.open( fname.c_str(), std::ios::app );
	else
		fout.open( fname.c_str(), std::ios::trunc );
	
	vector< vector< double > >::const_iterator i;
	vector< double >::const_iterator j;
	for ( i = table_.begin(); i != table_.end(); i++ ) {
		for ( j = i->begin(); j != i->end(); j++ )
			fout << *j << "\t";
		fout << "\n";
	}
	
	fout.close();
}

void Interpol2D::load( const string& fname, unsigned int skiplines )
{
	// Checking if xdivs/ydivs are different from default values. If they are,
	// then issue a warning.
	if ( xdivs() != 1 || ydivs() != 1 )
		cerr << "Warning: Interpol2D::innerLoad: Loading 2-D table from '" <<
			fname << "'. " <<
			"'xdivs' and 'ydivs' need not be specified. If you have set these fields, "
			"then they will be overridden while loading.\n";
	
	vector< double >::iterator i;
	std::ifstream fin( fname.c_str() );
	string line;
	if ( fin.good() ) {
		unsigned int i;
		for ( i = 0; i < skiplines; i++ ) {
			if ( fin.good () )
				getline( fin, line );
			else
				break;
		}
		if ( !fin.good() )
			return;
		
		table_.clear( );
		unsigned int lastWidth = ~0u;
		double y;
		while( fin.good() ) {
			table_.resize( table_.size() + 1 );
			
			getline( fin, line );
			istringstream sstream( line );
			while( sstream >> y )
				table_.back().push_back( y );
			
			/*
			 * In case the last line of a file is blank.
			 */
			if ( table_.back().empty() ) {
				table_.pop_back();
				break;
			}
			
			if ( lastWidth != ~0u &&
			     table_.back().size() != lastWidth )
			{
				cerr << "Error: Interpol2D::innerLoad: " <<
					"In file " << fname <<
					", line " << table_.size() <<
					", row widths are not uniform! Will stop loading now.\n";
				table_.clear();
				return;
			}
			
			lastWidth = table_.back().size();
		}
		
		invDx_ = xdivs() / ( xmax_ - xmin_ );
		invDy_ = ydivs() / ( ymax_ - ymin_ );
	} else {
		cerr << "Error: Interpol2D::innerLoad: Failed to open file " << 
			fname << endl;
	}
}

#ifdef DO_UNIT_TESTS
void testInterpol2D()
{
/*
	static const unsigned int XDIVS = 100;
	cout << "\nDoing Interpol2D tests";

	Element* i1 = interpol2DCinfo->create( Id::scratchId(), "i1" );
	Element* i2 = interpol2DCinfo->create( Id::scratchId(), "i2" );
	unsigned int i;
	double ret = 0;

	set< int >( i1, "xdivs", XDIVS );

	set< double >( i1, "xmin", 0 );
	get< double >( i1, "xmin", ret );
	ASSERT( ret == 0.0, "testInterpol2D" );

	set< double >( i1, "xmax", 20 );
	get< double >( i1, "xmax", ret );
	ASSERT( ret == 20.0, "testInterpol2D" );

	for ( i = 0; i <= XDIVS; i++ )
		lookupSet< double, unsigned int >(
						i1, "table", i * 10.0 - 100.0, i );

	for ( i = 0; i <= XDIVS; i++ ) {
		lookupGet< double, unsigned int >( i1, "table", ret, i );
		assert ( ret == i * 10.0 - 100.0 );
	}
	cout << ".";

	set< int >( i2, "xdivs", XDIVS );
	set< double >( i2, "xmin", 0 );
	set< double >( i2, "xmax", 10000.0 );

	// Here we use i2 as a dummy dest for the 
	// lookup operation, which takes place on i1.

	ASSERT(
		Eref( i1 ).add( "lookupSrc", i2, "xmin" ), "connecting interpol2Ds" );

		//	i1->findFinfo( "lookupSrc" )->add( i1, i2, i2->findFinfo( "xmin" ) ), "connecting interpol2Ds"

	set< double >( i1, "lookup", -10.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == -100.0, "Lookup minimum" );

	set< double >( i1, "lookup", 0.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == -100.0, "Lookup minimum" );

	set< double >( i1, "lookup", 2.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == 0.0, "Lookup middle" );

	set< double >( i1, "lookup", 2.1 );
	get< double >( i2, "xmin", ret );
	ASSERT( fabs( ret - 5.0 ) < 1.0e-10, "Lookup interpolation" );
	// ASSERT( ret == 5.0, "Lookup interpolation" );

	set< double >( i1, "lookup", 10.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == 400.0, "Lookup middle" );

	set< double >( i1, "lookup", 12.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == 500.0, "Lookup middle" );

	set< double >( i1, "lookup", 20.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == 900.0, "Lookup max" );

	set< double >( i1, "lookup", 20000.0 );
	get< double >( i2, "xmin", ret );
	ASSERT( ret == 900.0, "Lookup max" );
*/
}

#endif // DO_UNIT_TESTS