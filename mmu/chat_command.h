#ifndef _INCLUDE_MMU_CHAT_COMMAND_H_
#define _INCLUDE_MMU_CHAT_COMMAND_H_

#include <string>
#include <vector>

namespace mmu
{
	struct ChatCommand
	{
		// Command name after the prefix, lowercased.
		std::string name;
		// Quote-aware tokens after the name.
		std::vector<std::string> args;
		// Raw text after the name with leading spaces trimmed, quotes kept.
		std::string argLine;
		// Triggered via a silent prefix, the say message should be suppressed.
		bool silent = false;
	};

	// Strip the outer quotes CS2 wraps around say messages.
	inline std::string StripSayQuotes(const char *rawMessage)
	{
		std::string msg(rawMessage ? rawMessage : "");
		if (msg.size() >= 2 && msg.front() == '"' && msg.back() == '"')
		{
			msg = msg.substr(1, msg.size() - 2);
		}
		return msg;
	}

	// Parse a chat message into a command. Each prefix string is a set of trigger characters. 
	// A character in both sets is treated as silent.
	// Returns false if the message doesn't start with a prefix or has no command name.
	inline bool ParseChatCommand(const std::string &message, const std::string &normalPrefixes, const std::string &silentPrefixes, ChatCommand &out)
	{
		if (message.size() < 2)
		{
			return false;
		}

		char prefix = message[0];
		if (!silentPrefixes.empty() && silentPrefixes.find(prefix) != std::string::npos)
		{
			out.silent = true;
		}
		else if (!normalPrefixes.empty() && normalPrefixes.find(prefix) != std::string::npos)
		{
			out.silent = false;
		}
		else
		{
			return false;
		}

		std::string rest = message.substr(1);

		// Name runs to the first space.
		size_t space = rest.find(' ');
		out.name = (space == std::string::npos) ? rest : rest.substr(0, space);
		if (out.name.empty())
		{
			return false;
		}
		for (char &c : out.name)
		{
			c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}

		out.argLine.clear();
		if (space != std::string::npos)
		{
			size_t start = rest.find_first_not_of(' ', space);
			if (start != std::string::npos)
			{
				out.argLine = rest.substr(start);
			}
		}

		// Quote-aware tokenization of the arg line.
		out.args.clear();
		std::string current;
		bool inQuotes = false;
		for (char c : out.argLine)
		{
			if (c == '"')
			{
				inQuotes = !inQuotes;
			}
			else if (c == ' ' && !inQuotes)
			{
				if (!current.empty())
				{
					out.args.push_back(current);
					current.clear();
				}
			}
			else
			{
				current += c;
			}
		}
		if (!current.empty())
		{
			out.args.push_back(current);
		}

		return true;
	}
} // namespace mmu

#endif // _INCLUDE_MMU_CHAT_COMMAND_H_
