/* NetHack 3.6	makemon.c	$NHDT-Date: 1571531888 2019/10/20 00:38:08 $  $NHDT-Branch: NetHack-3.6 $:$NHDT-Revision: 1.141 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2012. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include <ctype.h>

/* this assumes that a human quest leader or nemesis is an archetype
   of the corresponding role; that isn't so for some roles (tourist
   for instance) but is for the priests and monks we use it for... */
#define quest_mon_represents_role(mptr, role_pm) \
    (mptr->mlet == S_HUMAN && Role_if(role_pm)   \
     && (mptr->msound == MS_LEADER || mptr->msound == MS_NEMESIS))

STATIC_DCL boolean FDECL(uncommon, (int));
STATIC_DCL int FDECL(align_shift, (struct permonst *));
STATIC_DCL boolean FDECL(mk_gen_ok, (int, int, int));
STATIC_DCL boolean FDECL(wrong_elem_type, (struct permonst *));
STATIC_DCL void FDECL(m_initgrp, (struct monst *, int, int, int, int));
STATIC_DCL void FDECL(m_initthrow, (struct monst *, int, int));
STATIC_DCL void FDECL(m_initweap, (struct monst *));
STATIC_DCL void FDECL(m_initinv, (struct monst *));
STATIC_DCL boolean FDECL(makemon_rnd_goodpos, (struct monst *,
                                               unsigned, coord *));

#define m_initsgrp(mtmp, x, y, mmf) m_initgrp(mtmp, x, y, 3, mmf)
#define m_initlgrp(mtmp, x, y, mmf) m_initgrp(mtmp, x, y, 10, mmf)
#define toostrong(monindx, lev) (mons[monindx].difficulty > lev)
#define tooweak(monindx, lev) (mons[monindx].difficulty < lev)

boolean
is_home_elemental(ptr)
struct permonst *ptr;
{
    if (ptr->mlet == S_ELEMENTAL)
        switch (monsndx(ptr)) {
        case PM_AIR_ELEMENTAL:
            return Is_airlevel(&u.uz);
        case PM_FIRE_ELEMENTAL:
            return Is_firelevel(&u.uz);
        case PM_EARTH_ELEMENTAL:
            return Is_earthlevel(&u.uz);
        case PM_WATER_ELEMENTAL:
            return Is_waterlevel(&u.uz);
        }
    return FALSE;
}

/*
 * Return true if the given monster cannot exist on this elemental level.
 */
STATIC_OVL boolean
wrong_elem_type(ptr)
struct permonst *ptr;
{
    if (ptr->mlet == S_ELEMENTAL) {
        return (boolean) !is_home_elemental(ptr);
    } else if (Is_earthlevel(&u.uz)) {
        /* no restrictions? */
    } else if (Is_waterlevel(&u.uz)) {
        /* just monsters that can swim */
        if (!is_swimmer(ptr))
            return TRUE;
    } else if (Is_firelevel(&u.uz)) {
        if (!pm_resistance(ptr, MR_FIRE))
            return TRUE;
    } else if (Is_airlevel(&u.uz)) {
        if (!(is_flyer(ptr) && ptr->mlet != S_TRAPPER) && !is_floater(ptr)
            && !amorphous(ptr) && !noncorporeal(ptr) && !is_whirly(ptr))
            return TRUE;
    }
    return FALSE;
}

