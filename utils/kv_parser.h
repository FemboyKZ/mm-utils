#ifndef _INCLUDE_MMU_KV_PARSER_H_
#define _INCLUDE_MMU_KV_PARSER_H_

// Minimal Valve KeyValues1 tokenizer and section parser.

#include "utils/log.h"

#include <fstream>
#include <istream>
#include <string>
#include <vector>

namespace kv
{

	// Kept in the stream itself, so a warning can name a line without the callers passing a counter.
	inline long &LinesRead(std::istream &in)
	{
		static const int index = std::ios_base::xalloc();
		return in.iword(index);
	}

	inline int LineNumber(std::istream &in)
	{
		return static_cast<int>(LinesRead(in)) + 1;
	}

	// Set on a stream to have \n, \t, \" and \\ in a quoted string read as escapes, as a phrase file needs.
	// Off, the string is taken as typed up to the next quote: a config value can be a path or a password.
	inline long &Escapes(std::istream &in)
	{
		static const int index = std::ios_base::xalloc();
		return in.iword(index);
	}

	inline int NextChar(std::istream &in)
	{
		int ch = in.get();
		if (ch == '\n')
		{
			LinesRead(in)++;
		}
		return ch;
	}

	enum class TokenType
	{
		String,
		OpenBrace,
		CloseBrace,
		EndOfFile
	};

	struct Token
	{
		TokenType kind;
		std::string value;
	};

	inline Token NextToken(std::istream &in)
	{
		Token tok;
		while (in.good())
		{
			int ch = NextChar(in);
			if (ch == EOF)
			{
				tok.kind = TokenType::EndOfFile;
				return tok;
			}

			if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
			{
				continue;
			}

			if (ch == '/')
			{
				int next = in.peek();
				if (next == '/')
				{
					while (in.good() && NextChar(in) != '\n')
						;
					continue;
				}
				// Block comments, SourceMod's configs use them.
				if (next == '*')
				{
					in.get();
					int prev = 0;
					while (in.good())
					{
						int c = NextChar(in);
						if (c == EOF || (prev == '*' && c == '/'))
						{
							break;
						}
						prev = c;
					}
					continue;
				}
			}

			if (ch == '{')
			{
				tok.kind = TokenType::OpenBrace;
				return tok;
			}
			if (ch == '}')
			{
				tok.kind = TokenType::CloseBrace;
				return tok;
			}

			if (ch == '"')
			{
				tok.kind = TokenType::String;
				tok.value.clear();
				const int opened = LineNumber(in);
				bool closed = false;
				while (in.good())
				{
					ch = NextChar(in);
					if (ch == '"')
					{
						closed = true;
						break;
					}
					if (ch == EOF)
					{
						break;
					}
					if (ch == '\\' && Escapes(in))
					{
						int esc = NextChar(in);
						if (esc == '"')
						{
							tok.value += '"';
						}
						else if (esc == '\\')
						{
							tok.value += '\\';
						}
						else if (esc == 'n')
						{
							tok.value += '\n';
						}
						else if (esc == 't')
						{
							tok.value += '\t';
						}
						else if (esc != EOF)
						{
							tok.value += '\\';
							tok.value += static_cast<char>(esc);
						}
					}
					else
					{
						tok.value += static_cast<char>(ch);
					}
				}
				if (!closed)
				{
					MMU_LOG_WARN("Config: the quote opened on line %d is never closed, the rest of the file was read as its text.\n", opened);
				}
				return tok;
			}

			tok.kind = TokenType::String;
			tok.value.clear();
			tok.value += static_cast<char>(ch);
			while (in.good())
			{
				int next = in.peek();
				if (next == ' ' || next == '\t' || next == '\r' || next == '\n' || next == '{' || next == '}' || next == '"' || next == EOF)
				{
					break;
				}
				tok.value += static_cast<char>(in.get());
			}
			return tok;
		}

		tok.kind = TokenType::EndOfFile;
		return tok;
	}

	typedef void (*Handler)(const std::string &section, const std::string &key, const std::string &value, void *userdata);

