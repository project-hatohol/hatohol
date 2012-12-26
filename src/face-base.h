#ifndef face_base_h
#define face_base_h

class face_base;

struct face_thread_arg_t {
	face_base *obj;
};

class face_base {
	GThread *m_thread;
	
	static gpointer thread_starter(gpointer data);
protected:
	// virtual methods
	virtual gpointer main_thread(face_thread_arg_t *arg) = 0;
public:
	face_base(void);
	virtual ~face_base();
	void start(void);
};

#endif // face_base_h

