# Guida al modding

Questa guida spiega cosa si può modificare del gioco partendo da questo repository,
dove si trova ogni cosa e come ricompilare la ROM.

> [!WARNING]
> Il gioco non è ancora decompilato del tutto. Modificare **dati** (file JSON/CSV) è
> sicuro. Cambiare la **dimensione** di strutture o funzioni (per esempio aggiungere
> elementi a una tabella usata da codice ancora in assembly) può far crashare il gioco.

## Compilare la ROM modificata

```bash
make release        # HeartGold  -> build/heartgold.us/pokeheartgold.us.nds
make release GAME_VERSION=SOULSILVER   # SoulSilver -> build/soulsilver.us/pokesoulsilver.us.nds
```

`make` da solo esegue anche i controlli di checksum, che servono a verificare che la ROM
sia identica all'originale: dopo una qualsiasi modifica **falliscono di proposito**.
Per una ROM modificata usa `make release`.

## Pokémon

| Cosa | File | Note |
| --- | --- | --- |
| Statistiche base, tipi, abilità, tasso di cattura, EXP, strumenti tenuti, gruppi uova | `files/poketool/personal/personal.json` | Una voce per specie/forma, in ordine di numero. |
| Compatibilità MT/MN | `files/poketool/personal/personal.json` (`tms`, `hms`) | Numeri MT 1-92 e MN 1-8. |
| Evoluzioni | `files/poketool/personal/evo.json` | `method` è un `EVO_*` (`include/constants/pokemon.h`), `param` è livello/oggetto/ecc. |
| Mosse imparate per livello | `files/poketool/personal/learnsets.json` | Vedi sotto. |
| Mosse dell'insegnante | `files/fielddata/wazaoshie/waza_oshie.json` | Costanti `TUTOR_*` (`include/constants/moves.h`). |
| Starter | `src/choose_starter.c`, `src/choose_starter_app.c` | La specie è in entrambi i file: aggiornali tutti e due. |

### Learnset (`learnsets.json`)

```json
{
    "species": "SPECIES_BULBASAUR",
    "moves": [
        { "level": 1, "move": "MOVE_TACKLE" },
        { "level": 3, "move": "MOVE_GROWL" }
    ]
}
```

- Le voci sono **in ordine di indice**: specie 0-493, poi `SPECIES_EGG`, `SPECIES_BAD_EGG`
  e le forme alternative (`SPECIES_DEOXYS_ATK` … `SPECIES_ROTOM_MOW`). Non riordinarle.
- Al massimo **21 mosse** per Pokémon (il buffer del gioco ha 22 slot, uno serve al terminatore).
- Livello massimo 100; le mosse vanno in ordine crescente di livello.

## Mosse

### Dati di battaglia (`files/poketool/waza/waza_tbl.json`)

```json
{
    "move": "MOVE_POUND",
    "name": "Pound",
    "effect": "MOVE_EFFECT_HIT",
    "category": "CATEGORY_PHYSICAL",
    "power": 40,
    "type": "TYPE_NORMAL",
    "accuracy": 100,
    "pp": 35,
    "effectChance": 0,
    "range": "RANGE_SINGLE_TARGET",
    "priority": 0,
    "flags": ["MOVE_FLAG_MAKES_CONTACT", "MOVE_FLAG_CAN_PROTECT", "MOVE_FLAG_CAN_MIRROR_MOVE", "MOVE_FLAG_TRIGGERS_KINGS_ROCK", "MOVE_FLAG_KEEP_HP_BAR"],
    "contestEffect": 5,
    "contestType": "CONTEST_TYPE_TOUGH"
}
```

| Campo | Significato |
| --- | --- |
| `effect` | Effetto secondario, `MOVE_EFFECT_*` in `include/constants/move_effects.h` (alcuni effetti senza nome sono scritti come numero). |
| `category` | `CATEGORY_PHYSICAL`, `CATEGORY_SPECIAL`, `CATEGORY_STATUS`. |
| `power`, `accuracy`, `pp` | 0-255. Precisione 0 = non manca mai. |
| `effectChance` | Probabilità (%) dell'effetto secondario. |
| `range` | Bersaglio, `RANGE_*` in `include/constants/moves.h`. |
| `priority` | Da -7 a +5 (Attacco Rapido è +1). |
| `flags` | Contatto, Protezione, Magivelo, Scippo, Speculmossa, Roccia di Re… (`MOVE_FLAG_*`). |
| `contestEffect`, `contestType` | Dati per le gare (`CONTEST_TYPE_*`). |
| `move`, `name` | Solo per leggibilità: l'ordine delle voci decide l'ID della mossa. |

