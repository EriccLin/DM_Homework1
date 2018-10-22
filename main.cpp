/*Author Eric Lin, NCKU CSIE
**DataMining Cource: Homework: to find the association rules from transaction dataset
**Implement FPGrowth Algorithm and utilize the apriori to solve the Associatoin Rulein C++
**Demo1 is the simple transaction itemset, you can make the new itemset by yourself
**Demo2 is can load the dataset generated by the IBM Dataset Generator (in ascii)
*/
#include "EricTool.h"
#include "CountFrequency.h"
#include "FrequentPatternTree.h"


void Demo1(){
	// Demo Dataset
	std::vector< std::vector<int> > vec;
	vec.reserve(10);
	vec.push_back( make_vector<int>() << 1 << 2 << 3 );
	vec.push_back( make_vector<int>() << 2 << 4 );
	vec.push_back( make_vector<int>() << 2 << 5 );
	vec.push_back( make_vector<int>() << 1 << 2 << 4 );
	vec.push_back( make_vector<int>() << 1 << 5 );
	vec.push_back( make_vector<int>() << 2 << 5 );
	vec.push_back( make_vector<int>() << 1 << 5 );
	vec.push_back( make_vector<int>() << 1 << 2 << 5 << 3 );
	vec.push_back( make_vector<int>() << 1 << 2 << 5 );
	// Demo Dataset--end
	itemsetMap itMap;
	CountFrequency cf;
	tic();
	tic();
	load( vec, cf, itMap);
	toc("loading dataset");

	tic();
	FrequentPatternTree fpt;
	int minsupport = 2;
	fpt.init( cf, itMap, minsupport);
	fpt.buildFPTree();
	fpt.findItemLink();
	toc("build Frequent Pattern Tree");
	//fpt.trace();
	//fpt.traceItem();
	tic();
	fpt.apriori();
	toc("run A-priori");
	tic();
	fpt.GenerateAssociationRule(0.3f);
	fpt.printAssociationRule();
	toc("generate Rule");
	toc("total-time");
}
void Demo2(std::string filename){
	itemsetMap itMap;
	CountFrequency cf;
	tic();
	tic();
	load(filename, cf, itMap);
	toc("loading dataset");
	
	tic();
	FrequentPatternTree fpt;
	float min_sup = 0.5f;
	int minsupportCount = min_sup * itMap.size();
	std::cout<<" minsupport = "<<minsupportCount<<" ("<<min_sup<<" ) \n";
	fpt.init( cf, itMap, minsupportCount);
	fpt.buildFPTree();
	fpt.findItemLink();
	toc("build Frequent Pattern Tree");
	//fpt.trace();
	//fpt.traceItem();
	tic();
	fpt.apriori();
	toc("run A-priori");
	tic();
	fpt.GenerateAssociationRule(0.9f);
	fpt.printAssociationRule();
	toc("generate Rule");
	toc("total-time");
	std::vector<std::pair<int, int> > vec = cf.objVector(0);
	std::cout<<"\n";
}

void main(int argc, char* argv[])
{
	std::string filename = "data4";
	Demo1();
	Demo2(filename);
	convertToArff(filename, "IBM_Dataset2");
	system("PAUSE");
	return 0;
}

