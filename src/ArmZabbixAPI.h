#ifndef ArmZabbixAPI_h
#define ArmZabbixAPI_h

#include "ArmBase.h"

class ArmZabbixAPI : public ArmBase
{
public:
	ArmZabbixAPI(CommandLineArg &cmdArg);

protected:
	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);
};

#endif // ArmZabbixAPI_h

