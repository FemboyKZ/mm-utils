#ifndef _INCLUDE_CS2MENUS_CAPI_H_
#define _INCLUDE_CS2MENUS_CAPI_H_

#include <stdint.h>

// Flat C ABI over ICS2Menus005 (see ics2menus.h).
//
// Callback lifetime: a select/end callback is a function pointer into the host's managed runtime.
// If the host unloads/hot-reloads its plugin assembly while a menu still exists,
// cs2menus will call a freed pointer.
// The host wrapper MUST cs2m_destroy every handle it created on plugin unload. See the C# wrapper.

#if defined(_WIN32)
#define CS2M_CALL __cdecl
#if defined(CS2MENUS_EXPORTS)
#define CS2M_API extern "C" __declspec(dllexport)
#else
#define CS2M_API extern "C" __declspec(dllimport)
#endif
#else
#define CS2M_CALL
#define CS2M_API extern "C" __attribute__((visibility("default")))
#endif

// Bump when the C ABI below changes incompatibly. Hosts gate on this.
// New exports appended at the end are backward-compatible (old hosts simply don't call them),
// so adding one does NOT bump this.
// Hosts probe a newer export by symbol (e.g. via NativeLibrary.TryGetExport) rather than gating on the version.
#define CS2M_ABI_VERSION 2

// Mirrors MenuType / MenuEndReason / MenuButton / MenuNavAction (ics2menus.h),
// passed as plain int across the boundary.
//   type:   -1 Default, 0 Chat, 1 Html, 2 Panorama
//   reason:  0 Selected, 1 Exit, 2 Timeout, 3 Disconnect, 4 Cancelled, 5 Destroyed
//   action:  0 Up, 1 Down, 2 Select, 3 Back
//   button:  0 Default, 1..13 W/A/S/D/Use/Speed/Duck/Jump/Reload/Attack/Attack2/Score/Inspect, 14 None
//   label:   0 Exit, 1 NextPage, 2 PrevPage, 3 Move, 4 Scroll, 5 Select, 6 On, 7 Off, 8 Adjust, 9 Done
//   style:   0 Align, 1 FontFace, 2 VisibleItems, 3 TitleColor, 4 TitleSize, 5 RawTitle, 6 ItemColor,
//            7 ItemSize, 8 DisabledColor, 9 SubmenuSuffix, 10 NavColor, 11 Marker, 12 HighlightText,
//            13 ShowCounter, 14 CounterColor, 15 CounterSize, 16 CounterFormat, 17 ShowFooter, 18 FooterColor,
//            19 FooterSize, 20 FooterSeparator, 21 FooterHintFormat, 22 FooterRangeFormat, 23 PagePrefixDelimiter,
//            24 ValueFormat, 25 EditFormat, 26 SectionFormat, 27 SectionColor
//   item type: 0 Normal, 1 Toggle, 2 Stepper, 3 Choice
//   layout:  0 List, 1 Grid, 2 Showcase, 3 Studio, 4 Columns
//   tile size: 0 Small, 1 Medium, 2 Large, 3 Cards
//   item role: 0 Button, 1 Readout, 2 Input, 3 Heading
//   corner:  0 None, 1 Star, 2 StarOn, 3 StarUndo, 4 Copy
//   tone:    0 Info, 1 Ok, 2 Warn, 3 Bad
//   teams:   bit 1 T, bit 2 CT

typedef uint32_t cs2m_handle; // 0 = invalid

