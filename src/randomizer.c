#include "randomizer.h"

#include "global.h"

#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/trainers.h"

#include "struct_defs/personal_narc_structs.h"

#include "party.h"
#include "player_data.h"
#include "pokemon.h"

// The three original starters, in Poké Ball order. Scripts that choose the
// rival's team compare the player's choice against these, so the starter
// variable keeps holding them instead of the randomized species.
static const u16 sVanillaStarters[3] = { SPECIES_CHIKORITA, SPECIES_CYNDAQUIL, SPECIES_TOTODILE };

// Level assumed for evolutions that don't happen by level (stones, trade,
// friendship, ...) when the rival's starter is evolved to fit a battle.
#define NON_LEVEL_EVO_LEVEL 32

// Rival battles: which party slot holds the rival's starter and which
// starter line it is (0/1/2 = Chikorita/Cyndaquil/Totodile line).
static const struct {
    u16 trainerId;
    u8 partySlot;
    u8 starterSlot;
} sRivalStarterSlots[] = {
    { 1,   2, 0 },
    { 2,   0, 1 },
    { 3,   0, 2 },
    { 263, 3, 0 },
    { 264, 5, 0 },
    { 265, 0, 0 },
    { 266, 2, 1 },
    { 267, 3, 1 },
    { 268, 5, 1 },
    { 269, 2, 2 },
    { 270, 3, 2 },
    { 271, 4, 2 },
    { 272, 5, 2 },
    { 285, 5, 0 },
    { 286, 5, 1 },
    { 287, 5, 2 },
    { 288, 4, 0 },
    { 289, 4, 1 },
    { 489, 5, 0 },
    { 490, 5, 1 },
    { 491, 5, 2 },
    { 495, 0, 0 }, // Cherrygrove City, before the rival is named
    { 496, 0, 1 },
    { 497, 0, 2 },
    { 735, 0, 0 }, // Dragon's Den multi battle partner
    { 736, 0, 1 },
    { 737, 0, 2 },
};

static u32 Randomizer_Hash(u32 x) {
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    x *= 0x846CA68Bu;
    x ^= x >> 16;
    return x;
}

static u32 Randomizer_GetSeed(SaveData *saveData) {
    return PlayerProfile_GetTrainerID(Save_PlayerData_GetProfile(saveData));
}

// A species from the whole National Dex, legendaries included. Unown is left
// out: its letter depends on the Ruins of Alph puzzles, and with none solved
// some encounter paths can't create it.
static u16 Randomizer_PickSpecies(u32 seed, u32 key1, u32 key2) {
    u32 hash = Randomizer_Hash(seed ^ Randomizer_Hash(key1 * 0x9E3779B1u + key2));
    while (TRUE) {
        u16 species = hash % NATIONAL_DEX_COUNT + 1;
        if (species != SPECIES_UNOWN) {
            return species;
        }
        hash = Randomizer_Hash(hash + 1);
    }
}

void Randomizer_GetStarters(SaveData *saveData, u16 *starters) {
    u32 seed = Randomizer_GetSeed(saveData);
    u32 attempt = 0;

    for (int i = 0; i < 3; i++) {
        BOOL duplicate;
        do {
            starters[i] = Randomizer_PickSpecies(seed, 0x53544152, attempt++); // 'STAR'
            duplicate = FALSE;
            for (int j = 0; j < i; j++) {
                if (starters[j] == starters[i]) {
                    duplicate = TRUE;
                }
            }
        } while (duplicate);
    }
}

u16 Randomizer_GetStarterSlotSpecies(SaveData *saveData, u16 species) {
    u16 starters[3];
    Randomizer_GetStarters(saveData, starters);
    for (int i = 0; i < 3; i++) {
        if (starters[i] == species) {
            return sVanillaStarters[i];
        }
    }
    return species;
}

u16 Randomizer_GetWildSpecies(SaveData *saveData, u32 area, u16 species) {
    if (species == SPECIES_NONE || species > NATIONAL_DEX_COUNT) {
        return species;
    }
    return Randomizer_PickSpecies(Randomizer_GetSeed(saveData), 0x57494C44 ^ area, species); // 'WILD'
}

