/* minigame.c */

#include "hack.h"
#ifdef MINIGAME
#include "minigame.h"

void
newegam(mtmp)
struct monst *mtmp;
{
    if (!mtmp->mextra)
        mtmp->mextra = newmextra();
    if (!EGAM(mtmp)) {
        EGAM(mtmp) = (struct egam *) alloc(sizeof (struct egam));
        (void) memset((genericptr_t) EGAM(mtmp),
                      0, sizeof (struct egam));
    }
}

void
free_egam(mtmp)
struct monst *mtmp;
{
    if (mtmp->mextra && EGAM(mtmp)) {
        free((genericptr_t) EGAM(mtmp));
        EGAM(mtmp) = (struct egam *) 0;
    }
}


int
use_dice(obj)
struct obj *obj;
{
    int result[] = {0, 0};
    struct obj *otmp = obj;
    if (obj->quan > 1L)
        otmp = splitobj(obj, 1L);
    You("roll %s.", yname(otmp));
    if (otmp->where == OBJ_INVENT)
        dropx(otmp);
    if (!is_lava(u.ux, u.uy) && !is_pool(u.ux, u.uy)) {
        if (otmp->otyp == LOADED_DICE && !(u.uluck > 0 && rn2(u.uluck) >= 7)) {
            /* A very lucky player can roll loaded dice as if they were fair */
            result[0] = rn2(2) ? 1 : 2 + rn2(5);
            result[1] = rn2(2) ? 1 : 2 + rn2(5);
        } else if ((otmp->cursed || u.uluck <= -7) && !rn2(7)) {
            /* Ordinarily, chance of rolling snake eyes is 1 in 36.
               This modifies the chance to 6 in 36.
                (1/7) + (6/7)*(1/36) = 6/36
            */
            result[0] = 1;
            result[1] = 1;
        } else {
            result[0] = rnd(6);
            result[1] = rnd(6);
        }
        if (!Blind)
            You("roll a %d and a %d (%d).",
                result[0], result[1], result[0] + result[1]);
        else
            pline("But you don't see the result.");
        return result[0] + result[1];
    } else {
        return 0;
    }
}

void
builddeck(obj)
struct obj *obj;
{
    struct obj *otmp;
    for (int spe = 0; spe < (NUM_CARD_SUITS * NUM_CARD_VALUES); spe++) {
        otmp = mksobj(PLAYING_CARD, FALSE, FALSE);
        otmp->spe = spe;
        otmp->cursed = obj->cursed;
        otmp->blessed = obj->blessed;
        add_to_container(obj, otmp);
    }
}

void
drawcard(obj)
struct obj *obj;
{
    int cidx, loadlev;
    long itemcount = count_contents(obj, FALSE, FALSE, TRUE);
    struct obj *otmp;
    if (obj->otyp != DECK_OF_CARDS) {
        impossible("Trying to draw a card from %s?", xname(obj));
        return;
    }

    You("draw a card.");
    cidx = rn2((int) itemcount);
    for (otmp = obj->cobj; cidx > 0; cidx--) {
        otmp = otmp->nobj;
        if (!otmp) {
            impossible("Trying to draw a card from an empty deck?");
            return;
        }
    }
    obj_extract_self(otmp);
    obj->owt = weight(obj);
    addinv(otmp);
    loadlev = near_capacity();
    prinv(loadlev ? ((loadlev < MOD_ENCUMBER)
                        ? "You have a little trouble taking"
                        : "You have much trouble taking")
                  : (char *) 0,
          otmp, 1);
    if (itemcount == 1L)
        useup(obj);
}

void
returncard(obj, mon)
struct obj *obj;
struct monst *mon;
{
    if (obj->otyp != PLAYING_CARD) {
        impossible("Trying to put non-card %s into the deck?", xname(obj));
        return;
    }
    struct obj *otmp;
    if (!mon)
        otmp = carrying(DECK_OF_CARDS);
    else
        otmp = m_carrying(mon, DECK_OF_CARDS);
    if (otmp) {
        if (!mon)
            You("shuffle %s into %s.",
                the(xname(obj)), yname(otmp));
    } else {
        otmp = mksobj(DECK_OF_CARDS, FALSE, FALSE);
        otmp->cknown = 1;
        otmp->blessed = obj->blessed;
        otmp->cursed = obj->cursed;
        otmp->bknown = obj->bknown;
        if (!mon) {
            You("turn %s face-down.", yname(obj));
            addinv(otmp);
        } else {
            mpickobj(mon, otmp);
        }
    }
    obj_extract_self(obj);
    add_to_container(otmp, obj);
    if (!mon)
        prinv("", otmp, 1);
}

