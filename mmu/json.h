#ifndef _INCLUDE_MMU_JSON_H_
#define _INCLUDE_MMU_JSON_H_

#include <cstdio>
#include <string>

namespace mmu
{
	namespace json
	{
		// Escape for a JSON string value, without the quotes. UTF-8 passes through.
		inline std::string Escape(const std::string &input)
		{
			std::string out;
			out.reserve(input.size() + 16);
			for (char c : input)
			{
				switch (c)
				{
					case '\"':
						out += "\\\"";
						break;
					case '\\':
						out += "\\\\";
						break;
					case '\b':
						out += "\\b";
						break;
					case '\f':
						out += "\\f";
						break;
					case '\n':
						out += "\\n";
						break;
					case '\r':
						out += "\\r";
						break;
					case '\t':
						out += "\\t";
						break;
					default:
						if (static_cast<unsigned char>(c) < 0x20)
						{
							char buf[8];
							snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
							out += buf;
						}
						else
						{
							out += c;
						}
						break;
				}
			}
			return out;
		}

		inline std::string Escape(const char *input)
		{
			return Escape(std::string(input ? input : ""));
		}

		// String value of the first `"key"` anywhere in `doc`, or "" if absent or not a string.
		// A scan, not a parser. Ignores nesting, so only use it on known response shapes.
		inline std::string GetString(const std::string &doc, const char *key)
		{
			std::string search = "\"";
			search += key;
			search += "\"";

			size_t pos = doc.find(search);
			if (pos == std::string::npos)
			{
				return "";
			}

			pos += search.size();
			while (pos < doc.size() && (doc[pos] == ' ' || doc[pos] == ':' || doc[pos] == '\t'))
			{
				pos++;
			}
			if (pos >= doc.size() || doc[pos] != '\"')
			{
				return "";
			}

			pos++; // opening quote
			std::string result;
			while (pos < doc.size() && doc[pos] != '\"')
			{
				if (doc[pos] == '\\' && pos + 1 < doc.size())
				{
					pos++;
					switch (doc[pos])
					{
						case '\"':
							result += '\"';
							break;
						case '\\':
							result += '\\';
							break;
						case 'n':
							result += '\n';
							break;
						case 'r':
							result += '\r';
							break;
						case 't':
							result += '\t';
							break;
						default:
							result += doc[pos];
							break;
					}
				}
				else
				{
					result += doc[pos];
				}
				pos++;
			}
			return result;
		}
	} // namespace json
} // namespace mmu

#endif // _INCLUDE_MMU_JSON_H_
