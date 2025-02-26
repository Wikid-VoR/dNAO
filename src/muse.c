/*	SCCS Id: @(#)muse.c	3.4	2002/12/23	*/
/*	Copyright (C) 1990 by Ken Arromdee			   */
/* NetHack may be freely redistributed.  See license for details.  */

/*
 * Monster item usage routines.
 */

#include "hack.h"
#include "edog.h"

extern const int monstr[];

boolean m_using = FALSE;

/* Let monsters use magic items.  Arbitrary assumptions: Monsters only use
 * scrolls when they can see, monsters know when wands have 0 charges, monsters
 * cannot recognize if items are cursed are not, monsters which are confused
 * don't know not to read scrolls, etc....
 */

STATIC_DCL struct permonst *FDECL(muse_newcham_mon, (struct monst *));
STATIC_DCL int FDECL(precheck, (struct monst *,struct obj *));
STATIC_DCL void FDECL(mzapmsg, (struct monst *,struct obj *,BOOLEAN_P));
STATIC_DCL void FDECL(mreadmsg, (struct monst *,struct obj *));
STATIC_DCL void FDECL(mquaffmsg, (struct monst *,struct obj *));
STATIC_PTR int FDECL(mbhitm, (struct monst *,struct obj *));
STATIC_DCL void FDECL(mbhit,
	(struct monst *,int,int FDECL((*),(MONST_P,OBJ_P)),
	int FDECL((*),(OBJ_P,OBJ_P)),struct obj *));
void FDECL(you_aggravate, (struct monst *));
STATIC_DCL void FDECL(mon_consume_unstone, (struct monst *,struct obj *,
	BOOLEAN_P,BOOLEAN_P));

static struct musable {
	struct obj *offensive;
	struct obj *defensive;
	struct obj *misc;
	int has_offense, has_defense, has_misc;
	/* =0, no capability; otherwise, different numbers.
	 * If it's an object, the object is also set (it's 0 otherwise).
	 */
} m;
static int trapx, trapy;
static boolean zap_oseen;
	/* for wands which use mbhitm and are zapped at players.  We usually
	 * want an oseen local to the function, but this is impossible since the
	 * function mbhitm has to be compatible with the normal zap routines,
	 * and those routines don't remember who zapped the wand.
	 */

/* Any preliminary checks which may result in the monster being unable to use
 * the item.  Returns 0 if nothing happened, 2 if the monster can't do anything
 * (i.e. it teleported) and 1 if it's dead.
 */
STATIC_OVL int
precheck(mon, obj)
struct monst *mon;
struct obj *obj;
{
	boolean vis;

	if (!obj) return 0;
	vis = cansee(mon->mx, mon->my);

	if (obj->oclass == POTION_CLASS) {
	    coord cc;
	    static const char * const empty = "The potion turns out to be empty.";
	    const char *potion_descr;
	    struct monst *mtmp;
#define POTION_OCCUPANT_CHANCE(n) (13 + 2*(n))	/* also in potion.c */

	    potion_descr = OBJ_DESCR(objects[obj->otyp]);
	    if (potion_descr && !strcmp(potion_descr, "milky")) {
	        if ( flags.ghost_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.ghost_count))) {
		    if (!enexto(&cc, mon->mx, mon->my, &mons[PM_GHOST])) return 0;
		    mquaffmsg(mon, obj);
		    m_useup(mon, obj);
		    mtmp = makemon(&mons[PM_GHOST], cc.x, cc.y, NO_MM_FLAGS);
		    if (!mtmp) {
			if (vis) pline(empty);
		    } else {
			if (vis) {
			    pline("As %s opens the bottle, an enormous %s emerges!",
			       mon_nam(mon),
			       Hallucination ? rndmonnam() : (const char *)"ghost");
			    pline("%s is frightened to death, and unable to move.",
				    Monnam(mon));
			}
			mon->mcanmove = 0;
			mon->mfrozen = 3;
		    }
		    return 2;
		}
	    }
	    if (potion_descr && !strcmp(potion_descr, "smoky") &&
		    flags.djinni_count < MAXMONNO &&
		    !rn2(POTION_OCCUPANT_CHANCE(flags.djinni_count))) {
		if (!enexto(&cc, mon->mx, mon->my, &mons[PM_DJINNI])) return 0;
		mquaffmsg(mon, obj);
		m_useup(mon, obj);
		mtmp = makemon(&mons[PM_DJINNI], cc.x, cc.y, NO_MM_FLAGS);
		if (!mtmp) {
		    if (vis) pline(empty);
		} else {
		    if (vis)
			pline("In a cloud of smoke, %s emerges!",
							a_monnam(mtmp));
		    pline("%s speaks.", vis ? Monnam(mtmp) : Something);
		/* I suspect few players will be upset that monsters */
		/* can't wish for wands of death here.... */
		    if (rn2(2)) {
			verbalize("You freed me!");
			mtmp->mpeaceful = 1;
			set_malign(mtmp);
		    } else {
			verbalize("It is about time.");
			if (vis) pline("%s vanishes.", Monnam(mtmp));
			mongone(mtmp);
		    }
		}
		return 2;
	    }
	}
	if (obj->oclass == WAND_CLASS && obj->cursed && !rn2(100)) {
	    int dam = d(obj->spe+2, 6);

	    if (flags.soundok) {
		if (vis) pline("%s %s %s, which suddenly explodes!",
			Monnam(mon), is_weeping(mon->data) ? "is zapping" : "zaps", an(xname(obj)));
		else You_hear("a zap and an explosion in the distance.");
	    }
	    m_useup(mon, obj);
	    if (mon->mhp <= dam) {
		monkilled(mon, "", AD_RBRE);
		return 1;
	    }
	    else mon->mhp -= dam;
	    m.has_defense = m.has_offense = m.has_misc = 0;
	    /* Only one needed to be set to 0 but the others are harmless */
	}
	return 0;
}

STATIC_OVL void
mzapmsg(mtmp, otmp, self)
struct monst *mtmp;
struct obj *otmp;
boolean self;
{
	if (!canseemon(mtmp)) {
		if (flags.soundok)
			You_hear("a %s zap.",
					(distu(mtmp->mx,mtmp->my) <= (BOLT_LIM+1)*(BOLT_LIM+1)) ?
					"nearby" : "distant");
	} else if (self)
		pline("%s %s %sself with %s!",
		      Monnam(mtmp), is_weeping(mtmp->data) ? "is zapping" : "zaps", mhim(mtmp), doname(otmp));
	else {
		pline("%s %s %s!", Monnam(mtmp), is_weeping(mtmp->data) ? "is zapping" : "zaps", an(xname(otmp)));
		stop_occupation();
	}
}

STATIC_OVL void
mreadmsg(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	boolean vismon = canseemon(mtmp);
	char onambuf[BUFSZ];
	short saverole;
	unsigned savebknown;

	if (!vismon && !flags.soundok)
	    return;		/* no feedback */

	otmp->dknown = 1;  /* seeing or hearing it read reveals its label */
	/* shouldn't be able to hear curse/bless status of unseen scrolls;
	   for priest characters, bknown will always be set during naming */
	savebknown = otmp->bknown;
	saverole = Role_switch;
	if (!vismon) {
	    otmp->bknown = 0;
	    if (Role_if(PM_PRIEST)) Role_switch = 0;
	}
	Strcpy(onambuf, singular(otmp, doname));
	Role_switch = saverole;
	otmp->bknown = savebknown;

	if (vismon)
	    pline("%s reads %s!", Monnam(mtmp), onambuf);
	else
	    You_hear("%s reading %s.",
		x_monnam(mtmp, ARTICLE_A, (char *)0,
		    (SUPPRESS_IT|SUPPRESS_INVISIBLE|SUPPRESS_SADDLE), FALSE),
		onambuf);

	if (mtmp->mconf)
	    pline("Being confused, %s mispronounces the magic words...",
		  vismon ? mon_nam(mtmp) : mhe(mtmp));
}

