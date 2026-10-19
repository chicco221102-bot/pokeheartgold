#include "global.h"

#include "constants/pokemon.h"
#include "constants/sndseq.h"

#include "bg_window.h"
#include "font.h"
#include "message_format.h"
#include "msgdata.h"
#include "pm_string.h"
#include "pokemon.h"
#include "sound.h"
#include "system.h"
#include "text.h"

// Pokémon Summary, Skills page: SELECT cycles the stat numbers between the
// real stats, EVs, IVs and the species' base stats. The summary app itself
// is still assembly (asm/unk_02088288.s, asm/unk_0208C3E4.s); these are the
// hooks it calls. `ctx` is its work struct.

#define SUMMARY_CTX_PAGE_WINDOWS 0x224 // Window * (HP, Atk, Def, SpA, SpD, Spe, ...)
#define SUMMARY_CTX_MSGDATA      0x7A0 // MsgData * (bank 302)
#define SUMMARY_CTX_MSGFORMAT    0x7A8 // MessageFormat *
#define SUMMARY_CTX_STRING       0x7AC // String *
#define SUMMARY_CTX_PAGE         0x7BC // s8, 1 = Skills

#define SUMMARY_PAGE_SKILLS      1
#define SUMMARY_MSG_NUMBER       0x78 // "{STRVAR_1 52, 0, 0}"

void sub_0208CBD4(u8 *ctx);

enum StatViewMode {
    STAT_VIEW_STATS,
    STAT_VIEW_EV,
    STAT_VIEW_IV,
    STAT_VIEW_BASE,
    STAT_VIEW_COUNT,
};

static u8 sStatViewMode;
static u16 sAltStats[STAT_VIEW_COUNT][NUM_STATS]; // in window order: HP, Atk, Def, SpA, SpD, Spe

static const u16 sEvIds[NUM_STATS] = { MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV };
static const u16 sIvIds[NUM_STATS] = { MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV, MON_DATA_SPATK_IV, MON_DATA_SPDEF_IV, MON_DATA_SPEED_IV };
static const u8 sBaseStatIds[NUM_STATS] = { BASE_HP, BASE_ATK, BASE_DEF, BASE_SPATK, BASE_SPDEF, BASE_SPEED };

static const u16 sLabelEv[] = { CHAR_E, CHAR_V, EOS };
static const u16 sLabelIv[] = { CHAR_I, CHAR_V, EOS };
static const u16 sLabelBase[] = { CHAR_B, CHAR_a, CHAR_s, CHAR_e, EOS };
static const u16 *const sLabels[STAT_VIEW_COUNT] = { NULL, sLabelEv, sLabelIv, sLabelBase };
static const u32 sColors[STAT_VIEW_COUNT] = {
    MAKE_TEXT_COLOR(1, 2, 0),
    MAKE_TEXT_COLOR(3, 4, 0), // EV: blue
    MAKE_TEXT_COLOR(5, 6, 0), // IV: red
    MAKE_TEXT_COLOR(1, 2, 0), // base stats
};

// Called whenever the summary loads a Pokémon's data.
void PokemonSummary_CacheAltStats(Pokemon *mon) {
    int species = GetMonData(mon, MON_DATA_SPECIES, NULL);
    int form = GetMonData(mon, MON_DATA_FORM, NULL);

    for (int i = 0; i < NUM_STATS; i++) {
        sAltStats[STAT_VIEW_EV][i] = GetMonData(mon, sEvIds[i], NULL);
        sAltStats[STAT_VIEW_IV][i] = GetMonData(mon, sIvIds[i], NULL);
        sAltStats[STAT_VIEW_BASE][i] = GetMonBaseStat_HandleAlternateForm(species, form, sBaseStatIds[i]);
    }
}

static void PrintRightAligned(u8 *ctx, Window *window, u16 value, u32 color) {
    MsgData *msgData = *(MsgData **)(ctx + SUMMARY_CTX_MSGDATA);
    MessageFormat *msgFormat = *(MessageFormat **)(ctx + SUMMARY_CTX_MSGFORMAT);
    String *str = *(String **)(ctx + SUMMARY_CTX_STRING);
    String *template = NewString_ReadMsgData(msgData, SUMMARY_MSG_NUMBER);

    BufferIntegerAsString(msgFormat, 0, value, 3, PRINTING_MODE_RIGHT_ALIGN, TRUE);
    StringExpandPlaceholders(msgFormat, str, template);
    String_Delete(template);
    AddTextPrinterParameterizedWithColor(window, FONT_SYSTEM, str, GetWindowWidth(window) * 8 - FontID_String_GetWidth(FONT_SYSTEM, str, 0), 0, TEXT_SPEED_NOTRANSFER, color, NULL);
}

// Called at the end of the Skills page drawing: replaces the stat numbers.
void PokemonSummary_DrawAltStats(u8 *ctx) {
    if (sStatViewMode == STAT_VIEW_STATS) {
        return;
    }

    Window *windows = *(Window **)(ctx + SUMMARY_CTX_PAGE_WINDOWS);
    String *str = *(String **)(ctx + SUMMARY_CTX_STRING);
    u32 color = sColors[sStatViewMode];

    for (int i = 0; i < NUM_STATS; i++) {
        FillWindowPixelBuffer(&windows[i], 0);
        PrintRightAligned(ctx, &windows[i], sAltStats[sStatViewMode][i], color);
    }
    // The HP window is wider: label the view there
    CopyU16ArrayToString(str, sLabels[sStatViewMode]);
    AddTextPrinterParameterizedWithColor(&windows[0], FONT_SYSTEM, str, 0, 0, TEXT_SPEED_NOTRANSFER, color, NULL);
}

// Called from the summary's input handler. Returns TRUE if it handled input.
BOOL PokemonSummary_HandleStatViewInput(u8 *ctx) {
    if (!(gSystem.newKeys & PAD_BUTTON_SELECT) || (s8)ctx[SUMMARY_CTX_PAGE] != SUMMARY_PAGE_SKILLS) {
        return FALSE;
    }

    sStatViewMode = (sStatViewMode + 1) % STAT_VIEW_COUNT;
    PlaySE(SEQ_SE_DP_SELECT);
    sub_0208CBD4(ctx);
    return TRUE;
}