L'ordine delle voci **è** l'ID della mossa (voce 1 = `MOVE_POUND`, ecc.). Le ultime tre
voci (`MOVE_UNUSED_468`-`470`) sono segnaposto già presenti nella ROM originale.

### Testi delle mosse

| Cosa | File |
| --- | --- |
| Nome | `files/msgdata/msg/msg_0750.json` |
| Nome in maiuscolo (Easy Chat) | `files/msgdata/msg/msg_0751.json` |
| Descrizione (4 righe) | `files/msgdata/msg/msg_0749.json` |

In tutti e tre, la posizione nel file corrisponde all'ID della mossa.

### MT e MN

La mossa insegnata da ogni MT/MN è in `sTMHMMoves` (`src/item.c`).

### Creare una mossa nuova

Il modo **sicuro** è *sostituire* una mossa esistente poco usata: cambi i suoi dati in
`waza_tbl.json`, nome e descrizione nei file `msg_07xx.json`, e la dai ai Pokémon
tramite `learnsets.json`. Puoi riusare qualsiasi effetto esistente (`MOVE_EFFECT_*`).

Aggiungere una mossa *in più* (ID 468 o superiore) oggi è rischioso, perché:

- `NUM_MOVES` dimensiona un array dentro le strutture di battaglia
  (`include/battle/battle.h`) e altro codice ancora in assembly ne usa gli offset;
- servono anche uno script di battaglia (`files/battledata/script/move_script/`),
  un'animazione (archivio binario `files/a/0/1/0`) e testi in tre archivi;
- gli effetti nuovi richiedono nuovi script in `files/battledata/script/effect_script/`.

## Allenatori

`files/poketool/trainer/trainers.json`: classe, nome, strumenti, IA (`ai_flags`), lotta
doppia, squadra (livello, specie, opzionalmente `item`, `moves`, `form`) e frasi.

## Pokémon selvatici

`files/fielddata/encountdata/gs_enc_data.json`, un unico file per entrambe le versioni
(le specie che cambiano tra le due hanno le chiavi `HEARTGOLD`/`SOULSILVER`). Ogni mappa
ha l'erba (mattino/giorno/notte, più gli slot Hoenn/Sinnoh della radio), il surf, la
pesca (Amo Vecchio/Buono/Super), Spaccaroccia e gli sciami, con livelli minimi e massimi
per ogni slot.

## Strumenti

`files/itemtool/itemdata/item_data.csv`: prezzo, effetto quando tenuto, tasca,
funzione d'uso, cure, ecc. Nomi e descrizioni sono in `files/msgdata/msg/msg_0222.json`
(nome) e `msg_0221.json` (descrizione).

## Testi e script

- Tutti i testi del gioco: `files/msgdata/msg/msg_*.json`.
- Script degli eventi sulle mappe: `files/fielddata/script/scr_seq/`.

## Movimento e bicicletta

Nel gioco originale la bici ha 4 livelli di velocità che salgono a ogni passo. La scelta
del movimento per livello è in `sub_0205D658` (`asm/unk_0205CB48.s`): `0x4C` → `0x10` →
`0x50` → `0x14` (la velocità massima, 2 frame per casella).

**In questo repository è attiva la "bici turbo"**: tutti e quattro i livelli usano
`0x14`, quindi la bici parte subito alla velocità massima. Per tornare alla bici
originale rimetti `0x4c`, `0x10`, `0x50` nei tre `mov r4, ...` di `sub_0205D658`.

La corsa con le Scarpe da Corsa usa il movimento `0x58` (4 frame per casella) in
`sub_0205D4B4`.

Un veicolo con un aspetto proprio richiede una nuova grafica 3D del personaggio
(file `.NSBTX` in `files/data/mmodel/mmodel/`), che va creata con uno strumento
esterno come NitroPaint, più nuove righe nelle tabelle degli sprite
(`asm/overlay_01_sprite_data.s`, `src/player_avatar.c`).
