/*	SCCS Id: @(#)timeout.c	3.4	2002/12/17	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#pragma clang diagnostic ignored "-Wint-to-void-pointer-cast"

#include "hack.h"
#include "lev.h"	/* for checking save modes */

STATIC_DCL void NDECL(stoned_dialogue);
STATIC_DCL void NDECL(golded_dialogue);
#ifdef CONVICT
STATIC_DCL void NDECL(phasing_dialogue);
#endif /* CONVICT */
STATIC_DCL void NDECL(vomiting_dialogue);
STATIC_DCL void NDECL(choke_dialogue);
STATIC_DCL void NDECL(slime_dialogue);
STATIC_DCL void NDECL(slip_or_trip);
STATIC_DCL void FDECL(see_lamp_flicker, (struct obj *, const char *));
STATIC_DCL void FDECL(lantern_message, (struct obj *));
STATIC_DCL void FDECL(cleanup_burn, (genericptr_t,long));

#ifdef OVLB

/* He is being petrified - dialogue by inmet!tower */
static NEARDATA const char * const stoned_texts[] = {
	"You are slowing down.",		/* 5 */
	"Your limbs are stiffening.",		/* 4 */
	"Your limbs have turned to stone.",	/* 3 */
	"You have turned to stone.",		/* 2 */
	"You are a statue."			/* 1 */
};

STATIC_OVL void
stoned_dialogue()
{
	register long i = (Stoned & TIMEOUT);

	if (i > 0L && i <= SIZE(stoned_texts)) {
		pline1(stoned_texts[SIZE(stoned_texts) - i]);
		nomul(0, NULL); /* fix for C343-74 */
	}
	if (i == 5L)
		HFast = 0L;
	if (i == 3L)
		nomul(-3, "getting stoned");
	exercise(A_DEX, FALSE);
}

static NEARDATA const char * const golded_texts[] = {
	"You are slowing down.",		/* 5 */
	"Your limbs are stiffening.",		/* 4 */
	"Your limbs have turned to gold.",	/* 3 */
	"You have turned to gold.",		/* 2 */
	"You are a gold statue."			/* 1 */
};

STATIC_OVL void
golded_dialogue()
{
	register long i = (Golded & TIMEOUT);

	if (i > 0L && i <= SIZE(golded_texts)) {
		pline1(golded_texts[SIZE(golded_texts) - i]);
		nomul(0, NULL); /* fix for C343-74 */
	}
	if (i == 5L)
		HFast = 0L;
	if (i == 3L)
		nomul(-3, "turning to gold");
	exercise(A_DEX, FALSE);
}

#ifdef CONVICT
STATIC_OVL void
phasing_dialogue()
{
    if (Phasing == 15) {
        if (!Hallucination) {
            Your("body is beginning to feel more solid.");
        } else {
            You_feel("more distant from the spirit world.");
        }
        stop_occupation();
    } else if (Phasing == 1) {
        if (!Hallucination) {
            Your("body is solid again.");
        } else {
            You_feel("totally separated from the spirit world.");
        }
        stop_occupation();
    }
}
#endif /* CONVICT */

/* He is getting sicker and sicker prior to vomiting */
static NEARDATA const char * const vomiting_texts[] = {
	"are feeling mildly nauseated.",	/* 14 */
	"feel slightly confused.",		/* 11 */
	"can't seem to think straight.",	/* 8 */
	"feel incredibly sick.",		/* 5 */
	"suddenly vomit!"			/* 2 */
};

STATIC_OVL void
vomiting_dialogue()
{
	register long i = (Vomiting & TIMEOUT) / 3L;

	if ((((Vomiting & TIMEOUT) % 3L) == 2) && (i >= 0)
	    && (i < SIZE(vomiting_texts)))
		You("%s", vomiting_texts[SIZE(vomiting_texts) - i - 1]);

	switch ((int) i) {
	case 0:
		vomit();
		morehungry(20);
		break;
	case 2:
		make_stunned(HStun + d(2,4), FALSE);
		/* fall through */
	case 3:
		make_confused(HConfusion + d(2,4), FALSE);
		break;
	}
	exercise(A_CON, FALSE);
}

static NEARDATA const char * const choke_texts[] = {
	"You find it hard to breathe.",
	"You're gasping for air.",
	"You can no longer breathe.",
	"You're turning %s.",
	"You suffocate."
};

static NEARDATA const char * const choke_texts2[] = {
	"Your %s is becoming constricted.",
	"Your blood is having trouble reaching your brain.",
	"The pressure on your %s increases.",
	"Your consciousness is fading.",
	"You suffocate."
};

STATIC_OVL void
choke_dialogue()
{
	register long i = u.divetimer;

	if(i > 0 && i <= SIZE(choke_texts)) {
	    if (!rn2(10))
			pline(choke_texts2[SIZE(choke_texts2) - i], body_part(NECK));
	    else {
		const char *str = choke_texts[SIZE(choke_texts)-i];

		if (index(str, '%'))
		    pline(str, hcolor(NH_BLUE));
		else
		    pline1(str);
	    }
	}
	exercise(A_STR, FALSE);
}

static NEARDATA const char * const slime_texts[] = {
	"You are turning a little %s.",           /* 5 */
	"Your limbs are getting oozy.",              /* 4 */
	"Your skin begins to peel away.",            /* 3 */
	"You are turning into %s.",       /* 2 */
	"You have become %s."             /* 1 */
};

STATIC_OVL void
slime_dialogue()
{
	register long i = (Slimed & TIMEOUT) / 2L;

	if (((Slimed & TIMEOUT) % 2L) && i >= 0L
		&& i < SIZE(slime_texts)) {
	    const char *str = slime_texts[SIZE(slime_texts) - i - 1L];

	    if (index(str, '%')) {
		if (i == 4L) {	/* "you are turning green" */
		    if (!Blind)	/* [what if you're already green?] */
			pline(str, hcolor(NH_GREEN));
		} else
		    pline(str, an(Hallucination ? rndmonnam() : "green slime"));
	    } else
		pline1(str);
	}
	if (i == 3L) {	/* limbs becoming oozy */
	    HFast = 0L;	/* lose intrinsic speed */
	    stop_occupation();
	    if (multi > 0) nomul(0, NULL);
	}
	exercise(A_DEX, FALSE);
}

void
burn_away_slime()
{
	if (Slimed) {
	    pline_The("slime that covers you is burned away!");
	    Slimed = 0L;
	    flags.botl = 1;
	}
	return;
}


#endif /* OVLB */
#ifdef OVL0

void
unbind(spir,forced)
long spir;
boolean forced;
{
	int i;
	boolean found = FALSE, gnosis = (spir == u.spirit[GPREM_SPIRIT]);
	
	if(spir==SEAL_JACK && forced && !gnosis && u.ulevel == 1){
		Your("life is saved!");
		pline("Unfortunately, your soul is torn to shreds.");
	}
	if(forced && !gnosis){
		if(spir == SEAL_ORTHOS && Hallucination) losexp("being eaten by a grue",TRUE,TRUE,TRUE);
		else losexp("shredding of the soul",TRUE,TRUE,TRUE);
		if(flags.run) nomul(0, NULL);
		stop_occupation();
	}
	
	if(forced && gnosis){
		You("are shaken to your core!");
		return;
	}
	
	if(forced && u.voidChime) return; //void chime allows you to keep spirits bound even if you break their taboos.
	
	if(spir == SEAL_ORTHOS && Hallucination) pline("Orthos has been eaten by a grue!");
	
	if(spir&SEAL_SPECIAL){
		for(i=0;i<(NUMINA-QUEST_SPIRITS);i++){
			if(((spir&(~SEAL_SPECIAL)) >> i) == 1L){
				Your("link with %s has %sbroken.", sealNames[i+(QUEST_SPIRITS-FIRST_SEAL)],forced?"been ":"");
				break;
			}
		}
		if(spir&SEAL_NUMINA) Your("links with the Numina have %sbroken.",forced?"been ":"");
	} else for(i=0;i<QUEST_SPIRITS;i++){
		if((spir >> i) == 1L){
			Your("link with %s has %sbroken.", sealNames[i],forced?"been ":"");
			break;
		}
	}
	
	if(!(spir&SEAL_SPECIAL)){
		int j;
		if(u.sealsActive&spir){
			u.sealsActive &= ~spir;
			if(spir&int_spirits) u.intSpirits--;
			else if(spir&wis_spirits) u.wisSpirits--;
			for(i=0; i<u.sealCounts; i++){
				if(u.spirit[i] == spir){
					found = TRUE;
				}
				if(found){
					if(i<u.sealCounts-1){
						u.spirit[i] = u.spirit[i+1];
						u.spiritT[i] = u.spiritT[i+1];
					} else {
						u.spirit[i]=0;
						u.spiritT[i]=0;
						u.sealCounts--;
					}
				}
			}
			if(!found){
				if(u.spirit[CROWN_SPIRIT] == spir){
					u.spirit[CROWN_SPIRIT] = 0;
				}
				else if(u.spirit[GPREM_SPIRIT] == spir){
					u.spirit[GPREM_SPIRIT] = 0;
				}
			}
		} else if(uwep && uwep->oartifact == ART_PEN_OF_THE_VOID){
			uwep->ovar1 &= ~spir;
			if(uwep->lamplit && !artifact_light(uwep)) end_burn(uwep, TRUE);
		} else if(uswapwep  && uswapwep->oartifact == ART_PEN_OF_THE_VOID){
			uswapwep->ovar1 &= ~spir;
			if(uswapwep->lamplit && !artifact_light(uswapwep)) end_burn(uswapwep, TRUE);
		}
		if(u.spiritTineA == spir){
			u.spiritTineA = u.spiritTineB;
			u.spiritTineTA = u.spiritTineTB;
			u.spiritTineB = 0;
			u.spiritTineTB = 0;
		} else if(u.spiritTineB == spir){
			u.spiritTineB = 0;
			u.spiritTineTB = 0;
		}
	} else {
		if(spir&SEAL_DAHLVER_NAR) u.wisSpirits--;
		if(spir&SEAL_ACERERAK) u.intSpirits--;
		u.specialSealsActive &= ~spir;
		if(u.specialSealsActive) u.specialSealsActive |= SEAL_SPECIAL;
		if(u.spirit[QUEST_SPIRIT] == spir){
			u.spirit[QUEST_SPIRIT]=0;
			u.spiritT[QUEST_SPIRIT]=0;
		} else if(u.spirit[ALIGN_SPIRIT] == spir){
			u.spirit[ALIGN_SPIRIT]=0;
			u.spiritT[ALIGN_SPIRIT]=0;
		} else if(u.spirit[OUTER_SPIRIT] == spir){
			u.spirit[OUTER_SPIRIT]=0;
			u.spiritT[OUTER_SPIRIT]=0;
		}
	}
	stop_occupation();
	if(flags.run) nomul(0, NULL);
	vision_full_recalc = 1;	/* visible monsters may have changed */
	doredraw();
	return;
}

