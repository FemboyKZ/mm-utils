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

		inline bool Hex4(const std::string &doc, size_t at, unsigned int &value)
		{
			if (at + 4 > doc.size())
			{
				return false;
			}
			value = 0;
			for (size_t i = at; i < at + 4; i++)
			{
				const char c = doc[i];
				const int digit = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
				if (digit < 0)
				{
					return false;
				}
				value = value * 16 + digit;
			}
			return true;
		}

		inline void AppendUtf8(std::string &out, unsigned int cp)
		{
			if (cp < 0x80)
			{
				out += static_cast<char>(cp);
			}
			else if (cp < 0x800)
			{
				out += static_cast<char>(0xC0 | (cp >> 6));
				out += static_cast<char>(0x80 | (cp & 0x3F));
			}
			else if (cp < 0x10000)
			{
				out += static_cast<char>(0xE0 | (cp >> 12));
				out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (cp & 0x3F));
			}
			else
			{
				out += static_cast<char>(0xF0 | (cp >> 18));
				out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
				out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
				out += static_cast<char>(0x80 | (cp & 0x3F));
			}
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
						case 'u':
						{
							unsigned int cp = 0;
							if (!Hex4(doc, pos + 1, cp))
							{
								result += 'u';
								break;
							}
							pos += 4;
							// Past U+FFFF a character is two escapes, a high and a low surrogate.
							unsigned int low = 0;
							if (cp >= 0xD800 && cp <= 0xDBFF && doc.compare(pos + 1, 2, "\\u") == 0 && Hex4(doc, pos + 3, low) && low >= 0xDC00
								&& low <= 0xDFFF)
							{
								cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
								pos += 6;
							}
							AppendUtf8(result, cp);
							break;
						}
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
