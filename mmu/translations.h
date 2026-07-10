#ifndef _INCLUDE_MMU_TRANSLATIONS_H_
#define _INCLUDE_MMU_TRANSLATIONS_H_

#include <string>
#include <unordered_map>

namespace mmu
{

	// SourceMod-style phrase tables resolved per client language.
	// Phrase values keep any printf-style format specifiers, the caller formats after translating.
	class Translations
	{
	public:
		// Load config.txt (cl_language -> short key map)
		// and every *.phrases.txt under <baseDir>/addons/<addonName>/translations.
		void Load(const char *baseDir, const char *addonName);

		// Whether Load resolves {color} tags in phrase values to raw chat color bytes.
		// Defaults to true. Turn off for text rendered outside chat (e.g. HTML menus).
		// Call before Load.
		void SetResolveColorTags(bool resolve);

		// Language key used when a client's language is unknown or a phrase lacks it.
		void SetDefaultLanguage(const std::string &lang);

		// Map a raw cl_language value ("english") to a phrase-file key ("en").
		// Unmapped values are returned as-is (lowercased), so Translate can fall back.
		std::string MapClientLanguage(const char *clLanguage) const;

		// Translate a phrase for a language. Falls back to the default language,
		// then to the phrase key itself, so unknown phrases pass through unchanged
		// (format specifiers intact, colors absent).
		std::string Translate(const std::string &lang, const std::string &phrase) const;

	private:
		static void PhraseHandler(const std::string &section, const std::string &key, const std::string &value, void *userdata);

		std::unordered_map<std::string, std::unordered_map<std::string, std::string>> m_phrases; // phrase -> lang -> text
		std::unordered_map<std::string, std::string> m_languageMap;                              // cl_language -> short key
		std::string m_defaultLang = "en";
		bool m_resolveColors = true;
	};

} // namespace mmu

#endif // _INCLUDE_MMU_TRANSLATIONS_H_