char *spiritFadeTerms[] = {"starting to weaken","growing weaker","very faint","fading fast","about to break"};

void
nh_timeout()
{
	register struct prop *upp;
	int sleeptime, i, m_idx, baseluck = (flags.moonphase == FULL_MOON) ? 1 : 0;
	
	if (flags.friday13) baseluck -= 1;
	
	if(u.lastcast) u.lastcast--;
	
	if (u.uluck != baseluck) {
	    int timeout = 600;
	    int time_luck = stone_luck(FALSE);
		int stoneluck = stone_luck(TRUE);
	/* Cursed luckstones slow bad luck timing out; blessed luckstones
	 * slow good luck timing out; normal luckstones slow both;
	 * neither is affected if you don't have a luckstone.
	 * Luck is based at 0 usually, +1 if a full moon and -1 on Friday 13th
	 */
	    if (has_luckitem() && 
		    (!time_luck ||
		     (time_luck > 0 && u.uluck > baseluck) ||
		     (time_luck < 0 && u.uluck < baseluck))
		) {
			/* The slowed timeout depends on the distance between your 
			 * luck (not including luck bonuses) and your base luck.
			 * 
			 * distance	timeout
			 * --------------------
			 *  1		20000
			 *  2		18000
			 *  3		16000
			 *  4		14000
			 *  5		12000
			 *  6		10000
			 *  7		 8000
			 *  8		 6000
			 *  9		 4000
			 *  10		 2000
			 *  11		(600)
			 */ 
			int base_dist = u.uluck - baseluck;
			int slow_timeout;
			if (!Role_if(PM_CONVICT) || 
				stoneluck < 0 || 
				base_dist <= stoneluck*3
			){
				if(base_dist < 0) base_dist *= -1; /* magnitude only */
				slow_timeout = 22000 - 2000 * (base_dist);
				if (slow_timeout > timeout) timeout = slow_timeout;
			}
	    }

	    if (u.uhave.amulet || u.ugangr[Align2gangr(u.ualign.type)]) timeout = timeout / 2;

	    if (moves >= u.luckturn + timeout) {
			if(u.uluck > baseluck)
			u.uluck--;
			else if(u.uluck < baseluck)
			u.uluck++;
			u.luckturn = moves;
	    }
	}
#ifdef CONVICT
    if(Phasing) phasing_dialogue();
#endif /* CONVICT */
	if(u.uinvulnerable || u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20) return; /* things past this point could kill you */
	if(Stoned) stoned_dialogue();
	if(Golded) golded_dialogue();
	if(Slimed) slime_dialogue();
	if(Vomiting) vomiting_dialogue();
	if(Strangled) choke_dialogue();
	if(u.mtimedone && !--u.mtimedone) {
		if (Unchanging)
			u.mtimedone = rnd(100*youmonst.data->mlevel + 1);
		else
			rehumanize();
	}
	if(u.ucreamed) u.ucreamed--;

	/* Dissipate spell-based protection. */
	if (u.usptime) {
	    if (--u.usptime == 0 && u.uspellprot) {
		u.usptime = u.uspmtime;
		u.uspellprot--;
		find_ac();
		if (!Blind)
		    Norep("The %s haze around you %s.", hcolor(NH_GOLDEN),
			  u.uspellprot ? "becomes less dense" : "disappears");
	    }
	}
	
	if(u.sowdisc) u.sowdisc--; //timeout for discord
	if(u.voidChime){
		u.voidChime--;
		if(!u.voidChime){
			if(u.spiritTineA) u.sealsActive&=(~u.spiritTineA);
				if(u.spiritTineA&wis_spirits) u.wisSpirits--;
				if(u.spiritTineA&int_spirits) u.intSpirits--;
			if(u.spiritTineB) u.sealsActive&=(~u.spiritTineB);
				if(u.spiritTineB&wis_spirits) u.wisSpirits--;
				if(u.spiritTineB&int_spirits) u.intSpirits--;
			if(uwep && uwep->oartifact==ART_PEN_OF_THE_VOID){
				uwep->ovar1 = 0;
				uwep->ovar1 |= u.spiritTineA;
				uwep->ovar1 |= u.spiritTineB;
				if(artifact_light(uwep) && !uwep->lamplit) begin_burn(uwep, FALSE);
				else if(!artifact_light(uwep) && uwep->lamplit) end_burn(uwep, TRUE);
			}
			else if(uswapwep && uswapwep->oartifact==ART_PEN_OF_THE_VOID){
				uswapwep->ovar1 = 0;
				uswapwep->ovar1 |= u.spiritTineA;
				uswapwep->ovar1 |= u.spiritTineB;
				// if(artifact_light(uswapwep) && !uswapwep->lamplit) begin_burn(uswapwep, FALSE);
				// else if(!artifact_light(uswapwep) && uswapwep->lamplit) end_burn(uswapwep, TRUE);
			}
		}
	}
	if(!u.voidChime){
		while(u.spirit[0] && u.spiritT[0] < moves) unbind(u.spirit[0],0);
		if(u.spiritTineB && u.spiritTineTB < moves) unbind(u.spiritTineB,0);
		if(u.spiritTineA && u.spiritTineTA < moves) unbind(u.spiritTineA,0);
		if(u.spirit[QUEST_SPIRIT] && u.spiritT[QUEST_SPIRIT] < moves) 
			unbind(u.spirit[QUEST_SPIRIT],0);
		if(u.spirit[ALIGN_SPIRIT] && u.spiritT[ALIGN_SPIRIT] < moves) 
			unbind(u.spirit[ALIGN_SPIRIT],0);
		if(u.spirit[CROWN_SPIRIT] && u.spiritT[CROWN_SPIRIT] < moves) 
			unbind(u.spirit[CROWN_SPIRIT],0);
		if(u.spirit[GPREM_SPIRIT] && u.spiritT[GPREM_SPIRIT] < moves) 
			unbind(u.spirit[GPREM_SPIRIT],0);
		//if(u.spirit[OUTER_SPIRIT] && u.spiritT[OUTER_SPIRIT] < moves) 
			//unbind(u.spirit[OUTER_SPIRIT]); Numina does not time out
	}
	if(u.sealsActive || u.specialSealsActive){
		int remaining;
		for(i=0;i<OUTER_SPIRIT;i++){
			remaining = 0;
			if(u.spiritT[i] > moves + 625) continue;
			else if(u.spiritT[i] <= moves) continue;
			else if(u.spiritT[i] == moves + 625) remaining = 1;
			else if(u.spiritT[i] == moves + 125) remaining = 2;
			else if(u.spiritT[i] == moves + 25) remaining = 3;
			else if(u.spiritT[i] == moves + 5) remaining = 4;
			else if(u.spiritT[i] == moves + 1) remaining = 5;
			if(remaining){
				int j;
				if(u.spirit[i]&SEAL_SPECIAL){
					for(j=0;j<32;j++){
						if(((u.spirit[i]&(~SEAL_SPECIAL)) >> j) == 1L){
							Your("link with %s is %s.", sealNames[j+(QUEST_SPIRITS-FIRST_SEAL)], spiritFadeTerms[remaining-1]);
						}
					}
				} else {
					for(j=0;j<32;j++){
						if((u.spirit[i] >> j) == 1L){
							Your("link with %s is %s.", sealNames[j], spiritFadeTerms[remaining-1]);
						}
					}
				}
			}
		}
	}
	if(Strangled){
		if(Breathless);//Do nothing
		else if(u.divetimer > 1) u.divetimer--;
		else {
			killer_format = KILLED_BY;
			killer = (u.uburied) ? "suffocation" : "strangulation";
			done(DIED);
		}
	} else if((u.usubwater || is_3dwater(u.ux,u.uy)) && u.divetimer > 0 && !Breathless && !amphibious(youracedata)){
		u.divetimer--;
		if(u.divetimer==3) You("are running short on air.");
		if(u.divetimer==1) You("are running out of air!");
	} else if (!u.usubwater && !u.ustuck){ /* limited duration dive, 2 turns to 6 turns naturally, 8 turns with magic */ 
		if(u.divetimer < (ACURR(A_CON))/3) u.divetimer++;
		else if(u.divetimer > (ACURR(A_CON))/3) u.divetimer--;
	}

	if(u.divetimer<=0){
		You("can't hold your breath any longer.");
		if((!Swimming && !Amphibious && is_pool(u.ux,u.uy, FALSE)) || is_3dwater(u.ux,u.uy)) drown();
		u.usubwater = 0;
		vision_full_recalc = 1;
		vision_recalc(2);	/* unsee old position */
		doredraw();
	}
#ifdef STEED
	if (u.ugallop) {
	    if (--u.ugallop == 0L && u.usteed)
	    	pline("%s stops galloping.", Monnam(u.usteed));
	}
#endif

	for(upp = u.uprops; upp < u.uprops+SIZE(u.uprops); upp++){
		if((youracedata == &mons[PM_SHOGGOTH] || youracedata == &mons[PM_PRIEST_OF_GHAUNADAUR]) && upp - u.uprops == BLINDED
			&&  upp->intrinsic & TIMEOUT
		){
			upp->intrinsic &= ~TIMEOUT;
			upp->intrinsic++;
			You("form new eyes.");
		}
	    if((upp->intrinsic & TIMEOUT) && !(--upp->intrinsic & TIMEOUT)) {
		switch(upp - u.uprops){
		case FIRE_RES:
			You_feel("warmer!");
			if(HFire_resistance){
				You(Hallucination ? "still be chillin', tho'." :
					"still feel a bit chill, though.");
			}
		break;
		case SLEEP_RES:
			You_feel("tired!");
			if(HSleep_resistance){
				You_feel("wide awake the next moment, though.");
			}
		break;
		case COLD_RES:
			You_feel("cooler!");
			if(HCold_resistance){
				You("still feel full of hot air, though.");
			}
		break;
		case SHOCK_RES:
			You_feel("conductive!");
			if(HShock_resistance){
				pline("...But only a bit.");
			}
		break;
		case ACID_RES:
			Your("skin feels more sensitive!");
			if(HAcid_resistance){
				pline("...But only a bit.");
			}
		break;
		case DRAIN_RES:
			You_feel("less energetic!");
			if(HDrain_resistance){
				pline("...But only a bit.");
			}
		break;
		case DISPLACED:
			if(Hallucination){
				You("calm down");
				if(Displaced){
					pline("...But only a bit.");
				}
			}
			if(!Displaced) Your("outline ceases shimmering.");
			else Your("outline becomes more distinct.");
		break;
		case STONED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = "killed by petrification";
			}
			done(STONING);
			break;
		case GOLDED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				/* leaving killer_format would make it
				   "petrified by petrification" */
				killer_format = NO_KILLER_PREFIX;
				killer = "killed by turning to gold";
			}
			done(GOLDING);
			break;
		case SLIMED:
			if (delayed_killer && !killer) {
				killer = delayed_killer;
				delayed_killer = 0;
			}
			if (!killer) {
				killer_format = NO_KILLER_PREFIX;
				killer = "turned into green slime";
			}
			done(TURNED_SLIME);
			break;
		case VOMITING:
			make_vomiting(0L, TRUE);
			break;
		case SICK:
			You("die from your illness.");
			killer_format = KILLED_BY_AN;
			killer = u.usick_cause;
			if ((m_idx = name_to_mon(killer)) >= LOW_PM) {
			    if (type_is_pname(&mons[m_idx])) {
				killer_format = KILLED_BY;
			    } else if (mons[m_idx].geno & G_UNIQ) {
				killer = the(killer);
				Strcpy(u.usick_cause, killer);
				killer_format = KILLED_BY;
			    }
			}
			u.usick_type = 0;
			done(POISONING);
			break;
		case FAST:
			if (!Very_fast)
				You_feel("yourself slowing down%s.",
							Fast ? " a bit" : "");
			break;
		case CONFUSION:
			HConfusion = 1; /* So make_confused works properly */
			make_confused(0L, TRUE);
			stop_occupation();
			break;
		case STUNNED:
			HStun = 1;
			make_stunned(0L, TRUE);
			stop_occupation();
			break;
		case BLINDED:
			Blinded = 1;
			make_blinded(0L, TRUE);
			stop_occupation();
			break;
		case INVIS:
			newsym(u.ux,u.uy);
			if (!Invis && !BInvis && !Blind) {
			    You(!See_invisible(u.ux,u.uy) ?
				    "are no longer invisible." :
				    "can no longer see through yourself.");
			    stop_occupation();
			}
			break;
		case SEE_INVIS:
			set_mimic_blocking(); /* do special mimic handling */
			see_monsters();		/* make invis mons appear */
			newsym(u.ux,u.uy);	/* make self appear */
			stop_occupation();
			break;
		case WOUNDED_LEGS:
			heal_legs();
			stop_occupation();
			break;
		case HALLUC:
			HHallucination = 1;
			(void) make_hallucinated(0L, TRUE, 0L);
			stop_occupation();
			break;
		case SLEEPING:
			if (unconscious() || Sleep_resistance)
				HSleeping += rnd(100);
			else if (Sleeping) {
				You("fall asleep.");
				sleeptime = rnd(20);
				fall_asleep(-sleeptime, TRUE);
				HSleeping += sleeptime + rnd(100);
			}
			break;
		case LEVITATION:
			(void) float_down(I_SPECIAL|TIMEOUT, 0L);
			break;
		case STRANGLED:
			if(!Breathless && !Strangled) Your("throat opens up!");
		break;
		case FUMBLING:
			/* call this only when a move took place.  */
			/* otherwise handle fumbling msgs locally. */
			if (u.umoved && !Levitation) {
			    slip_or_trip();
			    nomul(-2, "fumbling");
			    nomovemsg = "";
			    /* The more you are carrying the more likely you
			     * are to make noise when you fumble.  Adjustments
			     * to this number must be thoroughly play tested.
			     */
			    if ((inv_weight() > -500)) {
				You("make a lot of noise!");
				wake_nearby();
			    }
			}
			/* from outside means slippery ice; don't reset
			   counter if that's the only fumble reason */
			HFumbling &= ~FROMOUTSIDE;
			if (Fumbling)
			    HFumbling += rnd(20);
			break;
		case DETECT_MONSTERS:
			see_monsters();
			break;
		case NULLMAGIC:
			pline("The shimmering film around your body pops!");
		break;
		}
		}
	}

	run_timers();
}

