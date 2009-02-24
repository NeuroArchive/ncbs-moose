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
#include "SbmlReader.h"

using namespace std;
map<string,double>parmValueMap;
map<string,double>::iterator pvm_iter;
map<string,string>parmUnitMap;
//function to get the parameter used in the kinetic law
void SbmlReader::prn_parm(const ASTNode* p,vector <string> & parameters)
{
	string parm = "";	
	bool flag=0;  
	switch ( p->getType() ){
        	case (AST_NAME):
	   		cout << "_NAME" << " = " << p->getName() << endl;
	   		pvm_iter = parmValueMap.find(std::string(p->getName()));			
	    		if (pvm_iter != parmValueMap.end()){
			parm = pvm_iter->first;
			parameters.push_back(parm);
			//flag = 1;
			}
	   		// else flag = 0;
	   	 	break;				
    	}
    /*if (flag){ 
	return parm;
    }*/
    	int num = p->getNumChildren();
    	for( int i = 0; i < num; ++i )
    	{  
        	const ASTNode* child = p->getChild(i);
       	 	prn_parm(child,parameters);
        	/*if (!tmp.empty())
        	{
           	 parm = tmp;
          	  break;
        	}*/
    	}
    	//return parm;
 } 
 
//read a model into MOOSE 
void SbmlReader::read(string filename,Id location)
{
	
	FILE * fp = fopen(filename.c_str(), "r");
	if ( fp == NULL){
		cout << filename << " : File does not exist." << endl;
	}
	document_ = readSBML(filename.c_str());
	unsigned num_errors = document_->getNumErrors();
	
	if ( num_errors > 0)
	{
		cerr << "Errors encountered " << endl;
		document_->printErrors(cerr);
		return;
	}
	Model* model= document_->getModel();
	if (model == 0)
	{
		cout << "No model present." << endl;
	}
	if (!model->isSetId()){
		cout << "Id not set." << endl;
	}
	
	
	//printUnit(model);
	createCompartment(model,location);
}
/*string SbmlReader::getRules()
{
	unsigned int nr = model->getNumRules();
	for (int r=0;r<nr;r++)
	{
		Rule * rule =model->getRule(r);
		cout<<"rule :"<<rule->getFormula()<<endl;
		bool assign = rule->isAssignment();
		cout<<"is assignment :"<<assign<<endl;
		if (assign==1){
		unsigned int numia = model->getNumInitialAssignments(); 
		for (int i =0;i<numia;i++)
		{
			InitialAssignment * ia = model->getInitialAssignment (i);
			const string symbol = ia->getSymbol();
			cout<<"symbol :"<< symbol<<endl;
			//const ASTNode ast = ia->getMath();
			//cout<<"ast node :"<<ast<<endl;
	 
		}
		}
		bool algebraic = rule->isAlgebraic();
		cout<<"is algebraic :"<<algebraic<<endl;
		const std::string variab = rule->getVariable();
		cout<<"variable :"<<variab<<endl;
		Species * sp = model->getSpecies(variab);
		cout<<"species :"<<sp->getId()<<endl;
		const ASTNode * ast=rule->getMath();
		
		string parm = prn_parm(ast);
		cout<<"ast node from rule "<<parm<<endl;	
		
	}
}*/
double SbmlReader::transformUnits(double mvalue,UnitDefinition * ud)
{	cout<<"num units :"<<ud->getNumUnits()<<endl;
	double lvalue = mvalue;
	
	for (int ut=0;ut<ud->getNumUnits();ut++)
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
		if (unit->isLitre()){
			cout<<"unit is litre";
			lvalue *= pow(1e-3,exponent);
			//cout<<"size in function is : "<<lsize<<endl;	
		}
		if (unit->isMole()){
			lvalue *= pow(6e23,exponent);	
			cout<<"mole"<<endl;	
		}
		
		
	}
	cout<<"value before return "<<lvalue;
	return lvalue;
}
// create COMPARTMENT  
void SbmlReader::createCompartment(Model* model,Id location)
{
	static const Cinfo* kincomptCinfo = initKinComptCinfo();
	static const Finfo* sizeFinfo = kincomptCinfo->findFinfo( "size" );
	static const Finfo* dimensionFinfo = kincomptCinfo->findFinfo( "numDimensions" );
	map<string,Id> idMap;	
	Id outcompt; //outside compartment	
	double msize = 0.0,size=0.0;	
	::Compartment* compt;
	for (int i=0;i<model->getNumCompartments();i++)
	{
		compt = model->getCompartment(i);
		
		std::string id;
		if (compt->isSetId()){
			id = compt->getId();
		}
		
		std::string name;
		if (compt->isSetName()){
			name = compt->getName();
		} 

		std::string type;
		if (compt->isSetCompartmentType()){
			type =compt->getCompartmentType ();
		}
		
		std::string outside;
		if ( compt->isSetOutside()){
			outside =compt->getOutside ();
			
		}
		
		if (compt->isSetSize()){
			msize =compt->getSize();
		}
		UnitDefinition * ud=compt->getDerivedUnitDefinition();
		size=transformUnits(msize,ud);
		//cout<<"size returned from function is : "<<size<<endl;
		
		unsigned int dimension=compt->getSpatialDimensions();
		
		if (outside==""){		
			comptEl_ = Neutral::create( "KinCompt",id, location, Id::scratchId() ); //create Compartment 
			idMap[id]=comptEl_->id(); 
		}
		else{
			outcompt=idMap.find(outside)->second;
			comptEl_ = Neutral::create( "KinCompt",id, outcompt, Id::scratchId() ); //create Compartment inside 
			idMap[id]=comptEl_->id();
			static const Finfo* outsideFinfo = kincomptCinfo->findFinfo( "outside" );
			static const Finfo* insideFinfo = kincomptCinfo->findFinfo( "inside" );
			Eref(comptEl_ ).add(outsideFinfo->msg(),outcompt(),insideFinfo->msg(),ConnTainer::Default);
		}
		if (size != 0.0){
			
                    set< double >( comptEl_, sizeFinfo, size );
                                        
		}
		if (dimension != 0){
			
			set< unsigned int >( comptEl_,dimensionFinfo,dimension );		
		}
		
		/*if (id != ""){		
			static const Finfo* idFinfo = kincomptCinfo->findFinfo( "id" );
			set< string >( comptEl_,idFinfo,id );
		}
		if (name != ""){		
			static const Finfo* nameFinfo = kincomptCinfo->findFinfo( "name" );
			set< string >( comptEl_,nameFinfo,name );
		}
		if (type != ""){
			static const Finfo* typeFinfo = kincomptCinfo->findFinfo( "type" );
			set< string >( comptEl_,typeFinfo,type );
		}
		if (outside != ""){
			static const Finfo* outsideFinfo = kincomptCinfo->findFinfo( "outside" );
			set< string >( comptEl_,outsideFinfo,outside );
		}*/
		
		
							
	}
	createMolecule(model,idMap);
	
}

