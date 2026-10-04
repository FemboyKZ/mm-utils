#ifndef _INCLUDE_ICS2DRESSUP_H_
#define _INCLUDE_ICS2DRESSUP_H_

#include <cstdint>

// Main thread only. Ask MetaFactory for it again on OnPluginLoad and OnPluginUnload, the pointer dangles once cs2dressup unloads.
// A slot without loaded picks (empty, a bot, still loading) makes getters answer false, 0 or "" and setters false.
// `team` is kWSTeamT or kWSTeamCT, setters also take kWSTeamBoth.
// Setters don't check the player's permissions. What a player wears still goes by them, see CanUse.
// A const char * handed out aliases the plugin's storage, copy it.
// A list getter writes up to `max` ids and returns how many there are.
#define CS2DRESSUP_INTERFACE "ICS2Dressup001"

constexpr int kWSTeamBoth = 0;
constexpr int kWSTeamT = 2;
constexpr int kWSTeamCT = 3;

// A gun's model has spots for only some, see GetStickerSpots.
constexpr int kWSStickerSlots = 8;
// An agent takes GetMaxPatches() of them.
constexpr int kWSPatchSlots = 5;
constexpr int kWSNameSize = 128;

// What a pick can be besides an id. 0 is the player's own item.
constexpr int kWSAgentGloves = -1;   // gloves only, the agent model's own pair
constexpr int kWSRandom = -2;        // rolled each time it's given
constexpr int kWSRandomStarred = -3; // the same, among the player's starred ones

// WSSkin::random bits, what is rolled each time the item is given.
constexpr int kWSRollPaint = 1;
constexpr int kWSRollWear = 2;
constexpr int kWSRollSeed = 4;
constexpr int kWSRollStarred = 8; // the paint among the player's starred ones only

// WSPet::breed
constexpr int kWSPetChick = 2;
constexpr int kWSPetCatalana = 3;
constexpr int kWSPetSilkie = 4;
constexpr int kWSPetPolish = 5;
// WSPet::growth, a chick has none.
constexpr int kWSPetPullet = 2;
constexpr int kWSPetHen = 3;
// WSPet::hat from 1: helmet, armor helmet, alien, banana, glasses, nose glasses, party hat, sprout, top hat, wizard hat.
constexpr int kWSPetHats = 10;

enum class WSItemKind : int
{
	Weapon = 0, // guns, the Zeus and the C4
	Knife,
	Gloves,
	Agent,
};

enum class WSExtraKind : int
{
	Sticker = 0, // sticker kit id
	Charm,       // keychain id
	Patch,       // sticker kit id
	MusicKit,
	Pin,      // item definition index, coins, medals and trophies too
	Graffiti, // sticker kit id
};

enum class WSPick : int
{
	Knife = 0,
	Gloves,
	Agent,
	MusicKit,
	Pin,
};

// core.cfg's Teams, which team a menu pick is written to.
enum class WSTeamMode : int
{
	Shared = 0, // both
	Choice,     // the player says
	Team,       // the one picked for, and an agent only shows on its own team
};

enum class WSSetting : int
{
	Preview3D = 0,
	HideSprays,
	HidePets, // other players'
};

enum class WSFavourite : int
{
	Paint = 1, // per item, the id is WSPaintFavourite's
	Sticker,
	Charm,
	Patch,
	Agent,
	MusicKit,
	Pin,
	Model, // knife and glove models
	Graffiti,
};

constexpr int WSPaintFavourite(int defIndex, int paintKit)
{
	return (defIndex << 16) | paintKit;
}

// core.cfg's Permissions.
enum class WSFeature : int
{
	Weapons = 0,
	Knives,
	Gloves,
	Agents,
	MusicKits,
	Pins,
	Pets,
	Guns, // !guns
	Graffiti,
	Preview,
};

enum class WSResult : int
{
	Allow = 0,
	Block,
};

enum class WSInspectForm : int
{
	Code = 0,
	Command, // csgo_econ_action_preview <code>
	Link,    // steam://
};

enum class WSInspectParse : int
{
	Ok = 0,
	Invalid,
	NeedsSteam, // an inventory or market link, the item is only on Steam's servers
};

