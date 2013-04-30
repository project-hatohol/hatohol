#ifndef Synchronizer_h
#define Synchronizer_h

#include <glib.h>
#include <MutexLock.h>

class Synchronizer {
	mlpl::MutexLock *m_mutex;
public:
	Synchronizer(void);
	virtual ~Synchronizer();
	void lock(void);
	void unlock(void);
	void reset(void);
	bool trylock(void);
	bool isLocked(void);
	void wait(void);
};

#endif // Synchronizer_h