//create MOLECULE
void SbmlReader::createMolecule(Model* model,map<string,Id> &idMap)
{	
	
	map<string,Id>molMap;
	map<string,Eref>elmtMap;
	map<string,string>cmptMap;
	static const Cinfo* moleculeCinfo = initMoleculeCinfo();
	static const Finfo* modeFinfo = moleculeCinfo->findFinfo( "mode" );
	static const Finfo* nInitFinfo = moleculeCinfo->findFinfo( "nInit" );	
	static const Finfo* concInitFinfo = moleculeCinfo->findFinfo( "concInit" );
	static const Cinfo* kincomptCinfo = initKinComptCinfo();
	static const Finfo* dimensionFinfo = kincomptCinfo->findFinfo( "numDimensions" );
	static const Finfo* sizeFinfo = kincomptCinfo->findFinfo( "size" );
	
	
	for (unsigned int m=0;m<model->getNumSpecies();m++)
	{
		Species* s = model->getSpecies(m);
		
		std::string compt;		
		if (s->isSetCompartment())		
			compt = s->getCompartment();
		const string id=s->getId();
		cmptMap[id]=compt;
		Id comptEl=idMap[compt];
		molecule_= Neutral::create( "Molecule",id,comptEl,Id::scratchId() );//create Molecule
		molMap[id] = comptEl; 
		elmtMap[id]= Eref(molecule_);
		
		UnitDefinition * ud = s->getDerivedUnitDefinition();
		double initvalue;
		if (s->isSetInitialConcentration()){
			initvalue = s->getInitialConcentration();
		}
		
		if (s->isSetInitialAmount()){
			initvalue = s->getInitialAmount() ;
		}
		cout<<"initvalue is :"<<initvalue<<"from species"<<endl;
		
		double transvalue = transformUnits(initvalue,ud);
		
		bool bcondition = s->getBoundaryCondition();
		bool has_subunits = s->getHasOnlySubstanceUnits();
		//cout<<"has_sub "<<has_subunits<<endl;
		unsigned int dimension;
                get< unsigned int >(comptEl.eref(), dimensionFinfo,dimension);
		if (dimension > 0 && s->isSetInitialConcentration() ) { //has_subunits == false ){
					
			double size;
			get< double > (comptEl.eref(),sizeFinfo,size); 			
			transvalue *= size;			
		}
		set< double >(molecule_, nInitFinfo, transvalue); //initialAmount 	
		
		/*if (dimension > 0 || has_subunits == false)
			set< double >(molecule_, nInitFinfo, transvalue); //initialAmount 			
					
				
		else
			set< double >( molecule_, concInitFinfo, transvalue); //initial concentration	*/
		bool cons=s->getConstant(); 
		if (cons)
			set< int >(molecule_,modeFinfo,4); //getConstant=True indicates a buffered molecule
		else 
			set< int >(molecule_,modeFinfo,0);
		
		/*std::string sptype;		
		if (s->isSetSpeciesType())
			sptype=s->getSpeciesType();
		
		if (s->isSetSubstanceUnits()) 
			cout<<"substance units :"<<s->getSubstanceUnits()<<endl;*/
					
	}
	createReaction(model,molMap,elmtMap);
}

