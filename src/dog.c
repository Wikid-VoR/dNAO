/*	SCCS Id: @(#)dog.c	3.4	2002/09/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "edog.h"
#include "emin.h"
#include "epri.h"

#ifdef OVLB

STATIC_DCL int NDECL(pet_type);

void
initedog(mtmp)
register struct monst *mtmp;
{
	if(mtmp->data == &mons[PM_SURYA_DEVA]){
		struct monst *blade;
		for(blade = fmon; blade; blade = blade->nmon) if(blade->data == &mons[PM_DANCING_BLADE] && mtmp->m_id == blade->mvar1) break;
		if(blade && !blade->mtame) tamedog(blade, (struct obj *) 0);
	}
	
	mtmp->mtame = is_domestic(mtmp->data) ? 10 : 5;
	mtmp->mpeaceful = 1;
	mtmp->mavenge = 0;
	set_malign(mtmp); /* recalc alignment now that it's tamed */
	mtmp->mleashed = 0;
	mtmp->meating = 0;
	EDOG(mtmp)->droptime = 0;
	EDOG(mtmp)->dropdist = 10000;
	EDOG(mtmp)->apport = ACURR(A_CHA);
	EDOG(mtmp)->whistletime = 0;
	EDOG(mtmp)->hungrytime = 1000 + monstermoves;
	EDOG(mtmp)->ogoal.x = -1;	/* force error if used before set */
	EDOG(mtmp)->ogoal.y = -1;
	EDOG(mtmp)->abuse = 0;
	EDOG(mtmp)->revivals = 0;
	EDOG(mtmp)->mhpmax_penalty = 0;
	EDOG(mtmp)->killed_by_u = 0;
//ifdef BARD
	EDOG(mtmp)->friend = 0;
	EDOG(mtmp)->waspeaceful = 0;
//endif
	EDOG(mtmp)->loyal = 0;
}

STATIC_OVL int
pet_type()
{
	if(Race_if(PM_DROW)){
		if(Role_if(PM_NOBLEMAN)){
			if(flags.initgend) return (PM_GIANT_SPIDER);
			else return (PM_SMALL_CAVE_LIZARD);
		}
		if(preferred_pet == 's')
			return (PM_CAVE_SPIDER);
		else if(preferred_pet == ':' || preferred_pet == 'l')
			return (PM_BABY_CAVE_LIZARD);
		else 
			return (rn2(3) ? PM_CAVE_SPIDER : PM_BABY_CAVE_LIZARD);
	}
	else if(Race_if(PM_HALF_DRAGON) && Role_if(PM_NOBLEMAN)){
		return (PM_UNDEAD_KNIGHT);
		// if(flags.initgend) 
		// else 
	} else if(Race_if(PM_CHIROPTERAN))
		return PM_GIANT_BAT;
	else if (urole.petnum != NON_PM && urole.petnum != PM_LITTLE_DOG && urole.petnum != PM_KITTEN && urole.petnum != PM_PONY)
	    return (urole.petnum);
	else if(Race_if(PM_HALF_DRAGON))
		return urole.petnum == PM_PONY ? PM_RIDING_PSEUDODRAGON : PM_TINY_PSEUDODRAGON;
	else if (Role_if(PM_PIRATE)) {
		if (preferred_pet == 'B')
			return (PM_PARROT);
		else if(preferred_pet == 'Y')
			return PM_MONKEY;
		else
			return (rn2(2) ? PM_PARROT : PM_MONKEY);
	}
	else if (preferred_pet == 'c' || preferred_pet == 'f')
	    return (PM_KITTEN);
	else if (preferred_pet == 'd')
	    return (PM_LITTLE_DOG);
	else if (urole.petnum != NON_PM)
	    return (urole.petnum);
	else
	    return (rn2(2) ? PM_KITTEN : PM_LITTLE_DOG);
}

struct monst *
make_familiar(otmp,x,y,quietly)
register struct obj *otmp;
xchar x, y;
boolean quietly;
{
	struct permonst *pm;
	struct monst *mtmp = 0;
	int chance, trycnt = 100;

	do {
	    if (otmp) {	/* figurine; otherwise spell */
		int mndx = otmp->corpsenm;
		pm = &mons[mndx];
		/* activating a figurine provides one way to exceed the
		   maximum number of the target critter created--unless
		   it has a special limit (erinys, Nazgul) */
		if ((mvitals[mndx].mvflags & G_EXTINCT) &&
			mbirth_limit(mndx) < MAXMONNO) {
		    if (!quietly)
			/* have just been given "You <do something with>
			   the figurine and it transforms." message */
			pline("... into a pile of dust.");
		    break;	/* mtmp is null */
		}
	    } else if (!rn2(3)) {
		pm = &mons[pet_type()];
	    } else {
		pm = rndmonst();
		if (!pm) {
		  if (!quietly)
		    There("seems to be nothing available for a familiar.");
		  break;
		}
		if(stationary(pm) || sessile(pm))
			continue;
	    }

	    mtmp = makemon(pm, x, y, MM_EDOG|MM_IGNOREWATER);
	    if (otmp && !mtmp) { /* monster was genocided or square occupied */
	 	if (!quietly)
		   pline_The("figurine writhes and then shatters into pieces!");
		break;
	    }
	} while (!mtmp && --trycnt > 0);

	if (!mtmp) return (struct monst *)0;

	if (is_pool(mtmp->mx, mtmp->my, FALSE) && minliquid(mtmp))
		return (struct monst *)0;

	initedog(mtmp);
	mtmp->msleeping = 0;
	if (otmp) { /* figurine; resulting monster might not become a pet */
	    chance = rn2(10);	/* 0==tame, 1==peaceful, 2==hostile */
	    if (chance > 2) chance = otmp->blessed ? 0 : !otmp->cursed ? 1 : 2;
	    /* 0,1,2:  b=80%,10,10; nc=10%,80,10; c=10%,10,80 */
	    if (chance > 0 && 
			!(Role_if(PM_BARD) && rnd(20) < ACURR(A_CHA) && 
				!(is_animal(mtmp->data) || mindless_mon(mtmp)))
		) {

		mtmp->mtame = 0;	/* not tame after all */
		if (chance == 2) { /* hostile (cursed figurine) */
		    if (!quietly)
		       You("get a bad feeling about this.");
		    mtmp->mpeaceful = 0;
		    set_malign(mtmp);
		}
	    }
	    /* if figurine has been named, give same name to the monster */
	    if (otmp->onamelth)
		mtmp = christen_monst(mtmp, ONAME(otmp));
	}
	set_malign(mtmp); /* more alignment changes */
	newsym(mtmp->mx, mtmp->my);

	/* must wield weapon immediately since pets will otherwise drop it */
	if (mtmp->mtame && attacktype(mtmp->data, AT_WEAP)) {
		mtmp->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp);
	}
	return mtmp;
}

