/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2007 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _SBMLREADER_H
#define _SBMLREADER_H
#include <sbml/SBMLTypes.h>
class SbmlReader
{
	struct EnzymeInfo;

	public:
		SbmlReader() {;}
		~SbmlReader() {;}
		void  read(string filename,Id location);
		void  createCompartment(Model* model,Id location);
		void  createMolecule(Model* model,map<string,Id> &);
		void  printParameter(Model* model);
		void  printUnit(Model* model);
		void  createReaction(Model* model,map<string,Id> &);
		
				
	private:
		SBMLDocument* document_;
		SBMLReader reader_;
		Element* comptEl_;
		Element* molecule_;
		Element* reaction_;
		map<string,Eref>elmtMap_;
		void prn_parm(const ASTNode* p,vector <string> & parameters);
		double transformUnits(double msize,UnitDefinition * ud);
		string printAnnotation(SBase *sb,map<string,EnzymeInfo> &);
		//string printNotes(SBase *sb);
		void setupEnzymaticReaction(const EnzymeInfo & einfo);
		struct EnzymeInfo
		{
			Id enzyme;
			Id complex;
			vector<Id> substrates;
			vector<Id> products;
			string groupName;
			int stage;
	
		};
		
		
};
extern const Cinfo* initKinComptCinfo();
extern const Cinfo* initMoleculeCinfo();
extern const Cinfo* initReactionCinfo();
extern const Cinfo* initEnzymeCinfo();
#endif // _SBMLREADER_H