STATIC_OVL void
mquaffmsg(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;
{
	if (canseemon(mtmp)) {
		otmp->dknown = 1;
		pline("%s %s %s!", Monnam(mtmp), is_weeping(mtmp->data) ? "is drinking" : "drinks", singular(otmp, doname));
	} else
		if (flags.soundok)
			You_hear("a chugging sound.");
}

/* Defines for various types of stuff.  The order in which monsters prefer
 * to use them is determined by the order of the code logic, not the
 * numerical order in which they are defined.
 */
#define MUSE_SCR_TELEPORTATION 1
#define MUSE_WAN_TELEPORTATION_SELF 2
#define MUSE_POT_HEALING 3
#define MUSE_POT_EXTRA_HEALING 4
#define MUSE_WAN_DIGGING 5
#define MUSE_TRAPDOOR 6
#define MUSE_TELEPORT_TRAP 7
#define MUSE_UPSTAIRS 8
#define MUSE_DOWNSTAIRS 9
#define MUSE_WAN_CREATE_MONSTER 10
#define MUSE_SCR_CREATE_MONSTER 11
#define MUSE_UP_LADDER 12
#define MUSE_DN_LADDER 13
#define MUSE_SSTAIRS 14
#define MUSE_WAN_TELEPORTATION 15
#define MUSE_BUGLE 16
#define MUSE_UNICORN_HORN 17
#define MUSE_POT_FULL_HEALING 18
#define MUSE_LIZARD_CORPSE 19
/*
#define MUSE_INNATE_TPT 9999
 * We cannot use this.  Since monsters get unlimited teleportation, if they
 * were allowed to teleport at will you could never catch them.  Instead,
 * assume they only teleport at random times, despite the inconsistency that if
 * you polymorph into one you teleport at will.
 */

/* Select a defensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_defensive(mtmp)
struct monst *mtmp;
{
	register struct obj *obj = 0;
	struct trap *t;
	int x=mtmp->mx, y=mtmp->my;
	boolean stuck = (mtmp == u.ustuck);
	boolean immobile = (mtmp->data->mmove == 0);
	int fraction;

	if (is_animal(mtmp->data) || mindless_mon(mtmp))
		return FALSE;
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 25)
		return FALSE;
	if (u.uswallow && stuck) return FALSE;

	m.defensive = (struct obj *)0;
	m.has_defense = 0;

	/* since unicorn horns don't get used up, the monster would look
	 * silly trying to use the same cursed horn round after round
	 */
	if (mtmp->mconf || mtmp->mstun || !mtmp->mcansee) {
	    if (!(is_unicorn(mtmp->data) || mtmp->data == &mons[PM_KI_RIN]) && !nohands(mtmp->data)) {
			for(obj = mtmp->minvent; obj; obj = obj->nobj)
				if (obj->otyp == UNICORN_HORN && !obj->cursed)
				break;
	    }
	    if (obj || is_unicorn(mtmp->data) || mtmp->data == &mons[PM_KI_RIN]) {
			m.defensive = obj;
			m.has_defense = MUSE_UNICORN_HORN;
			return TRUE;
	    }
	}

	if (mtmp->mconf) {
	    for(obj = mtmp->minvent; obj; obj = obj->nobj) {
		if (obj->otyp == CORPSE && obj->corpsenm == PM_LIZARD) {
		    m.defensive = obj;
		    m.has_defense = MUSE_LIZARD_CORPSE;
		    return TRUE;
		}
	    }
	}

	/* It so happens there are two unrelated cases when we might want to
	 * check specifically for healing alone.  The first is when the monster
	 * is blind (healing cures blindness).  The second is when the monster
	 * is peaceful; then we don't want to flee the player, and by
	 * coincidence healing is all there is that doesn't involve fleeing.
	 * These would be hard to combine because of the control flow.
	 * Pestilence won't use healing even when blind.
	 */
	if (!mtmp->mcansee && !nohands(mtmp->data) &&
		mtmp->data != &mons[PM_PESTILENCE]) {
	    if ((obj = m_carrying(mtmp, POT_FULL_HEALING)) != 0) {
		m.defensive = obj;
		m.has_defense = MUSE_POT_FULL_HEALING;
		return TRUE;
	    }
	    if ((obj = m_carrying(mtmp, POT_EXTRA_HEALING)) != 0) {
		m.defensive = obj;
		m.has_defense = MUSE_POT_EXTRA_HEALING;
		return TRUE;
	    }
	    if ((obj = m_carrying(mtmp, POT_HEALING)) != 0) {
		m.defensive = obj;
		m.has_defense = MUSE_POT_HEALING;
		return TRUE;
	    }
	}

	fraction = u.ulevel < 10 ? 5 : u.ulevel < 14 ? 4 : 3;
	if(mtmp->mhp >= mtmp->mhpmax ||
			(mtmp->mhp >= 10 && mtmp->mhp*fraction >= mtmp->mhpmax))
		return FALSE;

	if (mtmp->mpeaceful) {
	    if (!nohands(mtmp->data)) {
		if ((obj = m_carrying(mtmp, POT_FULL_HEALING)) != 0) {
		    m.defensive = obj;
		    m.has_defense = MUSE_POT_FULL_HEALING;
		    return TRUE;
		}
		if ((obj = m_carrying(mtmp, POT_EXTRA_HEALING)) != 0) {
		    m.defensive = obj;
		    m.has_defense = MUSE_POT_EXTRA_HEALING;
		    return TRUE;
		}
		if ((obj = m_carrying(mtmp, POT_HEALING)) != 0) {
		    m.defensive = obj;
		    m.has_defense = MUSE_POT_HEALING;
		    return TRUE;
		}
	    }
	    return FALSE;
	}

	if (levl[x][y].typ == STAIRS && !stuck && !immobile) {
		if (x == xdnstair && y == ydnstair && !is_floater(mtmp->data) && mtmp->data != &mons[PM_SMAUG])
			m.has_defense = MUSE_DOWNSTAIRS;
		if (x == xupstair && y == yupstair && ledger_no(&u.uz) != 1)
	/* Unfair to let the monsters leave the dungeon with the Amulet */
	/* (or go to the endlevel since you also need it, to get there) */
			m.has_defense = MUSE_UPSTAIRS;
	} else if (levl[x][y].typ == LADDER && !stuck && !immobile) {
		if (x == xupladder && y == yupladder)
			m.has_defense = MUSE_UP_LADDER;
		if (x == xdnladder && y == ydnladder && !is_floater(mtmp->data))
			m.has_defense = MUSE_DN_LADDER;
	} else if (sstairs.sx && sstairs.sx == x && sstairs.sy == y) {
		m.has_defense = MUSE_SSTAIRS;
	} else if (!stuck && !immobile) {
	/* Note: trap doors take precedence over teleport traps. */
		int xx, yy;

		for(xx = x-1; xx <= x+1; xx++) for(yy = y-1; yy <= y+1; yy++)
		if (isok(xx,yy))
		if (xx != u.ux && yy != u.uy)
		if ((mtmp->data != &mons[PM_GRID_BUG] && mtmp->data != &mons[PM_BEBELITH]) || xx == x || yy == y)
		if((mtmp->data != &mons[PM_CLOCKWORK_SOLDIER] && mtmp->data != &mons[PM_CLOCKWORK_DWARF] && 
		   mtmp->data != &mons[PM_FABERGE_SPHERE] && mtmp->data != &mons[PM_FIREWORK_CART] && 
		   mtmp->data != &mons[PM_JUGGERNAUT] && mtmp->data != &mons[PM_ID_JUGGERNAUT]) ||
			(x + xdir[(int)mtmp->mvar1] == xx && 
			   y + ydir[(int)mtmp->mvar1] == yy 
			)
		)
		if ((xx==x && yy==y) || !level.monsters[xx][yy])
		if ((t = t_at(xx,yy)) != 0)
		if ((verysmall(mtmp->data) || throws_rocks(mtmp->data) ||
		     passes_walls(mtmp->data)) || !boulder_at(xx, yy))
		if (!onscary(xx,yy,mtmp)) {
			if ((t->ttyp == TRAPDOOR || t->ttyp == HOLE)
				&& !is_floater(mtmp->data)
				&& !mtmp->isshk && !mtmp->isgd
				&& !mtmp->ispriest
				&& Can_fall_thru(&u.uz)
						) {
				trapx = xx;
				trapy = yy;
				m.has_defense = MUSE_TRAPDOOR;
			} else if (t->ttyp == TELEP_TRAP && m.has_defense != MUSE_TRAPDOOR && !resists_magm(mtmp)) {
				trapx = xx;
				trapy = yy;
				m.has_defense = MUSE_TELEPORT_TRAP;
			}
		}
	}

	if (nohands(mtmp->data))	/* can't use objects */
		goto botm;

	if (is_mercenary(mtmp->data) && (obj = m_carrying(mtmp, BUGLE))) {
		int xx, yy;
		struct monst *mon;

		/* Distance is arbitrary.  What we really want to do is
		 * have the soldier play the bugle when it sees or
		 * remembers soldiers nearby...
		 */
		for(xx = x-3; xx <= x+3; xx++) for(yy = y-3; yy <= y+3; yy++)
		if (isok(xx,yy))
		if ((mon = m_at(xx,yy)) && is_mercenary(mon->data) &&
				mon->data != &mons[PM_GUARD] &&
				(mon->msleeping || (!mon->mcanmove && mon->mnotlaugh))) {
			m.defensive = obj;
			m.has_defense = MUSE_BUGLE;
		}
	}

	/* use immediate physical escape prior to attempting magic */
	if (m.has_defense)    /* stairs, trap door or tele-trap, bugle alert */
		goto botm;

	/* kludge to cut down on trap destruction (particularly portals) */
	t = t_at(x,y);
	if (t && (t->ttyp == PIT || t->ttyp == SPIKED_PIT ||
		  t->ttyp == WEB || t->ttyp == BEAR_TRAP))
		t = 0;		/* ok for monster to dig here */

#define nomore(x) if(m.has_defense==x) continue;
	for (obj = mtmp->minvent; obj; obj = obj->nobj) {
		/* don't always use the same selection pattern */
		if (m.has_defense && !rn2(3)) break;

		/* nomore(MUSE_WAN_DIGGING); */
		if (m.has_defense == MUSE_WAN_DIGGING) break;
		if (obj->otyp == WAN_DIGGING && obj->spe > 0 && !stuck && !t
		    && !mtmp->isshk && !mtmp->isgd && !mtmp->ispriest
		    && !is_floater(mtmp->data)
		    /* monsters digging in Sokoban can ruin things */
		    && !In_sokoban(&u.uz)
		    /* digging wouldn't be effective; assume they know that */
		    && !(levl[x][y].wall_info & W_NONDIGGABLE)
		    && !(Is_botlevel(&u.uz) || In_endgame(&u.uz))
		    && !(is_ice(x,y) || is_pool(x,y, TRUE) || is_lava(x,y))
		    && !(mtmp->data == &mons[PM_VLAD_THE_IMPALER]
			 && In_V_tower(&u.uz))) {
			m.defensive = obj;
			m.has_defense = MUSE_WAN_DIGGING;
		}
		nomore(MUSE_WAN_TELEPORTATION_SELF);
		nomore(MUSE_WAN_TELEPORTATION);
		if(obj->otyp == WAN_TELEPORTATION && obj->spe > 0) {
		    /* use the TELEP_TRAP bit to determine if they know
		     * about noteleport on this level or not.  Avoids
		     * ineffective re-use of teleportation.  This does
		     * mean if the monster leaves the level, they'll know
		     * about teleport traps.
		     */
		    if (!level.flags.noteleport ||
			!(mtmp->mtrapseen & (1 << (TELEP_TRAP-1)))) {
			m.defensive = obj;
			m.has_defense = (mon_has_amulet(mtmp))
				? MUSE_WAN_TELEPORTATION
				: MUSE_WAN_TELEPORTATION_SELF;
		    }
		}
		nomore(MUSE_SCR_TELEPORTATION);
		if(obj->otyp == SCR_TELEPORTATION && !is_blind(mtmp)
		   && haseyes(mtmp->data) && !is_weeping(mtmp->data)
		   && (!obj->cursed ||
		       (!(mtmp->isshk && inhishop(mtmp))
			    && !mtmp->isgd && !mtmp->ispriest))) {
		    /* see WAN_TELEPORTATION case above */
		    if (!level.flags.noteleport ||
			!(mtmp->mtrapseen & (1 << (TELEP_TRAP-1)))) {
			m.defensive = obj;
			m.has_defense = MUSE_SCR_TELEPORTATION;
		    }
		}

	    if (mtmp->data != &mons[PM_PESTILENCE]) {
		nomore(MUSE_POT_FULL_HEALING);
		if(obj->otyp == POT_FULL_HEALING) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_FULL_HEALING;
		}
		nomore(MUSE_POT_EXTRA_HEALING);
		if(obj->otyp == POT_EXTRA_HEALING) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_EXTRA_HEALING;
		}
		nomore(MUSE_WAN_CREATE_MONSTER);
		if(obj->otyp == WAN_CREATE_MONSTER && obj->spe > 0) {
			m.defensive = obj;
			m.has_defense = MUSE_WAN_CREATE_MONSTER;
		}
		nomore(MUSE_POT_HEALING);
		if(obj->otyp == POT_HEALING) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_HEALING;
		}
	    } else {	/* Pestilence */
		nomore(MUSE_POT_FULL_HEALING);
		if (obj->otyp == POT_SICKNESS) {
			m.defensive = obj;
			m.has_defense = MUSE_POT_FULL_HEALING;
		}
		nomore(MUSE_WAN_CREATE_MONSTER);
		if (obj->otyp == WAN_CREATE_MONSTER && obj->spe > 0) {
			m.defensive = obj;
			m.has_defense = MUSE_WAN_CREATE_MONSTER;
		}
	    }
		nomore(MUSE_SCR_CREATE_MONSTER);
		if(obj->otyp == SCR_CREATE_MONSTER && !is_weeping(mtmp->data)) {
			m.defensive = obj;
			m.has_defense = MUSE_SCR_CREATE_MONSTER;
		}
	}
botm:	return((boolean)(!!m.has_defense));
#undef nomore
}

/* Perform a defensive action for a monster.  Must be called immediately
 * after find_defensive().  Return values are 0: did something, 1: died,
 * 2: did something and can't attack again (i.e. teleported).
 */
int
use_defensive(mtmp)
struct monst *mtmp;
{
	int i, fleetim, how = 0;
	struct obj *otmp = m.defensive;
	boolean vis, vismon, oseen;
	const char *mcsa = "%s can see again.";

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	vis = cansee(mtmp->mx, mtmp->my);
	vismon = canseemon(mtmp);
	oseen = otmp && vismon;

	/* when using defensive choice to run away, we want monster to avoid
	   rushing right straight back; don't override if already scared */
	fleetim = !mtmp->mflee ? (33 - (30 * mtmp->mhp / mtmp->mhpmax)) : 0;
#define m_flee(m)	if (fleetim && !m->iswiz) \
			{ monflee(m, fleetim, FALSE, FALSE); }

	switch(m.has_defense) {
	case MUSE_UNICORN_HORN:
		if (vismon) {
		    if (otmp)
			pline("%s %s a unicorn horn!", Monnam(mtmp), is_weeping(mtmp->data) ? "is using" : "uses");
		    else
			pline_The("tip of %s's horn glows!", mon_nam(mtmp));
		}
		if (!mtmp->mcansee) {
		    mtmp->mcansee = 1;
		    mtmp->mblinded = 0;
		    if (vismon) pline(mcsa, Monnam(mtmp));
		} else if (mtmp->mconf || mtmp->mstun) {
		    mtmp->mconf = mtmp->mstun = 0;
		    if (vismon)
			pline("%s seems steadier now.", Monnam(mtmp));
		} else impossible("No need for unicorn horn?");
		return 2;
	case MUSE_BUGLE:
		if (vismon)
			pline("%s plays %s!", Monnam(mtmp), doname(otmp));
		else if (flags.soundok)
			You_hear("a bugle playing reveille!");
		awaken_soldiers();
		return 2;
	case MUSE_WAN_TELEPORTATION_SELF:
		if ((mtmp->isshk && inhishop(mtmp))
		       || mtmp->isgd || mtmp->ispriest) return 2;
		m_flee(mtmp);
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		how = WAN_TELEPORTATION;
mon_tele:
		if (tele_restrict(mtmp)) {	/* mysterious force... */
		    if (vismon && how)		/* mentions 'teleport' */
			makeknown(how);
		    /* monster learns that teleportation isn't useful here */
		    if (level.flags.noteleport)
			mtmp->mtrapseen |= (1 << (TELEP_TRAP-1));
		    return 2;
		}
		if ((
#if 0
			mon_has_amulet(mtmp) ||
#endif
			On_W_tower_level(&u.uz)) && !rn2(3)) {
		    if (vismon)
			pline("%s seems disoriented for a moment.",
				Monnam(mtmp));
		    return 2;
		}
		if (oseen && how) makeknown(how);
		(void) rloc(mtmp, FALSE);
		return 2;
	case MUSE_WAN_TELEPORTATION:
		zap_oseen = oseen;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		m_using = TRUE;
		mbhit(mtmp,rn1(8,6),mbhitm,bhito,otmp);
		/* monster learns that teleportation isn't useful here */
		if (level.flags.noteleport)
		    mtmp->mtrapseen |= (1 << (TELEP_TRAP-1));
		m_using = FALSE;
		return 2;
	case MUSE_SCR_TELEPORTATION:
	    {
		int obj_is_cursed = otmp->cursed;

		if (mtmp->isshk || mtmp->isgd || mtmp->ispriest) return 2;
		m_flee(mtmp);
		mreadmsg(mtmp, otmp);
		m_useup(mtmp, otmp);	/* otmp might be free'ed */
		how = SCR_TELEPORTATION;
		if (obj_is_cursed || mtmp->mconf) {
			int nlev;
			d_level flev;

			if (mon_has_amulet(mtmp) || In_endgame(&u.uz)) {
			    if (vismon)
				pline("%s seems very disoriented for a moment.",
					Monnam(mtmp));
			    return 2;
			}
			nlev = random_teleport_level();
			if (nlev == depth(&u.uz)) {
			    if (vismon)
				pline("%s shudders for a moment.",
								Monnam(mtmp));
			    return 2;
			}
			get_level(&flev, nlev);
			migrate_to_level(mtmp, ledger_no(&flev), MIGR_RANDOM,
				(coord *)0);
			if (oseen) makeknown(SCR_TELEPORTATION);
		} else goto mon_tele;
		return 2;
	    }
	case MUSE_WAN_DIGGING:
	    {	struct trap *ttmp;

		m_flee(mtmp);
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(WAN_DIGGING);
		if (IS_FURNITURE(levl[mtmp->mx][mtmp->my].typ) ||
		    IS_DRAWBRIDGE(levl[mtmp->mx][mtmp->my].typ) ||
		    (is_drawbridge_wall(mtmp->mx, mtmp->my) >= 0) ||
		    (sstairs.sx && sstairs.sx == mtmp->mx &&
				   sstairs.sy == mtmp->my)) {
			pline_The("digging ray is ineffective.");
			return 2;
		}
		if (!Can_dig_down(&u.uz)) {
		    if(canseemon(mtmp))
			pline_The("%s here is too hard to dig in.",
					surface(mtmp->mx, mtmp->my));
		    return 2;
		}
		ttmp = maketrap(mtmp->mx, mtmp->my, HOLE);
		if (!ttmp) return 2;
		seetrap(ttmp);
		if (vis) {
		    pline("%s has made a hole in the %s.", Monnam(mtmp),
				surface(mtmp->mx, mtmp->my));
		    pline("%s %s through...", Monnam(mtmp),
			  is_flyer(mtmp->data) ? "dives" : "falls");
		} else if (flags.soundok)
			You_hear("%s crash through the %s.", something,
				surface(mtmp->mx, mtmp->my));
		/* we made sure that there is a level for mtmp to go to */
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_RANDOM, (coord *)0);
		return 2;
	    }
	case MUSE_WAN_CREATE_MONSTER:
	    {	coord cc;
		    /* pm: 0 => random, eel => aquatic, croc => amphibious */
		struct permonst *pm = !is_pool(mtmp->mx, mtmp->my, FALSE) ? 0 :
			     &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];
		struct monst *mon;

