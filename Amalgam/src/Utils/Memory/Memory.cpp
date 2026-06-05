#include "Memory.h"

#include <format>
#include <Psapi.h>
#include <immintrin.h>

std::vector<byte> CMemory::PatternToByte(const char* szPattern)
{
	std::vector<byte> vPattern = {};

	const auto pStart = const_cast<char*>(szPattern);
	const auto pEnd = const_cast<char*>(szPattern) + strlen(szPattern);
	for (char* pCurrent = pStart; pCurrent < pEnd; ++pCurrent)
		vPattern.push_back(byte(std::strtoul(pCurrent, &pCurrent, 16)));

	return vPattern;
}

std::vector<int> CMemory::PatternToInt(const char* szPattern)
{
	std::vector<int> vPattern = {};

	const auto pStart = const_cast<char*>(szPattern);
	const auto pEnd = const_cast<char*>(szPattern) + strlen(szPattern);
	for (char* pCurrent = pStart; pCurrent < pEnd; ++pCurrent)
	{
		if (*pCurrent == '?') // Is current byte a wildcard? Simply ignore that that byte later
		{
			++pCurrent;
			if (*pCurrent == '?') // Check if following byte is also a wildcard
				++pCurrent;

			vPattern.push_back(-1);
		}
		else
			vPattern.push_back(std::strtoul(pCurrent, &pCurrent, 16));
	}

	return vPattern;
}

uintptr_t CMemory::FindSignature(const char* szModule, const char* szPattern)
{
	if (const auto hModule = GetModuleHandle(szModule))
	{
		// Get module information to search in the given module
		MODULEINFO lpModuleInfo;
		if (!GetModuleInformation(GetCurrentProcess(), hModule, &lpModuleInfo, sizeof(MODULEINFO)))
			return 0x0;

		// The region where we will search for the byte sequence
		const auto dwImageSize = lpModuleInfo.SizeOfImage;

		// Check if the image is faulty
		if (!dwImageSize)
			return 0x0;

		// Convert IDA-Style signature to a byte sequence
		const auto vPattern = PatternToInt(szPattern);
		const auto iPatternSize = vPattern.size();
		const int* iPatternBytes = vPattern.data();

		if (!iPatternSize)
			return 0x0;

		const auto pImageBytes = reinterpret_cast<byte*>(hModule);

		// SIMD first-byte prefilter: collect candidate offsets in 16-byte chunks
		// and only do the expensive full pattern check at those offsets.
		const byte firstByte = iPatternBytes[0] == -1 ? 0 : byte(iPatternBytes[0]);
		const bool bFirstWild = iPatternBytes[0] == -1;

		std::vector<size_t> candidates;
		candidates.reserve(64);

		const size_t dwScanLimit = dwImageSize > iPatternSize ? dwImageSize - iPatternSize : 0;

		if (bFirstWild)
		{
			for (size_t i = 0; i < dwScanLimit; ++i)
				candidates.push_back(i);
		}
		else
		{
			const __m128i needle = _mm_set1_epi8(char(firstByte));
			size_t i = 0;
			const size_t dwSimdEnd = dwScanLimit >= 16 ? dwScanLimit - 16 : 0;
			for (; i < dwSimdEnd; i += 16)
			{
				const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pImageBytes + i));
				const int mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, needle));
				if (!mask)
					continue;
				for (int k = 0; k < 16; ++k)
				{
					if (mask & (1 << k))
					{
						const size_t offset = i + k;
						if (offset < dwScanLimit)
							candidates.push_back(offset);
					}
				}
			}
			for (; i < dwScanLimit; ++i)
			{
				if (pImageBytes[i] == firstByte)
					candidates.push_back(i);
			}
		}

		// Now loop through all candidates and check if the full byte sequence matches
		for (auto i : candidates)
		{
			auto bFound = true;

			for (size_t j = 0; j < iPatternSize; ++j)
			{
				if (pImageBytes[i + j] != iPatternBytes[j] && iPatternBytes[j] != -1)
				{
					bFound = false;
					break;
				}
			}

			if (bFound)
				return uintptr_t(&pImageBytes[i]);
		}

		return 0x0;
	}

	return 0x0;
}

using CreateInterfaceFn = void*(*)(const char* pName, int* pReturnCode);

PVOID CMemory::FindInterface(const char* szModule, const char* szObject)
{
	const auto CreateInterface = GetModuleExport<CreateInterfaceFn>(szModule, "CreateInterface");
	return CreateInterface(szObject, nullptr);
}

std::string CMemory::GetModuleOffset(uintptr_t uAddress)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCSTR(uAddress), &hModule))
		return std::format("{:#x}", uAddress);

	uintptr_t uBase = uintptr_t(hModule);
	if (char buffer[MAX_PATH]; GetModuleBaseName(GetCurrentProcess(), hModule, buffer, sizeof(buffer) / sizeof(char)))
		return std::format("{}+{:#x}", buffer, uAddress - uBase);

	return std::format("{:#x}+{:#x}", uBase, uAddress - uBase);
}

uintptr_t CMemory::GetOffsetFromBase(uintptr_t uAddress)
{
	HMODULE hModule;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, LPCSTR(uAddress), &hModule))
		return -1;

	uintptr_t uBase = uintptr_t(hModule);
	return uAddress - uBase;
}