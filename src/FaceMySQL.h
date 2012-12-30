#ifndef FaceMySQL_h
#define FaceMySQL_h

#include <vector>
#include <string>
using namespace std;

#include <gio/gio.h>
#include "Utils.h"
#include "FaceBase.h"

class FaceMySQL : public FaceBase {
	GSocket *m_socket;
	int      m_port;

	size_t parseCmdArgPort(CommandLineArg &cmdArg, size_t idx);
protected:
	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

public:
	FaceMySQL(CommandLineArg &cmdArg);
	virtual ~FaceMySQL();
};

#endif // FaceMySQL_h