		if (!enexto(&cc, mtmp->mx, mtmp->my, pm)) return 0;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		mon = makemon((struct permonst *)0, cc.x, cc.y, NO_MM_FLAGS);
		if (mon && canspotmon(mon) && oseen)
		    makeknown(WAN_CREATE_MONSTER);
		return 2;
	    }
	case MUSE_SCR_CREATE_MONSTER:
	    {	coord cc;
		struct permonst *pm = 0, *fish = 0;
		int cnt = 1;
		struct monst *mon;
		boolean known = FALSE;

		if (!rn2(73)) cnt += rnd(4);
		if (mtmp->mconf || otmp->cursed) cnt += 12;
		if (mtmp->mconf) pm = fish = &mons[PM_ACID_BLOB];
		else if (is_pool(mtmp->mx, mtmp->my, FALSE))
		    fish = &mons[u.uinwater ? PM_GIANT_EEL : PM_CROCODILE];
		mreadmsg(mtmp, otmp);
		while(cnt--) {
		    /* `fish' potentially gives bias towards water locations;
		       `pm' is what to actually create (0 => random) */
		    if (!enexto(&cc, mtmp->mx, mtmp->my, fish)) break;
		    mon = makemon(pm, cc.x, cc.y, NO_MM_FLAGS);
		    if (mon && canspotmon(mon)) known = TRUE;
		}
		/* The only case where we don't use oseen.  For wands, you
		 * have to be able to see the monster zap the wand to know
		 * what type it is.  For teleport scrolls, you have to see
		 * the monster to know it teleported.
		 */
		if (known)
		    makeknown(SCR_CREATE_MONSTER);
		else if (!objects[SCR_CREATE_MONSTER].oc_name_known
			&& !objects[SCR_CREATE_MONSTER].oc_uname)
		    docall(otmp);
		m_useup(mtmp, otmp);
		return 2;
	    }
	case MUSE_TRAPDOOR:
		/* trap doors on "bottom" levels of dungeons are rock-drop
		 * trap doors, not holes in the floor.  We check here for
		 * safety.
		 */
		if (Is_botlevel(&u.uz)) return 0;
		m_flee(mtmp);
		if (vis) {
			struct trap *t;
			t = t_at(trapx,trapy);
			pline("%s %s into a %s!", Monnam(mtmp),
			makeplural(is_weeping(mtmp->data) ? "tips over" : locomotion(mtmp->data, "jump")),
			t->ttyp == TRAPDOOR ? "trap door" : "hole");
			if (levl[trapx][trapy].typ == SCORR) {
			    levl[trapx][trapy].typ = CORR;
			    unblock_point(trapx, trapy);
			}
			seetrap(t_at(trapx,trapy));
		}

		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_RANDOM, (coord *)0);
		return 2;
	case MUSE_UPSTAIRS:
		/* Monsters without amulets escape the dungeon and are
		 * gone for good when they leave up the up stairs.
		 * Monsters with amulets would reach the endlevel,
		 * which we cannot allow since that would leave the
		 * player stranded.
		 */
		if (ledger_no(&u.uz) == 1) {
			if (mon_has_special(mtmp))
				return 0;
			if (vismon)
			    pline("%s %s the dungeon!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
			mongone(mtmp);
			return 2;
		}
		m_flee(mtmp);
		if (Inhell && mon_has_amulet(mtmp) && !rn2(4) &&
			(dunlev(&u.uz) < dunlevs_in_dungeon(&u.uz) - 3) &&
			(u.uz.dlevel < wiz1_level.dlevel) &&
			(u.uz.dlevel > valley_level.dlevel) 
		) {
		    if (vismon) pline(
     "As %s climbs the stairs, a mysterious force momentarily surrounds %s...",
				     mon_nam(mtmp), mhim(mtmp));
		    /* simpler than for the player; this will usually be
		       the Wizard and he'll immediately go right to the
		       upstairs, so there's not much point in having any
		       chance for a random position on the current level */
		    migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				     MIGR_RANDOM, (coord *)0);
		} else {
		    if (vismon) pline("%s %s upstairs!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
		    migrate_to_level(mtmp, ledger_no(&u.uz) - 1,
				     MIGR_STAIRS_DOWN, (coord *)0);
		}
		return 2;
	case MUSE_DOWNSTAIRS:
		m_flee(mtmp);
		if (vismon) pline("%s %s downstairs!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_STAIRS_UP, (coord *)0);
		return 2;
	case MUSE_UP_LADDER:
		m_flee(mtmp);
		if (vismon) pline("%s %s up the ladder!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
		migrate_to_level(mtmp, ledger_no(&u.uz) - 1,
				 MIGR_LADDER_DOWN, (coord *)0);
		return 2;
	case MUSE_DN_LADDER:
		m_flee(mtmp);
		if (vismon) pline("%s %s down the ladder!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
		migrate_to_level(mtmp, ledger_no(&u.uz) + 1,
				 MIGR_LADDER_UP, (coord *)0);
		return 2;
	case MUSE_SSTAIRS:
		m_flee(mtmp);
		/* the stairs leading up from the 1st level are */
		/* regular stairs, not sstairs.			*/
		if (sstairs.up) {
			if (vismon)
			    pline("%s %s upstairs!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
			if(Inhell) {
			    migrate_to_level(mtmp, ledger_no(&sstairs.tolev),
					     MIGR_RANDOM, (coord *)0);
			    return 2;
			}
		} else	if (vismon)
		    pline("%s %s downstairs!", Monnam(mtmp), is_weeping(mtmp->data) ? "has escaped" : "escapes");
		migrate_to_level(mtmp, ledger_no(&sstairs.tolev),
				 MIGR_SSTAIRS, (coord *)0);
		return 2;
	case MUSE_TELEPORT_TRAP:
		m_flee(mtmp);
		if (vis) {
			pline("%s %s onto a teleport trap!", Monnam(mtmp),
				is_weeping(mtmp->data) ? "tips over" : makeplural(locomotion(mtmp->data, "jump")));
			if (levl[trapx][trapy].typ == SCORR) {
			    levl[trapx][trapy].typ = CORR;
			    unblock_point(trapx, trapy);
			}
			seetrap(t_at(trapx,trapy));
		}
		/*  don't use rloc_to() because worm tails must "move" */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);	/* update old location */
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);

		goto mon_tele;
	case MUSE_POT_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(6 + 2 * bcsign(otmp), 4);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax) mtmp->mhp = ++mtmp->mhpmax;
		if (!otmp->cursed && !mtmp->mcansee) {
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			if (vismon) pline(mcsa, Monnam(mtmp));
		}
		if (vismon) pline("%s looks better.", Monnam(mtmp));
		if (oseen) makeknown(POT_HEALING);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_EXTRA_HEALING:
		mquaffmsg(mtmp, otmp);
		i = d(6 + 2 * bcsign(otmp), 8);
		mtmp->mhp += i;
		if (mtmp->mhp > mtmp->mhpmax)
			mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 5 : 2));
		if (!mtmp->mcansee) {
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			if (vismon) pline(mcsa, Monnam(mtmp));
		}
		if (vismon) pline("%s looks much better.", Monnam(mtmp));
		if (oseen) makeknown(POT_EXTRA_HEALING);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_FULL_HEALING:
		mquaffmsg(mtmp, otmp);
		if (otmp->otyp == POT_SICKNESS) unbless(otmp); /* Pestilence */
		if(mtmp->mhpmax - mtmp->mhp > 400) mtmp->mhp += 400;
		else mtmp->mhp = (mtmp->mhpmax += (otmp->blessed ? 8 : 4));
		if (!mtmp->mcansee && otmp->otyp != POT_SICKNESS) {
			mtmp->mcansee = 1;
			mtmp->mblinded = 0;
			if (vismon) pline(mcsa, Monnam(mtmp));
		}
		if (vismon) pline("%s looks completely healed.", Monnam(mtmp));
		if (oseen) makeknown(otmp->otyp);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_LIZARD_CORPSE:
		/* not actually called for its unstoning effect */
		mon_consume_unstone(mtmp, otmp, FALSE, FALSE);
		return 2;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_defense);
		break;
	}
	return 0;
#undef m_flee
}

const int good_amulets[] = {
	AMULET_OF_DRAIN_RESISTANCE,
	AMULET_OF_ESP,
	AMULET_OF_LIFE_SAVING,
	AMULET_VERSUS_POISON,
	AMULET_VERSUS_CURSES,
	AMULET_OF_UNCHANGING,
	AMULET_OF_NULLIFY_MAGIC,
	AMULET_OF_REFLECTION,
	AMULET_OF_MAGICAL_BREATHING
};

int
rnd_good_amulet()
{
	return good_amulets[rn2(SIZE(good_amulets))];
}

int
rnd_defensive_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];
	int trycnt = 0;

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
    try_again:
	switch (rn2(8 + (difficulty > 3) + (difficulty > 6) +
				(difficulty > 8))) {
		case 6: case 9:
			if (level.flags.noteleport && ++trycnt < 2)
			    goto try_again;
			if (!rn2(3)) return WAN_TELEPORTATION;
			/* else FALLTHRU */
		case 0: case 1:
			return SCR_TELEPORTATION;
		case 8: case 10:
			if (!rn2(3)) return WAN_CREATE_MONSTER;
			/* else FALLTHRU */
		case 2: return SCR_CREATE_MONSTER;
		case 3: return POT_HEALING;
		case 4: return POT_EXTRA_HEALING;
		case 5: return (mtmp->data != &mons[PM_PESTILENCE]) ?
				POT_FULL_HEALING : POT_SICKNESS;
		case 7: if (is_floater(pm) || mtmp->isshk || mtmp->isgd
						|| mtmp->ispriest
									)
				return 0;
			else
				return WAN_DIGGING;
	}
	/*NOTREACHED*/
	return 0;
}

#define MUSE_WAN_DEATH 1
#define MUSE_WAN_SLEEP 2
#define MUSE_WAN_FIRE 3
#define MUSE_WAN_COLD 4
#define MUSE_WAN_LIGHTNING 5
#define MUSE_WAN_MAGIC_MISSILE 6
#define MUSE_WAN_STRIKING 7
#define MUSE_SCR_FIRE 8
#define MUSE_POT_PARALYSIS 9
#define MUSE_POT_BLINDNESS 10
#define MUSE_POT_CONFUSION 11
#define MUSE_FROST_HORN 12
#define MUSE_FIRE_HORN 13
#define MUSE_POT_ACID 14
/*#define MUSE_WAN_TELEPORTATION 15*/
#define MUSE_POT_SLEEPING 16
#define MUSE_WAN_DRAINING 17	/* KMH */
#define MUSE_SCR_EARTH 18
#define MUSE_POT_ENERGY_BLOOD 19
#define MUSE_POT_STONE_BLOOD 20
#define MUSE_MJOLLNIR 21

#define MUSE_WAN_CANCELLATION 22	/* Lethe */

/* Find a mask.
 */
struct permonst *
find_mask(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	int maskno = 0;
	for(obj = mtmp->minvent; obj; obj = obj->nobj){
		if(obj->otyp == MASK) maskno++;
	}
	if(!maskno) return mtmp->data;
	else maskno = rnd(maskno);
	
	for(obj = mtmp->minvent; obj; obj = obj->nobj){
		if(obj->otyp == MASK) maskno--;
		if(!maskno) return &mons[(int)(obj->corpsenm)];
	}
	return mtmp->data; //Should never reach
}

/* Select an offensive item/action for a monster.  Returns TRUE iff one is
 * found.
 */