#endif /* OVL0 */
#ifdef OVL1

void
fall_asleep(how_long, wakeup_msg)
int how_long;
boolean wakeup_msg;
{
	if(u.sealsActive&SEAL_HUGINN_MUNINN){
		unbind(SEAL_HUGINN_MUNINN,TRUE);
		return; //expel and end
	}
	stop_occupation();
	nomul(how_long, "sleeping");
	/* generally don't notice sounds while sleeping */
	if (wakeup_msg && multi == how_long) {
	    /* caller can follow with a direct call to Hear_again() if
	       there's a need to override this when wakeup_msg is true */
	    flags.soundok = 0;
	    afternmv = Hear_again;	/* this won't give any messages */
	}
	/* early wakeup from combat won't be possible until next monster turn */
	u.usleep = monstermoves;
	nomovemsg = wakeup_msg ? "You wake up." : You_can_move_again;
}

//#ifdef FIREARMS
/* Attach an explosion timeout to a given explosive device */
void
attach_bomb_blow_timeout(bomb, fuse, yours)
struct obj *bomb;
int fuse;
boolean yours;
{
	long expiretime;	

	if (bomb->cursed && !rn2(2)) return; /* doesn't arm if not armed */

	/* Now if you play with other people's property... */
	if (yours && ((!carried(bomb) && costly_spot(bomb->ox, bomb->oy) &&
		!bomb->no_charge) || bomb->unpaid)) {
	    verbalize("You play with it, you pay for it!");
	    bill_dummy_object(bomb);
	}

	expiretime = stop_timer(BOMB_BLOW, (genericptr_t) bomb);
	if (expiretime > 0L) fuse = fuse - (expiretime - monstermoves);
	bomb->yours = yours;
	bomb->oarmed = TRUE;

	(void) start_timer((long)fuse, TIMER_OBJECT, BOMB_BLOW, (genericptr_t)bomb);
}

/* timer callback routine: detonate the explosives */
void
bomb_blow(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *bomb;
	xchar x,y;
	boolean silent, underwater;
	struct monst *mtmp = (struct monst *)0;

	bomb = (struct obj *) arg;

	silent = (timeout != monstermoves);     /* exploded while away */

	if (get_obj_location(bomb, &x, &y, BURIED_TOO | CONTAINED_TOO)) {
		switch(bomb->where) {		
		    case OBJ_MINVENT:
		    	mtmp = bomb->ocarry;
			if (bomb == MON_WEP(mtmp)) {
			    bomb->owornmask &= ~W_WEP;
			    MON_NOWEP(mtmp);
			}
			if (!silent) {
			    if (canseemon(mtmp))
				You("see %s engulfed in an explosion!", mon_nam(mtmp));
			}
		    	mtmp->mhp -= d(2,5);
			if(mtmp->mhp < 1) {
				if(!bomb->yours) 
					monkilled(mtmp, 
						  (silent ? "" : "explosion"),
						  AD_PHYS);
				else xkilled(mtmp, !silent);
			}
			break;
		    case OBJ_INVENT:
		    	/* This shouldn't be silent! */
			pline("Something explodes inside your knapsack!");
			if (bomb == uwep) {
			    uwepgone();
			    stop_occupation();
			} else if (bomb == uswapwep) {
			    uswapwepgone();
			    stop_occupation();
			} else if (bomb == uquiver) {
			    uqwepgone();
			    stop_occupation();
			}
		    	losehp(d(2,5), "carrying live explosives", KILLED_BY);
		    	break;
		    case OBJ_FLOOR:
			underwater = is_pool(x, y, FALSE);
			if (!silent) {
			    if (x == u.ux && y == u.uy) {
				if (underwater && (Flying || Levitation))
				    pline_The("water boils beneath you.");
				else if (underwater && Wwalking)
				    pline_The("water erupts around you.");
				else pline("A bomb explodes under your %s!",
				  makeplural(body_part(FOOT)));
			    } else if (cansee(x, y))
				You(underwater ?
				    "see a plume of water shoot up." :
				    "see a bomb explode.");
			}
			if (underwater && (Flying || Levitation || Wwalking)) {
			    if (Wwalking && x == u.ux && y == u.uy) {
				struct trap trap;
				trap.ntrap = NULL;
				trap.tx = x;
				trap.ty = y;
				trap.launch.x = -1;
				trap.launch.y = -1;
				trap.ttyp = RUST_TRAP;
				trap.tseen = 0;
				trap.once = 0;
				trap.madeby_u = 0;
				trap.dst.dnum = -1;
				trap.dst.dlevel = -1;
				dotrap(&trap, 0);
			    }
			    goto free_bomb;
			}
		    	break;
		    default:	/* Buried, contained, etc. */
			if (!silent)
			    You_hear("a muffled explosion.");
			goto free_bomb;
			break;
		}
		grenade_explode(bomb, x, y, bomb->yours, silent ? 2 : 0);
		return;
	} /* Migrating grenades "blow up in midair" */

free_bomb:
	obj_extract_self(bomb);
	obfree(bomb, (struct obj *)0);
}
//#endif

/* Attach an egg hatch timeout to the given egg. */
void
attach_egg_hatch_timeout(egg)
struct obj *egg;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, (genericptr_t) egg);

	/*
	 * Decide if and when to hatch the egg.  The old hatch_it() code tried
	 * once a turn from age 151 to 200 (inclusive), hatching if it rolled
	 * a number x, 1<=x<=age, where x>150.  This yields a chance of
	 * hatching > 99.9993%.  Mimic that here.
	 */
	for (i = (MAX_EGG_HATCH_TIME-50)+1; i <= MAX_EGG_HATCH_TIME; i++)
	    if (rnd(i) > 150) {
		/* egg will hatch */
		(void) start_timer((long)i, TIMER_OBJECT,
						HATCH_EGG, (genericptr_t)egg);
		break;
	    }
}

/* prevent an egg from ever hatching */
void
kill_egg(egg)
struct obj *egg;
{
	/* stop previous timer, if any */
	(void) stop_timer(HATCH_EGG, (genericptr_t) egg);
}

