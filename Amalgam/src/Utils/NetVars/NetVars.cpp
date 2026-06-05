#include "NetVars.h"

#include "../../SDK/Definitions/Interfaces/CHLClient.h"
#include "../Hash/FNV1A.h"

#ifdef GetProp
	#undef GetProp
#endif

void CNetVars::Init()
{
	m_mOffsets.clear();
	m_mProps.clear();

	for (auto pCurrNode = I::Client->GetAllClasses(); pCurrNode; pCurrNode = pCurrNode->m_pNext)
		if (pCurrNode->m_pRecvTable)
			BuildTable(pCurrNode->m_pRecvTable, 0, FNV1A::Hash32(pCurrNode->m_pNetworkName));
}

void CNetVars::BuildTable(RecvTable* pTable, int nBaseOffset, uint32_t uTopLevelClassHash)
{
	if (!pTable)
		return;

	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		RecvProp* pProp = pTable->GetProp(i);
		if (!pProp)
			continue;

		const auto uPropHash = FNV1A::Hash32(pProp->m_pVarName);
		const uint64_t uKey = (uint64_t(uTopLevelClassHash) << 32) | uPropHash;
		const int nOffset = pProp->GetOffset() + nBaseOffset;

		m_mOffsets[uKey] = nOffset;
		m_mProps[uKey] = pProp;

		if (auto pDataTable = pProp->GetDataTable())
		{
			if (pDataTable != pTable)
				BuildTable(pDataTable, nOffset, uTopLevelClassHash);
		}
	}
}

int CNetVars::GetOffset(RecvTable* pTable, const char* szNetVar)
{
	const auto uClassHash = FNV1A::Hash32(pTable->GetName());
	const auto uPropHash = FNV1A::Hash32(szNetVar);
	const uint64_t uKey = (uint64_t(uClassHash) << 32) | uPropHash;

	if (auto it = m_mOffsets.find(uKey); it != m_mOffsets.end())
		return it->second;

	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		RecvProp* pProp = pTable->GetProp(i);
		if (uPropHash == FNV1A::Hash32(pProp->m_pVarName))
		{
			m_mOffsets[uKey] = pProp->GetOffset();
			return pProp->GetOffset();
		}

		if (auto pDataTable = pProp->GetDataTable())
		{
			if (auto nOffset = GetOffset(pDataTable, szNetVar))
			{
				const int nFullOffset = nOffset + pProp->GetOffset();
				m_mOffsets[uKey] = nFullOffset;
				return nFullOffset;
			}
		}
	}

	return 0;
}

int CNetVars::GetNetVar(const char* szClass, const char* szNetVar)
{
	const auto uClassHash = FNV1A::Hash32(szClass);
	const auto uPropHash = FNV1A::Hash32(szNetVar);
	const uint64_t uKey = (uint64_t(uClassHash) << 32) | uPropHash;

	if (auto it = m_mOffsets.find(uKey); it != m_mOffsets.end())
		return it->second;

	for (auto pCurrNode = I::Client->GetAllClasses(); pCurrNode; pCurrNode = pCurrNode->m_pNext)
	{
		if (uClassHash == FNV1A::Hash32(pCurrNode->m_pNetworkName))
		{
			const int nOffset = GetOffset(pCurrNode->m_pRecvTable, szNetVar);
			m_mOffsets[uKey] = nOffset;
			return nOffset;
		}
	}

	return 0;
}

RecvProp* CNetVars::GetProp(RecvTable* pTable, const char* szNetVar)
{
	const auto uClassHash = FNV1A::Hash32(pTable->GetName());
	const auto uPropHash = FNV1A::Hash32(szNetVar);
	const uint64_t uKey = (uint64_t(uClassHash) << 32) | uPropHash;

	if (auto it = m_mProps.find(uKey); it != m_mProps.end())
		return it->second;

	for (int i = 0; i < pTable->GetNumProps(); i++)
	{
		RecvProp* pProp = pTable->GetProp(i);
		if (uPropHash == FNV1A::Hash32(pProp->m_pVarName))
		{
			m_mProps[uKey] = pProp;
			return pProp;
		}

		if (auto pDataTable = pProp->GetDataTable())
		{
			if (pProp = GetProp(pDataTable, szNetVar))
			{
				m_mProps[uKey] = pProp;
				return pProp;
			}
		}
	}

	return nullptr;
}

RecvProp* CNetVars::GetNetProp(const char* szClass, const char* szNetVar)
{
	const auto uClassHash = FNV1A::Hash32(szClass);
	const auto uPropHash = FNV1A::Hash32(szNetVar);
	const uint64_t uKey = (uint64_t(uClassHash) << 32) | uPropHash;

	if (auto it = m_mProps.find(uKey); it != m_mProps.end())
		return it->second;

	for (auto pCurrNode = I::Client->GetAllClasses(); pCurrNode; pCurrNode = pCurrNode->m_pNext)
	{
		if (uClassHash == FNV1A::Hash32(pCurrNode->m_pNetworkName))
		{
			auto pProp = GetProp(pCurrNode->m_pRecvTable, szNetVar);
			m_mProps[uKey] = pProp;
			return pProp;
		}
	}

	return nullptr;
}
