

#include "stdafx.h"
#include "EricTool.h"

void load(std::string path, CountFrequency& itemFreq, itemsetMap& itMap){
	std::fstream fs;
	fs.open(path, std::ios::in);
	if (!fs.is_open() ){
		std::cout<<" failed to open the data source file\n";
		return;
	}
	itMap.clear();
	int cid, tid;
	ItemID itemId; 
	itemset_itor end = itMap.end();
	itemset_itor itor;
	int curIndex = 1;
	itemset is;
	while(!fs.eof() ){
		fs >> cid >> tid >> itemId;
		if ( curIndex != tid ){
			itMap[curIndex] = is;
			is.clear();
			curIndex = tid;
		}
		itemFreq.add( itemId);
		is.push_back( itemId);
	}
	itMap[curIndex] = is;
	fs.close();
	// sort item frequency by decreasing order
	itemFreq.SortByFrequency();
#if 0
	// check item frequency
	std::vector<std::pair< int, int> > vec = itemFreq_.objVector(0);
	for ( int i = 0, sz = (int)vec.size(); i < sz; ++i){
		std::pair< int, int>& pr = vec[i];
		std::cout<< " "<< pr.first<<"  "<<pr.second<<'\n';
	}
	// check itemset
	for( auto itor = aMap_.begin(); itor != end; ++itor ){
		std::cout<<itor->first<<" "<<" sz = "<< itor->second.size();
		for ( auto isitor = itor->second.begin(), isend = itor->second.end(); isitor != isend; ++isitor){
			std::cout<<" "<<*isitor;
		}
		std::cout<<'\n';
	}
#endif
}

void load(const std::vector< std::vector<int> >& vec, CountFrequency& itemFreq, itemsetMap& itMap){
	for( size_t i = 0; i < vec.size(); ++i){
		for( int j = 0, szJ = (int) vec[i].size(); j < szJ ; ++j){
			itemFreq.add(vec[i][j]);
		}
	}
	itemFreq.SortByFrequency();
	for( size_t i = 0; i < vec.size(); ++i){
		itemset is;
		for( int j = 0, szJ = (int) vec[i].size(); j < szJ ; ++j){
			is.push_back( (vec[i][j]));
		}
		itMap[i] = is;
	}
}

void convertToCSV(std::string path){
	std::fstream fs, fout;
	fs.open(path, std::ios::in);
	fout.open(path.append(".csv"), std::ios::out);
	if (!fs.is_open() || !fout.is_open() ){
		std::cout<<" failed to open/create the data file\n";
		return;
	}
	int custID, transID, itemID;
	while( !fs.eof() ){
		fs>>custID;
		fs>>transID;
		fs>>itemID;
		fout<<custID<<","<<transID<<","<<itemID<<'\n';
	}
	std::cout<<path<<'\n';
	std::cout<<"mark\n";
	fs.close();
	fout.close();
}
void convertToArff(std::string path, std::string relationName){
	CountFrequency	itemFreq;
	itemsetMap		itsetMap;
	load(path, itemFreq, itsetMap);

	std::fstream fout;
	fout.open(path.append(".arff"), std::ios::out);
	if ( !fout.is_open() ){
		std::cout<<" failed to create the Association Relation Format File (arff).\n";
		return;
	}
	fout<<"@RELATION "<<relationName<<'\n';
	std::vector< std::pair<int, int> > mainset = itemFreq.objVector(0);
	for( size_t i = 0, sz = mainset.size(); i < sz; ++i){
		// pair(itemID, Freq)
		fout<<"@ATTRIBUTE '"<<mainset[i].first<<"' { t}\n";
	}
	fout<<"@DATA\n";
	itemsetMap::iterator itor	= itsetMap.begin();
	itemsetMap::iterator end	= itsetMap.end();
	for( ; itor != end; ++itor){
		itemset subset = itemFreq.GetOrderByFrequency(itor->second, 0);
		size_t si = 0, ssz = subset.size();
		size_t mi = 0, msz = mainset.size();
		if( mainset[mi].first != subset[si] ){
			fout<< "?";
		}else{
			fout<< "t";
			++si;
		}
		++mi;
		for( ; mi < msz; ++mi){
			if( si == ssz || mainset[mi].first != subset[si] ){
				fout<<",?";
			}else{
				++si;
				fout<<",t";
			}
		}
		fout<<'\n';
	}
	fout.close();
}

void tic(){
	LARGE_INTEGER start;
	::QueryPerformanceCounter(&start); //取得開機到現在經過幾個CPU Cycle
	my_tictoc_stack.push( start);
	if( frequency.QuadPart == 0 ){
		::QueryPerformanceFrequency(&frequency); //取得CPU頻率
	}
	
}

void toc(std::string info){
	LARGE_INTEGER end, start = {0};
	if( !my_tictoc_stack.empty() )
		start = my_tictoc_stack.top();
	my_tictoc_stack.pop();
	::QueryPerformanceCounter(&end); //取得開機到現在經過幾個CPU Cycle
	double times = ((double)end.QuadPart-(double)start.QuadPart)/frequency.QuadPart;
    std::cout << info <<"   "<< std::fixed << std::setprecision(5) << times << "sec-elipsed" << std::endl;
}

template<class T>
std::string ConvertToString(T value){
	std::stringstream ss;
	ss<<std::setprecision(3)<<value;
	return ss.str();
}
