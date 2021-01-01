#include "AppManager.h"
#include "hook.h"
#include <iostream> // cout stuffs
#include <sstream>
#include <thread>
#include <chrono>
#include <csignal>
#include "GlobalHelpers.h"
#include "CPSMission.h"
#include "CTextStringManager.h"
#include "CString.h"
#include "ICPlayer.h"

using namespace std;

// Static
DiscordClient AppManager::DiscordClient;
GAME_STATE AppManager::m_GameState;

// Initialize core of the application
void AppManager::Initialize()
{
	// Attach console
	AllocConsole();
	freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	AppManager::InitHooks();
	CreateThread(0, 0, (LPTHREAD_START_ROUTINE)AppManager::InitDiscord, 0, 0, 0);
}

// Set the respectives hooks
void AppManager::InitHooks()
{
	replaceAddr(0x00DD92BC + 0x18, addr_from_this(&CPSMission::OnNetMsg));
	replaceAddr(0x00DD811C + 0x18, addr_from_this(&CPSMission::OnCharacterSelection));
	replaceAddr(0x00DD440C, addr_from_this(&CPSMission::OnPacketRecv));
}
// Initialize the rich presence for the current game in a independant thread
DWORD WINAPI AppManager::InitDiscord()
{
	discord::ClientId CLIENT_ID(792263872274366504);
	// Try to create discord api instance
	discord::Core* core{};
	int delay = 30000;
	do {
		auto result = discord::Core::Create(CLIENT_ID, DiscordCreateFlags_NoRequireDiscord, &core);
		// Check failure
		if (!core) {
			cout << "Discord Create (Err " << static_cast<int>(result) << ")" << endl;
			// Try again
			this_thread::sleep_for(chrono::milliseconds(delay));
		}
	} while (!core);
	// Success
	DiscordClient.Core.reset(core);

	// Log stuffs
	DiscordClient.Core->SetLogHook(
		discord::LogLevel::Debug, [](discord::LogLevel level, const char* message) {
		cout << "Log(" << static_cast<uint32_t>(level) << "): " << message << endl;
	});

	// Called when the discord client connects to this API
	DiscordClient.Core->UserManager().OnCurrentUserUpdate.Connect([]() {
		// Set current user
		auto result = DiscordClient.Core->UserManager().GetCurrentUser(&DiscordClient.CurrentUser);

		cout << "Connected user: "
			<< DiscordClient.CurrentUser.GetUsername() << "#"
			<< DiscordClient.CurrentUser.GetDiscriminator() << endl;

		// Updates the rich presence
		AppManager::UpdateGameState();
	});

	// Stops this thread on exit
	signal(SIGINT, [](int) {
		m_GameState = GAME_STATE::FINISH;
	});

	// Lock thread checking discord callbacks
	delay = 2000;
	while (GAME_STATE::FINISH)
	{
		DiscordClient.Core->RunCallbacks();
		this_thread::sleep_for(chrono::milliseconds(delay));
	}
	return 0;
}

// Updates the game state and discord rich presence associated
void AppManager::UpdateGameState(GAME_STATE State)
{
	m_GameState = State;
	UpdateDiscord();
}

// Updates the game state and discord rich presence associated
void AppManager::UpdateGameState()
{
	UpdateGameState(m_GameState);
}

// Updates discord rich presence
void AppManager::UpdateDiscord()
{
	// update rich presence only when the state has not been finished
	if (m_GameState != GAME_STATE::FINISH)
	{
		// Check if the discord has been initialized
		if (DiscordClient.Core) {
			discord::Activity activity{};
			// Game state
			switch (m_GameState)
			{
			case GAME_STATE::LOADING:
				activity.SetState("Loading");
				activity.GetAssets().SetLargeImage("logo");
				activity.GetAssets().SetLargeText("SilkroadLatino.com");
				break;
			case GAME_STATE::SERVER_SELECTION:
				activity.SetState("Selecting Server");
				activity.GetAssets().SetLargeImage("logo");
				activity.GetAssets().SetLargeText("SilkroadLatino.com");
				break;
			case GAME_STATE::CHARACTER_SELECTION:
				activity.SetState("Selecting Character");
				activity.GetAssets().SetLargeImage("logo");
				activity.GetAssets().SetLargeText("SilkroadLatino.com");
				break;
			case GAME_STATE::IN_GAME:
				activity.SetState("In Game");
				// Player details
				stringstream details;
				details << n_wstr_to_str(g_CICPlayer->m_Charname) << " Lv." << (int)g_CICPlayer->m_Level;
				// Check guild name
				//wstring guildName = L""; //g_CICPlayer->GetGuildName();
				//if (guildName != L"")
				//{
				//	wcout << "GuildName:" << guildName;
				//	details << " [" << wstr_to_str(guildName) << "]";
				//}
				activity.SetDetails(ss_to_str(details).c_str());
				// Region Name
				stringstream ssRegion;
				ssRegion << g_CICPlayer->m_Region;
				auto nwRegionName = g_CTextStringManager->GetString(ss_to_n_wstr(ssRegion));
				auto regionName = n_wstr_to_str(nwRegionName);
				activity.GetAssets().SetLargeText(regionName.c_str());
				// Region image
				if (regionName == "Jangan") {
					activity.GetAssets().SetLargeImage("loading_zangan");
				}
				else if (regionName == "Donwhang") {
					activity.GetAssets().SetLargeImage("loading_dunwhang");
				}
				else if (regionName == "Hotan") {
					activity.GetAssets().SetLargeImage("loading_hotan");
				}
				else if (regionName == "Samarkand") {
					activity.GetAssets().SetLargeImage("loading_samarkand");
				}
				else if (regionName == "Constantinople") {
					activity.GetAssets().SetLargeImage("loading_constantinople");
				}
				else if (regionName.rfind("Alexandria", 0) == 0) {
					activity.GetAssets().SetLargeImage("loading_alex");
				}
				else {
					activity.GetAssets().SetLargeImage("loading_default");
				}
				// Game icon or Character icon
				activity.GetAssets().SetSmallImage("logo");
				// Game website or Character race
				activity.GetAssets().SetSmallText("SilkroadLatino.com");
				break;
			}
			// Always will be playing mode
			activity.SetType(discord::ActivityType::Playing);
			// Set activity
			DiscordClient.Core->ActivityManager().UpdateActivity(activity, [](discord::Result result) {
				// Check error
				if (result != discord::Result::Ok)
					cout << "Discord Activity (Err " << static_cast<int>(result) << ")" << endl;
			});
		}
	}
}