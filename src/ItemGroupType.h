#ifndef ItemGroupType_h
#define ItemGroupType_h

#include <set>
using namespace std;

#include "ItemData.h"

/**
 * Methods except constructor in this class are 'const'. So they are
 * multi-thread safe.
 */
class ItemGroupType {
public:
	/**
	 * itemDataVector must not be changed by other thread
	 * while this function is being called.
	 */
	ItemGroupType(const ItemDataVector &itemDataVector);

	virtual ~ItemGroupType();
	const size_t getSize() const;
	ItemDataType getType(size_t index) const;
	bool operator<(const ItemGroupType &itemGroupType) const;
	bool operator==(const ItemGroupType &itemGroupType) const;
	bool operator!=(const ItemGroupType &itemGroupType) const;

private:
	vector<ItemDataType> m_typeVector;
};

struct ItemGroupTypeSetComp {
	bool operator()(const ItemGroupType *t0, const ItemGroupType *t1) const;
};

typedef set<ItemGroupType *, ItemGroupTypeSetComp> ItemGroupTypeSet;
typedef ItemGroupTypeSet::iterator       ItemGroupTypeSetIterator;
typedef ItemGroupTypeSet::const_iterator ItemGroupTypeSetConstIterator;

#endif // ItemGroupType_h

