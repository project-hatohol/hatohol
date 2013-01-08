#ifndef UsedCountable_h
#define UsedCountable_h

#include "ReadWriteLock.h"

class UsedCountable : public ReadWriteLock {
public:
	void ref(void) const;
	void unref(void);
	int getUsedCount(void) const;

protected:
	UsedCountable(int initialUsedCount = 1);
	virtual ~UsedCountable();

private:
	mutable int m_usedCount;
};

#endif // UsedCountable_h
