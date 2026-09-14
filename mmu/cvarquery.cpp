#include "mmu/cvarquery.h"
#include "mmu/log.h"
#include "mmu/recipient_filter.h"
#include "mmu/server_client.h"

#include <ISmmPlugin.h>
#include <eiface.h>
#include <khook.hpp>

#include <engine/igameeventsystem.h>
#include <netmessages.pb.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include <networksystem/netmessage.h>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>

extern IVEngineServer *g_pEngine;
extern INetworkMessages *g_pNetworkMessages;
extern IGameEventSystem *g_pGameEventSystem;

namespace
{
	constexpr int kMaxPlayers = 64;

	// Reserved for the two convars queried on connect.
	constexpr int kLanguageCounter = 0xFFFF;
	constexpr int kOperatingSystemCounter = 0xFFFE;

	struct ClientData
	{
		std::unordered_map<int, mmu::cvarquery::Callback> pending;
		std::string language;
		std::string operatingSystem;
	};

	std::array<ClientData, kMaxPlayers> s_clients;

	bool s_hooked = false;

	// Every plugin linking this hooks the same vtable entry and so sees every response.
	// A per module tag in the cookie's high bits stops one plugin from answering another's cookie.
	int s_cookieTag = 0;
	int s_counter = 0;

	int NextCookie()
	{
		s_counter++;
		// 0 is unused, and the top two counter values belong to the connect time queries.
		if (s_counter <= 0 || s_counter >= kOperatingSystemCounter)
		{
			s_counter = 1;
		}
		return (s_cookieTag << 16) | s_counter;
	}

	KHook::Return<bool> OnRespondCvarValue(CServerSideClient *client, const CNetMessagePB<CCLCMsg_RespondCvarValue> &msg)
	{
		const int slot = mmu::serverclient::Slot(client);
		const int cookie = msg.cookie();
		if (slot < 0 || slot >= kMaxPlayers || (cookie >> 16) != s_cookieTag)
		{
			return {KHook::Action::Ignore, true};
		}

		ClientData &data = s_clients[slot];
		switch (cookie & 0xFFFF)
		{
			case kLanguageCounter:
				data.language = msg.value();
				break;

			case kOperatingSystemCounter:
				data.operatingSystem = msg.value();
				break;

			default:
			{
				auto it = data.pending.find(cookie);
				if (it != data.pending.end())
				{
					// Taken out before it runs so the callback is free to start another query.
					mmu::cvarquery::Callback callback = std::move(it->second);
					data.pending.erase(it);
					callback(slot, static_cast<mmu::cvarquery::Status>(msg.status_code()), msg.name().c_str(), msg.value().c_str());
				}
				break;
			}
		}

		return {KHook::Action::Ignore, true};
	}

	KHook::Virtual<CServerSideClient, bool, const CNetMessagePB<CCLCMsg_RespondCvarValue> &>
		s_respondHook(mmu::serverclient::kProcessRespondCvarValueIndex, nullptr, &OnRespondCvarValue);

	// False means nothing went on the wire, so no callback is owed.
	bool SendQuery(int slot, const char *cvarName, int cookie)
	{
		if (!s_hooked || !cvarName || slot < 0 || slot >= kMaxPlayers)
		{
			return false;
		}
		if (!g_pEngine || !g_pEngine->GetPlayerNetInfo(CPlayerSlot(slot)))
		{
			return false;
		}
		if (!g_pNetworkMessages || !g_pGameEventSystem)
		{
			return false;
		}

		INetworkMessageInternal *pNetMsg = g_pNetworkMessages->FindNetworkMessagePartial("CSVCMsg_GetCvarValue");
		if (!pNetMsg)
		{
			return false;
		}

		CNetMessage *pData = pNetMsg->AllocateMessage();
		if (!pData)
		{
			return false;
		}

		auto *msg = pData->ToPB<CSVCMsg_GetCvarValue>();
		msg->set_cookie(cookie);
		msg->set_cvar_name(cvarName);

		CSingleRecipientFilter filter(slot);
		g_pGameEventSystem->PostEventAbstract(-1, false, &filter, pNetMsg, pData, 0);
		g_pNetworkMessages->DeallocateNetMessageAbstract(pNetMsg, pData);

		return true;
	}
} // namespace

namespace mmu
{
	namespace cvarquery
	{
		bool Init(const void *engineModuleAnchor)
		{
			if (s_hooked)
			{
				return true;
			}

			if (!serverclient::Resolve(engineModuleAnchor))
			{
				MMU_LOG_WARN("CServerSideClient vtable not found, client convar queries are disabled.\n");
				return false;
			}

			// From this module's own data address, so two plugins in one process differ.
			const uintptr_t seed = reinterpret_cast<uintptr_t>(&s_clients);
			s_cookieTag = static_cast<int>(((seed >> 4) ^ (seed >> 21)) & 0x7FFF);
			if (s_cookieTag == 0)
			{
				s_cookieTag = 1;
			}

			s_respondHook.AddGlobal(serverclient::HookTarget());
			s_hooked = true;
			return true;
		}

		void Shutdown()
		{
			if (s_hooked)
			{
				s_respondHook.RemoveGlobal(serverclient::HookTarget());
				s_hooked = false;
			}

			for (ClientData &data : s_clients)
			{
				data = ClientData();
			}
		}

		bool Query(int slot, const char *cvarName, Callback callback)
		{
			if (!callback)
			{
				return false;
			}

			const int cookie = NextCookie();
			if (!SendQuery(slot, cvarName, cookie))
			{
				return false;
			}

			s_clients[slot].pending[cookie] = std::move(callback);
			return true;
		}

		void OnClientConnected(int slot, bool fakePlayer)
		{
			if (slot < 0 || slot >= kMaxPlayers)
			{
				return;
			}

			// Slots get reused, drop the previous occupant's answers.
			s_clients[slot] = ClientData();
			if (fakePlayer)
			{
				return;
			}

			SendQuery(slot, "cl_language", (s_cookieTag << 16) | kLanguageCounter);
			SendQuery(slot, "engine_ostype", (s_cookieTag << 16) | kOperatingSystemCounter);
		}

		void OnClientDisconnect(int slot)
		{
			if (slot >= 0 && slot < kMaxPlayers)
			{
				s_clients[slot] = ClientData();
			}
		}

		const char *GetClientLanguage(int slot)
		{
			if (slot < 0 || slot >= kMaxPlayers || s_clients[slot].language.empty())
			{
				return nullptr;
			}
			return s_clients[slot].language.c_str();
		}

		const char *GetClientOS(int slot)
		{
			if (slot < 0 || slot >= kMaxPlayers || s_clients[slot].operatingSystem.empty())
			{
				return nullptr;
			}
			return s_clients[slot].operatingSystem.c_str();
		}
	} // namespace cvarquery
} // namespace mmu