boolean
find_offensive(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	boolean ranged_stuff = FALSE; 
	boolean reflection_skip = FALSE; 
	struct obj *helmet = which_armor(mtmp, W_ARMH);

	struct monst *target = mfind_target(mtmp, TRUE);
	
	if(tbx == 0 && tby == 0) return FALSE; //Target is not lined up.
	
	if(mtmp->data->maligntyp < 0 && Is_illregrd(&u.uz)) return 0;
	
	if (target)
	{
	    ranged_stuff = TRUE;
	    if (target == &youmonst)
	        reflection_skip = (Reflecting && rn2(2));
	}
	else
	    return FALSE; /* nothing to attack */

	m.offensive = (struct obj *)0;
	m.has_offense = 0;
	if (is_animal(mtmp->data) || mindless_mon(mtmp) ||
	    nohands(mtmp->data))
		return FALSE;
	if (target == &youmonst)
	{
	if (u.uswallow) return FALSE;
	if (in_your_sanctuary(mtmp, 0, 0)) return FALSE;
	if (dmgtype(mtmp->data, AD_HEAL) && !uwep
#ifdef TOURIST
	    && !uarmu
#endif
	    && !uarm && !uarmh && !uarms && !uarmg && !uarmc && !uarmf)
		return FALSE;
	}

	if (!ranged_stuff) return FALSE;
#define nomore(x) if(m.has_offense==x) continue;
	for(obj=mtmp->minvent; obj; obj=obj->nobj) {
	    if(mtmp->data == &mons[PM_VALKYRIE] && obj->oartifact == ART_MJOLLNIR && !obj->cursed && mtmp->misc_worn_check & ARM_GLOVES) {
			struct obj *otmp;
			for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
				if (otmp->owornmask & ARM_GLOVES && otmp->otyp == GAUNTLETS_OF_POWER){
					m.offensive = obj;
					m.has_offense = MUSE_MJOLLNIR;
			break;
				}
			}
	if(m.has_offense==MUSE_MJOLLNIR) break;
		}
		nomore(MUSE_WAN_DEATH);
		if (!reflection_skip) {
		    if(obj->otyp == WAN_DEATH && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_DEATH;
		    }
		    nomore(MUSE_WAN_SLEEP);
		    if(obj->otyp == WAN_SLEEP && obj->spe > 0 && multi >= 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_SLEEP;
		    }
		    nomore(MUSE_WAN_FIRE);
		    if(obj->otyp == WAN_FIRE && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_FIRE;
		    }
		    nomore(MUSE_FIRE_HORN);
		    if(obj->otyp == FIRE_HORN && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_FIRE_HORN;
		    }
		    nomore(MUSE_WAN_COLD);
		    if(obj->otyp == WAN_COLD && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_COLD;
		    }
		    nomore(MUSE_FROST_HORN);
		    if(obj->otyp == FROST_HORN && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_FROST_HORN;
		    }
		    nomore(MUSE_WAN_LIGHTNING);
		    if(obj->otyp == WAN_LIGHTNING && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_LIGHTNING;
		    }
		    nomore(MUSE_WAN_MAGIC_MISSILE);
		    if(obj->otyp == WAN_MAGIC_MISSILE && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_MAGIC_MISSILE;
		    }
		}
		// nomore(MUSE_POT_STONE_BLOOD);
		// if(obj->otyp == POT_BLOOD && !rn2(20)
			// && !is_vampire(mtmp->data)
			// && touch_petrifies(&mons[obj->corpsenm])) {
			// m.offensive = obj;
			// m.has_offense = MUSE_POT_STONE_BLOOD;
		// }
		nomore(MUSE_WAN_DRAINING);
		if(obj->otyp == WAN_DRAINING && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_DRAINING;
		}
		nomore(MUSE_WAN_STRIKING);
		if(obj->otyp == WAN_STRIKING && obj->spe > 0) {
			m.offensive = obj;
			m.has_offense = MUSE_WAN_STRIKING;
		}
		nomore(MUSE_POT_PARALYSIS);
		if(obj->otyp == POT_PARALYSIS && multi >= 0) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_PARALYSIS;
		}
		nomore(MUSE_POT_BLINDNESS);
		if(obj->otyp == POT_BLINDNESS && !attacktype(mtmp->data, AT_GAZE)) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_BLINDNESS;
		}
		nomore(MUSE_POT_CONFUSION);
		if(obj->otyp == POT_CONFUSION) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_CONFUSION;
		}
		nomore(MUSE_POT_SLEEPING);
		if(obj->otyp == POT_SLEEPING) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_SLEEPING;
		}
		nomore(MUSE_POT_ACID);
		if(obj->otyp == POT_ACID) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_ACID;
		}
		nomore(MUSE_POT_ENERGY_BLOOD);
		if(obj->otyp == POT_BLOOD 
			&& !is_vampire(mtmp->data)
			&& crpsdanger(&mons[obj->corpsenm])
		) {
			m.offensive = obj;
			m.has_offense = MUSE_POT_ENERGY_BLOOD;
		}
		/* we can safely put this scroll here since the locations that
		 * are in a 1 square radius are a subset of the locations that
		 * are in wand range
		 */
		nomore(MUSE_SCR_EARTH);
		if (obj->otyp == SCR_EARTH
		       && ((helmet && is_hard(helmet)) ||
				mtmp->mconf || amorphous(mtmp->data) ||
				passes_walls(mtmp->data) ||
				noncorporeal(mtmp->data) ||
				unsolid(mtmp->data) || !rn2(10))
		       && dist2(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy) <= 2
		       && !is_blind(mtmp) && haseyes(mtmp->data)
#ifdef REINCARNATION
		       && !Is_rogue_level(&u.uz)
#endif
		       && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
		    m.offensive = obj;
		    m.has_offense = MUSE_SCR_EARTH;
		}
		nomore(MUSE_WAN_CANCELLATION);
		if (obj->otyp == WAN_CANCELLATION && obj->spe > 0) {
			m.offensive   = obj;
			m.has_offense = MUSE_WAN_CANCELLATION;
		}
#if 0
		nomore(MUSE_SCR_FIRE);
		if (obj->otyp == SCR_FIRE && resists_fire(mtmp)
		   && dist2(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy) <= 2
		   && !is_blind(mtmp) && haseyes(mtmp->data)) {
			m.offensive = obj;
			m.has_offense = MUSE_SCR_FIRE;
		}
#endif
	}
	return((boolean)(!!m.has_offense));
#undef nomore
}

