#ifndef UsedCountable_h
#define UsedCountable_h

#include <glib.h>

class UsedCountable {
public:
	void ref(void);
	void unref(void);

protected:
	UsedCountable(int initialUsedCount = 1);
	virtual ~UsedCountable();

	void readLock(void);
	void readUnlock(void);
	void writeLock(void);
	void writeUnlock(void);

	int getUsedCount(void);

private:
	int          m_usedCount;
	GRWLock      m_lock;
};

#endif // UsedCountable_h