struct monst *
makedog()
{
	register struct monst *mtmp;
#ifdef STEED
	register struct obj *otmp;
#endif
	const char *petname;
	int   pettype;
	static int petname_used = 0;

	if (preferred_pet == 'n' || Role_if(PM_ANACHRONONAUT)) return((struct monst *) 0);

	pettype = pet_type();
	if (pettype == PM_LITTLE_DOG)
		petname = dogname;
	else if (pettype == PM_PONY)
		petname = horsename;
	else if (pettype == PM_PARROT)
		petname = parrotname;
	else if (pettype == PM_MONKEY)
		petname = monkeyname;
	else if(is_spider(&mons[pettype]))
		petname = spidername;
	else if(is_dragon(&mons[pettype]))
		petname = dragonname;
	else if(mons[pettype].mlet == S_LIZARD)
		petname = lizardname;
#ifdef CONVICT
	else if (pettype == PM_SEWER_RAT)
		petname = ratname;
#endif /* CONVICT */
	else
		petname = catname;

	/* default pet names */
	if (!*petname && pettype == PM_LITTLE_DOG) {
	    /* All of these names were for dogs. */
	    if(Role_if(PM_CAVEMAN)) petname = "Slasher";   /* The Warrior */
	    if(Role_if(PM_SAMURAI)) petname = "Hachi";     /* Shibuya Station */
	    if(Role_if(PM_BARBARIAN)) petname = "Idefix";  /* Obelix */
	    if(Role_if(PM_RANGER)) petname = "Sirius";     /* Orion's dog */
	}

	if (!*petname && pettype == PM_KITTEN) {
	    if(Role_if(PM_NOBLEMAN)) petname = "Puss";   /* Puss in Boots */
	    if(Role_if(PM_WIZARD) && flags.female) petname = rn2(2) ? "Crookshanks" : "Greebo";     /* Hermione and Nanny Ogg */
	    if(Role_if(PM_WIZARD) && !flags.female) petname = rn2(2) ? "Tom Kitten" : "Mister";     /* Beatrix Potter and Harry Dresden */
	}

#ifdef CONVICT
	if (!*petname && pettype == PM_SEWER_RAT) {
	    if(Role_if(PM_CONVICT)) petname = "Nicodemus"; /* Rats of NIMH */
    }
#endif /* CONVICT */
	mtmp = makemon(&mons[pettype], u.ux, u.uy, MM_EDOG);

	if(!mtmp) return((struct monst *) 0); /* pets were genocided */
	
	if(mtmp->m_lev < mtmp->data->mlevel) mtmp->m_lev = mtmp->data->mlevel;
	
	if(mtmp->m_lev) mtmp->mhpmax = 8*(mtmp->m_lev-1)+rnd(8);
	mtmp->mhp = mtmp->mhpmax;

#ifdef STEED
	/* Horses already wear a saddle */
	if ((pettype == PM_PONY || pettype == PM_GIANT_SPIDER || pettype == PM_SMALL_CAVE_LIZARD || pettype == PM_RIDING_PSEUDODRAGON)
		&& !!(otmp = mksobj(SADDLE, TRUE, FALSE))
	) {
	    if (mpickobj(mtmp, otmp))
		panic("merged saddle?");
	    mtmp->misc_worn_check |= W_SADDLE;
	    otmp->dknown = otmp->bknown = otmp->rknown = 1;
	    otmp->owornmask = W_SADDLE;
	    otmp->leashmon = mtmp->m_id;
	    update_mon_intrinsics(mtmp, otmp, TRUE, TRUE);
	}
#endif

	if (!petname_used++ && *petname)
		mtmp = christen_monst(mtmp, petname);

	initedog(mtmp);
	EDOG(mtmp)->loyal = TRUE;
	if(is_half_dragon(mtmp->data) && flags.HDbreath){
		switch(mtmp->mvar1){
			case AD_COLD:
				mtmp->mintrinsics[(COLD_RES-1)/32] &= ~(1 << (COLD_RES-1)%32);
			break;
			case AD_FIRE:
				mtmp->mintrinsics[(FIRE_RES-1)/32] &= ~(1 << (FIRE_RES-1)%32);
			break;
			case AD_SLEE:
				mtmp->mintrinsics[(SLEEP_RES-1)/32] &= ~(1 << (SLEEP_RES-1)%32);
			break;
			case AD_ELEC:
				mtmp->mintrinsics[(SHOCK_RES-1)/32] &= ~(1 << (SHOCK_RES-1)%32);
			break;
			case AD_DRST:
				mtmp->mintrinsics[(POISON_RES-1)/32] &= ~(1 << (POISON_RES-1)%32);
			break;
			case AD_ACID:
				mtmp->mintrinsics[(ACID_RES-1)/32] &= ~(1 << (ACID_RES-1)%32);
			break;
		}
		switch(flags.HDbreath){
			case AD_COLD:
				mtmp->mvar1 = AD_COLD;
				mtmp->mintrinsics[(COLD_RES-1)/32] |= (1 << (COLD_RES-1)%32);
			break;
			case AD_FIRE:
				mtmp->mvar1 = AD_FIRE;
				mtmp->mintrinsics[(FIRE_RES-1)/32] |= (1 << (FIRE_RES-1)%32);
			break;
			case AD_SLEE:
				mtmp->mvar1 = AD_SLEE;
				mtmp->mintrinsics[(SLEEP_RES-1)/32] |= (1 << (SLEEP_RES-1)%32);
			break;
			case AD_ELEC:
				mtmp->mvar1 = AD_ELEC;
				mtmp->mintrinsics[(SHOCK_RES-1)/32] |= (1 << (SHOCK_RES-1)%32);
			break;
			case AD_DRST:
				mtmp->mvar1 = AD_DRST;
				mtmp->mintrinsics[(POISON_RES-1)/32] |= (1 << (POISON_RES-1)%32);
			break;
			case AD_ACID:
				mtmp->mvar1 = AD_ACID;
				mtmp->mintrinsics[(ACID_RES-1)/32] |= (1 << (ACID_RES-1)%32);
			break;
		}
	}
	return(mtmp);
}

/* record `last move time' for all monsters prior to level save so that
   mon_arrive() can catch up for lost time when they're restored later */
void
update_mlstmv()
{
	struct monst *mon;

	/* monst->mlstmv used to be updated every time `monst' actually moved,
	   but that is no longer the case so we just do a blanket assignment */
	for (mon = fmon; mon; mon = mon->nmon)
	    if (!DEADMONSTER(mon)) mon->mlstmv = monstermoves;
}

void
losedogs()
{
	register struct monst *mtmp, *mtmp0 = 0, *mtmp2;

	while ((mtmp = mydogs) != 0) {
		mydogs = mtmp->nmon;
		mon_arrive(mtmp, TRUE);
	}

	for(mtmp = migrating_mons; mtmp; mtmp = mtmp2) {
		mtmp2 = mtmp->nmon;
		if (mtmp->mux == u.uz.dnum && mtmp->muy == u.uz.dlevel) {
		    if(mtmp == migrating_mons)
			migrating_mons = mtmp->nmon;
		    else
			mtmp0->nmon = mtmp->nmon;
		    mon_arrive(mtmp, FALSE);
		} else
		    mtmp0 = mtmp;
	}
}