enum class WSPatternKind : int
{
	None = 0,
	Fade,
	Blue,  // Case Hardened and Heat Treated
	Named, // like Fire & Ice
};

struct WSSticker
{
	int32_t id = 0;
	float wear = 0.0f; // 0 new to 1 scraped off
	// Off its spot, the rotation in degrees.
	float x = 0.0f;
	float y = 0.0f;
	float rotation = 0.0f;
};

struct WSSkin
{
	int32_t paint = 0; // paint kit id, 0 for vanilla
	float wear = 0.0f;
	int32_t seed = 0; // 0 to 1000
	char nametag[kWSNameSize] = "";
	bool stattrak = false;
	int32_t kills = 0;
	bool souvenir = false; // guns only, never with StatTrak
	WSSticker stickers[kWSStickerSlots];
	int32_t charm = 0;
	int32_t charmSeed = 0;
	int32_t charmSticker = 0;   // sealed in a Sticker Slab
	int32_t charmHighlight = 0; // an Austin or Budapest charm's highlight reel
	// Units along (x), across (y) and up (z) the gun. Unplaced, the game hangs it.
	bool charmPlaced = false;
	float charmX = 0.0f;
	float charmY = 0.0f;
	float charmZ = 0.0f;
	int32_t random = 0; // kWSRoll bits
};

struct WSPet
{
	int32_t breed = 0; // 0 for no pet
	int32_t seed = 0;  // feather colors, 0 to 100000
	int32_t growth = kWSPetHen;
	int32_t hat = 0;
	char name[kWSNameSize] = "";
};

// The strings of the three Info structs last until OnItemsLoaded.
struct WSItemInfo
{
	int32_t defIndex = 0;
	WSItemKind kind = WSItemKind::Weapon;
	const char *name = "";        // items_game name, like weapon_ak47
	const char *displayName = ""; // English
	const char *category = "";    // weapons only, like Pistols
	const char *model = "";       // agents only
	int32_t team = 0;             // 0 unless it's one team's
	int32_t rarity = 0;           // 1 to 7
	int32_t paintKits = 0;        // how many
	bool stock = false;           // a team's default knife or agent
	bool patchable = false;
};

struct WSPaintKitInfo
{
	int32_t id = 0;
	const char *name = "";
	const char *displayName = ""; // English
	// The range it drops in, SetSkin takes 0 to 1 regardless.
	float wearMin = 0.0f;
	float wearMax = 1.0f;
	int32_t rarity = 0;
	bool legacyModel = false; // the weapon's old model, with sticker spots of its own
};

struct WSExtraInfo
{
	int32_t id = 0;
	const char *name = "";
	const char *displayName = ""; // English
	const char *folder = "";      // its collection, see GetExtraFolderName
	const char *model = "";       // music kits and pins only, "" when the game ships none
	int32_t rarity = 0;           // music kits and pins only
};

// WSInspectItem::defIndex of what has no item definition of its own.
constexpr int kWSInspectMusicKit = 1314;
constexpr int kWSInspectPet = 4681;
constexpr int kWSInspectGraffiti = 1348;

struct WSInspectItem
{
	int32_t defIndex = 0;
	// An agent's patches and a graffiti's design are sticker ids. A pet's colors are the seed, its name the nametag.
	WSSkin skin;
	int32_t tint = 0;     // a graffiti's
	int32_t musicKit = 0; // skin.kills are its MVPs
	int32_t pet = 0;      // breed
	int32_t petGrowth = 0;
};

struct WSPatternInfo
{
	WSPatternKind kind = WSPatternKind::None;
	// Fade: percent in values[0]. Blue: percent per side, sides[1] "" for a skin with one.
	float values[2] = {};
	const char *sides[2] = {"", ""};
	int32_t place = 0; // among the item's 1001 patterns, from 1
	// Named and the bluest: "Fire & Ice", rank "Max %d", tier 1.
	const char *name = "";
	const char *rank = "";
	int32_t tier = 0;
};

struct WSSavedLoadout
{
	int32_t id = 0;
	char name[kWSNameSize] = "";
	bool active = false; // the one worn
};

struct WSLook
{
	int32_t id = 0;
	char name[kWSNameSize] = "";
	WSSkin skin;
};