/* timer callback routine: hatch the given egg */
void
hatch_egg(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *egg;
	struct monst *mon, *mon2;
	coord cc;
	xchar x, y;
	boolean yours, silent, knows_egg = FALSE;
	boolean cansee_hatchspot = FALSE;
	int i, mnum, hatchcount = 0;

	egg = (struct obj *) arg;
	/* sterilized while waiting */
	if (egg->corpsenm == NON_PM) return;

	mon = mon2 = (struct monst *)0;
	mnum = big_to_little(egg->corpsenm);
	/* The identity of one's father is learned, not innate */
	yours = (egg->spe || (!flags.female && carried(egg) && !rn2(2)));
	silent = (timeout != monstermoves);	/* hatched while away */

	/* only can hatch when in INVENT, FLOOR, MINVENT */
	if (get_obj_location(egg, &x, &y, 0)) {
	    hatchcount = rnd((int)egg->quan);
	    cansee_hatchspot = cansee(x, y) && !silent;
	    if (!(mons[mnum].geno & G_UNIQ) &&
		   !(mvitals[mnum].mvflags & G_GONE && !In_quest(&u.uz))) {
		for (i = hatchcount; i > 0; i--) {
		    if (!enexto(&cc, x, y, &mons[mnum]) ||
			 !(mon = makemon(&mons[mnum], cc.x, cc.y, NO_MINVENT)))
			break;
		    /* tame if your own egg hatches while you're on the
		       same dungeon level, or any dragon egg (or metroid) which hatches
		       while it's in your inventory */
		    if ((yours && !silent) ||
			(carried(egg) && mon->data->mlet == S_DRAGON) ||
			(carried(egg) && mon->data == &mons[PM_BABY_METROID]) ) { //metroid egg
			if ((mon2 = tamedog(mon, (struct obj *)0)) != 0) {
			    mon = mon2;
			    if (carried(egg) && mon->data->mlet != S_DRAGON)
				mon->mtame = 20;
			}
		    }
		    if (mvitals[mnum].mvflags & G_EXTINCT && !In_quest(&u.uz))
			break;	/* just made last one */
		    mon2 = mon;	/* in case makemon() fails on 2nd egg */
		}
		if (!mon) mon = mon2;
		hatchcount -= i;
		egg->quan -= (long)hatchcount;
	    }
	}
#if 0
	/*
	 * We could possibly hatch while migrating, but the code isn't
	 * set up for it...
	 */
	else if (obj->where == OBJ_MIGRATING) {
	    /*
	    We can do several things.  The first ones that come to
	    mind are:

	    + Create the hatched monster then place it on the migrating
	      mons list.  This is tough because all makemon() is made
	      to place the monster as well.    Makemon() also doesn't
	      lend itself well to splitting off a "not yet placed"
	      subroutine.

	    + Mark the egg as hatched, then place the monster when we
	      place the migrating objects.

	    + Or just kill any egg which gets sent to another level.
	      Falling is the usual reason such transportation occurs.
	    */
	    cansee_hatchspot = FALSE;
	    mon = ???
	    }
#endif

	if (mon) {
	    char monnambuf[BUFSZ], carriedby[BUFSZ];
	    boolean siblings = (hatchcount > 1), redraw = FALSE;

	    if (cansee_hatchspot) {
		Sprintf(monnambuf, "%s%s",
			siblings ? "some " : "",
			siblings ?
			makeplural(m_monnam(mon)) : an(m_monnam(mon)));
		/* we don't learn the egg type here because learning
		   an egg type requires either seeing the egg hatch
		   or being familiar with the egg already,
		   as well as being able to see the resulting
		   monster, checked below
		*/
	    }
	    switch (egg->where) {
		case OBJ_INVENT:
		    knows_egg = TRUE; /* true even if you are blind */
		    if (!cansee_hatchspot)
			You_feel("%s %s from your pack!", something,
			    locomotion(mon->data, "drop"));
		    else
			You("see %s %s out of your pack!",
			    monnambuf, locomotion(mon->data, "drop"));
		    if (yours) {
			pline("%s cries sound like \"%s%s\"",
			    siblings ? "Their" : "Its",
			    flags.female ? "mommy" : "daddy",
			    egg->spe ? "." : "?");
		    } else if (mon->data->mlet == S_DRAGON) {
			verbalize("Gleep!");		/* Mything eggs :-) */
		    }
		    break;

		case OBJ_FLOOR:
		    if (cansee_hatchspot) {
			knows_egg = TRUE;
			You("see %s hatch.", monnambuf);
			redraw = TRUE;	/* update egg's map location */
		    }
		    break;

		case OBJ_MINVENT:
		    if (cansee_hatchspot) {
			/* egg carring monster might be invisible */
			if (canseemon(egg->ocarry)) {
			    Sprintf(carriedby, "%s pack",
				     s_suffix(a_monnam(egg->ocarry)));
			    knows_egg = TRUE;
			}
			else if (is_pool(mon->mx, mon->my, FALSE))
			    Strcpy(carriedby, "empty water");
			else
			    Strcpy(carriedby, "thin air");
			You("see %s %s out of %s!", monnambuf,
			    locomotion(mon->data, "drop"), carriedby);
		    }
		    break;
#if 0
		case OBJ_MIGRATING:
		    break;
#endif
		default:
		    impossible("egg hatched where? (%d)", (int)egg->where);
		    break;
	    }

	    if (cansee_hatchspot && knows_egg)
		learn_egg_type(mnum);

	    if (egg->quan > 0) {
		/* still some eggs left */
		attach_egg_hatch_timeout(egg);
		if (egg->timed) {
		    /* replace ordinary egg timeout with a short one */
		    (void) stop_timer(HATCH_EGG, (genericptr_t)egg);
		    (void) start_timer((long)rnd(12), TIMER_OBJECT,
					HATCH_EGG, (genericptr_t)egg);
		}
	    } else if (carried(egg)) {
		useup(egg);
	    } else {
		/* free egg here because we use it above */
		obj_extract_self(egg);
		obfree(egg, (struct obj *)0);
	    }
	    if (redraw) newsym(x, y);
	}
}

/* Learn to recognize eggs of the given type. */
void
learn_egg_type(mnum)
int mnum;
{
	/* baby monsters hatch from grown-up eggs */
	mnum = little_to_big(mnum, rn2(2));
	mvitals[mnum].mvflags |= MV_KNOWS_EGG;
	/* we might have just learned about other eggs being carried */
	update_inventory();
}

/* Attach a fig_transform timeout to the given figurine. */
void
attach_fig_transform_timeout(figurine)
struct obj *figurine;
{
	int i;

	/* stop previous timer, if any */
	(void) stop_timer(FIG_TRANSFORM, (genericptr_t) figurine);

	/*
	 * Decide when to transform the figurine.
	 */
	i = rnd(9000) + 200;
	/* figurine will transform */
	(void) start_timer((long)i, TIMER_OBJECT,
				FIG_TRANSFORM, (genericptr_t)figurine);
}

/* give a fumble message */
STATIC_OVL void
slip_or_trip()
{
	struct obj *otmp = vobj_at(u.ux, u.uy);
	const char *what, *pronoun;
	char buf[BUFSZ];
	boolean on_foot = TRUE;
#ifdef STEED
	if (u.usteed) on_foot = FALSE;
#endif

	if (otmp && on_foot && !u.uinwater && is_pool(u.ux, u.uy, FALSE)) otmp = 0;

	if (otmp && on_foot) {		/* trip over something in particular */
	    /*
		If there is only one item, it will have just been named
		during the move, so refer to by via pronoun; otherwise,
		if the top item has been or can be seen, refer to it by
		name; if not, look for rocks to trip over; trip over
		anonymous "something" if there aren't any rocks.
	     */
	    pronoun = otmp->quan == 1L ? "it" : Hallucination ? "they" : "them";
	    what = !otmp->nexthere ? pronoun :
		  (otmp->dknown || !Blind) ? doname(otmp) :
		  ((otmp = sobj_at(ROCK, u.ux, u.uy)) == 0 ? something :
		  (otmp->quan == 1L ? "a rock" : "some rocks"));
	    if (Hallucination) {
		what = strcpy(buf, what);
		buf[0] = highc(buf[0]);
		pline("Egads!  %s bite%s your %s!",
			what, (!otmp || otmp->quan == 1L) ? "s" : "",
			body_part(FOOT));
	    } else {
		You("trip over %s.", what);
	    }
	} else if (rn2(3) && is_ice(u.ux, u.uy)) {
	    pline("%s %s%s on the ice.",
#ifdef STEED
		u.usteed ? upstart(x_monnam(u.usteed,
				u.usteed->mnamelth ? ARTICLE_NONE : ARTICLE_THE,
				(char *)0, SUPPRESS_SADDLE, FALSE)) :
#endif
		"You", rn2(2) ? "slip" : "slide", on_foot ? "" : "s");
	} else {
	    if (on_foot) {
		switch (rn2(4)) {
		  case 1:
			You("trip over your own %s.", Hallucination ?
				"elbow" : makeplural(body_part(FOOT)));
			break;
		  case 2:
			You("slip %s.", Hallucination ?
				"on a banana peel" : "and nearly fall");
			break;
		  case 3:
			You("flounder.");
			break;
		  default:
			You("stumble.");
			break;
		}
	    }
#ifdef STEED
	    else {
		switch (rn2(4)) {
		  case 1:
			Your("%s slip out of the stirrups.", makeplural(body_part(FOOT)));
			break;
		  case 2:
			You("let go of the reins.");
			break;
		  case 3:
			You("bang into the saddle-horn.");
			break;
		  default:
			You("slide to one side of the saddle.");
			break;
		}
		dismount_steed(DISMOUNT_FELL);
	    }
#endif
	}
}

/* Print a lamp flicker message with tailer. */
STATIC_OVL void
see_lamp_flicker(obj, tailer)
struct obj *obj;
const char *tailer;
{
	switch (obj->where) {
	    case OBJ_INVENT:
	    case OBJ_MINVENT:
		pline("%s flickers%s.", Yname2(obj), tailer);
		break;
	    case OBJ_FLOOR:
		You("see %s flicker%s.", an(xname(obj)), tailer);
		break;
	}
}

/* Print a dimming message for brass lanterns. */
STATIC_OVL void
lantern_message(obj)
struct obj *obj;
{
	/* from adventure */
	switch (obj->where) {
	    case OBJ_INVENT:
		Your("lantern is getting dim.");
		if (Hallucination)
		    pline("Batteries have not been invented yet.");
		break;
	    case OBJ_FLOOR:
		You("see a lantern getting dim.");
		break;
	    case OBJ_MINVENT:
		pline("%s lantern is getting dim.",
		    s_suffix(Monnam(obj->ocarry)));
		break;
	}
}

