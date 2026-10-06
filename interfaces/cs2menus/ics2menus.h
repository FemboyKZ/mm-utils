#ifndef _INCLUDE_ICS2MENUS_H_
#define _INCLUDE_ICS2MENUS_H_

#include <cstdint>
#include <functional>
#include <string>

// Public menu API for CS2Menus.
//
// Other Metamod plugins acquire this interface via:
//   ICS2Menus *menus = (ICS2Menus *)g_SMAPI->MetaFactory(CS2MENUS_INTERFACE, nullptr, nullptr);
//
// cs2menus owns player chat input: while a player has an open menu,
// it intercepts their "say"/"say_team" numeric input, drives the menu, and suppresses the chat line.
// Consumers only build menus and react to selections.
//
// Threading: every method is safe from the main (game) thread or a worker thread.
//  - Build/query calls (CreateMenu, AddItem, SetX, GetX...) run inline under a lock.
//  - DisplayMenu/CancelMenu off-thread are queued for the next GameFrame,
//    DisplayMenu then returns true optimistically.
//  - onSelect/onEnd/onChange callbacks always fire on the main thread.
//  - DestroyMenu off-thread invalidates the handle at once but skips the Destroyed callback.
//  - const char * getters alias internal storage, copy them, don't cache.
//  - Don't block a main-thread callback on a worker that re-enters this API (lock is held -> deadlock).
#define CS2MENUS_INTERFACE "ICS2Menus005"

// Opaque menu identifier returned by CreateMenu. 0 is the invalid sentinel.
// A handle stays valid until DestroyMenu (or until cs2menus unloads).
using MenuHandle = uint32_t;
static constexpr MenuHandle kInvalidMenuHandle = 0;

// Render style for a menu.
enum class MenuType : int
{
	Default = -1, // use the server's configured default type (see core.cfg)
	Chat = 0,     // numbered list printed to chat, navigated by typing 1-9/0
	Html = 1,     // center-screen HTML panel, navigated with movement keys
	Panorama = 2, // Panorama menu window, clicked with the mouse. Needs the cs2menus workshop addon.
};

// Why a player's menu closed. Delivered to the MenuEnd callback.
enum class MenuEndReason : int
{
	Selected = 0,   // player picked a (non-disabled) item, OnSelect already fired
	Exit = 1,       // player pressed the Exit button (slot 0)
	Timeout = 2,    // display duration elapsed
	Disconnect = 3, // player left the server
	Cancelled = 4,  // CancelMenu, or replaced by a newer DisplayMenu
	Destroyed = 5,  // the menu handle was destroyed while displayed
};

// Buttons usable as HTML-menu navigation keys (for per-menu key overrides).
// These map to the player's in-game button binds.
enum class MenuButton : int
{
	Default = 0, // inherit the server config binding for this action
	W,
	A,
	S,
	D,
	Use,     // E
	Speed,   // Shift (walk)
	Duck,    // Ctrl
	Jump,    // Space
	Reload,  // R
	Attack,  // Mouse1
	Attack2, // Mouse2
	Score,   // Tab
	Inspect, // F (look at weapon)
	None,    // disable this action for the menu
};

// Config names for each MenuButton.
struct MenuButtonName
{
	MenuButton button;
	const char *canonical; // written back to configs and the prefs DB
	const char *aliases;   // space-separated alternates
};

inline const MenuButtonName kMenuButtonNames[] = {
	{MenuButton::W, "w", "forward"},
	{MenuButton::S, "s", "back"},
	{MenuButton::A, "a", "left moveleft"},
	{MenuButton::D, "d", "right moveright"},
	{MenuButton::Use, "e", "use interact"},
	{MenuButton::Speed, "shift", "speed walk"},
	{MenuButton::Duck, "ctrl", "duck crouch"},
	{MenuButton::Jump, "space", "jump"},
	{MenuButton::Reload, "r", "reload"},
	{MenuButton::Attack, "mouse1", "attack"},
	{MenuButton::Attack2, "mouse2", "attack2"},
	{MenuButton::Score, "tab", "score"},
	{MenuButton::Inspect, "f", "inspect lookatweapon"},
};
inline constexpr int kMenuButtonNameCount = static_cast<int>(sizeof(kMenuButtonNames) / sizeof(kMenuButtonNames[0]));

// True if `name` is a whitespace-delimited token of `list`.
inline bool MenuButtonNameMatches(const char *list, const std::string &name)
{
	std::string tok;
	for (const char *p = list;; p++)
	{
		if (*p == ' ' || *p == '\0')
		{
			if (!tok.empty() && tok == name)
			{
				return true;
			}
			tok.clear();
			if (*p == '\0')
			{
				return false;
			}
		}
		else
		{
			tok += *p;
		}
	}
}