// Fired when a player selects an item. `item` is the absolute index.
typedef void(CS2M_CALL *cs2m_select_cb)(cs2m_handle menu, int slot, int item, void *user);
// Fired exactly once when a player's display ends, for any reason (`reason` above).
typedef void(CS2M_CALL *cs2m_end_cb)(cs2m_handle menu, int slot, int reason, void *user);
// Fired when a player changes a Toggle, Stepper or Choice item. `value` is already stored on the item.
typedef void(CS2M_CALL *cs2m_change_cb)(cs2m_handle menu, int slot, int item, int value, void *user);
// Fired when a player presses the Panorama refresh button, shown only while one is set. Rebuild the menu's items.
typedef void(CS2M_CALL *cs2m_refresh_cb)(cs2m_handle menu, int slot, void *user);
// Fired when a player clicks a tile's corner button instead of the tile. Set the new state with cs2m_set_item_corner.
typedef void(CS2M_CALL *cs2m_corner_cb)(cs2m_handle menu, int slot, int item, void *user);
// Fired when a click calls off the typing cs2m_begin_input started.
typedef void(CS2M_CALL *cs2m_input_cancel_cb)(cs2m_handle menu, int slot, void *user);
// Fired when a player clicks the input field's clear button.
typedef void(CS2M_CALL *cs2m_input_clear_cb)(cs2m_handle menu, int slot, void *user);
// Fired when a confirm dialog is answered, `confirmed` 1 for the confirm button.
typedef void(CS2M_CALL *cs2m_confirm_cb)(int slot, int confirmed, void *user);
// Fired when a player clicks one of the menu's own tabs.
typedef void(CS2M_CALL *cs2m_tab_cb)(cs2m_handle menu, int slot, int tab, void *user);
// Fired when a player clicks the header's scope chip.
typedef void(CS2M_CALL *cs2m_scope_cb)(cs2m_handle menu, int slot, void *user);
// Fired when a player clicks a chip: a filter's `selected` is already stored on it, an action's is the option picked.
typedef void(CS2M_CALL *cs2m_chip_cb)(cs2m_handle menu, int slot, int chip, int selected, void *user);

// --- Handshake ---

// CS2M_ABI_VERSION the loaded library was built with. Gate before any other call.
CS2M_API int CS2M_CALL cs2m_abi_version(void);
// 1 if the underlying ICS2Menus005 instance is reachable.
// Reserved for future out-of-DLL acquisition, currently always 1 when the symbol resolves.
CS2M_API int CS2M_CALL cs2m_available(void);

// Buffer convention for the string getters (cs2m_get_*): copies UTF-8 (NUL-terminated) into buf,
// returns bytes needed including the NUL (> buflen means truncated). Pass buf=null/buflen=0 to query the size first.

// --- Lifetime ---

// `on_select` may be null. `user` is echoed back to on_select and to the end callback. Returns 0 on failure.
CS2M_API cs2m_handle CS2M_CALL cs2m_create(int type, const char *title, cs2m_select_cb on_select, void *user);
CS2M_API void CS2M_CALL cs2m_destroy(cs2m_handle menu);
// 1 if the handle is live (created and not destroyed).
CS2M_API int CS2M_CALL cs2m_is_valid(cs2m_handle menu);

// --- Menu properties ---

