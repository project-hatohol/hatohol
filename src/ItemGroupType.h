#ifndef ItemGroupType_h
#define ItemGroupType_h

#include "ItemData.h"

class ItemGroupType {
public:
	ItemGroupType(void);
	virtual ~ItemGroupType();
	void addType(const ItemData *data);
	bool operator==(const ItemGroupType &itemGroupType) const;
	const vector<ItemDataType> &getTypeVector(void) const;

private:
	vector<ItemDataType> m_typeVector;
};

#endif // ItemGroupType_h

