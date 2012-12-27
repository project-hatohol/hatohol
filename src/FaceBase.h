#ifndef FaceBase_h
#define FaceBase_h

class FaceBase;

struct FaceThreadArg {
	FaceBase *obj;
};

class FaceBase {
	GThread *m_thread;
	
	static gpointer threadStarter(gpointer data);
protected:
	// virtual methods
	virtual gpointer mainThread(FaceThreadArg *arg) = 0;
public:
	FaceBase(void);
	virtual ~FaceBase();
	void start(void);
};

#endif // FaceBase_h

