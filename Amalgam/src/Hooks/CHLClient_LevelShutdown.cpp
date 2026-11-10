#include "../SDK/SDK.h"

#include "../Features/EnginePrediction/EnginePrediction.h"
#include "../Features/Backtrack/Backtrack.h"
#include "../Features/Spectate/Spectate.h"

MAKE_HOOK(CHLClient_LevelShutdown, U::Memory.GetVirtual(I::Client, 7), void,
	void* rcx)
{
	DEBUG_RETURN(CHLClient_LevelShutdown, rcx);

	H::Entities.Clear(true);
	F::Backtrack.Reset();
	F::EnginePrediction.Unload();
	F::Spectate.Reset();

	CALL_ORIGINAL(rcx);
}