// Expects a lowercase name. "none"/"off" give None, anything unknown gives Default.
inline MenuButton ParseMenuButton(const std::string &name)
{
	if (name == "none" || name == "off")
	{
		return MenuButton::None;
	}
	for (const MenuButtonName &k : kMenuButtonNames)
	{
		if (name == k.canonical || MenuButtonNameMatches(k.aliases, name))
		{
			return k.button;
		}
	}
	return MenuButton::Default;
}

// nullptr for Default and None.
inline const char *GetMenuButtonName(MenuButton button)
{
	for (const MenuButtonName &k : kMenuButtonNames)
	{
		if (k.button == button)
		{
			return k.canonical;
		}
	}
	return nullptr;
}

// HTML-menu navigation actions whose key can be overridden per menu.
enum class MenuNavAction : int
{
	Up = 0, // move cursor up
	Down,   // move cursor down
	Select, // activate highlighted item
	Back,   // close / exit
};

// Built-in text that SetMenuLabel can rename per menu.
enum class MenuLabel : int
{
	Exit = 0, // exit row / footer hint
	NextPage, // chat next-page row
	PrevPage, // chat previous-page row
	Move,     // HTML footer, shown when both up and down are bound
	Scroll,   // HTML footer, shown when only one of up/down is bound
	Select,   // HTML footer select hint
	On,       // Toggle value
	Off,      // Toggle value
	Adjust,   // HTML footer while editing a value
	Done,     // HTML footer while editing a value
	Count,    // label count, not a valid argument
};

enum class MenuItemType : int
{
	Normal = 0, // fires onSelect, or opens its submenu
	Toggle,     // 0 or 1
	Stepper,    // an integer in [min, max], moved by step
	Choice,     // an index into its options
};

// Per-menu HTML style fields settable via SetMenuStyle.
// Colors are "#RRGGBB" or "#RRGGBBAA".
// Each overrides the matching server default for this one menu.
// Pass "" to clear the override (inherit).
// HTML menus, ignored by chat ones. Panorama takes TitleColor, ItemColor, DisabledColor and PagePrefixDelimiter.
enum class MenuStyle : int
{
	// --- Global / layout ---
	Align = 0,    // line alignment: "left" / "center" / "right"
	FontFace,     // Panorama classes applied to every line, space-separated. "" = game default.
				  // Faces: stratum-{thin,light,regular,medium,bold,black}[-italic/-condensed],
				  // mono: stratum-{light,regular,bold}-mono / mono-spaced-font[-bold].
				  // Effects stack too: text-uppercase, text-letterspace-2px, text-shadow-basic.
				  // e.g. "stratum-bold text-uppercase text-shadow-basic".
	VisibleItems, // integer string (e.g. "10"): rows shown at once in the scroll window

	// --- Title ---
	TitleColor, // hex for the title line
	TitleSize,  // size token: "xs" "s" "sm" "m" "ml" "l" "xl" "xxl" "xxxl"
	RawTitle,   // "1" render the title as raw Panorama markup (unescaped, like SetItemRaw), "0" plain text

	// --- Items ---
	ItemColor,     // hex for normal (unselected, enabled) item text
	ItemSize,      // size token for item + cursor rows
	DisabledColor, // hex for greyed-out items
	SubmenuSuffix, // text appended to items that open a submenu (default " >"). A space = none

	// --- Cursor row ---
	NavColor,      // hex for the cursor row + marker
	Marker,        // literal text drawn before the cursor row (default "▶ ")
	HighlightText, // "1" recolor the cursor row's text to NavColor, "0" only the marker marks it

	// --- Position counter "[n/m]" ---
	ShowCounter,   // "1" show the counter on the title line, "0" hide it
	CounterColor,  // hex for the counter
	CounterSize,   // size token for the counter
	CounterFormat, // template, placeholders {cur} {total} (default "[{cur}/{total}]")

	// --- Key-hint footer ---
	ShowFooter,        // "1" show the footer, "0" hide it
	FooterColor,       // hex for the footer
	FooterSize,        // size token for the footer
	FooterSeparator,   // text between footer hint segments (default " | ")
	FooterHintFormat,  // one footer hint, placeholders {label} {keys} (default "{label}: {keys}")
	FooterRangeFormat, // the two keys in the Move hint, placeholders {up} {down} (default "{up}/{down}")

	// --- Panorama ---
	// A single character. Panorama page labels ("A - D") skip a leading item prefix of up to 5 letters or digits ending in it,
	// e.g. "_" labels "kz_grotto" and "surf_utopia" by G and U. "" (default) uses the first character.
	PagePrefixDelimiter,

