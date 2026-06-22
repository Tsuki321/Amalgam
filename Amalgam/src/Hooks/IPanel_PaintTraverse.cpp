#include "../SDK/SDK.h"

MAKE_HOOK(IPanel_PaintTraverse, U::Memory.GetVirtual(I::Panel, 41), void,
	void* rcx, VPANEL vguiPanel, bool forceRepaint, bool allowForce)
{
	DEBUG_RETURN(IPanel_PaintTraverse, rcx, vguiPanel, forceRepaint, allowForce);

	if (!Vars::Visuals::UI::StreamerMode.Value)
		return CALL_ORIGINAL(rcx, vguiPanel, forceRepaint, allowForce);

	// Cache panel name hashes to avoid repeated strlen() and FNV1A calculations
	static std::unordered_map<VPANEL, uint32_t> s_mPanelHashCache;

	uint32_t uHash;
	auto it = s_mPanelHashCache.find(vguiPanel);
	if (it != s_mPanelHashCache.end())
	{
		uHash = it->second;
	}
	else
	{
		uHash = FNV1A::Hash32(I::Panel->GetName(vguiPanel));
		s_mPanelHashCache[vguiPanel] = uHash;
	}

	switch (uHash)
	{
	case FNV1A::Hash32Const("SteamFriendsList"):
	case FNV1A::Hash32Const("avatar"):
	case FNV1A::Hash32Const("RankPanel"):
	case FNV1A::Hash32Const("ModelContainer"):
	case FNV1A::Hash32Const("ServerLabelNew"):
		return;
	}

	CALL_ORIGINAL(rcx, vguiPanel, forceRepaint, allowForce);
}