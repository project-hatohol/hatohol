#ifndef Synchronizer_h
#define Synchronizer_h

#include <glib.h>

class Synchronizer {
	GMutex g_mutex;
public:
	Synchronizer(void);
	virtual ~Synchronizer();
	void lock(void);
	void unlock(void);
	void forceUnlock(void);
	bool trylock(void);
	bool isLocked(void);
	void wait(void);
};

#endif // Synchronizer_h