	// `nested` is for its own recursion. False if the file ended before the closing brace.
	inline bool ParseSection(std::istream &in, const std::string &sectionName, Handler handler, void *userdata, bool nested = false)
	{
		Token tok = NextToken(in);
		while (true)
		{
			if (tok.kind == TokenType::EndOfFile)
			{
				MMU_LOG_WARN("Config: the file ends inside \"%s\", a } is missing.\n", sectionName.c_str());
				return false;
			}
			if (tok.kind == TokenType::CloseBrace)
			{
				// Nothing past the root's brace is ever read, so a stray } drops the rest of the file.
				const int closed = LineNumber(in);
				if (!nested && NextToken(in).kind != TokenType::EndOfFile)
				{
					MMU_LOG_WARN("Config: \"%s\" is closed on line %d, what comes after it is not read.\n", sectionName.c_str(), closed);
				}
				return true;
			}

			if (tok.kind != TokenType::String)
			{
				tok = NextToken(in);
				continue;
			}

			std::string key = tok.value;
			const int line = LineNumber(in);
			Token next = NextToken(in);

			if (next.kind == TokenType::OpenBrace)
			{
				if (!ParseSection(in, key, handler, userdata, true))
				{
					return false;
				}
			}
			else if (next.kind == TokenType::String)
			{
				handler(sectionName, key, next.value, userdata);
			}
			else
			{
				// `next` is this section's brace or the end of the file, handled as that.
				MMU_LOG_WARN("Config: \"%s\" on line %d has no value.\n", key.c_str(), line);
				tok = next;
				continue;
			}
			tok = NextToken(in);
		}
	}

	// Open `path` and parse its top-level "Root { ... }" body with `handler`.
	// Returns false if the file is missing or isn't a braced root section.
	inline bool LoadFile(const std::string &path, Handler handler, void *userdata, bool escapes = false)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			return false;
		}
		Escapes(file) = escapes;
		Token root = NextToken(file);
		if (root.kind != TokenType::String)
		{
			return false;
		}
		Token brace = NextToken(file);
		if (brace.kind != TokenType::OpenBrace)
		{
			return false;
		}
		ParseSection(file, root.value, handler, userdata);
		return true;
	}

	// A whole file as a tree, for nesting and repeated keys.
	struct Node
	{
		std::string key;
		std::string value;          // empty for a section
		std::vector<Node> children; // in file order, repeats kept
		bool section = false;

		// The first child with this key, or nullptr.
		const Node *Find(const std::string &name) const
		{
			for (const Node &child : children)
			{
				if (child.key == name)
				{
					return &child;
				}
			}
			return nullptr;
		}

		// The first child value with this key, or fallback.
		std::string Get(const std::string &name, const std::string &fallback = std::string()) const
		{
			const Node *child = Find(name);
			return child && !child->section ? child->value : fallback;
		}
	};

	// Children up to the closing brace or the end of the file.
	inline void ParseChildren(std::istream &in, std::vector<Node> &out)
	{
		while (true)
		{
			Token tok = NextToken(in);
			if (tok.kind == TokenType::CloseBrace || tok.kind == TokenType::EndOfFile)
			{
				return;
			}
			if (tok.kind != TokenType::String)
			{
				continue;
			}
			Node node;
			node.key = tok.value;
			Token next = NextToken(in);
			if (next.kind == TokenType::OpenBrace)
			{
				node.section = true;
				ParseChildren(in, node.children);
			}
			else if (next.kind == TokenType::String)
			{
				node.value = next.value;
			}
			else
			{
				out.push_back(std::move(node));
				return;
			}
			out.push_back(std::move(node));
		}
	}

	// Parse `path`'s "Root { ... }" into root. False if it's missing or malformed.
	inline bool LoadTree(const std::string &path, Node &root)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			return false;
		}
		Token name = NextToken(file);
		if (name.kind != TokenType::String || NextToken(file).kind != TokenType::OpenBrace)
		{
			return false;
		}
		root = Node();
		root.key = name.value;
		root.section = true;
		ParseChildren(file, root.children);
		return true;
	}

} // namespace kv

#endif // _INCLUDE_MMU_KV_PARSER_H_