STATIC_PTR
int
mbhitm(mtmp, otmp)
register struct monst *mtmp;
register struct obj *otmp;
{
	int tmp;

	boolean reveal_invis = FALSE;
	if (mtmp != &youmonst) {
		mtmp->msleeping = 0;
		if (mtmp->m_ap_type) seemimic(mtmp);
	}
	switch(otmp->otyp) {
	case WAN_STRIKING:
		reveal_invis = TRUE;
		if (mtmp == &youmonst) {
			if (zap_oseen) makeknown(WAN_STRIKING);
			if (Antimagic) {
			    shieldeff(u.ux, u.uy);
			    pline("Boing!");
			} else {
			    pline_The("wand hits you!");
			    tmp = d(2,12);
			    if(Half_spell_damage) tmp = (tmp+1) / 2;
			    losehp(tmp, "wand", KILLED_BY_AN);
			}
			stop_occupation();
			nomul(0, NULL);
		} else if (resists_magm(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
			pline("Boing!");
		} else {
			tmp = d(2,12);
			hit("wand", mtmp, exclam(tmp));
			(void) resist(mtmp, otmp->oclass, tmp, TELL);
			if (cansee(mtmp->mx, mtmp->my) && zap_oseen)
				makeknown(WAN_STRIKING);
		}
		break;
	case WAN_TELEPORTATION:
		if (mtmp == &youmonst) {
			if (zap_oseen) makeknown(WAN_TELEPORTATION);
			tele();
		} else {
			/* for consistency with zap.c, don't identify */
			if (mtmp->ispriest &&
				*in_rooms(mtmp->mx, mtmp->my, TEMPLE)) {
			    if (cansee(mtmp->mx, mtmp->my))
				pline("%s resists the magic!", Monnam(mtmp));
			    mtmp->msleeping = 0;
			    if(mtmp->m_ap_type) seemimic(mtmp);
			} else if (!tele_restrict(mtmp))
			    (void) rloc(mtmp, FALSE);
		}
		break;
	case WAN_CANCELLATION:
	case SPE_CANCELLATION:
		(void) cancel_monst(mtmp, otmp, FALSE, TRUE, FALSE,0);
		break;
	case WAN_DRAINING:	/* KMH */
		tmp = d(2,6);
		if (mtmp == &youmonst) {
			if (Drain_resistance) {
				shieldeff(u.ux, u.uy);
				pline("Boing!");
			} else {
				losexp("life drainage", TRUE, FALSE, FALSE);
			}
			if (zap_oseen)
				makeknown(WAN_DRAINING);
			stop_occupation();
			nomul(0, NULL);
			break;
		} else if (resists_drli(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
			break;	/* skip makeknown */
		} else if (!resist(mtmp, otmp->oclass, tmp, NOTELL) &&
				mtmp->mhp > 0) {
			mtmp->mhpmax -= tmp;
			if (mtmp->mhpmax <= 0 || mtmp->m_lev <= 0)
				monkilled(mtmp, "", AD_DRLI);
			else {
				mtmp->m_lev--;
				if (canseemon(mtmp)) {
					pline("%s suddenly seems weaker!", Monnam(mtmp));
				}
			}
		}
		if (cansee(mtmp->mx, mtmp->my) && zap_oseen)
			makeknown(WAN_DRAINING);
		break;
	}
	if (reveal_invis) {
	    if (mtmp->mhp > 0 && cansee(bhitpos.x,bhitpos.y)
							&& !canspotmon(mtmp))
		map_invisible(bhitpos.x, bhitpos.y);
	}
	return 0;
}

/* A modified bhit() for monsters.  Based on bhit() in zap.c.  Unlike
 * buzz(), bhit() doesn't take into account the possibility of a monster
 * zapping you, so we need a special function for it.  (Unless someone wants
 * to merge the two functions...)
 */
STATIC_OVL void
mbhit(mon,range,fhitm,fhito,obj)
struct monst *mon;			/* monster shooting the wand */
register int range;			/* direction and range */
int FDECL((*fhitm),(MONST_P,OBJ_P));
int FDECL((*fhito),(OBJ_P,OBJ_P));	/* fns called when mon/obj hit */
struct obj *obj;			/* 2nd arg to fhitm/fhito */
{
	register struct monst *mtmp;
	register struct obj *otmp;
	register uchar typ;
	int ddx, ddy;

	bhitpos.x = mon->mx;
	bhitpos.y = mon->my;
	ddx = sgn(tbx);
	ddy = sgn(tby);

	while(range-- > 0) {
		int x,y;

		bhitpos.x += ddx;
		bhitpos.y += ddy;
		x = bhitpos.x; y = bhitpos.y;

		if (!isok(x,y)) {
		    bhitpos.x -= ddx;
		    bhitpos.y -= ddy;
		    break;
		}
		if (find_drawbridge(&x,&y))
		    switch (obj->otyp) {
			case WAN_STRIKING:
			    destroy_drawbridge(x,y);
		    }
		if(bhitpos.x==u.ux && bhitpos.y==u.uy) {
			(*fhitm)(&youmonst, obj);
			range -= 3;
		} else if(MON_AT(bhitpos.x, bhitpos.y)){
			mtmp = m_at(bhitpos.x,bhitpos.y);
			if (cansee(bhitpos.x,bhitpos.y) && !canspotmon(mtmp))
			    map_invisible(bhitpos.x, bhitpos.y);
			(*fhitm)(mtmp, obj);
			range -= 3;
		}
		/* modified by GAN to hit all objects */
		if(fhito){
		    int hitanything = 0;
		    register struct obj *next_obj;

		    for(otmp = level.objects[bhitpos.x][bhitpos.y];
							otmp; otmp = next_obj) {
			/* Fix for polymorph bug, Tim Wright */
			next_obj = otmp->nexthere;
			hitanything += (*fhito)(otmp, obj);
		    }
		    if(hitanything)	range--;
		}
		typ = levl[bhitpos.x][bhitpos.y].typ;
		if (typ == IRONBARS && obj->otyp==WAN_STRIKING && !Is_illregrd(&u.uz)){
			char numbars;
			struct obj *obj;
			You_hear("a sharp crack!");
		    levl[bhitpos.x][bhitpos.y].typ = CORR;
			for(numbars = d(2,4)-1; numbars > 0; numbars--){
				obj = mksobj_at(BAR, bhitpos.x, bhitpos.y, FALSE, FALSE);
			    obj->spe = 0;
			    obj->cursed = obj->blessed = FALSE;
			}
		    newsym(bhitpos.x, bhitpos.y);
		}
		if(IS_DOOR(typ) || typ == SDOOR) {
		    switch (obj->otyp) {
			/* note: monsters don't use opening or locking magic
			   at present, but keep these as placeholders */
			case WAN_OPENING:
			case WAN_LOCKING:
			case WAN_STRIKING:
			    if (doorlock(obj, bhitpos.x, bhitpos.y)) {
				makeknown(obj->otyp);
				/* if a shop door gets broken, add it to
				   the shk's fix list (no cost to player) */
				if (levl[bhitpos.x][bhitpos.y].doormask ==
					D_BROKEN &&
				    *in_rooms(bhitpos.x, bhitpos.y, SHOPBASE))
				    add_damage(bhitpos.x, bhitpos.y, 0L);
			    }
			    break;
		    }
		}
		if(!ZAP_POS(typ) || (IS_DOOR(typ) &&
		   (levl[bhitpos.x][bhitpos.y].doormask & (D_LOCKED | D_CLOSED)))
		  ) {
			bhitpos.x -= ddx;
			bhitpos.y -= ddy;
			break;
		}
	}
}

/* Perform an offensive action for a monster.  Must be called immediately
 * after find_offensive().  Return values are same as use_defensive().
 */
int
use_offensive(mtmp)
struct monst *mtmp;
{
	int i;
	struct obj *otmp = m.offensive;
	boolean oseen;

	/* offensive potions are not drunk, they're thrown */
	if (otmp->oclass != POTION_CLASS && (i = precheck(mtmp, otmp)) != 0)
		return i;
	oseen = otmp && canseemon(mtmp);

	switch(m.has_offense) {
	case MUSE_WAN_DEATH:
	case MUSE_WAN_SLEEP:
	case MUSE_WAN_FIRE:
	case MUSE_WAN_COLD:
	case MUSE_WAN_LIGHTNING:
	case MUSE_WAN_MAGIC_MISSILE:
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		if (oseen) makeknown(otmp->otyp);
		m_using = TRUE;
		buzz(wand_adtype(otmp->otyp), WAND_CLASS, FALSE,
			(otmp->otyp == WAN_MAGIC_MISSILE) ? 2 : 6,
			mtmp->mx, mtmp->my,
			sgn(tbx), sgn(tby),0,0);
		m_using = FALSE;
		if(u.ux != mtmp->mux || u.uy != mtmp->muy){
			mtmp->mux = mtmp->muy = 0;
		}
		return (mtmp->mhp <= 0) ? 1 : 2;
	case MUSE_FIRE_HORN:
	case MUSE_FROST_HORN:
		if (oseen) {
			makeknown(otmp->otyp);
			pline("%s plays a %s!", Monnam(mtmp), xname(otmp));
		} else
			You_hear("a horn being played.");
		otmp->spe--;
		m_using = TRUE;
		buzz(((otmp->otyp==FROST_HORN) ? AD_COLD : AD_FIRE), WAND_CLASS, FALSE,
			rn1(6,6), mtmp->mx, mtmp->my,
			sgn(tbx), sgn(tby),0,0);
		m_using = FALSE;
		if(u.ux != mtmp->mux || u.uy != mtmp->muy){
			mtmp->mux = mtmp->muy = 0;
		}
		return (mtmp->mhp <= 0) ? 1 : 2;
	case MUSE_WAN_TELEPORTATION:
	case MUSE_WAN_STRIKING:
	case MUSE_WAN_DRAINING:	/* KMH */
	case MUSE_WAN_CANCELLATION:  /* Lethe */
		zap_oseen = oseen;
		mzapmsg(mtmp, otmp, FALSE);
		otmp->spe--;
		m_using = TRUE;
		mbhit(mtmp,rn1(8,6),mbhitm,bhito,otmp);
		m_using = FALSE;
		if(u.ux != mtmp->mux || u.uy != mtmp->muy){
			mtmp->mux = mtmp->muy = 0;
		}
		return 2;
	case MUSE_SCR_EARTH:
	    {
		/* TODO: handle steeds */
	    	register int x, y;
		/* don't use monster fields after killing it */
		boolean confused = (mtmp->mconf ? TRUE : FALSE);
		int mmx = mtmp->mx, mmy = mtmp->my;

		mreadmsg(mtmp, otmp);
	    	/* Identify the scroll */
		if (canspotmon(mtmp)) {
		    pline_The("%s rumbles %s %s!", ceiling(mtmp->mx, mtmp->my),
	    			otmp->blessed ? "around" : "above",
				mon_nam(mtmp));
		    if (oseen) makeknown(otmp->otyp);
		} else if (cansee(mtmp->mx, mtmp->my)) {
		    pline_The("%s rumbles in the middle of nowhere!",
			ceiling(mtmp->mx, mtmp->my));
		    if (mtmp->minvis)
			map_invisible(mtmp->mx, mtmp->my);
		    if (oseen) makeknown(otmp->otyp);
		}

	    	/* Loop through the surrounding squares */
	    	for (x = mmx-1; x <= mmx+1; x++) {
	    	    for (y = mmy-1; y <= mmy+1; y++) {
	    	    	/* Is this a suitable spot? */
	    	    	if (isok(x, y) && !closed_door(x, y) &&
	    	    			!IS_ROCK(levl[x][y].typ) &&
	    	    			!IS_AIR(levl[x][y].typ) &&
	    	    			(((x == mmx) && (y == mmy)) ?
	    	    			    !otmp->blessed : !otmp->cursed) &&
					(x != u.ux || y != u.uy)) {
			    register struct obj *otmp2;
			    register struct monst *mtmp2;

	    	    	    /* Make the object(s) */
	    	    	    otmp2 = mksobj(confused ? ROCK : BOULDER,
	    	    	    		FALSE, FALSE);
	    	    	    if (!otmp2) continue;  /* Shouldn't happen */
	    	    	    otmp2->quan = confused ? rn1(5,2) : 1;
	    	    	    otmp2->owt = weight(otmp2);

	    	    	    /* Find the monster here (might be same as mtmp) */
	    	    	    mtmp2 = m_at(x, y);
	    	    	    if (mtmp2 && !amorphous(mtmp2->data) &&
	    	    	    		!passes_walls(mtmp2->data) &&
	    	    	    		!noncorporeal(mtmp2->data) &&
	    	    	    		!unsolid(mtmp2->data)) {
				struct obj *helmet = which_armor(mtmp2, W_ARMH);
				int mdmg;

				if (cansee(mtmp2->mx, mtmp2->my)) {
				    pline("%s is hit by %s!", Monnam(mtmp2),
	    	    	    			doname(otmp2));
				    if (mtmp2->minvis && !canspotmon(mtmp2))
					map_invisible(mtmp2->mx, mtmp2->my);
				}
	    	    	    	mdmg = dmgval(otmp2, mtmp2, 0) * otmp2->quan;
				if (helmet) {
				    if(is_hard(helmet)) {
					if (canspotmon(mtmp2))
					    pline("Fortunately, %s is wearing a hard helmet.", mon_nam(mtmp2));
					else if (flags.soundok)
					    You_hear("a clanging sound.");
					if (mdmg > 2) mdmg = 2;
				    } else {
					if (canspotmon(mtmp2))
					    pline("%s's %s does not protect %s.",
						Monnam(mtmp2), xname(helmet),
						mhim(mtmp2));
				    }
				}
	    	    	    	mtmp2->mhp -= mdmg;
	    	    	    	if (mtmp2->mhp <= 0) {
				    pline("%s is killed.", Monnam(mtmp2));
	    	    	    	    mondied(mtmp2);
				}
	    	    	    }
	    	    	    /* Drop the rock/boulder to the floor */
	    	    	    if (!flooreffects(otmp2, x, y, "fall")) {
	    	    	    	place_object(otmp2, x, y);
	    	    	    	stackobj(otmp2);
	    	    	    	newsym(x, y);  /* map the rock */
	    	    	    }
	    	    	}
		    }
		}
		m_useup(mtmp, otmp);
		/* Attack the player */
		if (distmin(mmx, mmy, u.ux, u.uy) == 1 && !otmp->cursed) {
		    int dmg;
		    struct obj *otmp2;

		    /* Okay, _you_ write this without repeating the code */
		    otmp2 = mksobj(confused ? ROCK : BOULDER,
				FALSE, FALSE);
		    if (!otmp2) goto xxx_noobj;  /* Shouldn't happen */
		    otmp2->quan = confused ? rn1(5,2) : 1;
		    otmp2->owt = weight(otmp2);
		    if (!amorphous(youracedata) &&
			    !Passes_walls &&
			    !noncorporeal(youracedata) &&
			    !unsolid(youracedata)) {
			You("are hit by %s!", doname(otmp2));
			dmg = dmgval(otmp2, &youmonst, 0) * otmp2->quan;
			if (uarmh) {
			    if(is_hard(uarmh)) {
				pline("Fortunately, you are wearing a hard helmet.");
				if (dmg > 2) dmg = 2;
			    } else if (flags.verbose) {
				Your("%s does not protect you.",
						xname(uarmh));
			    }
			}
		    } else
			dmg = 0;
		    if (!flooreffects(otmp2, u.ux, u.uy, "fall")) {
			place_object(otmp2, u.ux, u.uy);
			stackobj(otmp2);
			newsym(u.ux, u.uy);
		    }
		    if (dmg) losehp(dmg, "scroll of earth", KILLED_BY_AN);
		}
	    xxx_noobj:

		return (mtmp->mhp <= 0) ? 1 : 2;
	    }
#if 0
	case MUSE_SCR_FIRE:
	      {
		boolean vis = cansee(mtmp->mx, mtmp->my);

		mreadmsg(mtmp, otmp);
		if (mtmp->mconf) {
			if (vis)
			    pline("Oh, what a pretty fire!");
		} else {
			struct monst *mtmp2;
			int num;

			if (vis)
			    pline_The("scroll erupts in a tower of flame!");
			shieldeff(mtmp->mx, mtmp->my);
			pline("%s is uninjured.", Monnam(mtmp));
			(void) destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
			(void) destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			num = (2*(rn1(3, 3) + 2 * bcsign(otmp)) + 1)/3;
			if (Fire_resistance)
			    You("are not harmed.");
			burn_away_slime();
			if (Half_spell_damage) num = (num+1) / 2;
			else losehp(num, "scroll of fire", KILLED_BY_AN);
			for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon) {
			   if(DEADMONSTER(mtmp2)) continue;
			   if(mtmp == mtmp2) continue;
			   if(dist2(mtmp2->mx,mtmp2->my,mtmp->mx,mtmp->my) < 3){
				if (resists_fire(mtmp2)) continue;
				mtmp2->mhp -= num;
				if (resists_cold(mtmp2))
				    mtmp2->mhp -= 3*num;
				if(mtmp2->mhp < 1) {
				    mondied(mtmp2);
				    break;
				}
			    }
			}
		}
		return 2;
	      }
#endif	/* 0 */
	case MUSE_POT_PARALYSIS:
	case MUSE_POT_BLINDNESS:
	case MUSE_POT_CONFUSION:
	case MUSE_POT_SLEEPING:
	case MUSE_POT_ACID:
	case MUSE_POT_ENERGY_BLOOD:
	case MUSE_POT_STONE_BLOOD:
	case MUSE_MJOLLNIR:
		/* Note: this setting of dknown doesn't suffice.  A monster
		 * which is out of sight might throw and it hits something _in_
		 * sight, a problem not existing with wands because wand rays
		 * are not objects.  Also set dknown in mthrowu.c.
		 */
		if (cansee(mtmp->mx, mtmp->my)) {
			otmp->dknown = 1;
			pline("%s hurls %s!", Monnam(mtmp),
						singular(otmp, doname));
		}
		m_throw(mtmp, mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),
			distmin(mtmp->mx,mtmp->my,mtmp->mux,mtmp->muy), otmp,
			TRUE);
		if(u.ux != mtmp->mux || u.uy != mtmp->muy){
			mtmp->mux = mtmp->muy = 0;
		}
		return 2;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_offense);
		break;
	}
	return 0;
}

int
rnd_offensive_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))], diesize;

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
	if (difficulty > 7 && !rn2(35)) return WAN_DEATH;
	
	diesize = 9;
	if(difficulty < 4) diesize -= 4;
	else if(difficulty < 7) diesize -= (7-difficulty);
	else diesize += 3;
	switch (rn2(diesize)) {
		case 0: {
		    struct obj *helmet = which_armor(mtmp, W_ARMH);

		    if ((helmet && is_hard(helmet)) || amorphous(pm) || passes_walls(pm) || noncorporeal(pm) || unsolid(pm))
			return SCR_EARTH;
		} /* fall through */
		case 1: return WAN_MAGIC_MISSILE;
		case 2: return POT_ACID;
		case 3: return POT_CONFUSION;
		case 4: return POT_BLINDNESS;
		case 5: return POT_SLEEPING;
		case 6: return POT_PARALYSIS;
		case 7: return WAN_STRIKING;
		case 8: return WAN_SLEEP;
		case 9: return WAN_FIRE;
		case 10: return WAN_COLD;
		case 11: return WAN_LIGHTNING;
	}
	/*NOTREACHED*/
	return 0;
}

int
rnd_attack_wand(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
	if (difficulty > 7 && !rn2(35)) return rnd(20) > 10 ? WAN_DRAINING : WAN_DEATH;
	switch (rnd(8)) {
		case 1: return WAN_STRIKING;
		case 2: case 3:
			return WAN_MAGIC_MISSILE;
		case 4: return WAN_SLEEP;
		case 5: return WAN_FIRE;
		case 6: return WAN_COLD;
		case 7: return WAN_LIGHTNING;
		case 8: return WAN_CREATE_MONSTER;
	}
	/*NOTREACHED*/
	return 0;
}

