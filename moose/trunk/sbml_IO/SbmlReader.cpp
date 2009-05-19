/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2007 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "moose.h"
#include <cmath>
#include <sbml/SBMLTypes.h>
#include <sbml/UnitDefinition.h>
#include <sbml/units/UnitFormulaFormatter.h>
#include <sbml/units/FormulaUnitsData.h>
#include "element/Neutral.h"
#include "kinetics/KinCompt.h"
#include "kinetics/Enzyme.h"
#include "kinetics/Molecule.h"
#include "SbmlReader.h"
#include <stdlib.h>

using namespace std;
map< string,double > parmValueMap;
map< string,double >::iterator pvm_iter;
map< string,string > parmUnitMap;

/* read a model into MOOSE  */
void SbmlReader::read( string filename,Id location )
{
	FILE * fp = fopen( filename.c_str(), "r" );
	if ( fp == NULL){
		cout << filename << " : File does not exist." << endl;
	}
	document_ = readSBML( filename.c_str() );
	unsigned num_errors = document_->getNumErrors();
	
	if ( num_errors > 0 )
	{
		cerr << "Errors encountered while reading" << endl;
		document_->printErrors( cerr );
		errorFlag_ = true;
		return;
	}
	model_= document_->getModel();
	if ( model_ == 0 )
	{
		cout << "No model present." << endl;
		errorFlag_ = true;
	}
	/*if ( !model_->isSetId() ){
		cout << "Model Id not set." << endl;
		errorFlag_ = true;
	}*/
	if ( !errorFlag_ )
		getGlobalParameter(); 
	map< string,Id > idMap;
	map< string,Id > molMap;
	if ( !errorFlag_ )
		idMap = createCompartment( location );
	if ( !errorFlag_ )
		molMap = createMolecule( idMap );
	if ( !errorFlag_ )
		getRules();
	if ( !errorFlag_ )
		createReaction( molMap );
	if ( errorFlag_ )
		return;
}

/* transform units */
double SbmlReader::transformUnits( double mvalue,UnitDefinition * ud )
{	cout<<"num units :"<<ud->getNumUnits()<<endl;
	double lvalue = mvalue;
	//cout<<"derived unit defn:"<<UnitDefinition::printUnits(prm->getDerivedUnitDefinition())<<endl;
	for ( unsigned int ut=0;ut<ud->getNumUnits();ut++ )
	{
		Unit * unit=ud->getUnit(ut);
		double exponent=unit->getExponent();
		cout<<"exponent  :"<<exponent<<endl;
		double multiplier=unit->getMultiplier();
		cout<<"multiplier :"<<multiplier<<endl;
		int scale=unit->getScale();
		cout<<"scale :"<<scale<<endl;
		double ofset=unit->getOffset(); 
		lvalue *= pow( multiplier * pow(10.0,scale), exponent ) + ofset;
		cout<<"lvalue "<<lvalue<<endl;
		if ( unit->isLitre() ){
			cout<<"unit is litre"<<endl;
			lvalue *= pow(1e-3,exponent);
		}
		if ( unit->isMole() ){
			cout<<"unit is mole"<<endl;
			lvalue *= pow( Molecule::NA ,exponent);	
			//lvalue *= pow(6e23,exponent);	
			/*if ( scale != -6 ){
				int s = scale -(-6);
				lvalue *= pow(10.0,-s);		
			}*/
		}
	}
	cout<<"value before return "<<lvalue<<endl;
	return lvalue;
}
/* create COMPARTMENT  */
map< string,Id > SbmlReader::createCompartment( Id location )
{
	static const Cinfo* kincomptCinfo = initKinComptCinfo();
	static const Finfo* sizeFinfo = kincomptCinfo->findFinfo( "size" );
	static const Finfo* dimensionFinfo = kincomptCinfo->findFinfo( "numDimensions" );
	map<string,Id> idMap;	
	Id outcompt; //outside compartment	
	double msize = 0.0,size=0.0;	
	::Compartment* compt;
	for ( unsigned int i=0;i<model_->getNumCompartments();i++ )
	{
		compt = model_->getCompartment(i);
		std::string id;
		if ( compt->isSetId() ){
			id = compt->getId();
		}
		std::string name;
		if ( compt->isSetName() ){
			name = compt->getName();
		} 
		std::string outside;
		if ( compt->isSetOutside() ){
			outside =compt->getOutside ();
			
		}
		if ( compt->isSetSize() ){
			msize =compt->getSize();
		}
		UnitDefinition * ud=compt->getDerivedUnitDefinition();
		//cout<<"compartment unit:"<<UnitDefinition::printUnits(compt->getDerivedUnitDefinition())<<endl;
		size=transformUnits( msize,ud );
		unsigned int dimension=compt->getSpatialDimensions();
		/*string cptId;
		if ( name != "" )
			cptId = name;
		else
			cptId = id;*/
		if ( outside=="" ){	
			comptEl_ = Neutral::create( "KinCompt",id, location, Id::scratchId() ); //create Compartment 
			idMap[id]=comptEl_->id(); 
		}
		else{
			outcompt=idMap.find( outside )->second;
			comptEl_ = Neutral::create( "KinCompt",id, outcompt, Id::scratchId() ); //create Compartment inside 
			idMap[id]=comptEl_->id();
			static const Finfo* outsideFinfo = kincomptCinfo->findFinfo( "outside" );
			static const Finfo* insideFinfo = kincomptCinfo->findFinfo( "inside" );
			Eref( comptEl_ ).add( outsideFinfo->msg(),outcompt(),insideFinfo->msg(),ConnTainer::Default );
		}
		if (size != 0.0){
			
                    set< double >( comptEl_, sizeFinfo, size );
                                        
		}
		if (dimension != 0){
			
			set< unsigned int >( comptEl_,dimensionFinfo,dimension );		
		}
	}
	//createMolecule( idMap );
	return idMap;
}

