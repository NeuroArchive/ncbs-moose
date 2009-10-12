/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "Message.h"


bool addMsgToFunc( Element* src, const Finfo* finfo, Element* dest, 
	FuncId fid )
{
	const SrcFinfo* sf = dynamic_cast< const SrcFinfo* >( finfo );
	if ( !sf ) {
		cout << "Warning: addMsgToFunc:: Source of message '" << 
			finfo->name() << "' is not a SrcFinfo\n";
		return 0;
	}
	Msg* m = new SingleMsg( Eref( src, 0 ), Eref( dest, 0 ) );
	Conn c;
	c.add( m );
	src->addConn( c, sf->getConnId() );
	src->addTargetFunc( fid, sf->getFuncIndex() );
	return 1;
}

bool addSharedMsg( Element* src, const Finfo* f1, Element* dest, 
	const Finfo* f2 )
{
	return 0;
}

/**
 * Normal message adds should be from srcFinfo to named opFunc.
 * Shared message adds are from srcFinfo to destfinfo, where both are
 * 	shared Finfos.
 */
bool add( Element* src, const string& srcField, 
	Element* dest, const string& destField )
{
	const Finfo* f1 = src->cinfo()->findFinfo( srcField );
	if ( !f1 ) {
		cout << "add: Error: Failed to find field " << srcField << 
			" on src:\n"; // Put name here.
		return 0;
	}

	FuncId fid = dest->cinfo()->getOpFuncId( destField );
	const OpFunc* func = dest->cinfo()->getOpFunc( fid );
	if ( func ) {
		if ( func->checkFinfo( f1 ) ) {
			return addMsgToFunc( src, f1, dest, fid );
		} else {
			cout << "add: Error: Function type mismatch for " << 
				destField << " on dest:\n"; // Put name here.
			return 0;
		}
	} else {
		const Finfo* f2 = dest->cinfo()->findFinfo( destField );
		if ( f2 ) {
			return addSharedMsg( src, f1, dest, f2 );
		} else {
			cout << "add: Error: Failed to find field " << destField << 
				" on dest:\n"; // Put name here.
			return 0;
		}
	}

}
