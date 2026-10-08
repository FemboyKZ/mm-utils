#ifndef _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_
#define _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_

// The SDK's IN_* masks (InputBitMask_t), the bits of m_pButtonStates[0].
#include <in_buttons.h>

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

#endif // _INCLUDE_MMU_ENTITY_IN_BUTTONS_H_