/* make a group just like mtmp */
STATIC_OVL void
m_initgrp(mtmp, x, y, n, mmflags)
struct monst *mtmp;
int x, y, n, mmflags;
{
    coord mm;
    register int cnt = rnd(n);
    struct monst *mon;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
    /* There is an unresolved problem with several people finding that
     * the game hangs eating CPU; if interrupted and restored, the level
     * will be filled with monsters.  Of those reports giving system type,
     * there were two DG/UX and two HP-UX, all using gcc as the compiler.
     * hcroft@hpopb1.cern.ch, using gcc 2.6.3 on HP-UX, says that the
     * problem went away for him and another reporter-to-newsgroup
     * after adding this debugging code.  This has almost got to be a
     * compiler bug, but until somebody tracks it down and gets it fixed,
     * might as well go with the "but it went away when I tried to find
     * it" code.
     */
    int cnttmp, cntdiv;

    cnttmp = cnt;
    debugpline4("init group call <%d,%d>, n=%d, cnt=%d.", x, y, n, cnt);
    cntdiv = ((u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1);
#endif
    /* Tuning: cut down on swarming at low character levels [mrs] */
    cnt /= (u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
    if (cnt != (cnttmp / cntdiv)) {
        pline("cnt=%d using %d, cnttmp=%d, cntdiv=%d", cnt,
              (u.ulevel < 3) ? 4 : (u.ulevel < 5) ? 2 : 1, cnttmp, cntdiv);
    }
#endif
    if (!cnt)
        cnt++;
#if defined(__GNUC__) && (defined(HPUX) || defined(DGUX))
    if (cnt < 0)
        cnt = 1;
    if (cnt > 10)
        cnt = 10;
#endif

    mm.x = x;
    mm.y = y;
    while (cnt--) {
        if (peace_minded(mtmp->data))
            continue;
        /* Don't create groups of peaceful monsters since they'll get
         * in our way.  If the monster has a percentage chance so some
         * are peaceful and some are not, the result will just be a
         * smaller group.
         */
        if (enexto(&mm, mm.x, mm.y, mtmp->data)) {
            mon = makemon(mtmp->data, mm.x, mm.y, (mmflags | MM_NOGRP));
            if (mon) {
                mon->mpeaceful = FALSE;
                mon->mavenge = 0;
                set_malign(mon);
                /* Undo the second peace_minded() check in makemon(); if the
                 * monster turned out to be peaceful the first time we
                 * didn't create it at all; we don't want a second check.
                 */
            }
        }
    }
}

STATIC_OVL
void
m_initthrow(mtmp, otyp, oquan)
struct monst *mtmp;
int otyp, oquan;
{
    register struct obj *otmp;

    otmp = mksobj(otyp, TRUE, FALSE);
    otmp->quan = (long) rn1(oquan, 3);
    otmp->owt = weight(otmp);
    if (otyp == ORCISH_ARROW)
        otmp->opoisoned = TRUE;
    (void) mpickobj(mtmp, otmp);
}

struct trobj {
    short trotyp;
    schar trspe;
    char trclass;
    Bitfield(trquan, 6);
    Bitfield(trbless, 2);
};

#define UNDEF_TYP	0
#define UNDEF_SPE	'\177'
#define RND_SPE         (0x80)
#define UNDEF_BLESS	2

extern struct trobj Archeologist[];
extern struct trobj Barbarian[];
extern struct trobj Cave_man[];
extern struct trobj Convict[];
extern struct trobj Healer[];
extern struct trobj Knight[];
extern struct trobj Monk[];
extern struct trobj Priest[];
extern struct trobj Ranger[];
extern struct trobj Rogue[];
extern struct trobj Samurai[];
extern struct trobj Tourist[];
extern struct trobj Valkyrie[];
extern struct trobj Wizard[];
extern struct trobj Tinopener[];
extern struct trobj Lamp[];
extern struct trobj Blindfold[];
extern struct trobj Xtra_food[];
extern struct trobj Leash[];
extern struct trobj Towel[];
extern struct trobj Wishing[];
extern struct trobj Money[];
extern struct trobj Pickaxe[];

struct trobj Level20Kit1[] = {
	{ SILVER_DRAGON_SCALE_MAIL, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ GAUNTLETS_OF_POWER, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ CLOAK_OF_MAGIC_RESISTANCE, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ SPEED_BOOTS, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ HELM_OF_TELEPATHY, (4 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ AMULET_OF_LIFE_SAVING, 0, AMULET_CLASS, 1, UNDEF_BLESS },
	{ BAG_OF_HOLDING, 0, TOOL_CLASS, 1, 1 },
	{ 0, 0, 0, 0, 0 }
};

struct trobj Level20Kit2[] = {
	{ GRAY_DRAGON_SCALE_MAIL, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ GAUNTLETS_OF_DEXTERITY, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ JUMPING_BOOTS, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ ROBE, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
	{ HELM_OF_BRILLIANCE, (4 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ AMULET_OF_REFLECTION, 0, AMULET_CLASS, 1, UNDEF_BLESS },
	{ BAG_OF_HOLDING, 0, TOOL_CLASS, 1, 1 },
	{ 0, 0, 0, 0, 0 }
};

struct trobj Level20Kit3[] = {
        { GOLD_DRAGON_SCALE_MAIL, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { GLOVES, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { WATER_WALKING_BOOTS, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { CLOAK_OF_PROTECTION, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { HELM_OF_SPEED, (4 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { AMULET_OF_MAGIC_RESISTANCE, 0, AMULET_CLASS, 1, UNDEF_BLESS },
        { BAG_OF_HOLDING, 0, TOOL_CLASS, 1, 1 },
        { 0, 0, 0, 0, 0 }
};

struct trobj Level20Kit4[] = {
        { BLACK_DRAGON_SCALE_MAIL, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { GAUNTLETS, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { WATER_WALKING_BOOTS, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { CLOAK_OF_DISPLACEMENT, (4 | RND_SPE), ARMOR_CLASS, 1, 1 },
        { HELM_OF_BRILLIANCE, (4 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { AMULET_OF_FLYING, 0, AMULET_CLASS, 1, UNDEF_BLESS },
        { BAG_OF_HOLDING, 0, TOOL_CLASS, 1, 1 },
        { 0, 0, 0, 0, 0 }
};

struct trobj Level10Kit1[] = {
	{ ARMOR, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ HELMET, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ HIGH_BOOTS, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ GAUNTLETS, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
	{ SACK, 0, TOOL_CLASS, 1, UNDEF_BLESS },
	{ 0, 0, 0, 0, 0 }
};

struct trobj Level10Kit2[] = {
        { STUDDED_ARMOR, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { ELVEN_HELM, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { LOW_BOOTS, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { GLOVES, (2 | RND_SPE), ARMOR_CLASS, 1, UNDEF_BLESS },
        { OILSKIN_SACK, 0, TOOL_CLASS, 1, UNDEF_BLESS },
        { 0, 0, 0, 0, 0 }
};

#define B_MAJOR	        0 /* two-handed sword or battle-axe  */
#define B_MINOR	        1 /* matched with axe or short sword */
#define C_AMMO          2
#define M_BOOK          2
#define RAN_BOW	        1
#define RAN_TWO_ARROWS	2
#define RAN_ZERO_ARROWS	3
#define R_DAGGERS	1
#define S_ARROWS	3
#define T_DARTS		0
#define W_MULTSTART	2
#define W_MULTEND	6

struct inv_sub { short race_pm, item_otyp, subs_otyp; };
extern struct inv_sub inv_subs[];

extern int FDECL(isqrt, (int));

STATIC_OVL void
ini_mon_inv(mtmp, origtrop, chance)
register struct monst *mtmp;
register struct trobj *origtrop;
unsigned short chance;
{
    struct obj *obj;
    struct trobj temptrop;
    register struct trobj *trop = &temptrop;
    int otyp;

    memcpy(&temptrop, origtrop, sizeof(struct trobj));

    while (origtrop->trclass) {
	   if ((chance <= 0) || !rn2(chance)) {
	       if (trop->trotyp != UNDEF_TYP) {
		   otyp = (int)trop->trotyp;
		   obj = mksobj(otyp, TRUE, FALSE);
	       } else {	/* UNDEF_TYP */
	           static NEARDATA short nocreate = STRANGE_OBJECT;
		   static NEARDATA short nocreate2 = STRANGE_OBJECT;
		   static NEARDATA short nocreate3 = STRANGE_OBJECT;
		   static NEARDATA short nocreate4 = STRANGE_OBJECT;
	           /*
		    * For random objects, do not create certain overly powerful
		    * items: wand of wishing, ring of levitation, or the
		    * polymorph/polymorph control combination.  Specific objects,
		    * i.e. the discovery wishing, are still OK.
		    * Also, don't get a couple of really useless items.  (Note:
		    * punishment isn't "useless".  Some players who start out with
		    * one will immediately read it and use the iron ball as a
		    * weapon.)
		    */
		   obj = mkobj(trop->trclass, FALSE);
			       otyp = obj->otyp;
		   while (otyp == WAN_WISHING
			  || otyp == nocreate
			  || otyp == nocreate2
			  || otyp == nocreate3
			  || otyp == nocreate4
			  || otyp == RIN_LEVITATION
			  /* 'useless' items */
			  || otyp == POT_HALLUCINATION
			  || otyp == POT_ACID
			  || otyp == SCR_AMNESIA
			  || otyp == SCR_FIRE
			  || otyp == SCR_BLANK_PAPER
			  || otyp == SPE_BLANK_PAPER
			  || otyp == RIN_AGGRAVATE_MONSTER
			  || otyp == RIN_HUNGER
			  || otyp == WAN_NOTHING
			  || (otyp == SCR_ENCHANT_WEAPON
			      && mtmp->mnum == PM_HUMAN_MONK)
			  || (otyp == SPE_FORCE_BOLT
			      && mtmp->mnum == PM_HUMAN_WIZARD)
			  || (obj->oclass == SPBOOK_CLASS
			      && objects[otyp].oc_level > 3)) {
				 dealloc_obj(obj);
				 obj = mkobj(trop->trclass, FALSE);
				 otyp = obj->otyp;
		   }

		   if (objects[otyp].oc_charged && obj->spe <= 0)
		       obj->spe = rne(3);

                   /* Heavily relies on the fact that 1) we create wands
                    * before rings, 2) that we create rings before
                    * spellbooks, and that 3) not more than 1 object of a
                    * particular symbol is to be prohibited.  (For more
                    * objects, we need more nocreate variables...)
                    */
                   switch (otyp) {
	               case WAN_POLYMORPH:
		       case RIN_POLYMORPH:
	               case POT_POLYMORPH:
		           nocreate = RIN_POLYMORPH_CONTROL;
		           break;
		       case RIN_POLYMORPH_CONTROL:
		           nocreate = RIN_POLYMORPH;
		           nocreate2 = SPE_POLYMORPH;
		           nocreate3 = POT_POLYMORPH;
                   }
                   /* Don't have 2 of the same ring or spellbook */
                   if (obj->oclass == RING_CLASS
                       || obj->oclass == SPBOOK_CLASS)
                       nocreate4 = otyp;
                   }

		   if (trop->trclass == COIN_CLASS) {
		       /* no "blessed" or "identified" money */
		       obj->quan = u.umoney0;
		   } else {
		       obj->cursed = 0;
		   if (obj->opoisoned && u.ualign.type != A_CHAOTIC)
		       obj->opoisoned = 0;
		   if (obj->oclass == WEAPON_CLASS
		       || obj->oclass == TOOL_CLASS) {
		       obj->quan = (long) trop->trquan;
		       trop->trquan = 1;
		   } else if (obj->oclass == GEM_CLASS &&
			      is_graystone(obj) && obj->otyp != FLINT) {
			      obj->quan = 1L;
		   }
		   if (trop->trspe != UNDEF_SPE) {
		       obj->spe = (trop->trspe & ~RND_SPE);
		       if (trop->trspe & RND_SPE) {
		           obj->spe--;
			   obj->spe += rn2(3);
		       }
		   }
		   if (trop->trbless != UNDEF_BLESS)
		       obj->blessed = trop->trbless;
		   if ((obj->oclass == WEAPON_CLASS
	               || obj->oclass == ARMOR_CLASS)
		       && chance > 1) {
		       obj->spe = (obj->spe * rn2(chance)) / chance;
		       }
                   }
		   /* defined after setting otyp+quan + blessedness */
		   obj->owt = weight(obj);
		   (void) mpickobj(mtmp, obj);
	       }

#if !defined(PYRAMID_BUG) && !defined(MAC)
        if (--trop->trquan)
            continue; /* make a similar object */
#else
        if (trop->trquan) { /* check if zero first */
            --trop->trquan;
            if (trop->trquan)
                continue; /* make a similar object */
        }
#endif
    origtrop++;
    memcpy(&temptrop, origtrop, sizeof(struct trobj));
    }
}

STATIC_OVL void
m_initweap(mtmp)
register struct monst *mtmp;
{
    register struct permonst *ptr = mtmp->data;
    register int mm = monsndx(ptr);
    struct obj *otmp = mtmp->minvent;
    int bias, w1, w2, randwand;

    if (Is_rogue_level(&u.uz))
        return;

    /* treat mplayers differently */
    if (is_mplayer(mtmp->data) && !In_endgame(&u.uz)) {
	if (mtmp->m_lev > 1) {
	    if (mtmp->m_lev > 10 || !rn2(10)) {
                if (rn2(2)) {
		    ini_mon_inv(mtmp, !rn2(2) ? Level20Kit1 : Level20Kit2,
                                (mtmp->m_lev >= 20) ? 1 : isqrt(23 - mtmp->m_lev));
                } else {
                    ini_mon_inv(mtmp, !rn2(2) ? Level20Kit3 : Level20Kit4,
                                (mtmp->m_lev >= 20) ? 1 : isqrt(23 - mtmp->m_lev));
                }
            }
	    ini_mon_inv(mtmp, !rn2(2) ? Level10Kit1 : Level10Kit2,
	                (mtmp->m_lev >= 10) ? 1 : isqrt(13 - mtmp->m_lev));
        }

    switch (mtmp->mnum) {
        case PM_HUMAN_ARCHEOLOGIST:
	    ini_mon_inv(mtmp, Archeologist, 1);
	    if (!rn2(10)) ini_mon_inv(mtmp, Tinopener);
	    else if (!rn2(4)) ini_mon_inv(mtmp, Lamp);
            else if (rn2(3)) ini_mon_inv(mtmp, Pickaxe);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_BARBARIAN:
	    if (rn2(100) >= 50) {
		Barbarian[B_MAJOR].trotyp = BATTLE_AXE;
		Barbarian[B_MINOR].trotyp = SHORT_SWORD;
	    }
	    else {
		Barbarian[B_MAJOR].trotyp = TWO_HANDED_SWORD;
		Barbarian[B_MINOR].trotyp = AXE;
	    }
	    ini_mon_inv(mtmp, Barbarian, 1);
	    ini_mon_inv(mtmp, Lamp, 6);
            mongets(mtmp, SKELETON_KEY);
	    break;
        case PM_HUMAN_CAVEMAN:
	case PM_HUMAN_CAVEWOMAN:
	    Cave_man[C_AMMO].trquan = rn1(11, 10);	/* 10..20 */
	    ini_mon_inv(mtmp, Cave_man, 1);
	    break;
        case PM_HUMAN_CONVICT:
            ini_mon_inv(mtmp, Convict, 1);
            mongets(mtmp, SKELETON_KEY);
            mongets(mtmp, GRAPPLING_HOOK);
            break;
	case PM_HUMAN_HEALER:
	    mkmonmoney(mtmp, (long) rn1(1000, 1001));
	    ini_mon_inv(mtmp, Healer, 1);
	    ini_mon_inv(mtmp, Lamp, 25);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_KNIGHT:
	    ini_mon_inv(mtmp, Knight, 1);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_MONK:
	    switch (rn2(90) / 30) {
		case 0: Monk[M_BOOK].trotyp = SPE_HEALING;
                    break;
		case 1: Monk[M_BOOK].trotyp = SPE_PROTECTION;
                    break;
		case 2: Monk[M_BOOK].trotyp = SPE_SLEEP;
                    break;
	    }
	    ini_mon_inv(mtmp, Monk, 1);
	    ini_mon_inv(mtmp, Lamp, 10);
            mongets(mtmp, SKELETON_KEY);
	    break;
        case PM_HUMAN_PRIEST:
	case PM_HUMAN_PRIESTESS:
            mkmonmoney(mtmp, (long) rn1(10, 20));
	    ini_mon_inv(mtmp, Priest, 1);
	    ini_mon_inv(mtmp, Lamp, 10);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_RANGER:
	    Ranger[RAN_TWO_ARROWS].trquan = rn1(10, 50);
	    Ranger[RAN_ZERO_ARROWS].trquan = rn1(10, 30);
	    ini_mon_inv(mtmp, Ranger, 1);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_ROGUE:
            mkmonmoney(mtmp, (long) rn1(1000, 500));
	    Rogue[R_DAGGERS].trquan = rn1(10, 6);
	    ini_mon_inv(mtmp, Rogue, 1);
	    ini_mon_inv(mtmp, Blindfold, 5);
	    break;
	case PM_HUMAN_SAMURAI:
	    Samurai[S_ARROWS].trquan = rn1(20, 26);
	    ini_mon_inv(mtmp, Samurai, 1);
	    ini_mon_inv(mtmp, Blindfold, 5);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_TOURIST:
	    Tourist[T_DARTS].trquan = rn1(20, 21);
	    mkmonmoney(mtmp, (long) rn1(1000, 1001));
	    ini_mon_inv(mtmp, Tourist, 1);
	    if (!rn2(25)) ini_mon_inv(mtmp, Tinopener, 1);
	    else if (!rn2(25)) ini_mon_inv(mtmp, Leash, 1);
	    else if (!rn2(25)) ini_mon_inv(mtmp, Towel, 1);
	    break;
	case PM_HUMAN_VALKYRIE:
	    ini_mon_inv(mtmp, Valkyrie, 1);
	    ini_mon_inv(mtmp, Lamp, 6);
            mongets(mtmp, SKELETON_KEY);
	    break;
	case PM_HUMAN_WIZARD:
	    ini_mon_inv(mtmp, Wizard, 1);
	    ini_mon_inv(mtmp, Blindfold, 5);
            mongets(mtmp, SKELETON_KEY);
	    break;
	default: /* impossible */
	    break;
    }

    {
        struct obj *bag = (struct obj *) 0;
	if (mtmp->m_lev > 1) {
	    for (; otmp; otmp = otmp->nobj) {
	        if (otmp->oclass == WEAPON_CLASS) {
		    if (mtmp->m_lev >= 20 || rn2(400) < mtmp->m_lev * mtmp->m_lev) {
		        if (!rn2(100 + 10 * nartifact_exist())) {
			    mk_artifact(otmp, sgn(mtmp->data->maligntyp));
                        } else if (!rn2(40)) {
                            create_oprop(otmp, FALSE);
			}
		    }
		}
	    }
	}

        for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
	    if (Is_container(otmp) && otmp->otyp != BAG_OF_TRICKS)
	        bag = otmp;
	    if (otmp->otyp == BAG_OF_HOLDING)
		break;
	}
	if (bag) {
	    int count = (mtmp->m_lev * mtmp->m_lev) / 25;
	    if (count < 1)
                count = 1;
	    if (count > 10)
                count = 10;
	    count += rn2((mtmp->m_lev / 10) + 2);
	    while (count-- > 0) {
	        int otyp =
		        (rn2(2) ? rnd_offensive_item(mtmp) :
			 rn2(2) ? rnd_defensive_item(mtmp) : rnd_misc_item(mtmp));
		otmp = mksobj(otyp, FALSE, FALSE);
	    if (otmp->oclass == WAND_CLASS) {
		int lim = (otmp->otyp == WAN_WISHING) ? 3
			   : (objects[otmp->otyp].oc_dir != NODIR) ? 8 : 15;
			   otmp->spe = rn2(lim);
	    }
	    if (otmp->otyp == WAN_CANCELLATION || otmp->otyp == BAG_OF_TRICKS)
		(void) mpickobj(mtmp, otmp);
	    else
		(void) add_to_container(bag, otmp);
	    }
        }
    }
    return;
}

    /*
     *  First a few special cases:
     *          giants get a boulder to throw sometimes
     *          ettins get clubs
     *          kobolds get darts to throw
     *          centaurs get some sort of bow & arrows or bolts
     *          soldiers get all sorts of things
     *          kops get clubs & cream pies.
     */
    switch (ptr->mlet) {
    case S_GIANT:
        if (rn2(2))
            (void) mongets(mtmp, (mm != PM_ETTIN) ? BOULDER : CLUB);
        break;
    case S_HUMAN:
        if (is_mercenary(ptr)) {
            w1 = w2 = 0;
            switch (mm) {
            case PM_WATCHMAN:
            case PM_HUMAN_SOLDIER:
            case PM_GNOMISH_SOLDIER:
            case PM_CENTAURIAN_SOLDIER:
            case PM_GIANT_SOLDIER:
                if (!rn2(3)) {
                    w1 = rn1(BEC_DE_CORBIN - PARTISAN + 1, PARTISAN);
                    w2 = rn2(2) ? DAGGER : KNIFE;
                } else
                    w1 = rn2(2) ? SPEAR : SHORT_SWORD;
                break;
            case PM_ELVEN_SOLDIER:
                if (!rn2(3)) {
                    mongets(mtmp, ELVEN_BOW);
                    m_initthrow(mtmp, ELVEN_ARROW, 12);
                    mongets(mtmp, ELVEN_DAGGER);
                } else
                    w1 = rn2(2) ? ELVEN_SPEAR : ELVEN_SHORT_SWORD;
                break;
            case PM_DWARVISH_SOLDIER:
                if (!rn2(3)) {
                    w1 = rn1(BEC_DE_CORBIN - PARTISAN + 1, PARTISAN);
                    w2 = rn2(2) ? DAGGER : AXE;
                } else
                    w1 = rn2(2) ? DWARVISH_SPEAR : DWARVISH_SHORT_SWORD;
                break;
            case PM_ORCISH_SOLDIER:
                if (!rn2(3)) {
                    mongets(mtmp, ORCISH_BOW);
                    m_initthrow(mtmp, ORCISH_ARROW, 12);
                    w1 = rn2(2) ? ORCISH_DAGGER : KNIFE;
                } else
                    w1 = rn2(2) ? ORCISH_SPEAR
                                : rn2(2) ? ORCISH_SHORT_SWORD
                                         : ORCISH_SCIMITAR;
                break;
            case PM_HUMAN_SERGEANT:
            case PM_GNOMISH_SERGEANT:
                w1 = rn2(2) ? FLAIL : MACE;
	        if (Is_stronghold(&u.uz)) {
		    w2 = BOW;
		    m_initthrow(mtmp, ARROW, 30);
                }
                break;
            case PM_ELVEN_SERGEANT:
                w1 = rn2(2) ? ELVEN_BROADSWORD : QUARTERSTAFF;
                break;
            case PM_DWARVISH_SERGEANT:
                w1 = rn2(2) ? MORNING_STAR : DWARVISH_MATTOCK;
                if (Is_stronghold(&u.uz)) {
                    w2 = BOW;
                    m_initthrow(mtmp, ARROW, 30);
                }
                break;
            case PM_ORCISH_SERGEANT:
                w1 = rn2(2) ? ORCISH_MORNING_STAR : ORCISH_SHORT_SWORD;
                if (Is_stronghold(&u.uz)) {
                    w2 = ORCISH_BOW;
                    m_initthrow(mtmp, ORCISH_ARROW, 30);
                }
                break;
            case PM_CENTAURIAN_SERGEANT:
                mongets(mtmp, CROSSBOW);
                m_initthrow(mtmp, CROSSBOW_BOLT, 12);
                w1 = rn2(2) ? CLUB : MACE;
                break;
            case PM_GIANT_SERGEANT:
                w1 = rn2(2) ? WAR_HAMMER : BATTLE_AXE;
                mongets(mtmp, BOULDER);
                break;
            case PM_HUMAN_LIEUTENANT:
            case PM_DWARVISH_LIEUTENANT:
            case PM_GNOMISH_LIEUTENANT:
            case PM_CENTAURIAN_LIEUTENANT:
                w1 = rn2(2) ? BROADSWORD : LONG_SWORD;
                if (Is_stronghold(&u.uz)) {
                    w2 = BOW;
                    m_initthrow(mtmp, ARROW, 30);
	        do {
		    randwand = rn2(7);
		} while (randwand > 5 && rn2(14));
		    switch (randwand) {
		        case 1:
			    randwand = WAN_MAGIC_MISSILE;
			    break;
			case 2:
			    randwand = WAN_SLEEP;
			    break;
			case 3:
			    randwand = WAN_FIRE;
			    break;
			case 4:
			    randwand = WAN_COLD;
			    break;
			case 5:
			    randwand = WAN_LIGHTNING;
			    break;
			case 6:
			    randwand = WAN_DEATH;
			    break;
			case 0:
			default:
			    randwand = WAN_STRIKING;
			    break;
		    }
		    (void) mongets(mtmp, randwand);
		}
                break;
            case PM_ELVEN_LIEUTENANT:
                w1 = rn2(2) ? ELVEN_BROADSWORD : ELVEN_LONG_SWORD;
                if (Is_stronghold(&u.uz)) {
                    w2 = ELVEN_BOW;
                    m_initthrow(mtmp, ELVEN_ARROW, 30);
                do {
                    randwand = rn2(7);
                } while (randwand > 5 && rn2(14));
                    switch (randwand) {
                        case 1:
                            randwand = WAN_MAGIC_MISSILE;
                            break;
                        case 2:
                            randwand = WAN_SLEEP;
                            break;
                        case 3:
                            randwand = WAN_FIRE;
                            break;
                        case 4:
                            randwand = WAN_COLD;
                            break;
                        case 5:
                            randwand = WAN_LIGHTNING;
                            break;
                        case 6:
                            randwand = WAN_DEATH;
                            break;
                        case 0:
                        default:
                            randwand = WAN_STRIKING;
                            break;
                    }
                    (void) mongets(mtmp, randwand);
                }
                break;
            case PM_ORCISH_LIEUTENANT:
                w1 = rn2(2) ? BROADSWORD : ORCISH_SCIMITAR;
                break;
            case PM_GIANT_LIEUTENANT:
                w1 = rn2(2) ? LONG_SWORD : BATTLE_AXE;
                mongets(mtmp, BOULDER);
                break;
            case PM_HUMAN_CAPTAIN:
            case PM_DWARVISH_CAPTAIN:
            case PM_GNOMISH_CAPTAIN:
                w1 = rn2(2) ? LONG_SWORD : SABER;
                mongets(mtmp, SKELETON_KEY);
                break;
            case PM_ELVEN_CAPTAIN:
                mongets(mtmp, ELVEN_LONG_SWORD);
                mongets(mtmp, SKELETON_KEY);
                break;
            case PM_GIANT_CAPTAIN:
                w1 = rn2(2) ? TWO_HANDED_SWORD : BATTLE_AXE;
                mongets(mtmp, SKELETON_KEY);
                break;
            case PM_PRISON_GUARD:
            case PM_WATCH_CAPTAIN:
                w1 = rn2(2) ? LONG_SWORD : SABER;
                mongets(mtmp, SKELETON_KEY);
                break;
            default:
                if (!rn2(4))
                    w1 = DAGGER;
                if (!rn2(7))
                    w2 = SPEAR;
                break;
            }
            if (w1)
                (void) mongets(mtmp, w1);
            if (!w2 && w1 != DAGGER && !rn2(4))
                w2 = KNIFE;
            if (w2)
                (void) mongets(mtmp, w2);
        } else if (is_elf(ptr)) {
            if (rn2(2))
                (void) mongets(mtmp,
		 (rn2(2) && (mm == PM_GREY_ELF || mm == PM_ELF_LORD
                             || mm == PM_ELF_LADY || mm == PM_ELVEN_WIZARD
                             || mm == PM_ELVEN_KING || mm == PM_ELVEN_QUEEN))
			     ? ELVEN_HELM : ELVEN_CLOAK);
            if (rn2(2) && mm != PM_ELVEN_WIZARD) {
                struct obj* mail = m_carrying(mtmp, ELVEN_CHAIN_MAIL);
                if (mail)
                    set_material(mail, MITHRIL);
            }
            else if (!rn2(4))
                (void) mongets(mtmp, ELVEN_BOOTS);
            if (rn2(2))
                (void) mongets(mtmp, ELVEN_DAGGER);
            switch (rn2(3)) {
            case 0:
                if (mm != PM_ELVEN_WIZARD) {
                    if (!rn2(4))
                        (void) mongets(mtmp, ELVEN_SHIELD);
                    if (rn2(3))
                        (void) mongets(mtmp, ELVEN_SHORT_SWORD);
                    (void) mongets(mtmp, ELVEN_BOW);
                    m_initthrow(mtmp, ELVEN_ARROW, 12);
                }
                break;
            case 1:
                if (mm != PM_ELVEN_WIZARD) {
                    (void) mongets(mtmp, rn2(2) ? ELVEN_LONG_SWORD : ELVEN_BROADSWORD);
                    if (rn2(2))
                        (void) mongets(mtmp, ELVEN_SHIELD);
                }
                break;
            case 2:
                if (mm != PM_ELVEN_WIZARD) {
                    if (rn2(2)) {
                        (void) mongets(mtmp, ELVEN_SPEAR);
                        (void) mongets(mtmp, ELVEN_SHIELD);
                    }
                }
                break;
            }
            if (mm == PM_ELVEN_KING || mm == PM_ELVEN_QUEEN) {
                if (rn2(3) || (in_mklev && Is_earthlevel(&u.uz)))
                    (void) mongets(mtmp, PICK_AXE);
                if (!rn2(50))
                    (void) mongets(mtmp, CRYSTAL_BALL);
            }
        } else if (ptr->msound == MS_PRIEST
                   || quest_mon_represents_role(ptr, PM_PRIEST)) {
            otmp = mksobj(MACE, FALSE, FALSE);
            otmp->spe = rnd(3);
            if (!rn2(2))
                curse(otmp);
            (void) mpickobj(mtmp, otmp);
        } else if (mm == PM_NINJA) { /* extra quest villains */
            (void) mongets(mtmp, rn2(4) ? SHURIKEN : DART);
            (void) mongets(mtmp, rn2(4) ? SHORT_SWORD : AXE);
        } else if (mm == PM_KING_ARTHUR) {
            /* If it has not yet been generated, of course
               Arthur gets Excalibur */
            otmp = mksobj(LONG_SWORD, FALSE, FALSE);
            otmp = oname(otmp, artiname(ART_EXCALIBUR));
            bless(otmp);
            otmp->oerodeproof = TRUE;
            otmp->spe = rnd(4) + 1;
            (void) mpickobj(mtmp, otmp);
            (void) mongets(mtmp, LARGE_SHIELD);
        } else if (mm == PM_MEDUSA) {
            (void) mongets(mtmp, ORCISH_BOW);
            m_initthrow(mtmp, ORCISH_ARROW, 24);
        } else if (ptr->msound == MS_GUARDIAN) {
            /* quest "guardians" */
            switch (mm) {
            case PM_STUDENT:
            case PM_ATTENDANT:
            case PM_ABBOT:
            case PM_ACOLYTE:
            case PM_GUIDE:
            case PM_APPRENTICE:
                if (rn2(2))
                    (void) mongets(mtmp, rn2(3) ? DAGGER : KNIFE);
                if (rn2(5))
                    (void) mongets(mtmp, rn2(3) ? JACKET
                                                : CLOAK);
                if (rn2(3))
                    (void) mongets(mtmp, rn2(3) ? LOW_BOOTS : HIGH_BOOTS);
                if (rn2(3))
                    (void) mongets(mtmp, POT_HEALING);
                break;
            case PM_CHIEFTAIN:
            case PM_PAGE:
            case PM_ROSHI:
            case PM_WARRIOR:
                (void) mongets(mtmp, rn2(3) ? LONG_SWORD : SHORT_SWORD);
                (void) mongets(mtmp, rn2(3) ? CHAIN_MAIL : ARMOR);
                if (rn2(2))
                    (void) mongets(mtmp, rn2(2) ? LOW_BOOTS : HIGH_BOOTS);
                if (!rn2(3))
                    (void) mongets(mtmp, CLOAK);
                if (!rn2(3)) {
                    (void) mongets(mtmp, BOW);
                    m_initthrow(mtmp, ARROW, 12);
                }
                break;
            case PM_HUNTER:
                (void) mongets(mtmp, rn2(3) ? SHORT_SWORD : DAGGER);
                if (rn2(2))
                    (void) mongets(mtmp, rn2(2) ? JACKET
                                                : ARMOR);
                (void) mongets(mtmp, BOW);
                m_initthrow(mtmp, ARROW, 12);
                break;
            case PM_THUG:
                (void) mongets(mtmp, CLUB);
                (void) mongets(mtmp, rn2(3) ? DAGGER : KNIFE);
                if (rn2(2))
                    (void) mongets(mtmp, GLOVES);
                (void) mongets(mtmp, rn2(2) ? JACKET : ARMOR);
                break;
            case PM_NEANDERTHAL:
                (void) mongets(mtmp, CLUB);
                (void) mongets(mtmp, ARMOR);
                break;
            }
        }
        else if (mm == PM_CROESUS) {
            (void) mongets(mtmp, TWO_HANDED_SWORD);
            struct obj* received = m_carrying(mtmp, TWO_HANDED_SWORD);
            if (received)
                set_material(received, GOLD);
            int item = rn2(2) ? BANDED_MAIL : PLATE_MAIL;
            (void) mongets(mtmp, item);
            received = m_carrying(mtmp, item);
            if (received)
                set_material(received, GOLD);
            item = rn2(2) ? HELMET : DWARVISH_HELM;
            (void) mongets(mtmp, item);
            received = m_carrying(mtmp, item);
            if (received)
                set_material(received, GOLD);
            item = rn2(2) ? KICKING_BOOTS : DWARVISH_BOOTS;
            (void) mongets(mtmp, item);
            received = m_carrying(mtmp, item);
            if (received)
                set_material(received, GOLD);
            otmp = mksobj(GAUNTLETS, TRUE, FALSE);
            set_material(otmp, GOLD);
            if (!rn2(2)) {
                otmp = oname(otmp, artiname(ART_MIDAS_TOUCH));
                curse(otmp);
            }
            (void) mpickobj(mtmp, otmp);
        }
        break;

    case S_ANGEL:
        if (humanoid(ptr)) {
            /* create minion stuff; can't use mongets */
            otmp = mksobj(LONG_SWORD, FALSE, FALSE);

            /* maybe make it special */
            if (!rn2(20) || is_lord(ptr)) {
                otmp = oname(otmp,
                             artiname(ART_SUNSWORD));
                if (!otmp->oartifact) {
                    otmp = oname(otmp, "");
                    if (!rn2(20))
                        create_oprop(otmp, FALSE);
                }
            }
            bless(otmp);
            otmp->oerodeproof = TRUE;
            otmp->spe = rn2(4);
            (void) mpickobj(mtmp, otmp);

            otmp = mksobj(!rn2(4) || is_lord(ptr) ? SHIELD_OF_REFLECTION
                                                  : LARGE_SHIELD,
                          FALSE, FALSE);
            /* uncurse(otmp); -- mksobj(,FALSE,) item is always uncursed */
            otmp->oerodeproof = TRUE;
            otmp->spe = 0;
            (void) mpickobj(mtmp, otmp);
        }
        break;

    case S_HUMANOID:
        if (is_hobbit(ptr)) {
            switch (rn2(3)) {
            case 0:
                (void) mongets(mtmp, DAGGER);
                break;
            case 1:
                (void) mongets(mtmp, ELVEN_DAGGER);
                break;
            case 2:
                (void) mongets(mtmp, SLING);
                break;
            }
	    if (!rn2(10)) {
                struct obj* mail = m_carrying(mtmp, ELVEN_CHAIN_MAIL);
                if (mail)
                    set_material(mail, MITHRIL);
            }
            if (!rn2(10))
                (void) mongets(mtmp, DWARVISH_CLOAK);
            if (mm == PM_HOBBIT_ROGUE)
                mkmonmoney(mtmp, (long) rn1(100, 20));
        } else if (is_dwarf(ptr)) {
            if (rn2(7))
                (void) mongets(mtmp, DWARVISH_CLOAK);
            if (rn2(7))
                (void) mongets(mtmp, DWARVISH_BOOTS);
            if (!rn2(4)) {
                (void) mongets(mtmp, DWARVISH_SHORT_SWORD);
                /* note: you can't use a mattock with a shield */
                if (rn2(2))
                    (void) mongets(mtmp, DWARVISH_MATTOCK);
                else {
                    (void) mongets(mtmp, rn2(2) ? AXE : DWARVISH_SPEAR);
                    (void) mongets(mtmp, DWARVISH_ROUNDSHIELD);
                }
                (void) mongets(mtmp, DWARVISH_HELM);
                if (!rn2(3) && mm != PM_DWARF) {
                    (void) mongets(mtmp, DWARVISH_CHAIN_MAIL);
                    if ((ptr == &mons[PM_DWARF_LORD] && !rn2(4))
                        || (ptr == &mons[PM_DWARF_LADY] && !rn2(4))
                        || (ptr == &mons[PM_DWARF_QUEEN] && !rn2(2))
                        || (ptr == &mons[PM_DWARF_KING] && !rn2(2))) {
                        struct obj* mail = m_carrying(mtmp, DWARVISH_CHAIN_MAIL);
                        if (mail)
                            set_material(mail, MITHRIL);
                    }
                }
            } else {
                (void) mongets(mtmp, !rn2(3) ? PICK_AXE : DAGGER);
            }
        } else if (mm == PM_MINER) {
            (void)mongets(mtmp, PICK_AXE);
            otmp = mksobj(LANTERN, TRUE, FALSE);
            (void) mpickobj(mtmp, otmp);
            begin_burn(otmp, FALSE);
        }
        break;
    case S_KOP:
        /* create Keystone Kops with cream pies to
           throw. As suggested by KAA.     [MRS] */
        if (!rn2(4))
            m_initthrow(mtmp, CREAM_PIE, 2);
        if (!rn2(3))
            (void) mongets(mtmp, (rn2(2)) ? CLUB : RUBBER_HOSE);
        break;
    case S_ORC:
        if (rn2(2))
            (void) mongets(mtmp, ORCISH_HELM);
        switch ((mm != PM_ORC_CAPTAIN) ? mm
                : rn2(2) ? PM_MORDOR_ORC : PM_URUK_HAI) {
        case PM_MORDOR_ORC:
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SCIMITAR);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SHIELD);
            if (!rn2(3))
                (void) mongets(mtmp, KNIFE);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_CHAIN_MAIL);
            break;
        case PM_URUK_HAI:
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_CLOAK);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SHORT_SWORD);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_BOOTS);
            if (!rn2(3)) {
                (void) mongets(mtmp, ORCISH_BOW);
                m_initthrow(mtmp, ORCISH_ARROW, 12);
            }
            if (!rn2(3))
                (void) mongets(mtmp, URUK_HAI_SHIELD);
            break;
        case PM_GOBLIN_OUTRIDER:
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_CLOAK);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SPEAR);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SHIELD);
            if (!rn2(3)) {
                (void) mongets(mtmp, ORCISH_BOW);
                m_initthrow(mtmp, ORCISH_ARROW, 12);
            }
            break;
        case PM_GOBLIN_CAPTAIN:
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_CLOAK);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SCIMITAR);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_SHIELD);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_MORNING_STAR);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_CHAIN_MAIL);
            if (!rn2(3))
                (void) mongets(mtmp, ORCISH_BOOTS);
            break;
        default:
            if (mm != PM_ORC_SHAMAN && rn2(2))
                (void) mongets(mtmp, (mm == PM_GOBLIN || rn2(2) == 0)
                                         ? ORCISH_DAGGER
                                         : ORCISH_SCIMITAR);
        }
        break;
    case S_OGRE:
        if (!rn2(mm == PM_OGRE_KING ? 3 : mm == PM_OGRE_LORD ? 6 : 12))
            (void) mongets(mtmp, BATTLE_AXE);
        if (!rn2(mm == PM_OGRE_QUEEN ? 3 : mm == PM_OGRE_LADY ? 6 : 12))
            (void) mongets(mtmp, BATTLE_AXE);
        else
            (void) mongets(mtmp, CLUB);
        break;
    case S_TROLL:
        if (!rn2(2))
            switch (rn2(4)) {
            case 0:
                (void) mongets(mtmp, RANSEUR);
                break;
            case 1:
                (void) mongets(mtmp, PARTISAN);
                break;
            case 2:
                (void) mongets(mtmp, GLAIVE);
                break;
            case 3:
                (void) mongets(mtmp, SPETUM);
                break;
            }
        break;
    case S_KOBOLD:
        if (!rn2(4))
            m_initthrow(mtmp, DART, 12);
        break;

    case S_CENTAUR:
        if (rn2(2)) {
            (void) mongets(mtmp, (rn2(2) ? ARMOR : STUDDED_ARMOR));
            (void) mongets(mtmp, (rn2(2) ? GLOVES : ELVEN_HELM));
        if (ptr == &mons[PM_FOREST_CENTAUR]) {
            (void) mongets(mtmp, BOW);
            m_initthrow(mtmp, ARROW, 12);
        } else {
            (void) mongets(mtmp, CROSSBOW);
            m_initthrow(mtmp, CROSSBOW_BOLT, 12);
            }
        }
        break;
    case S_WRAITH:
        (void) mongets(mtmp, KNIFE);
        (void) mongets(mtmp, LONG_SWORD);
        break;
    case S_ZOMBIE:
        if (!rn2(4))
            (void) mongets(mtmp, ARMOR);
        if (!rn2(4))
            (void) mongets(mtmp, (rn2(3) ? KNIFE : SHORT_SWORD));
        break;
    case S_LIZARD:
        if (mm == PM_SALAMANDER)
            (void) mongets(mtmp,
                           (rn2(7) ? SPEAR : rn2(3) ? TRIDENT : STILETTO));
        break;
    case S_DEMON:
        switch (mm) {
        case PM_BALROG:
            (void) mongets(mtmp, BULLWHIP);
            (void) mongets(mtmp, BROADSWORD);
            break;
        case PM_ORCUS:
            (void) mongets(mtmp, WAN_DEATH); /* the Wand of Orcus */
            break;
        case PM_HORNED_DEVIL:
            (void) mongets(mtmp, rn2(4) ? TRIDENT : BULLWHIP);
            break;
        case PM_DISPATER:
            (void) mongets(mtmp, WAN_STRIKING);
            break;
        case PM_YEENOGHU:
            (void) mongets(mtmp, FLAIL);
            break;
        }
        /* prevent djinn and mail daemons from leaving objects when
         * they vanish
         */
        if (!is_demon(ptr))
            break;
        /*FALLTHRU*/
    default:
        /*
         * Now the general case, some chance of getting some type
         * of weapon for "normal" monsters.  Certain special types
         * of monsters will get a bonus chance or different selections.
         */
        bias = is_lord(ptr) + is_prince(ptr) * 2 + extra_nasty(ptr);
        switch (rnd(14 - (2 * bias))) {
        case 1:
            if (strongmonst(ptr))
                (void) mongets(mtmp, BATTLE_AXE);
            else
                m_initthrow(mtmp, DART, 12);
            break;
        case 2:
            if (strongmonst(ptr))
                (void) mongets(mtmp, TWO_HANDED_SWORD);
            else {
                (void) mongets(mtmp, CROSSBOW);
                m_initthrow(mtmp, CROSSBOW_BOLT, 12);
            }
            break;
        case 3:
            (void) mongets(mtmp, BOW);
            m_initthrow(mtmp, ARROW, 12);
            break;
        case 4:
            if (strongmonst(ptr))
                (void) mongets(mtmp, LONG_SWORD);
            else
                m_initthrow(mtmp, DAGGER, 3);
            break;
        case 5:
            if (strongmonst(ptr))
                (void) mongets(mtmp, LUCERN_HAMMER);
            else
                (void) mongets(mtmp, AKLYS);
            break;
        default:
            break;
        }
        break;
    }

    if ((int) mtmp->m_lev > rn2(75))
        (void) mongets(mtmp, rnd_offensive_item(mtmp));
}

/*
 *   Makes up money for monster's inventory.
 *   This will change with silver & copper coins
 */
void
mkmonmoney(mtmp, amount)
struct monst *mtmp;
long amount;
{
    struct obj *gold = mksobj(GOLD_PIECE, FALSE, FALSE);

    gold->quan = amount;
    add_to_minv(mtmp, gold);
}

STATIC_OVL void
m_initinv(mtmp)
register struct monst *mtmp;
{
    register int cnt;
    register struct obj *otmp;
    register struct permonst *ptr = mtmp->data;

    if (Is_rogue_level(&u.uz))
        return;
    /*
     *  Soldiers get armour & rations - armour approximates their ac.
     *  Nymphs may get mirror or potion of object detection.
     */
    switch (ptr->mlet) {
    case S_HUMAN:
        if (is_mercenary(ptr)) {
            register int mac;

            switch (monsndx(ptr)) {
            case PM_GUARD:
                mac = -1;
                break;
            case PM_PRISON_GUARD:
                mac = -2;
                break;
            case PM_HUMAN_SOLDIER:
                mac = 3;
                break;
            case PM_ELVEN_SOLDIER:
                mac = 3;
                break;
            case PM_DWARVISH_SOLDIER:
                mac = 3;
                break;
            case PM_GNOMISH_SOLDIER:
                mac = 3;
                break;
            case PM_ORCISH_SOLDIER:
                mac = 3;
                break;
            case PM_GIANT_SOLDIER:
                mac = 3;
                break;
            case PM_CENTAURIAN_SOLDIER:
                mac = 3;
                break;
            case PM_HUMAN_SERGEANT:
                mac = 0;
                break;
            case PM_ELVEN_SERGEANT:
                mac = 0;
                break;
            case PM_DWARVISH_SERGEANT:
                mac = 0;
                break;
            case PM_GNOMISH_SERGEANT:
                mac = 0;
                break;
            case PM_ORCISH_SERGEANT:
                mac = 0;
                break;
            case PM_GIANT_SERGEANT:
                mac = 0;
                break;
            case PM_CENTAURIAN_SERGEANT:
                mac = 0;
                break;
            case PM_HUMAN_LIEUTENANT:
                mac = -2;
                break;
            case PM_ELVEN_LIEUTENANT:
                mac = -2;
                break;
            case PM_DWARVISH_LIEUTENANT:
                mac = -2;
                break;
            case PM_GNOMISH_LIEUTENANT:
                mac = -2;
                break;
            case PM_ORCISH_LIEUTENANT:
                mac = -2;
                break;
            case PM_GIANT_LIEUTENANT:
                mac = -2;
                break;
            case PM_CENTAURIAN_LIEUTENANT:
                mac = -2;
                break;
            case PM_HUMAN_CAPTAIN:
                mac = -3;
                break;
            case PM_ELVEN_CAPTAIN:
                mac = -3;
                break;
            case PM_DWARVISH_CAPTAIN:
                mac = -3;
                break;
            case PM_GNOMISH_CAPTAIN:
                mac = -3;
                break;
            case PM_GIANT_CAPTAIN:
                mac = -3;
                break;
            case PM_WATCHMAN:
                mac = 3;
                break;
            case PM_WATCH_CAPTAIN:
                mac = -2;
                break;
            default:
                impossible("odd mercenary %d?", monsndx(ptr));
                mac = 0;
                break;
            }

            if (mac < -1 && rn2(5) && !MH_GIANT)
                mac += 7 + mongets(mtmp, (rn2(5)) ? PLATE_MAIL
                                                  : CRYSTAL_PLATE_MAIL);
            else if (mac < 3 && rn2(5)
                       && (!MH_GIANT || !MH_ELF || !MH_ORC))
                mac += 6 + mongets(mtmp, (rn2(3)) ? SPLINT_MAIL
                                                  : BANDED_MAIL);
            else if (mac < 3 && rn2(5)
                     && (ptr == &mons[PM_ELVEN_SOLDIER]
                         || ptr == &mons[PM_ELVEN_SERGEANT]
                         || ptr == &mons[PM_ELVEN_LIEUTENANT]
                         || ptr == &mons[PM_ELVEN_CAPTAIN]))
                mac += 6 + mongets(mtmp, (rn2(3)) ? ELVEN_CHAIN_MAIL
                                                  : BANDED_MAIL);
            else if (rn2(5) && (!MH_GIANT || !MH_ELF || !MH_ORC))
                mac += 3 + mongets(mtmp, (rn2(3)) ? RING_MAIL
                                                  : STUDDED_ARMOR);
            else if (rn2(5) && (ptr == &mons[PM_ORCISH_SOLDIER]
                                || ptr == &mons[PM_ORCISH_SERGEANT]
                                || ptr == &mons[PM_ORCISH_LIEUTENANT]))
                mac += 3 + mongets(mtmp, (rn2(3)) ? ORCISH_RING_MAIL
                                                  : ARMOR);
            else
                mac += 2 + mongets(mtmp, ARMOR);

            if (mac < 10 && rn2(3) && (!MH_ELF || !MH_ORC))
                mac += 1 + mongets(mtmp, HELMET);
            else if (mac < 10 && rn2(2) && (!MH_ELF || !MH_ORC))
                mac += 1 + mongets(mtmp, DENTED_POT);
            else if (mac < 10 && rn2(3)
                     && (ptr == &mons[PM_ELVEN_SOLDIER]
                         || ptr == &mons[PM_ELVEN_SERGEANT]
                         || ptr == &mons[PM_ELVEN_LIEUTENANT]
                         || ptr == &mons[PM_ELVEN_CAPTAIN]))
                mac += 1 + mongets(mtmp, ELVEN_HELM);
            else if (mac < 10 && rn2(3)
                     && (ptr == &mons[PM_ORCISH_SOLDIER]
                         || ptr == &mons[PM_ORCISH_SERGEANT]
                         || ptr == &mons[PM_ORCISH_LIEUTENANT]))
                mac += 1 + mongets(mtmp, ORCISH_HELM);

            if (mac < 10 && rn2(3) && (!MH_ELF || !MH_ORC))
                mac += 1 + mongets(mtmp, SMALL_SHIELD);
            else if (mac < 10 && rn2(2) && (!MH_ELF || !MH_ORC))
                mac += 2 + mongets(mtmp, LARGE_SHIELD);
            else if (mac < 10 && rn2(2)
                     && (ptr == &mons[PM_ELVEN_SOLDIER]
                         || ptr == &mons[PM_ELVEN_SERGEANT]
                         || ptr == &mons[PM_ELVEN_LIEUTENANT]
                         || ptr == &mons[PM_ELVEN_CAPTAIN]))
                mac += 2 + mongets(mtmp, ELVEN_SHIELD);
            else if (mac < 10 && rn2(2)
                     && (ptr == &mons[PM_ORCISH_SOLDIER]
                         || ptr == &mons[PM_ORCISH_SERGEANT]
                         || ptr == &mons[PM_ORCISH_LIEUTENANT]))
                mac += 2 + mongets(mtmp, ORCISH_SHIELD);

            if (mac < 10 && rn2(3) && !MH_CENTAUR)
                mac += 1 + mongets(mtmp, LOW_BOOTS);
            else if (mac < 10 && rn2(2) && !MH_CENTAUR)
                mac += 2 + mongets(mtmp, HIGH_BOOTS);

            if (mac < 10 && rn2(3))
                mac += 1 + mongets(mtmp, (rn2(3)) ? GLOVES
                                                  : GAUNTLETS);
            else if (mac < 10 && rn2(2) && (!MH_GIANT || !MH_ELF || !MH_ORC))
                mac += 1 + mongets(mtmp, CLOAK);
            else if (mac < 10 && rn2(2)
                     && (ptr == &mons[PM_ELVEN_SOLDIER]
                         || ptr == &mons[PM_ELVEN_SERGEANT]
                         || ptr == &mons[PM_ELVEN_LIEUTENANT]
                         || ptr == &mons[PM_ELVEN_CAPTAIN]))
                mac += 1 + mongets(mtmp, ELVEN_CLOAK);
            else if (mac < 10 && rn2(2)
                     && (ptr == &mons[PM_ORCISH_SOLDIER]
                         || ptr == &mons[PM_ORCISH_SERGEANT]
                         || ptr == &mons[PM_ORCISH_LIEUTENANT]))
                mac += 1 + mongets(mtmp, ORCISH_CLOAK);

            nhUse(mac); /* suppress 'dead increment' from static analyzer */

            if (ptr == &mons[PM_WATCH_CAPTAIN]) {
                ; /* better weapon rather than extra gear here */
            } else if (ptr == &mons[PM_WATCHMAN]) {
                if (rn2(3)) /* most watchmen carry a whistle */
                    (void) mongets(mtmp, PEA_WHISTLE);
            } else if (ptr == &mons[PM_GUARD]) {
                /* if hero teleports out of a vault while being confronted
                   by the vault's guard, there is a shrill whistling sound,
                   so guard evidently carries a cursed whistle */
                otmp = mksobj(PEA_WHISTLE, TRUE, FALSE);
                curse(otmp);
                (void) mpickobj(mtmp, otmp);
            } else { /* soldiers and their officers */
                if (!rn2(3))
                    (void) mongets(mtmp, K_RATION);
                if (!rn2(2))
                    (void) mongets(mtmp, C_RATION);
                if ((!is_soldier(ptr)) && !rn2(3))
                    (void) mongets(mtmp, BUGLE);
            }
        } else if (ptr == &mons[PM_SHOPKEEPER]) {
            (void) mongets(mtmp, SKELETON_KEY);
            switch (rn2(4)) {
            /* MAJOR fall through ... */
            case 0:
                (void) mongets(mtmp, WAN_MAGIC_MISSILE);
                /*FALLTHRU*/
            case 1:
                (void) mongets(mtmp, POT_EXTRA_HEALING);
                /*FALLTHRU*/
            case 2:
                (void) mongets(mtmp, POT_HEALING);
                /*FALLTHRU*/
            case 3:
                (void) mongets(mtmp, WAN_STRIKING);
            }
        } else if (ptr->msound == MS_PRIEST
                   || quest_mon_represents_role(ptr, PM_PRIEST)) {
            (void) mongets(mtmp, rn2(7) ? ROBE
                                        : rn2(3) ? CLOAK_OF_PROTECTION
                                                 : CLOAK_OF_MAGIC_RESISTANCE);
            (void) mongets(mtmp, SMALL_SHIELD);
            mkmonmoney(mtmp, (long) rn1(10, 20));
        } else if (quest_mon_represents_role(ptr, PM_MONK)) {
            (void) mongets(mtmp, rn2(11) ? ROBE : CLOAK_OF_MAGIC_RESISTANCE);
        }
        break;
    case S_NYMPH:
        if (!rn2(2))
            (void) mongets(mtmp, MIRROR);
        if (!rn2(2))
            (void) mongets(mtmp, POT_OBJECT_DETECTION);
        break;
    case S_GIANT:
        if (ptr == &mons[PM_MINOTAUR] || ptr == &mons[PM_ELDER_MINOTAUR]) {
            if (!rn2(3) || (in_mklev && Is_earthlevel(&u.uz)))
                (void) mongets(mtmp, WAN_DIGGING);
        } else if (is_giant(ptr)) {
            for (cnt = rn2((int) (mtmp->m_lev / 2)); cnt; cnt--) {
                otmp = mksobj(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE - 1),
                              FALSE, FALSE);
                otmp->quan = (long) rn1(2, 3);
                otmp->owt = weight(otmp);
                (void) mpickobj(mtmp, otmp);
            }
        }
        break;
    case S_WRAITH:
        if (ptr == &mons[PM_NAZGUL]) {
            otmp = mksobj(RIN_INVISIBILITY, FALSE, FALSE);
            curse(otmp);
            (void) mpickobj(mtmp, otmp);
        }
        break;
    case S_LICH:
        if (ptr == &mons[PM_MASTER_LICH] && !rn2(13))
            (void) mongets(mtmp, (rn2(7) ? ATHAME : WAN_NOTHING));
        else if (ptr == &mons[PM_ARCH_LICH] && !rn2(3)) {
            otmp = mksobj(rn2(3) ? ATHAME : QUARTERSTAFF, TRUE,
                          rn2(13) ? FALSE : TRUE);
            if (otmp->spe < 2)
                otmp->spe = rnd(3);
            if (!rn2(4))
                otmp->oerodeproof = 1;
            (void) mpickobj(mtmp, otmp);
        }
        break;
    case S_MUMMY:
        if (rn2(7))
            (void) mongets(mtmp, MUMMY_WRAPPING);
        break;
    case S_QUANTMECH:
        if (!rn2(20)) {
            struct obj *catcorpse;

            otmp = mksobj(LARGE_BOX, FALSE, FALSE);
            /* we used to just set the flag, which resulted in weight()
               treating the box as being heavier by the weight of a cat;
               now we include a cat corpse that won't rot; when opening or
               disclosing the box's contents, the corpse might be revived,
               otherwise it's given a rot timer; weight is now ordinary */
            if ((catcorpse = mksobj(CORPSE, TRUE, FALSE)) != 0) {
                otmp->spe = 1; /* flag for special SchroedingersBox */
                set_corpsenm(catcorpse, PM_HOUSECAT);
                (void) stop_timer(ROT_CORPSE, obj_to_any(catcorpse));
                add_to_container(otmp, catcorpse);
                otmp->owt = weight(otmp);
            }
            (void) mpickobj(mtmp, otmp);
        }
        break;
    case S_LEPRECHAUN:
        mkmonmoney(mtmp, (long) d(level_difficulty(), 30));
        break;
    case S_DEMON:
        /* moved here from m_initweap() because these don't
           have AT_WEAP so m_initweap() is not called for them */
        if (ptr == &mons[PM_ICE_DEVIL] && !rn2(4)) {
            (void) mongets(mtmp, SPEAR);
        } else if (ptr == &mons[PM_ASMODEUS]) {
            (void) mongets(mtmp, WAN_COLD);
            (void) mongets(mtmp, WAN_FIRE);
        }
        break;
    case S_GNOME:
        if (!rn2((In_mines(&u.uz) && in_mklev) ? 20 : 60)) {
            otmp = mksobj(rn2(4) ? TALLOW_CANDLE : WAX_CANDLE, TRUE, FALSE);
            otmp->quan = 1;
            otmp->owt = weight(otmp);
            if (!mpickobj(mtmp, otmp) && !levl[mtmp->mx][mtmp->my].lit)
                begin_burn(otmp, FALSE);
        }
        break;
    default:
        break;
    }

    /* ordinary soldiers rarely have access to magic (or gold :-) */
    if ((is_soldier(ptr)) && rn2(13))
        return;

    if ((int) mtmp->m_lev > rn2(50))
        (void) mongets(mtmp, rnd_defensive_item(mtmp));
    if ((int) mtmp->m_lev > rn2(100))
        (void) mongets(mtmp, rnd_misc_item(mtmp));
    if (likes_gold(ptr) && !findgold(mtmp->minvent, TRUE) && !rn2(5))
        mkmonmoney(mtmp,
                   (long) d(level_difficulty(), mtmp->minvent ? 5 : 10));
}

/* Note: for long worms, always call cutworm (cutworm calls clone_mon) */
struct monst *
clone_mon(mon, x, y)
struct monst *mon;
xchar x, y; /* clone's preferred location or 0 (near mon) */
{
    coord mm;
    struct monst *m2;

    /* may be too weak or have been extinguished for population control */
    if (mon->mhp <= 1 || (mvitals[monsndx(mon->data)].mvflags & G_EXTINCT))
        return (struct monst *) 0;

    if (x == 0) {
        mm.x = mon->mx;
        mm.y = mon->my;
    } else {
        mm.x = x;
        mm.y = y;
    }
    if (!isok(mm.x, mm.y)) { /* paranoia */
        impossible("clone_mon trying to create a monster at <%d,%d>?",
                   mm.x, mm.y);
        return (struct monst *) 0;
    }
    if (MON_AT(mm.x, mm.y)) { /* (always True for the x==0 case) */
        if (!enexto(&mm, mm.x, mm.y, mon->data) || MON_AT(mm.x, mm.y))
            return (struct monst *) 0;
    }

    m2 = newmonst();
    *m2 = *mon; /* copy condition of old monster */
    m2->mextra = (struct mextra *) 0;
    m2->nmon = fmon;
    fmon = m2;
    m2->m_id = context.ident++;
    if (!m2->m_id)
        m2->m_id = context.ident++; /* ident overflowed */
    m2->mx = mm.x;
    m2->my = mm.y;

    m2->mundetected = 0;
    m2->mtrapped = 0;
    m2->mcloned = 1;
    m2->minvent = (struct obj *) 0; /* objects don't clone */
    m2->mleashed = 0;
    /* Max HP the same, but current HP halved for both.  The caller
     * might want to override this by halving the max HP also.
     * When current HP is odd, the original keeps the extra point.
     * We know original has more than 1 HP, so both end up with at least 1.
     */
    m2->mhpmax = mon->mhpmax;
    m2->mhp = mon->mhp / 2;
    mon->mhp -= m2->mhp;

    /* clone doesn't have mextra so mustn't retain special monster flags */
    m2->isshk = 0;
    m2->isgd = 0;
    m2->ispriest = 0;
    /* ms->isminion handled below */

    /* clone shouldn't be reluctant to move on spots 'parent' just moved on */
    (void) memset((genericptr_t) m2->mtrack, 0, sizeof m2->mtrack);

    place_monster(m2, m2->mx, m2->my);
    if (emits_light(m2->data))
        new_light_source(m2->mx, m2->my, emits_light(m2->data), LS_MONSTER,
                         monst_to_any(m2));
    /* if 'parent' is named, give the clone the same name */
    if (has_mname(mon)) {
        m2 = christen_monst(m2, MNAME(mon));
    } else if (mon->isshk) {
        m2 = christen_monst(m2, shkname(mon));
    }

    /* not all clones caused by player are tame or peaceful */
    if (!context.mon_moving && mon->mpeaceful) {
        if (mon->mtame)
            m2->mtame = rn2(max(2 + u.uluck, 2)) ? mon->mtame : 0;
        else if (mon->mpeaceful)
            m2->mpeaceful = rn2(max(2 + u.uluck, 2)) ? 1 : 0;
    }
    /* if guardian angel could be cloned (maybe after polymorph?),
       m2 could be both isminion and mtame; isminion takes precedence */
    if (m2->isminion) {
        int atyp;

        newemin(m2);
        *EMIN(m2) = *EMIN(mon);
        /* renegade when same alignment as hero but not peaceful or
           when peaceful while being different alignment from hero */
        atyp = EMIN(m2)->min_align;
        EMIN(m2)->renegade = (atyp != u.ualign.type) ^ !m2->mpeaceful;
    } else if (m2->mtame) {
        /* Because m2 is a copy of mon it is tame but not init'ed.
           However, tamedog() will not re-tame a tame dog, so m2
           must be made non-tame to get initialized properly. */
        m2->mtame = 0;
        if (tamedog(m2, (struct obj *) 0))
            *EDOG(m2) = *EDOG(mon);
        /* [TODO? some (most? all?) edog fields probably should be
           reinitialized rather that retain the 'parent's values] */
    }
    set_malign(m2);
    newsym(m2->mx, m2->my); /* display the new monster */

    return m2;
}

/*
 * Propagate a species
 *
 * Once a certain number of monsters are created, don't create any more
 * at random (i.e. make them extinct).  The previous (3.2) behavior was
 * to do this when a certain number had _died_, which didn't make
 * much sense.
 *
 * Returns FALSE propagation unsuccessful
 *         TRUE  propagation successful
 */
boolean
propagate(mndx, tally, ghostly)
int mndx;
boolean tally;
boolean ghostly;
{
    boolean result;
    uchar lim = mbirth_limit(mndx);
    boolean gone = (mvitals[mndx].mvflags & G_GONE) != 0; /* geno'd|extinct */

    result = (((int) mvitals[mndx].born < lim) && !gone) ? TRUE : FALSE;

    /* if it's unique, don't ever make it again */
    if ((mons[mndx].geno & G_UNIQ) && mndx != PM_HIGH_PRIEST)
        mvitals[mndx].mvflags |= G_EXTINCT;

    if (mvitals[mndx].born < 255 && tally
        && (!ghostly || (ghostly && result)))
        mvitals[mndx].born++;
    if ((int) mvitals[mndx].born >= lim && !(mons[mndx].geno & G_NOGEN)
        && !(mvitals[mndx].mvflags & G_EXTINCT)) {
        if (wizard) {
            debugpline1("Automatically extinguished %s.",
                        makeplural(mons[mndx].mname));
        }
        mvitals[mndx].mvflags |= G_EXTINCT;
        reset_rndmonst(mndx);
    }
    return result;
}

/* From xNetHack:
 * Hit dice size of a monster, based on its size. (Max HP is then usually
 * calculated by rolling a die of this size for each level the monster has.)
 * It used to be 8 for all monsters, but it makes more sense for, say, a mumak
 * to be beefier than a killer bee of the same level.
 * For EvilHack - we randomize the amount of HD just a bit. */
STATIC_OVL xchar
hd_size(ptr)
struct permonst * ptr;
{
    switch(ptr->msize) {
    case MZ_TINY:
        return 5 + rn2(2);
    case MZ_SMALL:
        return 7 + rn2(2);
    case MZ_MEDIUM:
        return 8 + rn2(2);
    case MZ_LARGE:
        return 10 + rn2(3);
    case MZ_HUGE:
        return 14 + rn2(3);
    case MZ_GIGANTIC:
        return 18 + rn2(4);
    default:
        impossible("hd_size: unknown monster size %d", ptr->msize);
        return 8;
    }
}

/* amount of HP to lose from level drain (or gain from Stormbringer) */
int
monhp_per_lvl(mon)
struct monst *mon;
{
    struct permonst *ptr = mon->data;
    int hp = rnd(hd_size(ptr)); /* default is d8 */

    /* like newmonhp, but home elementals are ignored, riders use normal d8 */
    if (is_golem(ptr)) {
        /* draining usually won't be applicable for these critters */
        hp = golemhp(monsndx(ptr)) / (int) ptr->mlevel;
    } else if (ptr->mlevel > 49) {
        /* arbitrary; such monsters won't be involved in draining anyway */
        hp = 4 + rnd(4); /* 5..8 */
    } else if (mon->m_lev == 0) {
        /* level 0 monsters use 1d4 instead of Nd8 */
        hp = rnd(4);
    }
    return hp;
}

/* Compute an appropriate maximum HP for a given monster type and level. */
int
monmaxhp(ptr, m_lev)
struct permonst *ptr;
uchar m_lev; /* not just a struct mon because polyself code also uses this */
{
    if (is_golem(ptr)) {
        return golemhp(monsndx(ptr));
    } else if (is_rider(ptr)) {
        /* we want low HP, but a high mlevel so they can attack well
         * Riders are a bit too easy to take down. Let's buff them up a bit */
        return 100 + d(10, 8);
    } else if (ptr->mlevel > 49) {
        /* "special" fixed hp monster
         * the hit points are encoded in the mlevel in a somewhat strange
         * way to fit in the 50..127 positive range of a signed character
         * above the 1..49 that indicate "normal" monster levels */
        return 2 * (ptr->mlevel - 6);
    } else if (m_lev == 0) {
        return rnd(4);
    } else {
        int hpmax = d(m_lev, hd_size(ptr));
        if (is_home_elemental(ptr))
            hpmax *= 3;
        if (is_mplayer(ptr))
            hpmax *= 1.25;
        return hpmax;
    }
}

/* set up a new monster's initial level and hit points;
   used by newcham() as well as by makemon() */
void
newmonhp(mon, mndx)
struct monst *mon;
int mndx;
{
    struct permonst *ptr = &mons[mndx];

    mon->m_lev = adj_lev(ptr);
    mon->mhpmax = mon->mhp = monmaxhp(ptr, mon->m_lev);
    if (ptr->mlevel > 49) {
        /* Second half of the "special" fixed hp monster code: adjust level */
        mon->m_lev = mon->mhp / 4; /* approximation */
    }
}

struct mextra *
newmextra()
{
    struct mextra *mextra;

    mextra = (struct mextra *) alloc(sizeof(struct mextra));
    mextra->mname = 0;
    mextra->egd = 0;
    mextra->epri = 0;
    mextra->eshk = 0;
    mextra->emin = 0;
    mextra->edog = 0;
    mextra->erid = 0;
    mextra->mcorpsenm = NON_PM;
    return mextra;
}

STATIC_OVL boolean
makemon_rnd_goodpos(mon, gpflags, cc)
struct monst *mon;
unsigned gpflags;
coord *cc;
{
    int tryct = 0;
    int nx,ny;
    boolean good;

    do {
        nx = rn1(COLNO - 3, 2);
        ny = rn2(ROWNO);
        good = (!in_mklev && cansee(nx,ny)) ? FALSE
                                            : goodpos(nx, ny, mon, gpflags);
    } while ((++tryct < 50) && !good);

    if (!good) {
        /* else go through all map positions, twice, first round
           ignoring positions in sight, and pick first good one.
           skip first round if we're in special level loader or blind */
        int xofs = nx;
        int yofs = ny;
        int dx,dy;
        int bl = (in_mklev || Blind) ? 1 : 0;

        for ( ; bl < 2; bl++) {
            for (dx = 0; dx < COLNO; dx++)
                for (dy = 0; dy < ROWNO; dy++) {
                    nx = ((dx + xofs) % (COLNO - 1)) + 1;
                    ny = ((dy + yofs) % (ROWNO - 1)) + 1;
                    if (bl == 0 && cansee(nx,ny))
                        continue;
                    if (goodpos(nx, ny, mon, gpflags))
                        goto gotgood;
                }
            if (bl == 0 && (!mon || mon->data->mmove)) {
                /* all map positions are visible (or not good),
                   try to pick something logical */
                if (dnstair.sx && !rn2(2)) {
                    nx = dnstair.sx;
                    ny = dnstair.sy;
                } else if (upstair.sx && !rn2(2)) {
                    nx = upstair.sx;
                    ny = upstair.sy;
                } else if (dnladder.sx && !rn2(2)) {
                    nx = dnladder.sx;
                    ny = dnladder.sy;
                } else if (upladder.sx && !rn2(2)) {
                    nx = upladder.sx;
                    ny = upladder.sy;
                }
                if (goodpos(nx, ny, mon, gpflags))
                    goto gotgood;
            }
        }
    } else {
 gotgood:
        cc->x = nx;
        cc->y = ny;
        return TRUE;
    }
    return FALSE;
}

extern void FDECL(get_mplname, (struct monst *, char *));

/*
 * called with [x,y] = coordinates;
 *      [0,0] means anyplace
 *      [u.ux,u.uy] means: near player (if !in_mklev)
 *
 *      In case we make a monster group, only return the one at [x,y].
 */
struct monst *
makemon(ptr, x, y, mmflags)
register struct permonst *ptr;
register int x, y;
int mmflags;
{
    register struct monst *mtmp;
    struct monst fakemon;
    coord cc;
    int mndx, mcham, ct, mitem;
    boolean anymon = (!ptr);
    boolean byyou = (x == u.ux && y == u.uy);
    boolean allow_minvent = ((mmflags & NO_MINVENT) == 0);
    boolean countbirth = ((mmflags & MM_NOCOUNTBIRTH) == 0);
    unsigned gpflags = (mmflags & MM_IGNOREWATER) ? MM_IGNOREWATER : 0;

    fakemon = zeromonst;
    cc.x = cc.y = 0;

    /* if caller wants random location, do it here */
    if (x == 0 && y == 0) {
        fakemon.data = ptr; /* set up for goodpos */
        if (!makemon_rnd_goodpos(ptr ? &fakemon : (struct monst *) 0,
                                 gpflags, &cc))
            return (struct monst *) 0;
        x = cc.x;
        y = cc.y;
    } else if (byyou && !in_mklev) {
        if (!enexto_core(&cc, u.ux, u.uy, ptr, gpflags))
            return (struct monst *) 0;
        x = cc.x;
        y = cc.y;
    }

    /* sanity check */
    if (!isok(x, y)) {
        impossible("makemon trying to create a monster at <%d,%d>?", x, y);
        return (struct monst *) 0;
    }

    /* Does monster already exist at the position? */
    if (MON_AT(x, y)) {
        if (!(mmflags & MM_ADJACENTOK)
            || !enexto_core(&cc, x, y, ptr, gpflags))
            return (struct monst *) 0;
        x = cc.x;
        y = cc.y;
    }

    if (ptr) {
        mndx = monsndx(ptr);
        /* if you are to make a specific monster and it has
           already been genocided, return */
        if (mvitals[mndx].mvflags & G_GENOD)
            return (struct monst *) 0;
        if (wizard && (mvitals[mndx].mvflags & G_EXTINCT)) {
            debugpline1("Explicitly creating extinct monster %s.",
                        mons[mndx].mname);
        }
    } else {
        /* make a random (common) monster that can survive here.
         * (the special levels ask for random monsters at specific
         * positions, causing mass drowning on the medusa level,
         * for instance.)
         */
        int tryct = 0; /* maybe there are no good choices */

        do {
            if (!(ptr = rndmonst())) {
                debugpline0("Warning: no monster.");
                return (struct monst *) 0; /* no more monsters! */
            }
            fakemon.data = ptr; /* set up for goodpos */
        } while (++tryct <= 50
                 /* in Sokoban, don't accept a giant on first try;
                    after that, boulder carriers are fair game */
                 && ((tryct == 1 && throws_rocks(ptr) && In_sokoban(&u.uz))
                     || !goodpos(x, y, &fakemon, gpflags)));
        mndx = monsndx(ptr);
    }
    (void) propagate(mndx, countbirth, FALSE);
    mtmp = newmonst();
    *mtmp = zeromonst; /* clear all entries in structure */

    if (mmflags & MM_EGD)
        newegd(mtmp);
    if (mmflags & MM_EPRI)
        newepri(mtmp);
    if (mmflags & MM_ESHK)
        neweshk(mtmp);
    if (mmflags & MM_EMIN)
        newemin(mtmp);
    if (mmflags & MM_EDOG)
        newedog(mtmp);
    if (mmflags & MM_ASLEEP)
        mtmp->msleeping = 1;
    if (mmflags & MM_ERID)
        newerid(mtmp);
    mtmp->nmon = fmon;
    fmon = mtmp;
    mtmp->m_id = context.ident++;
    if (!mtmp->m_id)
        mtmp->m_id = context.ident++; /* ident overflowed */
    set_mon_data(mtmp, ptr); /* mtmp->data = ptr; */
    if (ptr->msound == MS_LEADER && quest_info(MS_LEADER) == mndx)
        quest_status.leader_m_id = mtmp->m_id;
    mtmp->mnum = mndx;

    /* set up level and hit points */
    newmonhp(mtmp, mndx);

    if (is_female(ptr))
        mtmp->female = TRUE;
    else if (is_male(ptr))
        mtmp->female = FALSE;
    /* leader and nemesis gender is usually hardcoded in mons[],
       but for ones which can be random, it has already been chosen
       (in role_init(), for possible use by the quest pager code) */
    else if (ptr->msound == MS_LEADER && quest_info(MS_LEADER) == mndx)
        mtmp->female = quest_status.ldrgend;
    else if (ptr->msound == MS_NEMESIS && quest_info(MS_NEMESIS) == mndx)
        mtmp->female = quest_status.nemgend;
    else
        mtmp->female = rn2(2); /* ignored for neuters */

    if (In_sokoban(&u.uz) && !mindless(ptr)) /* know about traps here */
        mtmp->mtrapseen = (1L << (PIT - 1)) | (1L << (HOLE - 1));
    if (Is_stronghold(&u.uz) && !mindless(ptr)) /* know about the trap doors */
        mtmp->mtrapseen = (1L << (TRAPDOOR - 1));
    /* quest leader and nemesis both know about all trap types */
    if (ptr->msound == MS_LEADER || ptr->msound == MS_NEMESIS)
        mtmp->mtrapseen = ~0;

    place_monster(mtmp, x, y);
    mtmp->mcansee = mtmp->mcanmove = TRUE;
    mtmp->seen_resistance = M_SEEN_NOTHING;
    mtmp->mpeaceful = (mmflags & MM_ANGRY) ? FALSE : peace_minded(ptr);

    /* Here is where we match riding monsters with their mounts */
    if (!(mmflags & MM_REVIVE)) {
        switch (mndx) {
            case PM_DEATH:
                mount_monster(mtmp, PM_PALE_HORSE);
                break;
            case PM_FAMINE:
                mount_monster(mtmp, PM_BLACK_HORSE);
                break;
            case PM_PESTILENCE:
                mount_monster(mtmp, PM_WHITE_HORSE);
                break;
            case PM_NAZGUL:
                mount_monster(mtmp, PM_FELL_BEAST);
                break;
            case PM_GOBLIN_OUTRIDER:
                mount_monster(mtmp, PM_WOLF);
                break;
            case PM_GOBLIN_CAPTAIN:
                mount_monster(mtmp, PM_WARG);
                break;
            case PM_HUMAN_KNIGHT:
                rn2(2) ? mount_monster(mtmp, PM_HORSE)
                       : mount_monster(mtmp, PM_WARHORSE);
                break;
        }
    }

    switch (ptr->mlet) {
    case S_MIMIC:
        set_mimic_sym(mtmp);
        break;
    case S_SPIDER:
    case S_SNAKE:
        if (in_mklev)
            if (x && y)
                (void) mkobj_at(0, x, y, TRUE);
        (void) hideunder(mtmp);
        break;
    case S_LIGHT:
    case S_ELEMENTAL:
        if (mndx == PM_STALKER || mndx == PM_BLACK_LIGHT) {
            mtmp->perminvis = TRUE;
            mtmp->minvis = TRUE;
        }
        break;
    case S_GHOST:
        if (mndx == PM_GHOST && rn2(2)) {
            mtmp->minvis = TRUE; /* not permanent though */
        }
        break;
    case S_EEL:
        (void) hideunder(mtmp);
        break;
    case S_LEPRECHAUN:
        mtmp->msleeping = 1;
        break;
    case S_JABBERWOCK:
        break;
    case S_EYE:
        if (mtmp->data == &mons[PM_BEHOLDER]) {
            if (rn2(5) && !u.uhave.amulet)
                mtmp->msleeping = 1;
        }
        break;
    case S_NYMPH:
        if (rn2(5) && !u.uhave.amulet)
            mtmp->msleeping = 1;
        break;
    case S_ORC:
        if (Race_if(PM_ELF))
            mtmp->mpeaceful = FALSE;
        break;
    case S_UNICORN:
        if (is_unicorn(ptr) && sgn(u.ualign.type) == sgn(ptr->maligntyp))
            mtmp->mpeaceful = TRUE;
        break;
    case S_BAT:
        if (Inhell && is_bat(ptr))
            mon_adjust_speed(mtmp, 2, (struct obj *) 0);
        break;
    case S_DRAGON:
        /* Dragons are always generated awake and pissed for Knights. */
	if (Role_if(PM_KNIGHT)) {
	    mtmp->mpeaceful = mtmp->mtame = FALSE;
	    mtmp->msleeping = 0;
	}
	break;
    }
    if ((ct = emits_light(mtmp->data)) > 0)
        new_light_source(mtmp->mx, mtmp->my, ct, LS_MONSTER,
                         monst_to_any(mtmp));
    mitem = 0; /* extra inventory item for this monster */

    if (mndx == PM_VLAD_THE_IMPALER)
        mitem = CANDELABRUM_OF_INVOCATION;
    mtmp->cham = NON_PM; /* default is "not a shapechanger" */
    if (!Protection_from_shape_changers
        && (mcham = pm_to_cham(mndx)) != NON_PM) {
        /* this is a shapechanger after all */
        mtmp->cham = mcham;
        /* Vlad stays in his normal shape so he can carry the Candelabrum */
        if (mndx != PM_VLAD_THE_IMPALER
            /* Note:  shapechanger's initial form used to be chosen here
               with rndmonst(), yielding a monster which was appropriate
               to the level's difficulty but ignoring the changer's usual
               type selection, so was inappropriate for vampshifters.
               Let newcham() pick the shape. */
            && newcham(mtmp, (struct permonst *) 0, FALSE, FALSE))
            allow_minvent = FALSE;
    } else if (mndx == PM_WIZARD_OF_YENDOR) {
        mtmp->iswiz = TRUE;
        context.no_of_wizards++;
        if (context.no_of_wizards == 1 && Is_earthlevel(&u.uz))
            mitem = SPE_DIG;
    } else if (mndx == PM_GHOST && !(mmflags & MM_NONAME)) {
        mtmp = christen_monst(mtmp, rndghostname());
    } else if (mndx == urole.neminum) {
        mitem = BELL_OF_OPENING;
    } else if (mndx == PM_PESTILENCE) {
        mitem = POT_SICKNESS;
    }
    if (mitem && allow_minvent)
        (void) mongets(mtmp, mitem);

    if (in_mklev) {
        if ((is_ndemon(ptr) || mndx == PM_WUMPUS
             || mndx == PM_LONG_WORM || mndx == PM_GIANT_EEL)
            && !u.uhave.amulet && rn2(5))
            mtmp->msleeping = TRUE;
    } else {
        if (byyou) {
            newsym(mtmp->mx, mtmp->my);
            set_apparxy(mtmp);
        }
    }
    if (is_dprince(ptr) && ptr->msound == MS_BRIBE) {
        mtmp->mpeaceful = mtmp->minvis = mtmp->perminvis = 1;
        mtmp->mavenge = 0;
        if (uwep && uwep->oartifact == ART_EXCALIBUR)
            mtmp->mpeaceful = mtmp->mtame = FALSE;
    }
#ifndef DCC30_BUG
    if (mndx == PM_LONG_WORM && (mtmp->wormno = get_wormno()) != 0)
#else
    /* DICE 3.0 doesn't like assigning and comparing mtmp->wormno in the
       same expression. */
    if (mndx == PM_LONG_WORM
        && (mtmp->wormno = get_wormno(), mtmp->wormno != 0))
#endif
    {
        /* we can now create worms with tails - 11/91 */
        initworm(mtmp, rn2(5));
        if (count_wsegs(mtmp))
            place_worm_tail_randomly(mtmp, x, y);
    }
    /* it's possible to create an ordinary monster of some special
       types; make sure their extended data is initialized to
       something sensible if caller hasn't specified MM_EPRI|MM_EMIN
       (when they're specified, caller intends to handle this itself) */
    if ((mndx == PM_ALIGNED_PRIEST || mndx == PM_HIGH_PRIEST)
            ? !(mmflags & (MM_EPRI | MM_EMIN))
            : (mndx == PM_ANGEL && !(mmflags & MM_EMIN) && !rn2(3))) {
        struct emin *eminp;

        newemin(mtmp);
        eminp = EMIN(mtmp);

        mtmp->isminion = 1;            /* make priest be a roamer */
        eminp->min_align = rn2(3) - 1; /* no A_NONE */
        eminp->renegade = (boolean) ((mmflags & MM_ANGRY) ? 1 : !rn2(3));
        mtmp->mpeaceful = (eminp->min_align == u.ualign.type)
                              ? !eminp->renegade
                              : eminp->renegade;
    }
    set_malign(mtmp); /* having finished peaceful changes */
    if (anymon && !(mmflags & MM_NOGRP)) {
        if ((ptr->geno & G_SGROUP) && rn2(2)) {
            m_initsgrp(mtmp, mtmp->mx, mtmp->my, mmflags);
        } else if (ptr->geno & G_LGROUP) {
            if (rn2(3))
                m_initlgrp(mtmp, mtmp->mx, mtmp->my, mmflags);
            else
                m_initsgrp(mtmp, mtmp->mx, mtmp->my, mmflags);
        }
    }

    if (allow_minvent) {
        if (is_armed(ptr))
            m_initweap(mtmp); /* equip with weapons / armor */
        m_initinv(mtmp); /* add on a few special items incl. more armor */
        m_dowear(mtmp, TRUE);
        mon_wield_item(mtmp);
        (void) m_stash_items(mtmp, TRUE);

        if (!rn2(100) && is_domestic(ptr)
            && can_saddle(mtmp) && !which_armor(mtmp, W_SADDLE)) {
            struct obj *otmp = mksobj(SADDLE, TRUE, FALSE);

            put_saddle_on_mon(otmp, mtmp);
        }

    } else {
        /* no initial inventory is allowed */
        if (mtmp->minvent)
            discard_minvent(mtmp);
        mtmp->minvent = (struct obj *) 0; /* caller expects this */
    }
    if (ptr->mflags3 && !(mmflags & MM_NOWAIT)) {
        if (ptr->mflags3 & M3_WAITFORU)
            mtmp->mstrategy |= STRAT_WAITFORU;
        if (ptr->mflags3 & M3_CLOSE)
            mtmp->mstrategy |= STRAT_CLOSE;
        if (ptr->mflags3 & (M3_WAITMASK | M3_COVETOUS))
            mtmp->mstrategy |= STRAT_APPEARMSG;
    }

    if (allow_minvent && migrating_objs)
        deliver_obj_to_mon(mtmp, 1, DF_NONE); /* in case of waiting items */

    if (is_mplayer(ptr)) {
	char nam[PL_NSIZ];
        get_mplname(mtmp, nam);
	mtmp = christen_monst(mtmp, nam);
    }

    if (!in_mklev)
        newsym(mtmp->mx, mtmp->my); /* make sure the mon shows up */

    return mtmp;
}

int
mbirth_limit(mndx)
int mndx;
{
    /* There is an implicit limit of 4 for "high priest of <deity>",
     * but aligned priests can grow into high priests, thus they aren't
     * really limited to 4, so leave the default amount in place for them.
     */

    /* assert(MAXMONNO < 255); */
    return (mndx == PM_NAZGUL ? 9 : mndx == PM_ERINYS ? 3 : MAXMONNO);
}

/* used for wand/scroll/spell of create monster */
/* returns TRUE iff you know monsters have been created */
boolean
create_critters(cnt, mptr, neverask)
int cnt;
struct permonst *mptr; /* usually null; used for confused reading */
boolean neverask;
{
    coord c;
    int x, y;
    struct monst *mon;
    boolean known = FALSE;
    boolean ask = (wizard && !neverask);

    while (cnt--) {
        if (ask) {
            if (create_particular()) {
                known = TRUE;
                continue;
            } else
                ask = FALSE; /* ESC will shut off prompting */
        }
        x = u.ux, y = u.uy;
        /* if in water, try to encourage an aquatic monster
           by finding and then specifying another wet location */
        if (!mptr && u.uinwater && enexto(&c, x, y, &mons[PM_GIANT_EEL]))
            x = c.x, y = c.y;

        mon = makemon(mptr, x, y, NO_MM_FLAGS);
        if (mon && canspotmon(mon))
            known = TRUE;
    }
    return known;
}

STATIC_OVL boolean
uncommon(mndx)
int mndx;
{
    if (mons[mndx].geno & (G_NOGEN | G_UNIQ))
        return TRUE;
    if (mvitals[mndx].mvflags & G_GONE)
        return TRUE;
    if (Inhell)
        return (boolean) (mons[mndx].maligntyp > A_NEUTRAL);
    else
        return (boolean) ((mons[mndx].geno & G_HELL) != 0);
}

/*
 *      shift the probability of a monster's generation by
 *      comparing the dungeon alignment and monster alignment.
 *      return an integer in the range of 0-5.
 */
STATIC_OVL int
align_shift(ptr)
register struct permonst *ptr;
{
    static NEARDATA long oldmoves = 0L; /* != 1, starting value of moves */
    static NEARDATA s_level *lev;
    register int alshift;

    if (oldmoves != moves) {
        lev = Is_special(&u.uz);
        oldmoves = moves;
    }
    switch ((lev) ? lev->flags.align : dungeons[u.uz.dnum].flags.align) {
    default: /* just in case */
    case AM_NONE:
        alshift = 0;
        break;
    case AM_LAWFUL:
        alshift = (ptr->maligntyp + 20) / (2 * ALIGNWEIGHT);
        break;
    case AM_NEUTRAL:
        alshift = (20 - abs(ptr->maligntyp)) / ALIGNWEIGHT;
        break;
    case AM_CHAOTIC:
        alshift = (-(ptr->maligntyp - 20)) / (2 * ALIGNWEIGHT);
        break;
    }
    return alshift;
}

static NEARDATA struct {
    int choice_count;
    char mchoices[SPECIAL_PM]; /* value range is 0..127 */
} rndmonst_state = { -1, { 0 } };

/* select a random monster type */
struct permonst *
rndmonst()
{
    register struct permonst *ptr;
    register int mndx, ct;

    if (u.uz.dnum == quest_dnum && rn2(7) && (ptr = qt_montype()) != 0)
        return ptr;

    if (rndmonst_state.choice_count < 0) { /* need to recalculate */
        int zlevel, minmlev, maxmlev;
        boolean elemlevel;
        boolean upper;

        rndmonst_state.choice_count = 0;
        /* look for first common monster */
        for (mndx = LOW_PM; mndx < SPECIAL_PM; mndx++) {
            if (!uncommon(mndx))
                break;
            rndmonst_state.mchoices[mndx] = 0;
        }
        if (mndx == SPECIAL_PM) {
            /* evidently they've all been exterminated */
            debugpline0("rndmonst: no common mons!");
            return (struct permonst *) 0;
        } /* else `mndx' now ready for use below */
        zlevel = level_difficulty();
        /* determine the level of the weakest monster to make. */
        minmlev = zlevel / 6;
        /* determine the level of the strongest monster to make. */
        maxmlev = (zlevel + u.ulevel) / 2;
        upper = Is_rogue_level(&u.uz);
        elemlevel = In_endgame(&u.uz) && !Is_astralevel(&u.uz);

        /*
         * Find out how many monsters exist in the range we have selected.
         */
        for ( ; mndx < SPECIAL_PM; mndx++) { /* (`mndx' initialized above) */
            ptr = &mons[mndx];
            rndmonst_state.mchoices[mndx] = 0;
            if (tooweak(mndx, minmlev) || toostrong(mndx, maxmlev))
                continue;
            if (upper && !isupper((uchar) def_monsyms[(int) ptr->mlet].sym))
                continue;
            if (elemlevel && wrong_elem_type(ptr))
                continue;
            if (uncommon(mndx))
                continue;
            if (Inhell && (ptr->geno & G_NOHELL))
                continue;
            ct = (int) (ptr->geno & G_FREQ) + align_shift(ptr);
	    if (!is_mplayer(ptr))
	        ct *= 3;
            if (ct < 0 || ct > 127)
                panic("rndmonst: bad count [#%d: %d]", mndx, ct);
            rndmonst_state.choice_count += ct;
            rndmonst_state.mchoices[mndx] = (char) ct;
        }
        /*
         *      Possible modification:  if choice_count is "too low",
         *      expand minmlev..maxmlev range and try again.
         */
    } /* choice_count+mchoices[] recalc */

    if (rndmonst_state.choice_count <= 0) {
        /* maybe no common mons left, or all are too weak or too strong */
        debugpline1("rndmonst: choice_count=%d", rndmonst_state.choice_count);
        return (struct permonst *) 0;
    }

    /*
     *  Now, select a monster at random.
     */
    ct = rnd(rndmonst_state.choice_count);
    for (mndx = LOW_PM; mndx < SPECIAL_PM; mndx++)
        if ((ct -= (int) rndmonst_state.mchoices[mndx]) <= 0)
            break;

    if (mndx == SPECIAL_PM || uncommon(mndx)) { /* shouldn't happen */
        impossible("rndmonst: bad `mndx' [#%d]", mndx);
        return (struct permonst *) 0;
    }
    return &mons[mndx];
}

/* called when you change level (experience or dungeon depth) or when
   monster species can no longer be created (genocide or extinction) */
void
reset_rndmonst(mndx)
int mndx; /* particular species that can no longer be created */
{
    /* cached selection info is out of date */
    if (mndx == NON_PM) {
        rndmonst_state.choice_count = -1; /* full recalc needed */
    } else if (mndx < SPECIAL_PM) {
        rndmonst_state.choice_count -= rndmonst_state.mchoices[mndx];
        rndmonst_state.mchoices[mndx] = 0;
    } /* note: safe to ignore extinction of unique monsters */
}

/* decide whether it's ok to generate a candidate monster by mkclass() */
STATIC_OVL boolean
mk_gen_ok(mndx, mvflagsmask, genomask)
int mndx, mvflagsmask, genomask;
{
    struct permonst *ptr = &mons[mndx];

    if (mvitals[mndx].mvflags & mvflagsmask)
        return FALSE;
    if (ptr->geno & genomask)
        return FALSE;
    if (is_placeholder(ptr))
        return FALSE;
#ifdef MAIL
    /* special levels might ask for random demon type; reject this one */
    if (ptr == &mons[PM_MAIL_DAEMON])
        return FALSE;
#endif
    return TRUE;
}

/* Make one of the multiple types of a given monster class.
   The second parameter specifies a special casing bit mask
   to allow the normal genesis masks to be deactivated.
   Returns Null if no monsters in that class can be made. */
struct permonst *
mkclass(class, spc)
char class;
int spc;
{
    return mkclass_aligned(class, spc, A_NONE);
}

/* mkclass() with alignment restrictions; used by ndemon() */
struct permonst *
mkclass_aligned(class, spc, atyp)
char class;
int spc;
aligntyp atyp;
{
    register int first, last, num = 0;
    int k, nums[SPECIAL_PM + 1]; /* +1: insurance for final return value */
    int maxmlev, mask = (G_NOGEN | G_UNIQ) & ~spc;

    (void) memset((genericptr_t) nums, 0, sizeof nums);
    maxmlev = level_difficulty() >> 1;
    if (class < 1 || class >= MAXMCLASSES) {
        impossible("mkclass called with bad class!");
        return (struct permonst *) 0;
    }
    /*  Assumption #1:  monsters of a given class are contiguous in the
     *                  mons[] array.  Player monsters and quest denizens
     *                  are an exception; mkclass() won't pick them.
     *                  SPECIAL_PM is long worm tail and separates the
     *                  regular monsters from the exceptions.
     */
    for (first = LOW_PM; first < SPECIAL_PM; first++)
        if (mons[first].mlet == class)
            break;
    if (first == SPECIAL_PM) {
        impossible("mkclass found no class %d monsters", class);
        return (struct permonst *) 0;
    }

    /*  Assumption #2:  monsters of a given class are presented in ascending
     *                  order of strength.
     */
    for (last = first; last < SPECIAL_PM && mons[last].mlet == class; last++) {
        if (atyp != A_NONE && sgn(mons[last].maligntyp) != sgn(atyp))
            continue;
        if (mk_gen_ok(last, G_GONE, mask)) {
            /* consider it; don't reject a toostrong() monster if we
               don't have anything yet (num==0) or if it is the same
               (or lower) difficulty as preceding candidate (non-zero
               'num' implies last > first so mons[last-1] is safe);
               sometimes accept it even if high difficulty */
            if (num && toostrong(last, maxmlev)
                && mons[last].difficulty > mons[last - 1].difficulty
                && rn2(2))
                break;
            if ((k = (is_mplayer(&mons[last]) ? 1 : 5)
                * (mons[last].geno & G_FREQ)) > 0) {
                /* skew towards lower value monsters at lower exp. levels
                   (this used to be done in the next loop, but that didn't
                   work well when multiple species had the same level and
                   were followed by one that was past the bias threshold;
                   cited example was sucubus and incubus, where the bias
                   against picking the next demon resulted in incubus
                   being picked nearly twice as often as sucubus);
                   we need the '+1' in case the entire set is too high
                   level (really low level hero) */
                nums[last] = k + 1 - (adj_lev(&mons[last]) > (u.ulevel * 2));
                num += nums[last];
            }
        }
    }
    if (!num)
        return (struct permonst *) 0;

    /* the hard work has already been done; 'num' should hit 0 before
       first reaches last (which is actually one past our last candidate) */
    for (num = rnd(num); first < last; first++)
        if ((num -= nums[first]) <= 0)
            break;

    return nums[first] ? &mons[first] : (struct permonst *) 0;
}

/* like mkclass(), but excludes difficulty considerations; used when
   player with polycontrol picks a class instead of a specific type;
   genocided types are avoided but extinct ones are acceptable; we don't
   check polyok() here--caller accepts some choices !polyok() would reject */
int
mkclass_poly(class)
int class;
{
    register int first, last, num = 0;

    for (first = LOW_PM; first < SPECIAL_PM; first++)
        if (mons[first].mlet == class)
            break;
    if (first == SPECIAL_PM)
        return NON_PM;

    for (last = first; last < SPECIAL_PM && mons[last].mlet == class; last++)
        if (mk_gen_ok(last, G_GENOD, (G_NOGEN | G_UNIQ)))
            num += mons[last].geno & G_FREQ;
    if (!num)
        return NON_PM;

    for (num = rnd(num); num > 0; first++)
        if (mk_gen_ok(first, G_GENOD, (G_NOGEN | G_UNIQ)))
            num -= mons[first].geno & G_FREQ;
    first--; /* correct an off-by-one error */

    return first;
}

/* adjust strength of monsters based on u.uz and u.ulevel */
int
adj_lev(ptr)
register struct permonst *ptr;
{
    int tmp, tmp2;

    if (ptr == &mons[PM_WIZARD_OF_YENDOR]) {
        /* does not depend on other strengths, but does get stronger
         * every time he is killed
         */
        tmp = ptr->mlevel + mvitals[PM_WIZARD_OF_YENDOR].died;
        if (tmp > 49)
            tmp = 49;
        return tmp;
    }

    if (is_mplayer(ptr)) {
	tmp = rn1(5, u.ulevel - 2);
	if (tmp < 1) tmp = 1;
	if (tmp > 30) tmp = 30;
	return tmp;
    }

    if ((tmp = ptr->mlevel) > 49)
        return 50; /* "special" demons/devils */
    tmp2 = (level_difficulty() - tmp);
    if (tmp2 < 0)
        tmp--; /* if mlevel > u.uz decrement tmp */
    else
        tmp += (tmp2 / 5); /* else increment 1 per five diff */

    tmp2 = (u.ulevel - ptr->mlevel); /* adjust vs. the player */
    if (tmp2 > 0)
        tmp += (tmp2 / 4); /* level as well */

    tmp2 = (3 * ((int) ptr->mlevel)) / 2; /* crude upper limit */
    if (tmp2 > 49)
        tmp2 = 49;                                      /* hard upper limit */
    return ((tmp > tmp2) ? tmp2 : (tmp > 0 ? tmp : 0)); /* 0 lower limit */
}

/* monster earned experience and will gain some hit points; it might also
   grow into a bigger monster (baby to adult, soldier to officer, etc) */
struct permonst *
grow_up(mtmp, victim)
struct monst *mtmp, *victim;
{
    int oldtype, newtype, max_increase, cur_increase, lev_limit, hp_threshold;
    unsigned fem;
    struct permonst *ptr = mtmp->data;

    /* monster died after killing enemy but before calling this function */
    /* currently possible if killing a gas spore */
    if (DEADMONSTER(mtmp))
        return (struct permonst *) 0;

    /* note:  none of the monsters with special hit point calculations
       have both little and big forms */
    oldtype = monsndx(ptr);
    newtype = little_to_big(oldtype);
    if (newtype == PM_PRIEST && mtmp->female)
        newtype = PM_PRIESTESS;

    /* growth limits differ depending on method of advancement */
    if (victim) {                       /* killed a monster */
        /*
         * The HP threshold is the maximum number of hit points for the
         * current level; once exceeded, a level will be gained.
         * Possible bug: if somehow the hit points are already higher
         * than that, monster will gain a level without any increase in HP.
         */
        hp_threshold = mtmp->m_lev * 8; /* normal limit */
        if (!mtmp->m_lev)
            hp_threshold = 4;
        else if (is_golem(ptr)) /* strange creatures */
            hp_threshold = ((mtmp->mhpmax / 10) + 1) * 10 - 1;
        else if (is_home_elemental(ptr))
            hp_threshold *= 3;
        lev_limit = 3 * (int) ptr->mlevel / 2; /* same as adj_lev() */
        /* If they can grow up, be sure the level is high enough for that */
        if (oldtype != newtype && mons[newtype].mlevel > lev_limit)
            lev_limit = (int) mons[newtype].mlevel;
        /* number of hit points to gain; unlike for the player, we put
           the limit at the bottom of the next level rather than the top */
        max_increase = rnd((int) victim->m_lev + 1);
        if (mtmp->mhpmax + max_increase > hp_threshold + 1)
            max_increase = max((hp_threshold + 1) - mtmp->mhpmax, 0);
        cur_increase = (max_increase > 1) ? rn2(max_increase) : 0;
    } else {
        /* a gain level potion or wraith corpse; always go up a level
           unless already at maximum (49 is hard upper limit except
           for demon lords, who start at 50 and can't go any higher) */
        max_increase = cur_increase = rnd(8);
        hp_threshold = 0; /* smaller than `mhpmax + max_increase' */
        lev_limit = 50;   /* recalc below */
    }

    mtmp->mhpmax += max_increase;
    mtmp->mhp += cur_increase;
    if (mtmp->mhpmax <= hp_threshold)
        return ptr; /* doesn't gain a level */

    if (is_mplayer(ptr))
        lev_limit = 30; /* same as player */
    else if (lev_limit < 5)
        lev_limit = 5; /* arbitrary */
    else if (lev_limit > 49)
        lev_limit = (ptr->mlevel > 49 ? 50 : 49);

    if ((int) ++mtmp->m_lev >= mons[newtype].mlevel && newtype != oldtype) {
        ptr = &mons[newtype];
        /* new form might force gender change */
        fem = is_male(ptr) ? 0 : is_female(ptr) ? 1 : mtmp->female;

        if (mvitals[newtype].mvflags & G_GENOD) { /* allow G_EXTINCT */
            if (canspotmon(mtmp))
                pline("As %s grows up into %s, %s %s!", mon_nam(mtmp),
                      an(ptr->mname), mhe(mtmp),
                      nonliving(ptr) ? "expires" : "dies");
            set_mon_data(mtmp, ptr); /* keep mvitals[] accurate */
            mondied(mtmp);
            return (struct permonst *) 0;
        } else if (canspotmon(mtmp)) {
            char buf[BUFSZ];

            /* 3.6.1:
             * Temporary (?) hack to fix growing into opposite gender.
             */
            Sprintf(buf, "%s%s",
                    /* deal with female gnome becoming a gnome lord */
                    (mtmp->female && !fem) ? "male "
                        /* or a male gnome becoming a gnome lady
                           (can't happen with 3.6.0 mons[], but perhaps
                           slightly less sexist if prepared for it...) */
                      : (fem && !mtmp->female) ? "female " : "",
                    ptr->mname);
            pline("%s %s %s.", upstart(y_monnam(mtmp)),
                  (fem != mtmp->female) ? "changes into"
                                        : humanoid(ptr) ? "becomes"
                                                        : "grows up into",
                  an(buf));
        }
        set_mon_data(mtmp, ptr);
        newsym(mtmp->mx, mtmp->my);    /* color may change */
        lev_limit = (int) mtmp->m_lev; /* never undo increment */

        mtmp->female = fem; /* gender might be changing */
    }

    /* sanity checks */
    if ((int) mtmp->m_lev > lev_limit) {
        mtmp->m_lev--; /* undo increment */
        /* HP might have been allowed to grow when it shouldn't */
        if (mtmp->mhpmax == hp_threshold + 1)
            mtmp->mhpmax--;
    }
    if (mtmp->mhpmax > 50 * 8)
        mtmp->mhpmax = 50 * 8; /* absolute limit */
    if (mtmp->mhp > mtmp->mhpmax)
        mtmp->mhp = mtmp->mhpmax;

    return ptr;
}

int
mongets(mtmp, otyp)
register struct monst *mtmp;
int otyp;
{
    register struct obj *otmp;
    int tryct;
    int spe;

    if (!otyp)
        return 0;
    otmp = mksobj(otyp, TRUE, FALSE);
    if (otmp) {
        if (mtmp->data->mlet == S_DEMON) {
            /* demons never get blessed objects */
            if (otmp->blessed)
                curse(otmp);
        } else if (is_lminion(mtmp)) {
            /* lawful minions don't get cursed, bad, or rusting objects */
            otmp->cursed = FALSE;
            if (otmp->spe < 0)
                otmp->spe = 0;
            otmp->oerodeproof = TRUE;
        } else if (is_mplayer(mtmp->data) && is_sword(otmp)) {
            otmp->spe = (3 + rn2(4));
        }

        if (otmp->otyp == CANDELABRUM_OF_INVOCATION) {
            otmp->spe = 0;
            otmp->age = 0L;
            otmp->lamplit = FALSE;
            otmp->blessed = otmp->cursed = FALSE;
        } else if (otmp->otyp == BELL_OF_OPENING) {
            otmp->blessed = otmp->cursed = FALSE;
        } else if (otmp->otyp == SPE_BOOK_OF_THE_DEAD) {
            otmp->blessed = FALSE;
            otmp->cursed = TRUE;
        }

        /* leaders don't tolerate inferior quality battle gear
         * in fact, they don't settle for non-enchanted gear period */
        if (is_prince(mtmp->data) || is_lord(mtmp->data)) {
            if (otmp->oclass == WEAPON_CLASS && otmp->spe < 1)
                otmp->spe = rn2(3) + 1;
            else if (otmp->oclass == ARMOR_CLASS && otmp->spe < 1)
                otmp->spe = rn2(4) + 1;
        }

        /* Medusa's bow and arrows are also high quality */
        if (mtmp->data == &mons[PM_MEDUSA]) {
            if (otmp->oclass == WEAPON_CLASS && otmp->spe < 7)
                otmp->spe = rn2(3) + 3;
            otmp->blessed = TRUE;
            otmp->oerodeproof = TRUE;
        }

        /* if mtmp would hate the material of the object they're getting,
         * rerandomize it */
        tryct = 0;
        if (otmp->oclass == WEAPON_CLASS
            || is_weptool(otmp) || otmp->oclass == ARMOR_CLASS
            || otmp->oclass == AMULET_CLASS) {
            while (mon_hates_material(mtmp, otmp->material)) {
                init_obj_material(otmp);
                tryct++;
                if (tryct >= 100) {
                    /* will anything work? */
                    int mat;
                    for (mat = 1; mat < NUM_MATERIAL_TYPES; ++mat) {
                        if (valid_obj_material(otmp, mat)
                            && !mon_hates_material(mtmp, mat)) {
                            set_material(otmp, mat);
                            break;
                        }
                    }
                    if (mat == NUM_MATERIAL_TYPES) {
                        impossible("mon %d doesn't like any materials for obj %d",
                                   monsndx(mtmp->data), otmp->otyp);
                        set_material(otmp, objects[otmp->otyp].oc_material);
                    }
                    break;
                }
            }
        }

        spe = otmp->spe;
        if (mpickobj(mtmp, otmp)) {
            /* otmp was freed via merging with something else */
            otmp = (struct obj *) 0;
        }
        return spe;
    }
    return 0;
}

int
golemhp(type)
int type;
{
    switch (type) {
    case PM_STRAW_GOLEM:
        return 20;
    case PM_PAPER_GOLEM:
        return 20;
    case PM_ROPE_GOLEM:
        return 30;
    case PM_LEATHER_GOLEM:
        return 40;
    case PM_GOLD_GOLEM:
        return 40;
    case PM_WOOD_GOLEM:
        return 50;
    case PM_FLESH_GOLEM:
        return 40;
    case PM_CLAY_GOLEM:
        return 50;
    case PM_STONE_GOLEM:
        return 60;
    case PM_GLASS_GOLEM:
        return 60;
    case PM_IRON_GOLEM:
        return 80;
    default:
        return 0;
    }
}

/*
 *      Alignment vs. yours determines monster's attitude to you.
 *      (Some "animal" types are co-aligned, but also hungry.)
 */
boolean
peace_minded(ptr)
register struct permonst *ptr;
{
    aligntyp mal = ptr->maligntyp, ual = u.ualign.type;

    if (always_peaceful(ptr))
        return TRUE;
    if (always_hostile(ptr))
        return FALSE;
    if (ptr->msound == MS_LEADER || ptr->msound == MS_GUARDIAN)
        return TRUE;
    if (ptr->msound == MS_NEMESIS)
        return FALSE;

    if (race_peaceful(ptr))
        return TRUE;
    if (race_hostile(ptr))
        return FALSE;

    /* the monster is hostile if its alignment is different from the
     * player's */
    if (sgn(mal) != sgn(ual))
        return FALSE;

    /* Negative monster hostile to player with Amulet. */
    if (mal < A_NEUTRAL && u.uhave.amulet)
        return FALSE;

    /* minions are hostile to players that have strayed at all */
    if (is_minion(ptr))
        return (boolean) (u.ualign.record >= 0);

    /* Last case:  a chance of a co-aligned monster being
     * hostile.  This chance is greater if the player has strayed
     * (u.ualign.record negative) or the monster is not strongly aligned.
     */
    return (boolean) (!!rn2(16 + (u.ualign.record < -15 ? -15
                                                        : u.ualign.record))
                      && !!rn2(2 + abs(mal)));
}

/* Set malign to have the proper effect on player alignment if monster is
 * killed.  Negative numbers mean it's bad to kill this monster; positive
 * numbers mean it's good.  Since there are more hostile monsters than
 * peaceful monsters, the penalty for killing a peaceful monster should be
 * greater than the bonus for killing a hostile monster to maintain balance.
 * Rules:
 *   it's bad to kill peaceful monsters, potentially worse to kill always-
 *      peaceful monsters;
 *   it's never bad to kill a hostile monster, although it may not be good.
 */
void
set_malign(mtmp)
struct monst *mtmp;
{
    schar mal = mtmp->data->maligntyp;
    boolean coaligned;

    if (mtmp->ispriest || mtmp->isminion) {
        /* some monsters have individual alignments; check them */
        if (mtmp->ispriest && EPRI(mtmp))
            mal = EPRI(mtmp)->shralign;
        else if (mtmp->isminion && EMIN(mtmp))
            mal = EMIN(mtmp)->min_align;
        /* unless alignment is none, set mal to -5,0,5 */
        /* (see align.h for valid aligntyp values)     */
        if (mal != A_NONE)
            mal *= 5;
    }

    coaligned = (sgn(mal) == sgn(u.ualign.type));
    if (mtmp->data->msound == MS_LEADER) {
        mtmp->malign = -20;
    } else if (mal == A_NONE) {
        if (mtmp->mpeaceful)
            mtmp->malign = 0;
        else
            mtmp->malign = 20; /* really hostile */
    } else if (always_peaceful(mtmp->data)) {
        int absmal = abs(mal);
        if (mtmp->mpeaceful)
            mtmp->malign = -3 * max(5, absmal);
        else
            mtmp->malign = 3 * max(5, absmal); /* renegade */
    } else if (always_hostile(mtmp->data)) {
        int absmal = abs(mal);
        if (coaligned)
            mtmp->malign = 0;
        else
            mtmp->malign = max(5, absmal);
    } else if (coaligned) {
        int absmal = abs(mal);
        if (mtmp->mpeaceful)
            mtmp->malign = -3 * max(3, absmal);
        else /* renegade */
            mtmp->malign = max(3, absmal);
    } else /* not coaligned and therefore hostile */
        mtmp->malign = abs(mal);
}

/* allocate a new mcorpsenm field for a monster; only need mextra itself */
void
newmcorpsenm(mtmp)
struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    MCORPSENM(mtmp) = NON_PM; /* not initialized yet */
}

/* release monster's mcorpsenm field; basically a no-op */
void
freemcorpsenm(mtmp)
struct monst *mtmp;
{
    if (has_mcorpsenm(mtmp))
        MCORPSENM(mtmp) = NON_PM;
}

static NEARDATA char syms[] = {
    MAXOCLASSES,  MAXOCLASSES + 1, RING_CLASS,   WAND_CLASS,   WEAPON_CLASS,
    FOOD_CLASS,   COIN_CLASS,      SCROLL_CLASS, POTION_CLASS, ARMOR_CLASS,
    AMULET_CLASS, TOOL_CLASS,      ROCK_CLASS,   GEM_CLASS,    SPBOOK_CLASS,
    S_MIMIC_DEF,  S_MIMIC_DEF,
};

void
set_mimic_sym(mtmp)
register struct monst *mtmp;
{
    int typ, roomno, rt;
    unsigned appear, ap_type;
    int s_sym;
    struct obj *otmp;
    int mx, my;

    if (!mtmp || Protection_from_shape_changers)
        return;
    mx = mtmp->mx;
    my = mtmp->my;
    typ = levl[mx][my].typ;
    /* only valid for INSIDE of room */
    roomno = levl[mx][my].roomno - ROOMOFFSET;
    if (roomno >= 0)
        rt = rooms[roomno].rtype;
#ifdef SPECIALIZATION
    else if (IS_ROOM(typ))
        rt = OROOM, roomno = 0;
#endif
    else
        rt = 0; /* roomno < 0 case for GCC_WARN */

    if (OBJ_AT(mx, my)) {
        ap_type = M_AP_OBJECT;
        appear = level.objects[mx][my]->otyp;
    } else if (IS_DOOR(typ) || IS_WALL(typ) || typ == SDOOR || typ == SCORR) {
        ap_type = M_AP_FURNITURE;
        /*
         *  If there is a wall to the left that connects to this
         *  location, then the mimic mimics a horizontal closed door.
         *  This does not allow doors to be in corners of rooms.
         *  Since rogue has no closed doors, mimic a wall there
         *  (yes, mimics can end up on this level by various means).
         */
        if (mx != 0 && (levl[mx - 1][my].typ == HWALL
                        || levl[mx - 1][my].typ == TLCORNER
                        || levl[mx - 1][my].typ == TRWALL
                        || levl[mx - 1][my].typ == BLCORNER
                        || levl[mx - 1][my].typ == TDWALL
                        || levl[mx - 1][my].typ == CROSSWALL
                        || levl[mx - 1][my].typ == TUWALL))
            appear = Is_rogue_level(&u.uz) ? S_hwall : S_hcdoor;
        else
            appear = Is_rogue_level(&u.uz) ? S_vwall : S_vcdoor;
    } else if (level.flags.is_maze_lev && !In_sokoban(&u.uz) && rn2(2)) {
        ap_type = M_AP_OBJECT;
        appear = STATUE;
    } else if (roomno < 0 && !t_at(mx, my)) {
        ap_type = M_AP_OBJECT;
        appear = BOULDER;
    } else if (rt == ZOO || rt == VAULT) {
        ap_type = M_AP_OBJECT;
        appear = GOLD_PIECE;
    } else if (rt == DELPHI) {
        if (rn2(2)) {
            ap_type = M_AP_OBJECT;
            appear = STATUE;
        } else {
            ap_type = M_AP_FURNITURE;
            appear = S_fountain;
        }
    } else if (rt == TEMPLE) {
        ap_type = M_AP_FURNITURE;
        appear = S_altar;
        /*
         * We won't bother with beehives, morgues, barracks, throne rooms
         * since they shouldn't contain too many mimics anyway...
         */
    } else if (rt >= SHOPBASE) {
        s_sym = get_shop_item(rt - SHOPBASE);
        if (s_sym < 0) {
            ap_type = M_AP_OBJECT;
            appear = -s_sym;
        } else {
            if (s_sym == RANDOM_CLASS)
                s_sym = syms[rn2((int) sizeof(syms) - 2) + 2];
            goto assign_sym;
        }
    } else {
        s_sym = syms[rn2((int) sizeof(syms))];
 assign_sym:
        if (s_sym == MAXOCLASSES || s_sym == MAXOCLASSES + 1) {
            ap_type = M_AP_FURNITURE;
            appear = (s_sym == MAXOCLASSES) ? S_upstair : S_dnstair;
        } else {
            ap_type = M_AP_OBJECT;
            if (s_sym == S_MIMIC_DEF) {
                appear = STRANGE_OBJECT;
            } else if (s_sym == COIN_CLASS) {
                appear = GOLD_PIECE;
            } else {
                otmp = mkobj((char) s_sym, FALSE);
                appear = otmp->otyp;
                /* make sure container contents are free'ed */
                obfree(otmp, (struct obj *) 0);
            }
        }
    }
    mtmp->m_ap_type = ap_type;
    mtmp->mappearance = appear;
    /* when appearing as an object based on a monster type, pick a shape */
    if (ap_type == M_AP_OBJECT
        && (appear == STATUE || appear == FIGURINE
            || appear == CORPSE || appear == EGG || appear == TIN)) {
        int mndx = rndmonnum(),
            nocorpse_ndx = (mvitals[mndx].mvflags & G_NOCORPSE) != 0;

        if (appear == CORPSE && nocorpse_ndx)
            mndx = rn1(PM_WIZARD - PM_ARCHEOLOGIST + 1, PM_ARCHEOLOGIST);
        else if ((appear == EGG && !can_be_hatched(mndx))
                 || (appear == TIN && nocorpse_ndx))
            mndx = NON_PM; /* revert to generic egg or empty tin */

        newmcorpsenm(mtmp);
        MCORPSENM(mtmp) = mndx;
    } else if (ap_type == M_AP_OBJECT && appear == SLIME_MOLD) {
        newmcorpsenm(mtmp);
        MCORPSENM(mtmp) = context.current_fruit;
        /* if no objects of this fruit type have been created yet,
           context.current_fruit is available for re-use when the player
           assigns a new fruit name; override that--having a mimic as the
           current_fruit is equivalent to creating an instance of that
           fruit (no-op if a fruit of this type has actually been made) */
        flags.made_fruit = TRUE;
    } else if (has_mcorpsenm(mtmp)) {
        /* don't retain stale value from a previously mimicked shape */
        MCORPSENM(mtmp) = NON_PM;
    }

    if (does_block(mx, my, &levl[mx][my]))
        block_point(mx, my);
}

/* release monster from bag of tricks; return number of monsters created */
int
bagotricks(bag, tipping, seencount)
struct obj *bag;
boolean tipping; /* caller emptying entire contents; affects shop handling */
int *seencount;  /* secondary output */
{
    int moncount = 0;

    if (!bag || bag->otyp != BAG_OF_TRICKS) {
        impossible("bad bag o' tricks");
    } else if (bag->spe < 1) {
        /* if tipping known empty bag, give normal empty container message */
        pline1((tipping && bag->cknown) ? "It's empty." : nothing_happens);
        /* now known to be empty if sufficiently discovered */
        if (bag->dknown && objects[bag->otyp].oc_name_known)
            bag->cknown = 1;
    } else {
        struct monst *mtmp;
        int creatcnt = 1, seecount = 0;

        consume_obj_charge(bag, !tipping);

        if (!rn2(23))
            creatcnt += rnd(7);
        do {
            mtmp = makemon((struct permonst *) 0, u.ux, u.uy, NO_MM_FLAGS);
            if (mtmp) {
                ++moncount;
                if (canspotmon(mtmp))
                    ++seecount;
            }
        } while (--creatcnt > 0);
        if (seecount) {
            if (seencount)
                *seencount += seecount;
            if (bag->dknown)
                makeknown(BAG_OF_TRICKS);
        } else if (!tipping) {
            pline1(!moncount ? nothing_happens : "Nothing seems to happen.");
        }
    }
    return moncount;
}

/*makemon.c*/