void
mergedecks(odest, osrc)
struct obj *odest;
struct obj **osrc;
{
    You("shuffle %s and %s together.", yname(*osrc), yname(odest));
    struct obj *otmp, *otmp2;
    for (otmp = (*osrc)->cobj; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        obj_extract_self(otmp);
        add_to_container(odest, otmp);
        otmp->blessed = odest->blessed;
        otmp->cursed = odest->cursed;
    }
    odest->owt = weight(odest);
    useupf(*osrc, 1);
    *osrc = odest;
}

int
play_highroll(mon)
struct monst* mon;
{
#define game_stage      (EGAM(mon)->state[0])
#define wager           (EGAM(mon)->state[1])
#define player_roll     (EGAM(mon)->state[2])
#define games_played    (EGAM(mon)->state[3])
#define odice           (EGAM(mon)->otmp)
    /*  Game stage:
        -  0 = waiting to play
        -  1 = waiting for player to roll
        - -1 = refuse to play
    */
    if (!has_egam(mon)) {
        impossible("Playing game with non-gaming monster?");
        return 0;
    }
    switch (EGAM(mon)->state[0]) {
    case -1:
        pline("%s isn't interested.", Monnam(mon));
    case 0: {
        char buf[BUFSZ] = DUMMY;
        long umoney = money_cnt(invent);
        if (!(odice && odice->where == OBJ_MINVENT)) {
            pline("\"Blast, my dice are missing!\"");
            return 0;
        }
        switch(ynq("\"Willst thou play a game?\"")) {
        case 'y':
            if (!games_played++)
                pline("\"%s we are playing high-roll.\"",
                      night() ? "Tonight" : "Today");
            getlin("\"How much willst thou wager?", buf);
            if (sscanf(buf, "%d", &wager) != 1)
                wager = 0;
            if (wager > 0) {
                game_stage = 1;
                wager = wager < umoney ? wager : umoney;
                if (wager == umoney)
                    pline("You give %s all your gold.", Monnam(mon));
                else
                    pline("You give %s %d %s.",
                          Monnam(mon), wager, currency(wager));
                money2mon(mon, wager);
                context.botl = 1;
                pline("%s gives you %s.", Monnam(mon), yname(odice));
                obj_extract_self(odice);
                addinv(odice);
                prinv("", odice, 1);
            }
            break;
        default:
            break;
        }
        break;
    }
    case 1: {
        struct obj *otmp;
        struct obj *mongold = findgold(mon->minvent, TRUE);
        if (!(carrying(FAIR_DICE) || carrying(LOADED_DICE))) {
            pline("\"Where art thy dice?\"");
            return 0;
        }
        /* TODO: which dice will you use? */
        otmp = odice;
        switch(ynq("\"Art thou ready?\"")) {
        case 'y':
            player_roll = use_dice(otmp);
            break;
        default:
            return 0;
        }
        if (player_roll == 0) { /* Dice are lost */
            pline("\"My dice!\"");
            pline("%s gets angry!", Monnam(mon));
            odice = (struct obj *) 0;
            mon->mpeaceful = 0;
            set_malign(mon);
            game_stage = -1;
            break;
        }
        switch(yn("\"Thou mayest roll once more; willst thou do so?\"")) {
        case 'y':
            player_roll = use_dice(otmp);
            break;
        default:
            break;
        }
        odice = otmp;
        pline("%s snatches up %s.", Monnam(mon), yname(odice));
        obj_extract_self(odice);
        mpickobj(mon, odice);
        /* Dice are only unfair for the player... */
        int mresult[2];
        mresult[0] = rnd(6);
        mresult[1] = rnd(6);
        pline("%s rolls a %d and a %d (%d).", Monnam(mon),
              mresult[0], mresult[1], mresult[0] + mresult[1]);
        if (mresult[0] + mresult[1] >= player_roll) {
            pline("\"I win!\"");
            game_stage = 2;
            return -1;
        } else {
            pline("\"Blast! Thou hast won!\"");
            if (wager >= mongold->quan) {
                wager = mongold->quan;
                pline("\"Thou hast taken everything from me...\"");
                game_stage = -1;
            } else {
                game_stage = 2;
            }
            money2u(mon, wager);
            return 1;
        }
        break;
    }
    default:
        break;
    }
    return 0;
}

#endif /* MINIGAME */
/* minigame.c */
