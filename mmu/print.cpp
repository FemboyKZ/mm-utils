#include "mmu/print.h"
#include "mmu/chat_colors.h"
#include "mmu/recipient_filter.h"

#include <ISmmPlugin.h>
#include <iserver.h>

#include <engine/igameeventsystem.h>
#include <irecipientfilter.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworkserializer.h>
#include <networksystem/netmessage.h>
#include <usermessages.pb.h>

#include <cstdio>
#include <cstring>

extern ISmmAPI *g_SMAPI;
extern IVEngineServer *g_pEngine;
extern INetworkMessages *g_pNetworkMessages;
extern IGameEventSystem *g_pGameEventSystem;
extern INetworkServerService *g_pNetworkServerService;

#define HUD_PRINTTALK 3

namespace mmu
{

	CGlobalVars *GetGameGlobals()
	{
		INetworkGameServer *server = g_pNetworkServerService ? g_pNetworkServerService->GetIGameServer() : nullptr;
		if (!server)
		{
			return nullptr;
		}
		return server->GetGlobals();
	}

	// Cache the TextMsg network message pointer (lazy init)
	static INetworkMessageInternal *GetTextMsgMessage()
	{
		static INetworkMessageInternal *s_pTextMsg = nullptr;
		if (!s_pTextMsg && g_pNetworkMessages)
		{
			s_pTextMsg = g_pNetworkMessages->FindNetworkMessagePartial("TextMsg");
		}
		return s_pTextMsg;
	}

	void SendChatToFilter(IRecipientFilter *filter, const char *text)
	{
		INetworkMessageInternal *pNetMsg = GetTextMsgMessage();
		if (!pNetMsg || !g_pGameEventSystem)
		{
			return;
		}

		CNetMessage *pData = pNetMsg->AllocateMessage();
		if (!pData)
		{
			return;
		}

		auto *pTextMsg = pData->ToPB<CUserMessageTextMsg>();
		pTextMsg->set_dest(HUD_PRINTTALK);
		pTextMsg->add_param(text);

		g_pGameEventSystem->PostEventAbstract(-1, false, filter, pNetMsg, pData, 0);
		g_pNetworkMessages->DeallocateNetMessageAbstract(pNetMsg, pData);
	}

	void SendChatToSlot(int slot, const char *text)
	{
		if (slot < 0 || slot >= kMaxPlayers)
		{
			return;
		}
		CSingleRecipientFilter filter(slot);
		SendChatToFilter(&filter, text);
	}

	void SendConsoleToSlot(int slot, const char *text)
	{
		if (slot < 0 || slot >= kMaxPlayers || !g_pEngine)
		{
			return;
		}
		g_pEngine->ClientPrintf(CPlayerSlot(slot), text);
	}

	void StripChatColors(const char *src, char *dst, size_t dstlen)
	{
		char *out = dst;
		for (; *src && out < dst + dstlen - 1; src++)
		{
			unsigned char c = (unsigned char)*src;
			if (c < 0x01 || c > 0x10)
			{
				*out++ = *src;
			}
		}
		*out = '\0';
	}

	void ChatPrinter::Configure(const Setup &setup)
	{
		m_setup = setup;
	}

	std::string ChatPrinter::Translate(int slot, const char *phrase) const
	{
		if (!m_setup.translations)
		{
			return phrase ? phrase : "";
		}
		std::string lang = (slot >= 0 && m_setup.slotLanguage) ? m_setup.slotLanguage(slot) : "";
		return m_setup.translations->Translate(lang, phrase ? phrase : "");
	}

	void ChatPrinter::ComposeChatLine(char *dst, size_t dstlen, const char *text, bool addPrefix) const
	{
		const char *prefix = (addPrefix && m_setup.chatPrefix) ? m_setup.chatPrefix->c_str() : "";
		const char *reset = (addPrefix && m_setup.resetColorAfterPrefix) ? CHAT_COLOR_DEFAULT : "";
		// Leading space keeps the first color code from being eaten by the client.
		snprintf(dst, dstlen, " %s%s%s", prefix, reset, text);
	}

	// Tagged server console line, colors stripped, exactly one trailing newline.
	void ChatPrinter::ServerConsole(const char *text) const
	{
		char stripped[512];
		StripChatColors(text, stripped, sizeof(stripped));
		size_t len = strlen(stripped);
		while (len > 0 && stripped[len - 1] == '\n')
		{
			stripped[--len] = '\0';
		}
		if (m_setup.conTag && m_setup.conTag[0])
		{
			META_CONPRINTF("[%s] %s\n", m_setup.conTag, stripped);
		}
		else
		{
			META_CONPRINTF("%s\n", stripped);
		}
	}

	void ChatPrinter::SendPrefixedToSlot(int slot, const char *text, bool addPrefix)
	{
		char chatBuf[600];
		ComposeChatLine(chatBuf, sizeof(chatBuf), text, addPrefix);
		SendChatToSlot(slot, chatBuf);
	}