/*
 * Timeout callback for for objects that are burning. E.g. lamps, candles.
 * See begin_burn() for meanings of obj->age and obj->spe.
 */
void
burn_object(arg, timeout)
genericptr_t arg;
long timeout;
{
	struct obj *obj = (struct obj *) arg;
	boolean canseeit, many, menorah, need_newsym;
	xchar x, y;
	char whose[BUFSZ];

	menorah = obj->otyp == CANDELABRUM_OF_INVOCATION;
	many = menorah ? obj->spe > 1 : obj->quan > 1L;

	/* timeout while away */
	if (timeout != monstermoves && (obj->where != OBJ_MINVENT || (
				(!is_dwarf(obj->ocarry->data) || obj->otyp != DWARVISH_HELM)
				&& (!is_gnome(obj->ocarry->data) || obj->otyp != GNOMISH_POINTY_HAT)
				&& (!is_szcultist(obj->ocarry->data) || !(obj->otyp == TORCH || obj->otyp == SHADOWLANDER_S_TORCH))
	))) {
	    long how_long = monstermoves - timeout;

		if (how_long >= obj->age){
			obj->age = 0;
			end_burn(obj, FALSE);
			if (menorah) {
			    obj->spe = 0;	/* no more candles */
			} else if (Is_candle(obj) || obj->otyp == POT_OIL 
				|| obj->otyp == SHADOWLANDER_S_TORCH || obj->otyp == TORCH
				|| obj->otyp == SUNROD
			) {
			    /* get rid of candles and burning oil potions */
			    obj_extract_self(obj);
			    obfree(obj, (struct obj *)0);
			    obj = (struct obj *) 0;
//#ifdef FIREARMS
		} else if (obj->otyp == STICK_OF_DYNAMITE) {
			bomb_blow((genericptr_t) obj, timeout);
			return;
//#endif
			}
		} else {
			obj->age -= how_long;
		    begin_burn(obj, TRUE);
	    }
	    return;
	}

	/* only interested in INVENT, FLOOR, and MINVENT */
	if (get_obj_location(obj, &x, &y, 0)) {
	    canseeit = !Blind && cansee(x, y);
	    /* set up `whose[]' to be "Your" or "Fred's" or "The goblin's" */
	    (void) Shk_Your(whose, obj);
	} else {
	    canseeit = FALSE;
	}
	need_newsym = FALSE;

	if(obj->oartifact == ART_HOLY_MOONLIGHT_SWORD){	
	        if ((obj->where == OBJ_FLOOR) || 
		    (obj->where == OBJ_MINVENT && 
		    	(!MON_WEP(obj->ocarry) || MON_WEP(obj->ocarry) != obj)) ||
		    (obj->where == OBJ_INVENT &&
		    	((!uwep || uwep != obj) &&
		    	 (!u.twoweap || !uswapwep || obj != uswapwep))))
	            lightsaber_deactivate(obj, FALSE);
			if (obj && obj->age && obj->lamplit) /* might be deactivated */
				begin_burn(obj, TRUE);
	} else switch (obj->otyp) {
	    case POT_OIL:
		    /* this should only be called when we run out */
		    if (canseeit) {
			switch (obj->where) {
			    case OBJ_INVENT:
			    case OBJ_MINVENT:
				pline("%s potion of oil has burnt away.",
				    whose);
				break;
			    case OBJ_FLOOR:
				You("see a burning potion of oil go out.");
				need_newsym = TRUE;
				break;
			}
		    }
		    end_burn(obj, FALSE);	/* turn off light source */
		    obj_extract_self(obj);
		    obfree(obj, (struct obj *)0);
		    obj = (struct obj *) 0;
		    break;

   	    case DWARVISH_HELM:
	    case BRASS_LANTERN:
	    case OIL_LAMP:
		switch((int)obj->age) {
		    case 150:
		    case 100:
		    case 50:
			if (canseeit) {
			    if (obj->otyp == BRASS_LANTERN 
				|| obj->otyp == DWARVISH_HELM)
				lantern_message(obj);
			    else
				see_lamp_flicker(obj,
				    obj->age == 50L ? " considerably" : "");
			}
			//Dwarvish lamps don't go out in monster inventories
			if(obj->where == OBJ_MINVENT && 
				is_dwarf(obj->ocarry->data) &&
				obj->otyp == DWARVISH_HELM) obj->age = (long) rn1(250,250);
			break;

		    case 25:
			if (canseeit) {
			    if (obj->otyp == BRASS_LANTERN 
				|| obj->otyp == DWARVISH_HELM)
				lantern_message(obj);
			    else {
				switch (obj->where) {
				    case OBJ_MINVENT:
				    case OBJ_INVENT:
					pline("%s %s seems about to go out.",
					    whose, xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s about to go out.",
					    an(xname(obj)));
					break;
				}
			    }
			}
			//Dwarvish lamps don't go out in monster inventories
			if(obj->where == OBJ_MINVENT && 
				is_dwarf(obj->ocarry->data) &&
				obj->otyp == DWARVISH_HELM) obj->age = (long) rn1(50,25);
			break;

		    case 0:
			/* even if blind you'll know if holding it */
			if (canseeit || obj->where == OBJ_INVENT) {
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
			    if (obj->otyp == BRASS_LANTERN 
				|| obj->otyp == DWARVISH_HELM)
					pline("%s lantern has run out of power.",
					    whose);
				    else
					pline("%s %s has gone out.",
					    whose, xname(obj));
				    break;
				case OBJ_FLOOR:
			    if (obj->otyp == BRASS_LANTERN 
				|| obj->otyp == DWARVISH_HELM)
					You("see a lantern run out of power.");
				    else
					You("see %s go out.",
					    an(xname(obj)));
				    break;
			    }
			}
			end_burn(obj, FALSE);
			break;

		    default:
			/*
			 * Someone added fuel to the lamp while it was
			 * lit.  Just fall through and let begin burn
			 * handle the new age.
			 */
			break;
		}

		if (obj->age)
		    begin_burn(obj, TRUE);

		break;

	    case CANDELABRUM_OF_INVOCATION:
	    case TALLOW_CANDLE:
	    case WAX_CANDLE:
		case GNOMISH_POINTY_HAT:
		switch (obj->age) {
		    case 75:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s %scandle%s getting short.",
					whose,
					menorah ? "candelabrum's " : "",
					many ? "s are" : " is");
				    break;
				case OBJ_FLOOR:
				    You("see %scandle%s getting short.",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s" : "");
				    break;
			    }
			break;

		    case 15:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline(
					"%s %scandle%s flame%s flicker%s low!",
					    whose,
					    menorah ? "candelabrum's " : "",
					    many ? "s'" : "'s",
					    many ? "s" : "",
					    many ? "" : "s");
				    break;
				case OBJ_FLOOR:
				    You("see %scandle%s flame%s flicker low!",
					    menorah ? "a candelabrum's " :
						many ? "some " : "a ",
					    many ? "s'" : "'s",
					    many ? "s" : "");
				    break;
			    }
			if(obj->where == OBJ_MINVENT &&
				is_gnome(obj->ocarry->data) &&
				obj->otyp == GNOMISH_POINTY_HAT){
				obj->age = (long) rn1(50,15);
			}
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
			    if (menorah) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s candelabrum's flame%s.",
					    whose,
					    many ? "s die" : " dies");
					break;
				    case OBJ_FLOOR:
					You("see a candelabrum's flame%s die.",
						many ? "s" : "");
					break;
				}
				}else if(obj->otyp == GNOMISH_POINTY_HAT){
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s pointy hat's candle dies.",
					    whose);
					break;
				    case OBJ_FLOOR:
						You("see a pointy hat's candle die.");
					break;
				}
			    } else {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s %s consumed!",
					    whose,
					    xname(obj),
					    many ? "are" : "is");
					break;
				    case OBJ_FLOOR:
					/*
					You see some wax candles consumed!
					You see a wax candle consumed!
					*/
					You("see %s%s consumed!",
					    many ? "some " : "",
					    many ? xname(obj):an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ?
					(many ? "They shriek!" :
						"It shrieks!") :
					Blind ? "" :
					    (many ? "Their flames die." :
						    "Its flame dies."));
			    }
			}
			end_burn(obj, FALSE);

			if (menorah) {
			    obj->spe = 0;
			} else if(obj->otyp != GNOMISH_POINTY_HAT){
			    obj_extract_self(obj);
			    obfree(obj, (struct obj *)0);
			    obj = (struct obj *) 0;
			}
			break;
		}
		
		if (obj && obj->age)
		    begin_burn(obj, TRUE);

		break;

		case TORCH:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s torch is burning down.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch burning down.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s torch's flame flickers low!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch's flame flicker low!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its flame dies.");
			}
			end_burn(obj, FALSE);

			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		}
		
		if (obj && obj->age){
			if(obj->where == OBJ_MINVENT &&
				is_szcultist(obj->ocarry->data)
			){
				obj->age = (long) rn1(150,150);
			}
			end_burn(obj, FALSE);
		    begin_burn(obj, FALSE);
		}
		break;
		case SUNROD:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s sunrod is fading.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a sunrod fading.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s sunrod's light flickers faintly!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a sunrod's light flicker faintly!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its faint light dies.");
			}
			end_burn(obj, FALSE);

			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		}
		
		if (obj && obj->age){
			end_burn(obj, FALSE);
		    begin_burn(obj, FALSE);
		}
		break;
		case SHADOWLANDER_S_TORCH:
		switch (obj->age) {
		    case 150:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_INVENT:
				case OBJ_MINVENT:
				    pline("%s torch is burning down.", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch burning down.");
				    break;
			    }
			break;

		    case 50:
			if (canseeit)
			    switch (obj->where) {
				case OBJ_MINVENT:
				case OBJ_INVENT:
				    pline("%s torch's shadowy flame flickers low!", whose);
				    break;
				case OBJ_FLOOR:
				    You("see a torch's shadowy flame flicker low!");
				    break;
			    }
			break;

		    case 0:
			/* we know even if blind and in our inventory */
			if (canseeit || obj->where == OBJ_INVENT) {
				switch (obj->where) {
				    case OBJ_INVENT:
				    case OBJ_MINVENT:
					pline("%s %s is consumed!",
					    whose,
					    xname(obj));
					break;
				    case OBJ_FLOOR:
					You("see %s consumed!", an(xname(obj)));
					need_newsym = TRUE;
					break;
				}

				/* post message */
				pline(Hallucination ? 
						"It shrieks!" :
					Blind ? "" :
					    "Its shadowy flame dies.");
			}
			end_burn(obj, FALSE);

			obj_extract_self(obj);
			obfree(obj, (struct obj *)0);
			obj = (struct obj *) 0;
		}
		
		if (obj && obj->age){
			if(obj->where == OBJ_MINVENT &&
				is_szcultist(obj->ocarry->data)
			){
				obj->age = (long) rn1(150,150);
			}
			end_burn(obj, FALSE);
		    begin_burn(obj, FALSE);
		}
		break;

	    case DOUBLE_LIGHTSABER:
	    	if (obj->altmode && obj->cursed && !rn2(125)) {
		    obj->altmode = FALSE;
		    pline("%s %s reverts to single blade mode!",
			    whose, xname(obj));
	    	}
	    case LIGHTSABER: 
	    case BEAMSWORD:
	        /* Callback is checked every 1 turns - 
	        	lightsaber automatically deactivates if not wielded */
	        if ((obj->cursed && !rn2(250)) ||
	            (obj->where == OBJ_FLOOR) || 
		    (obj->where == OBJ_MINVENT && 
		    	(!MON_WEP(obj->ocarry) || MON_WEP(obj->ocarry) != obj)) ||
		    (obj->where == OBJ_INVENT &&
		    	((!uwep || uwep != obj) &&
		    	 (!u.twoweap || !uswapwep || obj != uswapwep))))
	            lightsaber_deactivate(obj, FALSE);
			if(obj->age <= 0){
				obj->age = 0;//From hitting targets
				lightsaber_deactivate(obj, FALSE);
			}
			if (obj && obj->age && obj->lamplit) /* might be deactivated */
				begin_burn(obj, TRUE);
		break;