/* create MOLECULE */
map< string,Id > SbmlReader::createMolecule( map< string,Id > &idMap )
{	
	map<string,Id>molMap;
	//map<string,string>cmptMap;//map between species id and its compartment
	static const Cinfo* moleculeCinfo = initMoleculeCinfo();
	static const Finfo* modeFinfo = moleculeCinfo->findFinfo( "mode" );
	static const Finfo* nInitFinfo = moleculeCinfo->findFinfo( "nInit" );	
	static const Cinfo* kincomptCinfo = initKinComptCinfo();
	static const Finfo* dimensionFinfo = kincomptCinfo->findFinfo( "numDimensions" );
	static const Finfo* sizeFinfo = kincomptCinfo->findFinfo( "size" );
	
	for ( unsigned int m=0;m<model_->getNumSpecies();m++ )
	{
		Species* s = model_->getSpecies(m);
		std::string compt;		
		if ( s->isSetCompartment() )		
			compt = s->getCompartment();
		string id = s->getId();
		cout<<"species is :"<<id<<endl;
		std::string name;
		if ( s->isSetName() ){
			name = s->getName();
		} 
		/*string spId;
		if ( name != "" )
			spId = name;
		else
			spId = id;*/
		//cmptMap[id] = compt;
		Id comptEl = idMap[compt];
		molecule_ = Neutral::create( "Molecule",id,comptEl,Id::scratchId() );//create Molecule
		molMap[id] = comptEl; 
		elmtMap_[id] = Eref( molecule_ );
		//printNotes(s);
		UnitDefinition * ud = s->getDerivedUnitDefinition();
		cout<<"species unit :"<<UnitDefinition::printUnits(s->getDerivedUnitDefinition())<<endl;
		double initvalue;
		if ( s->isSetInitialConcentration() )
			initvalue = s->getInitialConcentration();
		if ( s->isSetInitialAmount() )
			initvalue = s->getInitialAmount() ;
		double transvalue = transformUnits(1,ud);
		transvalue *= initvalue;
		//double transvalue = transformUnits(initvalue,ud);
		//bool has_subunits = s->getHasOnlySubstanceUnits();
		//cout<<"has_sub "<<has_subunits<<endl;
		unsigned int dimension;
                get< unsigned int >( comptEl.eref(), dimensionFinfo,dimension );
		if ( dimension > 0 && s->isSetInitialConcentration() ) { 
					
			double size;
			get< double > (comptEl.eref(),sizeFinfo,size); 			
			transvalue *= size;	
			//transvalue /= size;					
		}
		//cout<<"n init is :"<<transvalue<<endl;
		set< double >( molecule_, nInitFinfo, transvalue ); //initialAmount 	
		bool cons=s->getConstant(); 
		bool bcondition = s->getBoundaryCondition();
		if ( cons && bcondition )
			set< int >( molecule_,modeFinfo,4 ); //getConstant=True indicates a buffered molecule
		else if ( !cons && bcondition )
			set< int >( molecule_,modeFinfo,1 ); //indicates the species has assignment rule
		else if ( !cons && !bcondition )
			set< int >( molecule_,modeFinfo,0 );
	}
	//getRules();
	//createReaction( molMap );
	return molMap;
}
/*
*  get Assignment Rule
*/
void SbmlReader::getRules()
{
	unsigned int nr = model_->getNumRules();
	//cout<<"no of rules:"<<nr<<endl;
	for ( unsigned int r=0;r<nr;r++ )
	{
		Rule * rule =model_->getRule(r);
		cout<<"rule :"<<rule->getFormula()<<endl;
		bool assignRule = rule->isAssignment();
		cout<<"is assignment :"<<assignRule<<endl;
		if ( assignRule ){
			string rule_variable = rule->getVariable();
			cout<<"variable :"<<rule_variable<<endl;
			map< string,Eref >::iterator v_iter;
			map< string,Eref >::iterator m_iter;
			v_iter = elmtMap_.find( rule_variable );
			if ( v_iter != elmtMap_.end() ){
				
				Eref rVariable = elmtMap_.find(rule_variable)->second;
				//assert( rVariable != 0 );
				const ASTNode * ast = rule->getMath();
				vector< string > ruleMembers;
				ruleMembers.clear();
				printMembers( ast,ruleMembers );
				for ( unsigned int rm=0;rm<ruleMembers.size();rm++ )
				{
					m_iter = elmtMap_.find( ruleMembers[rm] );	
					if ( m_iter != elmtMap_.end() ){				
						Eref rMember = elmtMap_.find(ruleMembers[rm])->second;				
						rMember.add( "nSrc",rVariable,"sumTotal",ConnTainer::Default ); 
					}
					else{
						cerr <<"SbmlReader::getRules: Assignment rule member is not a species"<<endl;
						errorFlag_ = true;
					}	
				}
			}
			else{
				cerr <<"SbmlReader::getRules: for now Assignment rule for parameter/compartment is not handled"<<endl;
				errorFlag_ = true;
			}
		}
		bool rateRule = rule->isRate();
		if ( rateRule ){
			cout<<"warning : for now Rate Rule is not handled "<<endl;
			errorFlag_ = true;
		}
		bool  algebRule = rule->isAlgebraic(); 
		if ( algebRule ){
			cout<<"warning: for now Algebraic Rule is not handled"<<endl;
			errorFlag_ = true;
		}
	}
}

