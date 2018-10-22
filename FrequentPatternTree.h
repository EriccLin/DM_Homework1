
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <cassert>
#include "CountFrequency.h"
#include "AssociationRuleBasic.h"

#ifndef _FREQUENT_PATTERN_TREE_
#define _FREQUENT_PATTERN_TREE_

typedef struct FrequentPatternTreeNode {
	int itemId_;
	int frequency_;
	FrequentPatternTreeNode* parent_;
	std::unordered_map<int, FrequentPatternTreeNode*> childrens_;
	FrequentPatternTreeNode* left_,* right_;
} FPTreeNode;
typedef struct FrequentPatternTableNode {
	int itemId_;
	int frequency_;
	FrequentPatternTreeNode* next_;
} FPTableNode;
typedef struct ConditionalFrequentPatternTreeNode {
	int itemId_;
	int frequency_;
	int order_;
	FrequentPatternTreeNode* parent_;
	std::unordered_map<int, ConditionalFrequentPatternTreeNode*> childrens_;
} CFPTreeNode;
typedef struct FrequentPattern {
	std::vector<ItemID> data_;
	int support_;
} FP;
std::ostream& operator << (std::ostream& os, const FrequentPattern& fp){
	const std::vector< int>& vec = fp.data_;
	size_t sz = vec.size();
	os<<" [";
	for( size_t i = 0; i < sz; ++i){
		os<<" "<<vec[i];
	}
	os<<"]: sup = "<<fp.support_<<" ";
	return os;
}
struct FrequeutPatternHash {
	size_t operator () (const FrequentPattern& fp) const {
		size_t seed = 0;
		std::vector<int> vec = fp.data_;
		/* phi = ( 1 + sqrt(5) ) / 2.0   **
		** 2^32 / phi = 0x9e3779b9       */
		for( size_t i = 0, sz = vec.size(); i < sz; ++i){
			seed ^= std::hash_value(vec[i]) + 0x9e3779b9 + ( seed<<6 ) + ( seed >> 2 );
		}
		return seed;
	}
};
struct FrequeutPatternEqual {
	bool operator () (const FrequentPattern& lhs, const FrequentPattern& rhs) const {
		size_t sz1 = lhs.data_.size(), sz2 = rhs.data_.size();
		if ( sz1 != sz2 )
			return false;
		for( size_t i = 0; i < sz1; ++i){
			if(  lhs.data_[i] != rhs.data_[i] )
				return false;
		}
		return true;
	}
};
typedef std::unordered_set< FrequentPattern, FrequeutPatternHash, FrequeutPatternEqual > FPSet;
typedef struct PackedAssociationRule { // for rule  X -> Y
	FrequentPattern fp_; // i.e. X U Y
	FPSet FPsubset_; // all possible X
} packedAssocRule;
typedef struct AssociationRule {
	itemset X_;
	int supportX_;
	itemset Y_;
	int supportXY_;
} assocRule;
struct CompAssocRuleByConfidenceDec {
	bool operator () ( const AssociationRule& lhs, const AssociationRule& rhs) const {
		float lhsConf = lhs.supportX_ ? lhs.supportXY_/float(lhs.supportX_) : 0.f;
		float rhsConf = rhs.supportX_ ? rhs.supportXY_/float(rhs.supportX_) : 0.f;
		return lhsConf > rhsConf;
	}
};
std::ostream& operator << (std::ostream& os, const AssociationRule& assoc){
	os<<"[ (";
	const itemset& X = assoc.X_;
	for( size_t i = 0, sz = X.size(); i < sz; ++i){
		os<<" "<<X[i];
	}
	os<<") -> (";
	const itemset& Y = assoc.Y_;
	for( size_t i = 0, sz = Y.size(); i < sz; ++i){
		os<<" "<<Y[i];
	}
	os<<") supX = "<<assoc.supportX_
		<<" supXY = "<<assoc.supportXY_
		<<" conf = "<<assoc.supportXY_/float(assoc.supportX_)<<" ]";
	return os;
}
std::ostream& operator << (std::ostream& os, const PackedAssociationRule& assoc){
	const FrequentPattern& fp	= assoc.fp_;
	const FPSet& fpset			= assoc.FPsubset_;
	const itemset& mainset		= fp.data_;
	FPSet::iterator itor		= fpset.begin();
	FPSet::iterator end			= fpset.end();
	os<<"[ sup="<<assoc.fp_.support_<<'\n';
	for( ; itor != end; ++itor){
		os<<"(";
		const itemset& is	= itor->data_;
		int sup				= itor->support_;
		for( size_t j = 0, szJ = is.size(); j < szJ; ++j){
			os<<is[j]<<" ";
		}
		os<<") -> (";
		size_t mi = 0, si = 0;
		size_t msz = mainset.size(), ssz = is.size();
		while( mi < msz && si < ssz){
			if( mainset[mi]==is[si])
				++si;
			else
				os<<" "<<mainset[mi];
			++mi;
		}
		for( ; mi < msz; ++mi){
			os<<" "<<mainset[mi];
		}
		os<<"): sup="<<sup<<" conf = "<<float(fp.support_)/sup<<'\n';
	}
	os<<"]\n";
	return os;
}