//#ifdef FIREARMS
	    case STICK_OF_DYNAMITE:
		end_burn(obj, FALSE);
		bomb_blow((genericptr_t) obj, timeout);
		return;
//#endif
	    default:
		impossible("burn_object: unexpeced obj %s", xname(obj));
		break;
	}
	if (need_newsym) newsym(x, y);
}

/* lightsabers deactivate when they hit the ground/not wielded */
/* assumes caller checks for correct conditions */
void
lightsaber_deactivate (obj, timer_attached)
	struct obj *obj;
	boolean timer_attached;
{
	xchar x,y;
	char whose[BUFSZ];

	(void) Shk_Your(whose, obj);
	obj->lamplit = 0; //temp turn it off so that it prints the material instead of the color
	if (get_obj_location(obj, &x, &y, 0)) {
	    if (cansee(x, y)) {
		switch (obj->where) {
			case OBJ_INVENT:
			case OBJ_MINVENT:
			    pline("%s %s deactivates.",whose, xname(obj));
			    break;
			case OBJ_FLOOR:
			    You("see %s deactivate.", an(xname(obj)));
			    break;
		}
	    } else {
			if(obj->oartifact != ART_HOLY_MOONLIGHT_SWORD)
				You_hear("a lightsaber deactivate.");
	    }
	}
	obj->lamplit = 1; //turn back on for proper deactivation
	// if (obj->otyp == DOUBLE_LIGHTSABER)
	obj->altmode = FALSE;
	if ((obj == uwep) || (u.twoweap && obj != uswapwep)) unweapon = TRUE;
	end_burn(obj, timer_attached);
}
/*
 * Start a burn timeout on the given object. If not "already lit" then
 * create a light source for the vision system.  There had better not
 * be a burn already running on the object.
 *
 * Magic lamps stay lit as long as there's a genie inside, so don't start
 * a timer.
 *
 * Burn rules:
 *	potions of oil, lamps & candles:
 *		age = # of turns of fuel left
 *		spe = <unused>
 *
 *	magic lamps:
 *		age = <unused>
 *		spe = 0 not lightable, 1 lightable forever
 *
 *	candelabrum:
 *		age = # of turns of fuel left
 *		spe = # of candles
 *
 * Once the burn begins, the age will be set to the amount of fuel
 * remaining _once_the_burn_finishes_.  If the burn is terminated
 * early then fuel is added back.
 *
 * This use of age differs from the use of age for corpses and eggs.
 * For the latter items, age is when the object was created, so we
 * know when it becomes "bad".
 *
 * This is a "silent" routine - it should not print anything out.
 */
void
begin_burn(obj, already_lit)
	struct obj *obj;
	boolean already_lit;
{
	int radius;
	long turns = 0;
	boolean do_timer = TRUE;
	
	radius = 3;

	if (obj->age == 0 && 
		obj->otyp != MAGIC_LAMP && 
		obj->otyp != CANDLE_OF_INVOCATION &&
		obj->otyp != POT_STARLIGHT && 
		obj->otyp != CHUNK_OF_FOSSIL_DARK && 
		!artifact_light(obj) && 
		!arti_light(obj) && 
		obj->oartifact != ART_HOLY_MOONLIGHT_SWORD &&
		obj->oartifact != ART_ATMA_WEAPON
	) return;
	
	if(obj->oartifact == ART_HOLY_MOONLIGHT_SWORD){
		turns = 1;
		radius = 1;
	} else switch (obj->otyp) {
	    case MAGIC_LAMP:
		obj->lamplit = 1;
		do_timer = FALSE;
		break;
	    case CANDLE_OF_INVOCATION:
		obj->lamplit = 1;
		do_timer = FALSE;
		break;
	    case POT_STARLIGHT:
		obj->lamplit = 1;
		do_timer = FALSE;
		radius = 2;
		break;
	    case CHUNK_OF_FOSSIL_DARK:
		obj->lamplit = 1;
		do_timer = FALSE;
		radius = 2;
		break;
	    case DOUBLE_LIGHTSABER:
	    case LIGHTSABER:
	    case BEAMSWORD:
	    	if (obj->altmode && obj->age > 1) 
				obj->age--; /* Double power usage */
			if(obj->oartifact == ART_ATMA_WEAPON){
				if(obj->age <= 0){
					if(Drain_resistance) return;
					losexp("life force drain",TRUE,TRUE,TRUE);
					obj->cursed = FALSE;
					obj->age = 150000;
				} else if(!Drain_resistance) obj->age++;
			}
			turns = 1;
			if(obj->cobj && obj->cobj->oartifact == obj->oartifact && arti_light(obj->cobj)) 
				radius = (obj->cobj->blessed ? 3 : (obj->cobj->cursed ? 1 : 2));
    	    else if(obj->cobj && obj->cobj->otyp == JET) /*'mirrored' lightsaber*/ 
				radius = 0;
    	    else radius = 1;
		break;

	    case POT_OIL:
		turns = obj->age;
		radius = 1;	/* very dim light */
		break;

	    case STICK_OF_DYNAMITE:
		turns = obj->age;
		radius = 1;     /* very dim light */
		break;
		
		case GNOMISH_POINTY_HAT:
			turns = obj->age;
			radius = 2;
			if (obj->age > 75L)
				turns = obj->age - 75L;
			else if (obj->age > 15L)
				turns = obj->age - 15L;
		break;
		
	    case DWARVISH_HELM:
	    case BRASS_LANTERN:
	    case OIL_LAMP:
		/* magic times are 150, 100, 50, 25, and 0 */
		if (obj->age > 150L)
			turns = obj->age - 150L;
		else if (obj->age > 100L)
			turns = obj->age - 100L;
		else if (obj->age > 50L)
			turns = obj->age - 50L;
		else if (obj->age > 25L)
			turns = obj->age - 25L;
		else
			turns = obj->age;
		break;
		
	    case CANDELABRUM_OF_INVOCATION:
	    case TALLOW_CANDLE:
	    case WAX_CANDLE:
		/* magic times are 75, 15, and 0 */
		if (obj->age > 75L)
		    turns = obj->age - 75L;
		else if (obj->age > 15L)
		    turns = obj->age - 15L;
		else
		    turns = obj->age;
		radius = candle_light_range(obj);
		break;

	    case SUNROD:
		/* magic times are 150, 50, and 0 */
		if (obj->age > 150L){
		    turns = obj->age - 150L;
			radius = 5;
		} else if (obj->age > 50L){
		    turns = obj->age - 50L;
			radius = 3;
		} else {
		    turns = obj->age;
			radius = 1;
		}
		break;
	    case TORCH:
		/* magic times are 150, 50, and 0 */
		if (obj->age > 150L){
		    turns = obj->age - 150L;
			radius = 4;
		} else if (obj->age > 50L){
		    turns = obj->age - 50L;
			radius = 3;
		} else {
		    turns = obj->age;
			radius = 2;
		}
		break;
	    case SHADOWLANDER_S_TORCH:
		/* magic times are 150, 50, and 0 */
		if (obj->age > 150L){
		    turns = obj->age - 150L;
			radius = 4;
		} else if (obj->age > 50L){
		    turns = obj->age - 50L;
			radius = 3;
		} else {
		    turns = obj->age;
			radius = 2;
		}
		break;

	    default:
                /* [ALI] Support artifact light sources */
        if (artifact_light(obj) || arti_light(obj)) {
		    obj->lamplit = 1;
		    do_timer = FALSE;
			radius = (obj->blessed ? 3 : (obj->cursed ? 1 : 2));
		} else {
		    impossible("begin burn: unexpected %s", xname(obj));
		    turns = obj->age;
		}
		break;
	}

	if (do_timer) {
	    if (start_timer(turns, TIMER_OBJECT,
					BURN_OBJECT, (genericptr_t)obj)) {
		obj->lamplit = 1;
		obj->age -= turns;
		if (carried(obj) && !already_lit)
		    update_inventory();
	    } else {
		obj->lamplit = 0;
	    }
	} else {
	    if (carried(obj) && !already_lit)
		update_inventory();
	}

	if (obj->lamplit && !already_lit) {
	    xchar x, y;

	    if (get_obj_location(obj, &x, &y, CONTAINED_TOO|BURIED_TOO))
		new_light_source(x, y, radius, LS_OBJECT, (genericptr_t) obj);
	    else
		impossible("begin_burn: can't get obj position");
	}
}

/*
 * Stop a burn timeout on the given object if timer attached.  Darken
 * light source.
 */