CS2M_API void CS2M_CALL cs2m_set_title(cs2m_handle menu, const char *title);
CS2M_API int CS2M_CALL cs2m_get_title(cs2m_handle menu, char *buf, int buflen);
// The menu's base render type as created (-1 Default, 0 Chat, 1 Html, 2 Panorama); -1 for an invalid handle.
CS2M_API int CS2M_CALL cs2m_get_menu_type(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_exit_button(cs2m_handle menu, int enabled);
CS2M_API int CS2M_CALL cs2m_get_exit_button(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_close_on_select(cs2m_handle menu, int enabled);
CS2M_API int CS2M_CALL cs2m_get_close_on_select(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_exit_item(cs2m_handle menu, int enabled);
CS2M_API int CS2M_CALL cs2m_get_exit_item(cs2m_handle menu);
// Lock the menu's render type so the viewer's per-player preference can't change it (force!=0). See ICS2Menus::SetMenuForceType.
CS2M_API void CS2M_CALL cs2m_set_force_type(cs2m_handle menu, int force);
CS2M_API int CS2M_CALL cs2m_get_force_type(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_start_item(cs2m_handle menu, int item);
CS2M_API int CS2M_CALL cs2m_get_start_item(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_end_callback(cs2m_handle menu, cs2m_end_cb on_end, void *user);
CS2M_API void CS2M_CALL cs2m_set_menu_key(cs2m_handle menu, int action, int button);
// The per-menu nav-key override for `action` (0 Default, 14 None, else 1..13 W..Inspect).
CS2M_API int CS2M_CALL cs2m_get_menu_key(cs2m_handle menu, int action);
// Rename a built-in label (see `label` values above). "" restores the configured default.
CS2M_API void CS2M_CALL cs2m_set_menu_label(cs2m_handle menu, int label, const char *text);
CS2M_API int CS2M_CALL cs2m_get_menu_label(cs2m_handle menu, int label, char *buf, int buflen);
// Override one HTML style field (see `style` values above). "" inherits the server default.
// Sizes take a token, colors "#RRGGBB", toggles "1"/"0", templates the format string.
CS2M_API void CS2M_CALL cs2m_set_menu_style(cs2m_handle menu, int field, const char *value);
CS2M_API int CS2M_CALL cs2m_get_menu_style(cs2m_handle menu, int field, char *buf, int buflen);

// --- Items ---

CS2M_API int CS2M_CALL cs2m_add_item(cs2m_handle menu, const char *text, const char *info, int disabled);
// Insert an item at index `pos` (clamped to [0,count]); later items shift down. Returns the index, or -1.
CS2M_API int CS2M_CALL cs2m_insert_item(cs2m_handle menu, int pos, const char *text, const char *info, int disabled);
CS2M_API int CS2M_CALL cs2m_add_submenu(cs2m_handle parent, const char *text, cs2m_handle child, const char *info);
CS2M_API void CS2M_CALL cs2m_remove_item(cs2m_handle menu, int item);
CS2M_API void CS2M_CALL cs2m_remove_all_items(cs2m_handle menu);
CS2M_API int CS2M_CALL cs2m_item_count(cs2m_handle menu);
CS2M_API void CS2M_CALL cs2m_set_item_text(cs2m_handle menu, int item, const char *text);
CS2M_API int CS2M_CALL cs2m_get_item_text(cs2m_handle menu, int item, char *buf, int buflen);
CS2M_API void CS2M_CALL cs2m_set_item_info(cs2m_handle menu, int item, const char *info);
CS2M_API int CS2M_CALL cs2m_get_item_info(cs2m_handle menu, int item, char *buf, int buflen);
CS2M_API void CS2M_CALL cs2m_set_item_disabled(cs2m_handle menu, int item, int disabled);
CS2M_API int CS2M_CALL cs2m_get_item_disabled(cs2m_handle menu, int item);
// HTML menus: render an item's text as raw Panorama markup (unescaped). See ICS2Menus::SetItemRaw.
CS2M_API void CS2M_CALL cs2m_set_item_raw(cs2m_handle menu, int item, int raw);
CS2M_API int CS2M_CALL cs2m_get_item_raw(cs2m_handle menu, int item);
// HTML menus: show an image before the item's text (icon URL / packaged path). "" removes it.
CS2M_API void CS2M_CALL cs2m_set_item_icon(cs2m_handle menu, int item, const char *url);
CS2M_API int CS2M_CALL cs2m_get_item_icon(cs2m_handle menu, int item, char *buf, int buflen);
// Attach (child!=0) or detach (child=0) a submenu on an existing item; get returns the child or 0.
CS2M_API void CS2M_CALL cs2m_set_item_submenu(cs2m_handle menu, int item, cs2m_handle child);
CS2M_API cs2m_handle CS2M_CALL cs2m_get_item_submenu(cs2m_handle menu, int item);

// --- Display ---

CS2M_API int CS2M_CALL cs2m_display(cs2m_handle menu, int slot, float duration);
CS2M_API void CS2M_CALL cs2m_display_to_all(cs2m_handle menu, float duration);
CS2M_API void CS2M_CALL cs2m_cancel(int slot);
CS2M_API int CS2M_CALL cs2m_has_menu(int slot);
CS2M_API cs2m_handle CS2M_CALL cs2m_get_active_menu(int slot);
CS2M_API int CS2M_CALL cs2m_get_active_type(int slot);
// The type a menu created as `type` would show as for this player right now.
CS2M_API int CS2M_CALL cs2m_get_slot_type(int slot, int type);
CS2M_API int CS2M_CALL cs2m_get_selected_item(int slot);

// --- Host coordination ---

// Yield (busy=1) or reclaim (busy=0) a slot for a host menu system.
// While busy, cs2menus cancels any menu on that slot and refuses new displays.
// The host drives this off its own menu open/close. cs2menus never auto-reopens.
CS2M_API void CS2M_CALL cs2m_set_external_busy(int slot, int busy);
CS2M_API int CS2M_CALL cs2m_get_external_busy(int slot);

// --- Value items ---

CS2M_API int CS2M_CALL cs2m_add_toggle(cs2m_handle menu, const char *text, int on, const char *info);
CS2M_API int CS2M_CALL cs2m_add_stepper(cs2m_handle menu, const char *text, int value, int min, int max, int step, const char *info);
// Copies the `count` option strings.
CS2M_API int CS2M_CALL cs2m_add_choice(cs2m_handle menu, const char *text, const char *const *options, int count, int selected, const char *info);
// `item type` above, 0 for an invalid handle/index.
CS2M_API int CS2M_CALL cs2m_get_item_type(cs2m_handle menu, int item);
CS2M_API void CS2M_CALL cs2m_set_item_value(cs2m_handle menu, int item, int value);
CS2M_API int CS2M_CALL cs2m_get_item_value(cs2m_handle menu, int item);
// `on_change` may be null to clear it. `user` is echoed back to it.
CS2M_API void CS2M_CALL cs2m_set_change_callback(cs2m_handle menu, cs2m_change_cb on_change, void *user);

// --- Sections and grids ---

CS2M_API int CS2M_CALL cs2m_add_section(cs2m_handle menu, const char *name);
CS2M_API int CS2M_CALL cs2m_get_item_section(cs2m_handle menu, int item);
// `layout` above.
CS2M_API void CS2M_CALL cs2m_set_menu_layout(cs2m_handle menu, int layout);
CS2M_API int CS2M_CALL cs2m_get_menu_layout(cs2m_handle menu);
// An addon image class, like "ak47", see ICS2Menus::SetMenuLayout's section. "" removes it.
CS2M_API void CS2M_CALL cs2m_set_item_image(cs2m_handle menu, int item, const char *image);
CS2M_API int CS2M_CALL cs2m_get_item_image(cs2m_handle menu, int item, char *buf, int buflen);
CS2M_API void CS2M_CALL cs2m_set_item_subtext(cs2m_handle menu, int item, const char *subtext);
CS2M_API int CS2M_CALL cs2m_get_item_subtext(cs2m_handle menu, int item, char *buf, int buflen);

// --- Panorama layouts ---

// Grid minimum, `tile size` above, default Small.
CS2M_API void CS2M_CALL cs2m_set_menu_tile_size(cs2m_handle menu, int size);
CS2M_API int CS2M_CALL cs2m_get_menu_tile_size(cs2m_handle menu);
// Inside a showcase, beside the box otherwise. An addon image class like cs2m_set_item_image's. "" removes it.
CS2M_API void CS2M_CALL cs2m_set_menu_image(cs2m_handle menu, const char *image);
CS2M_API int CS2M_CALL cs2m_get_menu_image(cs2m_handle menu, char *buf, int buflen);
// Showcase: a wide button under the page's buttons, on every page. -1 for none. Set after the items.
CS2M_API void CS2M_CALL cs2m_set_menu_pinned_item(cs2m_handle menu, int item);
CS2M_API int CS2M_CALL cs2m_get_menu_pinned_item(cs2m_handle menu);

// --- History ---
// Browser-like per display, menus in it keep their page and highlighted row.

// On top of the current menu, which Back returns to. Works from a close-on-select select callback. Clears forward history.
CS2M_API int CS2M_CALL cs2m_push(cs2m_handle menu, int slot, float duration);
// In place of the current menu, which ends (Cancelled).
CS2M_API int CS2M_CALL cs2m_replace(cs2m_handle menu, int slot, float duration);
// Back `steps` times. Needs the display open, so not from a close-on-select select callback.
CS2M_API int CS2M_CALL cs2m_step_back(int slot, int steps);
// `on_refresh` may be null to clear it, which hides the button. `user` is echoed back to it.
CS2M_API void CS2M_CALL cs2m_set_refresh_callback(cs2m_handle menu, cs2m_refresh_cb on_refresh, void *user);

// --- Pausing a display ---

// Hides the display without ending it, like while the player types in chat. No input meanwhile, the active menu stays.
// Another menu on the slot resumes it.
CS2M_API void CS2M_CALL cs2m_suspend(int slot);
CS2M_API void CS2M_CALL cs2m_resume(int slot);

// --- Item presentation ---
// Panorama only, see ICS2Menus::SetItemRole and the ones after it.

// `item role` above.
CS2M_API void CS2M_CALL cs2m_set_item_role(cs2m_handle menu, int item, int role);
CS2M_API int CS2M_CALL cs2m_get_item_role(cs2m_handle menu, int item);
CS2M_API void CS2M_CALL cs2m_set_item_highlight(cs2m_handle menu, int item, int highlight);
// 1 to 3 columns.
CS2M_API void CS2M_CALL cs2m_set_item_span(cs2m_handle menu, int item, int columns);
// Studio: the item sits in the control panel beside the preview on every page. 0/1.
CS2M_API void CS2M_CALL cs2m_set_item_control(cs2m_handle menu, int item, int control);

// --- Tile badges ---
// Panorama image tiles, see ICS2Menus::SetItemRarity and the ones after it.

CS2M_API void CS2M_CALL cs2m_set_item_rarity(cs2m_handle menu, int item, const char *rarity);
CS2M_API int CS2M_CALL cs2m_get_item_rarity(cs2m_handle menu, int item, char *buf, int buflen);
// `style` picks the tag's look, "" for the plain one.
CS2M_API void CS2M_CALL cs2m_set_item_tag(cs2m_handle menu, int item, const char *tag, const char *style);
CS2M_API int CS2M_CALL cs2m_get_item_tag(cs2m_handle menu, int item, char *buf, int buflen);
// `teams` above.
CS2M_API void CS2M_CALL cs2m_set_item_teams(cs2m_handle menu, int item, int teams);
CS2M_API int CS2M_CALL cs2m_get_item_teams(cs2m_handle menu, int item);
CS2M_API void CS2M_CALL cs2m_set_item_locked(cs2m_handle menu, int item, int locked);
CS2M_API int CS2M_CALL cs2m_get_item_locked(cs2m_handle menu, int item);
// `corner` above.
CS2M_API void CS2M_CALL cs2m_set_item_corner(cs2m_handle menu, int item, int corner);
CS2M_API int CS2M_CALL cs2m_get_item_corner(cs2m_handle menu, int item);
// `on_corner` may be null to clear it. `user` is echoed back to it.
CS2M_API void CS2M_CALL cs2m_set_corner_callback(cs2m_handle menu, cs2m_corner_cb on_corner, void *user);
CS2M_API void CS2M_CALL cs2m_set_item_image_tint(cs2m_handle menu, int item, const char *tint);

// --- Info card ---
// Studio and showcase, see ICS2Menus::SetMenuInfo.

CS2M_API void CS2M_CALL cs2m_set_menu_info(cs2m_handle menu, const char *title, const char *subtitle, const char *subtitle_color);
// `bands` may be null with count 0 for one plain bar.
CS2M_API void CS2M_CALL cs2m_set_menu_info_meter(cs2m_handle menu, float value, float range_min, float range_max, const float *bands, int count,
												 const char *label, const char *value_text);
CS2M_API int CS2M_CALL cs2m_add_menu_info_row(cs2m_handle menu, const char *label, const char *value);
CS2M_API void CS2M_CALL cs2m_clear_menu_info(cs2m_handle menu);

// --- The header ---

// See ICS2Menus::SetMenuScope, `teams` above. `on_scope` may be null to clear it.
CS2M_API void CS2M_CALL cs2m_set_menu_scope(cs2m_handle menu, const char *label, int teams);
CS2M_API void CS2M_CALL cs2m_set_scope_callback(cs2m_handle menu, cs2m_scope_cb on_scope, void *user);
CS2M_API void CS2M_CALL cs2m_set_menu_edited(cs2m_handle menu, int edited);
CS2M_API int CS2M_CALL cs2m_get_menu_edited(cs2m_handle menu);

// --- Tabs and chips ---

// See ICS2Menus::AddMenuTab. `on_tab` may be null to clear it.
CS2M_API int CS2M_CALL cs2m_add_menu_tab(cs2m_handle menu, const char *label, int selected, int marked, int pinned);
CS2M_API void CS2M_CALL cs2m_set_tab_callback(cs2m_handle menu, cs2m_tab_cb on_tab, void *user);
// See ICS2Menus::AddMenuChip and AddMenuAction. `options` may be null with count 0: a toggle chip, a plain button.
CS2M_API int CS2M_CALL cs2m_add_menu_chip(cs2m_handle menu, const char *label, const char *const *options, int count, int selected);
CS2M_API int CS2M_CALL cs2m_add_menu_action(cs2m_handle menu, const char *label, const char *const *options, int count, int accent);
// See ICS2Menus::AddMenuNote.
CS2M_API int CS2M_CALL cs2m_add_menu_note(cs2m_handle menu, const char *label, const char *value);
// See ICS2Menus::SetMenuChipOptionTone, `tone` above.
CS2M_API void CS2M_CALL cs2m_set_chip_option_tone(cs2m_handle menu, int chip, int option, int tone);
// `on_chip` may be null to clear it. `user` is echoed back to it.
CS2M_API void CS2M_CALL cs2m_set_chip_callback(cs2m_handle menu, cs2m_chip_cb on_chip, void *user);

// --- The item area ---

// See ICS2Menus::SetMenuSecondaryItem. -1 for none.
CS2M_API void CS2M_CALL cs2m_set_menu_secondary_item(cs2m_handle menu, int item);
// See ICS2Menus::SetMenuEmpty.
CS2M_API void CS2M_CALL cs2m_set_menu_empty(cs2m_handle menu, const char *title, const char *text, int loading);
// See ICS2Menus::BeginMenuInput. Returns 0 where the menu has to be suspended instead.
CS2M_API int CS2M_CALL cs2m_begin_input(int slot, const char *prompt, const char *hint, cs2m_input_cancel_cb on_cancel, void *user);
CS2M_API void CS2M_CALL cs2m_end_input(int slot);
// See ICS2Menus::SetMenuInputClearCallback. `on_clear` may be null to clear it.
CS2M_API void CS2M_CALL cs2m_set_input_clear_callback(cs2m_handle menu, cs2m_input_clear_cb on_clear, void *user);

// --- The display ---

// See ICS2Menus::ShowMenuMessage, `tone` above. Returns 0 without a panorama window shown.
CS2M_API int CS2M_CALL cs2m_show_message(int slot, const char *text, int tone, float seconds);
// See ICS2Menus::ShowMenuConfirm. Returns 0 without a panorama window shown.
CS2M_API int CS2M_CALL cs2m_show_confirm(int slot, const char *title, const char *body, const char *cancel, const char *confirm, int danger,
										 cs2m_confirm_cb on_done, void *user);
// See ICS2Menus::AddMenuHint. `keys` separated by spaces, "" for a caption.
CS2M_API int CS2M_CALL cs2m_add_hint(int slot, const char *keys, const char *text);
CS2M_API void CS2M_CALL cs2m_clear_hint(int slot);
// See ICS2Menus::HideMenuHint.
CS2M_API void CS2M_CALL cs2m_hide_hint(int slot);
// See ICS2Menus::AddMenuHelp.
CS2M_API int CS2M_CALL cs2m_add_help(int slot, const char *keys, const char *text);
CS2M_API void CS2M_CALL cs2m_clear_help(int slot);
// See ICS2Menus::SetMenuMirrored.
CS2M_API void CS2M_CALL cs2m_set_mirrored(int slot, int mirrored);

#endif // _INCLUDE_CS2MENUS_CAPI_H_
