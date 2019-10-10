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
        (void) add_to_container(obj, otmp);
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
        if (!mon) {
            You("turn %s face-down.", yname(obj));
            addinv(otmp);
        } else {
            mpickobj(mon, otmp);
        }
    }
    obj_extract_self(obj);
    (void) add_to_container(otmp, obj);
    if (!mon)
        prinv("", otmp, 1);
}


#endif /* MINIGAME */
/* minigame.c */