void
end_burn(obj, timer_attached)
	struct obj *obj;
	boolean timer_attached;
{
	if (!obj->lamplit) {
	    impossible("end_burn: obj %s not lit", xname(obj));
	    return;
	}

	if (obj->otyp == MAGIC_LAMP 
		|| obj->otyp == CANDLE_OF_INVOCATION
		|| obj->otyp == POT_STARLIGHT
		|| obj->otyp == CHUNK_OF_FOSSIL_DARK
		|| artifact_light(obj)
	) timer_attached = FALSE;

	if (!timer_attached) {
	    /* [DS] Cleanup explicitly, since timer cleanup won't happen */
	    del_light_source(LS_OBJECT, (genericptr_t)obj, FALSE);
	    obj->lamplit = 0;
	    if (obj->where == OBJ_INVENT)
		update_inventory();
	} else if (!stop_timer(BURN_OBJECT, (genericptr_t) obj))
	    impossible("end_burn: obj %s not timed!", xname(obj));
	// if(darksight(youracedata) || catsight(youracedata)) doredraw();
	// So many different vision systems, just do a redraw
	doredraw();
}

#endif /* OVL1 */
#ifdef OVL0

/*
 * Cleanup a burning object if timer stopped.
 */
static void
cleanup_burn(arg, expire_time)
    genericptr_t arg;
    long expire_time;
{
    struct obj *obj = (struct obj *)arg;
    if (!obj->lamplit) {
	impossible("cleanup_burn: obj %s not lit", xname(obj));
	return;
    }

    del_light_source(LS_OBJECT, arg, FALSE);

    /* restore unused time */
    obj->age += expire_time - monstermoves;

    obj->lamplit = 0;

    if (obj->where == OBJ_INVENT)
	update_inventory();
}

#endif /* OVL0 */
#ifdef OVL1

void
do_storms()
{
    int nstrike;
    register int x, y;
    int dirx, diry;
    int count;

    /* no lightning if not the air level or too often, even then */
    if(!Is_airlevel(&u.uz) || rn2(8))
	return;

    /* the number of strikes is 8-log2(nstrike) */
    for(nstrike = rnd(64); nstrike <= 64; nstrike *= 2) {
	count = 0;
	do {
	    x = rnd(COLNO-1);
	    y = rn2(ROWNO);
	} while (++count < 100 && levl[x][y].typ != CLOUD);

	if(count < 100) {
	    dirx = rn2(3) - 1;
	    diry = rn2(3) - 1;
	    if(dirx != 0 || diry != 0)
		buzz(AD_ELEC, SPBOOK_CLASS, FALSE,/* "monster" LIGHTNING spell */
		     8, x, y, dirx, diry,0,0);
	}
    }

    if(levl[u.ux][u.uy].typ == CLOUD) {
	/* inside a cloud during a thunder storm is deafening */
	pline("Kaboom!!!  Boom!!  Boom!!");
	if(!(u.uinvulnerable || u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20)) {
	    stop_occupation();
	    nomul(-3, "hiding from thunderstorm");
	}
    } else
	You_hear("a rumbling noise.");
}
#endif /* OVL1 */


#ifdef OVL0
/* ------------------------------------------------------------------------- */
/*
 * Generic Timeout Functions.
 *
 * Interface:
 *
 * General:
 *	boolean start_timer(long timeout,short kind,short func_index,
 *							genericptr_t arg)
 *		Start a timer of kind 'kind' that will expire at time
 *		monstermoves+'timeout'.  Call the function at 'func_index'
 *		in the timeout table using argument 'arg'.  Return TRUE if
 *		a timer was started.  This places the timer on a list ordered
 *		"sooner" to "later".  If an object, increment the object's
 *		timer count.
 *
 *	long stop_timer(short func_index, genericptr_t arg)
 *		Stop a timer specified by the (func_index, arg) pair.  This
 *		assumes that such a pair is unique.  Return the time the
 *		timer would have gone off.  If no timer is found, return 0.
 *		If an object, decrement the object's timer count.
 *
 *	void run_timers(void)
 *		Call timers that have timed out.
 *
 *
 * Save/Restore:
 *	void save_timers(int fd, int mode, int range)
 *		Save all timers of range 'range'.  Range is either global
 *		or local.  Global timers follow game play, local timers
 *		are saved with a level.  Object and monster timers are
 *		saved using their respective id's instead of pointers.
 *
 *	void restore_timers(int fd, int range, boolean ghostly, long adjust)
 *		Restore timers of range 'range'.  If from a ghost pile,
 *		adjust the timeout by 'adjust'.  The object and monster
 *		ids are not restored until later.
 *
 *	void relink_timers(boolean ghostly)
 *		Relink all object and monster timers that had been saved
 *		using their object's or monster's id number.
 *
 * Object Specific:
 *	void obj_move_timers(struct obj *src, struct obj *dest)
 *		Reassign all timers from src to dest.
 *
 *	void obj_split_timers(struct obj *src, struct obj *dest)
 *		Duplicate all timers assigned to src and attach them to dest.
 *
 *	void obj_stop_timers(struct obj *obj)
 *		Stop all timers attached to obj.
 */

#ifdef WIZARD
STATIC_DCL const char *FDECL(kind_name, (SHORT_P));
STATIC_DCL void FDECL(print_queue, (winid, timer_element *));
#endif
STATIC_DCL void FDECL(insert_timer, (timer_element *));
STATIC_DCL timer_element *FDECL(remove_timer, (timer_element **, SHORT_P,
								genericptr_t));
STATIC_DCL void FDECL(write_timer, (int, timer_element *));
STATIC_DCL boolean FDECL(mon_is_local, (struct monst *));
STATIC_DCL boolean FDECL(timer_is_local, (timer_element *));
STATIC_DCL int FDECL(maybe_write_timer, (int, int, BOOLEAN_P));

/* ordered timer list */
static timer_element *timer_base;		/* "active" */
static unsigned long timer_id = 1;

/* If defined, then include names when printing out the timer queue */
#define VERBOSE_TIMER

typedef struct {
    timeout_proc f, cleanup;
#ifdef VERBOSE_TIMER
    const char *name;
# define TTAB(a, b, c) {a,b,c}
#else
# define TTAB(a, b, c) {a,b}
#endif
} ttable;

/* table of timeout functions */
static const ttable timeout_funcs[NUM_TIME_FUNCS] = {
    TTAB(rot_organic,	(timeout_proc)0,	"rot_organic"),
    TTAB(rot_corpse,	(timeout_proc)0,	"rot_corpse"),
    TTAB(moldy_corpse,	(timeout_proc)0,	"moldy_corpse"),
    TTAB(revive_mon,	(timeout_proc)0,	"revive_mon"),
    TTAB(burn_object,	cleanup_burn,		"burn_object"),
    TTAB(hatch_egg,		(timeout_proc)0,	"hatch_egg"),
    TTAB(fig_transform,	(timeout_proc)0,	"fig_transform"),
    TTAB(light_damage,	(timeout_proc)0,	"light_damage"),
    TTAB(slimy_corpse,	(timeout_proc)0,	"slimy_corpse"),
	TTAB(zombie_corpse,	(timeout_proc)0,	"zombie_corpse"),
    TTAB(shady_corpse,	(timeout_proc)0,	"shady_corpse"),
//#ifdef FIREARMS
    TTAB(bomb_blow,     (timeout_proc)0,	"bomb_blow")
//#endif
};
#undef TTAB


#if defined(WIZARD)

STATIC_OVL const char *
kind_name(kind)
    short kind;
{
    switch (kind) {
	case TIMER_LEVEL: return "level";
	case TIMER_GLOBAL: return "global";
	case TIMER_OBJECT: return "object";
	case TIMER_MONSTER: return "monster";
    }
    return "unknown";
}

STATIC_OVL void
print_queue(win, base)
    winid win;
    timer_element *base;
{
    timer_element *curr;
    char buf[BUFSZ], arg_address[20];

    if (!base) {
	putstr(win, 0, "<empty>");
    } else {
	putstr(win, 0, "timeout  id   kind   call");
	for (curr = base; curr; curr = curr->next) {
#ifdef VERBOSE_TIMER
	    Sprintf(buf, " %4ld   %4ld  %-6s %s(%s)",
		curr->timeout, curr->tid, kind_name(curr->kind),
		timeout_funcs[curr->func_index].name,
		fmt_ptr((genericptr_t)curr->arg, arg_address));
#else
	    Sprintf(buf, " %4ld   %4ld  %-6s #%d(%s)",
		curr->timeout, curr->tid, kind_name(curr->kind),
		curr->func_index,
		fmt_ptr((genericptr_t)curr->arg, arg_address));
#endif
	    putstr(win, 0, buf);
	}
    }
}

int
wiz_timeout_queue()
{
    winid win;
    char buf[BUFSZ];

    win = create_nhwindow(NHW_MENU);	/* corner text window */
    if (win == WIN_ERR) return 0;

    Sprintf(buf, "Current time = %ld.", monstermoves);
    putstr(win, 0, buf);
    putstr(win, 0, "");
    putstr(win, 0, "Active timeout queue:");
    putstr(win, 0, "");
    print_queue(win, timer_base);

    display_nhwindow(win, FALSE);
    destroy_nhwindow(win);

    return 0;
}

void
timer_sanity_check()
{
    timer_element *curr;
    char obj_address[20];

    /* this should be much more complete */
    for (curr = timer_base; curr; curr = curr->next)
	if (curr->kind == TIMER_OBJECT) {
	    struct obj *obj = (struct obj *) curr->arg;
	    if (obj->timed == 0) {
		pline("timer sanity: untimed obj %s, timer %ld",
		      fmt_ptr((genericptr_t)obj, obj_address), curr->tid);
	    }
	}
}

#endif /* WIZARD */


/*
 * Pick off timeout elements from the global queue and call their functions.
 * Do this until their time is less than or equal to the move count.
 */
void
run_timers()
{
    timer_element *curr;

    /*
     * Always use the first element.  Elements may be added or deleted at
     * any time.  The list is ordered, we are done when the first element
     * is in the future.
     */
    while (timer_base && timer_base->timeout <= monstermoves) {
	curr = timer_base;
	timer_base = curr->next;

	if (curr->kind == TIMER_OBJECT) ((struct obj *)(curr->arg))->timed--;
	(*timeout_funcs[curr->func_index].f)(curr->arg, curr->timeout);
	free((genericptr_t) curr);
    }
}


/*
 * Start a timer.  Return TRUE if successful.
 */
