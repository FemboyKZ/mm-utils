#ifndef _INCLUDE_MMU_COMMAND_ARGS_H_
#define _INCLUDE_MMU_COMMAND_ARGS_H_

#include "mmu/str_utils.h"

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace mmu
{
	struct ArgKey
	{
		const char *name;
		const char *alias = nullptr;
	};

	// The keys a command takes.
	// Text before the first key= fills `main`, so with main "player" `!kick jvn reason=afk` is `!kick player=jvn reason=afk`.
	struct ArgSpec
	{
		std::vector<ArgKey> keys;
		const char *main = nullptr;
	};

	// Values by ArgKey::name. A key given empty, like `reason=`, counts as left out.
	class Args
	{
	public:
		const std::string *Get(const char *name) const
		{
			auto it = m_values.find(name);
			return it != m_values.end() ? &it->second : nullptr;
		}

		std::string GetOr(const char *name, const std::string &fallback) const
		{
			const std::string *value = Get(name);
			return value ? *value : fallback;
		}

		bool Has(const char *name) const
		{
			return m_values.count(name) != 0;
		}

		bool Empty() const
		{
			return m_values.empty();
		}

		// For building a command's args in code, like a menu flow running the command it ends in.
		Args &Set(const char *name, std::string value)
		{
			if (value.empty())
			{
				m_values.erase(name);
			}
			else
			{
				m_values[name] = std::move(value);
			}
			return *this;
		}

	private:
		std::map<std::string, std::string> m_values;
	};

	enum class ArgError
	{
		None,
		UnknownKey,
		StrayText, // text before the first pair of a command with no main key
		OpenQuote,
	};

	// cs2kz-metamod's style: key=value pairs in any order, an unquoted value running until the next pair.
	// key="value" and "key=value" take quotes, so a value can hold a key= of its own.
	// Keys match case-insensitively. `what` gets the unknown key or the stray text.
	// Works on console command lines too, from CCommand::ArgS(). The engine tokenizer would cut a SteamID apart at its colons.
	inline ArgError ParseArgs(const std::string &line, const ArgSpec &spec, Args &out, std::string *what = nullptr)
	{
		// A key and '=' at `at`, maybe inside the quote of a "key=value" pair.
		auto pairAt = [&line](size_t at)
		{
			at += at < line.size() && line[at] == '"';
			const size_t key = at;
			while (at < line.size() && (std::isalnum(static_cast<unsigned char>(line[at])) || line[at] == '_'))
			{
				at++;
			}
			return at > key && at < line.size() && line[at] == '=';
		};
		auto store = [&](const std::string &key, const std::string &value)
		{
			const std::string name = str::ToLower(key);
			for (const ArgKey &k : spec.keys)
			{
				if (name == k.name || (k.alias && name == k.alias))
				{
					out.Set(k.name, str::Trim(value));
					return true;
				}
			}
			if (what)
			{
				*what = key;
			}
			return false;
		};
		// A value from `at`: quoted, or running until the next pair.
		auto readValue = [&](size_t &at, std::string &value)
		{
			if (at < line.size() && line[at] == '"')
			{
				const size_t close = line.find('"', at + 1);
				if (close == std::string::npos)
				{
					return false;
				}
				value = line.substr(at + 1, close - at - 1);
				at = close + 1;
				return true;
			}
			size_t end = at;
			for (; end < line.size(); end++)
			{
				const size_t next = std::isspace(static_cast<unsigned char>(line[end])) ? line.find_first_not_of(" \t", end) : std::string::npos;
				if (next != std::string::npos && pairAt(next))
				{
					break;
				}
			}
			value = line.substr(at, end - at);
			at = end;
			return true;
		};

		size_t at = line.find_first_not_of(" \t");
		if (at != std::string::npos && !pairAt(at))
		{
			std::string value;
			if (!readValue(at, value))
			{
				return ArgError::OpenQuote;
			}
			if (!spec.main)
			{
				if (what)
				{
					*what = str::Trim(value);
				}
				return ArgError::StrayText;
			}
			out.Set(spec.main, str::Trim(value));
		}

		while (at != std::string::npos && (at = line.find_first_not_of(" \t", at)) != std::string::npos)
		{
			if (!pairAt(at))
			{
				// Only after a quoted value, like `reason="afk" more`.
				if (what)
				{
					*what = str::Trim(line.substr(at));
				}
				return ArgError::StrayText;
			}
			if (line[at] == '"')
			{
				const size_t close = line.find('"', at + 1);
				if (close == std::string::npos)
				{
					return ArgError::OpenQuote;
				}
				const std::string pair = line.substr(at + 1, close - at - 1);
				const size_t equals = pair.find('=');
				if (!store(pair.substr(0, equals), pair.substr(equals + 1)))
				{
					return ArgError::UnknownKey;
				}
				at = close + 1;
				continue;
			}
			const size_t equals = line.find('=', at);
			const std::string key = line.substr(at, equals - at);
			at = equals + 1;
			std::string value;
			if (!readValue(at, value))
			{
				return ArgError::OpenQuote;
			}
			if (!store(key, value))
			{
				return ArgError::UnknownKey;
			}
		}
		return ArgError::None;
	}

	// A whole number, nothing else around it.
	inline bool ParseInt(const std::string &text, int &value)
	{
		if (text.empty() || std::isspace(static_cast<unsigned char>(text[0])))
		{
			return false;
		}
		char *end = nullptr;
		errno = 0;
		const long parsed = std::strtol(text.c_str(), &end, 10);
		if (*end != '\0' || errno == ERANGE || parsed < INT_MIN || parsed > INT_MAX)
		{
			return false;
		}
		value = static_cast<int>(parsed);
		return true;
	}

	// A decimal number, a comma taken as the point.
	inline bool ParseNumber(std::string text, double &value)
	{
		if (text.empty() || std::isspace(static_cast<unsigned char>(text[0])))
		{
			return false;
		}
		std::replace(text.begin(), text.end(), ',', '.');
		char *end = nullptr;
		value = std::strtod(text.c_str(), &end);
		return *end == '\0';
	}
} // namespace mmu

#endif // _INCLUDE_MMU_COMMAND_ARGS_H_
