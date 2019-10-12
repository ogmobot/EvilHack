/* minigame.c */

#include "hack.h"
#ifdef MINIGAME
#include "minigame.h"

STATIC_DCL int FDECL(greet_and_wager, (struct monst *, char *, long *));

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
    if (is_lava(u.ux, u.uy) || is_pool(u.ux, u.uy)
        || !can_reach_floor(FALSE) || nohands(youmonst.data)) {
        if (otmp->where == OBJ_INVENT)
            dropx(otmp);
    }
    if (!is_lava(u.ux, u.uy) && !is_pool(u.ux, u.uy)) {
        if (otmp->otyp == LOADED_DICE
            && !(u.uluck > 0 && rn2(u.uluck) >= 7)
            && !otmp->blessed) {
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

struct obj *
drawcard(obj, mon)
struct obj *obj;
struct monst *mon;
{
    int cidx, loadlev;
    long itemcount = count_contents(obj, FALSE, TRUE, TRUE);
    struct obj *otmp;
    if (obj->otyp != DECK_OF_CARDS) {
        impossible("Trying to draw a card from %s?", xname(obj));
        return 0;
    }

    cidx = rn2((int) itemcount);
    for (otmp = obj->cobj; cidx > otmp->quan; cidx -= (otmp->quan)) {
        otmp = otmp->nobj;
        if (!otmp) {
            impossible("Trying to draw a card from an empty deck?");
            return 0;
        }
    }
    if (otmp->quan > 1L)
        otmp = splitobj(otmp, 1);
    obj_extract_self(otmp);
    obj->owt = weight(obj);
    if (!mon) {
        addinv(otmp);
        loadlev = near_capacity();
        prinv(loadlev ? ((loadlev < MOD_ENCUMBER)
                            ? "You have a little trouble taking"
                            : "You have much trouble taking")
                      : (char *) 0,
              otmp, 1);
    } else {
        if (!Blind && cansee(mon->mx, mon->my))
            pline("%s draws %s.", Monnam(mon), yname(otmp));
        mpickobj(mon, otmp);
    }
    if (itemcount == 1L) {
        if (obj->where == OBJ_INVENT)
            useup(obj);
        else if (obj->where == OBJ_MINVENT)
            m_useup(obj->ocarry, obj);
        else
            useupf(obj, 1);
    }
    return otmp;
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
            You("turn your card%s face-down.", obj->quan == 1L ? "" : "s");
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

STATIC_OVL int
greet_and_wager(mon, game, wager)
struct monst *mon;
char *game;
long *wager;
{
    /* If game is not supplied, assume the hero already knows what we're
       playing. Otherwise, greet them and introduce the game.
    */
    char buf[BUFSZ] = DUMMY;
    long umoney = money_cnt(invent);
    if (game) {
        pline("\"%s, %s!\"", Hello(mon),
              is_demon(mon->data) ? (is_demon(youmonst.data)
                                      ? (flags.female ? "sister" : "brother" )
                                      : "mortal")
                                  : "adventurer");
    }
    switch(ynq("\"Willst thou play a game?\"")) {
    case 'y':
        if (game)
            pline("\"%s we are playing %s.\"",
                  night() ? "Tonight" : "Today", game);
        getlin("\"How much willst thou wager?\"", buf);
        if (sscanf(buf, "%ld", wager) != 1)
            *wager = 0;
        if (*wager > 0) {
            *wager = *wager < umoney ? *wager : umoney;
            if (*wager == umoney)
                pline("You give %s all your gold.", mon_nam(mon));
            else
                pline("You give %s %ld %s.",
                      mon_nam(mon), *wager, currency(*wager));
            money2mon(mon, *wager);
            context.botl = 1;
            return 1;
        }
        /* FALLTHRU */
    default:
        return 0;
    }
}

static NEARDATA const char valid_dice[] = { TOOL_CLASS, 0 };

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
    switch (game_stage) {
    case -1:
        pline("%s isn't interested.", Monnam(mon));
        break;
    case 0:
        if (!(odice && odice->where == OBJ_MINVENT && odice->ocarry == mon)) {
            /* Try to find a suitable pair... */
            odice = m_carrying(mon, LOADED_DICE);
            if (!odice)
                odice = m_carrying(mon, FAIR_DICE);
            if (!odice) {
                pline("\"Blast, my dice are missing!\"");
                return 0;
            }
        }
        if (greet_and_wager(mon, games_played ? 0 : "high-roll", &wager)) {
            game_stage = 1;
            pline("%s gives you %s %s.", Monnam(mon), mhis(mon), xname(odice));
            obj_extract_self(odice);
            addinv(odice);
            prinv("", odice, 1);
            games_played++;
        }
        break;
    case 1: {
        struct obj *mongold = findgold(mon->minvent, TRUE);
        if (!(carrying(FAIR_DICE) || carrying(LOADED_DICE))) {
            pline("\"Where art thy dice?\"");
            return 0;
        }
        switch(ynq("\"Art thou ready?\"")) {
        case 'y':
            odice = getobj(valid_dice, "use");
            if (!odice || odice == &zeroobj
                || (odice->otyp != LOADED_DICE && odice->otyp != FAIR_DICE))
                return 0;
            player_roll = use_dice(odice);
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
            player_roll = use_dice(odice);
            break;
        default:
            break;
        }
        pline("%s snatches up %s.", Monnam(mon), ysimple_name(odice));
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
            game_stage = 0;
            return -1;
        } else {
            pline("\"Blast! Thou hast won!\"");
            pline("%s gives you your winnings.", Monnam(mon));
            wager *= 2;
            if (wager >= mongold->quan) {
                wager = mongold->quan;
                pline("\"Thou hast taken everything from me...\"");
                game_stage = -1;
            } else {
                game_stage = 0;
            }
            money2u(mon, wager);
            context.botl = TRUE;
            return 1;
        }
        break;
    }
    default:
        break;
    }
    return 0;
#undef game_stage
#undef wager
#undef player_roll
#undef games_played
#undef odice
}

int
play_blackjack(mon)
struct monst *mon;
{
#define game_stage      (EGAM(mon)->state[0])
#define wager           (EGAM(mon)->state[1])
#define cards_drawn     (EGAM(mon)->state[2])
#define games_played    (EGAM(mon)->state[3])
#define odeck           (EGAM(mon)->otmp)
#define card_value(c)   ((((c)->spe)%13)+1)
    /*  Game stage:
        -  0 = waiting to play
        -  1 = asking player whether to hit
        -  2 = checking score
        - -1 = refuse to play
    */
    if (!has_egam(mon)) {
        impossible("Playing game with non-gaming monster?");
        return 0;
    }
    /* We should always have a deck, regardless of where the game is up to. */
    if (!(odeck && odeck->where == OBJ_MINVENT && odeck->ocarry == mon)) {
        /* Try to find a suitable deck... */
        odeck = m_carrying(mon, DECK_OF_CARDS);
        if (!(odeck && count_contents(odeck, FALSE, TRUE, TRUE) > 11 - (5 * game_stage))) {
            /* If number of cards isn't at least this many, they could run out */
            pline("\"Blast, my cards are missing!\"");
            return 0;
        }
    }
    switch (game_stage) {
    case -1:
        pline("%s isn't interested.", Monnam(mon));
        break;
    case 0:
        if (greet_and_wager(mon, games_played ? 0 : "blackjack", &wager)) {
            games_played++;
            pline("%s deals you two cards.", Monnam(mon));
            drawcard(odeck, 0);
            drawcard(odeck, 0);
            cards_drawn = 2;
            game_stage = 1;
        }
        break;
    case 1:
        switch (ynq("\"Willst thou take another?\"")) {
        case 'y':
            drawcard(odeck, 0);
            if (++cards_drawn >= 5)
                game_stage = 2;
            break;
        case 'n':
            game_stage = 2;
            break;
        default:
            break;
        }
        if (game_stage != 2)
            break;
        /* FALLTHRU */
    case 2: {
        struct obj *otmp;
        struct obj *mongold = findgold(mon->minvent, TRUE);
        struct obj *cards[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        int uscore = 0, mscore = 0, aces = 0, num_cards = 0, ret = 0;
        for (otmp = invent; otmp; otmp = otmp->nobj) {
            if (otmp->otyp == PLAYING_CARD) {
                if (otmp->spe >= 0
                    && otmp->spe < (NUM_CARD_SUITS * NUM_CARD_VALUES)) {
                    /* Ignore any odd-looking cards */
                    switch (card_value(otmp)) {
                    case 1:  /* ace   */
                        uscore += (11 * otmp->quan);
                        aces += otmp->quan;
                        break;
                    case 11: /* jack  */
                    case 12: /* queen */
                    case 13: /* king  */
                        uscore += (10 * otmp->quan);
                        break;
                    default:
                        uscore += (card_value(otmp) * otmp->quan);
                        break;
                    }
                    if (num_cards < 5)
                        cards[num_cards] = otmp;
                    num_cards++;
                }
            }
        }
        if (num_cards != cards_drawn) {
            pline("\"Hey! What'rt thou trying to pull?\"");
            pline("%s gets angry!", Monnam(mon));
            mon->mpeaceful = 0;
            set_malign(mon);
            game_stage = -1;
            break;
        }
        while (uscore > 21 && aces > 0) {
            uscore -= 10;
            aces--;
        }
        if (uscore > 21) {
            pline("\"Thy score is %d!\"", uscore);
            ret = -1;
        } else if (num_cards == 2 && uscore == 21) {
            pline("Blackjack!");
            ret = 1;
        } else if (num_cards >= 5) {
            pline("5 card Charlie!");
            ret = 1;
        } else {
            pline("\"Thy score is %d. Now, my turn.\"", uscore);
            aces = 0;
            for (int i = 0; i < 2 || (i < 5 && mscore < uscore); i++) {
                otmp = drawcard(odeck, mon);
                if (otmp->spe >= 0
                    && otmp->spe < (NUM_CARD_SUITS * NUM_CARD_VALUES)) {
                    switch (card_value(otmp)) {
                    case 1:  /* ace   */
                        mscore += (11 * otmp->quan);
                        aces += otmp->quan;
                        break;
                    case 11: /* jack  */
                    case 12: /* queen */
                    case 13: /* king  */
                        mscore += (10 * otmp->quan);
                        break;
                    default:
                        mscore += (card_value(otmp) * otmp->quan);
                        break;
                    }
                    if (num_cards < 10)
                        cards[num_cards] = otmp;
                    num_cards++;
                } else {
                    /* Odd-looking card */
                    pline("\"What's this? %s?\"", An(xname(otmp)));
                    pline("%s gets angry!", Monnam(mon));
                    mon->mpeaceful = 0;
                    set_malign(mon);
                    game_stage = -1;
                    return 0;
                }
                while (mscore > 21 && aces > 0) {
                    mscore -= 10;
                    aces--;
                }
            }
            pline("\"That's %d.\"", mscore);
            ret = (uscore > mscore || mscore > 21) ? 1 : -1;
        }
        switch (ret) {
        case -1:
            pline("\"You lose!\"");
            pline("%s snatches the cards back.", Monnam(mon));
            game_stage = 0;
            break;
        case 1:
            pline("\"Blast! Thou hast won!\"");
            pline("%s gives you your winnings and snatches the cards back.",
                  Monnam(mon));
            wager *= 2;
            if (wager >= mongold->quan) {
                wager = mongold->quan;
                pline("\"Thou hast taken everything from me...\"");
                game_stage = -1;
            } else {
                game_stage = 0;
            }
            money2u(mon, wager);
            context.botl = TRUE;
            break;
        }
        while (--num_cards >= 0) {
            returncard(cards[num_cards], mon);
        }
        return ret;
    }
    default:
        break;
    }
    return 0;
}

#endif /* MINIGAME */
/* minigame.c */
