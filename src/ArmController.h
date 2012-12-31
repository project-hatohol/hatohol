#ifndef ArmController_h
#define ArmController_h

#include <map>
using namespace std;

#include <SmartQueue.h>
using namespace mlpl;

#include "AsuraThreadBase.h"

class ArmController;
typedef bool (ArmController::*ArmFactory)(string &arg);

class ArmController : public AsuraThreadBase {
public:
	ArmController(CommandLineArg &cmdArg);

protected:
	bool factoryZabbix(string &arg);

	// virtual methods
	gpointer mainThread(AsuraThreadArg *arg);

private:
	SmartQueue m_cmdQueue;
	SmartQueue m_armQueue;
	map<string, ArmFactory> m_armFactoryMap;

	size_t parseCmdArgArm(CommandLineArg &cmdArg, size_t idx);
};

#endif // ArmController_h
