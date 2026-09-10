#ifndef _INCLUDE_CS2MENUS_MENU_STYLE_H_
#define _INCLUDE_CS2MENUS_MENU_STYLE_H_

#include "interfaces/cs2menus/ics2menus.h"

#include "mmu/str_utils.h"

#include <string>

// Menu style options for a consumer plugin's config. "default" defers to mm-cs2menus' own setting.
struct MenuStyleBlock
{
	// "default", "chat" or "html".
	std::string type = "default";
	// HTML nav keys. "default", "none", or any ParseMenuButton name.
	std::string navUp = "default";
	std::string navDown = "default";
	std::string navSelect = "default";
	std::string navBack = "default";

	MenuType Type() const
	{
		if (type == "chat")
		{
			return MenuType::Chat;
		}
		if (type == "html")
		{
			return MenuType::Html;
		}
		return MenuType::Default;
	}

	void ApplyKeys(ICS2Menus *menus, MenuHandle menu) const
	{
		if (!menus)
		{
			return;
		}
		menus->SetMenuKey(menu, MenuNavAction::Up, ParseMenuButton(navUp));
		menus->SetMenuKey(menu, MenuNavAction::Down, ParseMenuButton(navDown));
		menus->SetMenuKey(menu, MenuNavAction::Select, ParseMenuButton(navSelect));
		menus->SetMenuKey(menu, MenuNavAction::Back, ParseMenuButton(navBack));
	}

	// `key` must be lowercase. Returns false for keys it does not own.
	// Takes both "type"/"nav*" (cs2admin) and "menutype"/"menunav*" (cs2rockthevote).
	bool ApplyKey(const std::string &key, const std::string &value)
	{
		if (key == "type" || key == "menutype")
		{
			type = str::ToLower(value);
		}
		else if (key == "navup" || key == "menunavup")
		{
			navUp = str::ToLower(value);
		}
		else if (key == "navdown" || key == "menunavdown")
		{
			navDown = str::ToLower(value);
		}
		else if (key == "navselect" || key == "menunavselect")
		{
			navSelect = str::ToLower(value);
		}
		else if (key == "navback" || key == "menunavback")
		{
			navBack = str::ToLower(value);
		}
		else
		{
			return false;
		}
		return true;
	}
};

#endif // _INCLUDE_CS2MENUS_MENU_STYLE_H_