boolean
is_attack_wand(otyp)
int otyp;
{
	switch (otyp) {
		case WAN_STRIKING:
		case WAN_DRAINING:
		case WAN_DEATH:
		case WAN_SLEEP:
		case WAN_MAGIC_MISSILE:
		case WAN_FIRE:
		case WAN_COLD:
		case WAN_LIGHTNING:
		case WAN_CREATE_MONSTER:
		case WAN_TELEPORTATION:
		case WAN_SLOW_MONSTER:
		case WAN_UNDEAD_TURNING:
		case WAN_CANCELLATION:
		case WAN_LOCKING:
		
		return TRUE;
	}
	return FALSE;
}

int
rnd_utility_wand(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
	switch (rnd(6)) {
		case 1: return WAN_SPEED_MONSTER;
		case 2: case 3:
			return WAN_MAKE_INVISIBLE;
		case 4: return WAN_TELEPORTATION;
		case 5: return WAN_DIGGING;
		case 6: return WAN_CREATE_MONSTER;
	}
	/*NOTREACHED*/
	return 0;
}

int
rnd_attack_potion(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
	switch (rnd(6)) {
		case 1:	case 2: 
			return POT_ACID;
		case 3: return POT_CONFUSION;
		case 4: return POT_BLINDNESS;
		case 5: return POT_SLEEPING;
		case 6: return POT_PARALYSIS;
	}
	/*NOTREACHED*/
	return 0;
}

int
rnd_utility_potion(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST || pm->mlet == S_SHADE || pm->mlet == S_KETER
		) return 0;
	switch (rnd(16)) {
		case 1:	return POT_INVISIBILITY;
		case 2:	return POT_INVISIBILITY;
		case 3: return POT_SPEED;
		case 4: return POT_SPEED;
		case 5: return POT_CONFUSION;
		case 6: return POT_CONFUSION;
		case 7: return POT_CONFUSION;
		case 8: return POT_GAIN_LEVEL;
		case 9: return POT_GAIN_LEVEL;
		case 10: return POT_EXTRA_HEALING;
		case 11: return POT_EXTRA_HEALING;
		case 12: return POT_FULL_HEALING;
		case 13: return POT_FULL_HEALING;
		case 14: return POT_FULL_HEALING;
		case 15: return POT_GAIN_ENERGY;
		case 16: return SCR_REMOVE_CURSE;
	}
	/*NOTREACHED*/
	return 0;
}

#define MUSE_POT_GAIN_LEVEL 1
#define MUSE_WAN_MAKE_INVISIBLE 2
#define MUSE_POT_INVISIBILITY 3
#define MUSE_POLY_TRAP 4
#define MUSE_WAN_POLYMORPH 5
#define MUSE_POT_SPEED 6
#define MUSE_WAN_SPEED_MONSTER 7
#define MUSE_BULLWHIP 8
#define MUSE_POT_POLYMORPH 9
#define MUSE_SCR_REMOVE_CURSE 10
#define MUSE_POT_GAIN_ENERGY 11
#define MUSE_SCR_AMNESIA 12
#define MUSE_POT_AMNESIA 13
#define MUSE_POT_GAIN_ABILITY 14

boolean
find_misc(mtmp)
struct monst *mtmp;
{
	register struct obj *obj;
	struct permonst *mdat = mtmp->data;
	int x = mtmp->mx, y = mtmp->my;
	struct trap *t;
	int xx, yy;
	boolean immobile = (mdat->mmove == 0);
	boolean stuck = (mtmp == u.ustuck);
	boolean nomouth = mdat==&mons[PM_NIGHTGAUNT] || ((mtmp->misc_worn_check & W_ARMH) && which_armor(mtmp, W_ARMH) &&
			(((which_armor(mtmp, W_ARMH))->otyp) == PLASTEEL_HELM || ((which_armor(mtmp, W_ARMH))->otyp) == CRYSTAL_HELM || ((which_armor(mtmp, W_ARMH))->otyp) == PONTIFF_S_CROWN))
			 || ((mtmp->misc_worn_check & W_ARMC) && which_armor(mtmp, W_ARMC)
				&& (((which_armor(mtmp, W_ARMC))->otyp) == WHITE_FACELESS_ROBE
					|| ((which_armor(mtmp, W_ARMC))->otyp) == BLACK_FACELESS_ROBE
					|| ((which_armor(mtmp, W_ARMC))->otyp) == SMOKY_VIOLET_FACELESS_ROBE));

	m.misc = (struct obj *)0;
	m.has_misc = 0;
	if (is_animal(mdat) || mindless_mon(mtmp))
		return 0;
	if (u.uswallow && stuck) return FALSE;

	/* We arbitrarily limit to times when a player is nearby for the
	 * same reason as Junior Pac-Man doesn't have energizers eaten until
	 * you can see them...
	 */
	if(dist2(x, y, mtmp->mux, mtmp->muy) > 36)
		return FALSE;

	if (!stuck && !immobile && !mtmp->cham 
	&& (monstr[monsndx(mdat)] + (mtmp->m_lev - mdat->mlevel)) < 6 
	&& !resists_poly(mtmp->data)
	&& !resists_magm(mtmp)
	) {
	  boolean ignore_boulders = (verysmall(mdat) ||
				     throws_rocks(mdat) ||
				     passes_walls(mdat));
	  for(xx = x-1; xx <= x+1; xx++)
	    for(yy = y-1; yy <= y+1; yy++)
		if (isok(xx,yy) && (xx != u.ux || yy != u.uy))
		    if ((mdat != &mons[PM_GRID_BUG] && mtmp->data != &mons[PM_BEBELITH]) || xx == x || yy == y)
			if((mtmp->data != &mons[PM_CLOCKWORK_SOLDIER] && mtmp->data != &mons[PM_CLOCKWORK_DWARF] && 
			   mtmp->data != &mons[PM_FABERGE_SPHERE] && mtmp->data != &mons[PM_FIREWORK_CART] && 
			   mtmp->data != &mons[PM_JUGGERNAUT] && mtmp->data != &mons[PM_ID_JUGGERNAUT]) ||
				(x + xdir[(int)mtmp->mvar1] == xx && 
				   y + ydir[(int)mtmp->mvar1] == yy 
				)
			)
			if (/* (xx==x && yy==y) || */ !level.monsters[xx][yy])
			    if ((t = t_at(xx, yy)) != 0 &&
			      (ignore_boulders || !boulder_at(xx, yy))
			      && !onscary(xx, yy, mtmp)) {
				if (t->ttyp == POLY_TRAP) {
				    trapx = xx;
				    trapy = yy;
				    m.has_misc = MUSE_POLY_TRAP;
				    return TRUE;
				}
			    }
	}
	if (nohands(mdat))
		return 0;

#define nomore(x) if(m.has_misc==x) continue;
	for(obj=mtmp->minvent; obj; obj=obj->nobj) {
		/* Monsters shouldn't recognize cursed items; this kludge is */
		/* necessary to prevent serious problems though... */
		if(!nomouth && obj->otyp == POT_GAIN_LEVEL && (!obj->cursed ||
			    (!mtmp->isgd && !mtmp->isshk && !mtmp->ispriest))) {
			m.misc = obj;
			m.has_misc = MUSE_POT_GAIN_LEVEL;
		}
		nomore(MUSE_POT_GAIN_ENERGY);
		if(!nomouth && obj->otyp == POT_GAIN_ABILITY && (!obj->cursed ||
			    (!mtmp->isgd && !mtmp->isshk && !mtmp->ispriest))) {
			m.misc = obj;
			m.has_misc = MUSE_POT_GAIN_ABILITY;
		}
		nomore(MUSE_POT_GAIN_ABILITY);
		if(!nomouth && (mtmp->mcan || (mtmp->mhp <= .5*(mtmp->mhpmax) && mtmp->mspec_used > 2)) && obj->otyp == POT_GAIN_ENERGY) {
			m.misc = obj;
			m.has_misc = MUSE_POT_GAIN_ENERGY;
		}
		nomore(MUSE_BULLWHIP);
		if(obj->otyp == BULLWHIP && (MON_WEP(mtmp) == obj || MON_SWEP(mtmp) == obj) &&
		   distu(mtmp->mx,mtmp->my)==1 && uwep && !mtmp->mpeaceful) {
			m.misc = obj;
			m.has_misc = MUSE_BULLWHIP;
		}
		/* Note: peaceful/tame monsters won't make themselves
		 * invisible unless you can see them.  Not really right, but...
		 */
		nomore(MUSE_WAN_MAKE_INVISIBLE);
		if(obj->otyp == WAN_MAKE_INVISIBLE && obj->spe > 0 &&
		    !mtmp->minvis && !mtmp->invis_blkd &&
			(!mtmp->mpeaceful || See_invisible(mtmp->mx, mtmp->my)) &&
		    (!attacktype(mtmp->data, AT_GAZE) || mtmp->mcan)) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_MAKE_INVISIBLE;
		}
		nomore(MUSE_POT_INVISIBILITY);
		if(!nomouth && obj->otyp == POT_INVISIBILITY &&
		    !mtmp->minvis && !mtmp->invis_blkd &&
			(!mtmp->mpeaceful || See_invisible(mtmp->mx, mtmp->my)) &&
		    (!attacktype(mtmp->data, AT_GAZE) || mtmp->mcan)) {
			m.misc = obj;
			m.has_misc = MUSE_POT_INVISIBILITY;
		}
		nomore(MUSE_WAN_SPEED_MONSTER);
		if(obj->otyp == WAN_SPEED_MONSTER && obj->spe > 0
				&& mtmp->mspeed != MFAST && !mtmp->isgd) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_SPEED_MONSTER;
		}
		nomore(MUSE_POT_SPEED);
		if(!nomouth && obj->otyp == POT_SPEED && mtmp->mspeed != MFAST
				&& !mtmp->isgd && !resists_poly(mtmp->data)
		) {
			m.misc = obj;
			m.has_misc = MUSE_POT_SPEED;
		}
		nomore(MUSE_WAN_POLYMORPH);
		if(obj->otyp == WAN_POLYMORPH && obj->spe > 0 && !mtmp->cham
				&& monstr[monsndx(mdat)] < 6 && !resists_poly(mtmp->data)
		) {
			m.misc = obj;
			m.has_misc = MUSE_WAN_POLYMORPH;
		}
		nomore(MUSE_POT_POLYMORPH);
		if(!nomouth && obj->otyp == POT_POLYMORPH && !mtmp->cham
				&& monstr[monsndx(mdat)] < 6 && !resists_poly(mtmp->data)
		) {
			m.misc = obj;
			m.has_misc = MUSE_POT_POLYMORPH;
		}
		nomore(MUSE_SCR_REMOVE_CURSE);
		if(obj->otyp == SCR_REMOVE_CURSE)
		{
                        register struct obj *otmp;
			for (otmp = mtmp->minvent;
			     otmp; otmp = otmp->nobj)
			{
			    if (otmp->cursed && 
			        (otmp->otyp == LOADSTONE ||
				 otmp->owornmask))
			    {
			        m.misc = obj;
			        m.has_misc = MUSE_SCR_REMOVE_CURSE;
			    } 
			}
		}
		nomore(MUSE_SCR_AMNESIA);
		nomore(MUSE_POT_AMNESIA);
		if((mtmp->mcrazed || mtmp->mberserk) && (obj->otyp == SCR_AMNESIA)) {
			m.misc = obj;
			m.has_misc = MUSE_SCR_AMNESIA;
		}
		if(!nomouth && (mtmp->mcrazed || mtmp->mberserk) && (obj->otyp == POT_AMNESIA)) {
			m.misc = obj;
			m.has_misc = MUSE_POT_AMNESIA;
		}
	}
	return((boolean)(!!m.has_misc));
#undef nomore
}

/* type of monster to polymorph into; defaults to one suitable for the
   current level rather than the totally arbitrary choice of newcham() */

/* Now not used, was causing weak monsters to poly to very similar weak monsters */
static struct permonst *
muse_newcham_mon(mon)
struct monst *mon;
{
	struct obj *m_armr;

	if ((m_armr = which_armor(mon, W_ARM)) != 0) {
	    if (Is_dragon_scales(m_armr))
		return Dragon_scales_to_pm(m_armr);
	    else if (Is_dragon_mail(m_armr))
		return Dragon_mail_to_pm(m_armr);
	}
	return rndmonst();
}

int
use_misc(mtmp)
struct monst *mtmp;
{
	int i;
	struct obj *otmp = m.misc;
	boolean vis, vismon, oseen;
	char nambuf[BUFSZ];

	if ((i = precheck(mtmp, otmp)) != 0) return i;
	vis = cansee(mtmp->mx, mtmp->my);
	vismon = canseemon(mtmp);
	oseen = otmp && vismon;

