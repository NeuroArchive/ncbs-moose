class Tab: public Data
{
	public:
		void process( const ProcInfo* p, Eref e );
		void reinit( Eref e );
		void print();
		Finfo** initClassInfo();
	private:
		vector< double > vec_;
};