class FrequentPatternTree{
	FPTreeNode* root_;							// the root-pointer to the FP Tree
	std::vector< CFPTreeNode*> vecPtr_;			// the vec of root-pointer to the conditional FP Tree
	CountFrequency itemMap_;					// count the occurence of each item in the whole transactions
	itemsetMap itemsetMap_;						// record each itemset
	std::vector< FPTableNode > HeaderTable_;	// Header 
	std::vector< FPSet > vecFPSwithDiffLength_;	// Frequent Pattern Set { [0]: K = 1 [1]: K= 2, ...}
	int minsup_;								// minimum support: the counting threshold of accepted items
	std::vector< AssociationRule > vecAssoc_;	// vector of association rules
public:
	FrequentPatternTree(): root_(NULL), minsup_(0){}
	~FrequentPatternTree(){
		removeFPSubTree();
	}
	void init(const CountFrequency& imap, const itemsetMap& itemsetmap, int minsup){
		itemsetMap_	= itemsetmap;
		itemMap_	= imap;
		minsup_		= minsup;
		// initialize root of the Frequency Pattern Tree
		root_		= new FrequentPatternTreeNode;
		root_->itemId_		= INT_MIN;
		root_->frequency_	= 0;
		root_->left_		= NULL;
		root_->right_		= NULL;
		root_->parent_		= NULL;
		// initialize Header Table for the Frequency Pattern Tree
		HeaderTable_.clear();
		HeaderTable_.reserve(100);
		std::vector< std::pair<int, int> > vec = itemMap_.objVector(minsup_);
		int sz = (int)vec.size();
		for ( int i = 0; i < sz; ++i){
			FrequentPatternTableNode fptn;
			fptn.itemId_	= vec[i].first;			
			fptn.frequency_ = vec[i].second;	
			fptn.next_		= NULL;
			HeaderTable_.emplace_back(fptn);
		}
	}
	void add_without_order( const std::vector<ItemID>& itemVec){
		int i = 0;
		int sz = (int)itemVec.size();
		FrequentPatternTreeNode* preNode = NULL;
		FrequentPatternTreeNode* curNode = root_;
		while( i < sz){
			std::unordered_map<ItemID, FrequentPatternTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator itor;
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator end = umap.end();
			itor = umap.find(itemVec[i]);
			preNode = curNode;
			if ( itor == end ){
				curNode = new FrequentPatternTreeNode;
				umap[itemVec[i]] = curNode;
				curNode->itemId_ = itemVec[i];
				curNode->frequency_ = 1;					
				curNode->parent_ = preNode;			
				curNode->left_	 = NULL;			
				curNode->right_	 = NULL;		
			}else{
				curNode = itor->second;
				++(curNode->frequency_);
			}
			++i;
		}
	}
	void add(const std::vector<ItemID>& itemVec){
		itemset& vec = itemMap_.GetOrderByFrequency(itemVec, minsup_);
		add_without_order( vec );
	}
	void buildFPTree(){
		for( auto itor = itemsetMap_.begin(); itor != itemsetMap_.end(); ++itor){
			itemset& is = itor->second;
			add(is);
		}
	}
	void removeFPSubTree(FrequentPatternTreeNode* fptNode = NULL){
		if( fptNode == NULL )
			fptNode = root_;
		std::queue<FPTreeNode*> FPNodeQueue;
		FPNodeQueue.push( fptNode);
		FPTreeNode* curNode;
		while( !FPNodeQueue.empty() ){
			curNode = FPNodeQueue.front();
			FPNodeQueue.pop();
			std::unordered_map<ItemID, FrequentPatternTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator end = umap.end();
			while( itor != end ){
				FPNodeQueue.push( itor->second );
				++itor;
			}
			if( curNode != root_)
				delete curNode;
			else{
				root_->childrens_.clear();
			}
		}
	}
	void removeConditionalFPSubTree(CFPTreeNode* cfptNode = NULL){
		assert( cfptNode != NULL );
		std::queue<CFPTreeNode*> CFPNodeQueue;
		CFPNodeQueue.push( cfptNode);
		CFPTreeNode* curNode;
		while( !CFPNodeQueue.empty() ){
			curNode = CFPNodeQueue.front();
			CFPNodeQueue.pop();
			std::unordered_map<ItemID, CFPTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, CFPTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, CFPTreeNode*>::iterator end = umap.end();
			while( itor != end ){
				CFPNodeQueue.push( itor->second );
				++itor;
			}
			delete curNode;
		}
	}
	void findItemLink(){
		typedef union { FrequentPatternTableNode* tablePtr; FrequentPatternTreeNode* treePtr; } fpPtr;
		std::unordered_map<ItemID, std::pair<bool, fpPtr> > traceNodePtr;
		std::unordered_map<ItemID, std::pair<bool, fpPtr> >::iterator itorTraceNodePtr;
		int sz = (int)HeaderTable_.size();
		std::vector< std::pair< bool, fpPtr> > vec( sz );
		for( int i = 0; i < sz; ++i){
			FPTableNode& fptn = HeaderTable_[i];
			traceNodePtr[fptn.itemId_].first = false; // treat as the pointer to HeaderTable
			traceNodePtr[fptn.itemId_].second.tablePtr = &(HeaderTable_[i]);
		}
		std::stack<FrequentPatternTreeNode*> sf;	
		std::stack<FrequentPatternTreeNode*> stackForReverse;
		FrequentPatternTreeNode* curNode;
		curNode = root_;
		while( true ){
			std::unordered_map<ItemID, FrequentPatternTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator end = umap.end();
			while( itor != end ){
				stackForReverse.push( itor->second);
				++itor;
			}
			while( !stackForReverse.empty() ){
				sf.push( stackForReverse.top() );
				stackForReverse.pop();
			}
			if( sf.empty() )
				break;
			curNode = sf.top();
			sf.pop();
			ItemID curID	= curNode->itemId_;
			ItemID parentID = curNode->parent_ ? curNode->parent_->itemId_ : INT_MIN;
			//std::cout<<"  item: "<<curID<<" from: "<<parentID<<" freq : "<<curNode->frequency_<<'\n' ;
			itorTraceNodePtr = traceNodePtr.find( curID);
			assert( itorTraceNodePtr != traceNodePtr.end() );
			std::pair<bool, fpPtr>& pr = itorTraceNodePtr->second;
			if( pr.first ){
				 pr.second.treePtr->right_ = curNode;
				 curNode->left_ = pr.second.treePtr;
				 pr.second.treePtr = curNode;
			}else{
				 pr.first = true;
				 pr.second.tablePtr->next_ = curNode;
				 pr.second.treePtr = curNode;
			}
		}
	}
	void buildConditionalFPTree(int indexOfHeaderTable, CFPTreeNode** head, itemset& is){
		int sz = (int)HeaderTable_.size();
		assert( indexOfHeaderTable < sz );
		CFPTreeNode * cfptNode = NULL, * root;
		std::stack< FPTreeNode*> stackForReverse;
		FPTreeNode* curNode = NULL, *siblingNode = NULL;
		int freq = 0;
		ItemID headerItemID	= HeaderTable_[indexOfHeaderTable].itemId_;
		
		is.clear();
		// create root node
		root = new CFPTreeNode;
		root->itemId_		= INT_MIN;
		root->frequency_	= HeaderTable_[indexOfHeaderTable].frequency_;
		root->order_		= INT_MIN;
		root->parent_		= NULL;
		siblingNode			= HeaderTable_[indexOfHeaderTable].next_;
		while( siblingNode != NULL){ 
			curNode = siblingNode;
			freq = curNode->frequency_;
			curNode = curNode->parent_; // skip the sibling node
			while( curNode != NULL ){
				stackForReverse.push( curNode);
				curNode = curNode->parent_;
			}
			cfptNode = root;
			stackForReverse.pop(); // pop empty node (root_)

			while( !stackForReverse.empty() ){
				curNode = stackForReverse.top();
				stackForReverse.pop();
				std::unordered_map<ItemID, CFPTreeNode*>& umap = cfptNode->childrens_;
				std::unordered_map<ItemID, CFPTreeNode*>::iterator itor= umap.find(curNode->itemId_);
				std::unordered_map<ItemID, CFPTreeNode*>::iterator end = umap.end();
				if( itor != end ){
					cfptNode = itor->second;
					cfptNode->frequency_ += freq;
				}else{
					cfptNode	 = new CFPTreeNode;
					ItemID curID = curNode->itemId_;
					int order	 = itemMap_.lookForOrder(curID);
					umap[curID]	= cfptNode;
					cfptNode->frequency_	= freq;
					cfptNode->itemId_		= curID;
					cfptNode->order_		= order;
					cfptNode->parent_		= curNode->parent_;
					if( curID != headerItemID) // skip the header-ItemID
						is.push_back(curID);
				}				
			}
			siblingNode = siblingNode->right_;
		}
		*head = root;
		// std::cout<<" Conditional FPTree Item - " << HeaderTable_[indexOfHeaderTable].itemId_<<" : \n";
		// traceCFPTree(root);
		// traceItem(indexOfHeaderTable);
	}
	void traceCFPTree(CFPTreeNode* root){
		assert( root != NULL );
		std::queue<CFPTreeNode*> CFPNodeQueue;
		CFPNodeQueue.push( root );
		while( !CFPNodeQueue.empty() ){
			CFPTreeNode* curNode = CFPNodeQueue.front();
			CFPNodeQueue.pop();
			std::unordered_map<ItemID, CFPTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, CFPTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, CFPTreeNode*>::iterator end = umap.end();
			while( itor != end ){
				ItemID curID = itor->first;
				ItemID parentID = curNode->itemId_;
				std::cout<<curID<<"  from "<<parentID<<" order :"<<itor->second->order_<<" freq :"<<itor->second->frequency_<<'\n';
				CFPNodeQueue.push( itor->second );
					++itor;
			}
		}
		std::cout<<"end\n";
	}
	void trace(){
		std::queue<FPTreeNode*> FPNodeQueue;
		FPNodeQueue.push( root_);
		while( !FPNodeQueue.empty() ){
			FPTreeNode* curNode = FPNodeQueue.front();
			FPNodeQueue.pop();
			std::unordered_map<ItemID, FrequentPatternTreeNode*>& umap = curNode->childrens_;
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, FrequentPatternTreeNode*>::iterator end = umap.end();
			while( itor != end ){
				std::cout<< itor->first<<"  from "<<curNode->itemId_<<" :"<<itor->second->frequency_<<'\n';
				FPNodeQueue.push( itor->second );
				++itor;
			}
		}
		std::cout<<"end\n";
	}
	void traceItem(size_t indexOfOrder = -1){
		size_t i = 0, sz = (int)HeaderTable_.size();
		if( 0 <= indexOfOrder && indexOfOrder < sz ){
			i = indexOfOrder;
			sz = i + 1;
		}
		FPTreeNode* curNode;
		for( ; i < sz; ++i){
			std::cout<<" cur Item: "<< HeaderTable_[i].itemId_<<" freq : "<< HeaderTable_[i].frequency_<<"  ->  ";
			curNode = HeaderTable_[i].next_;	

			while( curNode != NULL){
				ItemID curID	= curNode->itemId_;
				ItemID parentID = curNode->parent_ ? curNode->parent_->itemId_ : INT_MIN;	
				std::cout<<" ["<<curID<<" freq: "<<curNode->frequency_<<" from "<<parentID<<"]  ";
				curNode = curNode->right_;
			}
			std::cout<<"\n";
		}
		std::cout<<"end\n";
	}
	void printLargestItemset(){
#define SHOW_LARGEST_ITEMSET 0
		std::cout<<" K = #"<<1<<" : has #"<<HeaderTable_.size()<<"  Largest Itemsets.\n";
#if SHOW_LARGEST_ITEMSET
		for( size_t i = 0, sz = HeaderTable_.size(); i < sz; ++i){
			std::cout<<"[ "<<HeaderTable_[i].itemId_<<"]: sup = "<<HeaderTable_[i].frequency_<<'\n';
		}
#endif
		for( size_t i = 0, sz = vecFPSwithDiffLength_.size(); i < sz; ++i ){
			FPSet& fpset = vecFPSwithDiffLength_[i];
			std::cout<<" K = #"<<i+2<<" : has #"<<fpset.size()<<"  Largest Itemsets.\n";
#if SHOW_LARGEST_ITEMSET
			for( auto fp : fpset ){
				std::cout<<fp;
			}
			std::cout<<'\n';
#endif
		}
	}
	void apriori(){
		vecFPSwithDiffLength_.clear();
		vecFPSwithDiffLength_.reserve(10);
		std::vector<FPSet> append;
		CFPTreeNode * root = NULL;
		ItemID itemIDInHeaderTable;
		// for each item-i, compute the FPSet in each ConditionalFP Tree
		size_t sz = HeaderTable_.size();
		for( size_t i = 1; i < sz; ++i){	// skip the first item (the most frequent itemID )
			itemIDInHeaderTable = HeaderTable_[i].itemId_;
			itemset is;
			// build Item-i FP Tree and save "is": the set of items (elements) in this FP tree to is ( exclude Item-i )
			buildConditionalFPTree( i, &root, is);
			if( is.size() == 0 )
				continue;
			//run apriori algorithm
			apriori( root, is, append, itemIDInHeaderTable);
			// append the result to vecFPSwithDiffLength_
			size_t j = 0; // skip append[0] case ( C1 ), start from [1] case ( C2 )
			size_t szJ	= append.size();
			for(; j < szJ; ++j){
				if( j == vecFPSwithDiffLength_.size() ){
					FPSet fpset;
					vecFPSwithDiffLength_.push_back( fpset );
				}
				FPSet& fpset = vecFPSwithDiffLength_[j];
				FPSet& FPSetSrc = append[j];
				for( auto itor = FPSetSrc.begin(), end = FPSetSrc.end(); itor != end; ++itor){
					FrequentPattern fp;
					fp.support_ = itor->support_;
					// add item-i (itemIDInHeaderTable) back to append
					std::vector<int>& to			= fp.data_;
					const std::vector<int>& from	= itor->data_;
					to.reserve( from.size() + 1);
					to.insert(to.end(), from.begin(), from.end() );
					to.push_back( itemIDInHeaderTable );
					fpset.insert( fp );
				}
			}
			// remove the memory space
			removeConditionalFPSubTree(root);
		}
		printLargestItemset();
	}
	/*input: item-i Conditional FP Tree
	**input: item list: the item elements in (item-i) Conditional FP Tree
	**output: append: the Frequent Patterns (exclude item-i)*/
	void apriori(CFPTreeNode * root, const itemset& is, std::vector<FPSet>& vecFPS, int itemIDInHeaderTable){
		if( is.empty() ) // if this is a empty Conditioal FP Tree
			return;
		size_t maxNum = is.size();
		vecFPS.clear();
		vecFPS.reserve( maxNum );
		itemset orderedItemset = itemMap_.GetOrderByFrequency(is, minsup_);
		// K = 1
		FPSet L1;
		for( size_t i = 0, szI = orderedItemset.size(); i < szI; ++i){
			FrequentPattern C1_element;
			int support = 0;
			std::vector<int>& vec = C1_element.data_;
			vec.reserve(1);
			vec.push_back(orderedItemset[i]);
			support = calcSupport( root, vec);
			if( support >= minsup_){
				C1_element.support_	= support;
				L1.insert(C1_element);
			}
		}
		if( L1.empty() )
			return;
		vecFPS.push_back( L1);
		// K > 1
		for( size_t k = 0; k < maxNum - 1; ++k){
			FPSet newFPS = GenerateFrequentItemsetK( root, vecFPS[k] );
			if( newFPS.empty() )
				break;
			vecFPS.push_back( newFPS );
		}
		// print		
#if 0
		for( size_t k = 0, sz = vecFPS.size() ; k < sz; ++k){
			std::cout<<" LK -K#"<<k+2<<" Frequent Pattern ( + "<<itemIDInHeaderTable<<"):\n";
			FPSet& fpset = vecFPS[k];
			for( auto itor = fpset.begin(), end = fpset.end(); itor != end; ++itor ){
				std::cout<<*itor<<'\n';
			}
		}
#endif
	}
	FPSet GenerateFrequentItemsetK(CFPTreeNode * root, const FPSet& LKMinus1){
		// LK-1 X LK-1 => CK => LK
		FPSet ret;		// LK
		FPSet::iterator itor;
		FPSet::iterator end		= ret.end();
		FPSet::iterator itor1	= LKMinus1.begin(); // LK-1
		FPSet::iterator itor2;						// LK-1
		FPSet::iterator end2	= LKMinus1.end();
		FPSet::iterator end1	= end2;			
		--end1;
		for( ; itor1 != end1; ++itor1){
			itor2 = itor1;
			++itor2;
			for( ; itor2 != end2; ++itor2){
				// Generate CK
				const itemset& is1 = itor1->data_; // this vec record the order of the itemsets
				const itemset& is2 = itor2->data_; // this vec record the order of the itemsets
				FrequentPattern fp;
				// Generate the new merging itemset, order: by each item's order
				size_t index1 = 0, index2 = 0, sz = is1.size(); // ( = is2.size() )
				fp.data_.reserve( sz + 1);
				itemset& is = fp.data_;
				int order1;
				int order2;
				while( index1 < sz && index2 < sz ){
					order1 = itemMap_.lookForOrder( is1[index1] );
					order2 = itemMap_.lookForOrder( is2[index2] );
					if( order1 == order2){
						is.push_back( is1[index1] );
						++index1;
						++index2;
					}else if( order1 < order2 ) {
						is.push_back( is1[index1] );
						++index1;
					}else{
						is.push_back( is2[index2] );
						++index2;
					}
				}
				if( index1 < sz ){
					while( index1 < sz ){
						if( is.back() == is1[index1])
							break;
						is.push_back( is1[index1] );
						++index1;
					}
				}else{
					while( index2 < sz ){
						if( is.back() == is2[index2])
							break;
						is.push_back( is2[index2] );
						++index2;
					}
				}
				if( is.size() != ( sz + 1) && ret.find( fp ) == end ){ // compute calcSupport is time-consuming
					continue;
				}
				// Generate LK
				int support = calcSupport(root, is);
				if ( support >= minsup_ ){
					fp.support_ = support;
					ret.insert( fp);
				}
			}
		}
		return ret;
	}
	int calcSupport(CFPTreeNode* subTreeRoot, const itemset& itemsetCandidate, int startCandidIndex = 0){
		int totalSupport = 0;
		int length = (int)itemsetCandidate.size();
		ItemID startCandidItemID		= itemsetCandidate[startCandidIndex];
		int startCandidItemIDOrder	= itemMap_.lookForOrder(startCandidItemID);
		int order					= 0;
		CFPTreeNode* curNode		= NULL;	
		std::queue<CFPTreeNode*> queueCFPT;
		queueCFPT.push(subTreeRoot);
		while( !queueCFPT.empty() ){
			curNode = queueCFPT.front();
			queueCFPT.pop();
			std::unordered_map<ItemID, CFPTreeNode*>& umap	= curNode->childrens_;
			std::unordered_map<ItemID, CFPTreeNode*>::iterator itor= umap.begin();
			std::unordered_map<ItemID, CFPTreeNode*>::iterator end = umap.end();
			for( ; itor != end ; ++itor){
				order = itor->second->order_;
				if( order < startCandidItemIDOrder )
					queueCFPT.push(itor->second);
				else if( order == startCandidItemIDOrder){
					if( startCandidIndex == (length-1) )
						totalSupport += itor->second->frequency_;
					else
						totalSupport += calcSupport( itor->second, itemsetCandidate, startCandidIndex+1);
				}
			}
		}
		return totalSupport;
	}
	void GenerateAssociationRule(float minconf){
		size_t count = 0;
		std::vector< std::vector< PackedAssociationRule > > vecPackedAR; // outter: K, inner: |X U Y|
		vecPackedAR.clear();
		vecPackedAR.reserve( vecFPSwithDiffLength_.size() );
		for( size_t i = 0, sz = vecFPSwithDiffLength_.size(); i < sz; ++i){
			std::vector< PackedAssociationRule > vecPackedAssocRule; // vec of Association Rule with same K (= |X U Y| )
			const FPSet& fpset = vecFPSwithDiffLength_[i];
			vecPackedAssocRule.reserve( fpset.size() );
			for( auto itor = fpset.begin(), end = fpset.end(); itor != end; ++itor ){
				PackedAssociationRule packedRule;
				findRule( *itor, minconf, packedRule);
				if( packedRule.FPsubset_.size() > 0 ){
					vecPackedAssocRule.push_back(packedRule);
					count += packedRule.FPsubset_.size();
				}
			}
			vecPackedAR.push_back(vecPackedAssocRule);
		}
		// unpacked 
		vecAssoc_.clear();
		vecAssoc_.reserve( count );
		for( size_t i = 0, sz = vecPackedAR.size(); i < sz; ++i ){
			for( size_t j = 0, szJ = vecPackedAR[i].size(); j < szJ; ++j ){
				PackedAssociationRule& packedRule	= vecPackedAR[i][j];
				FPSet& fpset						= packedRule.FPsubset_;
				FPSet::iterator itor				= fpset.begin();
				FPSet::iterator end					= fpset.end();
				int supportXY_						= packedRule.fp_.support_;
				itemset& mainset					= packedRule.fp_.data_;
				for( ; itor != end; ++itor){
					AssociationRule assocRule;
					assocRule.X_					= itor->data_;
					assocRule.supportX_				= itor->support_;
					assocRule.supportXY_			= supportXY_;
					itemset& isY					= assocRule.Y_;
					itemset& subset					= assocRule.X_;
					size_t mi = 0, si = 0;
					size_t msz = mainset.size(), ssz = subset.size();
					isY.reserve( msz - ssz );
					while( mi < msz && si < ssz){
						if( mainset[mi]==subset[si])
							++si;
						else
							isY.push_back(mainset[mi]);
						++mi;
					}
					for( ; mi < msz; ++mi){
						isY.push_back(mainset[mi]);
					}
					vecAssoc_.push_back( assocRule );
				}
			}
		}
		std::sort( vecAssoc_.begin(), vecAssoc_.end(), CompAssocRuleByConfidenceDec() );
	}
	void findRule(const FrequentPattern& FP, float minconf, PackedAssociationRule& packedAssocRule){
		const itemset& mainset	= FP.data_;
		size_t sz				= mainset.size();
		int thresh				= int(  FP.support_ /minconf);
		FPSet::iterator itor;
		FPSet::iterator end;
		std::queue< FrequentPattern > queFP;
		std::vector< itemset > vecItemsetTwo; // itemset with two items
		FPSet ret;
		packedAssocRule.fp_			= FP;
		FPSet& fpSubset			= packedAssocRule.FPsubset_;
		assert( mainset.size() > 1 );
		if( sz > 2){
			queFP.push(FP);
			while( !queFP.empty() ){
				FrequentPattern curFP	= queFP.front();
				queFP.pop();
				itemset& curItemset		= curFP.data_;
				size_t sz				= curItemset.size();
				if( sz == 2 )
					break;
				FPSet& fpset			= vecFPSwithDiffLength_[sz-3];
				end						= fpset.end();
				for( size_t i = 0; i < sz; ++i){
					FrequentPattern candid;
					itemset& isTmp		= candid.data_;
					isTmp.reserve(sz - 1);
					for( size_t j = 0; j < sz; ++j ){
						if( j != i )
							isTmp.push_back( curItemset[j]);
					}
					itor = fpset.find(candid);
					if( itor != end && itor->support_ <= thresh ){
						candid.support_ = itor->support_;
						if( isTmp.size() == 2 ){
							vecItemsetTwo.push_back(isTmp);
						}else{
							if( fpSubset.find(candid) != fpSubset.end() )
								queFP.push(*itor);
						}
						fpSubset.insert( candid );
					}
				}
			}
		}else{ // sz == 2
			vecItemsetTwo.push_back(mainset);
		}
		if( vecItemsetTwo.size() ){
			for( size_t j = 0, szJ = vecItemsetTwo.size(); j < szJ;	++j){
				itemset& is = vecItemsetTwo[j];
				for( size_t i = 0; i < 2; ++i){
					int support = itemMap_.lookForFrequency( is[i] );
					if( support <= thresh ){
						FrequentPattern fp;
						itemset& isTmp = fp.data_;
						fp.support_ = support;
						isTmp.push_back( is[i]);
 						fpSubset.insert( fp );
					}
				}
			}
		}						
	}
	void printAssociationRule(){
		for( size_t i = 0, sz = vecAssoc_.size(); i < sz; ++i ){
			std::cout<<vecAssoc_[i]<<'\n';
		}
	}
};


#endif
