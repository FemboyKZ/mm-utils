#ifndef _INCLUDE_MMU_PRINT_H_
#define _INCLUDE_MMU_PRINT_H_

#include "mmu/translations.h"

#include <cstdarg>
#include <cstddef>
#include <string>

class IRecipientFilter;
struct CGlobalVars;

namespace mmu
{
	constexpr int kMaxPlayers = 64;

	// CGlobalVars accessor, only valid during an active game.
	CGlobalVars *GetGameGlobals();

	// Send a HUD_PRINTTALK user message to a recipient filter. `text` is sent as-is.
	void SendChatToFilter(IRecipientFilter *filter, const char *text);

	// Send `text` as-is to one player's chat.
	void SendChatToSlot(int slot, const char *text);

	// Send `text` to one player's console via ClientPrintf.
	void SendConsoleToSlot(int slot, const char *text);

	// Strip Source chat color codes (0x01-0x10).
	void StripChatColors(const char *src, char *dst, size_t dstlen);

	// Prefix/translation-aware chat and console output for one plugin.
	// All methods take va_list, plugin-side printf-style wrappers own va_start/va_end.
	class ChatPrinter
	{
	public:
		struct Setup
		{
			// Phrase tables for the T variants. Null passes phrases through untranslated.
			const Translations *translations = nullptr;
			// Per-slot phrase-file language key resolver.
			std::string (*slotLanguage)(int slot) = nullptr;
			// Live pointer to the plugin's chat prefix string, config reloads apply immediately.
			const std::string *chatPrefix = nullptr;
			// Append CHAT_COLOR_DEFAULT after the prefix.
			bool resetColorAfterPrefix = false;
			// Bracketed tag for console output, e.g. "ADMIN" -> "[ADMIN] ".
			const char *conTag = "";
			// Connected non-bot check, e.g. via the plugin's player manager.
			bool (*slotIsHuman)(int slot) = nullptr;
		};

		void Configure(const Setup &setup);

		// Prefixed chat to one player. slot < 0 prints to the server console instead.
		void ChatToSlotV(int slot, const char *fmt, va_list args);
		// Translated variant.
		void ChatToSlotTV(int slot, const char *phrase, va_list args);

		// Prefixed (optional) chat to all connected humans, mirrored to the server console.
		void ChatToAllV(const char *fmt, va_list args, bool addPrefix = true);
		// Translated per recipient, each player sees their own language.
		// Console mirror renders once in the default language.
		void ChatToAllTV(const char *phrase, va_list args);

		// Chat to connected humans passing `pred`, no console mirror.
		// Prefix not added, callers embed it.
		void ChatToPredV(bool (*pred)(int slot), const char *fmt, va_list args);
		// Translated per recipient.
		void ChatToPredTV(bool (*pred)(int slot), const char *phrase, va_list args);

		// Command reply: player console (tagged) plus prefixed chat.
		// slot < 0 prints to the server console only.
		void ReplyV(int slot, const char *fmt, va_list args);
		void ReplyTV(int slot, const char *phrase, va_list args);

		// Raw console print to one player. slot < 0 prints to the server console.
		void ClientConsoleV(int slot, const char *fmt, va_list args);
		void ClientConsoleTV(int slot, const char *phrase, va_list args);

		// Console print to every connected human.
		void ConsoleToAllV(const char *fmt, va_list args);

	private:
		std::string Translate(int slot, const char *phrase) const;
		void ComposeChatLine(char *dst, size_t dstlen, const char *text, bool addPrefix) const;
		void ServerConsole(const char *text) const;
		void SendPrefixedToSlot(int slot, const char *text, bool addPrefix);

		Setup m_setup;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_PRINT_H_