	// --- Value items ---
	ValueFormat, // after a value item's text or an item's subtext, placeholder {value} (default ": {value}")
	EditFormat,  // the value being edited, placeholder {value} (default "‹ {value} ›")

	// --- Sections ---
	SectionFormat, // header line above a section, placeholder {section} (default "{section}")
	SectionColor,  // hex for that header
};

// How Panorama draws an item. Chat and HTML list every role as a plain row.
enum class MenuItemRole : int
{
	Button,  // picked, the default
	Readout, // never picked, its subtext labels its text
	Input,   // an input field over the items, its text what was typed, see BeginMenuInput
	Heading, // a label over the items after it, never picked
};

// A tile's corner button, see SetItemCorner.
enum class MenuCorner : int
{
	None,
	Star,     // hollow, shown on hover
	StarOn,   // filled, always shown
	StarUndo, // hollow, always shown: just unstarred
	Copy,     // shown on hover
};

// A message's color, and a chip list row's.
enum class MenuTone : int
{
	Info,
	Ok,
	Warn,
	Bad,
};

// SetMenuTextFeatures bits.
constexpr int kMenuTextConfirm = 1; // ShowMenuConfirm and the unsaved changes question
constexpr int kMenuTextPinned = 2;  // chat: the pinned and secondary item on every page
constexpr int kMenuTextIndex = 4;   // a long sorted list opens on its letter ranges ("A - D")

// SetItemTeams and SetMenuScope bits.
constexpr int kMenuTeamT = 1;
constexpr int kMenuTeamCT = 2;

// Panorama only, each falls back to List when the addon lacks it.
enum class MenuLayout : int
{
	List = 0, // rows
	Grid,     // image tiles, sections as tabs
	Showcase, // SetMenuImage on the left, items as 3-column buttons, sections as tabs
	// Showcase's buttons at the screen's edge, the rest left to a 3D preview the plugin puts there.
	// A click off the box lets the mouse turn the view until the next attack press.
	Studio,
	Columns, // a column per section, each scrolling on its own: up to 4 sections of 16 items, no pages
};

// Grows on its own while every section fits one page.
enum class MenuTileSize : int
{
	Small = 0, // 6 x 4
	Medium,    // 4 x 3
	Large,     // 3 x 2
	Cards,     // 3 x 1, full height
};

// Fired when a player selects an item.
// `item` is the absolute index into the menu (0-based, across pages), not the on-screen 1-9 slot.
// Use GetItemInfo / GetItemText to recover what was chosen.
using MenuItemSelectFn = std::function<void(MenuHandle menu, int slot, int item)>;

// Fired exactly once when a player's display of `menu` ends, for any reason.
// For Selected, this fires after the MenuItemSelectFn,
// and is skipped when that callback re-displayed or pushed the same menu for the same player.
// History navigation isn't an end. When the display ends, every menu in its history fires this.
// A menu also ends when it leaves the history (dropped forward history, ReplaceMenu).
// Use it to free per-menu state (e.g. call DestroyMenu for one-shot menus).
using MenuEndFn = std::function<void(MenuHandle menu, int slot, MenuEndReason reason)>;

// Panorama refresh button pressed. Rebuild, usually with ReplaceMenu.
using MenuRefreshFn = std::function<void(MenuHandle menu, int slot)>;

// A value item changed, `value` already stored. Keeps the menu open. SetItemValue inside overrides it.
using MenuItemChangeFn = std::function<void(MenuHandle menu, int slot, int item, int value)>;

// A tile's corner button was clicked instead of the tile. SetItemCorner inside sets the new state.
using MenuItemCornerFn = std::function<void(MenuHandle menu, int slot, int item)>;

// BeginMenuInput's typing was called off by a click in the window.
using MenuInputCancelFn = std::function<void(MenuHandle menu, int slot)>;

// The input field's clear button was clicked.
using MenuInputClearFn = std::function<void(MenuHandle menu, int slot)>;

// A confirm dialog was answered. False for cancel, a click off it and a dialog that takes its place.
using MenuConfirmFn = std::function<void(int slot, bool confirmed)>;

// One of the menu's own tabs was clicked.
using MenuTabFn = std::function<void(MenuHandle menu, int slot, int tab)>;

// The header's scope chip was clicked.
using MenuScopeFn = std::function<void(MenuHandle menu, int slot)>;

// A chip was clicked. A filter's `selected` is already stored: the option, -1 for none, or 1 and 0 without options.
// An action's is the option picked, 0 without options.
using MenuChipFn = std::function<void(MenuHandle menu, int slot, int chip, int selected)>;

class ICS2Menus
{
public:
	// ============================== Lifetime ==============================

