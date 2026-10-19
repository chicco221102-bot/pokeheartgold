#ifndef POKEHEARTGOLD_STRUCT_DEFS_MOVE_TBL_H
#define POKEHEARTGOLD_STRUCT_DEFS_MOVE_TBL_H

#include <nitro/types.h>

typedef struct MoveTbl {
    u16 effect;
    u8 category;
    u8 power;
    u8 type;
    u8 accuracy;
    u8 pp;
    u8 effectChance;
    u16 range;
    s8 priority;
    u8 unkB;
    struct {
        u8 unkC;
        u8 contestType;
        u16 unk_E;
    };
} MoveTbl;

#endif // POKEHEARTGOLD_STRUCT_DEFS_MOVE_TBL_H