void Randomizer_RandomizeEncounterData(EncounterData *encData, u32 bank) {
    SaveData *saveData = SaveData_Get();
    u32 area = RANDOMIZER_AREA_WILD | bank;
    int i;

    for (i = 0; i < NUM_ENCOUNTERS_LAND; i++) {
        encData->landSlots.species_morn[i] = Randomizer_GetWildSpecies(saveData, area, encData->landSlots.species_morn[i]);
        encData->landSlots.species_day[i] = Randomizer_GetWildSpecies(saveData, area, encData->landSlots.species_day[i]);
        encData->landSlots.species_nite[i] = Randomizer_GetWildSpecies(saveData, area, encData->landSlots.species_nite[i]);
    }
    for (i = 0; i < NUM_ENCOUNTERS_SOUNDS; i++) {
        encData->hoennSoundsSpecies[i] = Randomizer_GetWildSpecies(saveData, area, encData->hoennSoundsSpecies[i]);
        encData->sinnohSoundsSpecies[i] = Randomizer_GetWildSpecies(saveData, area, encData->sinnohSoundsSpecies[i]);
    }
    for (i = 0; i < NUM_ENCOUNTERS_SURF; i++) {
        encData->surfSlots[i].species = Randomizer_GetWildSpecies(saveData, area, encData->surfSlots[i].species);
    }
    for (i = 0; i < NUM_ENCOUNTERS_ROCKSMASH; i++) {
        encData->rockSmashSlots[i].species = Randomizer_GetWildSpecies(saveData, area, encData->rockSmashSlots[i].species);
    }
    for (i = 0; i < NUM_ENCOUNTERS_FISH; i++) {
        encData->oldRodSlots[i].species = Randomizer_GetWildSpecies(saveData, area, encData->oldRodSlots[i].species);
        encData->goodRodSlots[i].species = Randomizer_GetWildSpecies(saveData, area, encData->goodRodSlots[i].species);
        encData->superRodSlots[i].species = Randomizer_GetWildSpecies(saveData, area, encData->superRodSlots[i].species);
    }
    encData->landSwarm = Randomizer_GetWildSpecies(saveData, area, encData->landSwarm);
    encData->surfSwarm = Randomizer_GetWildSpecies(saveData, area, encData->surfSwarm);
    encData->nightFish = Randomizer_GetWildSpecies(saveData, area, encData->nightFish);
    encData->fishSwarm = Randomizer_GetWildSpecies(saveData, area, encData->fishSwarm);
}

// The Bug-Catching Contest judges a catch by looking its species up in the
// contest table, so the replacements have to stay distinct.
void Randomizer_RandomizeBugContestSpecies(SaveData *saveData, u16 *species, int count, int stride) {
    u32 seed = Randomizer_GetSeed(saveData);
    u32 attempt = 0;

    for (int i = 0; i < count; i++) {
        u16 *cur = (u16 *)((u8 *)species + i * stride);
        BOOL duplicate;
        do {
            *cur = Randomizer_PickSpecies(seed, 0x42554753, attempt++); // 'BUGS'
            duplicate = FALSE;
            for (int j = 0; j < i; j++) {
                if (*(u16 *)((u8 *)species + j * stride) == *cur) {
                    duplicate = TRUE;
                }
            }
        } while (duplicate);
    }
}

// Follow the species' evolutions as long as they would have happened by `level`.
static u16 Randomizer_EvolveToLevel(u16 species, int level) {
    struct Evolution evos[MAX_EVOS_PER_POKE + 1]; // the NARC member is padded past MAX_EVOS_PER_POKE

    for (int stage = 0; stage < 3; stage++) {
        u16 next = SPECIES_NONE;
        LoadMonEvolutionTable(species, evos);
        for (int i = 0; i < MAX_EVOS_PER_POKE && evos[i].method != EVO_NONE; i++) {
            int needed;
            switch (evos[i].method) {
            case EVO_LEVEL:
            case EVO_LEVEL_ATK_GT_DEF:
            case EVO_LEVEL_ATK_EQ_DEF:
            case EVO_LEVEL_ATK_LT_DEF:
            case EVO_LEVEL_PID_LO:
            case EVO_LEVEL_PID_HI:
            case EVO_LEVEL_NINJASK:
            case EVO_LEVEL_SHEDINJA:
            case EVO_LEVEL_MALE:
            case EVO_LEVEL_FEMALE:
                needed = evos[i].param;
                break;
            default:
                needed = NON_LEVEL_EVO_LEVEL;
                break;
            }
            if (needed <= level) {
                next = evos[i].target;
                break;
            }
        }
        if (next == SPECIES_NONE) {
            break;
        }
        species = next;
    }
    return species;
}

// Swap the rival's starter for the matching randomized starter, evolved to
// fit the battle's level. It gets its natural level-up moves.
void Randomizer_ApplyRivalStarter(BattleSetup *battleSetup, int partyIndex, SaveData *saveData) {
    u32 trainerId = battleSetup->trainerId[partyIndex];

    for (int i = 0; i < (int)NELEMS(sRivalStarterSlots); i++) {
        if (sRivalStarterSlots[i].trainerId != trainerId) {
            continue;
        }
        Pokemon *mon = Party_GetMonByIndex(battleSetup->party[partyIndex], sRivalStarterSlots[i].partySlot);
        if (mon == NULL || GetMonData(mon, MON_DATA_SPECIES, NULL) == SPECIES_NONE) {
            return;
        }

        u16 starters[3];
        Randomizer_GetStarters(saveData, starters);
        int level = GetMonData(mon, MON_DATA_LEVEL, NULL);
        int iv = GetMonData(mon, MON_DATA_HP_IV, NULL);
        u32 personality = GetMonData(mon, MON_DATA_PERSONALITY, NULL);
        u16 heldItem = GetMonData(mon, MON_DATA_HELD_ITEM, NULL);
        u16 species = Randomizer_EvolveToLevel(starters[sRivalStarterSlots[i].starterSlot], level);

        CreateMon(mon, species, level, iv, TRUE, personality, OT_ID_RANDOM_NO_SHINY, 0);
        SetMonData(mon, MON_DATA_HELD_ITEM, &heldItem);
        return;
    }
}
