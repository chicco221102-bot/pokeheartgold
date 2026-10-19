#ifndef POKEHEARTGOLD_MOVE_H
#define POKEHEARTGOLD_MOVE_H

#include "global.h"

#include "struct_defs/move_tbl.h"

typedef enum MoveAttr {
    MOVEATTR_EFFECT = 0,
    MOVEATTR_CLASS,
    MOVEATTR_POWER,
    MOVEATTR_TYPE,
    MOVEATTR_ACCURACY,
    MOVEATTR_PP,
    MOVEATTR_EFFECT_CHANCE,
    MOVEATTR_RANGE,
    MOVEATTR_PRIORTY,
    MOVEATTR_UNK9,
    MOVEATTR_UNK10,
    MOVEATTR_CONTEST_TYPE,
} MoveAttr;

u32 GetMoveAttr(u16 moveId, MoveAttr attrno);
u8 GetMoveMaxPP(u16 moveId, u8 ppUps);
void LoadMoveTbl(MoveTbl *dest);
u32 GetMoveTblAttr(MoveTbl *moveTbl, MoveAttr attr);
void LoadMoveEntry(u16 moveId, MoveTbl *moveTbl);

#endif // POKEHEARTGOLD_MOVE_H