	void ChatPrinter::ChatToSlotV(int slot, const char *fmt, va_list args)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		if (slot < 0)
		{
			ServerConsole(buffer);
			return;
		}
		SendPrefixedToSlot(slot, buffer, true);
	}

	void ChatPrinter::ChatToSlotTV(int slot, const char *phrase, va_list args)
	{
		std::string tmpl = Translate(slot, phrase);
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), tmpl.c_str(), args);

		if (slot < 0)
		{
			ServerConsole(buffer);
			return;
		}
		SendPrefixedToSlot(slot, buffer, true);
	}

	void ChatPrinter::ChatToAllV(const char *fmt, va_list args, bool addPrefix)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		char chatBuf[600];
		ComposeChatLine(chatBuf, sizeof(chatBuf), buffer, addPrefix);

		CMultiRecipientFilter filter;
		CGlobalVars *globals = GetGameGlobals();
		if (globals)
		{
			for (int i = 0; i < globals->maxClients; i++)
			{
				if (!m_setup.slotIsHuman || m_setup.slotIsHuman(i))
				{
					filter.AddRecipient(i);
				}
			}
		}
		SendChatToFilter(&filter, chatBuf);

		ServerConsole(buffer);
	}

	void ChatPrinter::ChatToAllTV(const char *phrase, va_list args)
	{
		CGlobalVars *globals = GetGameGlobals();
		int maxClients = globals ? globals->maxClients : 0;

		for (int i = 0; i < maxClients; i++)
		{
			if (m_setup.slotIsHuman && !m_setup.slotIsHuman(i))
			{
				continue;
			}

			std::string tmpl = Translate(i, phrase);
			char buffer[512];
			va_list copy;
			va_copy(copy, args);
			vsnprintf(buffer, sizeof(buffer), tmpl.c_str(), copy);
			va_end(copy);

			SendPrefixedToSlot(i, buffer, true);
		}

		// Console mirror, rendered once in the server default language.
		std::string conTmpl = Translate(-1, phrase);
		char conBuf[512];
		va_list copy;
		va_copy(copy, args);
		vsnprintf(conBuf, sizeof(conBuf), conTmpl.c_str(), copy);
		va_end(copy);
		ServerConsole(conBuf);
	}

	void ChatPrinter::ChatToPredV(bool (*pred)(int slot), const char *fmt, va_list args)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		char chatBuf[600];
		ComposeChatLine(chatBuf, sizeof(chatBuf), buffer, false);

		CMultiRecipientFilter filter;
		int count = 0;
		CGlobalVars *globals = GetGameGlobals();
		if (globals)
		{
			for (int i = 0; i < globals->maxClients; i++)
			{
				if (m_setup.slotIsHuman && !m_setup.slotIsHuman(i))
				{
					continue;
				}
				if (pred && !pred(i))
				{
					continue;
				}
				filter.AddRecipient(i);
				count++;
			}
		}
		if (count > 0)
		{
			SendChatToFilter(&filter, chatBuf);
		}
	}

	void ChatPrinter::ChatToPredTV(bool (*pred)(int slot), const char *phrase, va_list args)
	{
		CGlobalVars *globals = GetGameGlobals();
		if (!globals)
		{
			return;
		}

		for (int i = 0; i < globals->maxClients; i++)
		{
			if (m_setup.slotIsHuman && !m_setup.slotIsHuman(i))
			{
				continue;
			}
			if (pred && !pred(i))
			{
				continue;
			}

			std::string tmpl = Translate(i, phrase);
			char buffer[512];
			va_list copy;
			va_copy(copy, args);
			vsnprintf(buffer, sizeof(buffer), tmpl.c_str(), copy);
			va_end(copy);

			SendPrefixedToSlot(i, buffer, true);
		}
	}

	void ChatPrinter::ReplyV(int slot, const char *fmt, va_list args)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		if (slot < 0)
		{
			ServerConsole(buffer);
			return;
		}
		if (slot >= kMaxPlayers)
		{
			return;
		}

		char consoleBuffer[512];
		snprintf(consoleBuffer, sizeof(consoleBuffer), "[%s] %s", m_setup.conTag ? m_setup.conTag : "", buffer);
		SendConsoleToSlot(slot, consoleBuffer);

		SendPrefixedToSlot(slot, buffer, true);
	}

	void ChatPrinter::ReplyTV(int slot, const char *phrase, va_list args)
	{
		std::string tmpl = Translate(slot, phrase);
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), tmpl.c_str(), args);

		if (slot < 0)
		{
			ServerConsole(buffer);
			return;
		}
		if (slot >= kMaxPlayers)
		{
			return;
		}

		char consoleBuffer[512];
		snprintf(consoleBuffer, sizeof(consoleBuffer), "[%s] %s", m_setup.conTag ? m_setup.conTag : "", buffer);
		SendConsoleToSlot(slot, consoleBuffer);

		SendPrefixedToSlot(slot, buffer, true);
	}

	void ChatPrinter::ClientConsoleV(int slot, const char *fmt, va_list args)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		if (slot < 0)
		{
			META_CONPRINTF("%s", buffer);
			return;
		}
		SendConsoleToSlot(slot, buffer);
	}

	void ChatPrinter::ClientConsoleTV(int slot, const char *phrase, va_list args)
	{
		std::string tmpl = Translate(slot, phrase);
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), tmpl.c_str(), args);

		if (slot < 0)
		{
			META_CONPRINTF("%s", buffer);
			return;
		}
		SendConsoleToSlot(slot, buffer);
	}

	void ChatPrinter::ConsoleToAllV(const char *fmt, va_list args)
	{
		char buffer[512];
		vsnprintf(buffer, sizeof(buffer), fmt, args);

		CGlobalVars *globals = GetGameGlobals();
		if (!globals)
		{
			return;
		}
		for (int i = 0; i < globals->maxClients; i++)
		{
			if (m_setup.slotIsHuman && !m_setup.slotIsHuman(i))
			{
				continue;
			}
			SendConsoleToSlot(i, buffer);
		}
	}

} // namespace mmu
