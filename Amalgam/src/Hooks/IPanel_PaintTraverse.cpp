#include "../SDK/SDK.h"

MAKE_HOOK(IPanel_PaintTraverse, U::Memory.GetVirtual(I::Panel, 41), void,
	void* rcx, VPANEL vguiPanel, bool forceRepaint, bool allowForce)
{
	DEBUG_RETURN(IPanel_PaintTraverse, rcx, vguiPanel, forceRepaint, allowForce);

	if (!Vars::Visuals::UI::StreamerMode.Value)
		return CALL_ORIGINAL(rcx, vguiPanel, forceRepaint, allowForce);

	// Cache panel name hashes to avoid repeated strlen() and FNV1A calculations
	static std::unordered_map<VPANEL, uint32_t> s_mPanelHashCache;

	// Periodically clear cache to prevent unbounded growth from destroyed/recycled panels
	static int s_iFrameCount = 0;
	if (++s_iFrameCount > 10000)
	{
		s_mPanelHashCache.clear();
		s_iFrameCount = 0;
	}

	uint32_t uHash;
	auto it = s_mPanelHashCache.find(vguiPanel);
	if (it != s_mPanelHashCache.end())
	{
		uHash = it->second;
	}
	else
	{
		const char* sName = I::Panel->GetName(vguiPanel);
		if (!sName)
			return CALL_ORIGINAL(rcx, vguiPanel, forceRepaint, allowForce);

		uHash = FNV1A::Hash32(sName);
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