	// Create an empty menu of `type` (pass MenuType::Default to use the server's configured style).
	// `title` may contain chat color codes.
	// `onSelect` is invoked when a player picks an item (may be null if you only care about MenuEnd).
	// Returns kInvalidMenuHandle on failure.
	virtual MenuHandle CreateMenu(MenuType type, const char *title, MenuItemSelectFn onSelect) = 0;

	// Free a menu. Any player currently viewing it has their display closed (fires MenuEnd=Destroyed).
	// The handle is invalid afterwards.
	//
	// IMPORTANT: a menu's callbacks may capture pointers into YOUR plugin.
	// Destroy every menu you created (and CancelMenu open displays) in your plugin's Unload()
	// so cs2menus never calls a lambda inside an unloaded DLL.
	virtual void DestroyMenu(MenuHandle menu) = 0;

	// True if `menu` is a live handle (created and not yet destroyed).
	virtual bool IsValidMenu(MenuHandle menu) = 0;

	// ========================== Menu properties ==========================
	// Each Set re-renders any player currently viewing the menu where it matters.
	// Each Get returns the value last set (create-time default if never set), or a zero value for an invalid handle.

	// Title text (may contain chat color codes). Re-renders an open display.
	// GetTitle aliases internal storage, copy it, don't cache; "" for an invalid handle.
	virtual void SetTitle(MenuHandle menu, const char *title) = 0;
	virtual const char *GetTitle(MenuHandle menu) = 0;

	// The menu's base render type as created (may be MenuType::Default = "server default"). Set at CreateMenu.
	// For the per-viewer resolved type of an open display use GetActiveMenuType. Default for an invalid handle.
	virtual MenuType GetMenuType(MenuHandle menu) = 0;

	// Show/hide the trailing "0. Exit" entry (default: shown).
	virtual void SetExitButton(MenuHandle menu, bool enabled) = 0;
	virtual bool GetExitButton(MenuHandle menu) = 0;

	// Close the menu automatically after a selection (default: true).
	// When false, the menu is re-rendered after each pick so the player can choose again.
	virtual void SetCloseOnSelect(MenuHandle menu, bool enabled) = 0;
	virtual bool GetCloseOnSelect(MenuHandle menu) = 0;

	// HTML menus: show a selectable "Exit" row at the end of the list (default off).
	// Useful when the Back key is set to None, it's also auto-shown in that case so a
	// menu is never left unexitable. Requires SetExitButton(true). No-op for chat menus.
	virtual void SetExitItem(MenuHandle menu, bool enabled) = 0;
	virtual bool GetExitItem(MenuHandle menu) = 0;

	// Lock this menu's render type so the viewing player's saved type preference can't change it.
	// Use it when the menu depends on a specific type, e.g. HTML-only item icons or raw markup. Default: not forced.
	virtual void SetMenuForceType(MenuHandle menu, bool force) = 0;
	virtual bool GetMenuForceType(MenuHandle menu) = 0;

	// Item the menu opens on: HTML cursor row, or the chat page containing it.
	// Clamped to the item range when displayed. Default 0.
	virtual void SetStartItem(MenuHandle menu, int item) = 0;
	virtual int GetStartItem(MenuHandle menu) = 0;

	// Register the per-menu end callback. See MenuEndFn. (No getter: callbacks aren't introspectable.)
	virtual void SetMenuEndCallback(MenuHandle menu, MenuEndFn onEnd) = 0;

	// Override the HTML navigation key for one action.
	// MenuButton::Default clears the override (inherit the server binding).
	// MenuButton::None disables the action for this menu (e.g. disable Up so one key cycles, the cursor wraps).
	// The footer key hints update to match.
	// GetMenuKey returns the per-menu override: Default if unset, None if disabled.
	virtual void SetMenuKey(MenuHandle menu, MenuNavAction action, MenuButton button) = 0;
	virtual MenuButton GetMenuKey(MenuHandle menu, MenuNavAction action) = 0;

	// Rename one built-in label for this menu (Exit, page nav, footer hints). See MenuLabel.
	// Pass "" to restore the server-configured default.
	// GetMenuLabel returns the current key (a phrase key / literal, not translated text);
	// aliases internal storage, copy it, don't cache. "" for an invalid handle/label.
	virtual void SetMenuLabel(MenuHandle menu, MenuLabel label, const char *text) = 0;
	virtual const char *GetMenuLabel(MenuHandle menu, MenuLabel label) = 0;

