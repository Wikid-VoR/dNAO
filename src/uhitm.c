/*	SCCS Id: @(#)uhitm.c	3.4	2003/02/18	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"

STATIC_DCL boolean FDECL(known_hitum, (struct monst *,int *,struct attack *));
STATIC_DCL boolean FDECL(known_hitum_wepi, (struct monst *,int *,struct attack *, int));
STATIC_DCL boolean FDECL(pacifist_attack_checks, (struct monst *, struct obj *));
STATIC_DCL void FDECL(steal_it, (struct monst *, struct attack *));
STATIC_DCL boolean FDECL(hitum, (struct monst *,int,struct attack *));
STATIC_DCL boolean FDECL(hmon_hitmon, (struct monst *,struct obj *,int));
#ifdef STEED
STATIC_DCL int FDECL(joust, (struct monst *,struct obj *));
#endif
void NDECL(demonpet);
STATIC_DCL boolean FDECL(m_slips_free, (struct monst *mtmp,struct attack *mattk));
STATIC_DCL int FDECL(explum, (struct monst *,struct attack *));
STATIC_DCL void FDECL(start_engulf, (struct monst *));
STATIC_DCL void NDECL(end_engulf);
STATIC_DCL int FDECL(gulpum, (struct monst *,struct attack *));
STATIC_DCL void FDECL(nohandglow, (struct monst *));

extern boolean notonhead;	/* for long worms */
/* The below might become a parameter instead if we use it a lot */
static int dieroll;
/* Used to flag attacks caused by Stormbringer's maliciousness. */
static boolean override_confirmation = FALSE;

/* modified from hurtarmor() in mhitu.c */
/* This is not static because it is also used for monsters rusting monsters */
void
hurtmarmor(mdef, attk)
struct monst *mdef;
int attk;
{
	int	hurt;
	struct obj *target;

	switch(attk) {
	    /* 0 is burning, which we should never be called with */
	    case AD_RUST: hurt = 1; break;
	    case AD_CORR: hurt = 3; break;
	    default: hurt = 2; break;
	}
	/* What the following code does: it keeps looping until it
	 * finds a target for the rust monster.
	 * Head, feet, etc... not covered by metal, or covered by
	 * rusty metal, are not targets.  However, your body always
	 * is, no matter what covers it.
	 */
	while (1) {
	    switch(rn2(5)) {
	    case 0:
		target = which_armor(mdef, W_ARMH);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 1:
		target = which_armor(mdef, W_ARMC);
		if (target) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
		    break;
		}
		if ((target = which_armor(mdef, W_ARM)) != (struct obj *)0) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
#ifdef TOURIST
		} else if ((target = which_armor(mdef, W_ARMU)) != (struct obj *)0) {
		    (void)rust_dmg(target, xname(target), hurt, TRUE, mdef);
#endif
		}
		break;
	    case 2:
		target = which_armor(mdef, W_ARMS);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 3:
		target = which_armor(mdef, W_ARMG);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    case 4:
		target = which_armor(mdef, W_ARMF);
		if (!target || !rust_dmg(target, xname(target), hurt, FALSE, mdef))
		    continue;
		break;
	    }
	    break; /* Out of while loop */
	}
}

/* Extra pacifist attack checks; FALSE means it's OK to attack. */
STATIC_OVL boolean
pacifist_attack_checks(mtmp, wep)
struct monst *mtmp;
struct obj *wep; /* uwep for attack(), null for kick_monster() */
{
	if (Confusion || Hallucination || Stunned)
		return FALSE;

	/* Intelligent chaotic weapons (Stormbringer) want blood */
	if (wep && spec_ability2(wep, SPFX2_BLDTHRST)) {
		/* Don't show Stormbringer's message unless pacifist. */
		if (iflags.attack_mode == ATTACK_MODE_PACIFIST)
			override_confirmation = TRUE;
		return FALSE;
	}

	if (iflags.attack_mode == ATTACK_MODE_PACIFIST) {
		You("stop for %s.", mon_nam(mtmp));
		flags.move = 0;
		return TRUE;
	}

	return FALSE;
}

/* FALSE means it's OK to attack */
boolean
attack_checks(mtmp, wep)
struct monst *mtmp;
struct obj *wep;	/* uwep for attack(), null for kick_monster() */
{
	/* if you're close enough to attack, alert any waiting monster */
	mtmp->mstrategy &= ~STRAT_WAITMASK;

	if (u.uswallow && mtmp == u.ustuck) return FALSE;

	if (flags.forcefight) {
		/* Do this in the caller, after we checked that the monster
		 * didn't die from the blow.  Reason: putting the 'I' there
		 * causes the hero to forget the square's contents since
		 * both 'I' and remembered contents are stored in .glyph.
		 * If the monster dies immediately from the blow, the 'I' will
		 * not stay there, so the player will have suddenly forgotten
		 * the square's contents for no apparent reason.
		if (!canspotmon(mtmp) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph))
			map_invisible(u.ux+u.dx, u.uy+u.dy);
		 */
		return FALSE;
	}

	/* Put up an invisible monster marker, but with exceptions for
	 * monsters that hide and monsters you've been warned about.
	 * The former already prints a warning message and
	 * prevents you from hitting the monster just via the hidden monster
	 * code below; if we also did that here, similar behavior would be
	 * happening two turns in a row.  The latter shows a glyph on
	 * the screen, so you know something is there.
	 */
	if (!canspotmon(mtmp) &&
		    !glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy)) &&
		    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
		    !(!Blind && mtmp->mundetected && hides_under(mtmp->data))) {
		pline("Wait!  There's %s there you can't see!",
			something);
		map_invisible(u.ux+u.dx, u.uy+u.dy);
		/* if it was an invisible mimic, treat it as if we stumbled
		 * onto a visible mimic
		 */
		if(mtmp->m_ap_type && !Protection_from_shape_changers) {
		    if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
			u.ustuck = mtmp;
		}
		if(!mtmp->mpeaceful) wakeup(mtmp, TRUE); /* always necessary; also un-mimics mimics */
		return TRUE;
	}

	if (mtmp->m_ap_type && !Protection_from_shape_changers &&
	   !sensemon(mtmp) &&
	   !glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy))) {
		/* If a hidden mimic was in a square where a player remembers
		 * some (probably different) unseen monster, the player is in
		 * luck--he attacks it even though it's hidden.
		 */
		if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		    seemimic(mtmp);
		    return(FALSE);
		}
		stumble_onto_mimic(mtmp);
		return TRUE;
	}

	if (mtmp->mundetected && !canseemon(mtmp) && !sensemon(mtmp) &&
		!glyph_is_warning(glyph_at(u.ux+u.dx,u.uy+u.dy)) &&
		!MATCH_WARN_OF_MON(mtmp) &&
		(hides_under(mtmp->data) || mtmp->data->mlet == S_EEL)) {
	    mtmp->mundetected = mtmp->msleeping = 0;
	    newsym(mtmp->mx, mtmp->my);
	    if (glyph_is_invisible(levl[mtmp->mx][mtmp->my].glyph)) {
		seemimic(mtmp);
		return(FALSE);
	    }
	    if (!(Blind ? Blind_telepat : Unblind_telepat)) {
		struct obj *obj;

		if (Blind || (is_pool(mtmp->mx,mtmp->my, FALSE) && !Underwater))
		    pline("Wait!  There's a hidden monster there!");
		else if ((obj = level.objects[mtmp->mx][mtmp->my]) != 0)
		    pline("Wait!  There's %s hiding under %s!",
			  an(l_monnam(mtmp)), doname(obj));
		return TRUE;
	    }
	}

	/*
	 * make sure to wake up a monster from the above cases if the
	 * hero can sense that the monster is there.
	 */
	if ((mtmp->mundetected || mtmp->m_ap_type) && sensemon(mtmp)) {
	    mtmp->mundetected = 0;
	    wakeup(mtmp, TRUE);
	}

	if (mtmp->mpeaceful && !Confusion && !Hallucination && !Stunned) {
		/* Intelligent chaotic weapons (Stormbringer) want blood */
		/* NOTE:  now generalized to a flag, also, more lawful weapons than chaotic weps have it now :) */
		if (wep && spec_ability2(wep, SPFX2_BLDTHRST)) {
			/* Don't show Stormbringer's message if attack is intended. */
			if (iflags.attack_mode != ATTACK_MODE_FIGHT_ALL)
				override_confirmation = TRUE;
			return(FALSE);
		}
		if (canspotmon(mtmp)) {
			if (iflags.attack_mode == ATTACK_MODE_CHAT
				|| iflags.attack_mode == ATTACK_MODE_PACIFIST) {
				if (mtmp->ispriest) {
					/* Prevent accidental donation prompt. */
					pline("%s mutters a prayer.", Monnam(mtmp));
				} else if (!dochat(FALSE, u.dx, u.dy, 0)) {
					flags.move = 0;
				}
				return TRUE;
			} else if(iflags.attack_mode == ATTACK_MODE_ASK){
				char qbuf[QBUFSZ];
#ifdef PARANOID
				char buf[BUFSZ];
				if (iflags.paranoid_hit) {
					Sprintf(qbuf, "Really attack %s? [no/yes]",
						mon_nam(mtmp));
					getlin (qbuf, buf);
					(void) lcase (buf);
					if (strcmp (buf, "yes")) {
					  flags.move = 0;
					  return(TRUE);
					}
				} else {
#endif
					Sprintf(qbuf, "Really attack %s?", mon_nam(mtmp));
					if (yn(qbuf) != 'y') {
						flags.move = 0;
						return(TRUE);
					}
#ifdef PARANOID
				}
#endif
			}
		}
	}

	return pacifist_attack_checks(mtmp, wep);
}

/*
 * It is unchivalrous for a knight to attack the defenseless or from behind.
 */
void
check_caitiff(mtmp)
struct monst *mtmp;
{
	if (Role_if(PM_KNIGHT) && u.ualign.type == A_LAWFUL &&
	    (!mtmp->mcanmove || !mtmp->mnotlaugh || mtmp->msleeping ||
		(mtmp->mflee && mtmp->data != &mons[PM_BANDERSNATCH] && !mtmp->mavenge))){
		    You("caitiff!");
			if(u.ualign.record > 10) {
				u.ualign.sins++;
			    adjalign(-2); //slightly stiffer penalty
				u.hod++;
			}
			else if(u.ualign.record > -10) {
			    adjalign(-5); //slightly stiffer penalty
			}
			else{
			    adjalign(-5); //slightly stiffer penalty
				u.hod++;
			}
	}
/*	attacking peaceful creatures is bad for the samurai's giri */
	if (Role_if(PM_SAMURAI) && mtmp->mpeaceful){
        if(!(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_NINJA)){
          You("dishonorably attack the innocent!");
          u.ualign.sins++;
          u.ualign.sins++;
          u.hod++;
          adjalign(-1);
          if(u.ualign.record > -10) {
              adjalign(-4);
          }
        } else {
          You("dishonorably attack the innocent!");
          adjalign(1);
        }
	}
	
}

void
find_to_hit_rolls(mtmp,ptmp,pweptmp,ptchtmp)
	struct monst *mtmp;
	int *ptmp, *pweptmp, *ptchtmp;
{
	int tmp, weptmp, tchtmp;
	
	tmp = find_roll_to_hit(mtmp, FALSE);
	weptmp = find_roll_to_hit(mtmp, (uwep && arti_shining(uwep)) || u.sealsActive&SEAL_CHUPOCLOPS);
	tchtmp = find_roll_to_hit(mtmp, TRUE);
	
	tmp += u.uencouraged;
	weptmp += u.uencouraged;
	tchtmp += u.uencouraged;
	
	if(uwep && uwep->oartifact == ART_SINGING_SWORD){
		if(uwep->osinging == OSING_LIFE){
			tmp += uwep->spe;
			weptmp += uwep->spe+1;
			tchtmp += uwep->spe+1;
		}
	}
	
	if (wizard && u.uencouraged && ublindf && ublindf->otyp == LENSES)
		pline("[You +%d]", u.uencouraged);
	
	if(mtmp->mstdy){
		tmp += mtmp->mstdy;
		weptmp += mtmp->mstdy;
		tchtmp += mtmp->mstdy;
	}
	if(mtmp->ustdym){
		tmp += mtmp->ustdym;
		weptmp += mtmp->ustdym;
		tchtmp += mtmp->ustdym;
	}
	
	
	if (uwep) {
		weptmp += hitval(uwep, mtmp);
		weptmp += weapon_hit_bonus(uwep);
		if(uwep->objsize - youracedata->msize > 0){
			weptmp += -4*(uwep->objsize - youracedata->msize);
		}
		if(is_lightsaber(uwep) && litsaber(uwep)){
			if(u.fightingForm == FFORM_SHII_CHO && MON_WEP(mtmp) && is_lightsaber(MON_WEP(mtmp)) && litsaber(MON_WEP(mtmp))){
				weptmp -= 5;
				// switch(min(P_SKILL(FFORM_SHII_CHO), P_SKILL(weapon_type(uwep)))){
					// case P_ISRESTRICTED:weptmp -= 5; break;
					// case P_UNSKILLED:   weptmp -= 5; break;
					// case P_BASIC:       weptmp -= 5; break;
					// case P_SKILLED:     weptmp -= 2; break;
					// case P_EXPERT:      weptmp -= 1; break;
				// }
			}
		}
	}
	
	if(!uarm){
		if(!uwep) weptmp += weapon_hit_bonus(uwep);
		tmp += weapon_hit_bonus(0); /*Add to hit bonus for martial arts, if you aren't wearing body armor.*/
	}
	
	if(!uwep && Race_if(PM_HALF_DRAGON)){
		/*Claws get large bonus to hit thanks to instinct*/
		weptmp += 5;
		tmp += 5;
	}
	
    static int fgloves = 0;
    if (!fgloves) fgloves = find_fgloves();
    if (uarmf && uarmf->otyp == fgloves && uwep && !bimanual(uwep,youracedata) && !uarms) weptmp+=3;
	
	*ptmp = tmp;
	*pweptmp = weptmp;
	*ptchtmp = tchtmp;
}

int
find_roll_to_hit(mtmp, phasing)
struct monst *mtmp;
boolean phasing;
{
	int tmp;
	int tmp2;
	double bab = BASE_ATTACK_BONUS;
	int luckbon = 0;
		
	if(Luck) luckbon = sgn(Luck)*rnd(abs(Luck));
	
	if(phasing){
		tmp = 1 + luckbon + abon() + base_mac(mtmp) + u.uhitinc + u.spiritAttk +
			maybe_polyd(youmonst.data->mlevel, u.ulevel)*bab;
	} else {
		tmp = 1 + luckbon + abon() + find_mac(mtmp) + u.uhitinc + u.spiritAttk +
			maybe_polyd(youmonst.data->mlevel, u.ulevel)*bab;
	}
	
	if(u.sealsActive&SEAL_DANTALION && tp_sensemon(mtmp)) tmp += max(0,(ACURR(A_INT)-10)/2);
	
/*	Adjust vs. (and possibly modify) monster state.		*/

	if(mtmp->mstun) tmp += 2;
	if(mtmp->mflee) tmp += 2;

	if (mtmp->msleeping) {
		mtmp->msleeping = 0;
		tmp += 2;
	}
	if(!mtmp->mcanmove) {
		tmp += 4;
		if(!rn2(10)) {
			if(mtmp->data != &mons[PM_GIANT_TURTLE] || !(mtmp->mflee)){
				mtmp->mcanmove = 1;
				mtmp->mfrozen = 0;
			}
		}
	}
   if(mtmp->data == &mons[PM_GIANT_TURTLE] && mtmp->mflee && !mtmp->mcanmove)
     tmp -=6;  /* don't penalize enshelled turtles */
	if (is_orc(mtmp->data) && maybe_polyd(is_elf(youmonst.data),
			Race_if(PM_ELF)))
	    tmp++;
#ifdef CONVICT
    /* Adding iron ball as a weapon skill gives a -4 penalty for
    unskilled vs no penalty for non-weapon objects.  Add 4 to
    compensate. */
    if (uwep && (uwep->otyp == HEAVY_IRON_BALL)) {
        tmp += 4;   /* Compensate for iron ball weapon skill -4
                    penalty for unskilled vs no penalty for non-
                    weapon objects. */
    }
#endif /* CONVICT */
	if(Role_if(PM_MONK) && !Upolyd) {
		static boolean armmessage = TRUE;
	    if (uarm) {
			if(armmessage) Your("armor is rather cumbersome...");
			armmessage = FALSE;
			tmp -= 20; /*flat -20 for monks in armor*/
	    } else {
			if(!armmessage) armmessage = TRUE;
			if (!uwep && !uarms) {
				tmp += (u.ulevel / 3) + 2;
			}
	    }
	}

/*	with a lot of luggage, your agility diminishes */
	if ((tmp2 = near_capacity()) != 0) tmp -= (tmp2*2) - 1;
	if (u.utrap) tmp -= 3;
	
    static int cbootsa = 0;
    if (!cbootsa) cbootsa = find_cboots();
    if (uarmf && uarmf->otyp == cbootsa) tmp++; 
	
	return tmp;
}

#define ATTK_AMON		0
#define ATTK_CHUPOCLOPS	1
#define ATTK_IRIS		2
#define ATTK_NABERIUS	3
#define ATTK_OTIAX		4
#define ATTK_SIMURGH	5
#define ATTK_MISKA_ARM	6
#define ATTK_MISKA_WOLF	7
#define SPIRIT_NATTKS	(8+7+5+2)
//+7 for up to seven extra iris attacks, +2 for second wolf head and second arm
/**/
static struct attack spiritattack[] = 
{
	{AT_BUTT,AD_PHYS,1,9},
	{AT_BITE,AD_DRST,2,4},
	{AT_TENT,AD_IRIS,1,4},
	{AT_BITE,AD_NABERIUS,1,6},
	{AT_WISP,AD_OTIAX,1,5},
	{AT_CLAW,AD_SIMURGH,1,6},
	{AT_MARI, AD_PHYS, 1, 8},
	{AT_BITE, AD_DRST, 2, 4}
};
struct attack curspiritattacks[SPIRIT_NATTKS];
int mari;

/* try to attack; return FALSE if monster evaded */
/* u.dx and u.dy must be set */
boolean
attack(mtmp)
register struct monst *mtmp;
{
	int tmp,weptmp,tchtmp;
	boolean keepattacking;
	register struct permonst *mdat = mtmp->data;
	int x = mtmp->mx, y = mtmp->my, attacklimit, attacksmade;
	
	//Reset multi-arm count to zero
	mari = 0;
	
	/*
	 * Since some characters attack multiple times in one turn,
	 * allow user to specify a count prefix for 'F' to limit
	 * number of attack (to avoid possibly hitting a dangerous
	 * monster multiple times in a row and triggering lots of
	 * retalitory attacks).
	 *
	 */
	/* kludge to work around parse()'s pre-decrement of `multi' */
	attacklimit = (multi || save_cm) ? multi + 1 : 0;
	multi = 0;		/* reset; it's been used up */
	
	/* This section of code provides protection against accidentally
	 * hitting peaceful (like '@') and tame (like 'd') monsters.
	 * Protection is provided as long as player is not: blind, confused,
	 * hallucinating or stunned.
	 * changes by wwp 5/16/85
	 * More changes 12/90, -dkh-. if its tame and safepet, (and protected
	 * 07/92) then we assume that you're not trying to attack. Instead,
	 * you'll usually just swap places if this is a movement command
	 */
	/* Intelligent chaotic weapons (Stormbringer) want blood */
	if (is_safepet(mtmp) && !flags.forcefight) {
	    if (!uwep || !spec_ability2(uwep, SPFX2_BLDTHRST)) {
		/* there are some additional considerations: this won't work
		 * if in a shop or Punished or you miss a random roll or
		 * if you can walk thru walls and your pet cannot (KAA) or
		 * if your pet is a long worm (unless someone does better).
		 * there's also a chance of displacing a "frozen" monster.
		 * sleeping monsters might magically walk in their sleep.
		 */
		boolean foo = (Punished || !rn2(7) || is_longworm(mtmp->data)),
			inshop = FALSE;
		char *p;

		for (p = in_rooms(mtmp->mx, mtmp->my, SHOPBASE); *p; p++)
		    if (tended_shop(&rooms[*p - ROOMOFFSET])) {
			inshop = TRUE;
			break;
		    }

		if (inshop || foo ||
			(IS_ROCK(levl[u.ux][u.uy].typ) &&
					!passes_walls(mtmp->data))) {
		    char buf[BUFSZ];

		    monflee(mtmp, rnd(6), FALSE, FALSE);
		    Strcpy(buf, y_monnam(mtmp));
		    buf[0] = highc(buf[0]);
		    You("stop.  %s is in the way!", buf);
		    return(TRUE);
		} else if ((mtmp->mfrozen || (! mtmp->mcanmove)
				|| (mtmp->data->mmove == 0)) && rn2(6)) {
		    pline("%s doesn't seem to move!", Monnam(mtmp));
		    return(TRUE);
		} else return(FALSE);
	    }
	}

	/* possibly set in attack_checks;
	   examined in known_hitum, called via hitum or hmonas below */
	override_confirmation = FALSE;
	if (attack_checks(mtmp, uwep)) return(TRUE);

	if (Upolyd) {
		/* certain "pacifist" monsters don't attack */
		if(noattacks(youracedata)) {
			You("have no way to attack monsters physically.");
			mtmp->mstrategy &= ~STRAT_WAITMASK;
			goto atk_done;
		}
	}

	if(check_capacity("You cannot fight while so heavily loaded."))
	    goto atk_done;

	if (u.twoweap && !test_twoweapon())
		untwoweapon();

	if(unweapon) {
	    unweapon = FALSE;
	    if(flags.verbose) {
			if(uwep){
				if(uwep->oartifact == ART_LIECLEAVER) 
					You("begin slashing monsters with your %s.", aobjnam(uwep, (char *)0));
				else You("begin bashing monsters with your %s.",
					aobjnam(uwep, (char *)0));
			} else if (!cantwield(youracedata)){
				if(u.specialSealsActive&SEAL_BLACK_WEB)
					You("begin slashing monsters with your shadow-blades.");
				else
					You("begin %sing monsters with your %s %s.",
					Role_if(PM_MONK) ? "strik" : "bash",
					uarmg ? "gloved" : "bare",	/* Del Lamb */
					makeplural(body_part(HAND)));
			}
	    }
	}
	exercise(A_STR, TRUE);		/* you're exercising muscles */
	/* andrew@orca: prevent unlimited pick-axe attacks */
	u_wipe_engr(3);

	/* Is the "it died" check actually correct? */
	if(mdat->mlet == S_LEPRECHAUN && !mtmp->mfrozen && !mtmp->msleeping &&
	   !mtmp->mconf && !is_blind(mtmp) && !rn2(7) &&
	   (m_move(mtmp, 0) == 2 ||			    /* it died */
	   mtmp->mx != u.ux+u.dx || mtmp->my != u.uy+u.dy)) /* it moved */
		return(FALSE);
	
	find_to_hit_rolls(mtmp,&tmp,&weptmp,&tchtmp);
	
	check_caitiff(mtmp);
	
	if((u.specialSealsActive&SEAL_BLACK_WEB) && u.spiritPColdowns[PWR_WEAVE_BLACK_WEB] > moves+20){
		static struct attack webattack[] = 
		{
			{AT_SHDW,AD_SHDW,4,8},
			{0,0,0,0}
		};
		if(u.spiritPColdowns[PWR_WEAVE_BLACK_WEB] > moves+20){
			struct monst *mon;
			int i, tmp, weptmp, tchtmp;
			for(i=0; i<8;i++){
				if(isok(u.ux+xdir[i],u.uy+ydir[i])){
					mon = m_at(u.ux+xdir[i],u.uy+ydir[i]);
					if(mon && !mon->mpeaceful && mon != mtmp){
						find_to_hit_rolls(mon,&tmp,&weptmp,&tchtmp);
						hmonwith(mon, tmp, weptmp, tchtmp, webattack, 1);
					}
				}
			}
		}
	}
	
	if((u.specialSealsActive&SEAL_BLACK_WEB) && ((u.twoweap && !uswapwep) || (!u.twoweap && !uwep))){
		if(u.twoweap){
			if(!uwep){
				static struct attack webattack[] = 
				{
					{AT_SHDW,AD_SHDW,4,8},
					{AT_SHDW,AD_SHDW,4,8},
					{0,0,0,0}
				};
				hmonwith(mtmp, tmp, weptmp, tchtmp, webattack, 2);
			} else {
				static struct attack webattack[] = 
				{
					{AT_WEAP,AD_PHYS,0,0},
					{AT_SHDW,AD_SHDW,4,8},
					{0,0,0,0}
				};
				u.twoweap = 0; //Otherwise hits 3x, weapon-fist-shadow
				hmonwith(mtmp, tmp, weptmp, tchtmp, webattack, 2);
				u.twoweap = 1;
			}
		} else {
			static struct attack webattack[] = 
			{
				{AT_SHDW,AD_SHDW,4,8},
				{0,0,0,0}
			};
			hmonwith(mtmp, tmp, weptmp, tchtmp, webattack, 1);
		}
	} else {

		if (Upolyd 
			|| Race_if(PM_VAMPIRE) 
			|| Race_if(PM_CHIROPTERAN) 
			|| (!uwep && Race_if(PM_YUKI_ONNA))
		){
			keepattacking = hmonas(mtmp, youracedata, tmp, weptmp, tchtmp);
			attacksmade = 1;
		} else {
			keepattacking = hitum(mtmp, weptmp, youmonst.data->mattk);
			attacksmade = 1;
		}
		if(uwep && uwep->oartifact == ART_QUICKSILVER){
			if(keepattacking && u.ulevel > 10 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-10, youmonst.data->mattk);
			if(keepattacking && u.ulevel > 20 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-20, youmonst.data->mattk);
			if(keepattacking && u.ulevel ==30 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-30, youmonst.data->mattk);
		}
		if(Role_if(PM_BARBARIAN) && !Upolyd){
			if(keepattacking && u.ulevel >= 10 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-10, youmonst.data->mattk);
			if(keepattacking && u.ulevel >= 20 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-20, youmonst.data->mattk);
			if(keepattacking && u.ulevel == 30 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp-30, youmonst.data->mattk);
		}
		if(uwep && uwep->oartifact == ART_STAFF_OF_TWELVE_MIRRORS){
			if(keepattacking && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp, youmonst.data->mattk);
			if(Role_if(PM_BARBARIAN) && !Upolyd){
				if(keepattacking && u.ulevel >= 10 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-10, youmonst.data->mattk);
				if(keepattacking && u.ulevel >= 20 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-20, youmonst.data->mattk);
				if(keepattacking && u.ulevel == 30 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-30, youmonst.data->mattk);
			}
		}
		if(uwep && uwep->otyp == QUARTERSTAFF && martial_bonus() && P_SKILL(weapon_type(uwep)) >= P_EXPERT && P_SKILL(weapon_type((struct obj *)0)) >= P_EXPERT){
			if(keepattacking && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
				keepattacking = hitum(mtmp, weptmp, youmonst.data->mattk);
			if(Role_if(PM_BARBARIAN) && !Upolyd){
				if(keepattacking && u.ulevel >= 10 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-10, youmonst.data->mattk);
				if(keepattacking && u.ulevel >= 20 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-20, youmonst.data->mattk);
				if(keepattacking && u.ulevel == 30 && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp && (!attacklimit || attacksmade++ < attacklimit) && (multi==0)) 
					keepattacking = hitum(mtmp, weptmp-30, youmonst.data->mattk);
			}
		}
	}
	if((u.sealsActive || u.specialSealsActive) && keepattacking && !DEADMONSTER(mtmp) && m_at(x, y) == mtmp){
		static int nspiritattacks;
		nspiritattacks = 0;
		if(u.specialSealsActive&SEAL_MISKA){
			if(u.ulevel >= 26){
				curspiritattacks[nspiritattacks++] = spiritattack[ATTK_MISKA_ARM];
				curspiritattacks[nspiritattacks++] = spiritattack[ATTK_MISKA_ARM];
			}
			if(u.ulevel >= 18) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_MISKA_WOLF];
			if(u.ulevel >= 10) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_MISKA_WOLF];
		}
		if(u.sealsActive&SEAL_AMON) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_AMON];
		if(u.sealsActive&SEAL_CHUPOCLOPS) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_CHUPOCLOPS];
		if(u.sealsActive&SEAL_IRIS){
			curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			if(u.twoweap || (uwep && bimanual(uwep,youracedata))) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			if(u.specialSealsActive&SEAL_MISKA && u.ulevel >= 26){
				curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
				if(u.twoweap || (uwep && bimanual(uwep,youracedata))) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			}
		}
		if(youracedata == &mons[PM_MARILITH]){
			curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
			curspiritattacks[nspiritattacks++] = spiritattack[ATTK_IRIS];
		}
		if(u.sealsActive&SEAL_NABERIUS) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_NABERIUS];
		if(u.sealsActive&SEAL_OTIAX){
			int tendrils,tmp2;
			tendrils = rnd(5);
			tmp2 = spiritDsize();
			tendrils = min(tendrils,tmp2);
			for(;tendrils>0;tendrils--) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_OTIAX];
		}
		if(u.sealsActive&SEAL_SIMURGH) curspiritattacks[nspiritattacks++] = spiritattack[ATTK_SIMURGH];
		if(nspiritattacks) keepattacking = hmonwith(mtmp, tmp, weptmp, tchtmp,curspiritattacks,nspiritattacks);
	}
	mtmp->mstrategy &= ~STRAT_WAITMASK;
	
	if(keepattacking && u.sealsActive&SEAL_SHIRO){
		int i,dx,dy;
		struct obj *otmp;
		for(i=rnd(8);i>0;i--){
			dx = rn2(3)-1;
			dy = rn2(3)-1;
			otmp = mksobj(ROCK, TRUE, FALSE);
			otmp->blessed = 0;
			otmp->cursed = 0;
			if((dx||dy) && !DEADMONSTER(mtmp)){
				set_destroy_thrown(1); //state variable referenced in drop_throw
					m_throw(&youmonst, mtmp->mx + dx, mtmp->my + dy, -dx, -dy,
						1, otmp,TRUE);
				set_destroy_thrown(0);  //state variable referenced in drop_throw
			}
			else {
				set_destroy_thrown(1); //state variable referenced in drop_throw
					m_throw(&youmonst, u.ux, u.uy, u.dx, u.dy, 1, otmp,TRUE);
				set_destroy_thrown(0);  //state variable referenced in drop_throw
			}
		}
	}
	
	if(keepattacking && !DEADMONSTER(mtmp) && u.sealsActive&SEAL_AHAZU){
		if(mtmp->mhp < .1*mtmp->mhpmax){
#define MAXVALUE 24
			extern const int monstr[];
			int value = min(monstr[monsndx(mtmp->data)] + 1,MAXVALUE);
			pline("%s sinks into your deep black shadow!", Monnam(mtmp));
			cprefx(monsndx(mtmp->data), TRUE, TRUE);
			cpostfx(monsndx(mtmp->data), FALSE, TRUE, FALSE);
			if(u.ugangr[Align2gangr(u.ualign.type)]) {
				u.ugangr[Align2gangr(u.ualign.type)] -= ((value * (u.ualign.type == A_CHAOTIC ? 2 : 3)) / MAXVALUE);
				if(u.ugangr[Align2gangr(u.ualign.type)] < 0) u.ugangr[Align2gangr(u.ualign.type)] = 0;
			} else if(u.ualign.record < 0) {
				if(value > MAXVALUE) value = MAXVALUE;
				if(value > -u.ualign.record) value = -u.ualign.record;
				adjalign(value);
			} else if (u.ublesscnt > 0) {
				u.ublesscnt -=
				((value * (u.ualign.type == A_CHAOTIC ? 500 : 300)) / MAXVALUE);
				if(u.ublesscnt < 0) u.ublesscnt = 0;
			}
			mongone(mtmp);
		} else if(!rn2(5)){
			Your("deep black shadow pools under %s.", mon_nam(mtmp));
			mtmp->movement -= 12;
		}
	}
