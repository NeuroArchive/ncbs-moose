/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2006 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "../shell/Shell.h"
//////////////////////////////////////////////////////////////
//	ObjId I/O 
//////////////////////////////////////////////////////////////

const ObjId ObjId::bad()
{
  static ObjId bad_( Id(), ~0U );
  return bad_;
}

ostream& operator <<( ostream& s, const ObjId& i )
{
	if ( i.dataId == 0 && i.fieldIndex == 0 )
		s << i.id;
	else if ( i.fieldIndex == 0 )
		s << i.id << "[" << i.dataId << "]";
	else
		s << i.id << "[" << i.dataId << "][" << i.fieldIndex << "]";
	return s;
}

/**
 * need to complete implementation
 */
istream& operator >>( istream& s, ObjId& i )
{
	s >> i.id;
	return s;
}

//////////////////////////////////////////////////////////////
ObjId::ObjId( const string& path )
{
	Shell* shell = reinterpret_cast< Shell* >( Id().eref().data() );
	assert( shell );
	*this = shell->doFind( path );
}

Eref ObjId::eref() const
{
	return Eref( id.element(), dataId, fieldIndex );
}

bool ObjId::operator==( const ObjId& other ) const
{
	return ( id == other.id && dataId == other.dataId && 
					fieldIndex == other.fieldIndex );
}

bool ObjId::operator!=( const ObjId& other ) const
{
	return ( id != other.id || dataId != other.dataId || 
					fieldIndex != other.fieldIndex );
}

bool ObjId::isDataHere() const
{
	return true; // Later look up Element.
}

char* ObjId::data() const
{
	return id.element()->data( dataId, fieldIndex );
}

string ObjId::path() const
{
	return Neutral::path( eref() );
}

Element* ObjId::element() const
{
	return id.element();
}