// Remove it in your Unload(). Callbacks run after the change is saved, whoever made it, once per team written.
class ICS2DressupListener
{
public:
	virtual ~ICS2DressupListener() = default;

	// Also after !wsreload.
	virtual void OnItemsLoaded() {}

	// Also after !wsreload and ReloadPlayer.
	virtual void OnPlayerLoaded(int /*slot*/) {}

	// `skin` is null when it was taken off, and only valid for the call.
	virtual void OnSkinChanged(int /*slot*/, int /*team*/, int /*defIndex*/, const WSSkin * /*skin*/) {}

	virtual void OnPickChanged(int /*slot*/, int /*team*/, WSPick /*pick*/, int /*id*/) {}

	virtual void OnPatchesChanged(int /*slot*/, int /*team*/, int /*agent*/) {}

	virtual void OnPetChanged(int /*slot*/) {}

	virtual void OnGraffitiChanged(int /*slot*/) {}

	// Any pick may have changed: a saved loadout was equipped or picks were reset.
	virtual void OnPicksReplaced(int /*slot*/) {}

	virtual void OnStatTrakKill(int /*slot*/, int /*team*/, int /*defIndex*/, int /*kills*/) {}

	virtual void OnMusicKitMvp(int /*slot*/, int /*team*/, int /*musicKit*/, int /*mvps*/) {}

	virtual void OnSpray(int /*slot*/, int /*graffiti*/, int /*tint*/, int /*entity*/) {}

	// In between the player is frozen and, by core.cfg's defaults, takes no damage and isn't drawn.
	virtual void OnPreviewStart(int /*slot*/) {}

	virtual void OnPreviewEnd(int /*slot*/) {}

	// Slot -1 is the server console. `command` is COMMANDS.md's name without a prefix.
	// Fires after the command's permission check. Block stops it silently.
	virtual WSResult OnCommand(int /*slot*/, const char * /*command*/, const char * /*args*/)
	{
		return WSResult::Allow;
	}
};

class ICS2Dressup
{
public:
	virtual bool AddListener(ICS2DressupListener *listener) = 0;
	virtual bool RemoveListener(ICS2DressupListener *listener) = 0;

	virtual bool AreItemsLoaded() = 0;
	// The agents' mode or the other picks'. Writes here go to the team given whatever the mode.
	virtual WSTeamMode GetTeamMode(bool agents) = 0;

	// Without a team's default knife and agents.
	virtual int GetItems(WSItemKind kind, int32_t *defIndexes, int max) = 0;
	virtual bool GetItem(int defIndex, WSItemInfo *out) = 0;
	// Names come in the game language of the player in `slot`, English for -1.
	virtual const char *GetItemName(int defIndex, int slot) = 0;
	virtual int GetItemPaintKits(int defIndex, int32_t *paintKits, int max) = 0;
	// Bit N: the gun has a spot for sticker slot N in that paint kit.
	virtual uint8_t GetStickerSpots(int defIndex, int paintKit) = 0;
	virtual bool GetPaintKit(int paintKit, WSPaintKitInfo *out) = 0;
	virtual const char *GetPaintKitName(int paintKit, int slot) = 0;
	virtual int GetExtras(WSExtraKind kind, int32_t *ids, int max) = 0;
	virtual bool GetExtra(WSExtraKind kind, int id, WSExtraInfo *out) = 0;
	virtual const char *GetExtraName(WSExtraKind kind, int id, int slot) = 0;
	virtual const char *GetExtraFolderName(WSExtraKind kind, const char *folder, int slot) = 0;
	virtual int GetGraffitiTints(int32_t *ids, int max) = 0;
	virtual const char *GetGraffitiTintName(int tint) = 0;
	virtual int GetMaxPatches() = 0;
	// False for a pattern nothing is known of.
	virtual bool GetPatternInfo(int defIndex, int paintKit, int seed, WSPatternInfo *out) = 0;

	// Neither checks ids against the catalog.
	virtual const char *GetInspectText(const WSInspectItem *item, WSInspectForm form) = 0;
	virtual WSInspectParse ParseInspect(const char *text, WSInspectItem *out) = 0;

