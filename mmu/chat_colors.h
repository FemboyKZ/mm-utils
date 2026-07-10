#ifndef _INCLUDE_MMU_CHAT_COLORS_H_
#define _INCLUDE_MMU_CHAT_COLORS_H_

#include <string>

// CS2 chat color codes
#define CHAT_COLOR_DEFAULT  "\x01"
#define CHAT_COLOR_DARKRED  "\x02"
#define CHAT_COLOR_PURPLE   "\x03"
#define CHAT_COLOR_GREEN    "\x04"
#define CHAT_COLOR_OLIVE    "\x05"
#define CHAT_COLOR_LIME     "\x06"
#define CHAT_COLOR_RED      "\x07"
#define CHAT_COLOR_GREY     "\x08"
#define CHAT_COLOR_YELLOW   "\x09"
#define CHAT_COLOR_BLUEGREY "\x0A"
#define CHAT_COLOR_BLUE     "\x0B"
#define CHAT_COLOR_DARKBLUE "\x0C"
#define CHAT_COLOR_GREY2    "\x0D"
#define CHAT_COLOR_ORCHID   "\x0E"
#define CHAT_COLOR_LIGHTRED "\x0F"
#define CHAT_COLOR_GOLD     "\x10"

namespace mmu
{
	// Replace {color} tags ({default}, {red}, {gold}, ...) with raw chat color bytes.
	std::string ResolveColorTags(const std::string &input);
} // namespace mmu

#endif // _INCLUDE_MMU_CHAT_COLORS_H_
