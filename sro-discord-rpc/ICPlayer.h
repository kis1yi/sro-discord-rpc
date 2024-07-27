#pragma once
#include "CString.h"

// Possible types for job mode
enum JOB_TYPE : char {
	THIEF = 2,
	TRADER = 3,
	HUNTER = 1,
	NONE = 0
};

// Reflects a player entity in game
class CICPlayer
{
public:
	/// Entity
	unsigned char pad_0x00[124];
	unsigned short m_Region;
	unsigned char pad_0x7E[122];
	uint32_t m_EntityUniqueId;
	unsigned char pad_0xFC[344];
	/// Character
	unsigned char pad_0x254[785]; //[697]
	JOB_TYPE m_JobType;
	unsigned char pad_0x566[638]; //0x50E
	/// User
	unsigned char pad_0x7E4[160]; //0x78C
	/// Player
	unsigned char pad_0x884[44]; //0x82C
	n_wstring m_Charname;
	unsigned char m_Level;
	unsigned char pad_0x8CD[6216]; //0x869[6319]
public:
	// Returns the guild name or an empty string
	const n_wstring GetGuildName()
	{
		return reinterpret_cast<const n_wstring&(__thiscall*)(CICPlayer*)>(0x009EA560)(this);
	};
};

// The current player selected in game
#define g_CICPlayer (*((CICPlayer**)0x00FA57A4))