	switch(m.has_misc) {
	case MUSE_POT_GAIN_LEVEL:
		mquaffmsg(mtmp, otmp);
		if (otmp->cursed) {
		    if (Can_rise_up(mtmp->mx, mtmp->my, &u.uz)) {
			register int tolev = depth(&u.uz)-1;
			d_level tolevel;

			get_level(&tolevel, tolev);
			/* insurance against future changes... */
			if(on_level(&tolevel, &u.uz)) goto skipmsg;
			if (vismon) {
			    pline("%s rises up, through the %s!",
				  Monnam(mtmp), ceiling(mtmp->mx, mtmp->my));
			    if(!objects[POT_GAIN_LEVEL].oc_name_known
			      && !objects[POT_GAIN_LEVEL].oc_uname)
				docall(otmp);
			}
			m_useup(mtmp, otmp);
			migrate_to_level(mtmp, ledger_no(&tolevel),
					 MIGR_RANDOM, (coord *)0);
			return 2;
		    } else {
skipmsg:
			if (vismon) {
			    pline("%s looks uneasy.", Monnam(mtmp));
			    if(!objects[POT_GAIN_LEVEL].oc_name_known
			      && !objects[POT_GAIN_LEVEL].oc_uname)
				docall(otmp);
			}
			m_useup(mtmp, otmp);
			return 2;
		    }
		}
		if (vismon) pline("%s seems more experienced.", Monnam(mtmp));
		if (oseen) makeknown(POT_GAIN_LEVEL);
		m_useup(mtmp, otmp);
		if (!grow_up(mtmp,(struct monst *)0)) return 1;
			/* grew into genocided monster */
		return 2;
	case MUSE_POT_GAIN_ABILITY:
		mquaffmsg(mtmp, otmp);
		if (otmp->cursed) {
			switch(rnd(6)){
				case 1:
					if(mtmp->mstr > 3) mtmp->mstr--;
				break;
				case 2:
					if(mtmp->mdex > 3) mtmp->mdex--;
				break;
				case 3:
					if(mtmp->mcon > 3) mtmp->mcon--;
				break;
				case 4:
					if(mtmp->mint > 3) mtmp->mint--;
				break;
				case 5:
					if(mtmp->mwis > 3) mtmp->mwis--;
				break;
				case 6:
					if(mtmp->mcha > 3) mtmp->mcha--;
				break;
			}
			if (vismon) pline("%s seems more experienced.", Monnam(mtmp));
			if (oseen) makeknown(POT_GAIN_ABILITY);
		} else if(otmp->blessed){
			if(mtmp->mstr < 25) mtmp->mstr++;
			if(mtmp->mdex < 25) mtmp->mdex++;
			if(mtmp->mcon < 25) mtmp->mcon++;
			if(mtmp->mint < 25) mtmp->mint++;
			if(mtmp->mwis < 25) mtmp->mwis++;
			if(mtmp->mcha < 25) mtmp->mcha++;
		} else {
			switch(rnd(6)){
				case 1:
					if(mtmp->mstr < 25) mtmp->mstr++;
				break;
				case 2:
					if(mtmp->mdex < 25) mtmp->mdex++;
				break;
				case 3:
					if(mtmp->mcon < 25) mtmp->mcon++;
				break;
				case 4:
					if(mtmp->mint < 25) mtmp->mint++;
				break;
				case 5:
					if(mtmp->mwis < 25) mtmp->mwis++;
				break;
				case 6:
					if(mtmp->mcha < 25) mtmp->mcha++;
				break;
			}
		}
		m_useup(mtmp, otmp);
		if (!grow_up(mtmp,(struct monst *)0)) return 1;
			/* grew into genocided monster */
		return 2;
	case MUSE_WAN_MAKE_INVISIBLE:
	case MUSE_POT_INVISIBILITY:
		if (otmp->otyp == WAN_MAKE_INVISIBLE) {
		    mzapmsg(mtmp, otmp, TRUE);
		    otmp->spe--;
		} else
		    mquaffmsg(mtmp, otmp);
		/* format monster's name before altering its visibility */
		Strcpy(nambuf, See_invisible(mtmp->mx, mtmp->my) ? Monnam(mtmp) : mon_nam(mtmp));
		mon_set_minvis(mtmp);
		if (vismon && mtmp->minvis) {	/* was seen, now invisible */
			if (See_invisible(mtmp->mx, mtmp->my))
			pline("%s body takes on a %s transparency.",
			      s_suffix(nambuf),
			      Hallucination ? "normal" : "strange");
		    else
			pline("Suddenly you cannot see %s.", nambuf);
		    if (oseen) makeknown(otmp->otyp);
		}
		if (otmp->otyp == POT_INVISIBILITY) {
		    if (otmp->cursed) you_aggravate(mtmp);
		    m_useup(mtmp, otmp);
		}
		return 2;
	case MUSE_WAN_SPEED_MONSTER:
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		mon_adjust_speed(mtmp, 1, otmp);
		return 2;
	case MUSE_POT_SPEED:
		mquaffmsg(mtmp, otmp);
		/* note difference in potion effect due to substantially
		   different methods of maintaining speed ratings:
		   player's character becomes "very fast" temporarily;
		   monster becomes "one stage faster" permanently */
		mon_adjust_speed(mtmp, 1, otmp);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_GAIN_ENERGY:
		mquaffmsg(mtmp, otmp);
		if(!otmp->cursed){
			if (vismon) pline("%s looks lackluster.", Monnam(mtmp));
			mtmp->mspec_used = 0;
			mtmp->mcan = 0;
		} else {
			if (vismon) pline("%s looks full of energy.", Monnam(mtmp));
			mtmp->mcan = 1;
		}
		if (oseen) makeknown(POT_GAIN_ENERGY);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POT_AMNESIA:
		mquaffmsg(mtmp, otmp);
		if (oseen) makeknown(POT_AMNESIA);
		goto museamnesia;
	case MUSE_SCR_AMNESIA:
		mreadmsg(mtmp, otmp);
		if (oseen) makeknown(SCR_AMNESIA);
museamnesia:
		if(!otmp->cursed){
			if (vismon) pline("%s looks more tranquil.", Monnam(mtmp));
			if(!otmp->blessed){
				mtmp->mtame = 0;
				mtmp->mferal = 0;
				mtmp->mpeaceful = 1;
			}
			mtmp->mcrazed = 0;
			mtmp->mberserk = 0;
		} else {
			if (vismon) pline("%s looks angry and confused!", Monnam(mtmp));
			mtmp->mcrazed = 1;
			mtmp->mberserk = 1;
			mtmp->mconf = 1;
			mtmp->mtame = 0;
			mtmp->mferal = 0;
			mtmp->mpeaceful = 0;
		}
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_WAN_POLYMORPH:
		mzapmsg(mtmp, otmp, TRUE);
		otmp->spe--;
		if(!resists_poly(mtmp->data)) newcham(mtmp, (struct permonst *) 0, TRUE, FALSE);
		if (oseen) makeknown(WAN_POLYMORPH);
		return 2;
	case MUSE_POT_POLYMORPH:
		mquaffmsg(mtmp, otmp);
		if (vismon) pline("%s suddenly mutates!", Monnam(mtmp));
		if(!resists_poly(mtmp->data)) newcham(mtmp, (struct permonst *) 0, FALSE, FALSE);
		if (oseen) makeknown(POT_POLYMORPH);
		m_useup(mtmp, otmp);
		return 2;
	case MUSE_POLY_TRAP:
		if (vismon)
		    pline("%s deliberately %s onto a polymorph trap!",
			Monnam(mtmp),
			makeplural(locomotion(mtmp->data, "jump")));
		if (vis) seetrap(t_at(trapx,trapy));

		/*  don't use rloc() due to worms */
		remove_monster(mtmp->mx, mtmp->my);
		newsym(mtmp->mx, mtmp->my);
		place_monster(mtmp, trapx, trapy);
		if (mtmp->wormno) worm_move(mtmp);
		newsym(trapx, trapy);
		
		if(resists_magm(mtmp)) shieldeff(mtmp->mx, mtmp->my);
		else if(!resists_poly(mtmp->data)) newcham(mtmp, (struct permonst *)0, FALSE, FALSE);
		return 2;
	case MUSE_BULLWHIP:
		/* attempt to disarm hero */
		if (uwep && !rn2(5)) {
		    const char *The_whip = vismon ? "The bullwhip" : "A whip";
		    int where_to = rn2(4);
		    struct obj *obj = uwep;
		    const char *hand;
		    char the_weapon[BUFSZ];

		    Strcpy(the_weapon, the(xname(obj)));
		    hand = body_part(HAND);
		    if (bimanual(obj,youracedata)) hand = makeplural(hand);

		    if (vismon)
			pline("%s flicks a bullwhip towards your %s!",
			      Monnam(mtmp), hand);
		    if (obj->otyp == HEAVY_IRON_BALL) {
			pline("%s fails to wrap around %s.",
			      The_whip, the_weapon);
			return 1;
		    }
		    pline("%s wraps around %s you're wielding!",
			  The_whip, the_weapon);
		    if (welded(obj)) {
			pline("%s welded to your %s%c",
			      !is_plural(obj) ? "It is" : "They are",
			      hand, !obj->bknown ? '!' : '.');
			/* obj->bknown = 1; */ /* welded() takes care of this */
			where_to = 0;
		    }
		    if (!where_to) {
			pline_The("whip slips free.");  /* not `The_whip' */
			return 1;
		    } else if (where_to == 3 && hates_silver(mtmp->data) &&
			    (obj->obj_material == SILVER || arti_silvered(obj))) {
			/* this monster won't want to catch a silver
			   weapon; drop it at hero's feet instead */
			where_to = 2;
		    } else if (where_to == 3 && hates_iron(mtmp->data) &&
			    obj->obj_material == IRON) {
			/* this monster won't want to catch an iron
			   weapon; drop it at hero's feet instead */
			where_to = 2;
		    } else if (where_to == 3 && hates_unholy(mtmp->data) &&
			    is_unholy(obj)) {
			/* this monster won't want to catch a cursed
			   weapon; drop it at hero's feet instead */
			where_to = 2;
		    }
		    freeinv(obj);
		    uwepgone();
		    switch (where_to) {
			case 1:		/* onto floor beneath mon */
			    pline("%s yanks %s from your %s!", Monnam(mtmp),
				  the_weapon, hand);
			    place_object(obj, mtmp->mx, mtmp->my);
			    break;
			case 2:		/* onto floor beneath you */
			    pline("%s yanks %s to the %s!", Monnam(mtmp),
				  the_weapon, surface(u.ux, u.uy));
			    dropy(obj);
			    break;
			case 3:		/* into mon's inventory */
			    pline("%s snatches %s!", Monnam(mtmp),
				  the_weapon);
			    (void) mpickobj(mtmp,obj);
			    break;
		    }
		    return 1;
		}
		return 0;
	case MUSE_SCR_REMOVE_CURSE:
		mreadmsg(mtmp, otmp);
		if (canseemon(mtmp))
		{
		    if (mtmp->mconf)
		        You("feel as though %s needs some help.",
			    mon_nam(mtmp));
		    else
		        You("feel like someone is helping %s.", mon_nam(mtmp));
		    if(!objects[SCR_REMOVE_CURSE].oc_name_known
		      && !objects[SCR_REMOVE_CURSE].oc_uname)
		        docall(otmp);
		}
		{
		    register struct obj *obj;
		    for (obj = mtmp->minvent; obj; obj = obj->nobj)
		    {
#ifdef GOLDOBJ
			/* gold isn't subject to cursing and blessing */
			if (obj->oclass == COIN_CLASS) continue;
#endif
			if (otmp->blessed || otmp->owornmask ||
			     obj->otyp == LOADSTONE) {
			    if(mtmp->mconf) blessorcurse(obj, 2);
			    else uncurse(obj);
			}
		    }
		}
		m_useup(mtmp, otmp);
	        return 0;
	case 0: return 0; /* i.e. an exploded wand */
	default: impossible("%s wanted to perform action %d?", Monnam(mtmp),
			m.has_misc);
		break;
	}
	return 0;
}

void
you_aggravate(mtmp)
struct monst *mtmp;
{
	register struct monst *tmpm;

	for(tmpm = fmon; tmpm; tmpm = tmpm->nmon)
	    if (!DEADMONSTER(tmpm) && mtmp != tmpm) {
			tmpm->msleeping = 0;
			if(!tmpm->mcanmove && !rn2(5)) {
				tmpm->mfrozen = 0;
				if(tmpm->data != &mons[PM_GIANT_TURTLE] || !(tmpm->mflee))
					tmpm->mcanmove = 1;
			}
			tmpm->mux = mtmp->mx;
			tmpm->muy = mtmp->my;
	    }
	pline("For some reason, %s presence is known to you.",
		s_suffix(noit_mon_nam(mtmp)));
	cls();
#ifdef CLIPPING
	cliparound(mtmp->mx, mtmp->my);
#endif
	show_glyph(mtmp->mx, mtmp->my, mon_to_glyph(mtmp));
	display_self();
	You_feel("aggravated at %s.", noit_mon_nam(mtmp));
	display_nhwindow(WIN_MAP, TRUE);
	docrt();
	if (unconscious()) {
		multi = -1;
		nomovemsg =
		      "Aggravated, you are jolted into full consciousness.";
	}
	newsym(mtmp->mx,mtmp->my);
	if (!canspotmon(mtmp))
	    map_invisible(mtmp->mx, mtmp->my);
}

int
rnd_misc_item(mtmp)
struct monst *mtmp;
{
	struct permonst *pm = mtmp->data;
	int difficulty = monstr[(monsndx(pm))];

	if(is_animal(pm) || attacktype(pm, AT_EXPL) || mindless_mon(mtmp)
			|| pm->mlet == S_GHOST
			|| pm->mlet == S_SHADE
			|| pm->mlet == S_KETER
		) return 0;
	/* Unlike other rnd_item functions, we only allow _weak_ monsters
	 * to have this item; after all, the item will be used to strengthen
	 * the monster and strong monsters won't use it at all...
	 */
	if (difficulty < 6 && !rn2(30))
	    return rn2(6) ? POT_POLYMORPH : WAN_POLYMORPH;
	
