/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2006 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _SPARSE_MATRIX_H
#define _SPARSE_MATRIX_H

/**
 * Template for specialized SparseMatrix. Used both for the Kinetic
 * solver and for handling certain kinds of messages. Speciality is that
 * it can extract entire rows efficiently, for marching through a 
 * specified row for a matrix multiplication or for traversing messages.
 *
 * Requires that type T have an equality operator ==
 */

extern const unsigned int SM_MAX_ROWS;
extern const unsigned int SM_MAX_COLUMNS;
extern const unsigned int SM_RESERVE;
// extern unsigned int rowIndex( const Element* e, const DataId& d );

template< class T > class Triplet {
	public:
		Triplet( T a, unsigned int b, unsigned int c )
			: a_( a ), b_( b ), c_( c )
		{;}
		bool operator< ( const Triplet< T >& other ) const {
			return ( c_ < other.c_ );
		}
		T a_;
		unsigned int b_;
		unsigned int c_;
};

typedef vector< class T >::const_iterator constTypeIter;
template < class T > class SparseMatrix
{
	public:
		SparseMatrix()
			: nrows_( 0 ), ncolumns_( 0 )
		{
			N_.resize( 0 );
			N_.reserve( SM_RESERVE );
			colIndex_.resize( 0 );
			colIndex_.reserve( SM_RESERVE );
		}

		SparseMatrix( unsigned int nrows, unsigned int ncolumns )
		{
			setSize( nrows, ncolumns );
		}

		/**
		 * Should be called only at the start. Subsequent resizing destroys
		 * the contents.
		 */
		void setSize( unsigned int nrows, unsigned int ncolumns ) {
			if ( nrows < SM_MAX_ROWS && ncolumns < SM_MAX_COLUMNS ) {
				N_.resize( 0 );
				N_.reserve( 2 * nrows );
				nrows_ = nrows;
				ncolumns_ = ncolumns;
				rowStart_.resize( nrows + 1, 0 );
				colIndex_.resize( 0 );
				colIndex_.reserve( 2 * nrows );
			} else {
				cerr << "Error: SparseMatrix::setSize( " <<
				nrows << ", " << ncolumns << ") out of range: ( " <<
				SM_MAX_ROWS << ", " << SM_MAX_COLUMNS << ")\n";
			}
		}

		/**
		 * Assigns and if necessary adds an entry in the matrix. 
		 * This variant does NOT remove any existing entry.
		 */
		void set( unsigned int row, unsigned int column, T value )
		{
			vector< unsigned int >::iterator i;
			vector< unsigned int >::iterator begin = 
				colIndex_.begin() + rowStart_[ row ];
			vector< unsigned int >::iterator end = 
				colIndex_.begin() + rowStart_[ row + 1 ];
		
			if ( begin == end ) { // Entire row was empty.
				unsigned long offset = begin - colIndex_.begin();
				colIndex_.insert( colIndex_.begin() + offset, column );
				N_.insert( N_.begin() + offset, value );
				for ( unsigned int j = row + 1; j <= nrows_; j++ )
					rowStart_[ j ]++;
				return;
			}
		
			if ( column > *( end - 1 ) ) { // add entry at end of row.
				unsigned long offset = end - colIndex_.begin();
				colIndex_.insert( colIndex_.begin() + offset, column );
				N_.insert( N_.begin() + offset, value );
				for ( unsigned int j = row + 1; j <= nrows_; j++ )
					rowStart_[ j ]++;
				return;
			}
			for ( i = begin; i != end; i++ ) {
				if ( *i == column ) { // Found desired entry. By defn it is nonzero.
					N_[ i - colIndex_.begin()] = value;
					return;
				} else if ( *i > column ) { // Desired entry is blank.
					unsigned long offset = i - colIndex_.begin();
					colIndex_.insert( colIndex_.begin() + offset, column );
					N_.insert( N_.begin() + offset, value );
					for ( unsigned int j = row + 1; j <= nrows_; j++ )
						rowStart_[ j ]++;
					return;
				}
			}
		}

		/**
		 * Removes specified entry.
		 */
		void unset( unsigned int row, unsigned int column )
		{
			vector< unsigned int >::iterator i;
			vector< unsigned int >::iterator begin = 
				colIndex_.begin() + rowStart_[ row ];
			vector< unsigned int >::iterator end = 
				colIndex_.begin() + rowStart_[ row + 1 ];
		
			if ( begin == end ) { // Entire row was empty. Ignore
				return;
			}
		
			if ( column > *( end - 1 ) ) { // End of row. Ignore
				return;
			}
			for ( i = begin; i != end; i++ ) {
				if ( *i == column ) { // Found desired entry. Zap it.
					unsigned long offset = i - colIndex_.begin();
					colIndex_.erase( i );
					N_.erase( N_.begin() + offset );
					for ( unsigned int j = row + 1; j <= nrows_; j++ )
						rowStart_[ j ]--;
					return;
				} else if ( *i > column ) { //Desired entry is blank. Ignore
					return;
				}
			}
		}

		/**
		 * Returns the entry identified by row, column. Returns T(0)
		 * if not found
		 */
		T get( unsigned int row, unsigned int column ) const
		{
			assert( row < nrows_ && column < ncolumns_ );
			vector< unsigned int >::const_iterator i;
			vector< unsigned int >::const_iterator begin = 
				colIndex_.begin() + rowStart_[ row ];
			vector< unsigned int >::const_iterator end = 
				colIndex_.begin() + rowStart_[ row + 1 ];
		
			i = find( begin, end, column );
			if ( i == end ) { // most common situation for a sparse Stoich matrix.
				return 0;
			} else {
				return N_[ rowStart_[row] + (i - begin) ];
			}
		}

		/**
		 * Used to get an entire row of entries. 
		 * Returns # entries.
		 * Passes back iterators for the row and for the column index.
		 *
		 * Ideally I should provide a foreach type function so that the
		 * user passes in their operation as a functor, and it is 
		 * applied to the entire row.
		 *
		 */
		unsigned int getRow( unsigned int row, 
			const T** entry, const unsigned int** colIndex ) const
		{
			if ( row >= nrows_ )
				return 0;
			unsigned int rs = rowStart_[row];
			if ( rs >= N_.size() )
				return 0;			
			*entry = &( N_[ rs ] );
			*colIndex = &( colIndex_[rs] );
			return rowStart_[row + 1] - rs;
		}

		/**
		 * This is an unnatural lookup here, across the grain of the
		 * sparse matrix.
		 * Ideally should use copy_if, but the C++ chaps forgot it.
		 */
		unsigned int getColumn( unsigned int col, 
			vector< T >& entry, 
			vector< unsigned int >& rowIndex ) const
		{
			entry.resize( 0 );
			rowIndex.resize( 0 );

			unsigned int row = 0;
			for ( unsigned int i = 0; i < N_.size(); ++i ) {
				if ( col == colIndex_[i] ) {
					entry.push_back( N_[i] );
					while ( rowStart_[ row + 1 ] <= i )
						row++;
					rowIndex.push_back( row );
				}
			}
			return entry.size();
		}

		void rowOperation( unsigned int row, unary_function< T, void>& f )
		{
			assert( row < nrows_ );

			constTypeIter i;
			// vector< T >::const_iterator i;
			unsigned int rs = rowStart_[row];
			vector< unsigned int >::const_iterator j = colIndex_.begin() + rs;
			// vector< T >::const_iterator end = 
			constTypeIter end = 
				N_.begin() + rowStart_[ row + 1 ];

			// for_each 
			for ( i = N_.begin() + rs; i != end; ++i )
				f( *i );
		}

		unsigned int nRows() const {
			return nrows_;
		}

		unsigned int nColumns() const {
			return ncolumns_;
		}

		unsigned int nEntries() const {
			return N_.size();
		}

		void clear() {
			N_.resize( 0 );
			colIndex_.resize( 0 );
			assert( rowStart_.size() == (nrows_ + 1) );
			rowStart_.assign( nrows_ + 1, 0 );
		}

		/**
		 * Adds a row to the sparse matrix, must go strictly in row order.
		 */
		void addRow( unsigned int rowNum, const vector< T >& row ) {
			assert( rowNum < nrows_ );
			assert( rowStart_.size() == (nrows_ + 1 ) );
			assert( N_.size() == colIndex_.size() );
			for ( unsigned int i = 0; i < ncolumns_; ++i ) {
				if ( row[i] != T( ~0 ) ) {
					N_.push_back( row[i] );
					colIndex_.push_back( i );
				}
			}
			rowStart_[rowNum + 1] = N_.size();
		}

		/**
		 * Used to set an entire row of entries, already in sparse form.
		 * Assumes that the SparseMatrix has been suitably allocated.
		 * rowNum must be done in increasing order in successive calls.
		 */
		void addRow( unsigned int rowNum, 
			const vector < T >& entry, 
			const vector< unsigned int >& colIndexArg )
		{
			assert( rowNum < nrows_ );
			assert( rowStart_.size() == (nrows_ + 1 ) );
			assert( rowStart_[ rowNum ] == N_.size() );
			assert( entry.size() == colIndexArg.size() );
			assert( N_.size() == colIndex_.size() );
			N_.insert( N_.end(), entry.begin(), entry.end() );
			colIndex_.insert( colIndex_.end(), 
				colIndexArg.begin(), colIndexArg.end() );
			rowStart_[rowNum + 1] = N_.size();
		}

		void printTriplet( const vector< Triplet< T > >& t )
		{
			for ( unsigned int i = 0; i < t.size(); ++i ) {
				cout << i << "	" << t[i].a_ << "	" << t[i].b_ <<
					"	" << t[i].c_ << endl;
			}
		}

		/**
		 * Does a transpose, using as workspace a vector of size 3 N_
		 * 0257 -> 0011122
		 */
		void transpose() {
			vector< Triplet< T > > t;
			
			unsigned int rowIndex = 0;
			if ( rowStart_.size() < 2 )
				return;
			/*
			for ( unsigned int i = 0; i < rowStart_.size(); ++i )
				cout << rowStart_[i] << " ";
			cout << endl;
			*/
			// cout << "rowNum = ";
			unsigned int rs = rowStart_[0];
			for ( unsigned int i = 0; i < N_.size(); ++i ) {
				while( rs == rowStart_[ rowIndex + 1 ] ) {
					rowIndex++;
				}
				rs++;

				/*
				if ( i == rowStart_[j] ) {
					rowNum++;
					j++;
				}
				*/
			// cout << rowNum << " ";
				// The rowNum is going to be the new colIndex.
				Triplet< T > x( N_[i], rowIndex, colIndex_[i] );
				t.push_back( x );
			}
			// cout << endl;
			// cout << "before sort\n"; printTriplet( t );
			stable_sort( t.begin(), t.end() );
			// cout << "after sort\n"; printTriplet( t );

			unsigned int j = ~0;
			rowStart_.resize( 0 );
			rowStart_.push_back( 0 );
			unsigned int ci = 0;
			for ( unsigned int i = 0; i < N_.size(); ++i ) {
				N_[i] = t[i].a_;
				colIndex_[i] = t[i].b_;

				while ( ci != t[i].c_ ) {
					rowStart_.push_back( i );
					ci++;
				}

				/*
				if ( t[i].c_ != j ) {
					j = t[i].c_;
					rowStart_.push_back( i );
				}
				*/
			}
			rowStart_.push_back( N_.size() );
			j = nrows_;
			nrows_ = ncolumns_;
			ncolumns_ = j;
			assert( rowStart_.size() == nrows_ + 1 );
		}

		/**
		 * Prints out the contents in matrix form
		 */
		void print() const {
			for ( unsigned int i = 0; i < nrows_; ++i ) {
				unsigned int k = rowStart_[i];
				unsigned int end = rowStart_[i + 1];
				unsigned int nextColIndex = colIndex_[k];
				for ( unsigned int j = 0; j < ncolumns_; ++j ) {
					if ( j < nextColIndex ) {
						cout << "0	";
					} else if ( k < end ) {
						cout << N_[k] << "	";
						++k;
						nextColIndex = colIndex_[k];
					} else {
						cout << "0	";
					}
				}
				cout << endl;
			}
		}

		/**
		 * Prints out the contents in internal form
		 */
		void printInternal() const {
			unsigned int max = (nrows_ < N_.size() ) ? N_.size() : nrows_+1;
			cout << "#	";
			for ( unsigned int i = 0; i < max; ++i )
				cout << i << "	";
			cout << "\nrs	";
			for ( unsigned int i = 0; i < rowStart_.size(); ++i )
				cout << rowStart_[i] << "	";
			cout << "\ncol	";
			for ( unsigned int i = 0; i < N_.size(); ++i )
				cout << colIndex_[i] << "	";
			cout << "\nN	";
			for ( unsigned int i = 0; i < N_.size(); ++i )
				cout << N_[i] << "	";
			cout << endl;
		}

	protected:
		unsigned int nrows_;
		unsigned int ncolumns_;
		vector< T > N_;	/// Non-zero entries in the SparseMatrix.

		/* 
		 * Column index of each non-zero entry. 
		 * This matches up entry-by entry with the N_ vector.
		 */
		vector< unsigned int > colIndex_;	

		/// Start index in the N_ and colIndex_ vectors, of each row.
		vector< unsigned int > rowStart_;
};

#endif // _SPARSE_MATRIX_H
