#include "utils/translations.h"
#include "utils/chat_colors.h"
#include "utils/kv_parser.h"
#include "utils/log.h"
#include "utils/str.h"

#include <filesystem>
#include <string_view>

namespace mmu
{

	using str::ToLower;

	// cl_language -> short key. Called with key = cl_language value, value = short key.
	static void LanguageHandler(const std::string & /*section*/, const std::string &key, const std::string &value, void *userdata)
	{
		auto *map = static_cast<std::unordered_map<std::string, std::string> *>(userdata);
		(*map)[ToLower(key)] = value;
	}

	// The arguments a printf-style text takes, as their length and conversion letters: "sd" for "%s has %d".
	// A '*' width or precision takes one too, and '?' stands for a conversion printf doesn't have.
	static std::string FormatArgs(const std::string &text)
	{
		static constexpr std::string_view kFlags = "-+ #0123456789.*";
		static constexpr std::string_view kLengths = "hlLjzt";
		static constexpr std::string_view kConversions = "diouxXeEfFgGaAcsp";
		std::string args;
		for (size_t i = 0; i < text.size(); i++)
		{
			if (text[i] != '%')
			{
				continue;
			}
			size_t at = i + 1;
			if (at < text.size() && text[at] == '%')
			{
				i = at;
				continue;
			}
			for (; at < text.size() && kFlags.find(text[at]) != std::string_view::npos; at++)
			{
				if (text[at] == '*')
				{
					args += '*';
				}
			}
			for (; at < text.size() && kLengths.find(text[at]) != std::string_view::npos; at++)
			{
				args += text[at];
			}
			const bool known = at < text.size() && kConversions.find(text[at]) != std::string_view::npos;
			args += known ? text[at] : '?';
			i = known ? at : at - 1;
		}
		return args;
	}

	// phrase -> lang -> text. Called with section = phrase name, key = language.
	void Translations::PhraseHandler(const std::string &section, const std::string &key, const std::string &value, void *userdata)
	{
		// The caller formats the text with the arguments it has for the phrase name:
		// a translation that takes others would read what isn't there. Left out, the phrase falls back like one that isn't translated.
		if (FormatArgs(value) != FormatArgs(section))
		{
			MMU_LOG_WARN("Translation of \"%s\" in \"%s\" takes other format arguments than the phrase, left out.\n", section.c_str(), key.c_str());
			return;
		}
		auto *self = static_cast<Translations *>(userdata);
		self->m_phrases[section][ToLower(key)] = self->m_resolveColors ? ResolveColorTags(value) : value;
	}

	void Translations::Load(const char *baseDir, const char *addonName)
	{
		m_phrases.clear();
		m_languageMap.clear();

		namespace fs = std::filesystem;
		std::string dir = std::string(baseDir ? baseDir : "") + "/addons/" + (addonName ? addonName : "") + "/translations";

		kv::LoadFile(dir + "/config.txt", LanguageHandler, &m_languageMap);

		std::error_code ec;
		fs::directory_iterator it(dir, ec);
		if (ec)
		{
			return;
		}
		for (const auto &entry : it)
		{
			if (!entry.is_regular_file(ec))
			{
				continue;
			}
			std::string name = entry.path().filename().string();
			static const std::string suffix = ".phrases.txt";
			if (name.size() <= suffix.size() || name.compare(name.size() - suffix.size(), suffix.size(), suffix) != 0)
			{
				continue;
			}
			kv::LoadFile(entry.path().string(), PhraseHandler, this, true);
		}
	}

	void Translations::SetResolveColorTags(bool resolve)
	{
		m_resolveColors = resolve;
	}

	void Translations::SetDefaultLanguage(const std::string &lang)
	{
		if (!lang.empty())
		{
			m_defaultLang = ToLower(lang);
		}
	}

	std::string Translations::MapClientLanguage(const char *clLanguage) const
	{
		if (!clLanguage || !clLanguage[0])
		{
			return m_defaultLang;
		}
		std::string raw = ToLower(clLanguage);
		auto it = m_languageMap.find(raw);
		return it != m_languageMap.end() ? it->second : raw;
	}

	std::string Translations::Translate(const std::string &lang, const std::string &phrase) const
	{
		auto pit = m_phrases.find(phrase);
		if (pit == m_phrases.end())
		{
			return phrase; // not a known phrase: pass through as literal text
		}
		const auto &langs = pit->second;

		std::string want = lang.empty() ? m_defaultLang : ToLower(lang);
		auto lit = langs.find(want);
		if (lit != langs.end())
		{
			return lit->second;
		}
		if (want != m_defaultLang)
		{
			auto dit = langs.find(m_defaultLang);
			if (dit != langs.end())
			{
				return dit->second;
			}
		}
		return phrase; // known phrase but no matching/default language
	}

} // namespace mmu