	if (!rn2(40) && !nonliving_mon(mtmp)) return AMULET_OF_LIFE_SAVING;

	if(difficulty > 6 && rn2(50) < difficulty) return rnd_utility_potion(mtmp);

	switch (rn2(3)) {
		case 0:
			if (mtmp->isgd) return 0;
			return rn2(6) ? POT_SPEED : WAN_SPEED_MONSTER;
		case 1:
			if (mtmp->mpeaceful && !See_invisible(u.ux,u.uy)) return 0;
			return rn2(6) ? POT_INVISIBILITY : WAN_MAKE_INVISIBLE;
		case 2:
			return POT_GAIN_LEVEL;
	}
	/*NOTREACHED*/
	return 0;
}

boolean
searches_for_item(mon, obj)
struct monst *mon;
struct obj *obj;
{
	int typ = obj->otyp;

	if (is_animal(mon->data) ||
		mindless_mon(mon) ||
		mon->data == &mons[PM_SHADE] ||
		mon->data == &mons[PM_BROKEN_SHADOW] ||
		mon->data == &mons[PM_GHOST])	/* don't loot bones piles */
	    return FALSE;

	if (typ == WAN_MAKE_INVISIBLE || typ == POT_INVISIBILITY)
	    return (boolean)(!mon->minvis && !mon->invis_blkd && !attacktype(mon->data, AT_GAZE));
	if (typ == WAN_SPEED_MONSTER || typ == POT_SPEED)
	    return (boolean)(mon->mspeed != MFAST);

	switch (obj->oclass) {
	case WAND_CLASS:
	    if (obj->spe <= 0)
		return FALSE;
	    if (typ == WAN_DIGGING)
		return (boolean)(!is_floater(mon->data));
	    if (typ == WAN_POLYMORPH)
		return (boolean)(monstr[monsndx(mon->data)] < 6);
	    if (objects[typ].oc_dir == RAY ||
		    typ == WAN_STRIKING ||
		    typ == WAN_TELEPORTATION ||
		    typ == WAN_CREATE_MONSTER ||
		    typ == WAN_DRAINING	||
		    typ == WAN_CANCELLATION)
		return TRUE;
	    break;
	case POTION_CLASS:
	    if (typ == POT_HEALING ||
		    typ == POT_EXTRA_HEALING ||
		    typ == POT_FULL_HEALING ||
		    typ == POT_POLYMORPH ||
		    typ == POT_GAIN_LEVEL ||
		    typ == POT_PARALYSIS ||
		    typ == POT_SLEEPING ||
		    typ == POT_ACID ||
		    typ == POT_GAIN_ENERGY ||
		    typ == POT_AMNESIA ||
		    typ == POT_CONFUSION)
		return TRUE;
	    if (typ == POT_BLINDNESS && !attacktype(mon->data, AT_GAZE))
		return TRUE;
	    break;
	case SCROLL_CLASS:
	    if (typ == SCR_TELEPORTATION || typ == SCR_CREATE_MONSTER
		    || typ == SCR_EARTH
		    || typ == SCR_AMNESIA
		    || typ == SCR_REMOVE_CURSE)
		return TRUE;
	    break;
	case AMULET_CLASS:
	    if (typ == AMULET_OF_LIFE_SAVING)
		return (boolean)(!nonliving_mon(mon));
	    if (typ == AMULET_OF_REFLECTION)
		return TRUE;
	    break;
	case TOOL_CLASS:
	    if (typ == PICK_AXE)
		return (boolean)needspick(mon->data);
	    if (typ == UNICORN_HORN)
		return (boolean)(!obj->cursed && !is_unicorn(mon->data));
	    if (typ == FROST_HORN || typ == FIRE_HORN)
		return (obj->spe > 0);
	    break;
	case FOOD_CLASS:
	    if (typ == CORPSE)
		return (boolean)(((mon->misc_worn_check & W_ARMG) &&
				    touch_petrifies(&mons[obj->corpsenm])) ||
				(!resists_ston(mon) &&
				    (obj->corpsenm == PM_LIZARD ||
					(acidic(&mons[obj->corpsenm]) &&
					 obj->corpsenm != PM_GREEN_SLIME &&
					 obj->corpsenm != PM_FLUX_SLIME))));
	    if (typ == EGG)
		return (boolean)(touch_petrifies(&mons[obj->corpsenm]));
	    break;
	default:
	    break;
	}

	return FALSE;
}

boolean
mon_reflects(mon,str)
struct monst *mon;
const char *str;
{
	struct obj *orefl = which_armor(mon, W_ARMS);

	if (orefl &&
		(orefl->otyp == SHIELD_OF_REFLECTION ||
		 orefl->otyp == SILVER_DRAGON_SCALE_SHIELD)) {
	    if (str) {
		pline(str, s_suffix(mon_nam(mon)), "shield");
		makeknown(orefl->otyp);
	    }
	    return TRUE;
	} else if (arti_reflects(MON_WEP(mon))) {
	    /* due to wielded artifact weapon */
	    if (str)
		pline(str, s_suffix(mon_nam(mon)), "weapon");
	    return TRUE;
	} else if ((orefl = which_armor(mon, W_AMUL)) &&
				orefl->otyp == AMULET_OF_REFLECTION) {
	    if (str) {
		pline(str, s_suffix(mon_nam(mon)), "amulet");
		makeknown(AMULET_OF_REFLECTION);
	    }
	    return TRUE;
	} else if ((orefl = which_armor(mon, W_ARM)) &&
		(orefl->otyp == SILVER_DRAGON_SCALES || orefl->otyp == SILVER_DRAGON_SCALE_MAIL)) {
	    if (str)
		pline(str, s_suffix(mon_nam(mon)), "armor");
	    return TRUE;
	} else if ((orefl = which_armor(mon, W_SADDLE)) &&
				arti_reflects(orefl)) {
	    if (str) {
		pline(str, s_suffix(mon_nam(mon)), "saddle");
	    }
	    return TRUE;
	} else if(mon->mfaction == FRACTURED){
		if(str) 
		pline(str, s_suffix(mon_nam(mon)), "fractured surface");
		return TRUE;
	} else if (species_reflects(mon)){
		if (str)
		{
			switch (monsndx(mon->data))
			{
			case PM_SILVER_DRAGON:
			case PM_PLATINUM_DRAGON:
				pline(str, s_suffix(mon_nam(mon)), "scales");
				break;
			case PM_AOA:
			case PM_AOA_DROPLET:
			case PM_ARGENTUM_GOLEM:
			case PM_CENTER_OF_ALL:
				pline(str, s_suffix(mon_nam(mon)), "surface");
				break;
			case PM_BAALPHEGOR:
			case PM_HOD_SEPHIRAH:
				pline(str, s_suffix(mon_nam(mon)), "armor");
				break;
			case PM_AMM_KAMEREL:
				pline(str, s_suffix(mon_nam(mon)), "glassy skin");
				break;
			case PM_HUDOR_KAMEREL:
				pline(str, s_suffix(mon_nam(mon)), "watery skin");
				break;
			case PM_SHARAB_KAMEREL:
				pline(str, s_suffix(mon_nam(mon)), "shimmering aura");
				break;
			case PM_ARA_KAMEREL:
				pline(str, s_suffix(mon_nam(mon)), "gold mirrored arcs");
				break;
			default:
				impossible("Reflecting monster not listed in mon_reflects(): %s", mon_nam(mon));
				break;
			}
		}
		return TRUE;
	}
	return FALSE;
}

boolean
ureflects (fmt, str)
const char *fmt, *str;
{
	/* Check from outermost to innermost objects */
	if (EReflecting & W_ARMS) {
	    if (fmt && str) {
	    	pline(fmt, str, "shield");
	    	if(uarms && uarms->otyp == SHIELD_OF_REFLECTION) makeknown(SHIELD_OF_REFLECTION);
	    }
	    return TRUE;
	} else if (EReflecting & W_WEP) {
	    /* Due to wielded artifact weapon */
	    if (fmt && str)
	    	pline(fmt, str, "weapon");
	    return TRUE;
	} else if (uwep && is_lightsaber(uwep) && litsaber(uwep) && ((u.fightingForm == FFORM_SORESU && (!uarm || is_light_armor(uarm))) || ((u.fightingForm == FFORM_SHIEN) && (!uarm || is_light_armor(uarm) || is_medium_armor(uarm))))) {
	    /* Due to wielded lightsaber */
	    if (fmt && str)
	    	pline(fmt, str, "lightsaber");
	    return TRUE;
	} else if (EReflecting & W_ARMC) {
	    if (fmt && str) {
	    	pline(fmt, str, "cloak");
	    }
	    return TRUE;
	} else if (EReflecting & W_AMUL) {
	    if (fmt && str) {
	    	pline(fmt, str, "medallion");
	    	if(uamul && uamul->otyp == AMULET_OF_REFLECTION) makeknown(AMULET_OF_REFLECTION);
	    }
	    return TRUE;
	} else if (EReflecting & W_ARMG) {
	    if (fmt && str) {
	    	pline(fmt, str, "gauntlets");
	    }
	    return TRUE;
	} else if (EReflecting & (W_RINGL|W_RINGR)) {
	    if (fmt && str) {
	    	pline(fmt, str, "ring");
	    }
	    return TRUE;
	} else if (EReflecting & W_ARMH) {
	    if (fmt && str) {
	    	pline(fmt, str, "helm");
	    }
	    return TRUE;
	} else if (EReflecting & W_ARM) {
	    if (fmt && str)
	    	pline(fmt, str, "armor");
	    return TRUE;
	} else if (EReflecting & W_ARMU) {
	    if (fmt && str)
	    	pline(fmt, str, "underclothes");
	    return TRUE;
	} else if (EReflecting & W_ARMF) {
	    if (fmt && str)
	    	pline(fmt, str, "boots");
	    return TRUE;
	} else if (youracedata == &mons[PM_SILVER_DRAGON]) {
	    if (fmt && str)
	    	pline(fmt, str, "scales");
	    return TRUE;
	} else if (u.sealsActive&SEAL_EDEN) {
	    if (fmt && str)
	    	pline(fmt, str, "skin");
	    return TRUE;
	} else if (EReflecting) {
	    /* Catchall */
	    if (fmt && str)
	    	pline(fmt, str, "artifact");
	    return TRUE;
	}
	return FALSE;
}


/* TRUE if the monster ate something */
boolean
munstone(mon, by_you)
struct monst *mon;
boolean by_you;
{
	struct obj *obj;

	if (resists_ston(mon)) return FALSE;
	if (mon->meating || !mon->mcanmove || !mon->mnotlaugh || mon->msleeping) return FALSE;

	for(obj = mon->minvent; obj; obj = obj->nobj) {
	    /* Monsters can also use potions of acid */
	    if ((obj->otyp == POT_ACID) || (obj->otyp == CORPSE &&
	    		(obj->corpsenm == PM_LIZARD || (acidic(&mons[obj->corpsenm]) && obj->corpsenm != PM_GREEN_SLIME && obj->corpsenm != PM_FLUX_SLIME)))) {
		mon_consume_unstone(mon, obj, by_you, TRUE);
		return TRUE;
	    }
	}
	return FALSE;
}

STATIC_OVL void
mon_consume_unstone(mon, obj, by_you, stoning)
struct monst *mon;
struct obj *obj;
boolean by_you;
boolean stoning;
{
    int nutrit = (obj->otyp == CORPSE) ? dog_nutrition(mon, obj) : 0;
    /* also sets meating */

    /* give a "<mon> is slowing down" message and also remove
       intrinsic speed (comparable to similar effect on the hero) */
    mon_adjust_speed(mon, -3, (struct obj *)0);

    if (canseemon(mon)) {
	long save_quan = obj->quan;

	obj->quan = 1L;
	pline("%s %ss %s.", Monnam(mon),
		    (obj->otyp == POT_ACID) ? "quaff" : "eat",
		    distant_name(obj,doname));
	obj->quan = save_quan;
    } else if (flags.soundok)
	You_hear("%s.", (obj->otyp == POT_ACID) ? "drinking" : "chewing");
    if (((obj->otyp == POT_ACID) || acidic(&mons[obj->corpsenm])) &&
		    !resists_acid(mon)) {
		mon->mhp -= rnd(15);
		pline("%s has a very bad case of stomach acid.",
			Monnam(mon));
    }
    m_useup(mon, obj);
    if (mon->mhp <= 0) {
	pline("%s dies!", Monnam(mon));
	if (by_you) xkilled(mon, 0);
	else mondead(mon);
	return;
    }
    if (stoning && canseemon(mon)) {
	if (Hallucination)
    pline("What a pity - %s just ruined a future piece of art!",
	    mon_nam(mon));
	else
	    pline("%s seems limber!", Monnam(mon));
    }
    if (obj->otyp == CORPSE && obj->corpsenm == PM_LIZARD && mon->mconf) {
	mon->mconf = 0;
	if (canseemon(mon))
	    pline("%s seems steadier now.", Monnam(mon));
    }
    if (mon->mtame && !mon->isminion && nutrit > 0) {
	struct edog *edog = EDOG(mon);

	if (edog->hungrytime < monstermoves) edog->hungrytime = monstermoves;
	edog->hungrytime += nutrit;
	mon->mconf = 0;
    }
    mon->mlstmv = monstermoves; /* it takes a turn */
}

/*muse.c*/