	// Override one HTML style field for this menu (see MenuStyle). Pass "" to inherit the server default.
	// No-op for chat menus.
	// GetMenuStyle returns the effective value (override if set, else server default): sizes as the token,
	// colors as "#RRGGBB", toggles as "1"/"0", templates as the format string.
	// Aliases internal storage, copy it, don't cache. "" for an invalid handle/field.
	virtual void SetMenuStyle(MenuHandle menu, MenuStyle field, const char *value) = 0;
	virtual const char *GetMenuStyle(MenuHandle menu, MenuStyle field) = 0;

	// ================================ Items ===============================

	// Append an item. Returns the new item's absolute index, or -1 on failure.
	// `info` is an opaque tag echoed back via GetItemInfo, pass "" if unused.
	// A `disabled` item is shown greyed out and cannot be selected.
	virtual int AddItem(MenuHandle menu, const char *text, const char *info, bool disabled) = 0;

	// Insert an item at absolute index `pos` (clamped to [0, count]); later items shift down.
	// Viewers are re-rendered (cursor/page clamped to the new size). Returns the index, or -1.
	virtual int InsertItem(MenuHandle menu, int pos, const char *text, const char *info, bool disabled) = 0;

	// Append an item that opens a submenu when selected, instead of firing onSelect.
	// In the child, the Back key returns to this parent. Returns the new item's index, or -1.
	// `child` must be a live handle distinct from `parent`.
	virtual int AddSubMenu(MenuHandle parent, const char *text, MenuHandle child, const char *info) = 0;

	// Remove one item by absolute index (shifts later indices down). Viewers re-rendered, cursor/page clamped.
	virtual void RemoveItem(MenuHandle menu, int item) = 0;

	// Clear every item. Viewers are re-rendered (cursor/page reset to 0).
	virtual void RemoveAllItems(MenuHandle menu) = 0;

	// Number of items in the menu, or 0 for an invalid handle.
	virtual int GetItemCount(MenuHandle menu) = 0;

	// Item display text. SetItemText re-renders viewers.
	// GetItemText aliases internal storage, copy it, don't cache; "" for an invalid handle/index.
	virtual void SetItemText(MenuHandle menu, int item, const char *text) = 0;
	virtual const char *GetItemText(MenuHandle menu, int item) = 0;

	// Item info tag (opaque, see AddItem). No re-render.
	// GetItemInfo aliases internal storage, copy it, don't cache; "" for an invalid handle/index.
	virtual void SetItemInfo(MenuHandle menu, int item, const char *info) = 0;
	virtual const char *GetItemInfo(MenuHandle menu, int item) = 0;

	// Grey/un-grey an item (re-renders viewers). GetItemDisabled is false for an invalid handle/index.
	virtual void SetItemDisabled(MenuHandle menu, int item, bool disabled) = 0;
	virtual bool GetItemDisabled(MenuHandle menu, int item) = 0;

	// HTML menus: render the item's text as raw Panorama markup (unescaped, no chat-color translation),
	// so it can embed <img>/<font>/etc. The item still gets the row's size/face/color wrapper.
	// You own well-formedness. No-op for chat menus. GetItemRaw is false for an invalid handle/index.
	virtual void SetItemRaw(MenuHandle menu, int item, bool raw) = 0;
	virtual bool GetItemRaw(MenuHandle menu, int item) = 0;

	// HTML menus: show an image just before the item's text (e.g. a rank or role icon).
	// `url` is a Panorama image source ("http(s)://..." URL or a packaged material path). "" removes it.
	// Composes with the normal (escaped) text, so no SetItemRaw needed for "icon + label". No-op for chat menus.
	// GetItemIcon aliases internal storage, copy it; "" if none / invalid handle.
	virtual void SetItemIcon(MenuHandle menu, int item, const char *url) = 0;
	virtual const char *GetItemIcon(MenuHandle menu, int item) = 0;

	// Make an existing item open `child` as a submenu (kInvalidMenuHandle detaches).
	// Sets child's parent so Back in the child returns here. No-op for an invalid handle/index or child == menu.
	// GetItemSubmenu returns the child, or kInvalidMenuHandle if it's a normal item / invalid.
	virtual void SetItemSubmenu(MenuHandle menu, int item, MenuHandle child) = 0;
	virtual MenuHandle GetItemSubmenu(MenuHandle menu, int item) = 0;

	// =============================== Display ==============================

	// Display `menu` to `slot` for `duration` seconds (0 = no timeout).
	// Replaces any menu the player has open (firing its MenuEnd=Cancelled first).
	// Returns false for an invalid handle/slot.
	// Off-thread it returns true once the display is queued, which is not a guarantee it will show
	// (see the threading note above).
	virtual bool DisplayMenu(MenuHandle menu, int slot, float duration) = 0;

	// Display `menu` to every connected player for `duration` seconds (0 = no timeout).
	// Each player's existing menu is replaced (fires its MenuEnd=Cancelled). Safe off-thread.
	virtual void DisplayMenuToAll(MenuHandle menu, float duration) = 0;

