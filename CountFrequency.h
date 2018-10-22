
#include <iostream>
#include <map>
#include <vector>
#include <algorithm>
#include <cassert>
#include "AssociationRuleBasic.h"

#ifndef _COUTN_FREQUENCY_H_
#define _COUTN_FREQUENCY_H_


class CountFrequency {
	struct CompBySecondDec {
		bool operator () ( const std::pair< int, int>& lhs, const std::pair< int, int>& rhs) const {
			return lhs.second > rhs.second;
		}
	};
	struct CompBySecondAsc {
		bool operator () ( const std::pair< int, int>& lhs, const std::pair< int, int>& rhs) const {
			return lhs.second < rhs.second;
		}
	};
	typedef std::map< ItemID, std::pair<int, int>, std::less<int> > objMap;
	objMap omap_;
	std::vector< std::pair<ItemID, int> > info_;
public:
	CountFrequency(){};
	~CountFrequency(){};
	CountFrequency(const CountFrequency& cf){
		*this = cf;
	}
	void add(int obj){
		objMap::iterator itor;
		objMap::iterator end = omap_.end();
		itor = omap_.find( obj );
		if ( itor == end ) // init frequency & order
			omap_[obj] = std::make_pair(1, 0);
		else // increment frequency by one
			++(itor->second.first);
	}
	void SortByFrequency(){
		info_.clear();
		info_.reserve( omap_.size() );
		objMap::iterator itor = omap_.begin();
		objMap::iterator end  = omap_.end();
		for( int i = 0, sz = (int)omap_.size(); i < sz; ++i, ++itor) {
			// make pair of itemID & frequeucy
			info_.push_back( std::make_pair( itor->first, itor->second.first) );
		}
		// sort the order by frequeucy dec: e.g. 36, 12, 9, 3, ...
		std::sort( info_.begin(), info_.end(), CompBySecondDec());

		for( int i = 0, sz = (int)omap_.size(); i < sz; ++i){
			int id = info_[i].first;
			itor = omap_.find(id);
			// assign the order for each itemID by asc
			itor->second.second = i;
		}
	}
	std::vector<int> GetOrderByFrequency(const std::vector<int>& vec, int thresh = 0) {
		std::vector<int> ret;
		std::vector< std::pair<int, int> > tmp;
		objMap::iterator itor;
		int itemID;
		ret.reserve( vec.size() );
		tmp.reserve( vec.size() );
		for ( int i = 0, sz = (int)vec.size(); i < sz; ++i){
			itemID = vec[i];
			itor = omap_.find(itemID);
			// 1. make pair of itemID & order
			// 2. check each itemID has frequency higher than thresh
			if (itor->second.first >= thresh)
				tmp.push_back( std::make_pair(itemID, itor->second.second) );
		}
		// sort by the order asc
		std::sort( tmp.begin(), tmp.end(), CompBySecondAsc());
		for ( int i = 0, sz = (int)tmp.size(); i < sz; ++i){
			ret.push_back( tmp[i].first);
		}
		return ret;
	}
	std::vector< std::pair<int, int> > objVector(int thresh){
		thresh = thresh > 0 ? thresh : 1;
		int index = (int)omap_.size();
		if( thresh > 1){
			for( int i = 0, sz = (int)omap_.size(); i < sz; ++i){
				if( info_[i].second < thresh){
					index = i;
					break;
				}
			}
		}
		std::vector< std::pair<int, int> > vec( info_.begin(), info_.begin() + index );
		return vec;
	}
	int getNumOfDiffItems() const{
		return (int)info_.size();
	}
	inline int lookForOrder(int itemID){
		objMap::iterator itor;
		objMap::iterator end = omap_.end();
		itor = omap_.find( itemID);
		assert( itor != end );
		return itor == end ? INT_MIN : itor->second.second;
	}
	inline int lookForFrequency(int itemID){
		objMap::iterator itor;
		objMap::iterator end = omap_.end();
		itor = omap_.find( itemID);
		assert( itor != end );
		return itor == end ? INT_MIN : itor->second.first;
	}
	CountFrequency& operator = (const CountFrequency& cf){
		this->omap_ = cf.omap_;
		this->info_ = cf.info_;
		return *this;
	}
};

#endif
