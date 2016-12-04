#pragma once
#include <daedalus/DaedalusGameState.h>
#include <set>
#include "Controller.h"
#include "LogicDef.h"

namespace Logic
{
	class ItemController : public Controller
	{
	public:
		/**
		 * @param world World of the underlaying entity
		 * @param entity Entity owning this controller
		 */
		ItemController(World::WorldInstance& world, Handle::EntityHandle entity, Daedalus::GameState::ItemHandle scriptInstance);

		/**
         * @return The type of this class. If you are adding a new base controller, be sure to add it to ControllerTypes.h
         */
		virtual EControllerType getControllerType(){ return EControllerType::ItemController; }

		/**
		 * Removes this item from the world and adds it to the given NPCs inventory
		 * @param npc NPC to give the item to
		 */
		void pickUp(Handle::EntityHandle npc);

		/**
		 * Updates this vob from the script-side values (visual, etc)
		 */
		void updateVobFromScript();

	protected:

		struct {
			Daedalus::GameState::ItemHandle scriptInstance;
		}m_ScriptState;

	};
}
