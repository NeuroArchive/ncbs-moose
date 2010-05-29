/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _ONE_DIM_GLOBAL_HANDLER_H
#define _ONE_DIM_GLOBAL_HANDLER_H

/**
 * This class manages the data part of Elements. It handles a one-
 * dimensional array.
 */
class OneDimGlobalHandler: public DataHandler
{
	public:
		OneDimGlobalHandler( const DinfoBase* dinfo );

		~OneDimGlobalHandler();

		/**
		 * Copies contents into a 2-D array.
		 * If the copy is non-global, it does the usual node partitioning
		 * scheme for multinode simulations.
		 */
		DataHandler* copy( unsigned int n, bool toGlobal ) const;

		/**
		 * calls process on data, using threading info from the ProcInfo,
		 * and internal info about node decomposition.
		 */
		void process( const ProcInfo* p, Element* e ) const;

		/**
		 * Returns the data on the specified index.
		 */
		char* data( DataId index ) const;

		/**
		 * Returns the data at one level up of indexing. In this case it
		 * just returns the data on the specified index.
		 */
		char* data1( DataId index ) const;

		/**
		 * Returns the number of data entries.
		 */
		unsigned int numData() const {
			return size_;
		}

		/**
		 * Returns the number of data entries at index 1.
		 * For regular Elements this is identical to numData.
		 */
		unsigned int numData1() const {
			return size_;
		}

		/**
		 * Returns the number of data entries at index 2, if present.
		 * For regular Elements and 1-D arrays this is always 1.
		 */
		 unsigned int numData2( unsigned int index1 ) const {
		 	return 1;
		 }

		/**
		 * Returns the number of dimensions of the data.
		 */
		unsigned int numDimensions() const {
			return 1;
		}

		/**
		 * Assigns the size for the data dimension.
		 */
		void setNumData1( unsigned int size );

		/**
		 * Assigns the sizes of all array field entries at once.
		 * Ignore in this case, as there are none.
		 */
		void setNumData2( unsigned int start,
			const vector< unsigned int >& sizes );

		/**
		 * Looks up the sizes of all array field entries at once.
		 * Ignore in this case, as there are no array fields.
		 */
		unsigned int getNumData2( vector< unsigned int >& sizes ) const;

		/**
		 * Returns true if the node decomposition has the data on the
		 * current node
		 */
		bool isDataHere( DataId index ) const;

		bool isAllocated() const;

		void allocate();

		bool isGlobal() const
		{
			return 1;
		}

		iterator begin() const {
			return 0;
		}

		iterator end() const {
			return size_;
		}

		/**
		 * Adds another entry to the data. Copies the info over.
		 * Returns the index of the new data.
		 * Some derived classes can't handle this. They return 0.
		 */
		unsigned int addOneEntry( const char* data );

		/**
		 * This is relevant only for the 2 D cases like
		 * FieldDataHandlers.
		 */
		unsigned int startDim2index() const {
			return 0;
		}

		void setData( char* data, unsigned int numData ) {
			data_ = data;
			reserve_ = size_ = numData;
		}

	private:
		char* data_;
		unsigned int size_;	// Number of data entries in the whole array
		unsigned int reserve_; // Number of places actually allocated.
};

#endif	// _ONE_DIM_GLOBAL_HANDLER_H
