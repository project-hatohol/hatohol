#ifndef AsuraThreadBase_h
#define AsuraThreadBase_h

#include <glib.h>
#include <Utils.h>

class AsuraThreadBase;

struct AsuraThreadArg {
	AsuraThreadBase *obj;
};

class AsuraThreadBase {
public:
	AsuraThreadBase(void);
	virtual ~AsuraThreadBase();
	void start(void);
protected:
	// virtual methods
	virtual gpointer mainThread(AsuraThreadArg *arg) = 0;

private:
	GThread *m_thread;

	static gpointer threadStarter(gpointer data);
};

#endif // AsuraThreadBase_h