boolean
start_timer(when, kind, func_index, arg)
long when;
short kind;
short func_index;
genericptr_t arg;
{
    timer_element *gnu;

    if (func_index < 0 || func_index >= NUM_TIME_FUNCS)
	panic("start_timer");

    gnu = (timer_element *) alloc(sizeof(timer_element));
    gnu->next = 0;
    gnu->tid = timer_id++;
    gnu->timeout = monstermoves + when;
    gnu->kind = kind;
    gnu->needs_fixup = 0;
    gnu->func_index = func_index;
    gnu->arg = arg;
    insert_timer(gnu);

    if (kind == TIMER_OBJECT)	/* increment object's timed count */
	((struct obj *)arg)->timed++;

    /* should check for duplicates and fail if any */
    return TRUE;
}


/*
 * Remove the timer from the current list and free it up.  Return the time
 * it would have gone off, 0 if not found.
 */
long
stop_timer(func_index, arg)
short func_index;
genericptr_t arg;
{
    timer_element *doomed;
    long timeout;

    doomed = remove_timer(&timer_base, func_index, arg);

    if (doomed) {
	timeout = doomed->timeout;
	if (doomed->kind == TIMER_OBJECT)
	    ((struct obj *)arg)->timed--;
	if (timeout_funcs[doomed->func_index].cleanup)
	    (*timeout_funcs[doomed->func_index].cleanup)(arg, timeout);
	free((genericptr_t) doomed);
	return timeout;
    }
    return 0;
}


/*
 * Move all object timers from src to dest, leaving src untimed.
 */
void
obj_move_timers(src, dest)
    struct obj *src, *dest;
{
    int count;
    timer_element *curr;

    for (count = 0, curr = timer_base; curr; curr = curr->next)
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)src) {
	    curr->arg = (genericptr_t) dest;
	    dest->timed++;
	    count++;
	}
    if (count != src->timed)
	panic("obj_move_timers");
    src->timed = 0;
}


/*
 * Find all object timers and duplicate them for the new object "dest".
 */
void
obj_split_timers(src, dest)
    struct obj *src, *dest;
{
    timer_element *curr, *next_timer=0;

    for (curr = timer_base; curr; curr = next_timer) {
	next_timer = curr->next;	/* things may be inserted */
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)src) {
	    (void) start_timer(curr->timeout-monstermoves, TIMER_OBJECT,
					curr->func_index, (genericptr_t)dest);
	}
    }
}


/*
 * Stop all timers attached to this object.  We can get away with this because
 * all object pointers are unique.
 */
void
obj_stop_timers(obj)
    struct obj *obj;
{
    timer_element *curr, *prev, *next_timer=0;

    for (prev = 0, curr = timer_base; curr; curr = next_timer) {
	next_timer = curr->next;
	if (curr->kind == TIMER_OBJECT && curr->arg == (genericptr_t)obj) {
	    if (prev)
		prev->next = curr->next;
	    else
		timer_base = curr->next;
	    if (timeout_funcs[curr->func_index].cleanup)
		(*timeout_funcs[curr->func_index].cleanup)(curr->arg,
			curr->timeout);
	    free((genericptr_t) curr);
	} else {
	    prev = curr;
	}
    }
    obj->timed = 0;
}


/* Insert timer into the global queue */
STATIC_OVL void
insert_timer(gnu)
    timer_element *gnu;
{
    timer_element *curr, *prev;

    for (prev = 0, curr = timer_base; curr; prev = curr, curr = curr->next)
	if (curr->timeout >= gnu->timeout) break;

    gnu->next = curr;
    if (prev)
	prev->next = gnu;
    else
	timer_base = gnu;
}


STATIC_OVL timer_element *
remove_timer(base, func_index, arg)
timer_element **base;
short func_index;
genericptr_t arg;
{
    timer_element *prev, *curr;

    for (prev = 0, curr = *base; curr; prev = curr, curr = curr->next)
	if (curr->func_index == func_index && curr->arg == arg) break;

    if (curr) {
	if (prev)
	    prev->next = curr->next;
	else
	    *base = curr->next;
    }

    return curr;
}


STATIC_OVL void
write_timer(fd, timer)
    int fd;
    timer_element *timer;
{
    genericptr_t arg_save;

    switch (timer->kind) {
	case TIMER_GLOBAL:
	case TIMER_LEVEL:
	    /* assume no pointers in arg */
	    bwrite(fd, (genericptr_t) timer, sizeof(timer_element));
	    break;

	case TIMER_OBJECT:
	    if (timer->needs_fixup)
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
	    else {
		/* replace object pointer with id */
		arg_save = timer->arg;
		timer->arg = (genericptr_t)(intptr_t)((struct obj *)timer->arg)->o_id;
		timer->needs_fixup = 1;
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
		timer->arg = arg_save;
		timer->needs_fixup = 0;
	    }
	    break;

	case TIMER_MONSTER:
	    if (timer->needs_fixup)
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
	    else {
		/* replace monster pointer with id */
		arg_save = timer->arg;
		timer->arg = (genericptr_t)(intptr_t)((struct monst *)timer->arg)->m_id;
		timer->needs_fixup = 1;
		bwrite(fd, (genericptr_t)timer, sizeof(timer_element));
		timer->arg = arg_save;
		timer->needs_fixup = 0;
	    }
	    break;

	default:
	    panic("write_timer");
	    break;
    }
}


/*
 * Return TRUE if the object will stay on the level when the level is
 * saved.
 */
boolean
obj_is_local(obj)
    struct obj *obj;
{
    switch (obj->where) {
	case OBJ_INVENT:
	case OBJ_MAGIC_CHEST:
	case OBJ_MIGRATING:	return FALSE;
	case OBJ_FLOOR:
	case OBJ_BURIED:	return TRUE;
	case OBJ_CONTAINED:	return obj_is_local(obj->ocontainer);
	case OBJ_MINVENT:	return mon_is_local(obj->ocarry);
    }
    panic("obj_is_local");
    return FALSE;
}


/*
 * Return TRUE if the given monster will stay on the level when the
 * level is saved.
 */
STATIC_OVL boolean
mon_is_local(mon)
struct monst *mon;
{
    struct monst *curr;

    for (curr = migrating_mons; curr; curr = curr->nmon)
	if (curr == mon) return FALSE;
    /* `mydogs' is used during level changes, never saved and restored */
    for (curr = mydogs; curr; curr = curr->nmon)
	if (curr == mon) return FALSE;
    return TRUE;
}


/*
 * Return TRUE if the timer is attached to something that will stay on the
 * level when the level is saved.
 */
STATIC_OVL boolean
timer_is_local(timer)
    timer_element *timer;
{
    switch (timer->kind) {
	case TIMER_LEVEL:	return TRUE;
	case TIMER_GLOBAL:	return FALSE;
	case TIMER_OBJECT:	return obj_is_local((struct obj *)timer->arg);
	case TIMER_MONSTER:	return mon_is_local((struct monst *)timer->arg);
    }
    panic("timer_is_local");
    return FALSE;
}


/*
 * Part of the save routine.  Count up the number of timers that would
 * be written.  If write_it is true, actually write the timer.
 */
STATIC_OVL int
maybe_write_timer(fd, range, write_it)
    int fd, range;
    boolean write_it;
{
    int count = 0;
    timer_element *curr;

    for (curr = timer_base; curr; curr = curr->next) {
	if (range == RANGE_GLOBAL) {
	    /* global timers */

	    if (!timer_is_local(curr)) {
		count++;
		if (write_it) write_timer(fd, curr);
	    }

	} else {
	    /* local timers */

	    if (timer_is_local(curr)) {
		count++;
		if (write_it) write_timer(fd, curr);
	    }

	}
    }

    return count;
}


/*
 * Save part of the timer list.  The parameter 'range' specifies either
 * global or level timers to save.  The timer ID is saved with the global
 * timers.
 *
 * Global range:
 *		+ timeouts that follow the hero (global)
 *		+ timeouts that follow obj & monst that are migrating
 *
 * Level range:
 *		+ timeouts that are level specific (e.g. storms)
 *		+ timeouts that stay with the level (obj & monst)
 */
void
save_timers(fd, mode, range)
    int fd, mode, range;
{
    timer_element *curr, *prev, *next_timer=0;
    int count;

    if (perform_bwrite(mode)) {
	if (range == RANGE_GLOBAL)
	    bwrite(fd, (genericptr_t) &timer_id, sizeof(timer_id));

	count = maybe_write_timer(fd, range, FALSE);
	bwrite(fd, (genericptr_t) &count, sizeof count);
	(void) maybe_write_timer(fd, range, TRUE);
    }

    if (release_data(mode)) {
	for (prev = 0, curr = timer_base; curr; curr = next_timer) {
	    next_timer = curr->next;	/* in case curr is removed */

	    if ( !(!!(range == RANGE_LEVEL) ^ !!timer_is_local(curr)) ) {
		if (prev)
		    prev->next = curr->next;
		else
		    timer_base = curr->next;
		free((genericptr_t) curr);
		/* prev stays the same */
	    } else {
		prev = curr;
	    }
	}
    }
}


/*
 * Pull in the structures from disk, but don't recalculate the object and
 * monster pointers.
 */
void
restore_timers(fd, range, ghostly, adjust)
    int fd, range;
    boolean ghostly;	/* restoring from a ghost level */
    long adjust;	/* how much to adjust timeout */
{
    int count;
    timer_element *curr;

    if (range == RANGE_GLOBAL)
	mread(fd, (genericptr_t) &timer_id, sizeof timer_id);

    /* restore elements */
    mread(fd, (genericptr_t) &count, sizeof count);
    while (count-- > 0) {
	curr = (timer_element *) alloc(sizeof(timer_element));
	mread(fd, (genericptr_t) curr, sizeof(timer_element));
	if (ghostly)
	    curr->timeout += adjust;
	insert_timer(curr);
    }
}


/* reset all timers that are marked for reseting */
void
relink_timers(ghostly)
    boolean ghostly;
{
    timer_element *curr;
    unsigned nid;

    for (curr = timer_base; curr; curr = curr->next) {
	if (curr->needs_fixup) {
	    if (curr->kind == TIMER_OBJECT) {
		if (ghostly) {
		    if (!lookup_id_mapping((unsigned)(intptr_t)curr->arg, &nid))
			panic("relink_timers 1");
		} else
		    nid = (unsigned)(intptr_t)curr->arg;
		curr->arg = (genericptr_t) find_oid(nid);
		if (!curr->arg) panic("cant find o_id %d", nid);
		curr->needs_fixup = 0;
	    } else if (curr->kind == TIMER_MONSTER) {
		panic("relink_timers: no monster timer implemented");
	    } else
		panic("relink_timers 2");
	}
    }
}

#endif /* OVL0 */

/*timeout.c*/
