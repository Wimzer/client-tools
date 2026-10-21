#ifndef INCLUDED_InputSchemeHolocron_H
#define INCLUDED_InputSchemeHolocron_H

// The existing InputScheme owns profile loading and saving. Keep the migration
// operation separately testable without changing that owner or the InputMap API.
namespace InputSchemeHolocron
{
    template <typename Map>
    bool migrate(Map & map, int sceneBit, int & completed, int key, void (*saveOptions)())
    {
        if ((completed & sceneBit) != 0)
            return false;
        typename Map::Command const * const command = map.findCommandByName("CMD_uiHolocron", true);
        if (!command)
            return false;

        typename Map::CommandBindInfoSet * bindings = 0;
        unsigned int const count = map.getCommandBindings(bindings, command);
        bool alreadyBound = false;
        for (unsigned int i = 0; i < count; ++i)
            alreadyBound = alreadyBound || bindings[i].numBinds != 0;
        delete [] bindings;

        // The shipped maps assign bit 2 to Control. Preserve both a custom chord
        // occupant and a user's alternative binding for the Holocron itself.
        typename Map::BindInfo const chord(2, Map::IT_Key, key);
        if (!alreadyBound && !map.getCommandByBinding(chord))
        {
            if (!map.addBinding(chord, command) || !map.write(0, false))
                return false;
        }
        completed |= sceneBit;
        saveOptions();
        return true;
    }
}

#endif