/* called from resurrect() in addition to losedogs() */
void
mon_arrive(mtmp, with_you)
struct monst *mtmp;
boolean with_you;
{
	struct trap *t;
	xchar xlocale, ylocale, xyloc, xyflags, wander;
	int num_segs;

	mtmp->nmon = fmon;
	fmon = mtmp;
	if (mtmp->isshk)
	    set_residency(mtmp, FALSE);

	num_segs = mtmp->wormno;
	/* baby long worms have no tail so don't use is_longworm() */
	if ((mtmp->data == &mons[PM_LONG_WORM]) &&
#ifdef DCC30_BUG
	    (mtmp->wormno = get_wormno(), mtmp->wormno != 0))
#else
	    (mtmp->wormno = get_wormno()) != 0)
#endif
	{
	    initworm(mtmp, num_segs);
	    /* tail segs are not yet initialized or displayed */
	} else mtmp->wormno = 0;

	/* some monsters might need to do something special upon arrival
	   _after_ the current level has been fully set up; see dochug() */
	mtmp->mstrategy |= STRAT_ARRIVE;

	/* make sure mnexto(rloc_to(set_apparxy())) doesn't use stale data */
	mtmp->mux = 0,  mtmp->muy = 0;
	xyloc	= mtmp->mtrack[0].x;
	xyflags = mtmp->mtrack[0].y;
	xlocale = mtmp->mtrack[1].x;
	ylocale = mtmp->mtrack[1].y;
	mtmp->mtrack[0].x = mtmp->mtrack[0].y = 0;
	mtmp->mtrack[1].x = mtmp->mtrack[1].y = 0;

#ifdef STEED
	if (mtmp == u.usteed)
	    return;	/* don't place steed on the map */
#endif
	if (with_you) {
	    /* When a monster accompanies you, sometimes it will arrive
	       at your intended destination and you'll end up next to
	       that spot.  This code doesn't control the final outcome;
	       goto_level(do.c) decides who ends up at your target spot
	       when there is a monster there too. */
	    if (!MON_AT(u.ux, u.uy) &&
		    !rn2(mtmp->mtame ? 10 : mtmp->mpeaceful ? 5 : 2))
		rloc_to(mtmp, u.ux, u.uy);
	    else
		mnexto(mtmp);
	    return;
	}
	/*
	 * The monster arrived on this level independently of the player.
	 * Its coordinate fields were overloaded for use as flags that
	 * specify its final destination.
	 */

	if (mtmp->mlstmv < monstermoves - 1L) {
	    /* heal monster for time spent in limbo */
	    long nmv = monstermoves - 1L - mtmp->mlstmv;

	    mon_catchup_elapsed_time(mtmp, nmv);
	    mtmp->mlstmv = monstermoves - 1L;

	    /* let monster move a bit on new level (see placement code below) */
	    wander = (xchar) min(nmv, 8);
	} else
	    wander = 0;

	switch (xyloc) {
	 case MIGR_APPROX_XY:	/* {x,y}locale set above */
		break;
	 case MIGR_EXACT_XY:	wander = 0;
		break;
	 case MIGR_NEAR_PLAYER:	xlocale = u.ux,  ylocale = u.uy;
		break;
	 case MIGR_STAIRS_UP:	xlocale = xupstair,  ylocale = yupstair;
		break;
	 case MIGR_STAIRS_DOWN:	xlocale = xdnstair,  ylocale = ydnstair;
		break;
	 case MIGR_LADDER_UP:	xlocale = xupladder,  ylocale = yupladder;
		break;
	 case MIGR_LADDER_DOWN:	xlocale = xdnladder,  ylocale = ydnladder;
		break;
	 case MIGR_SSTAIRS:	xlocale = sstairs.sx,  ylocale = sstairs.sy;
		break;
	 case MIGR_PORTAL:
		if (In_endgame(&u.uz)) {
		    /* there is no arrival portal for endgame levels */
		    /* BUG[?]: for simplicity, this code relies on the fact
		       that we know that the current endgame levels always
		       build upwards and never have any exclusion subregion
		       inside their TELEPORT_REGION settings. */
		    xlocale = rn1(updest.hx - updest.lx + 1, updest.lx);
		    ylocale = rn1(updest.hy - updest.ly + 1, updest.ly);
		    break;
		}
		/* find the arrival portal */
		for (t = ftrap; t; t = t->ntrap)
		    if (t->ttyp == MAGIC_PORTAL) break;
		if (t) {
		    xlocale = t->tx,  ylocale = t->ty;
		    break;
		} else {
		    impossible("mon_arrive: no corresponding portal?");
		} /*FALLTHRU*/
	 default:
	 case MIGR_RANDOM:	xlocale = ylocale = 0;
		    break;
	    }

	if (xlocale && wander) {
	    /* monster moved a bit; pick a nearby location */
	    /* mnearto() deals w/stone, et al */
	    char *r = in_rooms(xlocale, ylocale, 0);
	    if (r && *r) {
		coord c;
		/* somexy() handles irregular rooms */
		if (somexy(&rooms[*r - ROOMOFFSET], &c))
		    xlocale = c.x,  ylocale = c.y;
		else
		    xlocale = ylocale = 0;
	    } else {		/* not in a room */
		int i, j;
		i = max(1, xlocale - wander);
		j = min(COLNO-1, xlocale + wander);
		xlocale = rn1(j - i, i);
		i = max(0, ylocale - wander);
		j = min(ROWNO-1, ylocale + wander);
		ylocale = rn1(j - i, i);
	    }
	}	/* moved a bit */

	mtmp->mx = 0;	/*(already is 0)*/
	mtmp->my = xyflags;
	if (xlocale)
	    (void) mnearto(mtmp, xlocale, ylocale, FALSE);
	else {
	    if (!rloc(mtmp,TRUE)) {
		/*
		 * Failed to place migrating monster,
		 * probably because the level is full.
		 * Dump the monster's cargo and leave the monster dead.
		 */
	    	struct obj *obj, *corpse;
		while ((obj = mtmp->minvent) != 0) {
		    obj_extract_self(obj);
		    obj_no_longer_held(obj);
		    if (obj->owornmask & W_WEP)
			setmnotwielded(mtmp,obj);
		    obj->owornmask = 0L;
		    if (xlocale && ylocale)
			    place_object(obj, xlocale, ylocale);
		    else {
		    	rloco(obj);
			get_obj_location(obj, &xlocale, &ylocale, 0);
		    }
		}
		corpse = mkcorpstat(CORPSE, (struct monst *)0, mtmp->data,
				xlocale, ylocale, FALSE);
#ifndef GOLDOBJ
		if (mtmp->mgold) {
		    if (xlocale == 0 && ylocale == 0 && corpse) {
			(void) get_obj_location(corpse, &xlocale, &ylocale, 0);
			(void) mkgold(mtmp->mgold, xlocale, ylocale);
		    }
		    mtmp->mgold = 0L;
		}
#endif
		mongone(mtmp);
	    }
	}
}

