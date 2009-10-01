/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"

Finfo::Finfo( const string& name, const string& doc )
	: name_( name ), doc_( doc )
{
	;
}

const string& Finfo::name( ) const
{
	return name_;
}

void Finfo::registerOpFuncs( 
	map< string, FuncId >& fnames, vector< OpFunc* >& funcs
	)
{
	;
}
