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
	gboolean trylock(void);
	void wait(void);
};

#endif // Synchronizer_h
