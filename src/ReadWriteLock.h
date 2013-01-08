#ifndef ReadWriteLock_h
#define ReadWriteLock_h

#include <glib.h>

class ReadWriteLock {
public:
	ReadWriteLock(void);
	virtual ~ReadWriteLock();

	void readLock(void) const;
	void readUnlock(void) const;
	void writeLock(void) const;
	void writeUnlock(void) const;

private:
	mutable GRWLock      m_lock;
};

#endif // ReadWriteLock_h