/* get annotation */
string SbmlReader::getAnnotation( Reaction* reaction,map<string,EnzymeInfo> &enzInfoMap )
{
	XMLNode * annotationNode = reaction->getAnnotation();
	EnzymeInfo einfo;
	string grpname = "",stage;
	if( annotationNode != NULL )
	{	cout<<"num of children :"<<annotationNode->getNumChildren()<<endl;
		for( unsigned int l = 0; l < annotationNode->getNumChildren(); l++ )
		{
			XMLNode &childNode = annotationNode->getChild( l );
			for( unsigned int m = 0; m < childNode.getNumChildren(); m++ )
			{
				XMLNode &grandChildNode = childNode.getChild( l );
				if ( grandChildNode.getPrefix() == "moose" && grandChildNode.getName() == "EnzymaticReaction" )
				{	for( unsigned int n = 0; n < grandChildNode.getNumChildren(); n++ )
					{
						XMLNode &greatGrandChildNode = grandChildNode.getChild( n );
						Eref elem;
						if ( greatGrandChildNode.getName() == "substrates" ){
							string str = greatGrandChildNode.getChild(0).toXMLString();
							elem=elmtMap_.find(str)->second; 
							einfo.substrates.push_back(elem.id());
						}
						else if ( greatGrandChildNode.getName() == "products" ){
							string str = greatGrandChildNode.getChild(0).toXMLString();
							elem=elmtMap_.find(str)->second; 
							einfo.products.push_back(elem.id());
						}
						else if ( greatGrandChildNode.getName() == "complex" ){
							string str = greatGrandChildNode.getChild(0).toXMLString();
							elem=elmtMap_.find(str)->second; 							
							einfo.complex=elem.id();
						}
						else if ( greatGrandChildNode.getName() == "enzyme" ){
							string str = greatGrandChildNode.getChild(0).toXMLString();
							elem=elmtMap_.find(str)->second; 
							einfo.enzyme=elem.id();
						}
						else if ( greatGrandChildNode.getName() == "groupName" ){
							grpname = greatGrandChildNode.getChild(0).toXMLString();
						}
						else if ( greatGrandChildNode.getName() == "stage" ){
							stage = greatGrandChildNode.getChild(0).toXMLString();
						}
						
					}
					if ( stage == "1" ){
						enzInfoMap[grpname].substrates = einfo.substrates;
						enzInfoMap[grpname].complex = einfo.complex;
						enzInfoMap[grpname].enzyme = einfo.enzyme;
						einfo.stage = 1;
						enzInfoMap[grpname].stage = einfo.stage;
						//cout<<"stage:"<<enzInfoMap[grpname].stage <<endl;
						KineticLaw * klaw=reaction->getKineticLaw();
						vector< double > rate ;
						rate.clear();						
						getKLaw(klaw,true,rate);
						if ( errorFlag_ )
							exit(0);
						else if ( !errorFlag_ ){
							enzInfoMap[grpname].k1 = rate[0];
							enzInfoMap[grpname].k2 = rate[1];
						}
					}					
					else if ( stage == "2" ){
						enzInfoMap[grpname].products = einfo.products;
						einfo.stage = 2;
						enzInfoMap[grpname].stage += einfo.stage;
						//cout<<"stage:"<<enzInfoMap[grpname].stage<<endl;
						KineticLaw * klaw=reaction->getKineticLaw();
						vector< double > rate;	
						rate.clear();
						getKLaw(klaw,false,rate);
						if ( errorFlag_ )
							exit(0);
						else if ( !errorFlag_ )
							enzInfoMap[grpname].k3 = rate[0];
		
					}
				}
			}
		}
	}
	return grpname;
}

