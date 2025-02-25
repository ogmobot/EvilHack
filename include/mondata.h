/* NetHack 3.6	mondata.h	$NHDT-Date: 1550524558 2019/02/18 21:15:58 $  $NHDT-Branch: NetHack-3.6.2-beta01 $:$NHDT-Revision: 1.37 $ */
/* Copyright (c) 1989 Mike Threepoint				  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef MONDATA_H
#define MONDATA_H

#define verysmall(ptr) ((ptr)->msize < MZ_SMALL)
#define bigmonst(ptr) ((ptr)->msize >= MZ_LARGE)
#define biggermonst(ptr) ((ptr)->msize > (youmonst.data)->msize)

#define pm_resistance(ptr, typ) (((ptr)->mresists & (typ)) != 0)

#define resists_fire(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_FIRE) != 0)
#define resists_cold(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_COLD) != 0)
#define resists_sleep(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_SLEEP) != 0)
#define resists_disint(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_DISINT) != 0)
#define resists_elec(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_ELEC) != 0)
#define resists_poison(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_POISON) != 0)
#define resists_acid(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_ACID) != 0)
#define resists_ston(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_STONE) != 0)
#define resists_psychic(mon) \
    ((((mon)->data->mresists | (mon)->mextrinsics) & MR_PSYCHIC) != 0)

#define has_telepathy(mon) \
    (telepathic((mon)->data) || ((mon)->mextrinsics & MR2_TELEPATHY) != 0)

#define can_wwalk(mon) (((mon)->mextrinsics & MR2_WATERWALK) != 0)
#define can_jump(mon)  (((mon)->mextrinsics & MR2_JUMPING) != 0)
#define has_displacement(mon) (((mon)->mextrinsics & MR2_DISPLACED) != 0)

#define resists_sick(mon)  ((mon)->data->mlet == S_FUNGUS                \
                            || is_undead((mon)->data)                    \
                            || (mon)->data == &mons[PM_BABY_GOLD_DRAGON] \
                            || (mon)->data == &mons[PM_GOLD_DRAGON])

#define is_lminion(mon) \
    (is_minion((mon)->data) && mon_aligntyp(mon) == A_LAWFUL)
#define is_jumper(ptr) ((ptr) == &mons[PM_HUMAN_KNIGHT]        \
                        || (ptr) == &mons[PM_JUMPING_SPIDER]   \
                        || (ptr) == &mons[PM_GIANT_CENTIPEDE])
#define is_flyer(ptr) (((ptr)->mflags1 & M1_FLY) != 0L)
#define is_floater(ptr) ((ptr)->mlet == S_EYE || (ptr)->mlet == S_LIGHT)
#define is_clinger(ptr) (((ptr)->mflags1 & M1_CLING) != 0L)
#define is_swimmer(ptr) (((ptr)->mflags1 & M1_SWIM) != 0L)
#define breathless(ptr) (((ptr)->mflags1 & M1_BREATHLESS) != 0L)
#define amphibious(ptr) \
    (((ptr)->mflags1 & (M1_AMPHIBIOUS | M1_BREATHLESS)) != 0L)
#define passes_walls(ptr) (((ptr)->mflags1 & M1_WALLWALK) != 0L)
#define amorphous(ptr) (((ptr)->mflags1 & M1_AMORPHOUS) != 0L)
#define noncorporeal(ptr) ((ptr)->mlet == S_GHOST)
#define tunnels(ptr) (((ptr)->mflags1 & M1_TUNNEL) != 0L)
#define needspick(ptr) (((ptr)->mflags1 & M1_NEEDPICK) != 0L)
#define hides_under(ptr) (((ptr)->mflags1 & M1_CONCEAL) != 0L)
#define is_hider(ptr) (((ptr)->mflags1 & M1_HIDE) != 0L)
#define haseyes(ptr) (((ptr)->mflags1 & M1_NOEYES) == 0L)
#define eyecount(ptr)                                         \
    (!haseyes(ptr) ? 0 : ((ptr) == &mons[PM_CYCLOPS]          \
                          || (ptr) == &mons[PM_FLOATING_EYE]) \
                             ? 1                              \
                             : 2)
#define nohands(ptr) (((ptr)->mflags1 & M1_NOHANDS) != 0L)
#define nolimbs(ptr) (((ptr)->mflags1 & M1_NOLIMBS) == M1_NOLIMBS)
#define notake(ptr) (((ptr)->mflags1 & M1_NOTAKE) != 0L)
#define has_head(ptr) (((ptr)->mflags1 & M1_NOHEAD) == 0L)
#define has_horns(ptr) (num_horns(ptr) > 0)
#define is_whirly(ptr) \
    ((ptr)->mlet == S_VORTEX || (ptr) == &mons[PM_AIR_ELEMENTAL])
#define flaming(ptr)                                                     \
    ((ptr) == &mons[PM_FIRE_VORTEX] || (ptr) == &mons[PM_FLAMING_SPHERE] \
     || (ptr) == &mons[PM_FIRE_ELEMENTAL] || (ptr) == &mons[PM_SALAMANDER])
#define is_silent(ptr) ((ptr)->msound == MS_SILENT)
#define unsolid(ptr) (((ptr)->mflags1 & M1_UNSOLID) != 0L)
#define mindless(ptr) (((ptr)->mflags1 & M1_MINDLESS) != 0L)
#define humanoid(ptr) (((ptr)->mflags1 & M1_HUMANOID) != 0L)
#define is_animal(ptr) (((ptr)->mflags1 & M1_ANIMAL) != 0L)
#define slithy(ptr) (((ptr)->mflags1 & M1_SLITHY) != 0L)
#define is_wooden(ptr) ((ptr) == &mons[PM_WOOD_GOLEM])
#define thick_skinned(ptr) (((ptr)->mflags1 & M1_THICK_HIDE) != 0L)
#define hug_throttles(ptr) ((ptr) == &mons[PM_ROPE_GOLEM])
#define slimeproof(ptr) \
    ((ptr) == &mons[PM_GREEN_SLIME] || flaming(ptr) || noncorporeal(ptr))
#define lays_eggs(ptr) (((ptr)->mflags1 & M1_OVIPAROUS) != 0L)
#define eggs_in_water(ptr) \
    (lays_eggs(ptr) && (ptr)->mlet == S_EEL && is_swimmer(ptr))
#define regenerates(ptr) (((ptr)->mflags1 & M1_REGEN) != 0L)
#define perceives(ptr) (((ptr)->mflags1 & M1_SEE_INVIS) != 0L)
#define can_teleport(ptr) (((ptr)->mflags1 & M1_TPORT) != 0L)
#define control_teleport(ptr) (((ptr)->mflags1 & M1_TPORT_CNTRL) != 0L)
#define telepathic(ptr)                                                \
    ((ptr) == &mons[PM_FLOATING_EYE] || (ptr) == &mons[PM_MIND_FLAYER] \
     || (ptr) == &mons[PM_MASTER_MIND_FLAYER]                          \
     || (ptr) == &mons[PM_GOBLIN_SHAMAN]                               \
     || (ptr) == &mons[PM_KOBOLD_SHAMAN]                               \
     || (ptr) == &mons[PM_ORC_SHAMAN]                                  \
     || (ptr) == &mons[PM_HILL_GIANT_SHAMAN]                           \
     || (ptr) == &mons[PM_ELVEN_WIZARD]                                \
     || (ptr) == &mons[PM_GNOMISH_WIZARD])
#define has_claws(ptr) \
    ((ptr)->mlet == S_COCKATRICE || (ptr)->mlet == S_FELINE     \
     || (ptr)->mlet == S_GREMLIN || (ptr)->mlet == S_IMP        \
     || (ptr)->mlet == S_MIMIC || (ptr)->mlet == S_SPIDER       \
     || (ptr)->mlet == S_ZRUTY || (ptr)->mlet == S_BAT          \
     || (ptr)->mlet == S_DRAGON || (ptr)->mlet == S_JABBERWOCK  \
     || (ptr)->mlet == S_RUSTMONST || (ptr)->mlet == S_TROLL    \
     || (ptr)->mlet == S_UMBER || (ptr)->mlet == S_YETI         \
     || (ptr)->mlet == S_DEMON || (ptr)->mlet == S_LIZARD)
#define has_claws_undead(ptr) \
    ((ptr)->mlet == S_MUMMY || (ptr)->mlet == S_ZOMBIE          \
     || (ptr)->mlet == S_WRAITH || (ptr)->mlet == S_VAMPIRE)
#define is_armed(ptr) attacktype(ptr, AT_WEAP)
#define acidic(ptr) (((ptr)->mflags1 & M1_ACID) != 0L)
#define poisonous(ptr) (((ptr)->mflags1 & M1_POIS) != 0L)
#define carnivorous(ptr) (((ptr)->mflags1 & M1_CARNIVORE) != 0L)
#define herbivorous(ptr) (((ptr)->mflags1 & M1_HERBIVORE) != 0L)
#define metallivorous(ptr) (((ptr)->mflags1 & M1_METALLIVORE) != 0L)
#define polyok(ptr) (((ptr)->mflags2 & M2_NOPOLY) == 0L)
#define is_shapeshifter(ptr) (((ptr)->mflags2 & M2_SHAPESHIFTER) != 0L)
#define is_undead(ptr) (((ptr)->mhflags & MH_UNDEAD) != 0L)
#define is_were(ptr) (((ptr)->mhflags & MH_WERE) != 0L)
#define is_elf(ptr) (((ptr)->mhflags & MH_ELF) != 0L)
#define is_hobbit(ptr) (((ptr)->mhflags & MH_HOBBIT) != 0L)
#define is_dwarf(ptr) (((ptr)->mhflags & MH_DWARF) != 0L)
#define is_gnome(ptr) (((ptr)->mhflags & MH_GNOME) != 0L)
#define is_orc(ptr) (((ptr)->mhflags & MH_ORC) != 0L)
#define is_human(ptr) (((ptr)->mhflags & MH_HUMAN) != 0L)
#define your_race(ptr) (((ptr)->mhflags & urace.selfmask) != 0L)
#define is_bat(ptr)                                         \
    ((ptr) == &mons[PM_BAT] || (ptr) == &mons[PM_GIANT_BAT] \
     || (ptr) == &mons[PM_VAMPIRE_BAT])
#define is_bird(ptr) ((ptr)->mlet == S_BAT && !is_bat(ptr))
#define has_beak(ptr) (is_bird(ptr) || (ptr) == &mons[PM_TENGU] \
                       || (ptr) == &mons[PM_VROCK]              \
                       || (ptr) == &mons[PM_BABY_OWLBEAR]       \
                       || (ptr) == &mons[PM_OWLBEAR]            \
                       || (ptr) == &mons[PM_FELL_BEAST])
# define is_rat(ptr) ((ptr) == &mons[PM_SEWER_RAT]       \
                      || (ptr) == &mons[PM_GIANT_RAT]    \
                      || (ptr) == &mons[PM_RABID_RAT]    \
                      || (ptr) == &mons[PM_ENORMOUS_RAT] \
                      || (ptr) == &mons[PM_RODENT_OF_UNUSUAL_SIZE])
#define has_trunk(ptr) ((ptr) == &mons[PM_MUMAK]       \
                        || (ptr) == &mons[PM_MASTODON] \
                        || (ptr) == &mons[PM_WOOLLY_MAMMOTH])
#define is_giant(ptr) (((ptr)->mhflags & MH_GIANT) != 0L)
#define is_centaur(ptr) (((ptr)->mhflags & MH_CENTAUR) != 0L)
#define is_golem(ptr) ((ptr)->mlet == S_GOLEM)
#define is_ogre(ptr) (((ptr)->mhflags & MH_OGRE) != 0L)
#define is_troll(ptr) (((ptr)->mhflags & MH_TROLL) != 0L)
#define is_illithid(ptr) (((ptr)->mhflags & MH_ILLITHID) != 0L)
#define is_not_zombie(ptr) ((ptr) == &mons[PM_GHOUL] || (ptr) == &mons[PM_SKELETON])
#define is_zombie(ptr) ((ptr)->mlet == S_ZOMBIE && !is_not_zombie(ptr))
#define is_bones_monster(ptr) ((ptr) == &mons[PM_GHOST] || (ptr) == &mons[PM_GHOUL]            \
                               || (ptr) == &mons[PM_SPECTRE] || (ptr) == &mons[PM_VAMPIRE]     \
                               || (ptr) == &mons[PM_VAMPIRE_MAGE] || (ptr) == &mons[PM_WRAITH] \
                               || (ptr)->mlet == S_MUMMY || is_zombie(ptr)                     \
                               || (ptr) == &mons[PM_BARROW_WIGHT])
#define is_domestic(ptr) (((ptr)->mflags2 & M2_DOMESTIC) != 0L)
#define is_demon(ptr) (((ptr)->mhflags & MH_DEMON) != 0L)
#define is_dragon(ptr) (((ptr)->mhflags & MH_DRAGON) != 0L)
#define is_pseudodragon(ptr) ((ptr) == &mons[PM_PSEUDODRAGON] || (ptr) == &mons[PM_ELDER_PSEUDODRAGON] \
                              || (ptr) == &mons[PM_ANCIENT_PSEUDODRAGON])
#define is_angel(ptr) (((ptr)->mhflags & MH_ANGEL) != 0L)
#define is_mercenary(ptr) (((ptr)->mflags2 & M2_MERC) != 0L)
#define is_soldier(ptr) ((ptr) == &mons[PM_HUMAN_SOLDIER] || (ptr) == &mons[PM_ELVEN_SOLDIER]          \
                         || (ptr) == &mons[PM_DWARVISH_SOLDIER] || (ptr) == &mons[PM_GNOMISH_SOLDIER]  \
                         || (ptr) == &mons[PM_ORCISH_SOLDIER] || (ptr) == &mons[PM_CENTAURIAN_SOLDIER] \
                         || (ptr) == &mons[PM_GIANT_SOLDIER])
#define is_sergeant(ptr) ((ptr) == &mons[PM_HUMAN_SERGEANT] || (ptr) == &mons[PM_ELVEN_SERGEANT]          \
                          || (ptr) == &mons[PM_DWARVISH_SERGEANT] || (ptr) == &mons[PM_GNOMISH_SERGEANT]  \
                          || (ptr) == &mons[PM_ORCISH_SERGEANT] || (ptr) == &mons[PM_CENTAURIAN_SERGEANT] \
                          || (ptr) == &mons[PM_GIANT_SERGEANT])
#define is_lieutenant(ptr) ((ptr) == &mons[PM_HUMAN_LIEUTENANT] || (ptr) == &mons[PM_ELVEN_LIEUTENANT]          \
                            || (ptr) == &mons[PM_DWARVISH_LIEUTENANT] || (ptr) == &mons[PM_GNOMISH_LIEUTENANT]  \
                            || (ptr) == &mons[PM_ORCISH_LIEUTENANT] || (ptr) == &mons[PM_CENTAURIAN_LIEUTENANT] \
                            || (ptr) == &mons[PM_GIANT_LIEUTENANT])
#define is_captain(ptr) ((ptr) == &mons[PM_HUMAN_CAPTAIN] || (ptr) == &mons[PM_ELVEN_CAPTAIN]         \
                         || (ptr) == &mons[PM_DWARVISH_CAPTAIN] || (ptr) == &mons[PM_GNOMISH_CAPTAIN] \
                         || (ptr) == &mons[PM_GIANT_CAPTAIN])
#define is_rogue(ptr) ((ptr) == &mons[PM_ROGUE] || (ptr) == &mons[PM_HOBBIT_ROGUE] \
                       || (ptr) == &mons[PM_HUMAN_ROGUE])
#define is_male(ptr) (((ptr)->mflags2 & M2_MALE) != 0L)
#define is_female(ptr) (((ptr)->mflags2 & M2_FEMALE) != 0L)
#define is_neuter(ptr) (((ptr)->mflags2 & M2_NEUTER) != 0L)
#define is_wanderer(ptr) (((ptr)->mflags2 & M2_WANDER) != 0L)
#define always_hostile(ptr) (((ptr)->mflags2 & M2_HOSTILE) != 0L)
#define always_peaceful(ptr) (((ptr)->mflags2 & M2_PEACEFUL) != 0L)
#define race_hostile(ptr) (((ptr)->mhflags & urace.hatemask) != 0L)
#define race_peaceful(ptr) (((ptr)->mhflags & urace.lovemask) != 0L)
#define extra_nasty(ptr) (((ptr)->mflags2 & M2_NASTY) != 0L)
#define strongmonst(ptr) (((ptr)->mflags2 & M2_STRONG) != 0L)
#define can_breathe(ptr) attacktype(ptr, AT_BREA)
#define cantwield(ptr) (nohands(ptr) || verysmall(ptr))
#define could_twoweap(ptr) ((ptr)->mattk[1].aatyp == AT_WEAP)
#define cantweararm(ptr) (breakarm(ptr) || sliparm(ptr))
#define throws_rocks(ptr) (((ptr)->mflags2 & M2_ROCKTHROW) != 0L)
#define type_is_pname(ptr) (((ptr)->mflags2 & M2_PNAME) != 0L)
#define is_lord(ptr) (((ptr)->mflags2 & M2_LORD) != 0L)
#define is_prince(ptr) (((ptr)->mflags2 & M2_PRINCE) != 0L)
#define is_ndemon(ptr) \
    (is_demon(ptr) && (((ptr)->mflags2 & (M2_LORD | M2_PRINCE)) == 0L))
#define is_dlord(ptr) (is_demon(ptr) && is_lord(ptr))
#define is_dprince(ptr) (is_demon(ptr) && is_prince(ptr))
#define is_minion(ptr) (((ptr)->mflags2 & M2_MINION) != 0L)
#define likes_gold(ptr) (((ptr)->mflags2 & M2_GREEDY) != 0L)
#define likes_gems(ptr) (((ptr)->mflags2 & M2_JEWELS) != 0L)
#define likes_objs(ptr) (((ptr)->mflags2 & M2_COLLECT) != 0L || is_armed(ptr))
#define likes_magic(ptr) (((ptr)->mflags2 & M2_MAGIC) != 0L)
#define webmaker(ptr) \
    ((ptr) == &mons[PM_CAVE_SPIDER] || (ptr) == &mons[PM_GIANT_SPIDER])
#define is_unicorn(ptr) ((ptr)->mlet == S_UNICORN && likes_gems(ptr))
#define is_longworm(ptr)                                                   \
    (((ptr) == &mons[PM_BABY_LONG_WORM]) || ((ptr) == &mons[PM_LONG_WORM]) \
     || ((ptr) == &mons[PM_LONG_WORM_TAIL]))
#define is_covetous(ptr) (((ptr)->mflags3 & M3_COVETOUS))
#define is_skittish(ptr) (((ptr)->mflags3 & M3_SKITTISH))
#define is_accurate(ptr) (((ptr)->mflags3 & M3_ACCURATE))
#define is_berserker(ptr) (((ptr)->mflags3 & M3_BERSERK))
#define infravision(ptr) (((ptr)->mflags3 & M3_INFRAVISION))
#define infravisible(ptr) (((ptr)->mflags3 & M3_INFRAVISIBLE))
#define is_displacer(ptr) (((ptr)->mflags3 & M3_DISPLACES) != 0L)
#define is_displaced(ptr) ((ptr) == &mons[PM_SHIMMERING_DRAGON] || \
    (ptr) == &mons[PM_BABY_SHIMMERING_DRAGON])
#define is_actual_player(ptr) \
    (((ptr) >= &mons[PM_ARCHEOLOGIST]) && ((ptr) <= &mons[PM_WIZARD]))
#define is_mplayer(ptr) \
    (((ptr) >= &mons[PM_HUMAN_ARCHEOLOGIST]) && ((ptr) <= &mons[PM_HUMAN_WIZARD]))
#define is_watch(ptr) \
    ((ptr) == &mons[PM_WATCHMAN] || (ptr) == &mons[PM_WATCH_CAPTAIN])
#define is_rider(ptr)                                      \
    ((ptr) == &mons[PM_DEATH] || (ptr) == &mons[PM_FAMINE] \
     || (ptr) == &mons[PM_PESTILENCE])
#define is_placeholder(ptr)                                 \
    ((ptr) == &mons[PM_ORC] || (ptr) == &mons[PM_GIANT]     \
     || (ptr) == &mons[PM_ELF] || (ptr) == &mons[PM_HUMAN]  \
     || (ptr) == &mons[PM_CENTAUR] || (ptr) == &mons[PM_ILLITHID])
/* return TRUE if the monster tends to revive */
#define is_reviver(ptr) (is_rider(ptr) || (ptr)->mlet == S_TROLL \
                        || is_zombie(ptr))