atk_done:
	/* see comment in attack_checks() */
	/* we only need to check for this if we did an attack_checks()
	 * and it returned 0 (it's okay to attack), and the monster didn't
	 * evade.
	 */
	if (flags.forcefight && mtmp && mtmp->mhp > 0 && !canspotmon(mtmp) &&
	    !glyph_is_invisible(levl[u.ux+u.dx][u.uy+u.dy].glyph) &&
	    !(u.uswallow && mtmp == u.ustuck))
		map_invisible(u.ux+u.dx, u.uy+u.dy);

	return(TRUE);
}

STATIC_OVL boolean
known_hitum(mon, mhit, uattk)	/* returns TRUE if monster still lives */
register struct monst *mon;
register int *mhit;
struct attack *uattk;
{
	register boolean malive = TRUE;

	if (override_confirmation) {
	    /* this may need to be generalized if weapons other than
	       Stormbringer acquire similar anti-social behavior... */
	    if (flags.verbose) Your("bloodthirsty weapon attacks!");
	}

	if(!*mhit) {
	    missum(mon, uattk);
	} else {
	    int oldhp = mon->mhp,
		x = u.ux + u.dx, y = u.uy + u.dy;

	    /* KMH, conduct */
	    if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
			u.uconduct.weaphit++;

	    /* we hit the monster; be careful: it might die or
	       be knocked into a different location */
	    notonhead = (mon->mx != x || mon->my != y);
	    malive = hmon(mon, uwep, 0);
	    /* this assumes that Stormbringer was uwep not uswapwep */ 
	    if (malive && (u.twoweap 
			&& !(uwep && (uwep->otyp == STILETTOS))) 
			&& !override_confirmation 
			&& m_at(x, y) == mon
		) {
			malive = hmon(mon, uswapwep, 0);
		}
	    if (malive) {
		/* monster still alive */
		if(((!rn2(25) && mon->mhp < mon->mhpmax/2) 
			|| mon->data == &mons[PM_QUIVERING_BLOB]
			|| (uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_FEAR && !mindless_mon(mon) && !is_deaf(mon) && !rn2(4))
			)
			    && !(u.uswallow && mon == u.ustuck)
			    && !mindless_mon(mon)
		) {
		    /* maybe should regurgitate if swallowed? */
		    if(!rn2(3)) {
			monflee(mon, rnd(100), FALSE, TRUE);
		    } else monflee(mon, 0, FALSE, TRUE);

		    if(u.ustuck == mon && !u.uswallow && !sticks(youracedata))
			u.ustuck = 0;
		}
		/* Vorpal Blade hit converted to miss */
		/* could be headless monster or worm tail */
		if (mon->mhp == oldhp) {
		    *mhit = 0;
		    /* a miss does not break conduct */
		    if (uwep &&
			(uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
			--u.uconduct.weaphit;
		}
		if (mon->wormno && *mhit)
		    cutworm(mon, x, y, uwep);
	    }
	}
	return(malive);
}

STATIC_OVL boolean
known_hitum_wepi(mon, mhit, uattk, i)	/* returns TRUE if monster still lives */
register struct monst *mon;
register int *mhit;
struct attack *uattk;
int i;
{
	register boolean malive = TRUE;

	if (override_confirmation) {
	    /* this may need to be generalized if weapons other than
	       Stormbringer acquire similar anti-social behavior... */
	    if (flags.verbose) Your("bloodthirsty weapon attacks!");
	}

	if(!*mhit) {
	    missum(mon, uattk);
	} else {
	    int oldhp = mon->mhp, count=0,
		x = u.ux + u.dx, y = u.uy + u.dy;
		struct obj *marweap = 0;
		
		for(marweap = invent; marweap; marweap = marweap->nobj){
			if(((marweap->oclass == WEAPON_CLASS || is_weptool(marweap)) && !bimanual(marweap,youracedata)) 
				&& (marweap != uwep) && (marweap != uswapwep || !u.twoweap)
			) count++;
			if(count == i)
				break;
		}
		
	    /* KMH, conduct */
	    if (marweap && (marweap->oclass == WEAPON_CLASS || is_weptool(marweap)))
			u.uconduct.weaphit++;

	    /* we hit the monster; be careful: it might die or
	       be knocked into a different location */
	    notonhead = (mon->mx != x || mon->my != y);
	    malive = hmon(mon, marweap, 0);
	    if (malive) {
		/* monster still alive */
		if(((!rn2(25) && mon->mhp < mon->mhpmax/2)
			|| mon->data == &mons[PM_QUIVERING_BLOB]
			|| (uwep && uwep->oartifact == ART_SINGING_SWORD && uwep->osinging == OSING_FEAR && !rn2(4))
			)
			    && !(u.uswallow && mon == u.ustuck)
			    && !mindless_mon(mon)
		) {
		    /* maybe should regurgitate if swallowed? */
		    if(!rn2(3)) {
			monflee(mon, rnd(100), FALSE, TRUE);
		    } else monflee(mon, 0, FALSE, TRUE);

		    if(u.ustuck == mon && !u.uswallow && !sticks(youracedata))
			u.ustuck = 0;
		}
		/* Vorpal Blade hit converted to miss */
		/* could be headless monster or worm tail */
		if (mon->mhp == oldhp) {
		    *mhit = 0;
		    /* a miss does not break conduct */
		    if (marweap &&
			(marweap->oclass == WEAPON_CLASS || is_weptool(marweap)))
			--u.uconduct.weaphit;
		} else {
			if((oldhp - mon->mhp) > 1) use_skill(weapon_type(marweap),1);
		}
		if (mon->wormno && *mhit)
		    cutworm(mon, x, y, marweap);
	    }
	}
	return(malive);
}

STATIC_OVL boolean
hitum(mon, tmp, uattk)		/* returns TRUE if monster still lives */
struct monst *mon;
int tmp;
struct attack *uattk;
{
	boolean malive;
	int mhit = (tmp > (dieroll = rnd(20)) || u.uswallow);
	
	if(mhit && is_displacer(mon->data) && rn2(2)){
		You("hit a displaced image!");
		return TRUE;
	}

	if(tmp > dieroll) exercise(A_DEX, TRUE);
	malive = known_hitum(mon, &mhit, uattk);
	(void) passive(mon, mhit, malive, uattk->aatyp, uattk->adtyp);
	
	if(u.sealsActive&SEAL_MARIONETTE && uwep){
		struct monst *m2 = m_at(mon->mx+u.dx,mon->my+u.dy);
		if(!m2 || DEADMONSTER(m2) || m2==mon) return(malive);
		int mhit = (tmp > (dieroll = rnd(20)) || u.uswallow);

		if(tmp > dieroll) exercise(A_DEX, TRUE);
		(void) passive(m2, mhit, known_hitum(m2, &mhit, uattk), uattk->aatyp, uattk->adtyp);
		
	}
	return(malive);
}

boolean			/* general "damage monster" routine */
hmon(mon, obj, thrown)		/* return TRUE if mon still alive */
struct monst *mon;
struct obj *obj;
int thrown;
{
	boolean result, anger_guards;

	anger_guards = (mon->mpeaceful &&
			    (mon->ispriest || mon->isshk ||
			     mon->data == &mons[PM_WATCHMAN] ||
			     mon->data == &mons[PM_WATCH_CAPTAIN]));
	result = hmon_hitmon(mon, obj, thrown);
	if (mon->ispriest && !rn2(2)) ghod_hitsu(mon);
	if (anger_guards) (void)angry_guards(!flags.soundok);
	return result;
}

/* guts of hmon() */
STATIC_OVL boolean
hmon_hitmon(mon, obj, thrown)
struct monst *mon;
struct obj *obj;
int thrown;
{
	int tmp;
	struct permonst *mdat = mon->data;
	int barehand_silver_rings = 0, barehand_iron_rings = 0, barehand_unholy_rings = 0, barehand_jade_rings = 0;
	int eden_silver = 0;
	/* The basic reason we need all these booleans is that we don't want
	 * a "hit" message when a monster dies, so we have to know how much
	 * damage it did _before_ outputting a hit message, but any messages
	 * associated with the damage don't come out until _after_ outputting
	 * a hit message.
	 */
	boolean hittxt = FALSE, destroyed = FALSE, already_killed = FALSE;
	boolean get_dmg_bonus = TRUE;
	int ispoisoned = 0;
	boolean needpoismsg = FALSE, needfilthmsg = FALSE, needdrugmsg = FALSE, needsamnesiamsg = FALSE, 
			needacidmsg = FALSE, poiskilled = FALSE, 
			filthkilled = FALSE, druggedmon = FALSE, poisblindmon = FALSE, amnesiamon = FALSE;
	boolean silvermsg = FALSE,  ironmsg = FALSE,  unholymsg = FALSE, sunmsg = FALSE,
	silverobj = FALSE, ironobj = FALSE, unholyobj = FALSE, lightmsg = FALSE;
	boolean valid_weapon_attack = FALSE;
	boolean unarmed = !uwep && !uarm && !uarms;
	boolean phasearmor = FALSE;
#ifdef STEED
	int jousting = 0;
#endif
	boolean vapekilled = FALSE; /* WAC added boolean for vamps vaporize */
	int wtype;
	struct obj *monwep;
	char yourbuf[BUFSZ];
	char unconventional[BUFSZ];	/* substituted for word "attack" in msg */
	char saved_oname[BUFSZ];
	int unarmedMult = Race_if(PM_HALF_DRAGON) ? 3 : (!uarmg && u.sealsActive&SEAL_ECHIDNA) ? 2 : 1;
	if(uarmg){
		if(uarmg->oartifact == ART_GREAT_CLAWS_OF_URDLEN) unarmedMult += 2;
	}
	
	static short jadeRing = 0;
	if(!jadeRing) jadeRing = find_jade_ring();
	
	unconventional[0] = '\0';
	saved_oname[0] = '\0';
	
    static int tgloves = 0;
    if (!tgloves) tgloves = find_tgloves();
	
	if(!helpless(mon)) wake_nearto(mon->mx, mon->my, Stealth ? combatNoise(youracedata)/2 : combatNoise(youracedata)); //Nearby monsters may be awakened
	wakeup(mon, TRUE);
	if(!obj) {	/* attack with bare hands */
	    if (insubstantial(mdat) && !insubstantial_aware(mon, obj, FALSE)) tmp = 0;
		else if (martial_bonus()){
			if(uarmc && uarmc->oartifact == ART_GRANDMASTER_S_ROBE){
				if(u.sealsActive&SEAL_EURYNOME) tmp = rn2(2) ? 
											exploding_d(1,max_ints(4*unarmedMult,rnd(5)*2+2*unarmedMult),0)
												+exploding_d(1,max_ints(4*unarmedMult,rnd(5)*2+2*unarmedMult),0) : 
											exploding_d(1,max_ints(4*unarmedMult,rnd(5)*2+2*unarmedMult),0);
				else tmp = rn2(2) ? exploding_d(2,4*unarmedMult,0) : exploding_d(1,4*unarmedMult,0);
			}
			else{
				tmp = u.sealsActive&SEAL_EURYNOME ? exploding_d(1,max_ints(4*unarmedMult,rnd(5)*2+2*unarmedMult),0) : rnd(4*unarmedMult);	/* bonus for martial arts */
			}
			if(uarmg && uarmg->otyp == tgloves) tmp += 2;
		}
	    else {
			tmp = u.sealsActive&SEAL_EURYNOME ? exploding_d(1,max_ints(2*unarmedMult,rnd(5)*2),0) : rnd(2*unarmedMult);
		}
		if(uarmg && (uarmg->oartifact == ART_PREMIUM_HEART || uarmg->oartifact == ART_GREAT_CLAWS_OF_URDLEN)) tmp += uarmg->spe;
		if(u.specialSealsActive&SEAL_DAHLVER_NAR) tmp += d(2,6)+min(u.ulevel/2,(u.uhpmax - u.uhp)/10);
		if(uarmg && uarmg->otyp == tgloves) tmp += 1;
	    valid_weapon_attack = (tmp > 1);
		
		//The Annulus is very heavy
		if(uright && uright->oartifact == ART_ANNULUS) tmp = (tmp+uright->spe)*2;
		else if(uleft && uleft->oartifact == ART_ANNULUS) tmp = (tmp+uleft->spe)*2;
		
		if(uarmg){
			/* blessed gloves give bonuses when fighting 'bare-handed' */
			if (uarmg->blessed && (is_undead_mon(mon) || is_demon(mdat))) tmp += rnd(4);
			/* silver gloves give sliver bonus -CM */
			if ((uarmg->obj_material == SILVER || arti_silvered(uarmg)) &&
				hates_silver(mdat)){
					tmp += rnd(20);
					silvermsg = TRUE;
			}
			if ((uarmg->obj_material == IRON) &&
				hates_iron(mdat)){
					tmp += rnd(mon->m_lev);
					ironmsg = TRUE;
			}
			if (is_unholy(uarmg) &&
				hates_unholy(mdat)){
					tmp += rnd(9);
					unholymsg = TRUE;
			}
			{ //artifact block
				int basedamage = tmp;
				int newdamage = tmp;
				int dieroll = rnd(20);
				if(uarmg->oartifact){
					hittxt = artifact_hit(&youmonst, mon, uarmg, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
					newdamage = basedamage;
				}
				if(uarmg->oproperties){
					hittxt |= oproperty_hit(&youmonst, mon, uarmg, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
				}
			} //artifact block
		}
	    if (!uarmg || uarmg->oartifact == ART_CLAWS_OF_THE_REVENANCER) {
			/* So do silver rings.  Note: rings are worn under gloves, so you
			 * don't get both bonuses.
			 */
			if (uleft 
				&& (uleft->obj_material == SILVER 
					|| arti_silvered(uleft) 
					|| (uleft->ohaluengr
						&& (isEngrRing(uleft->otyp) || isSignetRing(uleft->otyp))
						&& uleft->oward >= LOLTH_SYMBOL && uleft->oward <= LOST_HOUSE
					   )
				)
			) barehand_silver_rings++;
			else if(u.sealsActive&SEAL_EDEN) eden_silver++;
			if (uright 
				&& (uright->obj_material == SILVER 
					|| arti_silvered(uright)
					|| (uright->ohaluengr
						&& (isEngrRing(uright->otyp) || isSignetRing(uright->otyp))
						&& uright->oward >= LOLTH_SYMBOL && uright->oward <= LOST_HOUSE
					   )
				)
			) barehand_silver_rings++;
			else if(u.sealsActive&SEAL_EDEN) eden_silver++;
			if ((barehand_silver_rings || eden_silver) && hates_silver(mdat)) {
			    tmp += d(barehand_silver_rings+eden_silver,20);
			    silvermsg = TRUE;
			}
			
			/* Do iron rings.  Note: rings are worn under gloves, so you
			 * don't get both bonuses.
			 */
			if (uleft 
				&& (uleft->obj_material == IRON)
			) barehand_iron_rings++;
			if (uright 
				&& (uright->obj_material == IRON)
			) barehand_iron_rings++;
			
			if ((barehand_iron_rings) && hates_iron(mdat)) {
			    tmp += d(barehand_iron_rings,mon->m_lev);
			    ironmsg = TRUE;
			}
			
			/* Do cursed rings.  Note: rings are worn under gloves, so you
			 * don't get both bonuses.
			 */
			if (uleft 
				&& (is_unholy(uleft))
			) barehand_unholy_rings++;
			if (uright 
				&& (is_unholy(uright))
			) barehand_unholy_rings++;
			
			if ((barehand_unholy_rings) && hates_unholy(mdat)) {
			    tmp += d(barehand_unholy_rings,9);
			    ironmsg = TRUE;
			}
			
			if (uleft 
				&& uleft->ohaluengr
				&& (isEngrRing(uleft->otyp) || isSignetRing(uleft->otyp))
				&& uleft->oward == EDDER_SYMBOL
			) tmp += 5;
			if (uright 
				&& uright->ohaluengr
				&& (isEngrRing(uright->otyp) || isSignetRing(uright->otyp))
				&& uright->oward == EDDER_SYMBOL
			) tmp += 5;
			
			if (uleft && uleft->otyp == jadeRing)
			    barehand_jade_rings++;
			if (uright && uright->otyp == jadeRing )
			    barehand_jade_rings++;
			if (barehand_jade_rings && hates_silver(mdat)) {
			    tmp += d(barehand_jade_rings, 20);
			    silvermsg = TRUE; /* jade ring handled in same code block as silver ring */
			}
			/* posioned rings */
			if(uright && uright->opoisoned && isSignetRing(uright->otyp)){
				if(uright->opoisoned & OPOISON_BASIC && !rn2(10)){
					if (resists_poison(mon))
						needpoismsg = TRUE;
					else poiskilled = TRUE;
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_FILTH && !rn2(10)){
					if (resists_sickness(mon))
						needfilthmsg = TRUE;
					else filthkilled = TRUE;
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_SLEEP && !rn2(5)){
					if (resists_sleep(mon))
						needdrugmsg = TRUE;
					else if(sleep_monst(mon, rnd(12), POTION_CLASS)) druggedmon = TRUE;
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_BLIND && !rn2(10)){
					if (resists_poison(mon))
						needpoismsg = TRUE;
					 else {
						poisblindmon = TRUE;
					}
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_PARAL && !rn2(8)){
						if (mon->mcanmove) {
							mon->mcanmove = 0;
							mon->mfrozen = rnd(25);
						}
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_AMNES && !rn2(10)){
					if(mindless_mon(mon)) needsamnesiamsg = TRUE;
					else amnesiamon = TRUE;
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
				if(uright->opoisoned & OPOISON_ACID){
					if(resists_acid(mon)) needacidmsg = TRUE;
					else tmp += rnd(10);
					
					if(uright->opoisonchrgs-- <= 0) uright->opoisoned = OPOISON_NONE;
				}
			}
			if(uleft && uleft->opoisoned && isSignetRing(uleft->otyp)){
				if(uleft->opoisoned & OPOISON_BASIC && !rn2(10)){
					if (resists_poison(mon))
						needpoismsg = TRUE;
					else poiskilled = TRUE;
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_FILTH && !rn2(10)){
					if (resists_sickness(mon))
						needfilthmsg = TRUE;
					else filthkilled = TRUE;
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_SLEEP && !rn2(5)){
					if (resists_sleep(mon))
						needdrugmsg = TRUE;
					else if(sleep_monst(mon, rnd(12), POTION_CLASS)) druggedmon = TRUE;
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_BLIND && !rn2(10)){
					if (resists_poison(mon))
						needpoismsg = TRUE;
					 else {
						poisblindmon = TRUE;
					}
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_PARAL && !rn2(8)){
						if (mon->mcanmove) {
							mon->mcanmove = 0;
							mon->mfrozen = rnd(25);
						}
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_AMNES && !rn2(10)){
					if(mindless_mon(mon)) needsamnesiamsg = TRUE;
					else amnesiamon = TRUE;
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
				if(uleft->opoisoned & OPOISON_ACID){
					if(resists_acid(mon)) needacidmsg = TRUE;
					else tmp += rnd(10);
					
					if(uleft->opoisonchrgs-- <= 0) uleft->opoisoned = OPOISON_NONE;
				}
			}
	    }
		if(uclockwork && !resists_fire(mon) && u.utemp){
			if(u.utemp >= BURNING_HOT && (!uarmg || is_metallic(uarmg))){
				int heatdie = min(u.utemp, 20);
				tmp += rnd(heatdie);
				if(u.utemp >= MELTING){
					tmp += rnd(heatdie);
					if(u.utemp >= MELTED) tmp += rnd(heatdie);
				}
			} else if(u.utemp >= HOT){
				int heatdie = min(u.utemp, 20);
				tmp += rnd(heatdie)/2;
				if(u.utemp >= MELTING){
					tmp += rnd(heatdie)/2;
					if(u.utemp >= MELTED) tmp += rnd(heatdie)/2;
				}
			}
		}
	} else {
	    Strcpy(saved_oname, cxname(obj));
		if(obj->opoisoned && is_poisonable(obj))
			ispoisoned = obj->opoisoned;
	    if(obj->oclass == WEAPON_CLASS || is_weptool(obj) ||
#ifdef CONVICT
	       obj->otyp == HEAVY_IRON_BALL ||
#endif /* CONVICT */
	       obj->oclass == GEM_CLASS) {

		/* is it not a melee weapon? */
		if (/* if you strike with a bow... */
		    is_launcher(obj) ||
		    /* or strike with a missile in your hand... */
		    (!thrown && obj->otyp != CHAKRAM &&
				(is_missile(obj) || is_ammo(obj))
			) ||
		    /* or use a pole at short range and not mounted... */
		    (	!thrown &&
#ifdef STEED
				!u.usteed &&
#endif
				is_pole(obj) && 
				obj->otyp != AKLYS && 
				obj->otyp != FORCE_PIKE && 
				obj->otyp != NAGINATA && 
				obj->oartifact != ART_WEBWEAVER_S_CROOK && 
				obj->oartifact != ART_SILENCE_GLAIVE && 
				obj->oartifact != ART_HEARTCLEAVER && 
				obj->oartifact != ART_SOL_VALTIVA && 
				obj->oartifact != ART_SHADOWLOCK && 
				obj->oartifact != ART_PEN_OF_THE_VOID
			) ||
		    /* lightsaber that isn't lit ;) */
		    (is_lightsaber(obj) && !litsaber(obj)) ||
		    /* houchou that isn't thrown */
		    (!thrown && obj->oartifact == ART_HOUCHOU) ||
		    /* or throw a missile without the proper bow... */
		    (is_ammo(obj) && !(ammo_and_launcher(obj, uwep) || obj->oclass == GEM_CLASS))
		) {
			int resistmask = 0;
			int weaponmask = 0;
			static int warnedotyp = 0;
			static struct permonst *warnedptr = 0;
		    /* then do only 1-2 points of damage */
		    if (insubstantial(mdat) && !insubstantial_aware(mon, obj, TRUE))
				tmp = 0;
		    else if(obj->oartifact == ART_LIECLEAVER) tmp = 2*(rnd(12) + rnd(10) + obj->spe) + skill_dam_bonus(P_SCIMITAR);
		    else if(obj->oartifact == ART_ROGUE_GEAR_SPIRITS) tmp = 2*(rnd(bigmonst(mon->data) ? 2 : 4) + obj->spe) + skill_dam_bonus(P_PICK_AXE);
			
		    else if((obj->oartifact == ART_INFINITY_S_MIRRORED_ARC && !litsaber(obj))) tmp = d(1,6) + obj->spe + weapon_dam_bonus(0); //martial arts aid
		    else if((obj->otyp == KAMEREL_VAJRA && !litsaber(obj))) tmp = d(1,4) + (bigmonst(mdat) ? 0 : 1) + obj->spe + skill_dam_bonus(P_MACE); //small mace
		    else if((is_lightsaber(obj) && !litsaber(obj))) tmp = d(1,4) + obj->spe + weapon_dam_bonus(0); //martial arts aid

			else tmp = rnd(2);
			
			if(obj->oartifact == ART_ROGUE_GEAR_SPIRITS){
				weaponmask |= PIERCE;
			} else if(obj->oartifact == ART_LIECLEAVER){
				weaponmask |= SLASH;
			} else if(obj->oartifact == ART_INFINITY_S_MIRRORED_ARC){
				weaponmask |= WHACK|SLASH;
			} else if(obj->otyp == KAMEREL_VAJRA){
				weaponmask |= WHACK|PIERCE;
			} else {
				weaponmask |= WHACK;
			}
			
			if(resist_blunt(mdat) || (mon->mfaction == ZOMBIFIED)){
				resistmask |= WHACK;
			}
			if(resist_pierce(mdat) || (mon->mfaction == ZOMBIFIED || mon->mfaction == SKELIFIED || mon->mfaction == CRYSTALFIED)){
				resistmask |= PIERCE;
			}
			if(resist_slash(mdat) || (mon->mfaction == SKELIFIED || mon->mfaction == CRYSTALFIED)){
				resistmask |= SLASH;
			}
			
			if((weaponmask & ~(resistmask)) == 0L && !narrow_spec_applies(obj, mon)){
				tmp /= 4;
				if(warnedptr != mdat){
					pline("%s is ineffective against %s.", The(xname(obj)), mon_nam(mon));
					warnedptr = mdat;
				}
			} else {
				warnedptr = 0;
			}
			
			if(tmp > 1){
				if(obj->oartifact == ART_LIECLEAVER){
					use_skill(P_SCIMITAR,1);
				} else if(obj->oartifact == ART_ROGUE_GEAR_SPIRITS){
					use_skill(P_PICK_AXE,1);
				} else if(obj->otyp == KAMEREL_VAJRA && !litsaber(obj)){
					use_skill(P_MACE,1);
				} else if(is_lightsaber(obj) && !litsaber(obj)){
					use_skill(P_BARE_HANDED_COMBAT,1);
				}
			}
		    // if (tmp && obj->oartifact &&
				// artifact_hit(&youmonst, mon, obj, &tmp, dieroll)) {
				// if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
				    // return FALSE;
				// if (tmp == 0) return TRUE;
				// hittxt = TRUE;
			// }
			if(obj->oartifact == ART_GLAMDRING && (is_orc(mdat) || is_demon(mdat))){
				tmp += rnd(20); //I think this is the right thing to do here.  I don't think it enters the main silver section
				lightmsg = TRUE;
			}
		    if ((obj->obj_material == SILVER || arti_silvered(obj) || 
					(thrown && obj->otyp == SHURIKEN && uwep && uwep->oartifact == ART_SILVER_STARLIGHT) )
				&& hates_silver(mdat)) {
				tmp += rnd(20); //I think this is the right thing to do here.  I don't think it enters the main silver section
				if(obj->oartifact == ART_SUNSWORD) sunmsg = TRUE;
				else silvermsg = TRUE;
				silverobj = TRUE;
		    }
			if (obj->obj_material == IRON && hates_iron(mdat)) {
				tmp += rnd(mon->m_lev);	//I think this is the right thing to do here.  I don't think it enters the main iron section
				ironmsg = TRUE;
				ironobj = TRUE;
			}
			if (is_unholy(obj) && hates_unholy(mdat)) {
				tmp += rnd(9);	//I think this is the right thing to do here.  I don't think it enters the main unholy section
				unholymsg = TRUE;
				unholyobj = TRUE;
			}
		    if (!thrown && obj == uwep && obj->otyp == BOOMERANG &&
			    rnl(4) == 4-1 && obj->oartifact == 0) {
			boolean more_than_1 = (obj->quan > 1L);

			pline("As you hit %s, %s%s %s breaks into splinters.",
			      mon_nam(mon), more_than_1 ? "one of " : "",
			      shk_your(yourbuf, obj), xname(obj));
			if (!insubstantial(mdat) || insubstantial_aware(mon, obj, TRUE))
			    tmp++;
			if (!more_than_1) uwepgone();	/* set unweapon */
			useup(obj);
			if (!more_than_1) obj = (struct obj *) 0;
			hittxt = TRUE;
		    }
		} else {
			if(u.sealsActive&SEAL_MARIONETTE && distmin(u.ux, u.uy, mon->mx, mon->my) > 1)
				tmp = dmgval(obj, mon, SPEC_MARIONETTE);
			else if(obj == uwep && uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->ovar1&SEAL_MARIONETTE)
				tmp = dmgval(obj, mon, SPEC_MARIONETTE);
			else tmp = dmgval(obj, mon, 0);
			
			if(obj && ((is_lightsaber(obj) && litsaber(obj)) || arti_shining(obj))) phasearmor = TRUE;
		    
			/* a minimal hit doesn't exercise proficiency */
			valid_weapon_attack = (tmp > 1 || (obj && obj->otyp == SPOON && Role_if(PM_CONVICT)));
			if (!valid_weapon_attack || mon == u.ustuck || u.twoweap) {
			;	/* no special bonuses */
			} else {
				if (!(noncorporeal(mdat) || amorphous(mdat) || ((stationary(mdat) || sessile(mdat)) && (mdat->mlet == S_FUNGUS || mdat->mlet == S_PLANT)) || u.uswallow) && (
						((mon->mflee && mon->data != &mons[PM_BANDERSNATCH]) || is_blind(mon) || !mon->mcanmove || !mon->mnotlaugh || 
							mon->mstun || mon->mconf || mon->mtrapped || mon->msleeping || (mon->mux == 0 && mon->muy == 0) ||
								(sgn(mon->mx - u.ux) != sgn(mon->mx - mon->mux)
								&& sgn(mon->my - u.uy) != sgn(mon->my - mon->muy)) ||
								((mon->mux != u.ux || mon->muy != u.uy) && 
									((obj == uwep && uwep->oartifact == ART_LIFEHUNT_SCYTHE && has_head(mon->data) && !is_unalive(mon->data))
										|| distmin(u.ux, u.uy, mon->mx, mon->my) > BOLT_LIM)
								)
						) && 
						((Role_if(PM_ROGUE) && !Upolyd) ||
							(Role_if(PM_ANACHRONONAUT) && Race_if(PM_MYRKALFR) && !Upolyd) ||
							u.sealsActive&SEAL_ANDROMALIUS ||
							(obj == uwep && uwep->oartifact == ART_SPINESEEKER) ||
							(obj == uwep && uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->ovar1&SEAL_ANDROMALIUS) ||
							(Role_if(PM_CONVICT) && !Upolyd && obj == uwep && uwep->otyp == SPOON))
				)) {
					if((mon->mux != u.ux || mon->muy != u.uy) && distmin(u.ux, u.uy, mon->mx, mon->my) > BOLT_LIM)
						You("snipe the flat-footed %s!", l_monnam(mon));
					else if((mon->mux != u.ux || mon->muy != u.uy) && 
						(obj == uwep && uwep->oartifact == ART_LIFEHUNT_SCYTHE && !is_unalive(mon->data) && has_head(mon->data))
					){
						//Lifehunt scythe triggers sneak attack, but you have to actually HAVE sneak attack
						You("strike the flat-footed %s!", l_monnam(mon));
					} else if(mon->mflee || (mon->mux == 0 && mon->muy == 0) ||
						(sgn(mon->mx - u.ux) != sgn(mon->mx - mon->mux) 
						&& sgn(mon->my - u.uy) != sgn(mon->my - mon->muy))
					) You("strike %s from behind!", mon_nam(mon));
					else if(is_blind(mon)) You("strike the blinded %s!", l_monnam(mon));
					else if(mon->mtrapped) You("strike the trapped %s!", l_monnam(mon));
					else You("strike the helpless %s!", l_monnam(mon));
					
					if(obj == uwep && uwep->oartifact == ART_SPINESEEKER && !Upolyd) tmp += rnd(u.ulevel +
						((mon->mflee || (mon->mux == 0 && mon->muy == 0) ||
						  (sgn(mon->mx - u.ux) != sgn(mon->mx - mon->mux)
						  && sgn(mon->my - u.uy) != sgn(mon->my - mon->muy))) ? u.ulevel : 0));
					if(obj == uwep && uwep->oartifact == ART_PEN_OF_THE_VOID && uwep->ovar1&SEAL_ANDROMALIUS) 
						tmp += rnd(u.ulevel + ((mvitals[PM_ACERERAK].died > 0 ? u.ulevel/2 : 0)));
					if(Role_if(PM_ROGUE) &&!Upolyd) tmp += rnd(u.ulevel + ((obj == uwep && uwep->oartifact == ART_SILVER_STARLIGHT ? u.ulevel/2 : 0)));
					if(u.sealsActive&SEAL_ANDROMALIUS) tmp += rnd(u.ulevel + ((obj == uwep && uwep->oartifact == ART_SILVER_STARLIGHT ? u.ulevel/2 : 0)));
					if(Role_if(PM_CONVICT) && !Upolyd && obj == uwep && uwep->otyp == SPOON) tmp += rnd(u.ulevel);
					hittxt = TRUE;
				}
				if(obj == uwep && is_lightsaber(uwep) && litsaber(uwep)){
					if (
						(mon->mflee && mon->data != &mons[PM_BANDERSNATCH]) || is_blind(mon) || !mon->mcanmove || !mon->mnotlaugh ||
							mon->mstun || mon->mconf || mon->mtrapped || mon->msleeping || (mon->mux == 0 && mon->muy == 0) || stationary(mdat) || sessile(mdat) ||
								((mon->mux != u.ux || mon->muy != u.uy) && distmin(u.ux, u.uy, mon->mx, mon->my) == 1)
					) {
						if(P_SKILL(weapon_type(uwep)) >= P_BASIC){
							if(P_SKILL(FFORM_SHII_CHO) >= P_BASIC){
								if(u.fightingForm == FFORM_SHII_CHO || 
								 (u.fightingForm == FFORM_JUYO && (!uarm || is_light_armor(uarm)))
								) use_skill(FFORM_JUYO,1);
							}
						}
						if(u.fightingForm == FFORM_JUYO && (!uarm || is_light_armor(uarm))){
							if(stationary(mdat) || sessile(mdat)) You("rain blows on the immobile %s!", mon_nam(mon));
							else if(mon->mflee || (mon->mux == 0 && mon->muy == 0) ||
								(sgn(mon->mx - u.ux) != sgn(mon->mx - mon->mux) 
								&& sgn(mon->my - u.uy) != sgn(mon->my - mon->muy))
							) You("rain blows on %s from behind!", mon_nam(mon));
							else if(is_blind(mon)) You("rain blows on the blinded %s!", l_monnam(mon));
							else if(mon->mtrapped) You("rain blows on the trapped %s!", l_monnam(mon));
							else You("rain blows on the helpless %s!", l_monnam(mon));
							
							tmp += rnd(u.ulevel);
							
							switch(min(P_SKILL(FFORM_JUYO), P_SKILL(weapon_type(uwep)))){
								case P_BASIC:
									youmonst.movement += NORMAL_SPEED/4;
								break;
								case P_SKILLED:
									youmonst.movement += NORMAL_SPEED/3;
								break;
								case P_EXPERT:
									youmonst.movement += NORMAL_SPEED/2;
								break;
							}
							
							if(rnd(20) < min(P_SKILL(FFORM_JUYO), P_SKILL(weapon_type(uwep)))){
								if (canspotmon(mon))
									pline("%s %s from your powerful strikes!", Monnam(mon),
									  makeplural(stagger(mon->data, "stagger")));
								/* avoid migrating a dead monster */
								if (mon->mhp > tmp) {
									mhurtle(mon, u.dx, u.dy, 1);
									mon->mstun = TRUE;
									mdat = mon->data; /* in case of a polymorph trap */
									if (DEADMONSTER(mon)) already_killed = TRUE;
								}
								hittxt = TRUE;
							}
						}
					} else if(u.fightingForm == FFORM_JUYO && (!uarm || is_light_armor(uarm)) && 
						rnd(50) < min(P_SKILL(FFORM_JUYO), P_SKILL(weapon_type(uwep)))
					){
						if (canspotmon(mon))
							pline("%s %s from your powerful strike!", Monnam(mon),
							  makeplural(stagger(mon->data, "stagger")));
						/* avoid migrating a dead monster */
						if (mon->mhp > tmp) {
							mhurtle(mon, u.dx, u.dy, 1);
							mon->mstun = TRUE;
							mdat = mon->data; /* in case of a polymorph trap */
							if (DEADMONSTER(mon)) already_killed = TRUE;
						}
						hittxt = TRUE;
					}
					if(u.fightingForm == FFORM_DJEM_SO && (!uarm || is_light_armor(uarm) || is_medium_armor(uarm))){
						if(rnd(mon->mattackedu ? 20 : 100) < min(P_SKILL(FFORM_DJEM_SO), P_SKILL(weapon_type(uwep)))){
							if (canspotmon(mon))
								pline("%s %s from your powerful strike!", Monnam(mon),
								  makeplural(stagger(mon->data, "stagger")));
							/* avoid migrating a dead monster */
							if (mon->mhp > tmp) {
								mhurtle(mon, u.dx, u.dy, 1);
								mon->mstun = TRUE;
								mdat = mon->data; /* in case of a polymorph trap */
								if (DEADMONSTER(mon)) already_killed = TRUE;
							}
							hittxt = TRUE;
						}
					}
				}
				
				if(uwep 
					&& obj == uwep 
					&& uwep->otyp == RAKUYO
					&& !u.twoweap
				){
					youmonst.movement -= NORMAL_SPEED/4;
				}
				
				if(uwep 
					&& obj == uwep 
					&& uwep->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD 
				){
					if((dieroll <= 2+P_SKILL(uwep_skill_type())) &&
					((monwep = MON_WEP(mon)) != 0 &&
						!is_flimsy(monwep) &&
						!obj_resists(monwep, 0, 100))
					){
						setmnotwielded(mon,monwep);
						MON_NOWEP(mon);
						mon->weapon_check = NEED_WEAPON;
						pline("%s %s %s from the force of your blow!",
							  s_suffix(Monnam(mon)), xname(monwep),
							  otense(monwep, "shatter"));
						m_useup(mon, monwep);
						/* If someone just shattered MY weapon, I'd flee! */
						if (rn2(4) && !mindless_mon(mon)) {
							monflee(mon, d(2,3), TRUE, TRUE);
						}
						hittxt = TRUE;
					}
					if(rnd(20) < P_SKILL(weapon_type(uwep))){
						if (canspotmon(mon))
							pline("%s %s from your powerful blow!", Monnam(mon),
							  makeplural(stagger(mon->data, "stagger")));
						/* avoid migrating a dead monster */
						if (mon->mhp > tmp) {
							mhurtle(mon, sgn(mon->mx - u.ux), sgn(mon->my - u.uy), 1);
							mon->mstun = TRUE;
							mdat = mon->data; /* in case of a polymorph trap */
							if (DEADMONSTER(mon)) already_killed = TRUE;
						}
					}
				} else if ( (( (dieroll <= 2
						|| (Role_if(PM_BARBARIAN) && dieroll <= 4)
					) && 
					  obj == uwep &&
					  obj->oclass == WEAPON_CLASS &&
					  (bimanual(obj,youracedata) ||
						(Role_if(PM_SAMURAI) && obj->otyp == KATANA && !uarms) ||
						(obj->oartifact == ART_PEN_OF_THE_VOID && obj->ovar1&SEAL_BERITH)) &&
					  ((wtype = uwep_skill_type()) != P_NONE &&
					  P_SKILL(wtype) >= P_SKILLED)
					 ) || arti_shattering(obj)
					) &&
					((monwep = MON_WEP(mon)) != 0 &&
						!is_flimsy(monwep) &&
						!obj_resists(monwep, 0, 100))
				) {
					/*
					 * 5% chance of shattering defender's weapon when
					 * using a two-handed weapon; less if uwep is rusted.
					 * [dieroll == 2 is most successful non-beheading or
					 * -bisecting hit, in case of special artifact damage;
					 * the percentage chance is (2/20)*(50/100).]
					 * Barbarians get a 10% chance.
					 */
					setmnotwielded(mon,monwep);
					MON_NOWEP(mon);
					mon->weapon_check = NEED_WEAPON;
					if(is_lightsaber(obj)) Your("energy blade slices %s %s in two!",
						  s_suffix(mon_nam(mon)), xname(monwep));
					else pline("%s %s %s from the force of your blow!",
						  s_suffix(Monnam(mon)), xname(monwep),
						  otense(monwep, "shatter"));
					m_useup(mon, monwep);
					/* If someone just shattered MY weapon, I'd flee! */
					if (rn2(4) && !mindless_mon(mon)) {
						monflee(mon, d(2,3), TRUE, TRUE);
					}
					hittxt = TRUE;
				}
			}
			// if(uarm && uarm->otyp <= YELLOW_DRAGON_SCALES && uarm->otyp >= GRAY_DRAGON_SCALE_MAIL){
				// dragon_hit(mon, uarm, uarm->otyp, &tmp, &needpoismsg, &poiskilled, &druggedmon);
			// }
			{ //artifact block
				int basedamage = tmp;
				int newdamage = tmp;
				if (obj->oartifact) {
					hittxt = artifact_hit(&youmonst, mon, obj, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
					newdamage = basedamage;
				}
				if(obj->oproperties){
					hittxt |= oproperty_hit(&youmonst, mon, obj, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
				}
			} //artifact block
			if((monwep = MON_WEP(mon)) != 0 && monwep->oartifact != ART_GLAMDRING &&
				(arti_disarm(obj) || (obj->otyp == RANSEUR && 
										(wtype = uwep_skill_type()) != P_NONE && 
										P_SKILL(wtype) >= P_BASIC && 
										!rn2(20 - 5*P_SKILL(wtype))) /*Need to be separate from dieroll, */
				)){													/*	since Ranseurs are bimanual!*/
				if(monwep->quan > 1L){
					monwep = splitobj(monwep, 1L);
					obj_extract_self(monwep); //wornmask is cleared by splitobj
				} else{
					long unwornmask;
					if ((unwornmask = monwep->owornmask) != 0L) {
						mon->misc_worn_check &= ~unwornmask;
					}
					setmnotwielded(mon,monwep);
					MON_NOWEP(mon);
					mon->weapon_check = NEED_WEAPON;
					obj_extract_self(monwep);
					monwep->owornmask = 0L;
					update_mon_intrinsics(mon, monwep, FALSE, FALSE);
				}
				pline("%s %s is snagged by your %s.",
				      s_suffix(Monnam(mon)), xname(monwep), xname(obj));
				getdir((char *)0);
				if(u.dx || u.dy){
					You("toss it away.");
					m_throw(&youmonst, u.ux, u.uy, u.dx, u.dy,
							(int)((ACURRSTR)/2 - monwep->owt/40 + obj->spe) , monwep,TRUE);
				}
				else{
					You("drop it at your feet.");
					(void) dropy(monwep);
				}
				nomul(0, NULL);
				if(mon->mhp <= 0) /* flung weapon killed monster */
				    return FALSE;
				hittxt = TRUE;
			}
			if(obj->oartifact == ART_GLAMDRING && (is_orc(mdat) || is_demon(mdat))){
				lightmsg = TRUE;
			}
		    if ((obj->obj_material == SILVER || arti_silvered(obj)  || 
					(thrown && obj->otyp == SHURIKEN && uwep && uwep->oartifact == ART_SILVER_STARLIGHT) )
			   && hates_silver(mdat) && !(is_lightsaber(obj) && litsaber(obj))) {
				if(obj->oartifact == ART_SUNSWORD) sunmsg = TRUE;
				else silvermsg = TRUE;
				silverobj = TRUE;
		    }
		    if (obj->obj_material == IRON && hates_iron(mdat) && !(is_lightsaber(obj) && litsaber(obj))) {
				ironmsg = TRUE;
				ironobj = TRUE;
		    }
		    if (is_unholy(obj) && hates_unholy(mdat)) {
				unholymsg = TRUE;
				unholyobj = TRUE;
		    }
#ifdef STEED
		    if (u.usteed && !thrown && tmp > 0 &&
			    (weapon_type(obj) == P_LANCE ||
					(obj->oartifact && 
						(obj->oartifact == ART_ROD_OF_SEVEN_PARTS || 
							(obj->oartifact == ART_PEN_OF_THE_VOID && obj->ovar1&SEAL_BERITH)
						)
					)
				)&&
				mon != u.ustuck) {
				jousting = joust(mon, obj);
				/* exercise skill even for minimal damage hits */
				if (jousting) valid_weapon_attack = TRUE;
		    }
#endif
		    if (thrown && (is_ammo(obj) || is_missile(obj))) {
//			if (obj->oartifact == ART_HOUCHOU) {
//			    pline("There is a bright flash as it hits %s.",
//				the(mon_nam(mon)));
//			    tmp = dmgval(obj, mon, 0);
//			}
				if (ammo_and_launcher(obj, uwep)) {

			    /* Elves and Samurai do extra damage using
			     * their bows&arrows; they're highly trained.
			     */
					if(u.sealsActive&SEAL_DANTALION && tp_sensemon(mon)) tmp += max(0,(ACURR(A_INT)-10)/2);
					
				    if (Role_if(PM_SAMURAI) &&
						obj->otyp == YA && uwep->otyp == YUMI)
							tmp++;
				    else if (Race_if(PM_ELF) &&
				     obj->otyp == ELVEN_ARROW &&
				     uwep->otyp == ELVEN_BOW)
							tmp++;
					
					{//Artifact block
					int basedamage = tmp;
					int newdamage = tmp;
					if(uwep->oartifact){
						hittxt = artifact_hit(&youmonst, mon, uwep, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
						newdamage = basedamage;
					}
					if(uwep->oproperties){
						hittxt |= oproperty_hit(&youmonst, mon, uwep, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
						newdamage = basedamage;
					}
					if(obj->oartifact){
						hittxt = artifact_hit(&youmonst, mon, obj, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
						newdamage = basedamage;
					}
					if(obj->oproperties){
						hittxt |= oproperty_hit(&youmonst, mon, obj, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
						newdamage = basedamage;
					}
					if(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_ARCANE_ARCHER){
						hittxt = artifact_hit(&youmonst, mon, uarmh, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
						newdamage = basedamage;
					}
					if(uarmh && uarmh->oartifact 
					&& uarmh->oartifact == ART_HELM_OF_THE_ARCANE_ARCHER && uarmh->oproperties
					){
						hittxt |= oproperty_hit(&youmonst, mon, obj, &newdamage, dieroll);
						if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
							return FALSE; /* NOTE: worried this might cause crash from improperly handled arrows */
						if (newdamage == 0) return TRUE; /* NOTE: ditto */
						tmp += (newdamage - basedamage);
					}
					}//Artifact block
				}
			}
		}
	    } else if(obj->oclass == POTION_CLASS) {
			if (obj->quan > 1L)
			    obj = splitobj(obj, 1L);
			else
			    setuwep((struct obj *)0);
			freeinv(obj);
			potionhit(mon, obj, TRUE);
			obj = (struct obj *)0;
			if (mon->mhp <= 0) return FALSE;	/* killed */
			hittxt = TRUE;
			/* in case potion effect causes transformation */
			mdat = mon->data;
			tmp = (insubstantial(mdat) && !insubstantial_aware(mon, obj, TRUE)) ? 0 : 1;
		} else {
			if (insubstantial(mdat) && !insubstantial_aware(mon, obj, TRUE)) {
			    tmp = 0;
			    Strcpy(unconventional, cxname(obj));
			} else {
			    switch(obj->otyp) {
					case BOULDER:		/* 1d20 */
					case STATUE:
					case FOSSIL:
						if(!is_boulder(obj)) goto defaultvalue;
					case HEAVY_IRON_BALL:	/* 1d25 */
					case IRON_CHAIN:		/* 1d4+1 */
						tmp = dmgval(obj, mon, 0);
						if(obj && ((is_lightsaber(obj) && litsaber(obj)) || arti_shining(obj))) phasearmor = TRUE;
					break;
					case MIRROR:
						if (breaktest(obj)) {
							if(u.specialSealsActive&SEAL_NUDZIRATH){
								You("break %s mirror.  You feel a deep satisfaction!",
									shk_your(yourbuf, obj));
								change_luck(+2);
							} else {
								You("break %s mirror.  That's bad luck!",
									shk_your(yourbuf, obj));
								change_luck(-2);
							}
							useup(obj);
							obj = (struct obj *) 0;
							unarmed = FALSE;	/* avoid obj==0 confusion */
							get_dmg_bonus = FALSE;
							hittxt = TRUE;
						}
						tmp = 1;
					break;
#ifdef TOURIST
						case EXPENSIVE_CAMERA:
							You("succeed in destroying %s camera.  Congratulations!",
								shk_your(yourbuf, obj));
							useup(obj);
							return(TRUE);
							/*NOTREACHED*/
						break;
#endif
						case CORPSE:		/* fixed by polder@cs.vu.nl */
							if (touch_petrifies(&mons[obj->corpsenm])) {
								static const char withwhat[] = "corpse";
								tmp = 1;
								hittxt = TRUE;
								You("hit %s with %s %s.", mon_nam(mon),
									obj->dknown ? the(mons[obj->corpsenm].mname) :
									an(mons[obj->corpsenm].mname),
									(obj->quan > 1) ? makeplural(withwhat) : withwhat);
								if (!resists_ston(mon) && !munstone(mon, TRUE))
									minstapetrify(mon, TRUE);
								if (resists_ston(mon)) break;
								/* note: hp may be <= 0 even if munstoned==TRUE */
								return (boolean) (mon->mhp > 0);
#if 0
					} else if (touch_petrifies(mdat)) {
						/* maybe turn the corpse into a statue? */
#endif
							}
							tmp = (obj->corpsenm >= LOW_PM ?
								mons[obj->corpsenm].msize : 0) + 1;
						break;
						case EGG:
						  {
#define useup_eggs(o)	{ if (thrown) obfree(o,(struct obj *)0); \
				  else useupall(o); \
				  o = (struct obj *)0; }	/* now gone */
						long cnt = obj->quan;
			
						tmp = 1;		/* nominal physical damage */
						get_dmg_bonus = FALSE;
						hittxt = TRUE;		/* message always given */
						/* egg is always either used up or transformed, so next
						   hand-to-hand attack should yield a "bashing" mesg */
						if (obj == uwep) unweapon = TRUE;
						if (obj->spe && obj->corpsenm >= LOW_PM) {
							if (obj->quan < 5)
							change_luck((schar) -(obj->quan));
							else
							change_luck(-5);
						}

						if (touch_petrifies(&mons[obj->corpsenm])) {
							/*learn_egg_type(obj->corpsenm);*/
							pline("Splat! You hit %s with %s %s egg%s!",
								mon_nam(mon),
								obj->known ? "the" : cnt > 1L ? "some" : "a",
								obj->known ? mons[obj->corpsenm].mname : "petrifying",
								plur(cnt));
							obj->known = 1;	/* (not much point...) */
							useup_eggs(obj);
							if (!resists_ston(mon) && !munstone(mon, TRUE))
								minstapetrify(mon, TRUE);
							if (resists_ston(mon)) break;
							return (boolean) (mon->mhp > 0);
						} else {	/* ordinary egg(s) */
							const char *eggp =
								 (obj->corpsenm != NON_PM && obj->known) ?
									  the(mons[obj->corpsenm].mname) :
									  (cnt > 1L) ? "some" : "an";
							You("hit %s with %s egg%s.",
								mon_nam(mon), eggp, plur(cnt));
							if (touch_petrifies(mdat) && !stale_egg(obj)) {
								pline_The("egg%s %s alive any more...",
								  plur(cnt),
								  (cnt == 1L) ? "isn't" : "aren't");
								if (obj->timed) obj_stop_timers(obj);
								obj->otyp = ROCK;
								obj->oclass = GEM_CLASS;
								obj->oartifact = 0;
								obj->spe = 0;
								obj->known = obj->dknown = obj->bknown = 0;
								obj->owt = weight(obj);
								if (thrown) place_object(obj, mon->mx, mon->my);
							} else {
								pline("Splat!");
								useup_eggs(obj);
								exercise(A_WIS, FALSE);
							}
						}
					break;
#undef useup_eggs
					  }
					case CLOVE_OF_GARLIC:	/* no effect against demons */
						if (is_undead_mon(mon)) {
							monflee(mon, d(2, 4), FALSE, TRUE);
						}
						tmp = 1;
					break;
					case CREAM_PIE:
					case BLINDING_VENOM:
						mon->msleeping = 0;
						if (can_blnd(&youmonst, mon, (uchar)
								(obj->otyp == BLINDING_VENOM
								 ? AT_SPIT : AT_WEAP), obj)) {
							if (Blind) {
							pline(obj->otyp == CREAM_PIE ?
								  "Splat!" : "Splash!");
							} else if (obj->otyp == BLINDING_VENOM) {
							pline_The("venom blinds %s%s!", mon_nam(mon),
								  mon->mcansee ? "" : " further");
							} else {
							char *whom = mon_nam(mon);
							char *what = The(xname(obj));
							if (!thrown && obj->quan > 1)
								what = An(singular(obj, xname));
							/* note: s_suffix returns a modifiable buffer */
							if (haseyes(mdat)
								&& mdat != &mons[PM_FLOATING_EYE])
								whom = strcat(strcat(s_suffix(whom), " "),
									  mbodypart(mon, FACE));
							pline("%s %s over %s!",
								  what, vtense(what, "splash"), whom);
							}
							setmangry(mon);
							mon->mcansee = 0;
							tmp = rn1(25, 21);
							if(((int) mon->mblinded + tmp) > 127)
							mon->mblinded = 127;
							else mon->mblinded += tmp;
						} else {
							pline(obj->otyp==CREAM_PIE ? "Splat!" : "Splash!");
							setmangry(mon);
						}
						if (thrown) obfree(obj, (struct obj *)0);
						else useup(obj);
						obj = (struct obj *) 0;
						hittxt = TRUE;
						get_dmg_bonus = FALSE;
						tmp = 0;
					break;
					case ACID_VENOM: /* thrown (or spit) */
						if (resists_acid(mon)) {
							Your("venom hits %s harmlessly.",
								mon_nam(mon));
							tmp = 0;
						} else {
							Your("venom burns %s!", mon_nam(mon));
							tmp = dmgval(obj, mon, 0);
							if(obj && ((is_lightsaber(obj) && litsaber(obj)) || arti_shining(obj))) phasearmor = TRUE;
						}
						if (thrown) obfree(obj, (struct obj *)0);
						else useup(obj);
						obj = (struct obj *) 0;
						hittxt = TRUE;
						get_dmg_bonus = FALSE;
					break;
					default:
defaultvalue:
					/* non-weapons can damage because of their weight */
					/* (but not too much) */
					if(obj->otyp == STILETTOS){
						tmp = rnd(bigmonst(mon->data) ? 2 : 6) + obj->spe + dbon(obj);
						if(u.twoweap) tmp += rnd(bigmonst(mon->data) ? 2 : 6) + obj->spe;
					}
					else {
						tmp = obj->owt/100;
						if(tmp < 1) tmp = 1;
						else tmp = rnd(tmp);
						if(tmp > 6) tmp = 6;
					}

					/*
					 * Paimon's spellbook-bashing damage
					 */
					if (obj->oclass == SPBOOK_CLASS && u.sealsActive&SEAL_PAIMON)
						tmp = rnd(spiritDsize()) + objects[obj->otyp].oc_level;

					/*
					 * Things like silver wands can arrive here so
					 * so we need another silver check.
					 */
					if (obj && (obj->obj_material == SILVER || arti_silvered(obj) || 
							(thrown && obj->otyp == SHURIKEN && uwep && uwep->oartifact == ART_SILVER_STARLIGHT) )
								&& hates_silver(mdat)
					) {
						tmp += rnd(20);
						if(obj->oartifact == ART_SUNSWORD) sunmsg = TRUE;
						else silvermsg = TRUE;
						silverobj = TRUE;
					}
					if(obj->oartifact == ART_GLAMDRING && (is_orc(mdat) || is_demon(mdat))){
						tmp += rnd(20); //I think this is the right thing to do here.  I don't think it enters the main silver section
						lightmsg = TRUE;
					}
					if (obj && obj->obj_material == IRON && hates_iron(mdat)) {
						tmp += rnd(mon->m_lev);
						ironmsg = TRUE;
						ironobj = TRUE;
					}
					if (obj && is_unholy(obj) && hates_unholy(mdat)) {
						tmp += rnd(9);
						unholymsg = TRUE;
						unholyobj = TRUE;
					}
				}
			}
			if(obj){/*may have broken*/
				int basedamage = tmp;
				int newdamage = tmp;
				if (obj->oartifact) {
					hittxt = artifact_hit(&youmonst, mon, obj, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
					newdamage = basedamage;
				}
				if(obj->oproperties){
					hittxt |= oproperty_hit(&youmonst, mon, obj, &newdamage, dieroll);
					if(mon->mhp <= 0 || migrating_mons == mon) /* artifact killed or levelported monster */
						return FALSE;
					if (newdamage == 0) return TRUE;
					tmp += (newdamage - basedamage);
				}
				if(u.sealsActive&SEAL_PAIMON && mon && !DEADMONSTER(mon) && 
					!resists_drli(mon) && obj->oclass == SPBOOK_CLASS && 
					!uwep->oartifact && uwep->otyp != SPE_BLANK_PAPER && 
					uwep->otyp != SPE_SECRETS && !rn2(10)
				){
					int dmg = rnd(8);
					pline("%s seems weaker.",Monnam(mon));
					mon->mhpmax -= dmg;
					tmp += dmg;
					mon->m_lev--;
					if(obj->spestudied>0) obj->spestudied--;
					if(mon->mhpmax < 1) mon->mhpmax = 1;
				}
			}
	    }
	}
	
	if(mon->mstdy){
		tmp += mon->mstdy;
		if(mon->mstdy > 0) mon->mstdy -= 1;
		else mon->mstdy += 1;
	}
	if(mon->ustdym){
		tmp += rnd(mon->ustdym);
	}
	
	if(resist_attacks(mdat)){
		tmp = 0;
		valid_weapon_attack = 0;
	}
	
	if(tmp > 0){
		tmp += u.uencouraged;
		if(uwep && uwep->oartifact == ART_SINGING_SWORD){
			if(uwep->osinging == OSING_LIFE){
				tmp += uwep->spe+1;
			}
		}
		if(tmp < 0){
			tmp = 0;
			valid_weapon_attack = 0;
		}
	}
	/****** NOTE: perhaps obj is undefined!! (if !thrown && BOOMERANG)
	 *      *OR* if attacking bare-handed!! */

	if (get_dmg_bonus && tmp > 0) {
		if(obj && obj->oartifact == ART_TENTACLE_ROD) tmp += dbon(uwep)/2;
		else {
			tmp += u.udaminc;
			/* If you throw using a propellor, you don't get a strength
			 * bonus but you do get an increase-damage bonus.
			 */
			if(!thrown) tmp += dbon(obj);
			else{ //thrown
				if(!uwep || !ammo_and_launcher(obj, uwep))
					tmp += dbon(obj); //thrown by hand, use bonus for thrown item.
				else if(objects[uwep->otyp].oc_skill == P_SLING)
					tmp += dbon(uwep); //thrown by a sling, use weapon bonus.
				//else no bonus
			}
		}
	}

	if (valid_weapon_attack) {
	    struct obj *wep;

	    wep = (obj && ammo_and_launcher(obj, uwep)) ? uwep : obj;
	    if((thrown && objects[wep->otyp].oc_skill != P_LANCE && objects[wep->otyp].oc_skill != P_POLEARMS)
			|| (wep && wep->oartifact == ART_TENTACLE_ROD)
		){
			if((objects[wep->otyp].oc_skill == P_CROSSBOW ||
				wep->otyp == SNIPER_RIFLE
			  ) && !(noncorporeal(mdat) || amorphous(mdat) || ((stationary(mdat) || sessile(mdat)) && (mdat->mlet == S_FUNGUS || mdat->mlet == S_PLANT)))
			){
				int dambonus = weapon_dam_bonus(wep);
				int i=max(P_SKILL(objects[wep->otyp].oc_skill)-2,0); //Expert = 2
				if(Race_if(PM_GNOME)) i++;
				if(dambonus > 0) dambonus *= 3;
				tmp += dambonus;
				for(;i>0;i--){
					// pline("%d",i);
					tmp += dmgval(obj, mon, 0);
					if(obj && ((is_lightsaber(obj) && litsaber(obj)) || arti_shining(obj))) phasearmor = TRUE;
					if(wep->oartifact == ART_LIECLEAVER) tmp += rnd(10);
				}
			}
			//else tmp += 0;
		} else tmp += weapon_dam_bonus(wep);
		if(wep && wep->oartifact == ART_WRATHFUL_SPIDER) tmp = tmp/2+1;
	    /* [this assumes that `!thrown' implies wielded...] */
	    wtype = thrown ? weapon_type(wep) : uwep_skill_type();
	    use_skill(wtype, 1);
		if(!thrown && uwep && is_lightsaber(uwep) && litsaber(uwep) && P_SKILL(wtype) >= P_BASIC){
			use_skill(FFORM_SHII_CHO,1);
			if(P_SKILL(FFORM_SHII_CHO) >= P_BASIC || uwep->oartifact == ART_INFINITY_S_MIRRORED_ARC){
				if((u.fightingForm == FFORM_SHII_CHO || 
					 (u.fightingForm == FFORM_MAKASHI && (!uarm || is_light_armor(uarm) || is_medium_armor(uarm)))
					) &&
					!uarms && !u.twoweap && wtype == P_SABER
				) use_skill(FFORM_MAKASHI,1);
				if((u.fightingForm == FFORM_SHII_CHO || 
					 (u.fightingForm == FFORM_ATARU && (!uarm || is_light_armor(uarm)))
					) &&
					u.lastmoved + 1 >= monstermoves
				) use_skill(FFORM_ATARU,1);
				if((u.fightingForm == FFORM_SHII_CHO || 
					 (u.fightingForm == FFORM_DJEM_SO && (!uarm || is_light_armor(uarm) || is_medium_armor(uarm)))
					) &&
					mon->mattackedu
				) use_skill(FFORM_DJEM_SO,1);
				if((u.fightingForm == FFORM_SHII_CHO || 
					 (u.fightingForm == FFORM_NIMAN && (!uarm || !is_metallic(uarm)))
					) &&
					u.lastcast >= monstermoves
				) use_skill(FFORM_NIMAN,1);
			}
		}
	}
	if (ispoisoned || (obj && (arti_poisoned(obj) || obj->oartifact == ART_WEBWEAVER_S_CROOK || obj->oartifact == ART_MOONBEAM))) {
		int viperheads;
	    if Role_if(PM_SAMURAI) {
          if(!(uarmh && uarmh->oartifact && uarmh->oartifact == ART_HELM_OF_THE_NINJA)){
			You("dishonorably use a poisoned weapon!");
			adjalign(-sgn(u.ualign.type)*5); //stiffer penalty
			u.ualign.sins++;
			u.hod++;
          } else {
			You("dishonorably use a poisoned weapon!");
			adjalign(5);
          }
	    } else if ((u.ualign.type == A_LAWFUL) && !Race_if(PM_ORC) &&
				!((Race_if(PM_DROW) && !flags.initgend && 
					(Role_if(PM_PRIEST) || Role_if(PM_ROGUE) || Role_if(PM_RANGER) || Role_if(PM_WIZARD)) ) ||
				  ((Race_if(PM_HUMAN) || Race_if(PM_INCANTIFIER) || Race_if(PM_HALF_DRAGON)) && (Pantheon_if(PM_RANGER) || Role_if(PM_RANGER)))
				 ) && 
			(u.ualign.record > -10)
		) {
			You_feel("like an evil coward for using a poisoned weapon.");
			adjalign(-2);//stiffer penalty
			if(rn2(2)) u.hod++;
	    }
		for(viperheads = ((obj && obj->otyp == VIPERWHIP) ? (1+obj->ostriking) : 1); viperheads; viperheads--){
			if(ispoisoned & OPOISON_BASIC || (obj && arti_poisoned(obj))){
				if (resists_poison(mon))
					needpoismsg = TRUE;
				else if (rn2(10))
					tmp += rnd(6);
				else poiskilled = TRUE;
			}
			if(ispoisoned & OPOISON_FILTH || (obj && obj->oartifact == ART_SUNBEAM)){
				if (resists_sickness(mon))
					needfilthmsg = TRUE;
				else if (rn2(10))
					tmp += rnd(12);
				else filthkilled = TRUE;
			}
			if(ispoisoned & OPOISON_SLEEP || (obj && (obj->oartifact == ART_WEBWEAVER_S_CROOK || obj->oartifact == ART_MOONBEAM))){
				if (resists_sleep(mon))
					needdrugmsg = TRUE;
				else if(((obj && obj->oartifact == ART_MOONBEAM) || !rn2(5)) && 
					sleep_monst(mon, rnd(12), POTION_CLASS)) druggedmon = TRUE;
			}
			if(ispoisoned & OPOISON_BLIND || (obj && obj->oartifact == ART_WEBWEAVER_S_CROOK)){
				if (resists_poison(mon))
					needpoismsg = TRUE;
				else if (rn2(10))
					tmp += rnd(3);
				 else {
					tmp += 3;
					poisblindmon = TRUE;
				}
			}
			if(ispoisoned & OPOISON_PARAL || (obj && obj->oartifact == ART_WEBWEAVER_S_CROOK)){
				if (rn2(8))
					tmp += rnd(6);
				else {
					tmp += 6;
					if (mon->mcanmove) {
						mon->mcanmove = 0;
						mon->mfrozen = rnd(25);
					}
				}
			}
			if(ispoisoned & OPOISON_AMNES){
				if(mindless_mon(mon)) needsamnesiamsg = TRUE;
				else if(!rn2(10)) amnesiamon = TRUE;
			}
			if(ispoisoned & OPOISON_ACID){
				if (resists_acid(mon))
					needacidmsg = TRUE;
				else tmp += rnd(10);
			}
			
			if (obj && !rn2(20) && ispoisoned) {
				if(obj->quan > 1){
					struct obj *unpoisd = splitobj(obj, 1L);
					unpoisd->opoisoned = 0;
					pline("The coating on your %s has worn off.", xname(unpoisd));
					obj_extract_self(unpoisd);	/* free from inv */
					/* shouldn't merge */
					unpoisd = hold_another_object(unpoisd, "You drop %s!",
								  doname(unpoisd), (const char *)0);
				} else {
					if(obj->otyp == VIPERWHIP && obj->opoisonchrgs){
						obj->opoisonchrgs--;
						pline("Poison from the internal reservoir coats the fangs of your %s.", xname(obj));
					} else {
						obj->opoisoned = 0;
						pline("The coating on your %s has worn off.", xname(obj));
					}
				}
			}
		}
		if(uclockwork && u.utemp >= BURNING_HOT && obj && is_metallic(obj) && !resists_fire(mon)){
			int heatdie = min(u.utemp, 20);
			tmp += rnd(heatdie)/2;
			if(u.utemp >= MELTING){
				tmp += rnd(heatdie)/2;
				if(u.utemp >= MELTED) tmp += rnd(heatdie)/2;
			}
		}
	}
	if (tmp < 1) {
	    /* make sure that negative damage adjustment can't result
	       in inadvertently boosting the victim's hit points */
	    tmp = 0;
	    if (insubstantial(mdat)) {
			if (!hittxt) {
				const char *what = unconventional[0] ? unconventional : "attack";
				Your("%s %s harmlessly through %s.",
					what, vtense(what, "pass"),
				mon_nam(mon));
				hittxt = TRUE;
			}
	    } else {
			if (get_dmg_bonus) tmp = 1;
	    }
	}

	if (unarmed && !thrown && !obj && !Upolyd && !(u.sealsActive&SEAL_EURYNOME)) {//Eurynome makes your attacks monster attacks!
		int resistmask = 0;
		int weaponmask = 0;
		static int warnedotyp = 0;
		static struct permonst *warnedptr = 0;
		if(uarmg && (uarmg->oartifact == ART_GREAT_CLAWS_OF_URDLEN || uarmg->oartifact == ART_SHIELD_OF_THE_RESOLUTE_HEA || uarmg->oartifact == ART_PREMIUM_HEART)){
			//Digging claws, or heart-shaped bit
			weaponmask |= PIERCE;
		}
		if(uarmg && (uarmg->oartifact == ART_GREAT_CLAWS_OF_URDLEN || uarmg->oartifact == ART_CLAWS_OF_THE_REVENANCER)){
			weaponmask |= SLASH;
		} else if(!Upolyd && Race_if(PM_HALF_DRAGON)){
			weaponmask |= SLASH;
		}
		//Can always whack someone
		weaponmask |= WHACK;
		
		if(resist_blunt(mdat) || (mon->mfaction == ZOMBIFIED)){
			resistmask |= WHACK;
		}
		if(resist_pierce(mdat) || (mon->mfaction == ZOMBIFIED || mon->mfaction == SKELIFIED || mon->mfaction == CRYSTALFIED)){
			resistmask |= PIERCE;
		}
		if(resist_slash(mdat) || (mon->mfaction == SKELIFIED || mon->mfaction == CRYSTALFIED)){
			resistmask |= SLASH;
		}
		
		if((weaponmask & ~(resistmask)) == 0L && !(uarmg && narrow_spec_applies(uarmg, mon))){
			tmp /= 4;
			if(warnedptr != mdat){
				Your("%s are ineffective against %s.", makeplural(body_part(HAND)), mon_nam(mon));
				warnedptr = mdat;
			}
		} else {
			warnedptr = 0;
		}
	}
	
#ifdef STEED
	if (jousting) {
	    tmp += d(2, (obj == uwep) ? 10 : 2);	/* [was in dmgval()] */
	    You("joust %s%s",
			 mon_nam(mon), canseemon(mon) ? exclam(tmp) : ".");
	    if (jousting < 0) {
		Your("%s shatters on impact!", xname(obj));
		/* (must be either primary or secondary weapon to get here) */
		u.twoweap = FALSE;	/* untwoweapon() is too verbose here */
		if (obj == uwep) uwepgone();		/* set unweapon */
		/* minor side-effect: broken lance won't split puddings */
		useup(obj);
		obj = 0;
	    }
	    /* avoid migrating a dead monster */
	    if (mon->mhp > tmp) {
		mhurtle(mon, u.dx, u.dy, 1);
		mdat = mon->data; /* in case of a polymorph trap */
		if (DEADMONSTER(mon)) already_killed = TRUE;
	    }
	    hittxt = TRUE;
	} else
#endif
	// if(obj && (obj->oeroded || obj->oeroded2)){
	// if(obj && is_bludgeon(obj) && (obj->oeroded || obj->oeroded2)){
		// int breakmod = is_wood(obj) ? 1 : 0;
		// boolean breakwep = FALSE;
		// switch(greatest_erosion(obj)+breakmod){
			// case 1:
				// //Will not break
			// break;
			// case 2:
				// if(!rn2(1000)) breakwep = TRUE;
			// break;
			// case 3:
				// if(!rn2(100)) breakwep = TRUE;
			// break;
			// case 4: //Ie, an errode 3 wooded weapon
				// if(!rn2(10)) breakwep = TRUE;
			// break;
		// }
		// if(breakwep){
			// boolean more_than_1 = (obj->quan > 1L);
			
			// pline("As you hit %s, %s%s %s breaks and is ruined!",
				  // mon_nam(mon), more_than_1 ? "one of " : "",
				  // shk_your(yourbuf, obj), xname(obj));
			// if (!more_than_1) uwepgone();	/* set unweapon */
			// useup(obj);
			// if (!more_than_1) obj = (struct obj *) 0;
			// hittxt = TRUE;
		// }
	// } else if (unarmed && tmp > 1 && !thrown && !obj && !Upolyd) { 	/* VERY small chance of stunning opponent if unarmed. */
	if (unarmed && tmp > 1 && !thrown && !obj && !Upolyd) { 	/* VERY small chance of stunning opponent if unarmed. */
	    if (!bigmonst(mdat) && !thick_skinned(mdat)) {
			if((uarmg && uarmg->oartifact == ART_PREMIUM_HEART && rnd(20) < P_SKILL(P_BARE_HANDED_COMBAT)) || 
				rnd(100) < P_SKILL(P_BARE_HANDED_COMBAT)){
				if (canspotmon(mon))
					pline("%s %s from your powerful strike!", Monnam(mon),
					  makeplural(stagger(mon->data, "stagger")));
				/* avoid migrating a dead monster */
				if (mon->mhp > tmp) {
					mhurtle(mon, u.dx, u.dy, 1);
					mon->mstun = TRUE;
					mdat = mon->data; /* in case of a polymorph trap */
					if (DEADMONSTER(mon)) already_killed = TRUE;
				}
				hittxt = TRUE;
			}
	    }
	}
	
	/*Now apply damage*/
	// pline("Damage: %d",tmp);
	
	if(tmp){
		if(phasearmor){
			tmp -= base_mdr(mon);
		} else {
			tmp -= roll_mdr(mon, &youmonst);
		}
		if(tmp < 1) tmp = 1;
	}
	
	if (!already_killed){
		mon->mhp -= tmp;
		if(tmp > 0) mon->uhurtm = TRUE;
	}
	/* adjustments might have made tmp become less than what
	   a level draining artifact has already done to max HP */
	if (mon->mhp > mon->mhpmax) mon->mhp = mon->mhpmax;
	if (mon->mhp < 1)
		destroyed = TRUE;
	if (mon->mtame && (!mon->mflee || mon->mfleetim) && tmp > 0) {
		abuse_dog(mon);
		monflee(mon, 10 * rnd(tmp), FALSE, FALSE);
	}
	if((mdat == &mons[PM_BLACK_PUDDING] || mdat == &mons[PM_BROWN_PUDDING] 
		|| mdat == &mons[PM_DARKNESS_GIVEN_HUNGER])
		   && obj && obj == uwep
		   && obj->obj_material == IRON
		   && mon->mhp > 1 && !thrown && !mon->mcan
		   /* && !destroyed  -- guaranteed by mhp > 1 */ ) {
		if (clone_mon(mon, 0, 0)) {
			pline("%s divides as you hit it!", Monnam(mon));
			hittxt = TRUE;
		}
	}

	if (!hittxt &&			/*( thrown => obj exists )*/
	  (!destroyed || (thrown && m_shot.n > 1 && m_shot.o == obj->otyp))) {
		if (thrown) hit(mshot_xname(obj), mon, exclam(tmp));
		else if (!flags.verbose) You("hit it.");
		else You("%s %s%s", Role_if(PM_BARBARIAN) ? "smite" : "hit",
			 mon_nam(mon), canseemon(mon) ? exclam(tmp) : ".");
	}
	
	if (mdat == &mons[PM_URANIUM_IMP] && !mon->mcan){
	    if (mon->mhp <= 0) {
		killed(mon);
		return FALSE;
	    } else {
		if (canseemon(mon)) pline("%s %s reality!", Monnam(mon),
			level.flags.noteleport ? "tries to warp" : "warps");
		if (!level.flags.noteleport) {
		    tele(); coord cc;
		    enexto(&cc, u.ux, u.uy, &mons[PM_URANIUM_IMP]);
		    rloc_to(mon, cc.x, cc.y);
		}
	    }
	    return TRUE;
	}
	if(lightmsg){
		const char *fmt;
		char *whom = mon_nam(mon);
		char silverobjbuf[BUFSZ];

		if (canspotmon(mon)) {
			fmt = "The white-burning blade sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), " flesh");
		pline(fmt, whom);
	}
	if (silvermsg) {
		const char *fmt;
		char *whom = mon_nam(mon);
		char silverobjbuf[BUFSZ];
		int justeden = 0;

		if (canspotmon(mon)) {
		    if (barehand_silver_rings == 1)
			fmt = "Your %ssilver ring sears %s!";
		    else if (barehand_silver_rings == 2)
			fmt = "Your %ssilver rings sear %s!";
		    else if (barehand_jade_rings == 1 && barehand_silver_rings == 1)
			fmt = "Your %ssilver and jade rings sear %s!";
		    else if (barehand_jade_rings == 2)
			fmt = "Your %sjade rings sear %s!";
		    else if (barehand_jade_rings == 1)
			fmt = "Your %sjade ring sears %s!";
		    else if (silverobj && saved_oname[0]) {
		    	Sprintf(silverobjbuf, "Your %%s%s%s %s %%s!",
		    		strstri(saved_oname, "silver") ?
					"" : "silver ",
				saved_oname, vtense(saved_oname, "sear"));
		    	fmt = silverobjbuf;
		    } else if(eden_silver){
				fmt = "Your silver skin sears %s";
				justeden = 1;
			} else fmt = "The %ssilver sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), " flesh");
		if(canspotmon(mon)){
			if(justeden) pline(fmt, whom);
			else pline(fmt, (eden_silver) ? "silver skin and " : "", whom);
		} else pline(fmt, whom);
	}
	if (ironmsg) {
		const char *fmt;
		char *whom = mon_nam(mon);
		char ironobjbuf[BUFSZ];

		if (canspotmon(mon)) {
		    if (barehand_iron_rings == 1)
			fmt = "Your %scold-iron ring sears %s!";
		    else if (barehand_iron_rings == 2)
			fmt = "Your %scold-iron rings sear %s!";
		    else if (ironobj && saved_oname[0]) {
		    	Sprintf(ironobjbuf, "Your %%s%s%s %s %%s!",
		    		strstri(saved_oname, "iron") ?
					"" : "cold-iron ",
				saved_oname, vtense(saved_oname, "sear"));
		    	fmt = ironobjbuf;
		    }
			else fmt = "The %siron sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), " flesh");
		if(canspotmon(mon)) 
			pline(fmt, "", whom);
		else pline(fmt, whom);
	}
	if (unholymsg) {
		const char *fmt;
		char *whom = mon_nam(mon);
		char unholyobjbuf[BUFSZ];

		if (canspotmon(mon)) {
		    if (barehand_unholy_rings == 1)
			fmt = "Your %sunholy ring sears %s!";
		    else if (barehand_unholy_rings == 2)
			fmt = "Your %sunholy rings sear %s!";
		    else if (unholyobj && saved_oname[0]) {
		    	Sprintf(unholyobjbuf, "Your %%s%s%s %s %%s!",
		    		strstri(saved_oname, "cursed") ?
					"" : "unholy ",
				saved_oname, vtense(saved_oname, "sear"));
		    	fmt = unholyobjbuf;
		    } else fmt = "The %scurse sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), " flesh");
		if(canspotmon(mon)) 
			pline(fmt, "", whom);
		else pline(fmt, whom);
	}

	if (sunmsg) {
		const char *fmt;
		char *whom = mon_nam(mon);
		char silverobjbuf[BUFSZ];

		if (canspotmon(mon)) {
			if (silverobj && saved_oname[0]) {
		    	Sprintf(silverobjbuf, "Your %s %s %%s!",
					saved_oname, vtense(saved_oname, "sear"));
		    	fmt = silverobjbuf;
		    } else
			fmt = "The brilliant blade sears %s!";
		} else {
		    *whom = highc(*whom);	/* "it" -> "It" */
		    fmt = "%s is seared!";
		}
		/* note: s_suffix returns a modifiable buffer */
		if (!noncorporeal(mdat))
		    whom = strcat(s_suffix(whom), " flesh");
		pline(fmt, whom);
	}

	if (needpoismsg)
		pline_The("poison doesn't seem to affect %s.", mon_nam(mon));
	if (needfilthmsg)
		pline_The("filth doesn't seem to affect %s.", mon_nam(mon));
	if (needdrugmsg)
		pline_The("drug doesn't seem to affect %s.", mon_nam(mon));
	if (needsamnesiamsg)
		pline_The("lethe-rust doesn't seem to affect %s.", mon_nam(mon));
	if (needacidmsg)
		pline_The("acid-coating doesn't seem to affect %s.", mon_nam(mon));
	if (druggedmon){
		pline("%s falls asleep.", Monnam(mon));
		slept_monst(mon);
	}
	if (poisblindmon){
		if(haseyes(mon->data)) {
			if (canseemon(mon)) pline("It seems %s has gone blind!", mon_nam(mon));
			register int btmp = 64 + rn2(32) +
			rn2(32) * !resist(mon, POTION_CLASS, 0, NOTELL);
			btmp += mon->mblinded;
			mon->mblinded = min(btmp,127);
			mon->mcansee = 0;
		}
	}
	if (amnesiamon){
		if (canseemon(mon)) pline("%s looks around as if awakening from a dream.",
			   Monnam(mon));
		mon->mtame = FALSE;
		mon->mpeaceful = TRUE;
	}
	if (poiskilled) {
		pline_The("poison was deadly...");
		if (!already_killed) xkilled(mon, 0);
		return FALSE;
	} else if (filthkilled) {
		pline_The("tainted filth was deadly...");
		if (!already_killed) xkilled(mon, 0);
		return FALSE;
	} else if (vapekilled) {
		if (cansee(mon->mx, mon->my))
			pline("%s%ss body vaporizes!", Monnam(mon),
				canseemon(mon) ? "'" : "");                
		if (!already_killed) xkilled(mon, 2);
		return FALSE;
	} else if (destroyed) {
		if (!already_killed)
		    killed(mon);	/* takes care of most messages */
	} else if(u.umconf) {
		nohandglow(mon);
		if (!mon->mconf && !resist(mon, SPBOOK_CLASS, 0, NOTELL)) {
			mon->mconf = 1;
			if (!mon->mstun && mon->mcanmove && !mon->msleeping &&
				canseemon(mon))
			    pline("%s appears confused.", Monnam(mon));
		}
	}

	return((boolean)(destroyed ? FALSE : TRUE));
}

boolean
insubstantial_aware(mon, obj, you)
struct monst *mon;
struct obj *obj;
int you;
{
	struct permonst *ptr = mon->data;
	if(you && u.sealsActive&SEAL_CHUPOCLOPS)
		return TRUE;
	if (!obj){
		if(you && u.sealsActive&SEAL_EDEN && hates_silver(ptr))
			return TRUE;
		return FALSE;
	}
	
	if(is_lightsaber(obj) && litsaber(obj))
		return TRUE;
	if(hates_silver(ptr) && (obj->obj_material == SILVER || arti_silvered(obj)
		|| obj->otyp == MIRROR
		|| (obj->otyp == SHURIKEN && uwep && uwep->oartifact == ART_SILVER_STARLIGHT)
	))
		return TRUE;
	if(hates_iron(ptr) && obj->obj_material == IRON)
		return TRUE;
	if(hates_unholy(ptr) && is_unholy(obj))
		return TRUE;
	if(hates_holy_mon(mon) && obj->blessed)
		return TRUE;
	if(arti_shining(obj))
		return TRUE;
	if (obj->otyp == CLOVE_OF_GARLIC)	/* causes shades to flee */
		return TRUE;
	return FALSE;
}

int
insubstantial_damage(mon, obj, dmg, you)
struct monst *mon;
struct obj *obj;
int dmg;
int you;
{
	struct permonst *ptr = mon->data;
	if(you && u.sealsActive&SEAL_CHUPOCLOPS)
		return dmg;
	if (!obj){
		if(you && u.sealsActive&SEAL_EDEN && hates_silver(ptr))
			return rnd(20);
		return 0;
	}
	
	if(is_lightsaber(obj) && litsaber(obj))
		return dmg;
	if(hates_silver(ptr) && (obj->obj_material == SILVER || arti_silvered(obj)
		|| obj->otyp == MIRROR
		|| (obj->otyp == SHURIKEN && uwep && uwep->oartifact == ART_SILVER_STARLIGHT)
	))
		return rnd(20);
	if(hates_iron(ptr) && obj->obj_material == IRON)
		return rnd(mon->m_lev);
	if(hates_unholy(ptr) && is_unholy(obj))
		return rnd(9);
	if(hates_holy_mon(mon) && obj->blessed)
		return rnd(4);
	if(arti_shining(obj))
		return dmg;
	if (obj->otyp == CLOVE_OF_GARLIC)	/* causes shades to flee */
		return rnd(2);
	return 0;
}

/* check whether slippery clothing protects from hug or wrap attack */
/* [currently assumes that you are the attacker] */
STATIC_OVL boolean
m_slips_free(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *obj;

	if (mattk->adtyp == AD_DRIN) {
	    /* intelligence drain attacks the head */
	    obj = which_armor(mdef, W_ARMH);
	} else {
	    /* grabbing attacks the body */
	    obj = which_armor(mdef, W_ARMC);		/* cloak */
	    if (!obj) obj = which_armor(mdef, W_ARM);	/* suit */
#ifdef TOURIST
	    if (!obj) obj = which_armor(mdef, W_ARMU);	/* shirt */
#endif
	}

	/* if your cloak/armor is greased, monster slips off; this
	   protection might fail (33% chance) when the armor is cursed */
	if (obj && (obj->greased || obj->otyp == OILSKIN_CLOAK) &&
		(!obj->cursed || rn2(3))) {
	    You("%s %s %s %s!",
		mattk->adtyp == AD_WRAP ?
			"slip off of" : "grab, but cannot hold onto",
		s_suffix(mon_nam(mdef)),
		obj->greased ? "greased" : "slippery",
		/* avoid "slippery slippery cloak"
		   for undiscovered oilskin cloak */
		(obj->greased || objects[obj->otyp].oc_name_known) ?
			xname(obj) : cloak_simple_name(obj));

	    if (obj->greased && !rn2(2)) {
		pline_The("grease wears off.");
		obj->greased = 0;
	    }
	    return TRUE;
	}
	return FALSE;
}

/* used when hitting a monster with a lance while mounted */
STATIC_OVL int	/* 1: joust hit; 0: ordinary hit; -1: joust but break lance */
joust(mon, obj)
struct monst *mon;	/* target */
struct obj *obj;	/* weapon */
{
    int skill_rating, joust_dieroll;

    if (Fumbling || Stunned) return 0;
    /* sanity check; lance must be wielded in order to joust */
    if (obj != uwep && (obj != uswapwep || !u.twoweap)) return 0;

    /* if using two weapons, use worse of lance and two-weapon skills */
    skill_rating = P_SKILL(weapon_type(obj));	/* lance skill */
    if (u.twoweap && P_SKILL(P_TWO_WEAPON_COMBAT) < skill_rating)
	skill_rating = P_SKILL(P_TWO_WEAPON_COMBAT);
    if (skill_rating == P_ISRESTRICTED) skill_rating = P_UNSKILLED; /* 0=>1 */

    /* odds to joust are expert:80%, skilled:60%, basic:40%, unskilled:20% */
    if ((joust_dieroll = rn2(5)) < skill_rating) {
		if (!unsolid(mon->data) && !obj_resists(obj, 0, 100)){
			if(obj->otyp == DROVEN_LANCE && rnl(40) == (40-1)) return -1;	/* hit that breaks lance */
			else if(joust_dieroll == 0){ /* Droven lances are especially brittle */
				if(obj->otyp == ELVEN_LANCE && rnl(75) == (75-1)) return -1;	/* hit that breaks lance */
				else if(rnl(50) == (50-1)) return -1;	/* hit that breaks lance */
			}
		}
		return 1;	/* successful joust */
    }
    return 0;	/* no joust bonus; revert to ordinary attack */
}

/*
 * Send in a demon pet for the hero.  Exercise wisdom.
 *
 * This function used to be inline to damageum(), but the Metrowerks compiler
 * (DR4 and DR4.5) screws up with an internal error 5 "Expression Too Complex."
 * Pulling it out makes it work.
 */
void
demonpet()
{
	int i;
	struct permonst *pm;
	struct monst *dtmp;

	pline("Some hell-p has arrived!");
	i = (!is_demon(youracedata) || !rn2(6)) 
	     ? ndemon(u.ualign.type) : NON_PM;
	pm = i != NON_PM ? &mons[i] : youracedata;
	if(pm == &mons[PM_ANCIENT_OF_ICE] || pm == &mons[PM_ANCIENT_OF_DEATH]) {
	    pm = rn2(4) ? &mons[PM_METAMORPHOSED_NUPPERIBO] : &mons[PM_ANCIENT_NUPPERIBO];
	}
	if ((dtmp = makemon(pm, u.ux, u.uy, NO_MM_FLAGS)) != 0)
	    (void)tamedog(dtmp, (struct obj *)0);
	exercise(A_WIS, TRUE);
}

/*
 * Player uses theft attack against monster.
 *
 * If the target is wearing body armor, take all of its possesions;
 * otherwise, take one object.  [Is this really the behavior we want?]
 *
 * This routine implicitly assumes that there is no way to be able to
 * resist petfication (ie, be polymorphed into a xorn or golem) at the
 * same time as being able to steal (poly'd into nymph or succubus).
 * If that ever changes, the check for touching a cockatrice corpse
 * will need to be smarter about whether to break out of the theft loop.
 */
STATIC_OVL void
steal_it(mdef, mattk)
struct monst *mdef;
struct attack *mattk;
{
	struct obj *otmp, *stealoid, **minvent_ptr;
	long unwornmask;
	int petrifies = FALSE;
	char kbuf[BUFSZ];

	if (!mdef->minvent) return;		/* nothing to take */

	/* look for worn body armor */
	stealoid = (struct obj *)0;
	if (could_seduce(&youmonst, mdef, mattk)) {
	    /* find armor, and move it to end of inventory in the process */
	    minvent_ptr = &mdef->minvent;
	    while ((otmp = *minvent_ptr) != 0)
		if (otmp->owornmask & W_ARM) {
		    if (stealoid) panic("steal_it: multiple worn suits");
		    *minvent_ptr = otmp->nobj;	/* take armor out of minvent */
		    stealoid = otmp;
		    stealoid->nobj = (struct obj *)0;
		} else {
		    minvent_ptr = &otmp->nobj;
		}
	    *minvent_ptr = stealoid;	/* put armor back into minvent */
	}

	if (stealoid) {		/* we will be taking everything */
	    if (gender(mdef) == (int) u.mfemale &&
			youracedata->mlet == S_NYMPH)
		You("charm %s.  She gladly hands over her possessions.",
		    mon_nam(mdef));
	    else
		You("seduce %s and %s starts to take off %s clothes.",
		    mon_nam(mdef), mhe(mdef), mhis(mdef));
	}

	while ((otmp = mdef->minvent) != 0) {
	    /* take the object away from the monster */
	    obj_extract_self(otmp);
	    if ((unwornmask = otmp->owornmask) != 0L) {
		mdef->misc_worn_check &= ~unwornmask;
		if (otmp->owornmask & W_WEP) {
		    setmnotwielded(mdef,otmp);
		    MON_NOWEP(mdef);
		}
		if (otmp->owornmask & W_SWAPWEP){
			setmnotwielded(mdef,otmp);
			MON_NOSWEP(mdef);
		}
		otmp->owornmask = 0L;
		update_mon_intrinsics(mdef, otmp, FALSE, FALSE);

		if (otmp == stealoid)	/* special message for final item */
		    pline("%s finishes taking off %s suit.",
			  Monnam(mdef), mhis(mdef));
	    }
		
		if(near_capacity() < calc_capacity(otmp->owt)){
			You("steal %s %s and drop it to the %s.",
				  s_suffix(mon_nam(mdef)), xname(otmp), surface(u.ux, u.uy));
			if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
				Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
				petrifies = TRUE;
			}
			dropy(otmp);
		} else {
	    /* give the object to the character */
			if(otmp->otyp == CORPSE && touch_petrifies(&mons[otmp->corpsenm]) && !uarmg && !Stone_resistance){
				Sprintf(kbuf, "stolen %s corpse", mons[otmp->corpsenm].mname);
				petrifies = TRUE;
			}
			otmp = hold_another_object(otmp, "You snatched but dropped %s.",
						   doname(otmp), "You steal: ");
		}
	    /* more take-away handling, after theft message */
	    if (unwornmask & W_WEP) {		/* stole wielded weapon */
		possibly_unwield(mdef, FALSE);
	    } else if (unwornmask & W_ARMG) {	/* stole worn gloves */
		mselftouch(mdef, (const char *)0, TRUE);
		if (mdef->mhp <= 0)	/* it's now a statue */
		    return;		/* can't continue stealing */
	    }
	    if (petrifies) {
			instapetrify(kbuf);
			break;		/* stop the theft even if hero survives */
	    }

	    if (!stealoid) break;	/* only taking one item */
	}
}

int
damageum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register struct permonst *pd = mdef->data;
	register int	tmp;
	int armpro;
	boolean negated, phasearmor = FALSE;
	boolean weaponhit = (mattk->aatyp == AT_WEAP || mattk->aatyp == AT_XWEP || mattk->aatyp == AT_DEVA);
	struct attack alt_attk;

	armpro = magic_negation(mdef);
	/* since hero can't be cancelled, only defender's armor applies */
	negated = !((rn2(3) >= armpro) || !rn2(50));
	
	if(weaponhit && mattk->adtyp != AD_PHYS) tmp = 0;
	else tmp = d((int)mattk->damn, (int)mattk->damd);
	
	tmp += dbon((struct obj *)0);
	
	if (is_demon(youracedata) && !rn2(13) && !uwep
		&& u.umonnum != PM_SUCCUBUS && u.umonnum != PM_INCUBUS
		&& u.umonnum != PM_BALROG) {
	    demonpet();
	    return(0);
	}
	switch (weaponhit ? AD_PHYS : mattk->adtyp) {
	    case AD_STUN:
		if(!Blind)
		    pline("%s %s for a moment.", Monnam(mdef),
			  makeplural(stagger(mdef->data, "stagger")));
		mdef->mstun = 1;
		goto physical;
	    case AD_LEGS:
	     /* if (u.ucancelled) { */
	     /*    tmp = 0;	    */
	     /*    break;	    */
	     /* }		    */
		goto physical;
	    case AD_WERE:	    /* no special effect on monsters */
	    case AD_HEAL:	    /* likewise */
	    case AD_PHYS:
 physical:
		if(weaponhit) {
		    if(uwep) tmp = 0;
			// tack on bonus elemental damage, if applicable
			if (mattk->adtyp != AD_PHYS){
				alt_attk.aatyp = AT_NONE;
				if(mattk->adtyp == AD_OONA)
					alt_attk.adtyp = u.oonaenergy;
				else if(mattk->adtyp == AD_HDRG)
					alt_attk.adtyp = AD_COLD; //Go with the classic
				else if(mattk->adtyp == AD_RBRE){
					switch(rn2(3)){
						case 0:
							alt_attk.adtyp = AD_FIRE;
						break;
						case 1:
							alt_attk.adtyp = AD_COLD;
						break;
						case 2:
							alt_attk.adtyp = AD_ELEC;
						break;
					}
				} else alt_attk.adtyp = mattk->adtyp;
				alt_attk.damn = mattk->damn;
				alt_attk.damd = mattk->damd;
				// switch (alt_attk.adtyp)
				// {
				// case AD_FIRE:
				// case AD_COLD:
				// case AD_ELEC:
				// case AD_ACID:
					// alt_attk.damn = 4;
					// alt_attk.damd = 6;
					// break;
				// case AD_EFIR:
				// case AD_ECLD:
				// case AD_EELC:
				// case AD_EACD:
					// alt_attk.damn = 3;
					// alt_attk.damd = 7;
					// break;
				// case AD_STUN:
					// alt_attk.damn = 1;
					// alt_attk.damd = 4;
					// break;
				// default:
					// alt_attk.damn = 0;
					// alt_attk.damd = 0;
					// break;
				// }
				damageum(mdef, &alt_attk);
				if (DEADMONSTER(mdef))
					return 2;
			}
		} else if(mattk->aatyp == AT_KICK) {
		    if(thick_skinned(mdef->data)) tmp = 0;
		    if(insubstantial(mdef->data)) {
			    tmp = insubstantial_damage(mdef, uarmf, tmp, TRUE); /* bless damage */
		    }
		}
		break;
	    case AD_FIRE:
		if (negated) {
		    tmp = 0;
		    break;
		}
		if (resists_fire(mdef)) {
		    if (!Blind)
			pline_The("fire doesn't heat %s!", mon_nam(mdef));
		    golemeffects(mdef, AD_FIRE, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
		    break;
		}
		if (!Blind)
		    pline("%s is %s!", Monnam(mdef),
			  on_fire(mdef->data, mattk));
		if (pd == &mons[PM_STRAW_GOLEM] ||
		    pd == &mons[PM_PAPER_GOLEM] ||
		    pd == &mons[PM_SPELL_GOLEM]) {
		    if (!Blind)
			pline("%s burns completely!", Monnam(mdef));
		    xkilled(mdef,2);
		    tmp = 0;
		    break;
		    /* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		} else if (pd == &mons[PM_MIGO_WORKER]) {
		    if (!Blind)
			pline("%s's brain melts!", Monnam(mdef));
		    xkilled(mdef,2);
		    tmp = 0;
		    break;
		    /* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		}
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_EFIR:
		if (resists_fire(mdef)) {
		    golemeffects(mdef, AD_EFIR, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp /= 2;
		    break;
		}
		if (!Blind)
		    pline("%s is %s!", Monnam(mdef),
			  on_fire(mdef->data, mattk));
		if (pd == &mons[PM_STRAW_GOLEM] ||
		    pd == &mons[PM_PAPER_GOLEM] ||
		    pd == &mons[PM_SPELL_GOLEM]) {
		    if (!Blind)
			pline("%s burns completely!", Monnam(mdef));
		    xkilled(mdef,2);
		    tmp = 0;
		    break;
		    /* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		} else if (pd == &mons[PM_MIGO_WORKER]) {
		    if (!Blind)
			pline("%s's brain melts!", Monnam(mdef));
		    xkilled(mdef,2);
		    tmp = 0;
		    break;
		    /* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		}
		tmp += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
		tmp += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
		/* only potions damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_COLD:
		if (negated) {
		    tmp = 0;
		    break;
		}
		if (resists_cold(mdef)) {
		    shieldeff(mdef->mx, mdef->my);
		    if (!Blind)
			pline_The("frost doesn't chill %s!", mon_nam(mdef));
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp = 0;
			break;
		}
		if (!Blind) pline("%s is covered in frost!", Monnam(mdef));
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_ECLD:
		if (resists_cold(mdef)) {
		    shieldeff(mdef->mx, mdef->my);
		    golemeffects(mdef, AD_COLD, tmp);
		    tmp /= 2;
			break;
		}
		if (!Blind) pline("%s is covered in frost!", Monnam(mdef));
		tmp += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_ELEC:
		if (negated) {
		    tmp = 0;
		    break;
		}
		if (resists_elec(mdef)) {
		    if (!Blind)
			pline_The("zap doesn't shock %s!", mon_nam(mdef));
		    golemeffects(mdef, AD_ELEC, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp = 0;
			break;
		}
		if (!Blind) pline("%s is zapped!", Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_EELC:
		if (resists_elec(mdef)) {
		    if (!Blind)
			pline_The("zap doesn't shock %s!", mon_nam(mdef));
		    golemeffects(mdef, AD_EELC, tmp);
		    shieldeff(mdef->mx, mdef->my);
		    tmp /= 2;
			break;
		}
		if (!Blind) pline("%s is zapped!", Monnam(mdef));
		tmp += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);

		/* only rings damage resistant players in destroy_item */
		tmp += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_ACID:
		if (resists_acid(mdef)) tmp = 0;
		else {
			if (!rn2(10)) erode_armor(mdef, TRUE);
		}
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_EACD:
		if (resists_acid(mdef)) tmp /= 2;
		else {
			erode_armor(mdef, TRUE);
		}
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_STON:
		if (!resists_ston(mdef) && !munstone(mdef, TRUE))
		    minstapetrify(mdef, TRUE);
		tmp = 0;
		break;
///////////////////////////////////////////////////////////////////////////////////////////
#ifdef SEDUCE
	    case AD_SSEX:
#endif
	    case AD_SEDU:
	    case AD_SITM:
		steal_it(mdef, mattk);
		tmp = 0;
		break;
///////////////////////////////////////////////////////////////////////////////////////////
	    case AD_SGLD:
#ifndef GOLDOBJ
		if (mdef->mgold) {
		    u.ugold += mdef->mgold;
		    mdef->mgold = 0;
		    Your("purse feels heavier.");
		}
#else
                /* This you as a leprechaun, so steal
                   real gold only, no lesser coins */
	        {
		    struct obj *mongold = findgold(mdef->minvent);
	            if (mongold) {
		        obj_extract_self(mongold);  
		        if (merge_choice(invent, mongold) || inv_cnt() < 52) {
			    addinv(mongold);
			    Your("purse feels heavier.");
			} else {
                            You("grab %s's gold, but find no room in your knapsack.", mon_nam(mdef));
			    dropy(mongold);
		        }
		    }
	        }
#endif
		exercise(A_DEX, TRUE);
		tmp = 0;
		break;
	    case AD_TLPT:
		if (tmp <= 0) tmp = 1;
		if (!negated && tmp < mdef->mhp) {
		    char nambuf[BUFSZ];
		    boolean u_saw_mon = canseemon(mdef) ||
					(u.uswallow && u.ustuck == mdef);
		    /* record the name before losing sight of monster */
		    Strcpy(nambuf, Monnam(mdef));
		    if (u_teleport_mon(mdef, FALSE) &&
			    u_saw_mon && !canseemon(mdef))
			pline("%s suddenly disappears!", nambuf);
		}
		break;
	    case AD_BLND:
		if (can_blnd(&youmonst, mdef, mattk->aatyp, (struct obj*)0)) {
		    if(!Blind && mdef->mcansee)
			pline("%s is blinded.", Monnam(mdef));
		    mdef->mcansee = 0;
		    tmp += mdef->mblinded;
		    if (tmp > 127) tmp = 127;
		    mdef->mblinded = tmp;
		}
		tmp = 0;
		break;
	    case AD_CURS:
		if (night() && !rn2(10) && !mdef->mcan) {
		    if (mdef->data == &mons[PM_CLAY_GOLEM] || mdef->data == &mons[PM_SPELL_GOLEM]) {
			if (!Blind)
			    pline("Some writing vanishes from %s head!",
				s_suffix(mon_nam(mdef)));
			xkilled(mdef, 0);
			/* Don't return yet; keep hp<1 and tmp=0 for pet msg */
		    } else {
			mdef->mcan = 1;
			You("chuckle.");
		    }
		}
		tmp = 0;
		break;
	    case AD_VAMP:
	    case AD_DRLI:
		if(has_blood_mon(mdef) && 
			youracedata == &mons[PM_BLOOD_BLOATER]
		){
			if(Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax){
				You("bloat yourself with blood.");
				healup(tmp, 0, FALSE, FALSE);
			}
		}
		if (!negated && !rn2(3) && !resists_drli(mdef)
			&& (youracedata != &mons[PM_VAMPIRE_BAT] || mdef->msleeping)
		) {
			int xtmp = d(2,6);
			if (mdef->mhp < xtmp) xtmp = mdef->mhp;
			/* Player vampires are smart enough not to feed while
			   biting if they might have trouble getting it down */
			if (!Race_if(PM_INCANTIFIER) && is_vampire(youracedata)
				&& u.uhunger <= 1420 &&
			    mattk->aatyp == AT_BITE && has_blood_mon(mdef)) {
				/* For the life of a creature is in the blood
				   (Lev 17:11) */
				if (flags.verbose)
				    You("feed on the lifeblood.");
				/* [ALI] Biting monsters does not count against
				   eating conducts. The draining of life is
				   considered to be primarily a non-physical
				   effect */
				lesshungry(xtmp * 6);
			}
			if(has_blood_mon(mdef) && 
				youracedata == &mons[PM_BLOOD_BLOATER]
			){
				if(Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax){
					(void)split_mon(&youmonst, 0);
				}
			}
			pline("%s suddenly seems weaker!", Monnam(mdef));
			mdef->mhpmax -= xtmp;
			if(xtmp > 0) mdef->uhurtm = TRUE;
			if ((mdef->mhp -= xtmp) <= 0 || !mdef->m_lev) {
				pline("%s dies!", Monnam(mdef));
				xkilled(mdef,0);
			} else
				mdef->m_lev--;
			tmp = 0;
		}
		break;
	    case AD_RUST:
		if (pd == &mons[PM_IRON_GOLEM] || pd == &mons[PM_CHAIN_GOLEM]) {
			pline("%s falls to pieces!", Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(mdef, AD_RUST);
		tmp = 0;
		break;
	    case AD_CORR:
		hurtmarmor(mdef, AD_CORR);
		tmp = 0;
		break;
	    case AD_DCAY:
		if (pd == &mons[PM_WOOD_GOLEM] ||
			pd == &mons[PM_GROVE_GUARDIAN] ||
			pd == &mons[PM_LIVING_LECTERN] ||
		    pd == &mons[PM_LEATHER_GOLEM]) {
			pline("%s falls to pieces!", Monnam(mdef));
			xkilled(mdef,0);
		}
		hurtmarmor(mdef, AD_DCAY);
		tmp = 0;
		break;
	    case AD_STAR:
			if(hates_silver(pd)){
				tmp += rnd(20);
            	pline("The rapier of silver light sears %s!", mon_nam(mdef));
			}
			tmp += dtypbon(RAPIER);
		break;
	    case AD_BLUD:
			if(has_blood_mon(mdef) || (has_blood(pd) && mdef->mfaction == ZOMBIFIED)) {
            	tmp += mdef->m_lev;
            	pline("The blade of rotted blood tears through the veins of %s!", mon_nam(mdef));
            }
		break;
	    case AD_SHDW:
			// if(u.specialSealsActive&SEAL_BLACK_WEB) tmp = d(rnd(8),spiritDsize()+1);
			// else tmp = d(rnd(8),rnd(5)+1);
			tmp += dbon((struct obj *)0);
	    case AD_DRST:
	    case AD_DRDX:
	    case AD_DRCO:
		if (!negated && !rn2(8)) {
		    Your("%s was poisoned!", mpoisons_subj(&youmonst, mattk));
		    if (resists_poison(mdef))
			pline_The("poison doesn't seem to affect %s.",
				mon_nam(mdef));
		    else {
			if (!rn2(10)) {
			    Your("poison was deadly...");
			    tmp = mdef->mhp;
			} else tmp += rn1(10,6);
		    }
		}
		break;
	    case AD_EDRC:
		    Your("%s was poisoned!", mpoisons_subj(&youmonst, mattk));
		    if (resists_poison(mdef))
			pline_The("poison doesn't seem to affect %s.",
				mon_nam(mdef));
		    else {
			if (!rn2(10)) {
			    Your("poison was deadly...");
			    tmp = mdef->mhp;
			} else tmp += rn1(10,6);
		    }
		break;
		case AD_NPDC:
			// if(!cancelled) tmp += rnd(10);
			tmp += rnd(10);
		break;
	    case AD_DRIN:
		if (notonhead || !has_head(mdef->data)) {
		    pline("%s doesn't seem harmed.", Monnam(mdef));
		    tmp = 0;
		    if (!Unchanging && mdef->data == &mons[PM_GREEN_SLIME] && mdef->data == &mons[PM_FLUX_SLIME]) {
			if (!Slimed) {
			    You("suck in some slime and don't feel very well.");
			    Slimed = 10L;
			}
		    }
		    break;
		}
		if (m_slips_free(mdef, mattk)) break;

		if ((mdef->misc_worn_check & W_ARMH) && rn2(8)) {
		    pline("%s helmet blocks your attack to %s head.",
			  s_suffix(Monnam(mdef)), mhis(mdef));
		    break;
		}

		You("eat %s brain!", s_suffix(mon_nam(mdef)));
		u.uconduct.food++;
		if (touch_petrifies(mdef->data) && !Stone_resistance && !Stoned) {
		    Stoned = 5;
		    killer_format = KILLED_BY_AN;
		    delayed_killer = mdef->data->mname;
		}
		if (!vegan(mdef->data))
		    u.uconduct.unvegan++;
		if (!vegetarian(mdef->data))
		    violated_vegetarian();
		if (mindless_mon(mdef)) {
		    pline("%s doesn't notice.", Monnam(mdef));
		    break;
		}
		tmp += d(tmp-dbon((struct obj *)0),10); /*subtract out the damage bonus.  NOTE: I'm allowing this for incants*/
		morehungry(-rnd((tmp-dbon((struct obj *)0))*6)); /* cannot choke; aprox 30 points of hunger per point of 'int' drained */
		if (ABASE(A_INT) < AMAX(A_INT)) {
			ABASE(A_INT) += rnd(4);
			if (ABASE(A_INT) > AMAX(A_INT))
				ABASE(A_INT) = AMAX(A_INT);
			flags.botl = 1;
		}
		exercise(A_WIS, TRUE);
		exercise(A_INT, TRUE);
		break;
	    case AD_STCK:
		if (!negated && !sticks(mdef->data)) {
			if(mdef->data == &mons[PM_TOVE]) pline("It is too slithy to get stuck!");
			else u.ustuck = mdef; /* it's now stuck to you */
		}
		break;
	    case AD_WRAP:
		if (!sticks(mdef->data)) {
		    if (!u.ustuck && !rn2(10)) {
			if (m_slips_free(mdef, mattk)) {
			    tmp = 0;
			} else {
			    You("swing yourself around %s!",
				  mon_nam(mdef));
				if(mdef->data == &mons[PM_TOVE]) pline("Unfortunately, it is much too slithy to grab!");
				else u.ustuck = mdef; /* it's now stuck to you */
			}
		    } else if(u.ustuck == mdef) {
			/* Monsters don't wear amulets of magical breathing */
			if (is_pool(u.ux,u.uy, FALSE) && !is_swimmer(mdef->data) &&
			    !amphibious_mon(mdef)) {
			    You("drown %s...", mon_nam(mdef));
			    tmp = mdef->mhp;
			} else if(mattk->aatyp == AT_HUGS)
			    pline("%s is being crushed.", Monnam(mdef));
		    } else {
			tmp = 0;
			if (flags.verbose)
			    You("brush against %s %s.",
				s_suffix(mon_nam(mdef)),
				mbodypart(mdef, LEG));
		    }
		} else tmp = 0;
		break;
	    case AD_PLYS:
		if (!negated && mdef->mcanmove && !rn2(3) && tmp < mdef->mhp) {
		    if (!Blind) pline("%s is frozen by you!", Monnam(mdef));
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_TCKL:
		if (mdef->mcanmove && !rn2(3) && tmp < mdef->mhp) {
		    if (!Blind) You("mercilessly tickle %s!", mon_nam(mdef));
		    mdef->mcanmove = 0;
		    mdef->mfrozen = rnd(10);
		}
		break;
	    case AD_SLEE:
		if (!negated && !mdef->msleeping &&
			    sleep_monst(mdef, rnd(10), -1)) {
		    if (!Blind)
			pline("%s is put to sleep by you!", Monnam(mdef));
		    slept_monst(mdef);
		}
		break;
	    case AD_SLIM:
		if (negated) break;	/* physical damage only */
		{
		struct obj *armor = which_armor(mdef, W_ARM);
		struct obj *shield = which_armor(mdef, W_ARMS);
		if (!rn2(4) && !flaming(mdef->data)
				&& mdef->data != &mons[PM_RED_DRAGON]
				&& !(
					(armor && (armor->otyp == RED_DRAGON_SCALES || armor->otyp == RED_DRAGON_SCALE_MAIL)
					) || (shield && shield->otyp == RED_DRAGON_SCALE_SHIELD
					)
				) &&
				mdef->data != &mons[PM_GREEN_SLIME] && mdef->data != &mons[PM_FLUX_SLIME] && 
				!is_rider(mdef->data) && !resists_poly(mdef->data)
		) {
		    You("turn %s into slime.", mon_nam(mdef));
		    (void) newcham(mdef, &mons[PM_GREEN_SLIME], FALSE, FALSE);
		    tmp = 0;
		}
		}
		break;
	    case AD_WEBS:{
			struct trap *ttmp2 = maketrap(mdef->mx, mdef->my, WEB);
			if (ttmp2) mintrap(mdef);
		}break;
	    case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    if (!negated){
		    struct obj *obj = some_armor(mdef);

		    if (drain_item(obj)) {
				pline("%s's %s less effective.", Monnam(mdef), aobjnam(obj, "seem"));
		    }
		}
		break;
///////////////////////////////////////////////////////////////////////////////////////////
		case AD_WET:
			water_damage(mdef->minvent, FALSE, FALSE, FALSE, mdef);
		break;
	    case AD_SLOW:
		if (!negated && mdef->mspeed != MSLOW) {
		    unsigned int oldspeed = mdef->mspeed;

		    mon_adjust_speed(mdef, -1, (struct obj *)0);
		    if (mdef->mspeed != oldspeed && canseemon(mdef))
				pline("%s slows down.", Monnam(mdef));
		}
		break;
	    case AD_CONF:
		if (!mdef->mconf) {
		    if (canseemon(mdef))
			pline("%s looks confused.", Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
		case AD_IRIS:
			u.irisAttack = moves;
			if(!rn2(20)){
				int tmp2;
				if(nonliving(mdef->data) || is_anhydrous(mdef->data)){
					shieldeff(mdef->mx, mdef->my);
					break;
				}
				pline("%s shrivels at the touch of your iridescent tentacles!", Monnam(mdef));
				tmp2 = tmp+d(5,spiritDsize());
				tmp = min(tmp2,mdef->mhp);
				healup(tmp, 0, FALSE, FALSE);
			}
		break;
		case AD_NABERIUS:
			if(mdef->mflee){
				if(Upolyd ? u.mh < u.mhmax : u.uhp < u.uhpmax) Your("crimson fangs run red with blood.");
				healup(rnd(8), 0, FALSE, FALSE);
			}
			if(mdef->mpeaceful){
				Your("tarnished fangs %s in the %s.", levl[u.ux][u.uy].lit ? "shine": "glimmer", levl[u.ux][u.uy].lit ? "light" : "dark");
				mdef->mconf=1;
				mdef->mstun=1;
			}
		break;
		case AD_OTIAX:
		u.otiaxAttack = moves;
		if(!rn2(10)){
			struct obj *otmp2, **minvent_ptr;
			long unwornmask;

			/* Don't steal worn items, and downweight wielded items */
			if((otmp2 = mdef->minvent) != 0) {
				while(otmp2 && 
					  otmp2->owornmask&W_ARMOR && 
					  !( (otmp2->owornmask&W_WEP) && !rn2(10))
				) otmp2 = otmp2->nobj;
			}
			/* Still has handling for worn items, incase that changes */
			if(otmp2 != 0){
				int dx,dy;
				/* take the object away from the monster */
				if(otmp2->quan > 1L){
					otmp2 = splitobj(otmp2, 1L);
					obj_extract_self(otmp2); //wornmask is cleared by splitobj
				} else{
					obj_extract_self(otmp2);
					if ((unwornmask = otmp2->owornmask) != 0L) {
						mdef->misc_worn_check &= ~unwornmask;
						if (otmp2->owornmask & W_WEP) {
							setmnotwielded(mdef,otmp2);
							MON_NOWEP(mdef);
						}
						if (otmp2->owornmask & W_SWAPWEP){
							setmnotwielded(mdef,otmp2);
							MON_NOSWEP(mdef);
						}
						otmp2->owornmask = 0L;
						update_mon_intrinsics(mdef, otmp2, FALSE, FALSE);
					}
				}
				Your("mist tendrils free %s.",doname(otmp2));
				mdrop_obj(mdef,otmp2,FALSE);
				/* more take-away handling, after theft message */
				if (unwornmask & W_WEP) {		/* stole wielded weapon */
					possibly_unwield(mdef, FALSE);
				} else if (unwornmask & W_ARMG) {	/* stole worn gloves */
					mselftouch(mdef, (const char *)0, TRUE);
					if (mdef->mhp <= 0)	/* it's now a statue */
						return 1; /* monster is dead */
				}
			}
		}break;
		case AD_SIMURGH:
			if(hates_iron(mdef->data)){
				Your("claws of cold iron sear %s.",mon_nam(mdef));
				tmp+=d(2, mdef->m_lev);
			}
			pline("Radiant feathers slice through %s.",mon_nam(mdef));
			switch(rn2(15)){
				case 0:
					if(!resists_fire(mdef)){
						tmp+= rnd(spiritDsize());
						if(resists_cold(mdef)) tmp += rnd(spiritDsize());
					} else shieldeff(mdef->mx, mdef->my);
				break;
				case 1:
					if(!resists_cold(mdef)){
						tmp+= rnd(spiritDsize());
						if(resists_fire(mdef)) tmp += rnd(spiritDsize());
					} else shieldeff(mdef->mx, mdef->my);
				break;
				case 2:
					if(!resists_disint(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 3:
					if(pm_invisible(mdef->data)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 4:
					if(!resists_elec(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 5:
					if(!resists_poison(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 6:
					if(!resists_acid(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 7:
					if(!resists_ston(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 8:
					if(!resists_drain(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 9:
					if(!resists_sickness(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 10:
					if(is_undead_mon(mdef)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 11:
					if(is_fungus(mdef->data)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 12:
					if(infravision(mdef->data)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 13:
					if(opaque(mdef->data)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
				case 14:
					if(emits_light(mdef->data)) tmp+= rnd(spiritDsize());
					else shieldeff(mdef->mx, mdef->my);
				break;
			}
			if(!is_blind(mdef) && haseyes(mdef->data)){
				mdef->mstun = 1;
			}
		break;
		case AD_FRWK:{
			int x,y,i;
			for(i = rn2(3)+2; i > 0; i--){
				x = rn2(3)-1;
				y = rn2(3)-1;
				explode(u.ux+x, u.uy+y, AD_PHYS, -1, tmp, rn2(7), 1);		//-1 is unspecified source. 8 is physical
			}
			tmp=0;
		} break;
/*		case AD_VMSL:	//vorlon missile.  triple damage
			rehumanize();
			explode(u.ux, u.uy, 5, tmp, -1, EXPL_WET);		//-1 is unspecified source. 5 is electrical
			explode(u.ux, u.uy, 4, tmp, -1, EXPL_MAGICAL);	//-1 is unspecified source. 4 is disintegration
			explode(u.ux, u.uy, 8, tmp, -1, EXPL_DARK);		//-1 is unspecified source. 8 is physical
			tmp=0;		//damage was done by explode
		break;
*/
	    default:	tmp = 0;
		break;
	}

   if (mdef->data == &mons[PM_GIANT_TURTLE] && mdef->mflee) tmp = tmp/2;
	mdef->mstrategy &= ~STRAT_WAITFORU; /* in case player is very fast */
	
	if(mdef->mstdy){
		tmp += mdef->mstdy;
		if(mdef->mstdy > 0) mdef->mstdy -= 1;
		else mdef->mstdy += 1;
	}
	if(mdef->ustdym){
		tmp += rnd(mdef->ustdym);
	}
	
	// if(attacktype_fordmg(youracedata, AT_NONE, AD_STAR)){
		// if(otmp && otmp == uwep && !otmp->oartifact && otmp->spe <= 0) tmp = 0;
		// else if(!otmp || otmp != uwep) tmp /= 2;
	// }
	
	if(tmp){
		if(mattk->adtyp != AD_SHDW && mattk->adtyp != AD_STAR && !phasearmor){
			tmp -= roll_mdr(mdef, &youmonst);
		} else {
			tmp -= base_mdr(mdef);
		}
		if(tmp < 1) tmp = 1;
	}
	
	if(tmp > 1){
		if(mattk->adtyp == AD_SHDW){
			use_skill(P_BARE_HANDED_COMBAT,1);
		}
	}
	
	if((mdef->mhp -= tmp) < 1) {
	    if (mdef->mtame && !cansee(mdef->mx,mdef->my)) {
		You_feel("embarrassed for a moment.");
		if (tmp) xkilled(mdef, 0); /* !tmp but hp<1: already killed */
	    } else if (!flags.verbose) {
		You("destroy it!");
		if (tmp) xkilled(mdef, 0);
	    } else
		if (tmp) killed(mdef);
	    return(2);
	}
	return(1);
}

STATIC_OVL int
explum(mdef, mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp = d((int)mattk->damn, (int)mattk->damd);

	You("explode!");
	switch(mattk->adtyp) {
	    boolean resistance; /* only for cold/fire/elec */

	    case AD_BLND:
		if (!resists_blnd(mdef)) {
		    pline("%s is blinded by your flash of light!", Monnam(mdef));
		    mdef->mblinded = min((int)mdef->mblinded + tmp, 127);
		    mdef->mcansee = 0;
		}
		break;
	    case AD_HALU:
		if (haseyes(mdef->data) && mdef->mcansee) {
		    pline("%s is affected by your flash of light!",
			  Monnam(mdef));
		    mdef->mconf = 1;
		}
		break;
	    case AD_COLD:
		resistance = resists_cold(mdef);
		goto common;
	    case AD_FIRE:
		resistance = resists_fire(mdef);
		goto common;
	    case AD_ELEC:
		resistance = resists_elec(mdef);
common:
		if (!resistance) {
		    pline("%s gets blasted!", Monnam(mdef));
		    mdef->mhp -= tmp;
			if(tmp>0) mdef->uhurtm = TRUE;
		    if (mdef->mhp <= 0) {
			 killed(mdef);
			 return(2);
		    }
		} else {
		    shieldeff(mdef->mx, mdef->my);
		    if (is_golem(mdef->data))
			golemeffects(mdef, (int)mattk->adtyp, tmp);
		    else
			pline_The("blast doesn't seem to affect %s.",
				mon_nam(mdef));
		}
		break;
	    case AD_ECLD:
		resistance = resists_cold(mdef);
		goto ecommon;
	    case AD_EFIR:
		resistance = resists_fire(mdef);
		goto ecommon;
	    case AD_EELC:
		resistance = resists_elec(mdef);
ecommon:
		if (resistance) {
		    shieldeff(mdef->mx, mdef->my);
		    if (is_golem(mdef->data))
				golemeffects(mdef, (int)mattk->adtyp, tmp);
			tmp /= 2;
		}
		pline("%s gets blasted!", Monnam(mdef));
		mdef->mhp -= tmp;
		if(tmp>0) mdef->uhurtm = TRUE;
		if (mdef->mhp <= 0) {
		 killed(mdef);
		 return(2);
		}
		break;
	    default:
		break;
	}
	return(1);
}

STATIC_OVL void
start_engulf(mdef)
struct monst *mdef;
{
	if (!Invisible) {
		map_location(u.ux, u.uy, TRUE);
		tmp_at(DISP_ALWAYS, mon_to_glyph(&youmonst));
		tmp_at(mdef->mx, mdef->my);
	}
	You("engulf %s!", mon_nam(mdef));
	delay_output();
	delay_output();
}

STATIC_OVL void
end_engulf()
{
	if (!Invisible) {
		tmp_at(DISP_END, 0);
		newsym(u.ux, u.uy);
	}
}

STATIC_OVL int
gulpum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	register int tmp;
	register int dam = d((int)mattk->damn, (int)mattk->damd);
	struct obj *otmp;
	/* Not totally the same as for real monsters.  Specifically, these
	 * don't take multiple moves.  (It's just too hard, for too little
	 * result, to program monsters which attack from inside you, which
	 * would be necessary if done accurately.)  Instead, we arbitrarily
	 * kill the monster immediately for AD_DGST and we regurgitate them
	 * after exactly 1 round of attack otherwise.  -KAA
	 */

	if(mdef->data->msize >= MZ_HUGE) return 0;

	if(YouHunger < (Race_if(PM_INCANTIFIER) ? u.uenmax*3/4 : 1500) && !u.uswallow) {
	    for (otmp = mdef->minvent; otmp; otmp = otmp->nobj)
		(void) snuff_lit(otmp);

	    if(!touch_petrifies(mdef->data) || Stone_resistance) {
#ifdef LINT	/* static char msgbuf[BUFSZ]; */
		char msgbuf[BUFSZ];
#else
		static char msgbuf[BUFSZ];
#endif
		start_engulf(mdef);
		switch(mattk->adtyp) {
		    case AD_DGST:
			/* eating a Rider or its corpse is fatal */
			if (is_rider(mdef->data)) {
			 pline("Unfortunately, digesting any of it is fatal.");
			    end_engulf();
			    Sprintf(msgbuf, "unwisely tried to eat %s",
				    mdef->data->mname);
			    killer = msgbuf;
			    killer_format = NO_KILLER_PREFIX;
			    done(DIED);
			    return 0;		/* lifesaved */
			}

			if (Slow_digestion) {
			    dam = 0;
			    break;
			}

			/* KMH, conduct */
			u.uconduct.food++;
			if (!vegan(mdef->data))
			     u.uconduct.unvegan++;
			if (!vegetarian(mdef->data))
			     violated_vegetarian();

			/* Use up amulet of life saving */
			if (!!(otmp = mlifesaver(mdef))) m_useup(mdef, otmp);

			newuhs(FALSE);
			xkilled(mdef,2);
			if (mdef->mhp > 0) { /* monster lifesaved */
			    You("hurriedly regurgitate the sizzling in your %s.",
				body_part(STOMACH));
			} else {
			    tmp = 1 + (mdef->data->cwt >> 8);
			    if (corpse_chance(mdef, &youmonst, TRUE) &&
				!(mvitals[monsndx(mdef->data)].mvflags &
				  G_NOCORPSE || mdef->mvanishes)) {
				/* nutrition only if there can be a corpse */
				if(Race_if(PM_INCANTIFIER)) u.uen += mdef->m_lev;
				else u.uhunger += (mdef->data->cnutrit+1) / 2;
			    } else tmp = 0;
			    Sprintf(msgbuf, "You totally digest %s.",
					    mon_nam(mdef));
			    if (tmp != 0) {
				/* setting afternmv = end_engulf is tempting,
				 * but will cause problems if the player is
				 * attacked (which uses his real location) or
				 * if his See_invisible wears off
				 */
				You("digest %s.", mon_nam(mdef));
				if (Slow_digestion) tmp *= 2;
				nomul(-tmp, "digesting a victim");
				nomovemsg = msgbuf;
			    } else pline("%s", msgbuf);
			    if (mdef->data == &mons[PM_GREEN_SLIME] || mdef->data == &mons[PM_FLUX_SLIME]) {
				Sprintf(msgbuf, "%s isn't sitting well with you.",
					The(mdef->data->mname));
				if (!Unchanging) {
					Slimed = 5L;
					flags.botl = 1;
				}
			    } else
			    exercise(A_CON, TRUE);
			}
			end_engulf();
			return(2);
		    case AD_PHYS:
			if (youracedata == &mons[PM_FOG_CLOUD]) {
			    pline("%s is laden with your moisture.",
				  Monnam(mdef));
			    if (amphibious_mon(mdef) &&
				!flaming(mdef->data)) {
				dam = 0;
				pline("%s seems unharmed.", Monnam(mdef));
			    }
			} else
			    pline("%s is pummeled with your debris!",
				  Monnam(mdef));
			break;
		    case AD_ACID:
			pline("%s is covered with your goo!", Monnam(mdef));
			if (resists_acid(mdef)) {
			    pline("It seems harmless to %s.", mon_nam(mdef));
			    dam = 0;
			} else {
				erode_armor(mdef, TRUE);
			}
			break;
		    case AD_EACD:
			pline("%s is covered with your goo!", Monnam(mdef));
			if (resists_acid(mdef)) {
			    dam /= 2;
			} else {
				erode_armor(mdef, TRUE);
			}
			break;
		    case AD_BLND:
			if (can_blnd(&youmonst, mdef, mattk->aatyp, (struct obj *)0)) {
			    if (mdef->mcansee)
				pline("%s can't see in there!", Monnam(mdef));
			    mdef->mcansee = 0;
			    dam += mdef->mblinded;
			    if (dam > 127) dam = 127;
			    mdef->mblinded = dam;
			}
			dam = 0;
			break;
		    case AD_ELEC:
			if (rn2(2)) {
			    pline_The("air around %s crackles with electricity.", mon_nam(mdef));
			    if (resists_elec(mdef)) {
					pline("%s seems unhurt.", Monnam(mdef));
					dam = 0;
			    } else {
					dam += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
					dam += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
			    }
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_EELC:
			    pline_The("air around %s crackles with electricity.", mon_nam(mdef));
			    if (resists_elec(mdef)) {
					dam /= 2;
			    } else {
					dam += destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
					dam += destroy_mitem(mdef, RING_CLASS, AD_ELEC);
				}
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			break;
		    case AD_COLD:
			if (rn2(2)) {
			    if (resists_cold(mdef)) {
					pline("%s seems mildly chilly.", Monnam(mdef));
					dam = 0;
			    } else {
					pline("%s is freezing to death!",Monnam(mdef));
					dam += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
				}
				golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_ECLD:
			    if (resists_cold(mdef)) {
					pline("%s seems mildly chilly.", Monnam(mdef));
					dam /= 2;
			    } else {
					pline("%s is freezing to death!",Monnam(mdef));
					dam += destroy_mitem(mdef, POTION_CLASS, AD_COLD);
				}
				golemeffects(mdef,(int)mattk->adtyp,dam);
			break;
		    case AD_FIRE:
			if (rn2(2)) {
			    if (resists_fire(mdef)) {
					pline("%s seems mildly hot.", Monnam(mdef));
					dam = 0;
			    } else {
					pline("%s is burning to a crisp!",Monnam(mdef));
					dam += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
					dam += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
					dam += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
				}
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			} else dam = 0;
			break;
		    case AD_EFIR:
			    if (resists_fire(mdef)) {
					pline("%s seems mildly hot.", Monnam(mdef));
					dam /= 2;
			    } else {
					pline("%s is burning to a crisp!",Monnam(mdef));
					dam += destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
					dam += destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
					dam += destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
				}
			    golemeffects(mdef,(int)mattk->adtyp,dam);
			break;
		}
		end_engulf();
		if ((mdef->mhp -= dam) <= 0) {
		    killed(mdef);
		    if (mdef->mhp <= 0)	/* not lifesaved */
			return(2);
		}
		if(dam > 0) mdef->uhurtm = TRUE;
		You("%s %s!", is_animal(youracedata) ? "regurgitate"
			: "expel", mon_nam(mdef));
		if (Slow_digestion || is_animal(youracedata)) {
		    pline("Obviously, you didn't like %s taste.",
			  s_suffix(mon_nam(mdef)));
		}
	    } else {
		char kbuf[BUFSZ];

		You("bite into %s.", mon_nam(mdef));
		Sprintf(kbuf, "swallowing %s whole", an(mdef->data->mname));
		instapetrify(kbuf);
	    }
	}
	return(0);
}

void
missum(mdef,mattk)
register struct monst *mdef;
register struct attack *mattk;
{
	if (could_seduce(&youmonst, mdef, mattk))
		You("pretend to be friendly to %s.", mon_nam(mdef));
	else if(canspotmon(mdef) && flags.verbose)
		You("miss %s.", mon_nam(mdef));
	else
		You("miss it.");
	if (!mdef->msleeping && mdef->mcanmove)
		wakeup(mdef, TRUE);
}

boolean
hmonas(mon, mas, tmp, weptmp, tchtmp)		/* attack monster as a monster. */
register struct monst *mon;
register struct permonst *mas;
register int tmp, weptmp, tchtmp;
{
	struct attack *mattk, alt_attk;
	int	i, sum[NATTK], hittmp = 0;
	int	nsum = 0;
	int	dhit = 0;
	boolean Old_Upolyd = Upolyd, wepused;
	
	if(is_displacer(mon->data) && rn2(2)){
		You("attack a displaced image!");
		return TRUE;
	}
	
	for(i = 0; i < NATTK; i++) {
	    sum[i] = 0;
	    mattk = getmattk(mas, i, sum, &alt_attk);
		wepused = FALSE;
		
		if (mas == &mons[PM_GRUE] && (i>=2) && !((!levl[u.ux][u.uy].lit && !(viz_array[u.uy][u.ux] & TEMP_LIT1 && !(viz_array[u.uy][u.ux] & TEMP_DRK1)))
			|| (levl[u.ux][u.uy].lit && (viz_array[u.uy][u.ux] & TEMP_DRK1 && !(viz_array[u.uy][u.ux] & TEMP_LIT1)))))
			continue;
		
		/*Plasteel helms cover the face and prevent bite attacks*/
		if(uarmh && 
			(uarmh->otyp == PLASTEEL_HELM || uarmh->otyp == CRYSTAL_HELM || uarmh->otyp == PONTIFF_S_CROWN) && 
			(mattk->aatyp == AT_BITE || mattk->aatyp == AT_ENGL || mattk->aatyp == AT_LNCK || 
				(mattk->aatyp == AT_TENT && is_mind_flayer((&youmonst)->data)))
		) continue;
		if(uarmc && 
			(uarmc->otyp == WHITE_FACELESS_ROBE
			|| uarmc->otyp == BLACK_FACELESS_ROBE
			|| uarmc->otyp == SMOKY_VIOLET_FACELESS_ROBE) && 
			(mattk->aatyp == AT_BITE || mattk->aatyp == AT_ENGL || mattk->aatyp == AT_LNCK || 
				(mattk->aatyp == AT_TENT && is_mind_flayer((&youmonst)->data)))
		) continue;
		
	    switch(mattk->aatyp) {
		case AT_XWEP:
			/* general-case two-weaponing is covered in AT_WEAP */
			/* special case: marilith? */
			break;	
		case AT_WEAP:
use_weapon:
	/* Certain monsters don't use weapons when encountered as enemies,
	 * but players who polymorph into them have hands or claws and thus
	 * should be able to use weapons.  This shouldn't prohibit the use
	 * of most special abilities, either.
	 */
	/* Potential problem: if the monster gets multiple weapon attacks,
	 * we currently allow the player to get each of these as a weapon
	 * attack.  Is this really desirable?
	 */
			dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
			/* Enemy dead, before any special abilities used */
			if (!known_hitum(mon,&dhit,mattk)) {
			    sum[i] = 2;
			} else {
				sum[i] = dhit;
				/* might be a worm that gets cut in half */
				if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
				/* Do not print "You hit" message, since known_hitum
				 * already did it.
				 */
				if (dhit && mattk->adtyp != AD_SPEL
					&& mattk->adtyp != AD_PHYS)
					sum[i] = damageum(mon,mattk);
			}
			if(u.sealsActive&SEAL_MARIONETTE && mon && uwep && mattk->adtyp == AD_PHYS){
				struct monst *m2 = m_at(mon->mx+u.dx,mon->my+u.dy);
				if(!m2 || DEADMONSTER(m2) || m2==mon) break;
				dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
				/* Enemy dead, before any special abilities used */
				(void) passive(m2, dhit, known_hitum(m2, &dhit, mattk), mattk->aatyp, mattk->adtyp);
				/* might be a worm that gets cut in half */
			}
		break;
		case AT_MARI:{
		mari++;
		dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
		/* Enemy dead, before any special abilities used */
		if (!known_hitum_wepi(mon,&dhit,mattk,mari)) {
			    sum[i] = 2;
		} else {
				sum[i] = dhit;
			/* might be a worm that gets cut in half */
			if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
			/* Do not print "You hit" message, since known_hitum
			 * already did it.
			 */
			if (dhit && mattk->adtyp != AD_SPEL
				&& mattk->adtyp != AD_PHYS)
				sum[i] = damageum(mon,mattk);
		}
		if(u.sealsActive&SEAL_MARIONETTE && uwep && mattk->adtyp == AD_PHYS){
			struct monst *m2 = m_at(mon->mx+u.dx,mon->my+u.dy);
			if(!m2 || DEADMONSTER(m2) || m2==mon) break;
			dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
			/* Enemy dead, before any special abilities used */
			known_hitum_wepi(m2,&dhit,mattk,mari);
			/* might be a worm that gets cut in half */
		}
		}break;
		case AT_CLAW:
		case AT_LRCH: /*Note: long reach attacks are being treated as melee only for polymorph purposes*/
			if (i==0 && uwep && !cantwield(mas)) goto use_weapon;
#ifdef SEDUCE
			/* succubi/incubi are humanoid, but their _second_
			 * attack is AT_CLAW, not their first...
			 */
			if (i==1 && uwep && (u.umonnum == PM_SUCCUBUS ||
				u.umonnum == PM_INCUBUS)) goto use_weapon;
#endif
		case AT_KICK:
		case AT_BITE:
		case AT_LNCK: /*Note: long reach attacks are being treated as melee only for polymorph purposes*/
			/* [ALI] Vampires are also smart. They avoid biting
			   monsters if doing so would be fatal */
			if ((uwep || (u.twoweap && uswapwep) || uarmg) &&
				is_vampire(mas) &&
				(is_rider(mon->data) ||
				 mon->data == &mons[PM_GREEN_SLIME] ||
				 mon->data == &mons[PM_FLUX_SLIME])
			){
			    	break;
			}
		case AT_STNG:
		case AT_TUCH:
		case AT_5SQR:
		case AT_BUTT:
		case AT_TENT:
		case AT_WHIP:
			if (i==0 && uwep && (mas->mlet==S_LICH)) goto use_weapon;
			if ((uwep || (u.twoweap && uswapwep) || uarmg) &&
				mattk->adtyp != AD_STAR && mattk->adtyp != AD_BLUD && mattk->adtyp != AD_SHDW && 
				(touch_petrifies(mon->data) ||
				 mon->data == &mons[PM_MEDUSA])){
			    	break;
			}
			dhit = (((u.sealsActive&SEAL_CHUPOCLOPS || mattk->aatyp) ? tchtmp : tmp) > rnd(20) || u.uswallow);
			goto wisp_shdw_dhit;
		case AT_WISP:
			dhit = (tmp > rnd(20) || u.uswallow);
			goto wisp_shdw_dhit;
		case AT_SHDW:
			dhit = (tchtmp > rnd(20) || u.uswallow);
wisp_shdw_dhit:
			if (dhit != 0) {
			    int compat;

			    if (!u.uswallow &&
				(compat=could_seduce(&youmonst, mon, mattk))) {
				You("%s %s %s.",
				    !is_blind(mon) && haseyes(mon->data)
				    ? "smile at" : "talk to",
				    mon_nam(mon),
				    compat == 2 ? "engagingly":"seductively");
				/* doesn't anger it; no wakeup() */
				sum[i] = damageum(mon, mattk);
				break;
			    }
			    wakeup(mon, TRUE);
			    /* maybe this check should be in damageum()? */
			    if (insubstantial(mon->data) &&
					!(mattk->aatyp == AT_KICK && insubstantial_aware(mon, uarmf, TRUE)) 
						&& !(insubstantial_aware(mon, (struct obj *)0, TRUE))) {
				Your("attack passes harmlessly through %s.",
				    mon_nam(mon));
				break;
			    }
			    if (mattk->aatyp == AT_KICK)
				    You("kick %s.", mon_nam(mon));
			    else if (mattk->aatyp == AT_BITE || mattk->aatyp == AT_LNCK)
				    You("bite %s.", mon_nam(mon));
			    else if (mattk->aatyp == AT_STNG)
				    You("sting %s.", mon_nam(mon));
			    else if (mattk->aatyp == AT_BUTT)
				    You("butt %s.", mon_nam(mon));
			    else if (mattk->aatyp == AT_TUCH || mattk->aatyp == AT_5SQR){
					if(mattk->adtyp == AD_SHDW) You("slash %s with bladed shadows.", mon_nam(mon));
					else if(mattk->adtyp == AD_STAR)  You("slash %s with a starlight rapier.", mon_nam(mon));
					else if(mattk->adtyp == AD_BLUD) You("slash %s with a blade of blood.", mon_nam(mon));
				    else You("touch %s.", mon_nam(mon));
				} else if (mattk->aatyp == AT_TENT)
				    Your("tentacles suck %s.", mon_nam(mon));
			    else if (mattk->aatyp == AT_WHIP)
				    Your("barbed whips lash %s.", mon_nam(mon));
				else if(mattk->adtyp == AT_SHDW) {
					Your("bladed shadow strikes %s.", mon_nam(mon));
				} else if(mattk->adtyp == AT_SHDW) {
					You("slash %s with a starlight rapier.", mon_nam(mon));
				} else if(mattk->aatyp == AT_WISP) 
					Your("mist tendrils lash %s.", mon_nam(mon));
			    else You("hit %s.", mon_nam(mon));
			    sum[i] = damageum(mon, mattk);
			} else
			    missum(mon, mattk);
			break;

		case AT_HUGS:
			/* automatic if prev two attacks succeed, or if
			 * already grabbed in a previous attack
			 */
			dhit = 1;
			wakeup(mon, TRUE);
			if (insubstantial(mon->data) && !insubstantial_aware(mon, (struct obj *)0, TRUE))
			    Your("hug passes harmlessly through %s.",
				mon_nam(mon));
			else if (!sticks(mon->data) && !u.uswallow) {
			    if (mon==u.ustuck) {
				pline("%s is being %s.", Monnam(mon),
				    u.umonnum==PM_ROPE_GOLEM ? "choked":
				    "crushed");
				sum[i] = damageum(mon, mattk);
			    } else if(i >= 2 && sum[i-1] && sum[i-2]) {
				You("grab %s!", mon_nam(mon));
				u.ustuck = mon;
				sum[i] = damageum(mon, mattk);
			    }
			}
			break;

		case AT_EXPL:	/* automatic hit if next to */
			dhit = -1;
			wakeup(mon, TRUE);
			sum[i] = explum(mon, mattk);
			break;

		case AT_ENGL:
			if((dhit = (tmp > rnd(20+i)))) {
				wakeup(mon, TRUE);
				if (insubstantial(mon->data) && !insubstantial_aware(mon, (struct obj *)0, TRUE))
				    Your("attempt to surround %s is harmless.",
					mon_nam(mon));
				else {
				    sum[i]= gulpum(mon,mattk);
				    if (sum[i] == 2 &&
					    (mon->data->mlet == S_ZOMBIE ||
						mon->data->mlet == S_MUMMY) &&
					    rn2(5) &&
					    !Sick_resistance) {
					You_feel("%ssick.",
					    (Sick) ? "very " : "");
					mdamageu(mon, rnd(8));
				    }
				}
			} else
				missum(mon, mattk);
			break;

#ifdef YOUMONST_SPELL
		case AT_MAGC:
			/* No check for uwep; if wielding nothing we want to
			 * do the normal 1-2 points bare hand damage...
			 */
			/*
			if (i==0 && (mas->mlet==S_KOBOLD
				|| mas->mlet==S_ORC
				|| mas->mlet==S_GNOME
				)) goto use_weapon;
			*/
			sum[i] = castum(mon, mattk);
			continue;
#endif /* YOUMONST_SPELL */

		case AT_NONE:
		case AT_BOOM:
			continue;
			/* Not break--avoid passive attacks from enemy */

		case AT_BREA:
		case AT_BEAM:
		case AT_SPIT:
		case AT_GAZE:	/* all done using #monster command */
			dhit = 0;
			break;
		case AT_WDGZ:	/* passive */
			dhit = 0;
			break;

		default: /* Strange... */
			impossible("strange attack of yours (%d)",
				 mattk->aatyp);
	    }
	    if (dhit == -1) {
		u.mh = -1;	/* dead in the current form */
		rehumanize();
	    }
	    if (sum[i] == 2)
		return((boolean)passive(mon, 1, 0, mattk->aatyp, mattk->adtyp));
							/* defender dead */
	    else {
		(void) passive(mon, sum[i], 1, mattk->aatyp, mattk->adtyp);
		if (DEADMONSTER(mon))
			return FALSE;
		nsum |= sum[i];
	    }
	    if (Upolyd != Old_Upolyd)
		break; /* No extra attacks if form changed */
	    if (multi < 0)
		break; /* If paralyzed while attacking, i.e. floating eye */
	}
	return(!DEADMONSTER(mon));
}

boolean
hmonwith(mon, tmp, weptmp, tchtmp, attacklist, nattk)		/* attack monster with monster attacks. */
struct monst *mon;
int tmp, weptmp, tchtmp;
struct attack *attacklist;
int nattk;
{
	int	i, sum[nattk], hittmp = 0;
	int	nsum = 0;
	int	dhit = 0;
	struct attack *mattk;
	boolean Old_Upolyd = Upolyd;
	
	if(is_displacer(mon->data) && rn2(2)){
		You("attack a displaced image!");
		return TRUE;
	}
	
	for(i = 0; i < nattk; i++) {
	mattk = &attacklist[i];
	sum[i] = 0;
	switch(mattk->aatyp) {
	case AT_XWEP:
		/* general-case two-weaponing is covered in AT_WEAP */
		/* special case: marilith? */
		break;
	case AT_WEAP:
use_weapon:
	/* Certain monsters don't use weapons when encountered as enemies,
	 * but players who polymorph into them have hands or claws and thus
	 * should be able to use weapons.  This shouldn't prohibit the use
	 * of most special abilities, either.
	 */
	/* Potential problem: if the monster gets multiple weapon attacks,
	 * we currently allow the player to get each of these as a weapon
	 * attack.  Is this really desirable?
	 */
		dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
		/* Enemy dead, before any special abilities used */
		if (!known_hitum(mon,&dhit,mattk)) {
			    sum[i] = 2;
		} else {
				sum[i] = dhit;
			/* might be a worm that gets cut in half */
			if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
			/* Do not print "You hit" message, since known_hitum
			 * already did it.
			 */
			if (dhit && mattk->adtyp != AD_SPEL
				&& mattk->adtyp != AD_PHYS)
				sum[i] = damageum(mon,mattk);
		}
		if(u.sealsActive&SEAL_MARIONETTE && uwep && mattk->adtyp == AD_PHYS){
			struct monst *m2 = m_at(mon->mx+u.dx,mon->my+u.dy);
			if(!m2 || DEADMONSTER(m2) || m2==mon) break;
			dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
			/* Enemy dead, before any special abilities used */
			known_hitum(m2,&dhit,mattk);
			/* might be a worm that gets cut in half */
		}
	break;
	case AT_MARI:{
	mari++;
	dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
	/* Enemy dead, before any special abilities used */
	if (!known_hitum_wepi(mon,&dhit,mattk,mari)) {
			sum[i] = 2;
	} else {
			sum[i] = dhit;
		/* might be a worm that gets cut in half */
		if (m_at(u.ux+u.dx, u.uy+u.dy) != mon) return((boolean)(nsum != 0));
		/* Do not print "You hit" message, since known_hitum
		 * already did it.
		 */
		if (dhit && mattk->adtyp != AD_SPEL
			&& mattk->adtyp != AD_PHYS)
			sum[i] = damageum(mon,mattk);
	}
	if(u.sealsActive&SEAL_MARIONETTE && uwep && mattk->adtyp == AD_PHYS){
		struct monst *m2 = m_at(mon->mx+u.dx,mon->my+u.dy);
		if(!m2 || DEADMONSTER(m2) || m2==mon) break;
		dhit = (weptmp > (dieroll = rnd(20)) || u.uswallow);
		/* Enemy dead, before any special abilities used */
		known_hitum_wepi(m2,&dhit,mattk,mari);
		/* might be a worm that gets cut in half */
	}
	}break;
	case AT_CLAW:
	case AT_LRCH: /*Note: long reach attacks are being treated as melee only for polymorph purposes*/
		// if (i==0 && uwep && !cantwield(youracedata)) goto use_weapon;
#ifdef SEDUCE
		/* succubi/incubi are humanoid, but their _second_
		 * attack is AT_CLAW, not their first...
		 */
		// if (i==1 && uwep && (u.umonnum == PM_SUCCUBUS ||
			// u.umonnum == PM_INCUBUS)) goto use_weapon;
#endif
	case AT_KICK:
	case AT_BITE:
	case AT_LNCK: /*Note: long reach attacks are being treated as melee only for polymorph purposes*/
		/* [ALI] Vampires are also smart. They avoid biting
		   monsters if doing so would be fatal */
		if ((uwep || (u.twoweap && uswapwep)  || uarmg) &&
			is_vampire(youracedata) &&
			(is_rider(mon->data) ||
			 mon->data == &mons[PM_GREEN_SLIME] ||
			 mon->data == &mons[PM_FLUX_SLIME])){
				break;
			}
	case AT_STNG:
	case AT_TUCH:
	case AT_5SQR:
	case AT_BUTT:
	case AT_TENT:
	case AT_WHIP:
		if ((uwep || (u.twoweap && uswapwep) || uarmg) &&
			mattk->adtyp != AD_STAR && mattk->adtyp != AD_BLUD && mattk->adtyp != AD_SHDW && 
			(touch_petrifies(mon->data) ||
			 mon->data == &mons[PM_MEDUSA])){
				break;
		}
		dhit = (((u.sealsActive&SEAL_CHUPOCLOPS || mattk->aatyp) ? tchtmp : tmp) > rnd(20) || u.uswallow);
		goto wisp_shdw_dhit2;
	case AT_WISP:
		dhit = (tmp > rnd(20) || u.uswallow);
		goto wisp_shdw_dhit2;
	case AT_SHDW:
		dhit = (tchtmp > rnd(20) || u.uswallow);
wisp_shdw_dhit2:
		if ((dhit) != 0) {
			int compat;

			if (!u.uswallow &&
			(compat=could_seduce(&youmonst, mon, mattk))) {
			You("%s %s %s.",
				!is_blind(mon) && haseyes(mon->data)
				? "smile at" : "talk to",
				mon_nam(mon),
				compat == 2 ? "engagingly":"seductively");
			/* doesn't anger it; no wakeup() */
				sum[i] = damageum(mon, mattk);
			break;
			}
			wakeup(mon, TRUE);
			/* maybe this check should be in damageum()? */
			if (insubstantial(mon->data) &&
				!(mattk->aatyp == AT_KICK && insubstantial_aware(mon, uarmf, TRUE))
				&& !insubstantial_aware(mon, (struct obj *)0, TRUE)) {
			Your("attack passes harmlessly through %s.",
				mon_nam(mon));
			break;
			}
			if (mattk->aatyp == AT_KICK)
				You("kick %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_BITE || mattk->aatyp == AT_LNCK)
				You("bite %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_STNG)
				You("sting %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_BUTT)
				You("butt %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_TUCH || mattk->aatyp == AT_5SQR)
				You("touch %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_TENT)
				Your("tentacles suck %s.", mon_nam(mon));
			else if (mattk->aatyp == AT_WHIP)
				Your("barbed whips lash %s.", mon_nam(mon));
			else if(mattk->adtyp == AT_SHDW) {
				if(youracedata == &mons[PM_EDDERKOP]) You("slash %s with bladed shadows.", mon_nam(mon));
				else Your("bladed shadow strikes %s.", mon_nam(mon));
			} else if(mattk->aatyp == AT_WISP) 
				Your("mist tendrils lash %s.", mon_nam(mon));
			else You("hit %s.", mon_nam(mon));
		    sum[i] = damageum(mon, mattk);
		} else
			missum(mon, mattk);
	break;
	case AT_HITS:
			sum[i] = damageum(mon, mattk);
	break;
	case AT_HUGS:
		/* automatic if prev two attacks succeed, or if
		 * already grabbed in a previous attack
		 */
		dhit = 1;
		wakeup(mon, TRUE);
		if (insubstantial(mon->data) && !insubstantial_aware(mon, (struct obj *)0, TRUE))
			Your("hug passes harmlessly through %s.",
			mon_nam(mon));
		else if (!sticks(mon->data) && !u.uswallow) {
			if (mon==u.ustuck) {
				pline("%s is being %s.", Monnam(mon),
					u.umonnum==PM_ROPE_GOLEM ? "choked":
					"crushed");
				sum[i] = damageum(mon, mattk);
			    } else if(i >= 2 && sum[i-1] && sum[i-2]) {
				You("grab %s!", mon_nam(mon));
				u.ustuck = mon;
				sum[i] = damageum(mon, mattk);
			}
		}
		break;

	case AT_EXPL:	/* automatic hit if next to */
		dhit = -1;
		wakeup(mon, TRUE);
			sum[i] = explum(mon, mattk);
		break;

	case AT_ENGL:
		if((dhit = (tmp > rnd(20+i)))) {
			wakeup(mon, TRUE);
			if (insubstantial(mon->data) && !insubstantial_aware(mon, (struct obj *)0, TRUE))
				Your("attempt to surround %s is harmless.",
				mon_nam(mon));
			else {
				    sum[i]= gulpum(mon,mattk);
				    if (sum[i] == 2 &&
					(mon->data->mlet == S_ZOMBIE ||
					mon->data->mlet == S_MUMMY) &&
					rn2(5) &&
					!Sick_resistance) {
				You_feel("%ssick.",
					(Sick) ? "very " : "");
				mdamageu(mon, rnd(8));
				}
			}
		} else
			missum(mon, mattk);
		break;

#ifdef YOUMONST_SPELL
	case AT_MAGC:
		/* No check for uwep; if wielding nothing we want to
		 * do the normal 1-2 points bare hand damage...
		 */
		/*
		if (i==0 && (youracedata->mlet==S_KOBOLD
			|| youracedata->mlet==S_ORC
			|| youracedata->mlet==S_GNOME
			)) goto use_weapon;
		*/
			sum[i] = castum(mon, mattk);
			continue;
#endif /* YOUMONST_SPELL */

	case AT_NONE:
	case AT_BOOM:
			continue;
		/* Not break--avoid passive attacks from enemy */

	case AT_BREA:
	case AT_BEAM:
	case AT_SPIT:
	case AT_GAZE:	/* all done using #monster command */
		dhit = 0;
		break;
	case AT_WDGZ:	/* passive */
		dhit = 0;
		break;

	default: /* Strange... */
		impossible("strange attack of yours (%d)",
			 mattk->aatyp);
	}
	if (dhit == -1) {
	u.mh = -1;	/* dead in the current form */
	rehumanize();
	}
	    if (sum[i] == 2)
	return((boolean)passive(mon, 1, 0, mattk->aatyp, mattk->adtyp));
						/* defender dead */
	else {
		(void) passive(mon, sum[i], 1, mattk->aatyp, mattk->adtyp);
		if (DEADMONSTER(mon))
			return FALSE;
		nsum |= sum[i];
	}
	if (Upolyd != Old_Upolyd)
		break; /* No extra attacks if form changed */
	if (multi < 0)
		break; /* If paralyzed while attacking, i.e. floating eye */
	}
	return(!DEADMONSTER(mon));
}

/*	Special (passive) attacks on you by monsters done here.		*/

int
passive(mon, mhit, malive, aatyp, adtyp)
register struct monst *mon;
register boolean mhit;
register int malive;
uchar aatyp, adtyp;
{
	register struct permonst *ptr = mon->data;
	register int i, tmp, ptmp;
	struct obj *optr;
	char	 buf[BUFSZ];
	
	/* Otiax's mist tendrils don't bring you into contact. */
	if(aatyp == AT_WISP || aatyp == AT_SHDW) return (malive | mhit);
	
	if(u.sealsActive&SEAL_IRIS && !Blind && canseemon(mon) && !Invisible
		&& !is_vampire(youracedata)
		&& mon_reflects(mon,"You catch a glimpse of a stranger's reflection in %s %s.")
	){
		if(u.sealsActive&SEAL_IRIS) unbind(SEAL_IRIS,TRUE);
	}
	
	if (mhit && aatyp == AT_BITE && is_vampire(youracedata)) {
	    if (bite_monster(mon))
		return 2;			/* lifesaved */
	}
	for(i = 0; ; i++) {
	    if(i >= NATTK) return(malive | mhit);	/* no passive attacks */
	    if(ptr->mattk[i].aatyp == AT_NONE) break;	/* try this one */
	}
	/* Note: tmp not always used */
	if (ptr->mattk[i].damn)
	    tmp = d((int)ptr->mattk[i].damn, (int)ptr->mattk[i].damd);
	else if(ptr->mattk[i].damd)
	    tmp = d((int)mon->m_lev+1, (int)ptr->mattk[i].damd);
	else
	    tmp = 0;
	
/*	These affect you even if they just died */

	// Grue has no passive attacks while in the light
	if (ptr == &mons[PM_GRUE] && !((!levl[mon->mx][mon->my].lit && !(viz_array[mon->my][mon->mx] & TEMP_LIT1 && !(viz_array[mon->my][mon->mx] & TEMP_DRK1)))
		|| (levl[mon->mx][mon->my].lit && (viz_array[mon->my][mon->mx] & TEMP_DRK1 && !(viz_array[mon->my][mon->mx] & TEMP_LIT1)))))
		return (malive | mhit);	

	switch(ptr->mattk[i].adtyp) {
		
	  case AD_BARB:
		if(ptr == &mons[PM_RAZORVINE]) You("are hit by the springing vines!");
		else You("are hit by %s barbs!", s_suffix(mon_nam(mon)));
		if (tmp) {
			tmp -= roll_udr(mon);
			if (tmp < 1) tmp = 1;
		}
		mdamageu(mon, tmp);
	  break;
	  case AD_ACID:
	    if(mhit && rn2(2)) {
		if (Blind || !flags.verbose) You("are splashed!");
		else	You("are splashed by %s acid!",
			                s_suffix(mon_nam(mon)));

		if (!Acid_resistance)
			mdamageu(mon, tmp);
		if(!rn2(30)) erode_armor(&youmonst, TRUE);
	    }
	    if (mhit) {
		if (aatyp == AT_KICK) {
		    if (uarmf && !rn2(6))
			(void)rust_dmg(uarmf, xname(uarmf), 3, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_XWEP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH || aatyp == AT_5SQR)
		    passive_obj(mon, (struct obj*)0, &(ptr->mattk[i]));
	    }
	    exercise(A_STR, FALSE);
	    break;
	  case AD_STON:
	    if (mhit) {		/* successful attack */
		long protector = attk_protection((int)aatyp);

		/* hero using monsters' AT_MAGC attack isn't touching the monster */
		if (aatyp == AT_MAGC) break;
		
		/* Otiax's mist tendrils don't cause skin contact. */
		if (aatyp == AT_WISP || aatyp == AT_SHDW) break;

		if(adtyp == AD_SHDW || adtyp == AD_STAR || adtyp == AD_BLUD) break;
		
		if (protector == 0L ||		/* no protection */
			(protector == W_ARMG && !uarmg && !uwep) ||
			(protector == W_ARMF && !uarmf) ||
			(protector == W_ARMH && !uarmh) ||
			(protector == (W_ARMC|W_ARMG) && (!uarmc || !uarmg))) {
		    if (!Stone_resistance &&
			    !(poly_when_stoned(youracedata) &&
				polymon(PM_STONE_GOLEM))) {
			You("turn to stone...");
			done_in_by(mon);
			return 2;
		    }
		}
	    }
	    break;
	  case AD_RUST:
	    if(mhit && !mon->mcan) {
		if(mon->data==&mons[PM_NAIAD]){
		  if((malive && mon->mhp < .5*mon->mhpmax
		   && rn2(3)) || !malive){
			  pline("%s collapses into a puddle of water!", Monnam(mon));
			  if(malive){
				killed(mon);
				malive = 0;
				// mon->mhp = 0;
			  }
		  } else break;
		}
		if (aatyp == AT_KICK) {
		    if (uarmf)
			(void)rust_dmg(uarmf, xname(uarmf), 1, TRUE, &youmonst);
		} else if (aatyp == AT_WEAP || aatyp == AT_XWEP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH || aatyp == AT_5SQR)
		    passive_obj(mon, (struct obj*)0, &(ptr->mattk[i]));
	    }
	    break;
	  case AD_CORR:
	    if(mhit && !mon->mcan) {
		if (aatyp == AT_KICK) {
		    if (uarmf)
			(void)rust_dmg(uarmf, xname(uarmf), 3, TRUE, &youmonst);
		}
		else if (aatyp == AT_WEAP || aatyp == AT_XWEP || aatyp == AT_CLAW ||
			   aatyp == AT_MAGC || aatyp == AT_TUCH || aatyp == AT_5SQR)
		    passive_obj(mon, (struct obj*)0, &(ptr->mattk[i]));
	    }
	    break;
	  case AD_MAGM:
	    /* wrath of gods for attacking Oracle */
	    if(Antimagic) {
		shieldeff(u.ux, u.uy);
		pline("A hail of magic missiles narrowly misses you!");
	    } else {
		You("are hit by magic missiles appearing from thin air!");
		mdamageu(mon, tmp);
	    }
	    break;
	  case AD_WEBS:{
		struct trap *ttmp2 = maketrap(u.ux, u.uy, WEB);
		if (ttmp2) {
			pline_The("webbing sticks to you. You're caught!");
			dotrap(ttmp2, NOWEBMSG);
#ifdef STEED
			if (u.usteed && u.utrap) {
			/* you, not steed, are trapped */
			dismount_steed(DISMOUNT_FELL);
			}
#endif
		}
	 } break;
	  case AD_ENCH:	/* KMH -- remove enchantment (disenchanter) */
	    if (mhit) {
		struct obj *obj = (struct obj *)0;

		if (aatyp == AT_KICK) {
		    obj = uarmf;
		    if (!obj) break;
		} else if (aatyp == AT_BITE || aatyp == AT_LNCK || aatyp == AT_LRCH || aatyp == AT_BUTT ||
			   (aatyp >= AT_STNG && aatyp < AT_WEAP)) {
		    break;		/* no object involved */
		}
		passive_obj(mon, obj, &(ptr->mattk[i]));
	    }
	    break;
	    case AD_SHDW:
		pline("Its bladed shadow falls on you!");
		mdamageu(mon, tmp);
	    case AD_DRST:
		ptmp = A_STR;
		goto dobpois;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
	    case AD_DRDX:
		ptmp = A_DEX;
		goto dobpois;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
	    case AD_DRCO:
		ptmp = A_CON;
///////////////////////////////////////////////////////////////////////////////////////////////////////////
dobpois:
		if (!rn2(8)) {
			if(ptr->mattk[i].adtyp == AD_SHDW) Sprintf(buf, "%s shadow",
				s_suffix(Monnam(mon)));
			else {
				if(!mhit) break; //didn't draw blood, forget it.
				Sprintf(buf, "%s blood", s_suffix(Monnam(mon)));
			}
		    poisoned(buf, ptmp, mon->data->mname, 30, 0);
		}
	  break;
	  case AD_HLBD:
//		pline("hp: %d x: %d y: %d", (mon->mhp*100)/mon->mhpmax, mon->mx, mon->my);
	    if(!mhit) break; //didn't draw blood, forget it.
		if(mon->data == &mons[PM_LEGION]){
			int n = rnd(4);
			for(; n>0; n--) rn2(7) ? makemon(mkclass(S_ZOMBIE, G_NOHELL|G_HELL), mon->mx, mon->my, NO_MINVENT|MM_ADJACENTOK|MM_ADJACENTSTRICT): 
									  makemon(&mons[PM_LEGIONNAIRE], mon->mx, mon->my, NO_MINVENT|MM_ADJACENTOK|MM_ADJACENTSTRICT);
		} else {
			if(mon->mhp > .75*mon->mhpmax) makemon(&mons[PM_LEMURE], mon->mx, mon->my, MM_ADJACENTOK);
			else if(mon->mhp > .50*mon->mhpmax) makemon(&mons[PM_HORNED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			else if(mon->mhp > .25*mon->mhpmax) makemon(&mons[PM_BARBED_DEVIL], mon->mx, mon->my, MM_ADJACENTOK);
			else if(mon->mhp > .00*mon->mhpmax) makemon(&mons[PM_PIT_FIEND], mon->mx, mon->my, MM_ADJACENTOK);
		}
	  break;
	  case AD_OONA:
//			pline("hp: %d x: %d y: %d", (mon->mhp*100)/mon->mhpmax, mon->mx, mon->my);
			if(!mhit) break; //didn't draw blood, forget it.
			
			if(u.oonaenergy == AD_FIRE){
				if(rn2(2)) makemon(&mons[PM_FLAMING_SPHERE], mon->mx, mon->my, MM_ADJACENTOK);
				else makemon(&mons[PM_FIRE_VORTEX], mon->mx, mon->my, MM_ADJACENTOK);
			} else if(u.oonaenergy == AD_COLD){
				if(rn2(2)) makemon(&mons[PM_FREEZING_SPHERE], mon->mx, mon->my, MM_ADJACENTOK);
				else makemon(&mons[PM_ICE_VORTEX], mon->mx, mon->my, MM_ADJACENTOK);
			} else if(u.oonaenergy == AD_ELEC){
				if(rn2(2)) makemon(&mons[PM_SHOCKING_SPHERE], mon->mx, mon->my, MM_ADJACENTOK);
				else makemon(&mons[PM_ENERGY_VORTEX], mon->mx, mon->my, MM_ADJACENTOK);
			}
	  break;
	  case AD_DISE:
		  if(mon->data==&mons[PM_SWAMP_NYMPH]){
			  if(mon->mhp > 0 && mon->mhp < .5*mon->mhpmax
			   && rn2(3)){
  				  pline("A cloud of spores is released!");
				  diseasemu(mon->data);
				  pline("%s collapses in a puddle of noxious fluid!", Monnam(mon));
				  if(malive) killed(mon);
				  // malive = 0;
				  // mon->mhp = 0;
			  }
		  }
		  else if(mon->data==&mons[PM_SWAMP_FERN] && !mon->mspec_used){
			  if(!rn2(3)){
  				  pline("A cloud of spores is released!");
				  diseasemu(mon->data);
				  mon->mspec_used = 1;
			  }
		  }
		  else if(!mon->mspec_used){
			  pline("A cloud of spores is released!");
			  diseasemu(mon->data);
			  mon->mspec_used = 1;
		  }
	  break;
	  default:
	    break;
	}

	if(ptr->mattk[i].adtyp==AD_AXUS) u.uevent.uaxus_foe = 1;//enemy of the modrons
	/*	These only affect you if they still live */
	if(malive && !mon->mcan && rn2(3)) {

	    switch(ptr->mattk[i].adtyp) {
		case AD_AXUS:{
		  int mndx = 0;
		  struct monst *mtmp;
		   if(ward_at(u.ux,u.uy) != HAMSA){
		    if(canseemon(mon) && !is_blind(mon)) {//paralysis gaze
				if (Free_action)
				    You("momentarily stiffen under %s gaze!",
					    s_suffix(mon_nam(mon)));
				else if (ureflects("%s gaze is partially reflected by your %s.",
							s_suffix(Monnam(mon)))){
				    You("are frozen by %s gaze!",
					  s_suffix(mon_nam(mon)));
				    nomul(-1*d(1,10), "frozen by the gaze of Axus");
				}
				else {
				    You("are frozen by %s gaze!",
					  s_suffix(mon_nam(mon)));
				    nomul(-tmp, "frozen by the gaze of Axus");
				}
		    }
		   }
			// Moved to an active attack
			// pline("%s reaches out with %s %s!  A corona of dancing energy surrounds the %s!",
				// mon_nam(mon), mhis(mon), mbodypart(mon, ARM), mbodypart(mon, HAND));
			// if(Shock_resistance) {
			    // shieldeff(u.ux, u.uy);
			    // You_feel("a mild tingle.");
			    // ugolemeffects(AD_ELEC, tmp);
			// }
			// else{
				// You("are jolted with energy!");
				// mdamageu(mon, tmp);
				// if (!rn2(4)) (void) destroy_item(POTION_CLASS, AD_FIRE);
				// if (!rn2(4)) (void) destroy_item(SCROLL_CLASS, AD_FIRE);
				// if (!rn2(10)) (void) destroy_item(SPBOOK_CLASS, AD_FIRE);
				// if (!rn2(10)) (void) destroy_item(RING_CLASS, AD_ELEC);
				// if (!rn2(10)) (void) destroy_item(WAND_CLASS, AD_ELEC);
			// }

			// if(!Stunned) make_stunned((long)tmp, TRUE);
			// pline("%s reaches out with %s other %s!  A penumbra of shadows surrounds the %s!",
				// mon_nam(mon), mhis(mon), mbodypart(mon, ARM), mbodypart(mon, HAND));
		    // if(Cold_resistance) {
				// shieldeff(u.ux, u.uy);
				// You_feel("a mild chill.");
				// ugolemeffects(AD_COLD, tmp);
		    // }
			// else{
			    // You("are suddenly very cold!");
			    // mdamageu(mon, tmp);
				// if (!rn2(4)) (void) destroy_item(POTION_CLASS, AD_COLD);
			// }
			// if (!Drain_resistance) {
			    // losexp("life force drain",TRUE,FALSE,FALSE);
			// }
		  // }
		  
			for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
				mndx = monsndx(mtmp->data);
				if(mndx <= PM_QUINON && mndx >= PM_MONOTON && mtmp->mpeaceful){
					pline("%s gets angry...", Amonnam(mtmp));
					mtmp->mpeaceful = 0;
					mtmp->mtame = 0;
				}
			}
		} break;
	  	  case AD_UNKNWN:
			  if(uwep && uwep->oartifact && uwep->oartifact != ART_SILVER_KEY && uwep->oartifact != ART_ANNULUS
				&& uwep->oartifact != ART_PEN_OF_THE_VOID && CountsAgainstGifts(uwep->oartifact)
			  ){
					You_feel("%s tug gently on your %s.",mon_nam(mon), ONAME(uwep));
					if(yn("Release it?")=='n'){
						You("hold on tight.");
					}
					else{
						You("let %s take your %s.",mon_nam(mon), ONAME(uwep));
						pline_The(Hallucination ? "world pats you on the head." : "world quakes around you.  Perhaps it is the voice of a god?");
						do_earthquake(u.ux, u.uy, 10, 2, FALSE, (struct monst *)0);
						optr = uwep;
						uwepgone();
						if(optr->gifted != A_NONE && !Role_if(PM_EXILE)){
							gods_angry(optr->gifted);
							gods_upset(optr->gifted);
						}
						useup(optr);
						u.regifted++;
						mongone(mon);
						if (u.regifted == 5){
							u.uevent.uunknowngod = 1;
							You_feel("worthy.");
							if (Role_if(PM_EXILE))
							{
								pline("The image of an unknown and strange seal fills your mind!");
								u.specialSealsKnown |= SEAL_UNKNOWN_GOD;
							}
						}
					}
			  }
		  break;

	  	  case AD_SSEX:{
			int mndx = monsndx(mon->data);
			
			static int engagering4 = 0;
			if (!engagering4) engagering4 = find_engagement_ring();
			if ( (uleft && uleft->otyp == engagering4) || (uright && uright->otyp == engagering4)) break;
			
			if(mndx == PM_MOTHER_LILITH)
				if(mon->mcan) mon->mcan=0;
			if(mndx == PM_MOTHER_LILITH && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1){
				dolilithseduce(mon);
			}
			else if(mndx == PM_BELIAL && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1){
			}
//			else if(mndx == PM_SHAMI_AMOURAE && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
//				&& !mon->mcan){
//				dosflseduce(mon);
//			}
//			else if(mndx == PM_THE_DREAMER && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
//				&& !mon->mcan){
//			}
//			else if(mndx == PM_XINIVRAE && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
//				&& !mon->mcan){
//			}
//			else if(mndx == PM_LYNKHAB && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
//				&& !mon->mcan){
//			}
			else if(mndx == PM_MALCANTHET && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
				&& !mon->mcan){
				domlcseduce(mon);
			}
			else if(mndx == PM_GRAZ_ZT && could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
				&& !mon->mcan){
			}
			else if(mndx == PM_PALE_NIGHT){
				dopaleseduce(mon);
				nomul(-1,"staring at a veil");
			}
			else if(could_seduce(mon, &youmonst, &(ptr->mattk[i])) == 1 
				&& !mon->mcan){
			    doseduce(mon);
			}
		  }break;

	      case AD_PLYS:
		if(ptr->mlet == S_EYE) {
			if(ward_at(u.ux,u.uy) == HAMSA) return(malive | mhit);
		    if (!canseemon(mon)) {
				break;
		    }
		    if(!is_blind(mon)) {
				if (ureflects("%s gaze is reflected by your %s.",
					    s_suffix(Monnam(mon))));
				else if (Free_action)
				    You("momentarily stiffen under %s gaze!",
					    s_suffix(mon_nam(mon)));
				else {
				    You("are frozen by %s gaze!",
					  s_suffix(mon_nam(mon)));
				    nomul(-tmp, "frozen by a monster's gaze");
				}
		    } else {
				pline("%s cannot defend itself.",
					Adjmonnam(mon,"blind"));
				if(!rn2(500)) change_luck(-1);
		    }
		} else if (Free_action) {
		    You("momentarily stiffen.");
		} else { /* gelatinous cube */
		    You("are frozen by %s!", mon_nam(mon));
	    	    nomovemsg = 0;	/* default: "you can move again" */
		    nomul(-tmp, "frozen by a monster");
		    exercise(A_DEX, FALSE);
		}
		break;
		case AD_COLD:		/* brown mold or blue jelly */
			if(monnear(mon, u.ux, u.uy)) {
			    if(Cold_resistance) {
					shieldeff(u.ux, u.uy);
					You_feel("a mild chill.");
					ugolemeffects(AD_COLD, tmp);
					break;
			    }
			    You("are suddenly very cold!");
			    mdamageu(mon, tmp);
			/* monster gets stronger with your heat! */
			    mon->mhp += tmp / 2;
			    if (mon->mhpmax < mon->mhp) mon->mhpmax = mon->mhp;
			/* at a certain point, the monster will reproduce! */
			    if(mon->mhpmax > ((int) (mon->m_lev+1) * 8) && (mon->data == &mons[PM_BROWN_MOLD] || mon->data == &mons[PM_BLUE_JELLY]))
				(void)split_mon(mon, &youmonst);
			}
		break;
        case AD_STUN:		/* specifically yellow mold */
			if(!Stunned)
			    make_stunned((long)tmp, TRUE);
			break;
	    case AD_FIRE:
			if(monnear(mon, u.ux, u.uy)) {
				    if(Fire_resistance) {
					shieldeff(u.ux, u.uy);
					You_feel("mildly warm.");
					ugolemeffects(AD_FIRE, tmp);
					break;
			    }
			    You("are suddenly very hot!");
			    mdamageu(mon, tmp);
			}
		break;
	      case AD_ELEC:
		if(Shock_resistance) {
		    shieldeff(u.ux, u.uy);
		    You_feel("a mild tingle.");
		    ugolemeffects(AD_ELEC, tmp);
		    break;
		}
		You("are jolted with electricity!");
		mdamageu(mon, tmp);
		break;
	      default:
		break;
	    }
	}
	return(malive | mhit);
}

/*
 * Special (passive) attacks on an attacking object by monsters done here.
 * Assumes the attack was successful.
 */
void
passive_obj(mon, obj, mattk)
register struct monst *mon;
register struct obj *obj;	/* null means pick uwep, uswapwep or uarmg */
struct attack *mattk;		/* null means we find one internally */
{
	register struct permonst *ptr = mon->data;
	register int i;

	/* if caller hasn't specified an object, use uwep, uswapwep or uarmg */
	if (!obj) {
	    obj = (u.twoweap && uswapwep && !rn2(2)) ? uswapwep : uwep;
	    if (!obj && mattk->adtyp == AD_ENCH)
		obj = uarmg;		/* no weapon? then must be gloves */
	    if (!obj) return;		/* no object to affect */
	}

	/* if caller hasn't specified an attack, find one */
	if (!mattk) {
	    for(i = 0; ; i++) {
		if(i >= NATTK) return;	/* no passive attacks */
		if(ptr->mattk[i].aatyp == AT_NONE) break; /* try this one */
	    }
	    mattk = &(ptr->mattk[i]);
	}

	switch(mattk->adtyp) {

	case AD_ACID:
	    if(!rn2(6)) {
		erode_obj(obj, TRUE, FALSE);
	    }
	    break;
	case AD_RUST:
	    if(!mon->mcan) {
		erode_obj(obj, FALSE, FALSE);
	    }
	    break;
	case AD_CORR:
	    if(!mon->mcan) {
		erode_obj(obj, TRUE, FALSE);
	    }
	    break;
	case AD_ENCH:
	    if (!mon->mcan) {
		if (drain_item(obj) && carried(obj) &&
		    (obj->known || obj->oclass == ARMOR_CLASS)) {
		    Your("%s less effective.", aobjnam(obj, "seem"));
	    	}
	    	break;
	    }
	  default:
	    break;
	}

	if (carried(obj)) update_inventory();
}

/* Note: caller must ascertain mtmp is mimicking... */
void
stumble_onto_mimic(mtmp)
struct monst *mtmp;
{
	const char *fmt = "Wait!  That's %s!",
		   *generic = "a monster",
		   *what = 0;

	if(!u.ustuck && !mtmp->mflee && dmgtype(mtmp->data,AD_STCK))
	    u.ustuck = mtmp;

	if (Blind) {
	    if (!Blind_telepat)
		what = generic;		/* with default fmt */
	    else if (mtmp->m_ap_type == M_AP_MONSTER)
		what = a_monnam(mtmp);	/* differs from what was sensed */
	} else {
	    int glyph = levl[u.ux+u.dx][u.uy+u.dy].glyph;

	    if (glyph_is_cmap(glyph) &&
		    (glyph_to_cmap(glyph) == S_hcdoor ||
		     glyph_to_cmap(glyph) == S_vcdoor))
		fmt = "The door actually was %s!";
	    else if (glyph_is_object(glyph) &&
		    glyph_to_obj(glyph) == GOLD_PIECE)
		fmt = "That gold was %s!";

	    /* cloned Wiz starts out mimicking some other monster and
	       might make himself invisible before being revealed */
	    if (mtmp->minvis && !See_invisible(mtmp->mx,mtmp->my))
		what = generic;
	    else
		what = a_monnam(mtmp);
	}
	if (what) pline(fmt, what);

	wakeup(mtmp, TRUE);	/* clears mimicking */
}

STATIC_OVL void
nohandglow(mon)
struct monst *mon;
{
	char *hands=makeplural(body_part(HAND));

	if (!u.umconf || mon->mconf) return;
	if (u.umconf == 1) {
		if (Blind)
			Your("%s stop tingling.", hands);
		else
			Your("%s stop glowing %s.", hands, hcolor(NH_RED));
	} else {
		if (Blind)
			pline_The("tingling in your %s lessens.", hands);
		else
			Your("%s no longer glow so brightly %s.", hands,
				hcolor(NH_RED));
	}
	u.umconf--;
}

int
flash_hits_mon(mtmp, otmp)
struct monst *mtmp;
struct obj *otmp;	/* source of flash */
{
	int tmp, amt, res = 0, useeit = canseemon(mtmp);

	if (mtmp->msleeping) {
	    mtmp->msleeping = 0;
	    if (useeit) {
		pline_The("flash awakens %s.", mon_nam(mtmp));
		res = 1;
	    }
	} else if (mtmp->data->mlet != S_LIGHT) {
	    if (!resists_blnd(mtmp)) {
		tmp = dist2(otmp->ox, otmp->oy, mtmp->mx, mtmp->my);
		if (useeit) {
		    pline("%s is blinded by the flash!", Monnam(mtmp));
		    res = 1;
		}
		if (mtmp->data == &mons[PM_GREMLIN]) {
		    /* Rule #1: Keep them out of the light. */
		    amt = otmp->otyp == WAN_LIGHT ? d(1 + otmp->spe, 4) :
		          rn2(min(mtmp->mhp,4));
		    pline("%s %s!", Monnam(mtmp), amt > mtmp->mhp / 2 ?
			  "wails in agony" : "cries out in pain");
		    if ((mtmp->mhp -= amt) <= 0) {
			if (flags.mon_moving)
			    monkilled(mtmp, (char *)0, AD_BLND);
			else
			    killed(mtmp);
		    } else if (cansee(mtmp->mx,mtmp->my) && !canspotmon(mtmp)){
			map_invisible(mtmp->mx, mtmp->my);
		    }
			if(mtmp && amt > 0) mtmp->uhurtm = TRUE;
		}
		if (mtmp->mhp > 0) {
		    if (!flags.mon_moving) setmangry(mtmp);
		    if (tmp < 9 && !mtmp->isshk && rn2(4)) {
			if (rn2(4))
			    monflee(mtmp, rnd(100), FALSE, TRUE);
			else
			    monflee(mtmp, 0, FALSE, TRUE);
		    }
		    mtmp->mcansee = 0;
		    mtmp->mblinded = (tmp < 3) ? 0 : rnd(1 + 50/tmp);
		}
	    }
	}
	return res;
}

/*uhitm.c*/

void
reset_udieroll()
{
	dieroll = 10;
}
