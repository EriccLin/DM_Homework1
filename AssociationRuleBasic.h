
#include <vector>
#include <map>
#include <iterator>


#ifndef _ITEM_
#define _ITEM_
typedef int ItemID;
typedef int ItemsetID;
typedef std::vector < ItemID> itemset;
typedef std::map< ItemsetID, itemset, std::less < ItemsetID> > itemsetMap;
typedef itemsetMap::iterator itemset_itor;
#endif