/* heal monster for time spent elsewhere */
void
mon_catchup_elapsed_time(mtmp, nmv)
struct monst *mtmp;
long nmv;		/* number of moves */
{
	int imv = 0;	/* avoid zillions of casts and lint warnings */

#if defined(DEBUG) || defined(BETA)
	if (nmv < 0L) {			/* crash likely... */
	    panic("catchup from future time?");
	    /*NOTREACHED*/
	    return;
	} else if (nmv == 0L) {		/* safe, but should'nt happen */
	    impossible("catchup from now?");
	} else
#endif
	if (nmv >= LARGEST_INT)		/* paranoia */
	    imv = LARGEST_INT - 1;
	else
	    imv = (int)nmv;

	/* might stop being afraid, blind or frozen */
	/* set to 1 and allow final decrement in movemon() */
	if (mtmp->mblinded) {
	    if (imv >= (int) mtmp->mblinded) mtmp->mblinded = 1;
	    else mtmp->mblinded -= imv;
	}
	if (mtmp->mfrozen) {
	    if (imv >= (int) mtmp->mfrozen) mtmp->mfrozen = 1;
	    else mtmp->mfrozen -= imv;
	}
	if (mtmp->mfleetim) {
	    if (imv >= (int) mtmp->mfleetim) mtmp->mfleetim = 1;
	    else mtmp->mfleetim -= imv;
	}

	/* might recover from temporary trouble */
	if (mtmp->mtrapped && rn2(imv + 1) > 40/2) mtmp->mtrapped = 0;
	if (mtmp->mconf    && rn2(imv + 1) > 50/2) mtmp->mconf = 0;
	if (mtmp->mstun    && rn2(imv + 1) > 10/2) mtmp->mstun = 0;

	/* might finish eating or be able to use special ability again */
	if (imv > mtmp->meating) mtmp->meating = 0;
	else mtmp->meating -= imv;
	if (imv > mtmp->mspec_used) mtmp->mspec_used = 0;
	else mtmp->mspec_used -= imv;

	/* reduce tameness for every 150 moves you are separated */
	if (mtmp->mtame && !mtmp->isminion && !(EDOG(mtmp)->loyal) && !(
		In_quest(&u.uz) && 
		((Is_qstart(&u.uz) && !flags.stag) || 
		 (Is_nemesis(&u.uz) && flags.stag)) &&
	 !(Race_if(PM_DROW) && Role_if(PM_NOBLEMAN) && !flags.initgend) &&
	 !(Role_if(PM_ANACHRONONAUT) && quest_status.leader_is_dead) &&
	 !(Role_if(PM_EXILE))
	) && !In_sokoban(&u.uz)
	) {
	    int wilder = (imv + 75) / 150;
		if(mtmp->mwait && !EDOG(mtmp)->friend) wilder = max(0, wilder - 11);
		if(P_SKILL(P_BEAST_MASTERY) > 1 && !EDOG(mtmp)->friend) wilder = max(0, wilder - (3*(P_SKILL(P_BEAST_MASTERY)-1) + 1));
#ifdef BARD
	    /* monsters under influence of Friendship song go wilder faster */
	    if (EDOG(mtmp)->friend)
		    wilder *= 150;
#endif
	    if (mtmp->mtame > wilder) mtmp->mtame -= wilder;	/* less tame */
	    else if (mtmp->mtame > rn2(wilder)) mtmp->mtame = 0;  /* untame */
	    else{
			mtmp->mtame = mtmp->mpeaceful = 0;		/* hostile! */
			mtmp->mferal = 1;
		}
	}
	/* check to see if it would have died as a pet; if so, go wild instead
	 * of dying the next time we call dog_move()
	 */
	if (mtmp->mtame && !mtmp->isminion &&
		(carnivorous(mtmp->data) || herbivorous(mtmp->data))
	){
	    struct edog *edog = EDOG(mtmp);
		if(!(In_quest(&u.uz) && 
			 ((Is_qstart(&u.uz) && !flags.stag) || 
				(Is_nemesis(&u.uz) && flags.stag)) &&
			 !(Race_if(PM_DROW) && Role_if(PM_NOBLEMAN) && !flags.initgend) &&
			 !(Role_if(PM_ANACHRONONAUT) && quest_status.leader_is_dead) &&
			 !(Role_if(PM_EXILE))
			) && !In_sokoban(&u.uz)
		) {
			if ((monstermoves > edog->hungrytime + 500 && mtmp->mhp < 3) ||
				(monstermoves > edog->hungrytime + 750)
			){
				mtmp->mtame = mtmp->mpeaceful = 0;		/* hostile! */
				mtmp->mferal = 1;
			}
		} else {
			if(edog->hungrytime < monstermoves + 500)
				edog->hungrytime = monstermoves + 500;
		}
	}

	if (!mtmp->mtame && mtmp->mleashed) {
	    /* leashed monsters should always be with hero, consequently
	       never losing any time to be accounted for later */
	    impossible("catching up for leashed monster?");
	    m_unleash(mtmp, FALSE);
	}

	/* recover lost hit points */
	if (regenerates(mtmp->data)){
		if (mtmp->mhp + imv >= mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		else mtmp->mhp += imv;
	}
	if(!nonliving_mon(mtmp)){
		imv = imv*(mtmp->m_lev + mtmp->mcon)/30;
		if (mtmp->mhp + imv >= mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
		else mtmp->mhp += imv;
	}
}

#endif /* OVLB */
#ifdef OVL2

/* called when you move to another level */
void
keepdogs(pets_only)
boolean pets_only;	/* true for ascension or final escape */
{
	register struct monst *mtmp, *mtmp2;
	register struct obj *obj;
	int num_segs;
	boolean stay_behind;
	boolean all_pets = FALSE;
	int pet_dist = P_SKILL(P_BEAST_MASTERY);
	if(pet_dist < 1)
		pet_dist = 1;
	if(uwep && uwep->otyp == SHEPHERD_S_CROOK)
		pet_dist++;
	if(u.specialSealsActive&SEAL_COSMOS ||
		(uarmh && uarmh->oartifact == ART_CROWN_OF_THE_SAINT_KING) ||
		(uarmh && uarmh->oartifact == ART_HELM_OF_THE_DARK_LORD)
	) all_pets = TRUE;
	
	if(!all_pets) for(obj = invent; obj; obj=obj->nobj)
		if(obj->oartifact == ART_LYRE_OF_ORPHEUS){
			all_pets = TRUE;
			break;
		}
	for (mtmp = fmon; mtmp; mtmp = mtmp2) {
	    mtmp2 = mtmp->nmon;
	    if (DEADMONSTER(mtmp)) continue;
	    if (pets_only && !mtmp->mtame) continue;
	    if (mtmp->data == &mons[PM_DANCING_BLADE]) continue;
	    if (((monnear(mtmp, u.ux, u.uy) && levl_follower(mtmp)) || 
			(mtmp->mtame && (all_pets ||
							// (u.sealsActive&SEAL_MALPHAS && mtmp->data == &mons[PM_CROW]) || //Allow distant crows to get left behind.
							(distmin(mtmp->mx, mtmp->my, u.ux, u.uy) <= pet_dist)
							)
			) ||
#ifdef STEED
			(mtmp == u.usteed) ||
#endif
		/* the wiz will level t-port from anywhere to chase
		   the amulet; if you don't have it, will chase you
		   only if in range. -3. */
			(u.uhave.amulet && mtmp->iswiz))
		&& ((!mtmp->msleeping && mtmp->mcanmove)
#ifdef STEED
		    /* eg if level teleport or new trap, steed has no control
		       to avoid following */
		    || (mtmp == u.usteed)
#endif
		    )
		/* monster won't follow if it hasn't noticed you yet */
		&& !(mtmp->mstrategy & STRAT_WAITFORU)) {
			stay_behind = FALSE;
			if (mtmp->mtame && mtmp->mwait && (mtmp->mwait+100 > monstermoves)) {
				if (canseemon(mtmp))
					pline("%s obediently waits for you to return.", Monnam(mtmp));
				stay_behind = TRUE;
			} else if (mtmp->mtame && mtmp->meating && mtmp != u.usteed) {
				if (canseemon(mtmp))
					pline("%s is still eating.", Monnam(mtmp));
				stay_behind = TRUE;
			} else if (mon_has_amulet(mtmp)) {
				if (canseemon(mtmp))
					pline("%s seems very disoriented for a moment.",
					Monnam(mtmp));
				stay_behind = TRUE;
			} else if (mtmp->mtame && mtmp->mtrapped && mtmp != u.usteed) {
				if (canseemon(mtmp))
					pline("%s is still trapped.", Monnam(mtmp));
				stay_behind = TRUE;
			}
#ifdef STEED
			// if (mtmp == u.usteed) stay_behind = FALSE;
			if (mtmp == u.usteed && stay_behind) {
			    pline("%s vanishes from underneath you.", Monnam(mtmp));
				dismount_steed(DISMOUNT_VANISHED);
			}
#endif
			if (stay_behind) {
				if (mtmp->mleashed) {
					pline("%s leash suddenly comes loose.",
						humanoid(mtmp->data)
							? (mtmp->female ? "Her" : "His")
							: "Its");
					m_unleash(mtmp, FALSE);
				}
				continue;
			}
			if (mtmp->isshk)
				set_residency(mtmp, TRUE);

			if (mtmp->wormno) {
				register int cnt;
				/* NOTE: worm is truncated to # segs = max wormno size */
				cnt = count_wsegs(mtmp);
				num_segs = min(cnt, MAX_NUM_WORMS - 1);
				wormgone(mtmp);
			} else num_segs = 0;

			/* set minvent's obj->no_charge to 0 */
			for(obj = mtmp->minvent; obj; obj = obj->nobj) {
				if (Has_contents(obj))
				picked_container(obj);	/* does the right thing */
				obj->no_charge = 0;
			}

			relmon(mtmp);
			newsym(mtmp->mx,mtmp->my);
			mtmp->mx = mtmp->my = 0; /* avoid mnexto()/MON_AT() problem */
			mtmp->wormno = num_segs;
			mtmp->mlstmv = monstermoves;
			mtmp->nmon = mydogs;
			mydogs = mtmp;
			if(mtmp->data == &mons[PM_SURYA_DEVA]){
				struct monst *blade;
				for(blade = fmon; blade; blade = blade->nmon) if(blade->data == &mons[PM_DANCING_BLADE] && mtmp->m_id == blade->mvar1) break;
				if(blade) {
					if(mtmp2 == blade) mtmp2 = mtmp2->nmon; /*mtmp2 is about to end up on the migrating mons chain*/
					/* set minvent's obj->no_charge to 0 */
					for(obj = blade->minvent; obj; obj = obj->nobj) {
						if (Has_contents(obj))
						picked_container(obj);	/* does the right thing */
						obj->no_charge = 0;
					}
					relmon(blade);
					newsym(blade->mx,blade->my);
					blade->mx = blade->my = 0; /* avoid mnexto()/MON_AT() problem */
					blade->mlstmv = monstermoves;
					blade->nmon = mydogs;
					mydogs = blade;
				}
			}
	    } else if (quest_status.touched_artifact && Race_if(PM_DROW) && !flags.initgend && Role_if(PM_NOBLEMAN) && mtmp->m_id == quest_status.leader_m_id) {
			mongone(mtmp);
	    // } else if(u.uevent.qcompleted && mtmp->data == &mons[PM_ORION]){
			// mondied(mtmp);
	    } else if (mtmp->iswiz || 
			mtmp->data == &mons[PM_ILLURIEN_OF_THE_MYRIAD_GLIMPSES] || 
			mtmp->data == &mons[PM_HUNGRY_DEAD] ||
			mtmp->mtame
		) {
			if (mtmp->mleashed) {
				/* this can happen if your quest leader ejects you from the
				   "home" level while a leashed pet isn't next to you */
				pline("%s leash goes slack.", s_suffix(Monnam(mtmp)));
				m_unleash(mtmp, FALSE);
			}
			/* we want to be able to find him when his next resurrection
			   chance comes up, but have him resume his present location
			   if player returns to this level before that time */
			if(mtmp->data == &mons[PM_SURYA_DEVA] && mtmp2 && mtmp2->data == &mons[PM_DANCING_BLADE] && mtmp2->mvar1 == mtmp->m_id)
				mtmp2 = mtmp2->nmon; /*mtmp2 is about to end up on the migrating mons chain*/
			if(mtmp->data != &mons[PM_DANCING_BLADE]) migrate_to_level(mtmp, ledger_no(&u.uz),
					 MIGR_EXACT_XY, (coord *)0);
	    }
	}
}

#endif /* OVL2 */
#ifdef OVLB

void
migrate_to_level(mtmp, tolev, xyloc, cc)
	register struct monst *mtmp;
	int tolev;	/* destination level */
	xchar xyloc;	/* MIGR_xxx destination xy location: */
	coord *cc;	/* optional destination coordinates */
{
	register struct obj *obj;
	d_level new_lev;
	xchar xyflags;
	int num_segs = 0;	/* count of worm segments */

	if (mtmp->isshk)
	    set_residency(mtmp, TRUE);

	if (mtmp->wormno) {
	    register int cnt;
	  /* **** NOTE: worm is truncated to # segs = max wormno size **** */
	    cnt = count_wsegs(mtmp);
	    num_segs = min(cnt, MAX_NUM_WORMS - 1);
	    wormgone(mtmp);
	}

	/* set minvent's obj->no_charge to 0 */
	for(obj = mtmp->minvent; obj; obj = obj->nobj) {
	    if (Has_contents(obj))
		picked_container(obj);	/* does the right thing */
	    obj->no_charge = 0;
	}

	if (mtmp->mleashed) {
		mtmp->mtame--;
		m_unleash(mtmp, TRUE);
	}
	relmon(mtmp);
	mtmp->nmon = migrating_mons;
	migrating_mons = mtmp;
	newsym(mtmp->mx,mtmp->my);

	new_lev.dnum = ledger_to_dnum(tolev);
	new_lev.dlevel = ledger_to_dlev(tolev);
	/* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as */
	/* destination codes (setup flag bits before altering mx or my) */
	xyflags = (depth(&new_lev) < depth(&u.uz));	/* 1 => up */
	if (In_W_tower(mtmp->mx, mtmp->my, &u.uz)) xyflags |= 2;
	mtmp->wormno = num_segs;
	mtmp->mlstmv = monstermoves;
	mtmp->mtrack[1].x = cc ? cc->x : mtmp->mx;
	mtmp->mtrack[1].y = cc ? cc->y : mtmp->my;
	mtmp->mtrack[0].x = xyloc;
	mtmp->mtrack[0].y = xyflags;
	mtmp->mux = new_lev.dnum;
	mtmp->muy = new_lev.dlevel;
	mtmp->mx = mtmp->my = 0;	/* this implies migration */
	
	if(mtmp->data == &mons[PM_SURYA_DEVA]){
		struct monst *blade;
		for(blade = fmon; blade; blade = blade->nmon) if(blade->data == &mons[PM_DANCING_BLADE] && mtmp->m_id == blade->mvar1) break;
		if(blade) {
			/* set minvent's obj->no_charge to 0 */
			for(obj = blade->minvent; obj; obj = obj->nobj) {
				if (Has_contents(obj))
				picked_container(obj);	/* does the right thing */
				obj->no_charge = 0;
			}

			if (blade->mleashed) {
				blade->mtame--;
				m_unleash(blade, TRUE);
			}
			
			relmon(blade);
			newsym(blade->mx,blade->my);
			blade->nmon = migrating_mons;
			migrating_mons = blade;
			
			new_lev.dnum = ledger_to_dnum((xchar)tolev);
			new_lev.dlevel = ledger_to_dlev((xchar)tolev);
			/* overload mtmp->[mx,my], mtmp->[mux,muy], and mtmp->mtrack[] as */
			/* destination codes (setup flag bits before altering mx or my) */
			xyflags = (depth(&new_lev) < depth(&u.uz));	/* 1 => up */
			if (In_W_tower(blade->mx, blade->my, &u.uz)) xyflags |= 2;
			blade->mlstmv = monstermoves;
			blade->mtrack[1].x = cc ? cc->x : blade->mx;
			blade->mtrack[1].y = cc ? cc->y : blade->my;
			blade->mtrack[0].x = xyloc;
			blade->mtrack[0].y = xyflags;
			blade->mux = new_lev.dnum;
			blade->muy = new_lev.dlevel;
			blade->mx = blade->my = 0;	/* this implies migration */
		}
	}
}

#endif /* OVLB */
#ifdef OVL1

/* return quality of food; the lower the better */
/* fungi will eat even tainted food */
int
dogfood(mon,obj)
struct monst *mon;
register struct obj *obj;
{
	boolean carni = carnivorous(mon->data);
	boolean herbi = herbivorous(mon->data);
	struct permonst *fptr = &mons[obj->corpsenm];
	boolean starving;

	if (is_quest_artifact(obj) || obj_resists(obj, 0, 95))
	    return (obj->cursed ? TABU : APPORT);

	switch(obj->oclass) {
	case FOOD_CLASS:
	    if (obj->otyp == CORPSE &&
		((touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
		 || is_rider(fptr)))
		    return TABU;

	    /* Ghouls only eat old corpses... yum! */
	    if (mon->data == &mons[PM_GHOUL]){
		return (obj->otyp == CORPSE && obj->corpsenm != PM_ACID_BLOB &&
		  peek_at_iced_corpse_age(obj) + 5*rn1(20,10) <= monstermoves) ?
			DOGFOOD : TABU;
	    }
	    /* vampires only "eat" very fresh corpses ... 
	     * Assume meat -> blood
	     */
	    if (is_vampire(mon->data)) {
		return (obj->otyp == CORPSE &&
		  has_blood(&mons[obj->corpsenm]) && !obj->oeaten &&
	    	  peek_at_iced_corpse_age(obj) + 5 >= monstermoves) ?
				DOGFOOD : TABU;
	    }

	    if (!carni && !herbi)
		    return (obj->cursed ? UNDEF : APPORT);

	    /* a starving pet will eat almost anything */
	    starving = (mon->mtame && !mon->isminion &&
			EDOG(mon)->mhpmax_penalty);

	    switch (obj->otyp) {
		case TRIPE_RATION:
		case MEATBALL:
		case MEAT_RING:
		case MEAT_STICK:
		case MASSIVE_CHUNK_OF_MEAT:
		    return (carni ? DOGFOOD : MANFOOD);
		case EGG:
		    if (touch_petrifies(&mons[obj->corpsenm]) && !resists_ston(mon))
			return POISON;
		    return (carni ? CADAVER : MANFOOD);
		case CORPSE:
rock:
		   if ((peek_at_iced_corpse_age(obj) + 50L <= monstermoves
					    && obj->corpsenm != PM_LIZARD
					    && obj->corpsenm != PM_BABY_CAVE_LIZARD
					    && obj->corpsenm != PM_SMALL_CAVE_LIZARD
					    && obj->corpsenm != PM_CAVE_LIZARD
					    && obj->corpsenm != PM_LARGE_CAVE_LIZARD
					    && obj->corpsenm != PM_LICHEN
					    && obj->corpsenm != PM_BEHOLDER
					    && mon->data->mlet != S_FUNGUS) ||
			(mons[obj->corpsenm].mflagsa == mon->data->mflagsa && !(mindless_mon(mon) || is_animal(mon->data))) ||
			(polyfodder(obj) && !resists_poly(mon->data) && (!(mindless_mon(mon) || is_animal(mon->data)) || goodsmeller(mon->data))) ||
			(acidic(&mons[obj->corpsenm]) && !resists_acid(mon)) ||
			(freezing(&mons[obj->corpsenm]) && !resists_cold(mon)) ||
			(burning(&mons[obj->corpsenm]) && !resists_fire(mon)) ||
			(poisonous(&mons[obj->corpsenm]) &&
						!resists_poison(mon))||
			 (touch_petrifies(&mons[obj->corpsenm]) &&
			  !resists_ston(mon)))
			return POISON;
		    else if (vegan(fptr))
			return (herbi ? CADAVER : MANFOOD);
		    else return (carni ? CADAVER : MANFOOD);
		case CLOVE_OF_GARLIC:
		    return (is_undead_mon(mon) ? TABU :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));
		case TIN:
		    return (metallivorous(mon->data) ? ACCFOOD : MANFOOD);
		case APPLE:
		case CARROT:
		    return (herbi ? DOGFOOD : starving ? ACCFOOD : MANFOOD);
		case BANANA:
		    return ((mon->data->mlet == S_YETI) ? DOGFOOD :
			    ((herbi || starving) ? ACCFOOD : MANFOOD));

                case K_RATION:
		case C_RATION:
                case CRAM_RATION:
		case LEMBAS_WAFER:
		case FOOD_RATION:
		    if (is_human(mon->data) ||
		        is_elf(mon->data) ||
			is_dwarf(mon->data) ||
			is_gnome(mon->data) ||
			is_orc(mon->data))
		        return ACCFOOD; 

		default:
		    if (starving) return ACCFOOD;
		    return (obj->otyp > SLIME_MOLD ?
			    (carni ? ACCFOOD : MANFOOD) :
			    (herbi ? ACCFOOD : MANFOOD));
	    }
	default:
	    if (obj->otyp == AMULET_OF_STRANGULATION ||
			obj->otyp == RIN_SLOW_DIGESTION)
		return TABU;
	    if (hates_silver(mon->data) &&
		obj->obj_material == SILVER)
		return(TABU);
	    if (hates_iron(mon->data) &&
		obj->obj_material == IRON)
		return(TABU);
	    if (hates_unholy(mon->data) &&
		is_unholy(obj))
		return(TABU);
	    if (herbi && (obj->otyp == SHEAF_OF_HAY || obj->otyp == SEDGE_HAT))
		return CADAVER;
	    if (mon->data == &mons[PM_GELATINOUS_CUBE] && is_organic(obj))
		return(ACCFOOD);
	    if (metallivorous(mon->data) && is_metallic(obj) && (is_rustprone(obj) || mon->data != &mons[PM_RUST_MONSTER])) {
		/* Non-rustproofed ferrous based metals are preferred. */
		return((is_rustprone(obj) && !obj->oerodeproof) ? DOGFOOD :
			ACCFOOD);
	    }
	    if(!obj->cursed && obj->oclass != BALL_CLASS &&
						obj->oclass != CHAIN_CLASS)
		return(APPORT);
	    /* fall into next case */
	case ROCK_CLASS:
	    return(UNDEF);
	}
}

#endif /* OVL1 */
#ifdef OVLB

void
enough_dogs(numdogs)
int numdogs;
{
	// finds weakest pet, and if there's more than 6 pets that count towards your limit
	// it sets the weakest friendly
	struct monst *curmon = 0, *weakdog = 0;
	for(curmon = fmon; curmon; curmon = curmon->nmon){
			if(curmon->mtame && !(EDOG(curmon)->friend) && !(EDOG(curmon)->loyal) && !is_suicidal(curmon->data)
				&& !curmon->mspiritual && curmon->mvanishes < 0
			){
				numdogs++;
				if(!weakdog) weakdog = curmon;
				if(weakdog->m_lev > curmon->m_lev) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
			}
		}

	if(weakdog && numdogs > (ACURR(A_CHA)/3)) EDOG(weakdog)->friend = 1;
}

void
vanish_dogs()
{
	// if there's a spiritual pet that isn't already marked for vanishing,
	// give it 5 turns before it disappears.
	struct monst *weakdog, *curmon;
	int numdogs;
	do {
		weakdog = (struct monst *)0;
		numdogs = 0;
		for(curmon = fmon; curmon; curmon = curmon->nmon){
			if(curmon->mspiritual && curmon->mvanishes < 0){
				numdogs++;
				if(!weakdog) weakdog = curmon;
				if(weakdog->m_lev > curmon->m_lev) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
				else if(weakdog->mtame > curmon->mtame) weakdog = curmon;
			}
		}
		if(weakdog && numdogs > (ACURR(A_CHA)/3) ) weakdog->mvanishes = 5;
	} while(weakdog && numdogs > (ACURR(A_CHA)/3));
}


struct monst *
tamedog(mtmp, obj)
struct monst *mtmp;
struct obj *obj;
{
	struct monst *mtmp2, *curmon, *weakdog = (struct monst *) 0;
	/* The Wiz, Medusa and the quest nemeses aren't even made peaceful. || mtmp->data == &mons[PM_MEDUSA] */
	if (is_untamable(mtmp->data) || mtmp->notame || mtmp->iswiz
		|| (&mons[urole.neminum] == mtmp->data)
	) return((struct monst *)0);

	/* worst case, at least it'll be peaceful. */
	if(!obj || !is_instrument(obj)){
		mtmp->mpeaceful = 1;
		mtmp->mtraitor  = 0;	/* No longer a traitor */
		set_malign(mtmp);
	}

	/* pacify monster cannot tame */
	if (obj && obj->otyp == SPE_PACIFY_MONSTER){
		mtmp->mpeacetime = max(mtmp->mpeacetime, ACURR(A_CHA));
		return((struct monst *)0);
	}

	if(flags.moonphase == FULL_MOON && night() && rn2(6) && obj && !is_instrument(obj)
		&& obj->oclass != SPBOOK_CLASS && obj->oclass != SCROLL_CLASS
		&& mtmp->data->mlet == S_DOG
	) return((struct monst *)0);

#ifdef CONVICT
    if (Role_if(PM_CONVICT) && (is_domestic(mtmp->data) && obj && !is_instrument(obj) && obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS)) {
        /* Domestic animals are wary of the Convict */
        pline("%s still looks wary of you.", Monnam(mtmp));
        return((struct monst *)0);
    }
#endif
	/* If we cannot tame it, at least it's no longer afraid. */
	mtmp->mflee = 0;
	mtmp->mfleetim = 0;

	/* make grabber let go now, whether it becomes tame or not */
	if (mtmp == u.ustuck) {
	    if (u.uswallow)
		expels(mtmp, mtmp->data, TRUE);
	    else if (!(sticks(youracedata)))
		unstuck(mtmp);
	}

	/* feeding it treats makes it tamer */
	if (mtmp->mtame && obj && !is_instrument(obj) && obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS) {
	    int tasty;

	    if (mtmp->mcanmove && !mtmp->mconf && !mtmp->meating &&
		((tasty = dogfood(mtmp, obj)) == DOGFOOD ||
		 (tasty <= ACCFOOD && EDOG(mtmp)->hungrytime <= monstermoves))) {
		/* pet will "catch" and eat this thrown food */
		if (canseemon(mtmp)) {
		    boolean big_corpse = (obj->otyp == CORPSE &&
					  obj->corpsenm >= LOW_PM &&
				mons[obj->corpsenm].msize > mtmp->data->msize);
		    pline("%s catches %s%s",
			  Monnam(mtmp), the(xname(obj)),
			  !big_corpse ? "." : ", or vice versa!");
		} else if (cansee(mtmp->mx,mtmp->my))
		    pline("%s.", Tobjnam(obj, "stop"));

		/* Don't stuff ourselves if we know better */
		if (is_animal(mtmp->data) || mindless_mon(mtmp))
		{
		/* dog_eat expects a floor object */
		place_object(obj, mtmp->mx, mtmp->my);
		    if (dog_eat(mtmp, obj, mtmp->mx, mtmp->my, FALSE) == 2
		        && rn2(4))
		    {
		        /* You choked your pet, you cruel, cruel person! */
		        You_feel("guilty about losing your pet like this.");
				u.ugangr[Align2gangr(u.ualign.type)]++;
				adjalign(-15);
				u.hod += 5;
		    }
		}
		/* eating might have killed it, but that doesn't matter here;
		   a non-null result suppresses "miss" message for thrown
		   food and also implies that the object has been deleted */
		return mtmp;
	    } else
		return (struct monst *)0;
	}

	if (mtmp->mtame || (!mtmp->mcanmove && !mtmp->moccupation) ||
	    /* monsters with conflicting structures cannot be tamed */
	    mtmp->isshk || mtmp->isgd || mtmp->ispriest || mtmp->isminion ||
	    mtmp->data == &mons[urole.neminum] ||
	    (is_demon(mtmp->data) && !is_demon(youracedata)) ||
	    (obj && !is_instrument(obj) && obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS && dogfood(mtmp, obj) >= MANFOOD)) return (struct monst *)0;

	if (mtmp->m_id == quest_status.leader_m_id)
	    return((struct monst *)0);

	/* before officially taming the target, check how many pets there are and untame one if there are too many */
	if(!(obj && obj->oclass == SCROLL_CLASS && Confusion)){
		enough_dogs(1);
	}
	
	/* make a new monster which has the pet extension */
	mtmp2 = newmonst(sizeof(struct edog) + mtmp->mnamelth);
	*mtmp2 = *mtmp;
	mtmp2->mxlth = sizeof(struct edog);
	if (mtmp->mnamelth) Strcpy(NAME(mtmp2), NAME(mtmp));
	initedog(mtmp2);
	if(obj && obj->otyp == SPE_CHARM_MONSTER){
		mtmp2->mpeacetime = 1;
	}
	replmon(mtmp, mtmp2);
	/* `mtmp' is now obsolete */

	if (obj && !is_instrument(obj) && obj->oclass != SCROLL_CLASS && obj->oclass != SPBOOK_CLASS) {		/* thrown food */
	    /* defer eating until the edog extension has been set up */
	    place_object(obj, mtmp2->mx, mtmp2->my);	/* put on floor */
	    /* devour the food (might grow into larger, genocided monster) */
	    if (dog_eat(mtmp2, obj, mtmp2->mx, mtmp2->my, TRUE) == 2)
		return mtmp2;		/* oops, it died... */
	    /* `obj' is now obsolete */
	}

	newsym(mtmp2->mx, mtmp2->my);
	if (attacktype(mtmp2->data, AT_WEAP)) {
		mtmp2->weapon_check = NEED_HTH_WEAPON;
		(void) mon_wield_item(mtmp2);
	}
	return(mtmp2);
}

struct monst *
make_pet_minion(mnum,alignment)
int mnum;
aligntyp alignment;
{
    register struct monst *mon;
    register struct monst *mtmp2;
    mon = makemon(&mons[mnum], u.ux, u.uy, NO_MM_FLAGS);
    if (!mon) return 0;
    /* now tame that puppy... */
    mtmp2 = newmonst(sizeof(struct edog) + mon->mnamelth);
    *mtmp2 = *mon;
    mtmp2->mxlth = sizeof(struct edog);
    if(mon->mnamelth) Strcpy(NAME(mtmp2), NAME(mon));
    initedog(mtmp2);
    replmon(mon,mtmp2);
    newsym(mtmp2->mx, mtmp2->my);
    mtmp2->mpeaceful = 1;
    set_malign(mtmp2);
    mtmp2->mtame = 10;
    /* this section names the creature "of ______" */
    if (mons[mnum].pxlth == 0) {
		mtmp2->isminion = TRUE;
		EMIN(mtmp2)->min_align = alignment;
    } else if (mnum == PM_ANGEL) {
		 mtmp2->isminion = TRUE;
		 EPRI(mtmp2)->shralign = alignment;
    }
    return mtmp2;
}

/*
 * Called during pet revival or pet life-saving.
 * If you killed the pet, it revives wild.
 * If you abused the pet a lot while alive, it revives wild.
 * If you abused the pet at all while alive, it revives untame.
 * If the pet wasn't abused and was very tame, it might revive tame.
 */
void
wary_dog(mtmp, was_dead)
struct monst *mtmp;
boolean was_dead;
{
    struct edog *edog;
    boolean quietly = was_dead;

    mtmp->meating = 0;
    if (!mtmp->mtame) return;
    edog = !mtmp->isminion ? EDOG(mtmp) : 0;

    /* if monster was starving when it died, undo that now */
    if (edog && edog->mhpmax_penalty) {
	mtmp->mhpmax += edog->mhpmax_penalty;
	mtmp->mhp += edog->mhpmax_penalty;	/* heal it */
	edog->mhpmax_penalty = 0;
    }

    if (edog && (edog->killed_by_u == 1 || edog->abuse > 2)) {
	mtmp->mpeaceful = mtmp->mtame = 0;
	if (edog->abuse >= 0 && edog->abuse < 10)
	    if (!rn2(edog->abuse + 1)) mtmp->mpeaceful = 1;
	if(!quietly && cansee(mtmp->mx, mtmp->my)) {
	    if (haseyes(youracedata)) {
		if (haseyes(mtmp->data))
			pline("%s %s to look you in the %s.",
				Monnam(mtmp),
				mtmp->mpeaceful ? "seems unable" :
					    "refuses",
				body_part(EYE));
		else 
			pline("%s avoids your gaze.",
				Monnam(mtmp));
	    }
	}
    } else {
	/* chance it goes wild anyway - Pet Semetary */
	if (!(edog && edog->loyal) && !rn2(mtmp->mtame)) {
	    mtmp->mpeaceful = mtmp->mtame = 0;
	}
    }
    if (!mtmp->mtame) {
	newsym(mtmp->mx, mtmp->my);
	/* a life-saved monster might be leashed;
	   don't leave it that way if it's no longer tame */
	if (mtmp->mleashed) m_unleash(mtmp, TRUE);
    }

    /* if its still a pet, start a clean pet-slate now */
    if (edog && mtmp->mtame) {
		edog->revivals++;
		edog->killed_by_u = 0;
		edog->abuse = 0;
		edog->ogoal.x = edog->ogoal.y = -1;
		if (was_dead || edog->hungrytime < monstermoves + 500L)
		    edog->hungrytime = monstermoves + 500L;
		if (was_dead) {
		    edog->droptime = 0L;
		    edog->dropdist = 10000;
		    edog->whistletime = 0L;
		    edog->apport = ACURR(A_CHA);
		} /* else lifesaved, so retain current values */
    }
}

void
abuse_dog(mtmp)
struct monst *mtmp;
{
	if (!mtmp->mtame) return;

	if (Aggravate_monster || Conflict) mtmp->mtame /=2;
	else mtmp->mtame--;

	if (mtmp->mtame && !mtmp->isminion)
	    EDOG(mtmp)->abuse++;

	if (!mtmp->mtame && mtmp->mleashed)
	    m_unleash(mtmp, TRUE);

	/* don't make a sound if pet is in the middle of leaving the level */
	/* newsym isn't necessary in this case either */
	if (mtmp->mx != 0) {
	    if (mtmp->mtame && rn2(mtmp->mtame)) yelp(mtmp);
	    else growl(mtmp);	/* give them a moment's worry */
	
	    if (mtmp->mtame) betrayed(mtmp);

	    /* Give monster a chance to betray you now */
		if (!mtmp->mtame) newsym(mtmp->mx, mtmp->my);
	}
}

#endif /* OVLB */

/*dog.c*/