/* monsters whose corpses and statues need special handling;
   note that high priests and the Wizard of Yendor are flagged
   as unique even though they really aren't; that's ok here */
#define unique_corpstat(ptr) (((ptr)->geno & G_UNIQ) != 0)

/* this returns the light's range, or 0 if none; if we add more light emitting
   monsters, we'll likely have to add a new light range field to mons[] */
#define emits_light(ptr)                                          \
    (((ptr)->mlet == S_LIGHT || (ptr) == &mons[PM_FLAMING_SPHERE] \
      || (ptr) == &mons[PM_SHOCKING_SPHERE]                       \
      || (ptr) == &mons[PM_GOLD_DRAGON]                           \
      || (ptr) == &mons[PM_BABY_GOLD_DRAGON]                      \
      || (ptr) == &mons[PM_FIRE_VORTEX])                          \
         ? 1                                                      \
         : ((ptr) == &mons[PM_FIRE_ELEMENTAL]) ? 1 : 0)
/*	[note: the light ranges above were reduced to 1 for performance...] */
#define likes_lava(ptr)                                             \
    (ptr == &mons[PM_FIRE_ELEMENTAL]                                \
     || ptr == &mons[PM_SALAMANDER] || ptr == &mons[PM_LAVA_DEMON])
#define pm_invisible(ptr) \
    ((ptr) == &mons[PM_STALKER] || (ptr) == &mons[PM_BLACK_LIGHT])