	virtual bool IsPlayerLoaded(int slot) = 0;
	// From the database. False without a connection.
	virtual bool ReloadPlayer(int slot) = 0;
	virtual bool CanUse(int slot, WSFeature feature) = 0;
	// 0 when dead or holding someone else's weapon.
	virtual int GetHeldItem(int slot) = 0;
	// Read off a living player's entities, so random picks come out as rolled.
	virtual int GetWornItems(int slot, WSInspectItem *items, int max) = 0;

	// False without a saved skin, the player's own item shows then.
	virtual bool GetSkin(int slot, int team, int defIndex, WSSkin *out) = 0;
	virtual int GetSkinItems(int slot, int team, int32_t *defIndexes, int max) = 0;
	// False for a paint kit the item doesn't take, for gloves 0 too unless kWSRollPaint is set.
	// The rest is clamped or dropped like !import does. `kills` is kept as given.
	// A knife's or gloves' skin shows once that model is the pick.
	virtual bool SetSkin(int slot, int team, int defIndex, const WSSkin *skin) = 0;
	virtual bool RemoveSkin(int slot, int team, int defIndex) = 0;

	virtual int GetPick(int slot, int team, WSPick pick) = 0;
	virtual bool SetPick(int slot, int team, WSPick pick, int id) = 0;
	virtual bool GetMusicStatTrak(int slot, int team) = 0;
	virtual bool SetMusicStatTrak(int slot, int team, bool on) = 0;
	// The kit's own count, not the shared one the scoreboard may show.
	virtual int GetMusicMvps(int slot, int team, int musicKit) = 0;
	virtual bool SetMusicMvps(int slot, int team, int musicKit, int mvps) = 0;
	// Writes kWSPatchSlots ids.
	virtual bool GetPatches(int slot, int team, int agent, int32_t *patches) = 0;
	virtual bool SetPatch(int slot, int team, int agent, int patchSlot, int patch) = 0;

	virtual bool GetPet(int slot, WSPet *out) = 0;
	virtual bool SetPet(int slot, const WSPet *pet) = 0;
	// Entity index, -1 without one out.
	virtual int GetPetEntity(int slot) = 0;
	// Slot, -1 for any other entity.
	virtual int GetPetOwner(int entity) = 0;

	// Either pointer may be null.
	virtual bool GetGraffiti(int slot, int *graffiti, int *tint) = 0;
	virtual bool SetGraffiti(int slot, int graffiti, int tint) = 0;
	// No cooldown. `graffiti` 0 sprays the player's own pick.
	virtual bool Spray(int slot, int graffiti, int tint) = 0;
	// 0 for everyone's. Returns how many went.
	virtual int ClearSprays(uint64_t steamID64) = 0;

	virtual bool GetSetting(int slot, WSSetting setting) = 0;
	virtual bool SetSetting(int slot, WSSetting setting, bool on) = 0;
	virtual bool IsFavourite(int slot, WSFavourite kind, int id) = 0;
	virtual int GetFavourites(int slot, WSFavourite kind, int32_t *ids, int max) = 0;
	virtual bool SetFavourite(int slot, WSFavourite kind, int id, bool starred) = 0;

	// None until the player made a second one.
	virtual int GetSavedLoadouts(int slot, int32_t *ids, int max) = 0;
	virtual bool GetSavedLoadout(int slot, int id, WSSavedLoadout *out) = 0;
	// False for the active one.
	virtual bool EquipSavedLoadout(int slot, int id) = 0;
	virtual int GetLooks(int slot, int defIndex, int32_t *ids, int max) = 0;
	virtual bool GetLook(int slot, int defIndex, int id, WSLook *out) = 0;

	virtual bool CanPreview(int slot) = 0;
	virtual bool IsPreviewing(int slot) = 0;
	// Closes its menu too.
	virtual bool ClosePreview(int slot) = 0;
	// The bot an agent's preview stands on, a real one that takes a slot.
	virtual bool IsPreviewBot(int slot) = 0;

	// Like !guns, the weapon in that slot drops.
	virtual bool GiveWeapon(int slot, int defIndex) = 0;
	// As if the player typed it: permissions, the cooldown and OnCommand apply. Slot -1 is the server console.
	// `command` without a prefix, `args` as in COMMANDS.md or null.
	virtual bool RunCommand(int slot, const char *command, const char *args) = 0;
};

#endif // _INCLUDE_ICS2DRESSUP_H_