//print PARAMETERS
void SbmlReader::printParameter(Model* model)
{	
	//map<string,double>parmValueMap;
	//map<string,double>::iterator pvm_iter;
	//map<string,string>parmUnitMap;
	for (int pm=0;pm<model->getNumParameters();pm++)
	{
		Parameter* prm=model->getParameter(pm);
		std::string id,unit;
		if (prm->isSetId()){
			id = prm->getId();
		}
		double value;		
		if (prm->isSetValue()){		
			value=prm->getValue();	
		}
		parmValueMap[id]=value;
		/*if (prm->isSetUnits()){
			unit=prm->getUnits();			
		}
		parmUnitMap[id]=unit;*/
		
	}
	cout<<"inside model->parameter()"<<endl;
}


//create REACTION
void SbmlReader::createReaction(Model* model,map<string,Id> &molMap,map<string,Eref> &elmtMap)
{	
	map<string,double>rctMap;
	map <string, double>::iterator rctMap_iter;
	map<string,double>pdtMap;
	map <string, double>::iterator pdtMap_iter;
	map<string,Eref>::iterator elemt_iter;
	double rctorder,pdtorder;
	static const Cinfo* moleculeCinfo = initMoleculeCinfo();
	static const Finfo* reacFinfo =moleculeCinfo->findFinfo( "reac" );	
	static const Cinfo* reactionCinfo = initReactionCinfo();
	static const Finfo* subFinfo = reactionCinfo->findFinfo( "sub" );
	static const Finfo* prdFinfo = reactionCinfo->findFinfo( "prd" );
	static const Finfo* kfFinfo = reactionCinfo->findFinfo( "kf" );	
	static const Finfo* kbFinfo = reactionCinfo->findFinfo( "kb" );	
	static const Cinfo* kincomptCinfo = initKinComptCinfo();
	static const Finfo* sizeFinfo = kincomptCinfo->findFinfo( "size" );
	//static const Finfo* concInitFinfo = moleculeCinfo->findFinfo( "concInit" );
	//printParameter(model); //invoke the function 'parameter'	
	Reaction* reac;	
	for (int r=0;r<model->getNumReactions();r++)
	{	
		reac=model->getReaction(r); 
		const string id=reac->getId();
		cout<<"reaction is "<<id<<endl;
		bool rev=reac->getReversible();
		//bool fast=reac->getFast();
		cout<<"is rev"<<rev<<endl;  
		const SpeciesReference* rect=reac->getReactant(0);
		std::string sp=rect->getSpecies();
		Id m=molMap.find(sp)->second; //gives compartment of sp
		reaction_=Neutral::create( "Reaction",id,m,Id::scratchId() ); //create Reaction
		
		//numreact=reac->getNumReactants();
		//cout<<"num of rct :"<<numreact<<endl;
		double rctcount=0.0;	
		rctMap.clear();
		//double frate=1.0,brate=1.0;
		for (int rt=0;rt<reac->getNumReactants();rt++)
		{	
			const SpeciesReference* rct=reac->getReactant(rt);
			sp=rct->getSpecies();
			//cout<<"reactant is "<<sp<<endl;
			rctMap_iter = rctMap.find(sp);			
			if (rctMap_iter != rctMap.end()){	
				rctcount = rctMap_iter->second;
			}		
			else {
				rctcount = 0.0;
			}
			rctcount += rct->getStoichiometry();
			rctMap[sp] = rctcount;
			for (int i=0;(int)i<rct->getStoichiometry();i++)
			{	
				Eref(reaction_).add(subFinfo->msg(),elmtMap[sp],reacFinfo->msg(),ConnTainer::Default);
				
			}
		}
		//numpdt=reac->getNumProducts();
		//cout<<"no of pdt :"<<numpdt<<endl;
		double pdtcount = 0.0;
		pdtMap.clear();
		for (int pt=0;pt<reac->getNumProducts();pt++)
		{
			const SpeciesReference* pdt=reac->getProduct(pt);
			sp=pdt->getSpecies();	
			//cout<<"product is "<<sp<<endl;
			pdtMap_iter = pdtMap.find(sp);
			if (pdtMap_iter != pdtMap.end()){	
				pdtcount = pdtMap_iter->second;
			}		
			else {
				pdtcount = 0.0;
			}
			pdtcount += pdt->getStoichiometry();
			pdtMap[sp] = pdtcount;	
			for (int i=0;i<pdt->getStoichiometry();i++)
			{	
				Eref(reaction_).add(prdFinfo->msg(),elmtMap[sp],reacFinfo->msg(),ConnTainer::Default);
			}
			
		}
		//order of reactants
		rctorder = 0.0;	
		string rsp = "",psp = "";
		for (rctMap_iter=rctMap.begin();rctMap_iter!=rctMap.end();rctMap_iter++)
		{
			rctorder += rctMap_iter->second;
			rsp=rctMap_iter->first;	//species of the reactant
			//cout<<"rsp "<<rsp<<endl;	
		}	
		cout<<"rct order = "<<rctorder<<endl;
		Id r=molMap.find(rsp)->second;
		//cout<<"r"<< r <<endl;
		
		//order of products
		pdtorder = 0.0;
		for (pdtMap_iter=pdtMap.begin();pdtMap_iter!=pdtMap.end();pdtMap_iter++)
		{
			pdtorder += pdtMap_iter->second;
			psp=pdtMap_iter->first;	//species of the product	
			//cout<<"psp "<<psp<<endl;	
			
		}
		cout<<"pdt order = "<<pdtorder<<endl;
		Id p;
		bool noproduct = false;
		if (psp != ""){		
			p=molMap.find(psp)->second;
			//cout<<"p"<< p <<endl;
		}
		else if (psp == "")
			noproduct = true;
		if (reac->isSetKineticLaw())
		{	KineticLaw * klaw=reac->getKineticLaw();
			
			string timeunit = klaw->getTimeUnits(); 
			string subunit=klaw->getSubstanceUnits();
			std::string id,unit;
			double value = 0.0,rvalue,pvalue;
			UnitDefinition * kfud;
			UnitDefinition * kbud;
			const ASTNode* astnode=klaw->getMath();
			cout <<SBML_formulaToString(astnode) << endl;	
			int np = klaw->getNumParameters();
			cout<<"no of parms : "<<np<<endl;
			vector< string > parameters;
			bool flag = true;
			for (int pi=0;pi<np;pi++)
			{
				Parameter * p = klaw->getParameter(pi);
				if (p->isSetId()){
					id = p->getId();
				}
				if (p->isSetValue()){		
					value=p->getValue();
					cout<<"value of param in kl:"<<value<<endl;	
				}
				parmValueMap[id]=value;
				parameters.push_back(id);
				flag = false;
			}
			double kf=0.0,kb=0.0,kfvalue,kbvalue;
			string kfparm,kbparm;
			if (flag){
				printParameter(model); //invoke the function 'parameter'	
				prn_parm(astnode,parameters);
			}
			if (parameters.size() > 2 ){
				cout<<"Sorry! for now MOOSE cannot handle more than 2 parameters .";
				return;
			}
			else if (parameters.size() == 1){
				kfparm = parameters[0];
				kbparm = parameters[0];
			}
			else{
				kfparm = parameters[0];
				kbparm = parameters[1];
			}
			kfvalue = parmValueMap[kfparm];
			kbvalue = parmValueMap[kbparm];
			Parameter* kfp;
			Parameter* kbp;
			if (flag){
				kfp = model->getParameter(kfparm);
				kbp = model->getParameter(kbparm);
			}
			else{
				kfp = klaw->getParameter(kfparm);
				kbp = klaw->getParameter(kbparm);
			}			
			kfud = kfp->getDerivedUnitDefinition();
			kbud = kbp->getDerivedUnitDefinition();
			double csize;
		        get<double>(r.eref(), sizeFinfo, csize); //getting compartment size
			//cout<<"size :"<<csize<<endl;
			double transkf = transformUnits(1,kfud);	
			cout<<"parm kf trans value : "<<transkf<<endl;
			cout<<"kfvalue :"<<kfvalue<<endl;
			double transkb = transformUnits(1,kbud);	
			cout<<"parm kb trans value : "<<transkb<<endl;
			cout<<"kbvalue :"<<kbvalue<<endl;
			
			
			if (rctorder == 1){
				rvalue=transkf*kfvalue;
				//pvalue=transkb*kbvalue;
			}
			else{			
				double NA = 6.02214199e23; //Avogardo's number	
				rvalue=NA*kfvalue*pow((transkf/csize),rctorder-1);
				//pvalue=NA*kbvalue*pow((transkb/csize),pdtorder-1);
				
			}
			if (pdtorder == 1){
				
				pvalue=transkb*kbvalue;
			}
			else{
				double NA = 6.02214199e23; //Avogardo's number	
				pvalue=NA*kbvalue*pow((transkb/csize),pdtorder-1);
			}
			if (noproduct){
				double size;
                		get<double>(r.eref(), sizeFinfo, size); 
				cout<<"size "<<size<<endl;				
				kf = size * rvalue;
				kb = 0;	
				//cout<<"kf = "<<kf<<"kb = "<<kb<<endl;	
			}							
			else if (r != p){
				double psize,rsize;
                		get<double>(p.eref(), sizeFinfo, psize); 
				cout<<"psize "<<psize<<endl;				
				kf = psize * rvalue;
				if (rev){
					get<double>(r.eref(), sizeFinfo, rsize); 
					cout<<"rsize "<<rsize<<endl;		
					kb = rsize * pvalue;
				}	
				//cout<<"kf = "<<kf<<"kb = "<<kb<<endl;		
			}
			else if ((r == p) && (noproduct == false)){ 
				kf = csize * rvalue;
				if (rev)				
					kb = csize * pvalue;
					
			}
			set< double >( reaction_, kfFinfo, kf); 
			set< double >( reaction_, kbFinfo, kb); 
		} //kinetic law		
	}//reaction 
}//create reaction


