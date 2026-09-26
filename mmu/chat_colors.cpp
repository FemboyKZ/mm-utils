#include "mmu/chat_colors.h"

namespace mmu
{
	namespace
	{
		struct ChatColor
		{
			const char *name;
			const char *code;
		};

		const ChatColor kChatColors[] = {
			{"default", CHAT_COLOR_DEFAULT}, {"darkred", CHAT_COLOR_DARKRED},   {"purple", CHAT_COLOR_PURPLE},     {"green", CHAT_COLOR_GREEN},
			{"olive", CHAT_COLOR_OLIVE},     {"lime", CHAT_COLOR_LIME},         {"red", CHAT_COLOR_RED},           {"grey", CHAT_COLOR_GREY},
			{"yellow", CHAT_COLOR_YELLOW},   {"bluegrey", CHAT_COLOR_BLUEGREY}, {"blue", CHAT_COLOR_BLUE},         {"darkblue", CHAT_COLOR_DARKBLUE},
			{"grey2", CHAT_COLOR_GREY2},     {"orchid", CHAT_COLOR_ORCHID},     {"lightred", CHAT_COLOR_LIGHTRED}, {"gold", CHAT_COLOR_GOLD},
		};
	} // namespace

	const char *ChatColorByName(const std::string &name)
	{
		for (const ChatColor &c : kChatColors)
		{
			if (name == c.name)
			{
				return c.code;
			}
		}
		return nullptr;
	}

	std::string ResolveColorTags(const std::string &input)
	{
		std::string result = input;
		for (const ChatColor &c : kChatColors)
		{
			std::string tag = std::string("{") + c.name + "}";
			size_t pos = 0;
			while ((pos = result.find(tag, pos)) != std::string::npos)
			{
				result.replace(pos, tag.size(), c.code);
			}
		}
		return result;
	}

} // namespace mmu
