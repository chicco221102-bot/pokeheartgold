#ifndef POKEHEARTGOLD_RANDOMIZER_H
#define POKEHEARTGOLD_RANDOMIZER_H

#include "battle/battle_setup.h"
#include "save.h"
#include "wild_encounter.h"

// Per-save randomizer. Everything is derived from the player's trainer ID,
// which is rolled anew for every new game and kept in the save, so a save
// always sees the same starters and wild Pokémon while every new game gets
// different ones.

// Random-encounter namespaces, so tables with the same index don't collide.
#define RANDOMIZER_AREA_WILD     0x00000000
#define RANDOMIZER_AREA_HEADBUTT 0x00010000
#define RANDOMIZER_AREA_SAFARI   0x00020000

void Randomizer_GetStarters(SaveData *saveData, u16 *starters);
u16 Randomizer_GetStarterSlotSpecies(SaveData *saveData, u16 species);
u16 Randomizer_GetWildSpecies(SaveData *saveData, u32 area, u16 species);
void Randomizer_RandomizeEncounterData(EncounterData *encData, u32 bank);
void Randomizer_RandomizeBugContestSpecies(SaveData *saveData, u16 *species, int count, int stride);
void Randomizer_RandomizeTrainerParty(BattleSetup *battleSetup, int partyIndex, SaveData *saveData);

#endif // POKEHEARTGOLD_RANDOMIZER_H
