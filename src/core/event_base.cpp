#include "core/event_base.h"
#include "core/event.h"
#include "core/error.h"

namespace nft
{
// std::string IEventBase::code = "";

KeyEvent::KeyEvent(EventHandler* event_handler, uint32_t key, uint32_t action, uint32_t mods): IEvent(event_handler)
{
	this->key = static_cast<Key>(key);

	switch (action)
	{
	case GLFW_PRESS: this->action = Action::Press; break;
	case GLFW_RELEASE: this->action = Action::Release; break;
	case GLFW_REPEAT: this->action = Action::Repeat; break;
	default: NFT_ERROR(EventFatal, "Invalid action code: {}", action);
	}

	if (mods < 0 || mods > 0x0020)
		NFT_ERROR(EventFatal, "Invalid modifier code: {}", mods);
	// if (mods & ~0x0020)
	//	NFT_ERROR(EventFatal, "Unknown modifier bits set: {}", mods);

	if (mods & GLFW_MOD_SHIFT)
		this->mods = this->mods | Modifier::Shift;
	if (mods & GLFW_MOD_CONTROL)
		this->mods = this->mods | Modifier::Control;
	if (mods & GLFW_MOD_ALT)
		this->mods = this->mods | Modifier::Alt;
	if (mods & GLFW_MOD_SUPER)
		this->mods = this->mods | Modifier::Super;
	if (mods & GLFW_MOD_CAPS_LOCK)
		this->mods = this->mods | Modifier::CapsLock;
	if (mods & GLFW_MOD_NUM_LOCK)
		this->mods = this->mods | Modifier::NumLock;
	this->event_handler->SetKeyState(this->key, this->action);
}

MouseButtonEvent::MouseButtonEvent(EventHandler* event_handler, uint32_t button, uint32_t action, uint32_t mods):
	IEvent(event_handler)
{
	this->button = static_cast<Button>(button);
	switch (action)
	{
	case GLFW_PRESS: this->action = Action::Press; break;
	case GLFW_RELEASE: this->action = Action::Release; break;
	default: NFT_ERROR(EventFatal, "Invalid mouse button action code: {}", action);
	}
	if (mods < 0 || mods > 0x0020)
		NFT_ERROR(EventFatal, "Invalid modifier code: {}", mods);
	// if (mods & ~0x0020)
	//	NFT_ERROR(EventFatal, "Unknown modifier bits set: {}", mods);
	if (mods & GLFW_MOD_SHIFT)
		this->mods = this->mods | Modifier::Shift;
	if (mods & GLFW_MOD_CONTROL)
		this->mods = this->mods | Modifier::Control;
	if (mods & GLFW_MOD_ALT)
		this->mods = this->mods | Modifier::Alt;
	if (mods & GLFW_MOD_SUPER)
		this->mods = this->mods | Modifier::Super;
	if (mods & GLFW_MOD_CAPS_LOCK)
		this->mods = this->mods | Modifier::CapsLock;
	if (mods & GLFW_MOD_NUM_LOCK)
		this->mods = this->mods | Modifier::NumLock;
}

MouseMoveEvent::MouseMoveEvent(EventHandler* event_handler, float x, float y): IEvent(event_handler), pos(x, y) {}

}	 // namespace nft