	// Close whatever menu `slot` has open (fires MenuEnd=Cancelled). No-op if none.
	virtual void CancelMenu(int slot) = 0;

	// True if `slot` currently has any menu open.
	virtual bool HasMenu(int slot) = 0;

	// The handle of the menu `slot` has open, or kInvalidMenuHandle if none.
	virtual MenuHandle GetActiveMenu(int slot) = 0;

	// The render type of the menu `slot` has open, or MenuType::Chat if none.
	// Lets a consumer tell whether the center-screen channel is in use (Html),
	// e.g. to yield its own HUD only for HTML menus, not chat ones.
	virtual MenuType GetActiveMenuType(int slot) = 0;

	// Abs index of the item a player currently has highlighted in an HTML menu,
	// or -1 if they have no menu / it's a chat menu / the Exit row is highlighted.
	virtual int GetSelectedItem(int slot) = 0;

	// ========================= Host coordination =========================

	// Yield a slot to another menu system (e.g. a managed SwiftlyS2 / CS# menu).
	// While busy, any cs2menus menu on the slot is cancelled and further DisplayMenu calls for it are refused,
	// so cs2menus won't fight for chat input or the center-HTML channel.
	// The caller drives this off the other system's menu open/close. cs2menus never auto-reopens.
	// Pairs with GetActiveMenuType/HasMenu so the other system can yield in turn.
	virtual void SetExternalBusy(int slot, bool busy) = 0;
	virtual bool GetExternalBusy(int slot) = 0;

	// ============================ Value items ===========================
	// Report changes to onChange, never onSelect. A Toggle flips on pick, a Stepper or Choice opens for editing.
	// Each Add returns the index, or -1.

	virtual int AddToggle(MenuHandle menu, const char *text, bool on, const char *info) = 0;
	// Swaps min > max, step at least 1, value clamped.
	virtual int AddStepper(MenuHandle menu, const char *text, int value, int min, int max, int step, const char *info) = 0;
	// Copies the options.
	virtual int AddChoice(MenuHandle menu, const char *text, const char *const *options, int optionCount, int selected, const char *info) = 0;

	virtual MenuItemType GetItemType(MenuHandle menu, int item) = 0;
	// Toggle 0/1, Stepper value, Choice index. Set clamps and skips onChange.
	virtual void SetItemValue(MenuHandle menu, int item, int value) = 0;
	virtual int GetItemValue(MenuHandle menu, int item) = 0;

	virtual void SetMenuChangeCallback(MenuHandle menu, MenuItemChangeFn onChange) = 0;

	// ======================== Sections and subtext =======================

	// Holds the items added after it. Panorama shows tabs or the left column, chat and HTML header lines. Returns the index, or -1.
	virtual int AddSection(MenuHandle menu, const char *name) = 0;
	// -1 for none.
	virtual int GetItemSection(MenuHandle menu, int item) = 0;

	// Shown after the text or under a tile. Value items show their value instead.
	virtual void SetItemSubtext(MenuHandle menu, int item, const char *subtext) = 0;
	virtual const char *GetItemSubtext(MenuHandle menu, int item) = 0;

	// ========================== Panorama layouts =========================
	// Ignored by chat and HTML. An image is a class of the cs2menus addon, named after the game's own image file:
	// an equipment icon like "ak47", a skin, sticker or agent render. The addon's README lists the kinds.

	virtual void SetMenuLayout(MenuHandle menu, MenuLayout layout) = 0;
	virtual MenuLayout GetMenuLayout(MenuHandle menu) = 0;

	// Grid minimum, default Small.
	virtual void SetMenuTileSize(MenuHandle menu, MenuTileSize size) = 0;
	virtual MenuTileSize GetMenuTileSize(MenuHandle menu) = 0;

	// Grid tile image.
	virtual void SetItemImage(MenuHandle menu, int item, const char *image) = 0;
	virtual const char *GetItemImage(MenuHandle menu, int item) = 0;

	// Inside a showcase, beside the box otherwise (hidden while a value popup is open).
	virtual void SetMenuImage(MenuHandle menu, const char *image) = 0;
	virtual const char *GetMenuImage(MenuHandle menu) = 0;

	// Showcase: a wide button under the page's buttons, on every page. -1 for none. Set after the items.
	virtual void SetMenuPinnedItem(MenuHandle menu, int item) = 0;
	virtual int GetMenuPinnedItem(MenuHandle menu) = 0;

	// ============================= History ==============================
	// Browser-like per display. Menus in it keep their page and highlighted row.

