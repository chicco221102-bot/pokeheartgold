#!/usr/bin/env python3
"""Randomize wild encounters and the starter Pokémon.

Wild encounters
    Every species on every map is replaced by a random species from the whole
    National Dex (#1-#493, legendaries included). This covers grass
    (morning/day/night), surf, fishing, Rock Smash, swarms, the Hoenn/Sinnoh
    radio slots, Headbutt trees and the Safari Zone, for both HeartGold and
    SoulSilver. Levels and encounter rates are not touched, so every area keeps
    its original level range. Within one map, every slot that held the same
    species gets the same replacement, so common slots stay common.

Starters
    Picks three different random species for Prof. Elm's Poké Balls and
    updates everything that depends on them:
      - include/constants/starters.h (used by the starter app and by the
        rival's battle scripts);
      - Prof. Elm's dialogue in the starter app (msg_0190);
      - the rival's starter in every rival battle, evolved to match the level
        it had in the original game.

Usage: tools/randomizer/randomize.py [--seed N] [--no-wild] [--no-starters]
Running it again re-randomizes the current data.
"""

import argparse
import json
import os
import random
import re

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))


def path(*parts):
    return os.path.join(ROOT, *parts)


NUM_DEX_SPECIES = 493

# (file, key of the top-level list whose elements are maps/areas)
WILD_FILES = [
    (path('files', 'fielddata', 'encountdata', 'gs_enc_data.json'), 'encounters'),
    (path('files', 'arc', 'headbutt.json'), 'tables'),
    (path('files', 'arc', 'safari_enc.json'), 'encounters'),
]

STARTERS_H = path('include', 'constants', 'starters.h')
TRAINERS = path('files', 'poketool', 'trainer', 'trainers.json')
STARTER_MSG = path('files', 'msgdata', 'msg', 'msg_0190.json')
SPECIES_NAMES = path('files', 'msgdata', 'msg', 'msg_0237.json')
PERSONAL = path('files', 'poketool', 'personal', 'personal.json')
EVOS = path('files', 'poketool', 'personal', 'evo.json')

# Rival battles: trainers.json index -> (party slot, starter slot). Starter
# slot 0/1/2 is the line of STARTER_1/2/3 (originally Chikorita, Cyndaquil,
# Totodile). The original level of that party slot decides the evolution.
RIVAL_STARTER_SLOTS = {
    1: (2, 0), 2: (0, 1), 3: (0, 2),
    263: (3, 0), 264: (5, 0), 265: (0, 0),
    266: (2, 1), 267: (3, 1), 268: (5, 1),
    269: (2, 2), 270: (3, 2), 271: (4, 2), 272: (5, 2),
    285: (5, 0), 286: (5, 1), 287: (5, 2),
    288: (4, 0), 289: (4, 1),
    489: (5, 0), 490: (5, 1), 491: (5, 2),
    495: (0, 0), 496: (0, 1), 497: (0, 2),  # Cherrygrove (rival not yet named)
    735: (0, 0), 736: (0, 1), 737: (0, 2),  # Dragon's Den multi battle partner
}

# Level used for evolutions that don't happen by level (stones, trade,
# friendship, ...) when deciding which stage the rival's starter should be.
NON_LEVEL_EVO_LEVEL = 32

# Text color of the starter's name in Prof. Elm's dialogue, by primary type
# (1 = red, 2 = blue, 3 = green); other types use the color of the original
# Poké Ball in that position.
TYPE_TEXT_COLORS = {'TYPE_FIRE': 1, 'TYPE_WATER': 2, 'TYPE_ICE': 2, 'TYPE_GRASS': 3, 'TYPE_BUG': 3}
STARTER_TEXT_COLORS = [3, 1, 2]

TYPE_NAMES = {
    'TYPE_NORMAL': 'Normal', 'TYPE_FIGHTING': 'Fighting', 'TYPE_FLYING': 'Flying',
    'TYPE_POISON': 'Poison', 'TYPE_GROUND': 'Ground', 'TYPE_ROCK': 'Rock',
    'TYPE_BUG': 'Bug', 'TYPE_GHOST': 'Ghost', 'TYPE_STEEL': 'Steel',
    'TYPE_MYSTERY': '???', 'TYPE_FIRE': 'Fire', 'TYPE_WATER': 'Water',
    'TYPE_GRASS': 'Grass', 'TYPE_ELECTRIC': 'Electric', 'TYPE_PSYCHIC': 'Psychic',
    'TYPE_ICE': 'Ice', 'TYPE_DRAGON': 'Dragon', 'TYPE_DARK': 'Dark',
}

SPECIES_RE = re.compile(r'"(SPECIES_\w+)"')


def load_json(p):
    with open(p, encoding='utf-8') as f:
        return json.load(f)


def write_text(p, text):
    with open(p, 'w', encoding='utf-8', newline='') as f:
        f.write(text)


def load_species():
    """National Dex species constants, index = dex number."""
    species = {}
    with open(path('include', 'constants', 'species.h')) as f:
        for line in f:
            m = re.match(r'#define (SPECIES_\w+)\s+(\d+)\s*$', line)
            if m:
                species.setdefault(int(m.group(2)), m.group(1))
    return [species[i] for i in range(NUM_DEX_SPECIES + 1)]


