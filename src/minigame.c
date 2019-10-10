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

#endif /* MINIGAME */
/* minigame.c */