	// On top of the current menu, which Back returns to. Works from a CloseOnSelect OnSelect. Clears forward history.
	// Keeps the current display's timeout.
	virtual bool PushMenu(MenuHandle menu, int slot, float duration) = 0;

	// In place of the current menu, which ends (Cancelled).
	virtual bool ReplaceMenu(MenuHandle menu, int slot, float duration) = 0;

	// Back `steps` times. Needs the display open, so not from a CloseOnSelect OnSelect.
	virtual bool StepBack(int slot, int steps) = 0;

	// Panorama refresh button, see MenuRefreshFn.
	virtual void SetMenuRefreshCallback(MenuHandle menu, MenuRefreshFn onRefresh) = 0;

	// ========================= Pausing a display ========================

	// Hides the display without ending it, e.g. while the player types in chat. No input meanwhile, GetActiveMenu still returns it.
	// Another menu on the slot resumes it.
	virtual void SuspendMenu(int slot) = 0;
	virtual void ResumeMenu(int slot) = 0;

	// =====================================================================
	// ICS2Menus005
	// =====================================================================

	// What a menu created as `type` would show as for this player now: their preference, then the fallbacks for what can't render.
	virtual MenuType GetSlotMenuType(int slot, MenuType type) = 0;

	// ========================== Item presentation ========================
	// Panorama only. Showcase and studio draw a Choice of 2 to 5 options in place, a segment per option.

	// An Input item fires onSelect like any other: call BeginMenuInput there, with SetCloseOnSelect off.
	virtual void SetItemRole(MenuHandle menu, int item, MenuItemRole role) = 0;
	virtual MenuItemRole GetItemRole(MenuHandle menu, int item) = 0;

	// The accent, like on the equipped pick.
	virtual void SetItemHighlight(MenuHandle menu, int item, bool highlight) = 0;

	// Showcase and studio: 1 to 3 of the three columns. Columns: 2 for its column's width, 1 for half.
	virtual void SetItemSpan(MenuHandle menu, int item, int columns) = 0;

	// Studio: in the control panel beside the preview, on every page. Its section is its tab there.
	virtual void SetItemControl(MenuHandle menu, int item, bool control) = 0;

	// ============================= Tile badges ===========================
	// Image tiles. A showcase or studio button without an image takes the rarity and the tag only.
	// Tokens become class names: letters, digits and dashes.

	// consumer, industrial, milspec, restricted, classified, covert, contraband, highgrade, remarkable, exotic or
	// extraordinary. "" for none.
	virtual void SetItemRarity(MenuHandle menu, int item, const char *rarity) = 0;
	virtual const char *GetItemRarity(MenuHandle menu, int item) = 0;
	// A short label, like "ST". `style` is "orange", "gold" or "" for the plain look.
	virtual void SetItemTag(MenuHandle menu, int item, const char *tag, const char *style) = 0;
	virtual const char *GetItemTag(MenuHandle menu, int item) = 0;
	// kMenuTeamT and kMenuTeamCT bits.
	virtual void SetItemTeams(MenuHandle menu, int item, int teams) = 0;
	virtual int GetItemTeams(MenuHandle menu, int item) = 0;
	// Picking it still fires onSelect.
	virtual void SetItemLocked(MenuHandle menu, int item, bool locked) = 0;
	virtual bool GetItemLocked(MenuHandle menu, int item) = 0;
	// A button of its own: a click fires the corner callback, not onSelect.
	virtual void SetItemCorner(MenuHandle menu, int item, MenuCorner corner) = 0;
	virtual MenuCorner GetItemCorner(MenuHandle menu, int item) = 0;
	virtual void SetMenuCornerCallback(MenuHandle menu, MenuItemCornerFn onCorner) = 0;
	// A token the addon has a "tint-<token>" class for, like a graffiti tint's id. "" for none.
	virtual void SetItemImageTint(MenuHandle menu, int item, const char *tint) = 0;

	// ============================== Info card ============================
	// Showcase and studio.

	// "" title hides the card. `subtitleColor` is "#RRGGBB", a rarity or "".
	virtual void SetMenuInfo(MenuHandle menu, const char *title, const char *subtitle, const char *subtitleColor) = 0;
	// A 0 to 1 bar marked at `value`, dimmed outside the range. `bands` are each band's upper end, at most 5.
	// A negative value hides it.
	virtual void SetMenuInfoMeter(MenuHandle menu, float value, float rangeMin, float rangeMax, const float *bands, int bandCount, const char *label,
								  const char *valueText) = 0;
	// At most 10. Returns the index, or -1.
	virtual int AddMenuInfoRow(MenuHandle menu, const char *label, const char *value) = 0;
	virtual void ClearMenuInfo(MenuHandle menu) = 0;