# ---------------------------------------------------------------------------
# Wild encounters
# ---------------------------------------------------------------------------

def list_element_spans(text, key):
    """Character spans of each element of the top-level JSON list `key`."""
    start = text.index('"%s"' % key)
    i = text.index('[', start) + 1
    spans, depth, in_str, elem_start = [], 0, False, None
    while True:
        c = text[i]
        if in_str:
            if c == '\\':
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c in '{[':
            if depth == 0:
                elem_start = i
            depth += 1
        elif c in '}]':
            if depth == 0:
                return spans
            depth -= 1
            if depth == 0:
                spans.append((elem_start, i + 1))
        i += 1


def randomize_wild(rng, species):
    pool = species[1:]
    for file, key in WILD_FILES:
        with open(file, encoding='utf-8', newline='') as f:
            text = f.read()
        out, last = [], 0
        spans = list_element_spans(text, key)
        for begin, end in spans:
            mapping = {}  # fresh per map: consistent within the map only

            def repl(m):
                old = m.group(1)
                if old == 'SPECIES_NONE':
                    return m.group(0)
                if old not in mapping:
                    mapping[old] = rng.choice(pool)
                return '"%s"' % mapping[old]

            out.append(text[last:begin])
            out.append(SPECIES_RE.sub(repl, text[begin:end]))
            last = end
        out.append(text[last:])
        write_text(file, ''.join(out))
        print('Randomized %s (%d areas)' % (os.path.relpath(file, ROOT), len(spans)))


# ---------------------------------------------------------------------------
# Starters
# ---------------------------------------------------------------------------

def evolve_to_level(base, level, evos, rng):
    """Follow `base`'s evolutions as long as they would happen by `level`."""
    current = base
    while True:
        options = []
        for evo in evos.get(current, []):
            needed = evo['param'] if 'LEVEL' in evo['method'] else NON_LEVEL_EVO_LEVEL
            if needed <= level:
                options.append(evo['target'])
        if not options:
            return current
        current = rng.choice(sorted(options))


def randomize_starters(rng, species):
    starters = rng.sample(species[1:], 3)

    with open(STARTERS_H, encoding='utf-8', newline='') as f:
        header = f.read()
    for i, s in enumerate(starters):
        header, n = re.subn(r'(#define STARTER_%d )\w+' % (i + 1), r'\g<1>' + s, header)
        assert n == 1
    write_text(STARTERS_H, header)

    # Prof. Elm's lines in the starter app.
    names = [m['en_US'] for m in load_json(SPECIES_NAMES)['messages']]
    types = {}
    for m in load_json(PERSONAL)['baseStats']:
        types.setdefault(m['species'], m['types'][0])  # later entries are alternate forms
    msg = load_json(STARTER_MSG)
    for i, s in enumerate(starters):
        dex = species.index(s)
        type_const = types[s[len('SPECIES_'):]]
        color = TYPE_TEXT_COLORS.get(type_const, STARTER_TEXT_COLORS[i])
        name = '{COLOR %d}%s{COLOR 0}' % (color, names[dex])
        type_name = TYPE_NAMES[type_const]
        question = ['So, you like', 'You’ll take', 'Do you want'][i]
        msg['messages'][1 + i]['en_US'] = [
            'Professor Elm: %s %s,\n' % (question, name),
            'the %s-type Pokémon?' % type_name,
        ]
        msg['messages'][4 + i]['en_US'] = [
            '%s, the %s-type\n' % (name, type_name),
            'Pokémon, is in this Poké Ball!',
        ]
    write_text(STARTER_MSG, json.dumps(msg, indent=2, ensure_ascii=False) + '\n')

    # The rival's starter line in each rival battle.
    evos = {}
    for entry in load_json(EVOS)['evoTable']:
        evos[entry['baseSpecies']] = [e for e in entry['evos'] if e['method'] != 'EVO_NONE']
    trainers = load_json(TRAINERS)
    for index, (slot, starter_slot) in RIVAL_STARTER_SLOTS.items():
        mon = trainers['trainers'][index]['party'][slot]
        mon['species'] = evolve_to_level(starters[starter_slot], mon['level'], evos, rng)
        mon.pop('form', None)
    write_text(TRAINERS, json.dumps(trainers, indent=2))

    print('Starters: %s' % ', '.join(names[species.index(s)] for s in starters))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--seed', type=int, default=None, help='random seed (default: random)')
    parser.add_argument('--no-wild', action='store_true', help="don't randomize wild encounters")
    parser.add_argument('--no-starters', action='store_true', help="don't randomize the starters")
    args = parser.parse_args()

    seed = args.seed if args.seed is not None else random.randrange(1 << 32)
    rng = random.Random(seed)
    species = load_species()
    if not args.no_wild:
        randomize_wild(rng, species)
    if not args.no_starters:
        randomize_starters(rng, species)
    print('Seed: %d' % seed)


if __name__ == '__main__':
    main()
