/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _Z_ENZ_H
#define _Z_ENZ_H

class ZEnz: public CplxEnzBase
{
	public: 
		ZEnz();
		~ZEnz();

		//////////////////////////////////////////////////////////////////
		// Field assignment stuff
		//////////////////////////////////////////////////////////////////

		void vSetK1( const Eref& e, const Qinfo* q, double v );
		double vGetK1( const Eref& e, const Qinfo* q ) const;
		void vSetK2( const Eref& e, const Qinfo* q, double v );
		double vGetK2( const Eref& e, const Qinfo* q ) const;
		void vSetKcat( const Eref& e, const Qinfo* q, double v );
		double vGetKcat( const Eref& e, const Qinfo* q ) const;

		void vSetKm( const Eref& e, const Qinfo* q, double v );
		double vGetKm( const Eref& e, const Qinfo* q ) const;
		void vSetNumKm( const Eref& e, const Qinfo* q, double v );
		double vGetNumKm( const Eref& e, const Qinfo* q ) const;
		void vSetRatio( const Eref& e, const Qinfo* q, double v );
		double vGetRatio( const Eref& e, const Qinfo* q ) const;
		void vSetConcK1( const Eref& e, const Qinfo* q, double v );
		double vGetConcK1( const Eref& e, const Qinfo* q ) const;

		//////////////////////////////////////////////////////////////////
		// Dest funcs are mostly dummies.
		//////////////////////////////////////////////////////////////////

		/*
		void vCplx( double n ); //does nothing, dummy defined in CplxEnzBase
		void vProcess( const Eref& e, ProcPtr p );
		void vReinit( const Eref& e, ProcPtr p );
		void dummy( double n );
		*/
		void vRemesh( const Eref& e, const Qinfo* q );

		//////////////////////////////////////////////////////////////////
		// Utility  funcs
		//////////////////////////////////////////////////////////////////
		ZeroOrder* makeHalfReaction( 
			Element* orig, double rate, const SrcFinfo* finfo, Id enz )
			const;

		void setSolver( Id solver, Id orig );

		static const Cinfo* initCinfo();
	private:
		StoichCore* stoich_;
		double concK1_;
};

#endif // _Z_ENZ_H