	// ============================== The header ===========================

	// A chip after the title saying what the picks go to, like "T side". "" hides it. A button with a callback.
	virtual void SetMenuScope(MenuHandle menu, const char *label, int teams) = 0;
	virtual void SetMenuScopeCallback(MenuHandle menu, MenuScopeFn onScope) = 0;

	// Unsaved changes: closing or backing out asks first.
	virtual void SetMenuEdited(MenuHandle menu, bool edited) = 0;
	virtual bool GetMenuEdited(MenuHandle menu) = 0;

	// ============================ Tabs and chips =========================
	// Every layout but the list. A click only calls back, the plugin then usually shows another menu with ReplaceMenu.
	// What a row has no room for goes behind a "+N" tab.

	// In place of the sections' tabs. `marked` puts a dot on it. The selected and any `pinned` one stay in the row.
	// Returns the index, or -1 past 10.
	virtual int AddMenuTab(MenuHandle menu, const char *label, bool selected, bool marked, bool pinned) = 0;
	virtual void SetMenuTabCallback(MenuHandle menu, MenuTabFn onTab) = 0;

	// A filter. With options a click lists them, `selected` the one picked or -1. Without, it toggles: 1 or 0.
	// An option's text after a line break is a second line. Each Add returns the index, or -1 past the row's room.
	virtual int AddMenuChip(MenuHandle menu, const char *label, const char *const *options, int optionCount, int selected) = 0;
	// A button, or with options a list of them. Keeps nothing. `accent` draws it lit.
	virtual int AddMenuAction(MenuHandle menu, const char *label, const char *const *options, int optionCount, bool accent) = 0;
	// Takes no click: `label`, then `value`.
	virtual int AddMenuNote(MenuHandle menu, const char *label, const char *value) = 0;
	// A list row's color.
	virtual void SetMenuChipOptionTone(MenuHandle menu, int chip, int option, MenuTone tone) = 0;
	virtual void SetMenuChipCallback(MenuHandle menu, MenuChipFn onChip) = 0;

	// ============================= The item area =========================

	// Showcase and studio: a small button before the pinned item's. -1 for none. Set after the items.
	virtual void SetMenuSecondaryItem(MenuHandle menu, int item) = 0;

	// Shown while a page has no items. `loading` pulses it. "" title for nothing.
	virtual void SetMenuEmpty(MenuHandle menu, const char *title, const char *text, bool loading) = 0;

	// Waits for chat with the menu up, its input field showing `prompt` and, at its end, `hint`. Ends with EndMenuInput,
	// another menu, or a click in the window, which calls `onCancel`. False on the list and the other types: suspend instead.
	virtual bool BeginMenuInput(int slot, const char *prompt, const char *hint, MenuInputCancelFn onCancel) = 0;
	virtual void EndMenuInput(int slot) = 0;
	// With a callback the input field has a clear button while it has text.
	virtual void SetMenuInputClearCallback(MenuHandle menu, MenuInputClearFn onClear) = 0;

	// ============================== The display ==========================
	// For the slot's panorama window, whatever menu is in it. False without one.

	// Under the window for `seconds`. On false, print it in chat.
	virtual bool ShowMenuMessage(int slot, const char *text, MenuTone tone, float seconds) = 0;

	// `danger` draws the confirm button red. On false, go ahead or ask in chat.
	// With kMenuTextConfirm chat and HTML ask too.
	virtual bool ShowMenuConfirm(int slot, const char *title, const char *body, const char *cancel, const char *confirm, bool danger,
								 MenuConfirmFn onDone) = 0;

	// Studio: a part of the hint pill, `keys` separated by spaces, "" for a caption. Up to 5 parts of 4 keys.
	// Stays until the display ends.
	virtual bool AddMenuHint(int slot, const char *keys, const char *text) = 0;
	virtual void ClearMenuHint(int slot) = 0;
	// No pill at all, until a part is added or the hint cleared.
	virtual void HideMenuHint(int slot) = 0;

	// Studio: a row of the key list behind the control panel's "?". Up to 8 rows of 3 keys.
	virtual bool AddMenuHelp(int slot, const char *keys, const char *text) = 0;
	virtual void ClearMenuHelp(int slot) = 0;
	// Studio: the box and the control panel swap sides, until the display ends.
	virtual void SetMenuMirrored(int slot, bool mirrored) = 0;

	// ===================== Panorama features as text =====================

	// kMenuText bits for the menu's chat and HTML displays, 0 by default.
	virtual void SetMenuTextFeatures(MenuHandle menu, int features) = 0;
	virtual int GetMenuTextFeatures(MenuHandle menu) = 0;
};

#endif // _INCLUDE_ICS2MENUS_H_