/* could probably add more */
#define likes_fire(ptr)                                                  \
    ((ptr) == &mons[PM_FIRE_VORTEX] || (ptr) == &mons[PM_FLAMING_SPHERE] \
     || likes_lava(ptr))

#define touch_petrifies(ptr) \
    ((ptr) == &mons[PM_COCKATRICE] || (ptr) == &mons[PM_CHICKATRICE] \
     || (ptr) == &mons[PM_BASILISK])

#define is_mind_flayer(ptr) \
    ((ptr) == &mons[PM_MIND_FLAYER] || (ptr) == &mons[PM_MASTER_MIND_FLAYER])

#define is_vampire(ptr) ((ptr)->mlet == S_VAMPIRE)

#define hates_light(ptr) ((ptr) == &mons[PM_GREMLIN])

/* used to vary a few messages */
#define weirdnonliving(ptr) (is_golem(ptr) || (ptr)->mlet == S_VORTEX)
#define nonliving(ptr) \
    (is_undead(ptr) || (ptr) == &mons[PM_MANES] || weirdnonliving(ptr))

/* no corpse (ie, blank scrolls) if killed by fire */
#define completelyburns(ptr) \
    ((ptr) == &mons[PM_PAPER_GOLEM] || (ptr) == &mons[PM_STRAW_GOLEM])