/* Enzymatic Reaction */
void SbmlReader::setupEnzymaticReaction( const EnzymeInfo & einfo,string name )
{
	static const Cinfo* enzymeCinfo = initEnzymeCinfo();	
	static const Finfo* k1Finfo = enzymeCinfo->findFinfo( "k1" );
	static const Finfo* k2Finfo = enzymeCinfo->findFinfo( "k2" );
	static const Finfo* k3Finfo = enzymeCinfo->findFinfo( "k3" );
	Eref E = ( einfo.enzyme )();
	Element* enzyme_ = Neutral::create( "Enzyme",name,E.id(),Id::scratchId() );//create Enzyme
	Eref complx = einfo.complex(); 
	Eref(enzyme_).add( "enz",E,"reac",ConnTainer::Default ); 
 	Eref(enzyme_).add( "cplx",complx,"reac",ConnTainer::Default ); 
	vector< Id >::const_iterator sub_itr;
	for ( sub_itr = einfo.substrates.begin(); sub_itr != einfo.substrates.end(); sub_itr++ )
	{	Eref S = (*sub_itr)();
		Eref( enzyme_ ).add( "sub",S,"reac",ConnTainer::Default ); 

	}
	vector< Id >::const_iterator prd_itr;
	for ( prd_itr = einfo.products.begin(); prd_itr != einfo.products.end(); prd_itr++ )
	{	Eref P = (*prd_itr)();
		Eref( enzyme_ ).add( "prd",P,"prd",ConnTainer::Default );

	}
	set< double >( enzyme_, k1Finfo, einfo.k1 );
	set< double >( enzyme_, k2Finfo, einfo.k2 );
	set< double >( enzyme_, k3Finfo, einfo.k3 );
	set< bool >( enzyme_,"mode",0 );
	set( complx,"destroy" );
}
/*
*  set up Michalies Menten reaction
*/
void SbmlReader::setupMMEnzymeReaction( Reaction * reac,string rid )
{
	string::size_type loc = rid.find( "_MM_Reaction_" );
	if( loc != string::npos ){
	     	int strlen = rid.length(); 
		rid.erase( loc,strlen-loc );
	}
     	static const Cinfo* enzymeCinfo = initEnzymeCinfo();
	static const Finfo* kmFinfo = enzymeCinfo->findFinfo( "Km" );
	static const Finfo* kcatFinfo = enzymeCinfo->findFinfo( "kcat" );	
	for ( unsigned int m=0;m<reac->getNumModifiers();m++ )
	{	
		const ModifierSpeciesReference* modfr=reac->getModifier(m);
		string sp = modfr->getSpecies();
		Eref E = elmtMap_.find(sp)->second;
		Element* enzyme_ = Neutral::create( "Enzyme",rid,E.id(),Id::scratchId() );//create Enzyme
		KineticLaw * klaw=reac->getKineticLaw();
		vector< double > rate;
		rate.clear();
		getKLaw( klaw,true,rate );
		if ( errorFlag_ )
			return;
		else if ( !errorFlag_ ){
			set< bool >( enzyme_,"mode",1 );
			Eref(enzyme_).add( "enz",E,"reac",ConnTainer::Default ); 
			for ( unsigned int rt=0;rt<reac->getNumReactants();rt++ )
			{	
				const SpeciesReference* rct=reac->getReactant(rt);
				sp=rct->getSpecies();
				Eref S = elmtMap_.find(sp)->second;
				Eref(enzyme_).add( "sub",S,"reac",ConnTainer::Default ); 
			}
			for ( unsigned int pt=0;pt<reac->getNumProducts();pt++ )
			{
				const SpeciesReference* pdt=reac->getProduct(pt);
				sp=pdt->getSpecies();
				Eref P = elmtMap_.find(sp)->second;
				Eref(enzyme_).add( "prd",P,"prd",ConnTainer::Default );
			}
			set< double >( enzyme_, kcatFinfo, rate[0] );
			set< double >( enzyme_, kmFinfo, rate[1] );
		}

	}	
}
//get  global PARAMETERS
void SbmlReader::getGlobalParameter()
{	
	for ( unsigned int pm=0;pm<model_->getNumParameters();pm++ )
	{
		Parameter* prm=model_->getParameter(pm);
		std::string id,unit;
		if ( prm->isSetId() ){
			id = prm->getId();
		}
		double value;		
		if ( prm->isSetValue() ){		
			value=prm->getValue();	
		}
		parmValueMap[id]=value;
	}
}
//create REACTION
void SbmlReader::createReaction( map< string,Id > &molMap )
{	
	map< string,double > rctMap;
	map< string,double >::iterator rctMap_iter;
	map< string,double >pdtMap;
	map< string,double >::iterator pdtMap_iter;
	map< string,Eref >::iterator elemt_iter;
	map< string,EnzymeInfo >enzInfoMap;
	double rctorder,pdtorder;
	static const Cinfo* moleculeCinfo = initMoleculeCinfo();
	static const Finfo* reacFinfo =moleculeCinfo->findFinfo( "reac" );	
	static const Cinfo* reactionCinfo = initReactionCinfo();
	static const Finfo* subFinfo = reactionCinfo->findFinfo( "sub" );
	static const Finfo* prdFinfo = reactionCinfo->findFinfo( "prd" );
	static const Finfo* kfFinfo = reactionCinfo->findFinfo( "kf" );	
	static const Finfo* kbFinfo = reactionCinfo->findFinfo( "kb" );	
	Reaction* reac;	
	for ( unsigned int r=0;r<model_->getNumReactions();r++ )
	{	
		reac=model_->getReaction(r); 
		const string id=reac->getId();
		cout<<"reaction is "<<id<<endl;
		std::string name;
		if ( reac->isSetName() ){
			name = reac->getName();
		}
		/*string rtnId;
		if ( name != "" )
			rtnId = name;
		else
			rtnId = id;*/
		string grpname = getAnnotation(reac,enzInfoMap);
		if ( (grpname != "") && (enzInfoMap[grpname].stage == 3) )
			setupEnzymaticReaction( enzInfoMap[grpname],grpname );
		else if ( grpname == "" )
		{
			if ( reac->getNumModifiers()> 0 )
				 setupMMEnzymeReaction( reac,id );
			else{
				bool rev=reac->getReversible();
				bool fast=reac->getFast();
				if ( fast ){
					cout<<"warning: for now fast attribute is not handled"<<endl;
					errorFlag_ = true;
				}
				int numRcts = reac->getNumReactants();
				int numPdts = reac->getNumProducts();
				if ( numRcts == 0 && numPdts != 0 ){
					const SpeciesReference* pdt = reac->getProduct( 0 );
					std::string spName = pdt->getSpecies();     
					Id parent = molMap.find( spName )->second; //gives compartment of spName
					string parentCompt = parent()->name();
					cout<<"parent of reactant :"<<parentCompt<<endl;
					ostringstream spId;
					spId <<id<<"_Src";
					molecule_ = Neutral::create( "Molecule",spId.str(),parent,Id::scratchId() );//create Molecule
					molMap[spId.str()] = parent; 
					elmtMap_[spId.str()] = Eref( molecule_ );
					set< double >( molecule_,"conc", 1 );
					set< int >( molecule_,"mode",4 );
					reaction_ = Neutral::create( "Reaction",id,parent,Id::scratchId() ); //create Reaction
					Eref( reaction_ ).add( subFinfo->msg(),elmtMap_[spId.str()],reacFinfo->msg(),ConnTainer::Default );
				}
				else{	
					const SpeciesReference* rect=reac->getReactant(0);
					std::string sp=rect->getSpecies();
					Id m = molMap.find(sp)->second; //gives compartment of sp
					reaction_ = Neutral::create( "Reaction",id,m,Id::scratchId() ); //create Reaction
					double rctcount=0.0;	
					rctMap.clear();
					for ( unsigned int rt=0;rt<reac->getNumReactants();rt++ )
					{	
						const SpeciesReference* rct=reac->getReactant(rt);
						sp=rct->getSpecies();
						rctMap_iter = rctMap.find(sp);			
						if ( rctMap_iter != rctMap.end() ){	
							rctcount = rctMap_iter->second;
						}		
						else {
							rctcount = 0.0;
						}
						rctcount += rct->getStoichiometry();
						rctMap[sp] = rctcount;
						for ( int i=0;(int)i<rct->getStoichiometry();i++ )
						{	
							Eref(reaction_).add( subFinfo->msg(),elmtMap_[sp],reacFinfo->msg(),ConnTainer::Default );
				
						}
					}
				}
				double pdtcount = 0.0;
				pdtMap.clear();
				for ( unsigned int pt=0;pt<reac->getNumProducts();pt++ )
				{
					const SpeciesReference* pdt=reac->getProduct(pt);
					std::string sp=pdt->getSpecies();	
					pdtMap_iter = pdtMap.find(sp);
					if ( pdtMap_iter != pdtMap.end() ){	
						pdtcount = pdtMap_iter->second;
					}		
					else {
						pdtcount = 0.0;
					}
					pdtcount += pdt->getStoichiometry();
					pdtMap[sp] = pdtcount;	
					for ( int i=0;i<pdt->getStoichiometry();i++ )
					{	
						Eref(reaction_).add( prdFinfo->msg(),elmtMap_[sp],reacFinfo->msg(),ConnTainer::Default );
					}
			
				}
				//order of reactants
				rctorder = 0.0;	
				string rsp = "",psp = "";
				for ( rctMap_iter=rctMap.begin();rctMap_iter!=rctMap.end();rctMap_iter++ )
				{
					rctorder += rctMap_iter->second;
					rsp=rctMap_iter->first;	//species of the reactant
				}	
				cout<<"rct order = "<<rctorder<<endl;
				//order of products
				pdtorder = 0.0;
				for ( pdtMap_iter=pdtMap.begin();pdtMap_iter!=pdtMap.end();pdtMap_iter++ )
				{
					pdtorder += pdtMap_iter->second;
					psp=pdtMap_iter->first;	//species of the product	
				}
				cout<<"pdt order = "<<pdtorder<<endl;
				if ( reac->isSetKineticLaw() )
				{	KineticLaw * klaw=reac->getKineticLaw();
					//vector< double > rate = getKLaw( klaw,rev );
					vector< double > rate;
					rate.clear();
					getKLaw( klaw,rev,rate );
					if ( errorFlag_ )
						return;
					else if ( !errorFlag_ ){
						set< double >( reaction_, kfFinfo, rate[0] ); 
						set< double >( reaction_, kbFinfo, rate[1] );	
					}
			
				}

			}//else modifier
		}//else 	
	}//reaction 
}//create reaction
/*
*  get Kinetic Law
*/
void SbmlReader::getKLaw( KineticLaw * klaw,bool rev,vector< double > & rate )
{								
	std::string id;
	double value = 0.0;
	UnitDefinition * kfud;
	UnitDefinition * kbud;
	int np = klaw->getNumParameters();
	bool flag = true;
	for ( int pi=0;pi<np;pi++ )
	{
		Parameter * p = klaw->getParameter(pi);
		if ( p->isSetId() )
			id = p->getId();
		if ( p->isSetValue() )		
			value=p->getValue();
		parmValueMap[id] = value;
		flag = false;
	}
	double kf=0.0,kb=0.0,kfvalue,kbvalue;
	string kfparm,kbparm;
	vector< string > parameters;
	parameters.clear();	
	const ASTNode* astnode=klaw->getMath();
	cout <<"kinetic law is :"<<SBML_formulaToString(astnode) << endl;	
	getParameters( astnode,parameters );
	if ( errorFlag_ )
		return;
	else if ( !errorFlag_ )
	{
		if ( parameters.size() == 1 ){
			kfparm = parameters[0];
			kbparm = parameters[0];
		}
		else if ( parameters.size() == 2 ){
			kfparm = parameters[0];
			kbparm = parameters[1];
		}
		kfvalue = parmValueMap[kfparm];
		kbvalue = parmValueMap[kbparm];
		Parameter* kfp;
		Parameter* kbp;
		if ( flag ){
			kfp = model_->getParameter( kfparm );
			kbp = model_->getParameter( kbparm );
		}
		else{
			kfp = klaw->getParameter( kfparm );
			kbp = klaw->getParameter( kbparm );
		}
		cout<<"parameter unit :"<<UnitDefinition::printUnits(kfp->getDerivedUnitDefinition())<<endl;
		if ( kfp->isSetUnits() ){
			kfud = kfp->getDerivedUnitDefinition();
			cout<<"parameter unit :"<<UnitDefinition::printUnits(kfp->getDerivedUnitDefinition())<<endl;
			double transkf = transformUnits( 1,kfud );	
			cout<<"parm kf trans value : "<<transkf<<endl;
			cout<<"kfvalue :"<<kfvalue<<endl;
			kf = kfvalue * transkf;
			kb = 0.0;
		}
		else if (! kfp->isSetUnits() ){
			kf = kfvalue;
			kb = 0.0;
		}
		cout<<"parameter unit :"<<UnitDefinition::printUnits(kbp->getDerivedUnitDefinition())<<endl;
		if ( ( kbp->isSetUnits() ) && ( rev ) ){ 
			kbud = kbp->getDerivedUnitDefinition();	
			cout<<"parameter unit :"<<UnitDefinition::printUnits(kbp->getDerivedUnitDefinition())<<endl;
			double transkb = transformUnits( 1,kbud );
			cout<<"parm kb trans value : "<<transkb<<endl;
			cout<<"kbvalue :"<<kbvalue<<endl;
			kb = kbvalue * transkb;	
		}			
		/*if ( rev ){
			double transkb = transformUnits( 1,kbud );
			cout<<"parm kb trans value : "<<transkb<<endl;
			cout<<"kbvalue :"<<kbvalue<<endl;
			kb = kbvalue * transkb;
		}*/
		if ( (! kbp->isSetUnits() ) && ( rev ) ){
			kb = kbvalue;
		}
		rate.push_back( kf );
		rate.push_back( kb );
	}
	
} 
/*
*  get Parameters from Kinetic Law
*/
void SbmlReader::getParameters( const ASTNode* node,vector <string> & parameters )
{
	assert( parameters.empty() );	
	if ( node->getType() == AST_MINUS ){
		const ASTNode* lchild = node->getLeftChild();  
		pushParmstoVector( lchild,parameters );
		if ( parameters.size() == 1 ) {
			const ASTNode* rchild = node->getRightChild();
			pushParmstoVector( rchild,parameters );		
		}
	}
	else if ( node->getType() == AST_DIVIDE ){
		const ASTNode* lchild = node->getLeftChild();  
	 	pushParmstoVector( lchild,parameters );
		if (( parameters.size() == 1 ) || ( parameters.size() == 0 )) {
			const ASTNode* rchild = node->getRightChild();
			pushParmstoVector( rchild,parameters );	
		}
	}
	else if ( node->getType() == AST_TIMES )
		pushParmstoVector( node,parameters );
	else if ( node->getType() == AST_PLUS )
		pushParmstoVector( node,parameters );
	else if ( node->getType() == AST_NAME )
		pushParmstoVector( node,parameters );
	if ( parameters.size() > 2 ){
		cout<<"Sorry! for now MOOSE cannot handle more than 2 parameters ."<<endl;
		errorFlag_ = true;
	}		
}
/*
*  push the Parameters used in Kinetic law to a vector
*/
void SbmlReader::pushParmstoVector(const ASTNode* p,vector <string> & parameters)
{
	string parm = "";
	if ( p->getType() == AST_NAME ){
        	cout << "_NAME" << " = " << p->getName() << endl;
	   	pvm_iter = parmValueMap.find( std::string(p->getName()) );			
	    	if ( pvm_iter != parmValueMap.end() ){
			parm = pvm_iter->first;
			parameters.push_back( parm );
		}
	}
       	int num = p->getNumChildren();
    	for( int i = 0; i < num; ++i )
    	{  
        	const ASTNode* child = p->getChild(i);
       	 	pushParmstoVector( child,parameters );
        }
} 
/*  get members of rule */ 
void SbmlReader::printMembers( const ASTNode* p,vector <string> & ruleMembers )
{
	if ( p->getType() == AST_NAME ){
	   	cout << "_NAME" << " = " << p->getName() << endl;
	   	ruleMembers.push_back( p->getName() );
	}
	int num = p->getNumChildren();
    	for( int i = 0; i < num; ++i )
    	{  
        	const ASTNode* child = p->getChild(i);
       	 	printMembers( child,ruleMembers );
        }
} 
