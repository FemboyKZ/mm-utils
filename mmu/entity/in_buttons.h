#ifndef _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_
#define _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_

#include <cstdint>

// Per-button state across a tick, spread over the three m_pButtonStates words.
// A button's value is bit(states[0]) + 2 * bit(states[1]) + 4 * bit(states[2]),
// and the name spells out its up/down transitions in order.
// Anything >= IN_BUTTON_UP_DOWN was pressed at some point during the tick.
// From cs2kz src/sdk/cinbuttonstate.h.
enum EInButtonState : unsigned int
{
	IN_BUTTON_UP = 0,
	IN_BUTTON_DOWN = 1,
	IN_BUTTON_DOWN_UP = 2,
	IN_BUTTON_UP_DOWN = 3,
	IN_BUTTON_UP_DOWN_UP = 4,
	IN_BUTTON_DOWN_UP_DOWN = 5,
	IN_BUTTON_DOWN_UP_DOWN_UP = 6,
	IN_BUTTON_UP_DOWN_UP_DOWN = 7,
};

// Player button bitmask values (m_pButtonStates[0]). From CS2Fixes globaltypes.h.
namespace in_button
{
	static constexpr uint64_t Attack = 0x1;          // mouse1
	static constexpr uint64_t Jump = 0x2;            // space
	static constexpr uint64_t Duck = 0x4;            // ctrl
	static constexpr uint64_t Forward = 0x8;         // W
	static constexpr uint64_t Back = 0x10;           // S
	static constexpr uint64_t Use = 0x20;            // E
	static constexpr uint64_t MoveLeft = 0x200;      // A
	static constexpr uint64_t MoveRight = 0x400;     // D
	static constexpr uint64_t Attack2 = 0x800;       // mouse2
	static constexpr uint64_t Reload = 0x2000;       // R
	static constexpr uint64_t Speed = 0x10000;       // shift (walk)
	static constexpr uint64_t Score = 0x200000000;   // tab
	static constexpr uint64_t Inspect = 0x800000000; // F (look at weapon)
} // namespace in_button

#endif // _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_