/* Used for conduct with corpses, tins, and digestion attacks */
/* G_NOCORPSE monsters might still be swallowed as a purple worm */
/* Maybe someday this could be in mflags... */
#define vegan(ptr)                                                 \
    ((ptr)->mlet == S_BLOB || (ptr)->mlet == S_JELLY               \
     || (ptr)->mlet == S_FUNGUS || (ptr)->mlet == S_VORTEX         \
     || (ptr)->mlet == S_LIGHT                                     \
     || ((ptr)->mlet == S_ELEMENTAL && (ptr) != &mons[PM_STALKER]) \
     || ((ptr)->mlet == S_GOLEM && (ptr) != &mons[PM_FLESH_GOLEM]  \
         && (ptr) != &mons[PM_LEATHER_GOLEM]) || noncorporeal(ptr))
#define vegetarian(ptr) \
    (vegan(ptr)         \
     || ((ptr)->mlet == S_PUDDING && (ptr) != &mons[PM_BLACK_PUDDING]))

/* monkeys are tameable via bananas but not pacifiable via food,
   otherwise their theft attack could be nullified too easily;
   dogs and cats can be tamed by anything they like to eat and are
   pacified by any other food;
   horses can be tamed by always-veggy food or lichen corpses but
   not tamed or pacified by other corpses or tins of veggy critters */
#define befriend_with_obj(ptr, obj) \
    (((ptr) == &mons[PM_MONKEY] || (ptr) == &mons[PM_APE])               \
     ? (obj)->otyp == BANANA                                             \
     : ((is_domestic(ptr) || (is_rat(ptr) && Role_if(PM_CONVICT)))       \
        && (obj)->oclass == FOOD_CLASS                                   \
        && ((ptr)->mlet != S_UNICORN                                     \
            || obj->material == VEGGY                                    \
            || ((obj)->otyp == CORPSE && (obj)->corpsenm == PM_LICHEN))))

/* Noise that a monster makes when engaged in combat. Assume that vocalizations
 * account for some noise, so monsters capable of vocalizing make more.
 * This gets used as an argument to wake_nearto, which expects a squared value,
 * so we square the result. */
#define combat_noise(ptr) \
    ((ptr)->msound ? ((ptr)->msize * 2 + 1) * ((ptr)->msize * 2 + 1) \
                   : ((ptr)->msize + 1) * ((ptr)->msize + 1))

#endif /* MONDATA_H */
