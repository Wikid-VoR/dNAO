/*	SCCS Id: @(#)mcastu.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

extern const int monstr[];
extern void demonpet();

extern void you_aggravate(struct monst *);

STATIC_DCL void FDECL(cursetxt,(struct monst *,BOOLEAN_P));
STATIC_DCL int FDECL(choose_magic_spell, (int,int,BOOLEAN_P));
STATIC_DCL int FDECL(choose_clerical_spell, (int,int,BOOLEAN_P));
STATIC_DCL boolean FDECL(is_undirected_spell,(int));
STATIC_DCL boolean FDECL(is_aoe_spell,(int));
STATIC_DCL boolean FDECL(spell_would_be_useless,(struct monst *,int));
STATIC_DCL boolean FDECL(mspell_would_be_useless,(struct monst *,struct monst *,int));
STATIC_DCL boolean FDECL(uspell_would_be_useless,(struct monst *,int));
STATIC_DCL void FDECL(ucast_spell,(struct monst *,struct monst *,int,int));

#ifdef OVL0

/* feedback when frustrated monster couldn't cast a spell */
STATIC_OVL
void
cursetxt(mtmp, undirected)
struct monst *mtmp;
boolean undirected;
{
	if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my)) {
	    const char *point_msg;  /* spellcasting monsters are impolite */

	    if (undirected)
		point_msg = "all around, then curses";
	    else if ((Invis && !perceives(mtmp->data) &&
			(mtmp->mux != u.ux || mtmp->muy != u.uy)) ||
		    (youmonst.m_ap_type == M_AP_OBJECT &&
			youmonst.mappearance == STRANGE_OBJECT) ||
		    u.uundetected)
		point_msg = "and curses in your general direction";
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		point_msg = "and curses at your displaced image";
	    else
		point_msg = "at you, then curses";

	    pline("%s points %s.", Monnam(mtmp), point_msg);
	} else if ((!(moves % 4) || !rn2(4))) {
	    if (flags.soundok) Norep("You hear a mumbled curse.");
	}
}

#endif /* OVL0 */
#ifdef OVLB

/* default spell selection for mages */
STATIC_OVL int
choose_magic_spell(spellval,mid,hostile)
int spellval;
int mid;
boolean hostile;
{
	/* Alternative spell lists: since the alternative lists contain spells that aren't
		yet implemented for m vs m combat, non-hostile monsters always use the vanilla 
		list. Alternate list selection is based on the monster's ID number, which is
		annotated as staying constant.
	*/
	if(!hostile || mid % 10 < 5){
    switch (spellval % 24) {
    case 23:
    case 22:
    return TURN_TO_STONE;
    case 21:
    case 20:
	return DEATH_TOUCH;
    case 19:
    case 18:
	return CLONE_WIZ;
    case 17:
    case 16:
    case 15:
	return SUMMON_MONS;
    case 14:
    case 13:
	return AGGRAVATION;
    case 12:
    case 11:
    case 10:
	return CURSE_ITEMS;
    case 9:
    case 8:
	return DESTRY_ARMR;
    case 7:
    case 6:
	return WEAKEN_YOU;
    case 5:
    case 4:
	return DISAPPEAR;
    case 3:
	return STUN_YOU;
    case 2:
	return HASTE_SELF;
    case 1:
	return CURE_SELF;
    case 0:
    default:
	return PSI_BOLT;
    }
	}else if(mid % 10 < 7){
     switch (spellval % 24) {
   case 23:
    case 22:
    return DRAIN_LIFE;
    case 21:
    case 20:
	return DRAIN_ENERGY;
    case 19:
    case 18:
	return CLONE_WIZ;
    case 17:
    case 16:
    case 15:
	return WEAKEN_STATS;
    case 14:
    case 13:
	return CONFUSE_YOU;
    case 12:
    case 11:
    case 10:
	return DRAIN_LIFE;
    case 9:
    case 8:
	return DESTRY_WEPN;
    case 7:
    case 6:
	return WEAKEN_YOU;
    case 5:
    case 4:
	return DARKNESS;
    case 3:
	return STUN_YOU;
    case 2:
	return MAKE_WEB;
    case 1:
	return CURE_SELF;
    case 0:
    default:
	return PSI_BOLT;
	}
	}else if(mid % 10 < 9){
    switch (spellval % 24) {
    case 23:
    case 22:
    return CURE_SELF;
    case 21:
    case 20:
	return ARROW_RAIN;
    case 19:
    case 18:
	return CLONE_WIZ;
    case 17:
    case 16:
    case 15:
	return SUMMON_MONS;
    case 14:
    case 13:
	return SUMMON_SPHERE;
    case 12:
    case 11:
    case 10:
	return DROP_BOULDER;
    case 9:
    case 8:
	return DESTRY_ARMR;
    case 7:
    case 6:
	return EVIL_EYE;
    case 5:
    case 4:
	return MAKE_VISIBLE;
    case 3:
	return SUMMON_SPHERE;
    case 2:
	return HASTE_SELF;
    case 1:
	return CURE_SELF;
    case 0:
    default:
	return SUMMON_SPHERE;
	}
	}else{
    switch (spellval % 24) {
    case 23:
    case 22:
    case 21:
    case 20:
    case 19:
	return DRAIN_LIFE;
    case 18:
	return CLONE_WIZ;
    case 17:
    case 16:
    case 15:
	return SUMMON_MONS;
    case 14:
    case 13:
	return AGGRAVATION;
    case 12:
    case 11:
    case 10:
	return CURSE_ITEMS;
    case 9:
	return DESTRY_WEPN;
    case 8:
	return DESTRY_ARMR;
    case 7:
    case 6:
	return EVIL_EYE;
    case 5:
	return MAKE_VISIBLE;
    case 4:
	return DISAPPEAR;
    case 3:
	return STUN_YOU;
    case 2:
	return HASTE_SELF;
    case 1:
	return CONFUSE_YOU;
    case 0:
    default:
	return PSI_BOLT;
	}
	}
}

/* default spell selection for priests/monks */
STATIC_OVL int
choose_clerical_spell(spellnum,mid,hostile)
int spellnum;
int mid;
boolean hostile;
{
	/* Alternative spell lists: since the alternative lists contain spells that aren't
		yet implemented for m vs m combat, non-hostile monsters always use the vanilla 
		list. Alternate list slection is based on the monster's ID number, which is
		annotated as staying constant. Priests are divided up into constructive and
		destructive casters (constructives favor heal self, destructives favor 
		inflict wounds). Their spell list is divided into blocks. The order that
		they recieve spells from each block is radomized based on their monster
		ID.
	*/
	if(!hostile){
     switch (spellnum % 18) {
     case 17:
       return PUNISH;
     case 16:
       return SUMMON_ANGEL;
     case 14:
       return PLAGUE;
    case 13:
       return ACID_RAIN;
    case 12:
	return GEYSER;
    case 11:
	return FIRE_PILLAR;
    case 9:
	return LIGHTNING;
    case 8:
       return DRAIN_LIFE;
    case 7:
       return CURSE_ITEMS;
    case 6:
       return INSECTS;
    case 4:
	return BLIND_YOU;
    case 3:
	return PARALYZE;
    case 2:
	return CONFUSE_YOU;
    case 1:
	return OPEN_WOUNDS;
    case 0:
    default:/*5,10,15,18+*/
	return CURE_SELF;
    }
	}else{
		 spellnum = spellnum % 18;
		//case "17"
		if(spellnum == ((mid/100+3)%4)+14) return PUNISH;
		//case "16"
		//case "15"
		//Cure/Inflict
		if(spellnum == ((mid/100+2)%4)+14) return (mid % 2) ? SUMMON_ANGEL : SUMMON_DEVIL;
		//case "14"
		if(spellnum == ((mid/100+0)%4)+14) return PLAGUE;
		//case "13"
		if(spellnum == ((mid+4)%5)+9) return EARTHQUAKE;
		//case "12"
		if(spellnum == ((mid+3)%5)+9) return ( (mid/11) % 2) ? GEYSER : ACID_RAIN;
		//case "11"
		if(spellnum == ((mid+2)%5)+9) return ( (mid/13) % 2) ? FIRE_PILLAR : ICE_STORM;
		//case "10"
		//Cure/Inflict
		//case "9"
		if(spellnum == ((mid+0)%5)+9) return LIGHTNING;
		//case "8"
		if(spellnum == ((mid/10+3)%4)+5) return DRAIN_LIFE;
		//case "7"
		if(spellnum == ((mid/10+2)%4)+5) return ( (mid/3) % 2) ? CURSE_ITEMS : EVIL_EYE;
		//case "6"
		if(spellnum == ((mid/10+1)%4)+5) return INSECTS;
		//case "5"
		//Cure/Inflict
		//case "4"
		if(spellnum == ((mid+2)%3)+2) return BLIND_YOU;
		//case "3"
		if(spellnum == ((mid+1)%3)+2) return PARALYZE;
		//case "2"
		if(spellnum == ((mid+0)%3)+2) return CONFUSE_YOU;
		//case "1"
		if(spellnum == ((mid+1)%2)+0) return ( (mid+1) % 2) ? CURE_SELF : OPEN_WOUNDS;
		//case "0", "5", "10", "15", "18+"
		return (mid % 2) ? CURE_SELF : OPEN_WOUNDS;
	}
}

/* ...but first, check for monster-specific spells */
STATIC_OVL int
choose_magic_special(mtmp, type)
struct monst *mtmp;
unsigned int type;
{
    if (rn2(2)) {
       switch(monsndx(mtmp->data)) {
       case PM_WIZARD_OF_YENDOR:
           return (rn2(4) ? rnd(STRANGLE) :
                 (!rn2(3) ? STRANGLE : !rn2(2) ? CLONE_WIZ : HASTE_SELF));

       case PM_ORCUS:
       case PM_NALZOK:
           if (rn2(2)) return RAISE_DEAD;
		break;
       case PM_DISPATER:
           if (rn2(2)) return (rn2(2) ? TURN_TO_STONE : CURSE_ITEMS);
		break;
       case PM_DEMOGORGON:
           return (!rn2(3) ? HASTE_SELF : rn2(2) ? FILTH : WEAKEN_YOU);
		   
       case PM_LAMASHTU:
			// pline("favored");
			switch(rnd(8)){
				case 1:
					return SUMMON_ANGEL;
				break;
				case 2:
					return SUMMON_DEVIL;
				break;
				case 3:
					return SUMMON_ALIEN;
				break;
				case 4:
					return NIGHTMARE;
				break;
				case 5:
					return FILTH;
				break;
				case 6:
					return CURSE_ITEMS;
				break;
				case 7:
					return DEATH_TOUCH;
				break;
				case 8:
					return EVIL_EYE;
				break;
			}
		break;

       case PM_APPRENTICE:
          if (rn2(3)) return SUMMON_SPHERE;
       /* fallthrough */
       case PM_NEFERET_THE_GREEN:
           return ARROW_RAIN;

       case PM_DARK_ONE:
           return (!rn2(4) ? TURN_TO_STONE : !rn2(3) ? RAISE_DEAD :
                    rn2(2) ? DARKNESS : MAKE_WEB);

       case PM_THOTH_AMON:
           if (!rn2(3)) return NIGHTMARE;
       /* fallthrough */
       case PM_CHROMATIC_DRAGON:
           return (rn2(2) ? DESTRY_WEPN : EARTHQUAKE);

       case PM_PLATINUM_DRAGON:
           return (rn2(2) ? LIGHTNING : FIRE_PILLAR);
			
       case PM_IXOTH:
			return FIRE_PILLAR;
       case PM_NIMUNE:
           if(!rn2(3)) return NIGHTMARE;
		   else if(rn2(2)) return MASS_CURE_CLOSE;
		   else return SLEEP;
	   break;
       case PM_MAHADEVA:
		switch(rn2(10)){
			case 0:
				return MASS_CURE_CLOSE;
			break;
			case 1:
			case 2:
			case 3:
				return ARROW_RAIN;
			break;
			case 4:
			case 5:
			case 6:
				return ICE_STORM;
			break;
			case 7:
			case 8:
			case 9:
				return GEYSER;
			break;
		}
	   break;
       case PM_MOVANIC_DEVA:
		return MASS_CURE_FAR;
	   break;
       case PM_GRAHA_DEVA:
		switch(rn2(15)){
			case 0:
				return OPEN_WOUNDS;
			break;
				return LIGHTNING;
			case 1:
				return FIRE_PILLAR;
			case 2:
				return GEYSER;
			case 3:
				return ACID_RAIN;
			case 4:
				return ICE_STORM;
			case 5:
				return MASS_CURE_CLOSE;
			case 6:
				return MASS_CURE_FAR;
			break;
				return MAKE_VISIBLE;
			case 7:
				return BLIND_YOU;
			case 8:
				return CURSE_ITEMS;
			case 9:
				return INSECTS;
			case 10:
				return SUMMON_ANGEL;
			case 11:
				return AGGRAVATION;
			case 12:
				return PUNISH;
			case 13:
				return EARTHQUAKE;
			case 14:
				return VULNERABILITY;
		}
	   break;
       case PM_SURYA_DEVA:
           return (rn2(2) ? MASS_CURE_CLOSE : FIRE_PILLAR);
	
       case PM_GRAND_MASTER:
       case PM_MASTER_KAEN:
          return (rn2(2) ? WEAKEN_YOU : EARTHQUAKE);

       case PM_MINION_OF_HUHETOTL:
           return (rn2(2) ? CURSE_ITEMS : (rn2(2) ? DESTRY_WEPN : DROP_BOULDER));

       case PM_TITAN:
           return (rn2(2) ? DROP_BOULDER : LIGHTNING);
       case PM_THRONE_ARCHON:
           return (rn2(2) ? SUMMON_ANGEL : LIGHTNING);
       case PM_KI_RIN:
           return FIRE_PILLAR;

       case PM_ARCH_LICH:
           if (!rn2(6)) return TURN_TO_STONE;
       /* fallthrough */
#if 0
       case PM_VAMPIRE_MAGE:
#endif
       case PM_MASTER_LICH:
           if (!rn2(5)) return RAISE_DEAD;
       /* fallthrough */
       case PM_DEMILICH:
           if (!rn2(4)) return DRAIN_LIFE;
       /* fallthrough */
       case PM_LICH:
           if (!rn2(3)) return CURSE_ITEMS;
       /* fallthrough */
       // case PM_NALFESHNEE:
           if (rn2(2)) return (rn2(2) ? DESTRY_ARMR : DESTRY_WEPN);
       /* fallthrough */
       case PM_BARROW_WIGHT:
           return (!rn2(3) ? DARKNESS : (rn2(2) ? MAKE_WEB : SLEEP));

       case PM_BAELNORN:
			switch(rnd(6)){
				case 1: return ACID_RAIN;
				case 2: return CURSE_ITEMS;
				case 3: return MASS_CURE_CLOSE;
				case 4: return SLEEP;
				case 5: return DARKNESS;
				case 6: return CONFUSE_YOU;
			}
	   break;
       case PM_PRIEST_OF_GHAUNADAUR:
           if (rn2(2)) return FIRE_PILLAR;
		   else if(rn2(2)) return MON_FIRAGA;
		break; /*Go on to regular spell list*/

       case PM_GNOMISH_WIZARD:
           if (rn2(2)) return SUMMON_SPHERE;
       }
    }//50% favored spells
	
	//100% favored spells
	switch(monsndx(mtmp->data)) {
	case PM_DWARF_CLERIC:
	case PM_DWARF_QUEEN:
		switch (rnd(4)) {
			case 4:
			return MON_PROTECTION;
			break;
			case 3:
			return MASS_CURE_CLOSE;
			break;
			case 2:
			return MASS_CURE_FAR;
			break;
			case 1:
			return AGGRAVATION;
			break;
		}
	break;
	case PM_WARRIOR_OF_SUNLIGHT:
		switch (rn2(mtmp->m_lev-10)) {
			default:/* 15 -> 19*/
				return LIGHTNING;
			case 14:
			case 13:
			case 12:
			case 11:
			case 10:
				return MON_PROTECTION;
			case 9:
			case 8:
			case 7:
			case 6:
			case 5:
				return MASS_CURE_CLOSE;
			case 4:
			case 3:
			case 2:
			case 1:
			case 0:
				return CURE_SELF;
		}
	break;
	case PM_HALF_STONE_DRAGON:
		switch (rn2(mtmp->m_lev)) {
			default:/* 10 -> 29*/
				return LIGHTNING;
			case 9:
			case 8:
			case 7:
			case 6:
			case 5:
				return MASS_CURE_CLOSE;
			case 4:
			case 3:
			case 2:
			case 1:
			case 0:
				return CURE_SELF;
		}
	break;
	case PM_ELF_LADY:
	case PM_ELVENQUEEN:
		switch (rnd(6)) {
			case 6:
			return CURE_SELF;
			break;
			case 5:
			return MASS_CURE_FAR;
			break;
			case 4:
			return SLEEP;
			break;
			case 3:
			return BLIND_YOU;
			break;
			case 2:
			return CONFUSE_YOU;
			break;
			case 1:
			return DISAPPEAR;
			break;
		}
	break;
	case PM_MINOTAUR_PRIESTESS:
		switch (d(1,5)+8) {
			case 13:
			return GEYSER;
			break;
			case 12:
			return FIRE_PILLAR;
			break;
			case 11:
			return LIGHTNING;
			break;
			case 10:
			case 9:
			return CURSE_ITEMS;
			break;
			default:
			return OPEN_WOUNDS;
			break;
		}
	break;
	case PM_GNOLL_MATRIARCH:
		switch (d(1,10)-4) {
			case 6:
				return BLIND_YOU;
			break;
			case 5:
			case 4:
				return PARALYZE;
			break;
			case 3:
			case 2:
				return CONFUSE_YOU;
			break;
			case 1:
				return MASS_CURE_FAR;
			break;
			case 0:
			default:
				return OPEN_WOUNDS;
			break;
		}
	break;
	case PM_AMM_KAMEREL:
	case PM_HUDOR_KAMEREL:
	case PM_ARA_KAMEREL:
		return OPEN_WOUNDS;
	break;
	case PM_SHARAB_KAMEREL:
		return PSI_BOLT;
	break;
	case PM_PLUMACH_RILMANI:
		return SOLID_FOG;
	break;
	case PM_FERRUMACH_RILMANI:
		if(rn2(4)) return HAIL_FLURY;
		return SOLID_FOG;
	break;
	case PM_STANNUMACH_RILMANI:
		switch(rn2(6)){
			case 0: return ICE_STORM;
			case 1: return SOLID_FOG;
			case 2: return DISAPPEAR;
			case 3: return MAKE_VISIBLE;
			case 4: return MASS_CURE_FAR;
			case 5: return MON_PROTECTION;
		}
	break;
	case PM_CUPRILACH_RILMANI:
		switch(rn2(6)){
			case 0: return DRAIN_LIFE;
			case 1: return ACID_BLAST;
			case 2: return SOLID_FOG;
			case 3: return DISAPPEAR;
			case 4: return MON_POISON_GAS;
			case 5: return MAKE_VISIBLE;
		}
	break;
	case PM_ARGENACH_RILMANI:
		if(rn2(4)) return SILVER_RAYS;
		switch(rn2(4)){
			case 0: return ICE_STORM;
			case 1: return SOLID_FOG;
			case 2: return DISAPPEAR;
			case 3: return MAKE_VISIBLE;
		}
	break;
	case PM_AURUMACH_RILMANI:
		if(rn2(4)) return GOLDEN_WAVE;
		switch(rn2(7)){
			case 0: return ICE_STORM;
			case 1: return ACID_RAIN;
			case 2: return SOLID_FOG;
			case 3: return DISAPPEAR;
			case 4: return MON_POISON_GAS;
			case 5: return MAKE_VISIBLE;
			case 6: return PRISMATIC_SPRAY;
		}
	break;
	case PM_UVUUDAUM:
		switch(rn2(8)){
			case 0: return PSI_BOLT;
			case 1: return MON_WARP;
			case 2: return STUN_YOU;
			case 3: return PARALYZE;
			case 4: return MON_TIME_STOP;
			case 5: return TIME_DUPLICATE;
			case 6: return NAIL_TO_THE_SKY;
			case 7: return PRISMATIC_SPRAY;
		}
	break;
	case PM_GREAT_HIGH_SHAMAN_OF_KURTULMAK:
		return SUMMON_DEVIL; 
	case PM_LICH__THE_FIEND_OF_EARTH:
		if(mtmp->mvar1 > 3) mtmp->mvar1 = 0;
		switch(mtmp->mvar1++){
			case 0: return MON_FLARE;
			case 1: return PARALYZE;
			case 2: return MON_WARP;
			case 3: return DEATH_TOUCH;
		}
	break;
	case PM_KARY__THE_FIEND_OF_FIRE:
		if(mtmp->mvar1 > 7) mtmp->mvar1 = 0;
		switch(mtmp->mvar1++){
			case 0: return MON_FIRAGA;
			case 1: return BLIND_YOU;
			case 2: return MON_FIRAGA;
			case 3: return BLIND_YOU;
			case 4: return MON_FIRAGA;
			case 5: return STUN_YOU;
			case 6: return MON_FIRAGA;
			case 7: return STUN_YOU;
		}
	break;
	case PM_KRAKEN__THE_FIEND_OF_WATER:
		if(rn2(100)<71) return MON_THUNDARA;
		else return BLIND_YOU;
	break;
	case PM_TIAMAT__THE_FIEND_OF_WIND:
		if(rn2(3)){
			if(mtmp->mvar1 > 3) mtmp->mvar1 = 0;
			switch(mtmp->mvar1++){
				case 0: return PLAGUE;
				case 1: return MON_BLIZZARA;
				case 2: return MON_THUNDARA;
				case 3: return MON_FIRA;
			}
		} else {
			if(mtmp->mvar2 > 3) mtmp->mvar1 = 0;
			switch(mtmp->mvar2++){
				case 0: return LIGHTNING;
				case 1: return MON_POISON_GAS;
				case 2: return ICE_STORM;
				case 3: return FIRE_PILLAR;
			}
		}
	break;
	case PM_CHAOS:
		if(rn2(2)){
			if(mtmp->mvar1 > 7) mtmp->mvar1 = 0;
			switch(mtmp->mvar1++){
				case 0: return MON_BLIZZAGA;
				case 1: return WEAKEN_STATS;
				case 2: return MON_THUNDAGA;
				case 3: return CURE_SELF;
				case 4: return HASTE_SELF;
				case 5: return MON_FIRAGA;
				case 6: return ICE_STORM;
				case 7: return MON_FLARE;
			}
		} else {
			if(mtmp->mvar2 > 3) mtmp->mvar1 = 0;
			switch(mtmp->mvar2++){
				case 0: return FIRE_PILLAR;
				case 1: return GEYSER;
				case 2: return MON_POISON_GAS;
				case 3: return EARTHQUAKE;
			}
		}
	break;
	case PM_SHOGGOTH:
		if(!rn2(20)) return SUMMON_MONS; 
		else return 0;
	case PM_VERIER: 
		if(!rn2(3)) return WEAKEN_YOU;
		else return DESTRY_ARMR;
	case PM_CRONE_LILITH:
		switch(rn2(6)){
			case 0:
			case 1:
			case 2:
				return CURSE_ITEMS;
			break;
			case 3:
				return WEAKEN_STATS;
			break;
			case 4:
				return CURE_SELF;
			break;
			case 5:
				return DEATH_TOUCH;
			break;
		}
	case PM_MILITANT_CLERIC:
		switch(rn2(6)){
			case 0:
			case 1:
				return CURE_SELF;
			break;
			case 2:
				return MASS_CURE_FAR;
			break;
			case 3:
				return MASS_CURE_CLOSE;
			break;
			case 4:
				return RECOVER;
			break;
			case 5:
				return FIRE_PILLAR;
			break;
		}
	case PM_ADVENTURING_WIZARD:
		switch(rn2(4)){
			case 0:
			case 1:
				return SLEEP;
			break;
			case 2:
				return MAGIC_MISSILE;
			break;
			case 3:
				return SUMMON_SPHERE;
			break;
		}
	case PM_NALFESHNEE:
		switch(rn2(5)){
			case 0:
				return OPEN_WOUNDS;
			break;
			case 1:
				return CURSE_ITEMS;
			break;
			case 2:
				return LIGHTNING;
			break;
			case 3:
				return FIRE_PILLAR;
			break;
			case 4:
				return PUNISH;
			break;
		}
	break;
	case PM_FAERINAAL:{
		int spelln;
		do spelln = rnd(MON_LASTSPELL);
		while(spelln == CLONE_WIZ);
		return spelln;
	} break;
	case PM_PALE_NIGHT:
		switch(rn2(5)){
			case 0:
				return OPEN_WOUNDS;
			break;
			case 1:
				return WEAKEN_YOU;
			break;
			case 2:
				return PSI_BOLT;
			break;
			case 3:
				return CURSE_ITEMS;
			break;
			case 4:
				return DEATH_TOUCH;
			break;
		}
	break;
	case PM_ASMODEUS:
		switch(rn2(9)){
			case 0:
				return CURE_SELF;
			break;
			case 1:
				return OPEN_WOUNDS;
			break;
			case 2:
				return ACID_RAIN;
			break;
			case 3:
				return LIGHTNING;
			break;
			case 4:
				return FIRE_PILLAR;
			break;
			case 5:
				return GEYSER;
			break;
			case 6:
				return SUMMON_MONS;
			break;
			case 7:
				return PARALYZE;
			break;
			case 8:
				return DEATH_TOUCH;
			break;
		}
	break;
	case PM_ALRUNES:
		switch(rn2(6)){
			case 0:
				return MON_PROTECTION;
			break;
			case 1:
				return MASS_CURE_CLOSE;
			break;
			case 2:
				return MASS_CURE_FAR;
			break;
			case 3:
				return AGGRAVATION;
			break;
			case 4:
				return VULNERABILITY;
			break;
			case 5:
				return EVIL_EYE;
			break;
		}
	break;
	case PM_HATEFUL_WHISPERS:
		switch(rn2(6)){
			case 0:
				return SUMMON_MONS;
			break;
			case 1:
				return CURE_SELF;
			break;
			case 2:
				return SUMMON_DEVIL;
			break;
			case 3:
				return AGGRAVATION;
			break;
			case 4:
				return VULNERABILITY;
			break;
			case 5:
				return EVIL_EYE;
			break;
		}
	break;
	case PM_DOMIEL:
		switch(rn2(7)){
			case 0:
				return MON_PROTECTION;
			break;
			case 1:
				return HASTE_SELF;
			break;
			case 2:
				return MASS_CURE_CLOSE;
			break;
			case 3:
				return MASS_CURE_FAR;
			break;
			case 4:
				return AGGRAVATION;
			break;
			case 5:
				return FIRE_PILLAR;
			break;
			case 6:
				return STUN_YOU;
			break;
		}
	break;
	case PM_ERATHAOL:
		switch(rn2(7)){
			case 0:
				return MON_PROTECTION;
			break;
			case 1:
				return VULNERABILITY;
			break;
			case 2:
				return EVIL_EYE;
			break;
			case 3:
				return BLIND_YOU;
			break;
			case 4:
				return AGGRAVATION;
			break;
			case 5:
				return CONFUSE_YOU;
			break;
			case 6:
				return HASTE_SELF;
			break;
		}
	break;
	case PM_SEALTIEL:
		switch(rn2(7)){
			case 0:
				return MON_PROTECTION;
			break;
			case 1:
				return HASTE_SELF;
			break;
			case 2:
				return MASS_CURE_CLOSE;
			break;
			case 3:
				return MASS_CURE_FAR;
			break;
			case 4:
				return ICE_STORM;
			break;
			case 5:
				return DROP_BOULDER;
			break;
			case 6:
				return TURN_TO_STONE;
			break;
		}
	break;
	case PM_ZAPHKIEL:
		switch(rn2(7)){
			case 0:
				return MON_PROTECTION;
			break;
			case 1:
				return SUMMON_ANGEL;
			break;
			case 2:
				return MASS_CURE_CLOSE;
			break;
			case 3:
				return MASS_CURE_FAR;
			break;
			case 4:
				return AGGRAVATION;
			break;
			case 5:
				return RECOVER;
			break;
			case 6:
				return MASS_HASTE;
			break;
		}
	break;
	}
    if (type == AD_CLRC)
        return choose_clerical_spell(mtmp->m_id == 0 ? (rn2(u.ulevel) * 18 / 30) : rn2(mtmp->m_lev),mtmp->m_id,!(mtmp->mpeaceful));
    return choose_magic_spell(mtmp->m_id == 0 ? (rn2(u.ulevel) * 24 / 30) : rn2(mtmp->m_lev),mtmp->m_id,!(mtmp->mpeaceful));
}

/* return values:
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmu(mtmp, mattk, thinks_it_foundyou, foundyou)
	register struct monst *mtmp;
	register struct attack *mattk;
	boolean thinks_it_foundyou;
	boolean foundyou;
{
	int	dmg, ml = mtmp->m_lev;
	int ret;
	int spellnum = 0, chance;
	int dmd, dmn;
	struct obj *mirror;

	if(mtmp->data->maligntyp < 0 && Is_illregrd(&u.uz)) return 0;
	if(is_derived_undead_mon(mtmp) && mtmp->mfaction != FRACTURED) return 0;
	/* Three cases:
	 * -- monster is attacking you.  Search for a useful spell.
	 * -- monster thinks it's attacking you.  Search for a useful spell,
	 *    without checking for undirected.  If the spell found is directed,
	 *    it fails with cursetxt() and loss of mspec_used.
	 * -- monster isn't trying to attack.  Select a spell once.  Don't keep
	 *    searching; if that spell is not useful (or if it's directed),
	 *    return and do something else. 
	 * Since most spells are directed, this means that a monster that isn't
	 * attacking casts spells only a small portion of the time that an
	 * attacking monster does.
	 */
	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
	    int cnt = 40;
		
		// if(Race_if(PM_DROW) && mtmp->data == &mons[PM_AVATAR_OF_LOLTH] && !Role_if(PM_EXILE) && !mtmp->mpeaceful){
		if(mtmp->data == &mons[PM_AVATAR_OF_LOLTH] && !mtmp->mpeaceful && !strcmp(urole.cgod,"Lolth")){
			u.ugangr[Align2gangr(A_CHAOTIC)]++;
			angrygods(A_CHAOTIC);
			return 1;
		}
        do {
			spellnum = choose_magic_special(mtmp, mattk->adtyp);
			if(!spellnum) return 0; //The monster's spellcasting code aborted the cast.
			/* not trying to attack?  don't allow directed spells */
			if (!thinks_it_foundyou) {
				if (!is_undirected_spell(spellnum) ||
						   spell_would_be_useless(mtmp, spellnum)
				) {
//					if (foundyou)
//						impossible("spellcasting monster found you and doesn't know it?");
					return 0;
				}
				break;
			}
	    } while(--cnt > 0 &&
                   spell_would_be_useless(mtmp, spellnum));
	    if (cnt == 0) return 0;
	}

	/* monster unable to cast spells? */
	if(mtmp->mcan || (mtmp->mspec_used && !nospellcooldowns(mtmp->data)) || !ml || u.uinvulnerable || u.spiritPColdowns[PWR_PHASE_STEP] >= moves+20) {
	    cursetxt(mtmp, is_undirected_spell(spellnum));
	    return(0);
	}

	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && !nospellcooldowns(mtmp->data)) {
	    if(mtmp->data == &mons[PM_HEDROW_WARRIOR]) mtmp->mspec_used = d(4,4);
		else mtmp->mspec_used = 10 - mtmp->m_lev;
	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
	}

	/* monster can cast spells, but is casting a directed spell at the
	   wrong place?  If so, give a message, and return.  Do this *after*
	   penalizing mspec_used. */
	if (!foundyou && thinks_it_foundyou &&
		!is_undirected_spell(spellnum) &&
		!is_aoe_spell(spellnum)) {
	    pline("%s casts a spell at %s!",
		canseemon(mtmp) ? Monnam(mtmp) : "Something",
		levl[mtmp->mux][mtmp->muy].typ == WATER
		    ? "empty water" : "thin air");
	    return(0);
	}

	nomul(0, NULL);
	/* increased spell success rate vs vanilla to make kobold/orc shamans less helpless */
	if(is_kamerel(mtmp->data)){
		/* find mirror */
		for (mirror = mtmp->minvent; mirror; mirror = mirror->nobj)
			if (mirror->otyp == MIRROR && !mirror->cursed)
				break;
	}
	
	if(!is_kamerel(mtmp->data) || !mirror){
		chance = 2;
		if(mtmp->mconf) chance += 8;
		if(u.uz.dnum == neutral_dnum && u.uz.dlevel <= sum_of_all_level.dlevel){
			if(u.uz.dlevel == sum_of_all_level.dlevel) chance -= 1;
			else if(u.uz.dlevel == spire_level.dlevel-1) chance += 10;
			else if(u.uz.dlevel == spire_level.dlevel-2) chance += 8;
			else if(u.uz.dlevel == spire_level.dlevel-3) chance += 6;
			else if(u.uz.dlevel == spire_level.dlevel-4) chance += 4;
			else if(u.uz.dlevel == spire_level.dlevel-5) chance += 2;
		}
		
		if((u.uz.dnum == neutral_dnum && u.uz.dlevel == spire_level.dlevel) || rn2(ml*2) < chance) {	/* fumbled attack */
			if (canseemon(mtmp) && flags.soundok)
			pline_The("air crackles around %s.", mon_nam(mtmp));
			return(0);
		}
	}
	if (canspotmon(mtmp) || !is_undirected_spell(spellnum)) {
	    pline("%s casts a spell%s!",
		  canspotmon(mtmp) ? Monnam(mtmp) : "Something",
		  is_undirected_spell(spellnum) ? "" :
		  (Invisible && !perceives(mtmp->data) && 
		   (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  " at a spot near you" :
		  (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy)) ?
		  " at your displaced image" :
		  " at you");
	}

/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	if (!foundyou) {
		if(is_aoe_spell(spellnum)){
			dmd = 6;
			dmn = min(MAX_BONUS_DICE, ml/3+1);
			if (mattk->damd) dmd = (int)(mattk->damd);
			
			if (mattk->damn) dmn+= (int)(mattk->damn);
			
			if(dmn < 1) dmn = 1;
			
			dmg = d(dmn, dmd);
	    } else{
			 dmg = 0;
		}
	    if (mattk->adtyp != AD_SPEL && mattk->adtyp != AD_CLRC) {
		impossible(
	      "%s casting non-hand-to-hand version of hand-to-hand spell %d?",
			   Monnam(mtmp), mattk->adtyp);
		return(0);
	    }
	} else {
		int dmd = 6, dmn = min(MAX_BONUS_DICE, ml/3+1);
		
		if (dmn > 15) dmn = 15;
		
		if (mattk->damd) dmd = (int)(mattk->damd);
		
		if (mattk->damn) dmn+= (int)(mattk->damn);
		
		if(dmn < 1) dmn = 1;
		
	    dmg = d(dmn, dmd);
	}
	if (Half_spell_damage && !is_aoe_spell(spellnum)) dmg = (dmg+1) / 2;

	ret = 1;

	switch (mattk->adtyp) {
		case AD_OONA:
			switch(u.oonaenergy){
				case AD_ELEC: goto elec_spell;
				case AD_FIRE: goto fire_spell;
				case AD_COLD: goto cold_spell;
				default: pline("Bad Oona spell type?");
			}
		break;
		case AD_RBRE:
			switch(rnd(3)){
				case 1: goto elec_spell;
				case 2: goto fire_spell;
				case 3: goto cold_spell;
			}
		break;
		case AD_SLEE:
		pline("You're enveloped in a puff of gas.");
		if(Sleep_resistance) {
			shieldeff(u.ux, u.uy);
			You("don't feel sleepy!");
		} else {
			fall_asleep(-dmg, TRUE);
		}
		dmg = 0;
		break;
	    case AD_ELEC:
elec_spell:
		pline("Lightning crackles around you.");
		if(Shock_resistance) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			dmg = 0;
		}
		if(!EShock_resistance){
			destroy_item(WAND_CLASS, AD_ELEC);
			if(!rn2(10)) destroy_item(RING_CLASS, AD_ELEC);
		}
		
		stop_occupation();
		break;
	    case AD_FIRE:
fire_spell:
		pline("You're enveloped in flames.");
		if(Fire_resistance) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			dmg = 0;
		}
		if(!EFire_resistance) {
			destroy_item(POTION_CLASS, AD_FIRE);
			if(!rn2(6)) destroy_item(SCROLL_CLASS, AD_FIRE);
			if(!rn2(10)) destroy_item(SPBOOK_CLASS, AD_FIRE);
		}
		burn_away_slime();
		stop_occupation();
		break;
	    case AD_COLD:
cold_spell:
		pline("You're covered in frost.");
		if(Cold_resistance) {
			shieldeff(u.ux, u.uy);
			pline("But you resist the effects.");
			dmg = 0;
		}
		if(!ECold_resistance) destroy_item(POTION_CLASS, AD_COLD);
		stop_occupation();
		break;
	    case AD_MAGM:
		You("are hit by a shower of missiles!");
		if(Antimagic) {
			shieldeff(u.ux, u.uy);
			pline_The("missiles bounce off!");
			dmg = 0;
		} //else dmg = d((int)mtmp->m_lev/2 + 1,6);
		stop_occupation();
		break;
	    case AD_STAR:
		You("are hit by a shower of silver stars!");
		dmg /= 2;
		drain_en(dmg);
		if(hates_silver(youracedata)) dmg += d(dmn,20);
		dmg -= roll_udr(mtmp);
		if(Half_physical_damage) dmg /= 2;
		if(dmg < 1) dmg = 1;
		stop_occupation();
		break;
        default:
	    {
        cast_spell(mtmp, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}
	if(dmg) mdamageu(mtmp, dmg);
	return(ret);
}

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
void
cast_spell(mtmp, dmg, spellnum)
struct monst *mtmp;
int dmg;
int spellnum;
{
    boolean malediction = (mtmp && (mtmp->iswiz || (mtmp->data->msound == MS_NEMESIS && rn2(2))));
    int zap; /* used for ray spells */
    
    if (dmg == 0 && !is_undirected_spell(spellnum)) {
	impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
	return;
    }

    switch (spellnum) {
    case DEATH_TOUCH:
	pline("Oh no, %s's using the touch of death!", mtmp ? mhe(mtmp) : "something");
	if (nonliving(youracedata) || is_demon(youracedata)) {
	    You("seem no deader than before.");
		dmg = 0; //you don't take damage
	} else if (ward_at(u.ux,u.uy) == CIRCLE_OF_ACHERON) {
	    You("are already beyond Acheron.");
		dmg = 0; //you don't take damage
	} else if (!Antimagic && (!mtmp || rn2(mtmp->m_lev) > 12) && !(u.sealsActive&SEAL_OSE)) {
	    if (Hallucination) {
			You("have an out of body experience.");
	    } else if(Upolyd ? (u.mh >= 100) : (u.uhp >= 100)){
			Your("%s stops!  When it finally beats again, it is weak and thready.", body_part(HEART));
			if(Upolyd) u.mh -= d(8,8);	//Same as death's touch attack, sans special effects
			else u.uhp -= d(8,8);		//Not reduced by AC
		} else {
			killer_format = KILLED_BY_AN;
			killer = "touch of death";
			dmg = 0; //no additional damage
			done(DIED);
	    }
	} else if(!(u.sealsActive&SEAL_OSE || resists_death(&youmonst))){
	    if (Antimagic) shieldeff(u.ux, u.uy);
//	    pline("Lucky for you, it didn't work!");
		Your("%s flutters!", body_part(HEART));
		dmg = 8; //you still take damage
	} else{
		dmg = 0;
		shieldeff(u.ux, u.uy);
	}
	stop_occupation();
	break;
    case CLONE_WIZ:
	if (mtmp && mtmp->iswiz && flags.no_of_wizards == 1) {
	    pline("Double Trouble...");
	    clonewiz();
	} else {
	    if(mtmp) impossible("bad wizard cloning?");
	}
		//else end with no message.
	dmg = 0;
	break;
    case FILTH:
    {
       struct monst *mtmp2;
       long old;
       pline("A cascade of filth pours onto you!");
       if (freehand() && rn2(3)) {
           old = Glib;
           Glib += rn1(20, 9);
           Your("%s %s!", makeplural(body_part(HAND)),
               (old ? "are filthier than ever" : "get slimy"));
       }
	   if(uwep && !rn2(20)){
			Your("%s is coated in gunk!", xname(uwep));
			uwep->greased = TRUE;
			Glib += rn1(20, 9);
			if(is_poisonable(uwep)) uwep->opoisoned = OPOISON_FILTH;
			if(uwep->otyp == VIPERWHIP) uwep->opoisonchrgs = 0;
	   }
	   if(uswapwep && u.twoweap && !rn2(20)){
			Your("%s is coated in gunk!", xname(uswapwep));
			uswapwep->greased = TRUE;
			Glib += rn1(20, 9);
			if(is_poisonable(uswapwep)) uswapwep->opoisoned = OPOISON_FILTH;
			if(uswapwep->otyp == VIPERWHIP) uswapwep->opoisonchrgs = 0;
	   }
       if(haseyes(youracedata) && !Blindfolded && !(mtmp && monsndx(mtmp->data) == PM_DEMOGORGON) && rn2(3)) {
           old = u.ucreamed;
           u.ucreamed += rn1(20, 9);
           Your("%s is coated in %sgunk!", body_part(FACE),
                   (old ? "even more " : ""));
           make_blinded(Blinded + (long)u.ucreamed - old, FALSE);
       }
       You("smell putrid!%s", !uclockwork?" You gag and vomit.":"");
		if (!uclockwork) vomit();
		/* same effect as "This water gives you bad breath!" */
		for(mtmp2 = fmon; mtmp2; mtmp2 = mtmp2->nmon) {
			if(!DEADMONSTER(mtmp2) && (mtmp2 != mtmp))
			monflee(mtmp2, 0, FALSE, FALSE);
		}
		if(!Sick && !uclockwork) make_sick((long)rn1(ACURR(A_CON), 20), /* Don't make the PC more sick */
								(char *)0, TRUE, SICK_NONVOMITABLE);
		dmg = rnd(Half_physical_damage ? 5 : 10);
		stop_occupation();
	break;
	}
    case STRANGLE:
    {
       struct obj *otmp;
       if (uamul && (Antimagic || uamul->oartifact || uamul->otyp == AMULET_OF_YENDOR)) {
	    shieldeff(u.ux, u.uy);
            if (!Blind) Your("%s looks vaguely %s for a moment.", xname(uamul),
                               OBJ_DESCR(objects[AMULET_OF_STRANGULATION]));
           else You_feel("a momentary pressure around your %s.",body_part(NECK));
	} else {
           if (uamul) {
               Your("%s warps strangely, then turns %s.", xname(uamul),
                               OBJ_DESCR(objects[AMULET_OF_STRANGULATION]));
               poly_obj(uamul, AMULET_OF_STRANGULATION);
               curse(uamul);
               Amulet_on();
           }
           else {
               if (malediction)
                       verbalize(rn2(2) ? "Thou desirest the amulet? I'll give thee the amulet!" :
                                          "Here is the only amulet you'll need!");
               otmp = mksobj(AMULET_OF_STRANGULATION, FALSE, FALSE);
               curse(otmp);
               (void) addinv(otmp);
               pline("%s appears around your %s!",An(xname(otmp)),body_part(NECK));
               setworn(otmp,W_AMUL);
               Amulet_on();
           }
	}
	dmg = 0;
	stop_occupation();
	break;
    }
     case TURN_TO_STONE:
		if (malediction) /* give a warning to the player */
		   verbalize(rn2(2) ? "I shall make a statue of thee!" :
							  "I condemn thee to eternity unmoving!");
        if (!(poly_when_stoned(youracedata) && polymon(PM_STONE_GOLEM))) {
           if(!Stone_resistance && !Free_action && (!rn2(10) || !have_lizard()) ){
			You_feel("less limber.");
			Stoned = 5;
		   }else{
			You_feel("a momentary stiffness.");
		   }
        } 
		dmg = 0;
	 stop_occupation();
     break;
     case MAKE_VISIBLE:
       HInvis &= ~INTRINSIC;
       You_feel("paranoid.");
       dmg = 0;
	   stop_occupation();
     break;
     case PLAGUE:
       if (!Sick_resistance && !uclockwork) {
          You("are afflicted with disease!");
           make_sick(Sick ? Sick/3L + 1L : (long)rn1(ACURR(A_CON), 20),
                        (char *)0, TRUE, SICK_NONVOMITABLE);
       } else You_feel("slightly infectious.");
       dmg = 0;
	   stop_occupation();
    break;
    case PUNISH:
		if(u.ualign.record <= 1 || !rn2(min(u.ualign.record,20))){
			if (!Punished) {
					punish((struct obj *)0);
				   if (mtmp && is_prince(mtmp->data)) uball->owt += 160;
			} else {
						Your("iron ball gets heavier!");
						if (mtmp && is_prince(mtmp->data)) uball->owt += 240;
						else uball->owt += 160;
			}
		} else Your("sins do not demand punishment.");
		dmg = 0;
		stop_occupation();
	break;
    case EARTHQUAKE:
		pline_The("entire %s is shaking around you!",
               In_endgame(&u.uz) ? "plane" : "dungeon");
        /* Quest nemesis maledictions */
		if (malediction && (!In_endgame(&u.uz) || Is_earthlevel(&u.uz))) {
			if (rn2(2)) verbalize("The earth trembles before my %s!",
                                    rn2(2) ? "power" : "might");
            else verbalize("Open thy maw, mighty earth!");
		}
		if (mtmp) {
			do_earthquake(u.ux, u.uy, min(((int)mtmp->m_lev - 1) / 3 + 1,24), min(((int)mtmp->m_lev - 1) / 6 + 1, 8), TRUE, mtmp);
		} else {
			do_earthquake(u.ux, u.uy, d(2,12), rnd(8), TRUE, (struct monst *) 0);
		}
		aggravate(); /* wake up without scaring */
		dmg = 0;
		stop_occupation();
		doredraw();
	break;
    case ACID_RAIN: /* as seen in the Lethe patch */
		pline("A torrent of burning acid rains down on you!");
		if(uarmh && uarmh->otyp == SEDGE_HAT){
			pline("It runs off the brim of your wide straw hat.");
			dmg = 0;
		} else if(uarmh && uarmh->otyp == WAR_HAT) {
			pline("It runs off the brim of your wide helm.");
			erode_obj(uarmh, TRUE, FALSE);
			dmg = 0;
		} else {
			dmg = d(8, 6);
			if (Acid_resistance) {
				shieldeff(u.ux, u.uy);
				pline("It feels mildly uncomfortable.");
				dmg = 0;
			} 
			if (!EAcid_resistance) {
				destroy_item(POTION_CLASS, AD_FIRE);
			}
			erode_obj(uwep, TRUE, FALSE);
			erode_obj(uswapwep, TRUE, FALSE);
			erode_armor(&youmonst, TRUE);
			water_damage(invent, FALSE, FALSE, FALSE, &youmonst);
			if (!resists_blnd(&youmonst) && rn2(2)) {
				pline_The("acid gets into your %s!", eyecount(youracedata) == 1 ?
						body_part(EYE) : makeplural(body_part(EYE)));
				make_blinded((long)rnd(Acid_resistance ? 10 : 50),FALSE);
				if (!Blind) Your1(vision_clears);
			}
		}
		/* TODO: corrode floor objects */
	stop_occupation();
    break;
    case AGGRAVATION:
	You_feel("that monsters are aware of your presence.");
	aggravate();
	dmg = 0;
	stop_occupation();
	break;
    case GEYSER:{
	static int mboots3 = 0;
	if (!mboots3) mboots3 = find_mboots();
	/* this is physical damage, not magical damage */
	dmg = 0;
	if(Wwalking){
		pline("A sudden geyser erupts under your feet!");
		if(ACURR(A_DEX) >= 14){
			You("put the added momentum to good use.");
			if(ACURR(A_DEX) == 25) youmonst.movement += 12;
			else if(ACURR(A_DEX) >= 18) youmonst.movement += 8;
			else youmonst.movement += 6;
		} else if(ACURR(A_DEX) <= 10){
			You("are knocked around by the geyser's force!");
			if(ACURR(A_DEX) <= 3) dmg = d(8, 6);
			else if(ACURR(A_DEX) <= 6) dmg = d(4, 6);
			else if(ACURR(A_DEX) <= 10) dmg = rnd(6);
		}
		if(uarmf && uarmf->otyp == WATER_WALKING_BOOTS) makeknown(uarmf->otyp);
	} else {
		pline("A sudden geyser slams into you from nowhere!");
		dmg = d(8, 6);
		if(uarmf && uarmf->otyp == mboots3 )
			pline("Good thing you're wearing mud boots!");
		else water_damage(invent, FALSE, FALSE, FALSE, &youmonst);
		if (Half_physical_damage) dmg = (dmg + 1) / 2;
	}
	stop_occupation();
	}break;
    case FIRE_PILLAR:
	pline("A pillar of fire strikes all around you!");
	if (Fire_resistance) {
	    shieldeff(u.ux, u.uy);
	    dmg = 0;
	} else{
	    dmg = d(8, 6);
	}
	if (!EFire_resistance) {
		destroy_item(SCROLL_CLASS, AD_FIRE);
		destroy_item(POTION_CLASS, AD_FIRE);
		destroy_item(SPBOOK_CLASS, AD_FIRE);
	}
	if (Half_spell_damage) dmg = (dmg + 1) / 2;
	burn_away_slime();
	(void) burnarmor(&youmonst);
	(void) burn_floor_paper(u.ux, u.uy, TRUE, FALSE);
	stop_occupation();
	break;
    case ICE_STORM:
	pline("Chunks of ice pummel you from all sides!");
	dmg = d(4, 8);
	
	if(u.udr > 0){
		dmg -= u.udr;//Use average: this is attacking with many chunks of ice all at once
	}
	
	if (dmg < 1) dmg = 1;
	
	if (Half_physical_damage) dmg = (dmg + 1) / 2;
		
	if (Cold_resistance) {
	    shieldeff(u.ux, u.uy);
	} else {
	    dmg += Half_spell_damage ? (d(4, 8)+1)/2 : d(4, 8);
	}
	if (!ECold_resistance) {
		destroy_item(POTION_CLASS, AD_COLD);
	}
	stop_occupation();
	break;
    case HAIL_FLURY:{
		int hfdmg;
		pline("Hailstones pummel you from all sides!");
		if(dmg > 60)
			dmg = 60;
		hfdmg = dmg;
		dmg = hfdmg;
		
		if(u.udr > 0){
			dmg -= u.udr;//Use average: this is attacking with many chunks of ice all at once
		}
		
		if (dmg < 1) dmg = 1;
		
		if (Half_physical_damage) dmg = (dmg + 1) / 2;
			
		if (Cold_resistance) {
			shieldeff(u.ux, u.uy);
		} else {
			dmg += Half_spell_damage ? (hfdmg+1)/2 : hfdmg;
		}
		if (!ECold_resistance) {
			if(hfdmg > rnd(20)) destroy_item(POTION_CLASS, AD_COLD);
		}
		stop_occupation();
	}break;
    case LIGHTNING:
		if (mtmp && !dmgtype(mtmp->data, AD_CLRC)) {
		   zap = AD_ELEC;
		   goto ray;
		} else {
			boolean reflects;

			pline("A bolt of lightning strikes down at you from above!");
			reflects = ureflects("It bounces off your %s%s.", "");
			if (reflects || Shock_resistance) {
				shieldeff(u.ux, u.uy);
				dmg = 0;
				if (reflects) break;
			} else {
				dmg = d(8, 6);
			}
			if (!(reflects || EShock_resistance)) {
				destroy_item(WAND_CLASS, AD_ELEC);
				destroy_item(RING_CLASS, AD_ELEC);
			}
			if (Half_spell_damage) dmg = (dmg + 1) / 2;
			if (!resists_blnd(&youmonst)) {
			   You("are blinded by the flash!");
			   make_blinded((long)rnd(100),FALSE);
			   if (!Blind) Your1(vision_clears);
			}
			stop_occupation();
			break;
		}
	case SILVER_RAYS:{
		int n = 0;
		char * rays;
		dmg = 0;
		if(zap_hit(&youmonst, 0, TRUE))
			n++;
		if(zap_hit(&youmonst, 0, TRUE))
			n++;
		if(!n){
			pline("Silver rays whiz past you!");
			break;
		}
		if (n == 1)
			rays = "a ray";
		if (n >= 2)
			rays = "rays";
		if(hates_silver(youracedata)){
			You("are seared by %s of silver light!", rays);
			dmg = d(n*2,20);
		} else if(!Fire_resistance && species_resists_cold(&youmonst)){
			You("are burned by %s of silver light!", rays);
			dmg = (d(n,20)*3+1)/2;
			destroy_item(SCROLL_CLASS, AD_FIRE);
			destroy_item(POTION_CLASS, AD_FIRE);
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		} else if(!Cold_resistance && species_resists_fire(&youmonst)){
			You("are frozen by %s of silver light!", rays);
			dmg = (d(n,20)*3+1)/2;
			destroy_item(POTION_CLASS, AD_COLD);
		} else if(hates_unholy(youracedata)){
			You("are seared by %s of unholy light!", rays);
			dmg = d(n,20) + d(n,9);
		} else if(hates_holy(youracedata)){
			You("are seared by %s of holy light!", rays);
			dmg = d(n,20) + d(n,7);
		} else if(!Fire_resistance){
			You("are burned by %s of silver light!", rays);
			dmg = d(n,20);
			destroy_item(SCROLL_CLASS, AD_FIRE);
			destroy_item(POTION_CLASS, AD_FIRE);
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		} else if(!Shock_resistance){
			You("are shocked by %s of silver light!", rays);
			dmg = d(n,20);
			destroy_item(WAND_CLASS, AD_ELEC);
			destroy_mitem(mtmp, RING_CLASS, AD_ELEC);
		} else if(!Cold_resistance){
			You("are frozen by %s of silver light!", rays);
			dmg = d(n,20);
			destroy_item(POTION_CLASS, AD_COLD);
		} else if(!Acid_resistance){
			You("are burned by %s of silver light!", rays);
			dmg = d(n,20);
			destroy_item(POTION_CLASS, AD_FIRE);
		} else {
			You("are pierced by %s of silver light!", rays);
			dmg = 0;
			dmg += rnd(20) - roll_udr(mtmp);
			if(dmg < 1)
				dmg = 1;
			if(n == 2){
				dmg += rnd(20) - roll_udr(mtmp);
				if(dmg < 2)
					dmg = 2;
			}
		}
	}break;
	case GOLDEN_WAVE:
		dmg = 0;
		if(!Fire_resistance && species_resists_cold(&youmonst)){
			You("are burned by golden light!");
			dmg = (d(2,12)*3+1)/2;
			destroy_item(SCROLL_CLASS, AD_FIRE);
			destroy_item(POTION_CLASS, AD_FIRE);
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		} else if(!Cold_resistance && species_resists_fire(&youmonst)){
			You("are frozen by golden light!");
			dmg = (d(2,12)*3+1)/2;
			destroy_item(POTION_CLASS, AD_COLD);
		} else if(hates_silver(youracedata)){
			You("are seared by golden light!");
			dmg = d(2,12) + d(1,20);
		} else if(hates_unholy(youracedata)){
			You("are seared by unholy light!");
			dmg = d(2,12) + d(1,9);
		} else if(hates_holy(youracedata)){
			You("are seared by holy light!");
			dmg = d(2,12) + d(1,7);
		} else if(!Fire_resistance){
			You("are burned by golden light!");
			dmg = d(2,12);
			destroy_item(SCROLL_CLASS, AD_FIRE);
			destroy_item(POTION_CLASS, AD_FIRE);
			destroy_item(SPBOOK_CLASS, AD_FIRE);
		} else if(!Shock_resistance){
			You("are shocked by golden light!");
			dmg = d(2,12);
			destroy_item(WAND_CLASS, AD_ELEC);
			destroy_mitem(mtmp, RING_CLASS, AD_ELEC);
		} else if(!Cold_resistance){
			You("are frozen by golden light!");
			dmg = d(2,12);
			destroy_item(POTION_CLASS, AD_COLD);
		} else if(!Acid_resistance){
			You("are burned by golden light!");
			dmg = d(2,12);
			destroy_item(POTION_CLASS, AD_FIRE);
		} else {
			You("are slashed by golden light!");
			dmg = 0;
			dmg += d(2,12) - roll_udr(mtmp);
			if(dmg < 1)
				dmg = 1;
		}
	break;
	case PRISMATIC_SPRAY:{
		int dx = 0, dy = 0;
		dmg /= 10;
		if(dmg > 7) dmg = 7;
		for(; dmg; dmg--){
			switch(rn2(7)){
				case 0:
					//Physical
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_PHYS, MON_EXPLODE, d(6,6), EXPL_RED, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 1:
					//Fire
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_FIRE, MON_EXPLODE, d(6,6), EXPL_FIERY, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 2:
					//Poison
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_DRST, MON_EXPLODE, d(6,6),  EXPL_YELLOW, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 3:
					//Acid
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_ACID, MON_EXPLODE, d(6,6), EXPL_LIME, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 4:
					//Cold
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_COLD, MON_EXPLODE, d(6,6), EXPL_BBLUE, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 5:
					//Electricity
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_ELEC, MON_EXPLODE, d(6,6), EXPL_MAGENTA, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
				case 6:
					//Disintegration
					explode(mtmp->mux+dx, mtmp->muy+dy, AD_DISN, MON_EXPLODE, d(6,6), EXPL_MAGICAL, 2); //explode(x, y, type, dam, olet, expltype, radius)
				break;
			}
				dx = rnd(3) - 2; 
				dy = rnd(3) - 2;
				if (!isok(mtmp->mux + dx, mtmp->muy + dy) ||
					IS_STWALL(levl[mtmp->mux + dx][mtmp->muy + dy].typ)
				) {
					/* Spell is reflected back to center */
					dx = 0;
					dy = 0;
				}
		}
		dmg = 0;
		stop_occupation();
	}break;
	case ACID_BLAST:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux, mtmp->muy, AD_ACID, MON_EXPLODE, dmg, EXPL_NOXIOUS, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_FIRA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux, mtmp->muy, AD_FIRE, MON_EXPLODE, dmg, EXPL_FIERY, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_FIRAGA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_BLIZZARA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux, mtmp->muy, AD_COLD, MON_EXPLODE, dmg, EXPL_FROSTY, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_BLIZZAGA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2, EXPL_FROSTY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2, EXPL_FROSTY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2, EXPL_FROSTY, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_THUNDARA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux, mtmp->muy, AD_ELEC, MON_EXPLODE, dmg, EXPL_MAGICAL, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_THUNDAGA:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		dmg = 0;
		stop_occupation();
	break;
	case MON_FLARE:
		if(dmg > 60) dmg = 60;
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_FROSTY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_FIERY, 1);
		explode(mtmp->mux+rn2(3)-1, mtmp->muy+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_MUDDY, 1);
		explode(mtmp->mux, mtmp->muy, AD_PHYS, MON_EXPLODE, dmg, EXPL_FROSTY, 2);
		dmg = 0;
		stop_occupation();
	break;
	case MON_WARP:
		if(Half_spell_damage) dmg /= 2;
		if(Half_physical_damage) dmg /= 2;
		if(dmg > 100) dmg = 100;
		pline("Space warps into deadly blades!");
		stop_occupation();
	break;
	case MON_POISON_GAS:
		if(!mtmp){
			flags.cth_attk=TRUE;//state machine stuff.
			create_gas_cloud(u.ux, u.uy, rnd(3), rnd(3)+1);
			flags.cth_attk=FALSE;
		} else {
			flags.cth_attk=TRUE;//state machine stuff.
			create_gas_cloud(mtmp->mux, mtmp->muy, rnd(3), rnd(3)+1);
			flags.cth_attk=FALSE;
		}
		dmg = 0;
		stop_occupation();
	break;
	case SOLID_FOG:
		if(!mtmp){
			flags.cth_attk=TRUE;//state machine stuff.
			create_fog_cloud(u.ux, u.uy, 3, 8);
			flags.cth_attk=FALSE;
		} else {
			flags.cth_attk=TRUE;//state machine stuff.
			create_fog_cloud(mtmp->mux, mtmp->muy, 3, 8);
			flags.cth_attk=FALSE;
			if(mtmp->data == &mons[PM_PLUMACH_RILMANI]) mtmp->mcan = 1;
		}
		dmg = 0;
		stop_occupation();
	break;
	case MON_TIME_STOP:{
		int extraturns = d(1,4)+1, i;
		struct monst *tmpm;
		if(!mtmp || u.summonMonster)
			goto psibolt;
		// if(canseemon(mtmp))
			// pline("%s blurs with speed!", Monnam(mtmp));
		// mtmp->movement += (extraturns)*12;
		// for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			// if(tmpm->data == &mons[PM_UVUUDAUM] && tmpm != mtmp){
				// tmpm->movement += (extraturns)*12;
				// if(canseemon(tmpm))
					// pline("%s blurs with speed!", Monnam(tmpm));
			// }
		// }
		// u.summonMonster = TRUE;//Not exactly a summoning, but don't stack this too aggressively.
		//Note: 1-4 free turns is too strong.  Just give that much healing instead.
		if(canseemon(mtmp))
			pline("%s blurs with speed!", Monnam(mtmp));
			for(i= extraturns; i > 0; i--){
				mon_regen(mtmp, TRUE);
				timeout_problems(mtmp);
			}
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(tmpm->data == &mons[PM_UVUUDAUM] && tmpm != mtmp && !DEADMONSTER(tmpm)){
				if(canseemon(tmpm))
					pline("%s blurs with speed!", Monnam(tmpm));
				for(i= extraturns; i > 0; i--){
					mon_regen(tmpm, TRUE);
					timeout_problems(tmpm);
				}
			}
		}
		dmg = 0;
	}break;
	case TIME_DUPLICATE:{
		struct monst *tmpm;
		if(!mtmp) goto psibolt;
		tmpm = makemon(mtmp->data, mtmp->mux, mtmp->muy, MM_ADJACENTOK|MM_NOCOUNTBIRTH|NO_MINVENT);
		tmpm->mvanishes = d(1,4)+1;
		tmpm->mclone = 1;
		dmg = 0;
	}break;
	case NAIL_TO_THE_SKY:{
		HLevitation &= ~I_SPECIAL;
		if(!Levitation) {
			/* kludge to ensure proper operation of float_up() */
			HLevitation = 1;
			float_up();
			/* reverse kludge */
			HLevitation = 0;
			if (!Is_waterlevel(&u.uz)) {
				if((u.ux != xupstair || u.uy != yupstair)
				   && (u.ux != sstairs.sx || u.uy != sstairs.sy || !sstairs.up)
				   && (!xupladder || u.ux != xupladder || u.uy != yupladder)
				) {
					You("hit your %s on the %s.",
						body_part(HEAD),
						ceiling(u.ux,u.uy));
					losehp(uarmh ? 1 : rnd(10),
						"colliding with the ceiling",
						KILLED_BY);
				} else (void) doup();
			}
		}
		incr_itimeout(&HLevitation, (d(1,4)+1)*100);
		spoteffects(FALSE);	/* for sinks */
	}break;
    case SUMMON_ANGEL: /* cleric only */
    {
       struct monst *mtmp2;
	   if(!mtmp) goto psibolt;
	   if(is_alienist(mtmp->data)) goto summon_alien;
	   mtmp2 = mk_roamer(&mons[PM_ANGEL],
               sgn(mtmp->data->maligntyp), mtmp->mux, mtmp->muy, FALSE);
       if (mtmp2) {
           if (canspotmon(mtmp2))
               pline("%s %s!",
                       An(Hallucination ? rndmonnam() : "angel"),
                       Is_astralevel(&u.uz) ? "appears near you" :
                                              "descends from above");
           else
               You("sense the arrival of %s.",
                       an(Hallucination ? rndmonnam() : "hostile angel"));
       }
       dmg = 0;
	   stop_occupation();
       break;
    }
    case SUMMON_ALIEN: /* special only */
summon_alien:
    {
       struct monst *mtmp2;
	   int tries = 0;
	   static struct permonst *aliens[] = {&mons[PM_HOOLOOVOO],
									&mons[PM_SHAMBLING_HORROR],
									&mons[PM_STUMBLING_HORROR],
									&mons[PM_WANDERING_HORROR],
									&mons[PM_MASTER_MIND_FLAYER],
									&mons[PM_EDDERKOP],
									&mons[PM_AOA],
									&mons[PM_HUNTING_HORROR],
									&mons[PM_BYAKHEE],
									&mons[PM_UVUUDAUM]};
	   if(!mtmp) goto psibolt;
	   do {
	       mtmp2 = makemon(aliens[rn2(SIZE(aliens))], mtmp->mux, mtmp->muy, MM_ADJACENTOK|MM_NOCOUNTBIRTH);
	   } while (!mtmp2 && tries++ < 10);
       if (mtmp2) {
           if (canspotmon(mtmp2))
               pline("The world tears open, and %s steps through!",
                       an(Hallucination ? rndmonnam() : "alien"));
           else
               You("sense the arrival of %s.",
                       an(Hallucination ? rndmonnam() : "alien"));
       }
       dmg = 0;
	   stop_occupation();
       break;
    }
    case SUMMON_DEVIL: /* cleric only */
    {
       struct monst *mtmp2;
	   if(!mtmp) goto psibolt;
	   if(is_alienist(mtmp->data)) goto summon_alien;
	   if(!(mtmp->data->maligntyp)) mtmp2 = summon_minion(A_NEUTRAL, FALSE, TRUE, FALSE);
	   else if((mtmp->data->maligntyp) > 0) mtmp2 = summon_minion(A_LAWFUL, FALSE, TRUE, FALSE);
	   else mtmp2 = summon_minion(A_CHAOTIC, FALSE, TRUE, FALSE);
       if (mtmp2) {
           if (canspotmon(mtmp2))
               pline("%s ascends from below!",
                       An(Hallucination ? rndmonnam() : "fiend"));
           else
               You("sense the arrival of %s.",
                       an(Hallucination ? rndmonnam() : "hostile fiend"));
       }
       dmg = 0;
	   stop_occupation();
       break;
    }
    case SUMMON_MONS:
    {
	int count;
	if(!mtmp || u.summonMonster) goto psibolt;
	
	if(Role_if(PM_ANACHRONONAUT) && In_quest(&u.uz))
		break;
	
	u.summonMonster = TRUE;
	count = nasty(mtmp);	/* summon something nasty */
	if (mtmp->iswiz)
	    verbalize("Destroy the thief, my pet%s!", plur(count));
	else {
	    const char *mappear =
		(count == 1) ? "A monster appears" : "Monsters appear";

	    /* messages not quite right if plural monsters created but
	       only a single monster is seen */
	    if (Invisible && !perceives(mtmp->data) &&
				    (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around a spot near you!", mappear);
	    else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
		pline("%s around your displaced image!", mappear);
	    else
		pline("%s from nowhere!", mappear);
	}
	dmg = 0;
    stop_occupation();
	break;
    }
    case INSECTS:
	if(!mtmp) goto psibolt;
      {
	/* Try for insects, and if there are none
	   left, go for (sticks to) snakes.  -3. */
	struct permonst *mdat = mtmp->data;
	struct permonst *pm;
	struct monst *mtmp2 = (struct monst *)0;
	char let;
	boolean success;
	int i,j;
	coord bypos;
	int quan;
	
	if(is_drow(mdat)){
		j = 0;
		do pm = mkclass(S_SPIDER, G_NOHELL|G_HELL);
		while(!is_spider(pm) && j++ < 30);
		let = (pm ? S_SPIDER : S_SNAKE);
	} else{
		pm = mkclass(S_ANT,G_NOHELL|G_HELL);
		let = (pm ? S_ANT : S_SNAKE);
	}
	quan = (mtmp->m_lev < 2) ? 1 : rnd((int)mtmp->m_lev / 2);
	if (quan < 3) quan = 3;
	success = pm ? TRUE : FALSE;
	for (i = 0; i <= quan; i++) {
	    if (!enexto(&bypos, mtmp->mux, mtmp->muy, mtmp->data)) break;
		if(is_drow(mdat)){
			j = 0;
			do pm = mkclass(S_SPIDER, G_NOHELL|G_HELL);
			while(!is_spider(pm) && j++ < 30);
			let = (pm ? S_SPIDER : S_SNAKE);
		} else{
			pm = mkclass(S_ANT,G_NOHELL|G_HELL);
			let = (pm ? S_ANT : S_SNAKE);
		}
	    if (pm != 0 && (mtmp2 = makemon(pm, bypos.x, bypos.y, NO_MM_FLAGS)) != 0) {
			success = TRUE;
			mtmp2->msleeping = mtmp2->mpeaceful = mtmp2->mtame = 0;
				   /* arbitrarily strengthen enemies in astral and sanctum */
				   if (Is_astralevel(&u.uz) || Is_sanctum(&u.uz)) {
					   mtmp2->m_lev += rn1(3,3);
					   mtmp2->mhp = (mtmp2->mhpmax += rn1((int)mtmp->m_lev,20));
				   }
			set_malign(mtmp2);
	    }
	}
	/* Not quite right:
         * -- message doesn't always make sense for unseen caster (particularly
	 *    the first message)
         * -- message assumes plural monsters summoned (non-plural should be
         *    very rare, unlike in nasty())
         * -- message assumes plural monsters seen
         */
	if (!success)
	    pline("%s casts at a clump of sticks, but nothing happens.",
		Monnam(mtmp));
	else if (let == S_SNAKE)
	    pline("%s transforms a clump of sticks into snakes!",
		Monnam(mtmp));
	else if (Invisible && !perceives(mtmp->data) &&
				(mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline("%s summons %s around a spot near you!",
		Monnam(mtmp), let == S_SPIDER ? "arachnids" : "insects");
	else if (Displaced && (mtmp->mux != u.ux || mtmp->muy != u.uy))
	    pline("%s summons %s around your displaced image!",
		Monnam(mtmp), let == S_SPIDER ? "arachnids" : "insects");
	else
	    pline("%s summons %s!", Monnam(mtmp), let == S_SPIDER ? "arachnids" : "insects");
	dmg = 0;
    stop_occupation();
	break;
      }
     case RAISE_DEAD:
     {
       coord mm;
       register int x, y;
       pline("%s raised the dead!", mtmp ? Monnam(mtmp) : "Something");
       mm.x = mtmp ? mtmp->mx : u.ux;
       mm.y = mtmp ? mtmp->my : u.uy;
       mkundead(&mm, TRUE, NO_MINVENT);
       dmg = 0;
	   stop_occupation();
       break;
     }
    case CONE_OF_COLD: /* simulates basic cone of cold */
       zap = AD_COLD;
       goto ray;
    case CURSE_ITEMS:
       You_feel("as if you need some help.");
       rndcurse();
       dmg = 0;
	   stop_occupation();
       break;
     case ARROW_RAIN: /* actually other things as well */
     {
       struct obj *otmp;
       int weap, i, tdmg = 0;
	   int shienChance = 0, shienCount = 0;
       dmg = 0;
		if(uwep && is_lightsaber(uwep) && litsaber(uwep)){
			if(u.fightingForm == FFORM_SHIEN && (!uarm || is_light_armor(uarm))){
				switch(min(P_SKILL(FFORM_SHIEN), P_SKILL(weapon_type(uwep)))){
					case P_BASIC:
						shienChance = 33;
					break;
					case P_SKILLED:
						shienChance = 66;
					break;
					case P_EXPERT:
						shienChance = 100;
					break;
				}
			}
		}
       if (rn2(3)) weap = ARROW;
       else if (!rn2(3)) weap = DAGGER;
       else if (!rn2(3)) weap = SPEAR;
       else if (!rn2(3)) weap = KNIFE;
       else if (!rn2(3)) weap = JAVELIN;
       else if (!rn2(3)) weap = AXE;
       else {
          weap = rnd_class(ARROW,WORM_TOOTH-1);
           if (weap == TRIDENT) weap = JAVELIN;
       }
       otmp = mksobj(weap, TRUE, FALSE);
       otmp->quan = (long) rn1(mtmp ? (min(MAX_BONUS_DICE, mtmp->m_lev/3 + 1)) : (rn2(12) + 1), 4);
	   otmp->quan = min(otmp->quan, 16L);
       otmp->owt = weight(otmp);
       otmp->spe = 0;
       You("are hit from all directions by a %s of %s!",
               rn2(2) ? "shower" : "hail", xname(otmp));
       for (i = 0; i < otmp->quan; i++) {
			if(shienChance <= rnd(100)){
				shienCount++;
				tdmg = 0;
			} else tdmg = dmgval(otmp, &youmonst, 0);
			if (tdmg){
				tdmg -= roll_udr(mtmp);
				if (tdmg < 1) tdmg = 1;
				
			}
			if (Half_physical_damage) tdmg = (tdmg + 1) / 2;
			dmg += tdmg;
       }
		if(shienCount < otmp->quan){
			otmp->quan -= shienCount;
			otmp->owt = weight(otmp);
	        if (!flooreffects(otmp, u.ux, u.uy, "fall")) {
	            place_object(otmp, u.ux, u.uy);
	            stackobj(otmp);
	            newsym(u.ux, u.uy);
	        }
		} else {
			delobj(otmp);
		}
	   stop_occupation();
     } break;
     case DROP_BOULDER:
     {
        struct obj *otmp;
		dmg = 0;
        boolean iron = (!rn2(4) ||
#ifdef REINCARNATION
            Is_rogue_level(&u.uz) || 
#endif
            (In_endgame(&u.uz) && !Is_earthlevel(&u.uz)));
        otmp = mksobj(iron ? HEAVY_IRON_BALL : BOULDER,
                        FALSE, FALSE);
        otmp->quan = 1;
        otmp->owt = weight(otmp);
       if (iron) otmp->owt += 160 * rn2(2);
        pline("%s drops out of %s and hits you!", An(xname(otmp)),
                iron ? "nowhere" : the(ceiling(u.ux,u.uy)));
        dmg = dmgval(otmp, &youmonst, 0);
        if (uarmh) {
            if (is_hard(uarmh)) {
                pline("Fortunately, you are wearing a hard helmet.");
                if (dmg > 2) dmg = 2;
            } else if (flags.verbose) {
                Your("%s does not protect you.",
                        xname(uarmh));
            }
        }
        if (!flooreffects(otmp, u.ux, u.uy, "fall")) {
            place_object(otmp, u.ux, u.uy);
            stackobj(otmp);
            newsym(u.ux, u.uy);
        }
       if (Half_physical_damage) dmg = (dmg + 1) / 2;
	   stop_occupation();
        break;
    }
     case DESTRY_WEPN:
     {
        struct obj *otmp = uwep;
        const char *hands;
       hands = bimanual(otmp,youracedata) ? makeplural(body_part(HAND)) : body_part(HAND);
        if (otmp->oclass == WEAPON_CLASS && !Antimagic && !otmp->oartifact && rn2(4)) {
               /* Quest nemesis maledictions */
			   if(otmp->spe > -7){
					otmp->spe -= 1;
					if(otmp->spe < -7) otmp->spe = -7;
					pline("Your %s has been damaged!", xname(otmp));
			   }
               else if ((rn2(3) && mtmp->data->maligntyp < 0) && !Hallucination) {
                   if (malediction)
                       verbalize("%s, your %s broken!", plname, aobjnam(otmp,"are"));
                   Your("%s to pieces in your %s!", aobjnam(otmp, "shatter"), hands);
                   setuwep((struct obj *)0);
                   useup(otmp);
               } else {
                  Your("%s shape in your %s.", aobjnam(otmp, "change"), hands);
                  poly_obj(otmp, BANANA);
               }
        } else if (otmp && !welded(otmp) && otmp->otyp != LOADSTONE && (!Antimagic || !rn2(4))){
			if(mtmp){
				if(rn2(((int)mtmp->m_lev)) > (ACURRSTR)) {
					Your("%s knocked out of your %s!",
						aobjnam(otmp,"are"), hands);
					setuwep((struct obj *)0);
					dropx(otmp);
				}
				else Your("%s for a moment.", aobjnam(otmp, "shudder"));
			} else {
				if(ACURRSTR < rnd(25) ){
					Your("%s knocked out of your %s!",
						aobjnam(otmp,"are"), hands);
					setuwep((struct obj *)0);
					dropx(otmp);
				}
				else Your("%s for a moment.", aobjnam(otmp, "shudder"));
			}
        } else{
            Your("%s for a moment.", aobjnam(otmp, "shudder"));
        } dmg = 0;
	   stop_occupation();
    } break;
    case DESTRY_ARMR:{
		struct obj *smarm;
		if (Antimagic) {
		   shieldeff(u.ux, u.uy);
		   pline("A field of force surrounds you!");
		} else if ((smarm = some_armor(&youmonst)) == (struct obj *)0) {
		   Your("skin itches.");
		/* Quest nemesis maledictions */
		} else if(objects[smarm->otyp].oc_oprop == DISINT_RES){
			if(smarm->spe <= -1*objects[smarm->otyp].a_ac) destroy_arm(smarm);
			else{
				smarm->spe -= 1;
				if(smarm->spe < -1*objects[smarm->otyp].a_ac) smarm->spe = -1*objects[smarm->otyp].a_ac;
				pline("A field of force surrounds your %s!", xname(smarm));
			}
			if (malediction) {
				if (rn2(2)) verbalize("Thy defenses are useless!");
				else verbalize("Thou might as well be naked!");
			}
		}
		dmg = 0;
		stop_occupation();
	} break;
    case EVIL_EYE:
		if(mtmp){
			struct attack evilEye = {AT_GAZE, AD_LUCK, 1, 1};
			gazemu(mtmp, &evilEye);
		}
		else{
			You_feel("your luck running out.");
			change_luck(-1);
		}
    stop_occupation();
	dmg = 0;
    break;
    case VULNERABILITY:{
		int x, y, n, prot;
		struct monst *cmon;
		if(!mtmp){
			x = (int) u.ux;
			y = (int) u.uy;
			n = 8;
		} else {
			x = (int) mtmp->mux;
			y = (int) mtmp->muy;
			n = min(MAX_BONUS_DICE, mtmp->m_lev/3+1)+3;
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon != mtmp &&
				cmon->mhp<cmon->mhpmax && 
				!DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful != mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				prot = rnd(n);
				if(resists_magm(cmon)) cmon->mstdy = min(prot, cmon->mstdy + (prot/2+1));
				else cmon->mstdy = min(prot, cmon->mstdy+prot);
			}
		}
		prot = rnd(n);
		if(Antimagic) u.ustdy = min(prot, u.ustdy + (prot/2+1));
		else u.ustdy = min(prot, u.ustdy+prot);
		You_feel("vulnerable!");
	    dmg = 0;
	}
	stop_occupation();
    break;
    case DRAIN_LIFE:  /* simulates player spell "drain life" */
		if(!mtmp || distmin(mtmp->mx,mtmp->my,u.ux,u.uy) < 2){
			if (Drain_resistance) {
				/* note: magic resistance doesn't protect
			 * against "drain life" spell
			 */
				shieldeff(u.ux, u.uy);
				You_feel("momentarily frail.");
			} else {
				Your("body deteriorates!");
				exercise(A_CON, FALSE);
				losexp("life drainage",TRUE,FALSE,FALSE);
			}
			dmg = 0;
			stop_occupation();
		}
	    stop_occupation();
        break;
    case WEAKEN_YOU:		/* drain strength */
	if (Fixed_abil) {
		You_feel("momentarily weakened.");
	} else if (Antimagic) {
	    shieldeff(u.ux, u.uy);
		if(rn2(2)){
			You_feel("a bit weaker.");
		    losestr(1);
		    if (u.uhp < 1 && mtmp)
				done_in_by(mtmp) ;
	    }else You_feel("momentarily weakened.");
	} else {
	    You("suddenly feel weaker!");
	    dmg = rnd(2);
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
			losestr(dmg);
	    if (u.uhp < 1 && mtmp)
			done_in_by(mtmp);
	}
	dmg = 0;
	stop_occupation();
	break;
    case WEAKEN_STATS:           /* drain any stat */
       if (Fixed_abil) {
           You_feel("momentarily weakened.");
       } else if (mtmp ? is_prince(mtmp->data) : rn2(3)) {
          int typ = 0;
          boolean change = FALSE;
          do {
              if (adjattrib(typ, -rnd(2), -1)) change = TRUE;
          } while (++typ < A_MAX);
          if (!change) goto drainhp;
       } else {
          int typ = rn2(A_MAX);
           dmg = rnd(4);
           if (Half_spell_damage) dmg = (dmg + 1) / 2;
           if (dmg < 1) dmg = 1;
          /* try for a random stat */
           if (adjattrib(typ, -dmg, -1)) {
               /* Quest nemesis maledictions */
              if (malediction) verbalize("Thy powers are waning, %s!", plname);
           } else { /* if that fails, drain max HP a bit */
drainhp:
              You_feel("your life force draining away...");
			  dmg*=5;
              if (dmg > 20) dmg = 20;
              if (Upolyd) {
                  u.mh -= dmg;
                  u.mhmax -= dmg;
              } else {
                  u.uhp -= dmg;
                  u.uhpmax -= dmg;
              }
              if (u.uhp < 1 && mtmp) done_in_by(mtmp);
               /* Quest nemesis maledictions */
              if (malediction)
                 verbalize("Verily, thou art no mightier than the merest newt.");
          }
       }
       dmg = 0;
	   stop_occupation();
       break;
    case NIGHTMARE:
		dmg = mtmp ? rnd((int)mtmp->m_lev) : rnd(10);
		You_hear("%s laugh menacingly as the world blurs around you...", mtmp ? mon_nam(mtmp) : "Someone");
		if(Antimagic||Half_spell_damage) dmg = (dmg + 1) / ((Antimagic + Half_spell_damage) * 2);
		make_confused(HConfusion + dmg*10, FALSE);
		make_stunned(HStun + dmg, FALSE);
		make_hallucinated(HHallucination + dmg*15, FALSE, 0L);
		dmg = 0;
		stop_occupation();
	break;
    case MAKE_WEB:
       You("become entangled in hundreds of %s!",
               Hallucination ? "two-minute noodles" : "thick cobwebs");
       /* We've already made sure this is safe */
       dotrap(maketrap(u.ux,u.uy,WEB), NOWEBMSG);
       newsym(u.ux,u.uy);
        /* Quest nemesis maledictions */
       if (malediction) {
           if (rn2(ACURR(A_STR)) > 15) verbalize("Thou art dressed like a meal for %s!",
                                        rn2(2) ? "Ungoliant" : "Arachne");
            else verbalize("Struggle all you might, but it will get thee nowhere.");
       }
        dmg = 0;
	   stop_occupation();
       break;
    case DISAPPEAR:            /* makes self invisible */
		if(!mtmp) goto openwounds;
       if (!mtmp->minvis && !mtmp->invis_blkd) {
           if (canseemon(mtmp))
               pline("%s suddenly %s!", Monnam(mtmp),
                     !See_invisible(mtmp->mx,mtmp->my) ? "disappears" : "becomes transparent");
           mon_set_minvis(mtmp);
          if (malediction && !canspotmon(mtmp))
               You_hear("%s fiendish laughter all around you.", s_suffix(mon_nam(mtmp)));
       } else
           impossible("no reason for monster to cast disappear spell?");
	   dmg = 0;
       break;
     case DRAIN_ENERGY: /* stronger than antimagic field */
        if(Antimagic && !Race_if(PM_INCANTIFIER)) {
            shieldeff(u.ux, u.uy);
           You_feel("momentarily lethargic.");
        } else drain_en(rn1(u.ulevel,dmg));
        dmg = 0;
	    stop_occupation();
        break;
    case SLEEP:
       zap = AD_SLEE;
       goto ray;
    case MAGIC_MISSILE:
       zap = AD_MAGM;
ray:
	   if(!mtmp) goto psibolt;
       nomul(0, NULL);
       if(canspotmon(mtmp))
           pline("%s zaps you with a %s!", Monnam(mtmp),
                     flash_type(zap, SPBOOK_CLASS));
       buzz(zap, SPBOOK_CLASS, FALSE, min(MAX_BONUS_DICE, (mtmp->m_lev/3)+1), mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),0,0);
       dmg = 0;
	   stop_occupation();
       break;
    case BLIND_YOU:
	/* note: resists_blnd() doesn't apply here */
	if (!Blinded) {
	    int num_eyes = eyecount(youracedata);
	    if (mtmp && dmgtype(mtmp->data, AD_CLRC))
	    pline("Scales cover your %s!",
		  (num_eyes == 1) ?
		  body_part(EYE) : makeplural(body_part(EYE)));
	    else if (Hallucination)
		pline("Oh, bummer!  Everything is dark!  Help!");
	    else pline("A cloud of darkness falls upon you.");
	    make_blinded(Half_spell_damage ? 100L : 200L, FALSE);
	    if (!Blind) Your1(vision_clears);
	    dmg = 0;
	} else
	    impossible("no reason for monster to cast blindness spell?");
	stop_occupation();
	break;
    case PARALYZE:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    if (multi >= 0){
			You("stiffen briefly.");
			if(!Free_action) nomul(-1, "paralyzed by a monster");
		}
	} else {
	    if (multi >= 0) You("are frozen in place!");
	    dmg = min_ints(rnd(4), mtmp ? rnd((int)mtmp->m_lev) : rnd(30));
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    nomul(-dmg, "paralyzed by a monster");
	}
	dmg = 0;
	stop_occupation();
	break;
    case CONFUSE_YOU:
	if (Antimagic || Free_action) {
	    shieldeff(u.ux, u.uy);
	    You_feel("momentarily dizzy.");
	} else {
	    boolean oldprop = !!Confusion;

	    dmg = rnd(10) + (mtmp ? rnd((int)mtmp->m_lev) : rnd(30));
	    if (Half_spell_damage) dmg = (dmg + 1) / 2;
	    make_confused(HConfusion + dmg, TRUE);
	    if (Hallucination)
		You_feel("%s!", oldprop ? "trippier" : "trippy");
	    else
		You_feel("%sconfused!", oldprop ? "more " : "");
	}
	dmg = 0;
	stop_occupation();
	break;
    case STUN_YOU:
       if (Antimagic || Free_action) {
           shieldeff(u.ux, u.uy);
           if (!Stunned)
               You_feel("momentarily disoriented.");
           if(!Free_action) make_stunned(1L, FALSE);
       } else {
           You(Stunned ? "struggle to keep your balance." : "reel...");
           dmg = d(ACURR(A_DEX) < 12 ? 2 : 1, 4);
           if (Half_spell_damage) dmg = (dmg + 1) / 2;
           make_stunned(HStun + dmg, FALSE);
       }
       dmg = 0;
	   stop_occupation();
       break;
    case SUMMON_SPHERE:
    {
       /* For a change, let's not assume the spheres are together. : ) */
       int sphere = (!rn2(3) ? PM_FLAMING_SPHERE : (!rn2(2) ?
                             PM_FREEZING_SPHERE : PM_SHOCKING_SPHERE));
       boolean created = FALSE;
       struct monst *mon;
       if (!(mvitals[sphere].mvflags & G_GONE && !In_quest(&u.uz)) &&
		(mon = makemon(&mons[sphere],
			u.ux, u.uy, MM_ADJACENTOK|NO_MINVENT)) != 0)
				if (canspotmon(mon)) created++;
       if (created)
           pline("%s is created!",
                      Hallucination ? rndmonnam() : Amonnam(mon));
       dmg = 0;
	   stop_occupation();
       break;
    }
    case DARKNESS:
       litroom(FALSE, (struct obj *)0);
       dmg = 0;
	   stop_occupation();
       break;
    case HASTE_SELF:
	   if(!mtmp) goto psibolt;
       mon_adjust_speed(mtmp, 1, (struct obj *)0);
       dmg = 0;
       break;
    case CURE_SELF:
	if(!mtmp) goto openwounds;
	if (mtmp->mhp < mtmp->mhpmax) {
	    if (canseemon(mtmp))
		pline("%s looks better.", Monnam(mtmp));
	    /* note: player healing does 6d4; this used to do 1d8; then it did 3d6 */
	    if ((mtmp->mhp += d(min(MAX_BONUS_DICE, mtmp->m_lev/3+1),8)) > mtmp->mhpmax)
			mtmp->mhp = mtmp->mhpmax;
	}
	dmg = 0;
	break;
	case MASS_HASTE:{
		int x, y, n;
		struct monst *cmon;
		if(!mtmp){
			x = (int) u.ux;
			y = (int) u.uy;
			n = 8;
		} else {
			x = (int) mtmp->mx;
			y = (int) mtmp->my;
			n = min(MAX_BONUS_DICE, mtmp->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( !DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful == mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				mon_adjust_speed(cmon, 1, (struct obj *)0);
			}
		}
		if(mtmp->mtame && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			if(!Very_fast)
				You("are suddenly moving %sfaster.",
					Fast ? "" : "much ");
			else {
				Your("%s get new energy.",
					makeplural(body_part(LEG)));
			}
			exercise(A_DEX, TRUE);
			incr_itimeout(&HFast, rn1(10, 100));
		}
	    dmg = 0;
	}break;
	case MASS_CURE_CLOSE:{
		int x, y, n;
		struct monst *cmon;
		if(!mtmp){
			x = (int) u.ux;
			y = (int) u.uy;
			n = 8;
		} else {
			x = (int) mtmp->mx;
			y = (int) mtmp->my;
			n = min(MAX_BONUS_DICE, mtmp->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon->mhp < cmon->mhpmax && 
				!DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful == mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				if((cmon->mhp += d(n,8)) > cmon->mhpmax)
					cmon->mhp = cmon->mhpmax;
				if (canseemon(cmon))
					pline("%s looks better.", Monnam(mtmp));
			}
		}
		if(mtmp->mtame && ((Upolyd && u.mh<u.mhmax) || (!Upolyd && u.uhp < u.uhpmax)) && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			healup(d(n,8), 0, FALSE, FALSE);
			You("feel better.");
		}
	    dmg = 0;
	}break;
	case MASS_CURE_FAR:{
		int x, y, n;
		struct monst *cmon;
		if(!mtmp){
			x = (int) u.ux;
			y = (int) u.uy;
			n = 8;
		} else {
			if(mtmp->mux == 0 && mtmp->muy == 0){
				x = (int) mtmp->mx;
				y = (int) mtmp->my;
			} else {
				x = (int) mtmp->mux;
				y = (int) mtmp->muy;
			}
			n = min(MAX_BONUS_DICE, mtmp->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon != mtmp &&
				cmon->mhp<cmon->mhpmax && 
				!DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful == mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				if((cmon->mhp += d(n,8)) > cmon->mhpmax)
					cmon->mhp = cmon->mhpmax;
				if (canseemon(cmon))
					pline("%s looks better.", Monnam(mtmp));
			}
		}
		if(mtmp->mtame && ((Upolyd && u.mh<u.mhmax) || (!Upolyd && u.uhp < u.uhpmax)) && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			healup(d(n,8), 0, FALSE, FALSE);
			You("feel better.");
		}
	    dmg = 0;
	}break;
	case MON_PROTECTION:{
		int x, y, n, prot;
		struct monst *cmon;
		if(!mtmp){
			x = (int) u.ux;
			y = (int) u.uy;
			n = 8;
		} else {
			if(mtmp->mux == 0 && mtmp->muy == 0){
				x = (int) mtmp->mx;
				y = (int) mtmp->my;
			} else {
				x = (int) mtmp->mux;
				y = (int) mtmp->muy;
			}
			n = min(MAX_BONUS_DICE, mtmp->m_lev/3+1)+3;
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon != mtmp &&
				!DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful == mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				prot = -1*rnd(n);
				cmon->mstdy = max(prot, cmon->mstdy+prot);
				if (canseemon(cmon))
					pline("A shimmering shield surrounds %s!", mon_nam(cmon));
			}
		}
		if(mtmp->mtame && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			const char *hgolden = hcolor(NH_GOLDEN);
			if (u.uspellprot)
				pline_The("%s haze around you becomes more dense.",
					  hgolden);
			else
				pline_The("%s around you begins to shimmer with %s haze.",
					  (Underwater || Is_waterlevel(&u.uz)) ? "water" :
					   u.uswallow ? mbodypart(u.ustuck, STOMACH) :
					  IS_STWALL(levl[u.ux][u.uy].typ) ? "stone" : "air",
					  an(hgolden));
			u.uspellprot = max(rnd(n),u.uspellprot);
			u.uspmtime = max(1,u.uspmtime);
			if (!u.usptime)
				u.usptime = u.uspmtime;
			find_ac();
		}
	    dmg = 0;
	}break;
	case RECOVER:
		if(!mtmp) goto openwounds;
		if(!mtmp->perminvis) mtmp->minvis = 0;
		if(mtmp->permspeed == MSLOW) mtmp->permspeed = 0;
		mtmp->mcan = 0;
		mtmp->mcrazed = 0; 
		mtmp->mcansee = 1;
		mtmp->mblinded = 0;
		mtmp->mcanmove = 1;
		mtmp->mfrozen = 0;
		mtmp->msleeping = 0;
		mtmp->mstun = 0;
		mtmp->mconf = 0;
	    if (canseemon(mtmp))
			pline("%s looks recovered.", Monnam(mtmp));
	    dmg = 0;
	break;
    case OPEN_WOUNDS:
	openwounds:
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    dmg = (dmg + 1) / 2;
		if( dmg > 30) dmg = 30;
	} else if( dmg > 60) dmg = 60;
	if(dmg < (Upolyd ? u.mh : u.uhp)){
		if (dmg <= 5)
		    Your("skin itches badly for a moment.");
		else if (dmg <= 15)
		    pline("Wounds appear on your body!");
		else if (dmg <= 30)
		    pline("Severe wounds appear on your body!");
		else
		    Your("body is covered with deep, painful wounds!");
	}
	else{
		Your("body is covered with deadly wounds!");
		dmg = max(Upolyd ? u.mh - 5 : u.uhp -5, 0);
	}
	stop_occupation();
	break;
    case PSI_BOLT:
	psibolt:
	/* prior to 3.4.0 Antimagic was setting the damage to 1--this
	   made the spell virtually harmless to players with magic res. */
	if (Antimagic) {
	    shieldeff(u.ux, u.uy);
	    dmg = (dmg + 1) / 2;
		if(dmg > 25) dmg = 25;
	}
	if (dmg <= 5)
	    You("get a slight %sache.", body_part(HEAD));
	else if (dmg <= 10)
	    Your("brain is on fire!");
	else if (dmg <= 20)
	    Your("%s suddenly aches painfully!", body_part(HEAD));
	else{
	    Your("%s suddenly aches very painfully!", body_part(HEAD));
		if(dmg > 50) dmg = 50;
	}
	stop_occupation();
	break;
    default:
       impossible("mcastu: invalid magic spell (%d)", spellnum);
	dmg = 0;
	break;
    }

    if (dmg) mtmp ? mdamageu(mtmp, dmg) : losehp(dmg, "malevolent spell", KILLED_BY_AN);;
}

STATIC_DCL
boolean
is_undirected_spell(spellnum)
int spellnum;
{
	switch (spellnum) {
	case CLONE_WIZ:
	case RAISE_DEAD:
	case MON_TIME_STOP:
	case SUMMON_ANGEL:
	case SUMMON_ALIEN:
	case SUMMON_DEVIL:
	case SUMMON_MONS:
	case DISAPPEAR:
	case HASTE_SELF:
	case CURE_SELF:
	case INSECTS:
	case MASS_HASTE:
	case MASS_CURE_CLOSE:
	case MASS_CURE_FAR:
	case MON_PROTECTION:
	    return TRUE;
	default:
	    break;
	}
    return FALSE;
}

STATIC_DCL
boolean
is_aoe_spell(spellnum)
int spellnum;
{
	switch (spellnum) {
	case MON_FIRA:
	case MON_FIRAGA:
	case MON_BLIZZARA:
	case MON_BLIZZAGA:
	case MON_THUNDARA:
	case MON_THUNDAGA:
	case MON_FLARE:
	case MON_POISON_GAS:
	case MASS_CURE_FAR:
	case MON_PROTECTION:
	case VULNERABILITY:
	    return TRUE;
	default:
	    break;
	}
    return FALSE;
}

/* Some spells are useless under some circumstances. */
STATIC_DCL
boolean
spell_would_be_useless(mtmp, spellnum)
struct monst *mtmp;
int spellnum;
{
	int wardAt = ward_at(mtmp->mux,mtmp->muy);
	struct monst *tmpm;
    /* Some spells don't require the player to really be there and can be cast
     * by the monster when you're invisible, yet still shouldn't be cast when
     * the monster doesn't even think you're there.
     * This check isn't quite right because it always uses your real position.
     * We really want something like "if the monster could see mux, muy".
     */
    boolean mcouldseeu = couldsee(mtmp->mx, mtmp->my);
	
	/*Don't cast at warded spaces*/
	if(onscary(mtmp->mux, mtmp->muy, mtmp) && !is_undirected_spell(spellnum))
		return TRUE;
	
	if(spellnum == DEATH_TOUCH && (wardAt == CIRCLE_OF_ACHERON || wardAt == HEPTAGRAM || wardAt == HEXAGRAM))
		return TRUE;
	
	//Nothing to come
	if((Role_if(PM_ANACHRONONAUT) && In_quest(&u.uz)) 
		&& (spellnum == SUMMON_MONS
			|| spellnum == SUMMON_ANGEL
			|| spellnum == SUMMON_DEVIL
		)
	) return TRUE;
	
	//Raised dead would be hostile to the player
	if(mtmp->mtame && spellnum == RAISE_DEAD) 
		return TRUE;
		
	
	if(is_drow(mtmp->data) && !(Role_if(PM_ANACHRONONAUT) && In_quest(&u.uz))){
		if(!Race_if(PM_DROW)){
			if(sengr_at("Elbereth", mtmp->mux, mtmp->muy)) return TRUE;
		} else {
			if(sengr_at("Lolth", mtmp->mux, mtmp->muy) && (mtmp->m_lev < u.ulevel || u.ualign.record-- > 0)) return TRUE;
		}
	}
    /* only the wiz makes a clone */
	if ((!mtmp->iswiz || flags.no_of_wizards > 1) && spellnum == CLONE_WIZ) return TRUE;
	/* aggravate monsters won't be cast by peaceful monsters */
	if (mtmp->mpeaceful && (spellnum == AGGRAVATION))
	    return TRUE;
       /* don't make "ruler" monsters cast ineffective spells */
	if (is_prince(mtmp->data) && 
		(((spellnum == MAGIC_MISSILE || spellnum == DESTRY_ARMR ||
		spellnum == PARALYZE || spellnum == CONFUSE_YOU ||
		spellnum == STUN_YOU || spellnum == DRAIN_ENERGY) && Antimagic) ||
		((spellnum == MAGIC_MISSILE || spellnum == SLEEP ||
		spellnum == CONE_OF_COLD || spellnum == LIGHTNING) && Reflecting) ||
		((spellnum == STUN_YOU || spellnum == PARALYZE) && Free_action) ||
		(spellnum == FIRE_PILLAR && Fire_resistance) ||
		(spellnum == CONE_OF_COLD && Cold_resistance) ||
		(spellnum == SLEEP && Sleep_resistance) ||
		(spellnum == LIGHTNING && Shock_resistance) ||
		(spellnum == ACID_RAIN && Acid_resistance) ||
		(spellnum == DRAIN_LIFE && Drain_resistance) ||
		(spellnum == PLAGUE && Sick_resistance) ||
		(spellnum == WEAKEN_YOU && Fixed_abil)))
	  return TRUE;
       /* the wiz won't use the following cleric-specific or otherwise weak spells */
       if (mtmp->iswiz && (spellnum == SUMMON_SPHERE || spellnum == DARKNESS ||
		spellnum == PUNISH || spellnum == INSECTS ||
		spellnum == SUMMON_ANGEL || spellnum == DROP_BOULDER))
	  return TRUE;
       /* ray attack when monster isn't lined up */
       if(!lined_up(mtmp) && (spellnum == MAGIC_MISSILE ||
              spellnum == SLEEP || spellnum == CONE_OF_COLD || spellnum == LIGHTNING))
          return TRUE;
       /* drain energy when you have less than 5 */
       if(spellnum == DRAIN_ENERGY && u.uen < 5) return TRUE;
       /* don't cast acid rain if the player is petrifying */
       if (spellnum == ACID_RAIN && (Stoned || Golded)) return TRUE;
       /* don't cast drain life if not in range */
       if (spellnum == DRAIN_LIFE &&  distmin(mtmp->mx,mtmp->my,u.ux,u.uy) < 2) return TRUE;
       /* don't destroy weapon if not wielding anything */
       if (spellnum == DESTRY_WEPN && !uwep) return TRUE;
       /* don't do strangulation if already strangled or there's no room in inventory */
       if (((inv_cnt() == 52 && (!uamul || uamul->oartifact ||
       uamul->otyp == AMULET_OF_YENDOR)) || Strangled) && spellnum == STRANGLE)
           return TRUE;
       /* sleep ray when already asleep */
       if (Sleeping && spellnum == SLEEP)
          return TRUE;
       /* hallucination when already hallucinating */
       if ((u.umonnum == PM_BLACK_LIGHT || u.umonnum == PM_VIOLET_FUNGUS ||
               Hallucination) && spellnum == NIGHTMARE)
           return TRUE;
       /* turn to stone when already/can't be stoned */
       if ((Stoned || Stone_resistance) && spellnum == TURN_TO_STONE)
           return TRUE;
       /* earthquake where the pits won't be effective */
       if ((Underwater || Levitation || Flying || In_endgame(&u.uz) || 
               (u.utrap && u.utraptype == TT_PIT)) && spellnum == EARTHQUAKE)
           return TRUE;
       /* various conditions where webs won't be effective */
       if ((levl[u.ux][u.uy].typ <= IRONBARS || levl[u.ux][u.uy].typ > ICE ||
               t_at(u.ux,u.uy) || amorphous(youracedata) ||
               is_whirly(youracedata) || flaming(youracedata) ||
               unsolid(youracedata) || (uwep && uwep->oartifact == ART_STING) ||
               ACURR(A_STR) >= 18) && spellnum == MAKE_WEB)
           return TRUE;
       /* don't summon spheres when one type is gone */
       if (spellnum == SUMMON_SPHERE && !In_quest(&u.uz) &&
               ((mvitals[PM_FLAMING_SPHERE].mvflags & G_GONE) ||
                (mvitals[PM_FREEZING_SPHERE].mvflags & G_GONE) ||
                (mvitals[PM_SHOCKING_SPHERE].mvflags & G_GONE)))
	    return TRUE;
	/* haste self when already fast */
	if (mtmp->permspeed == MFAST && spellnum == HASTE_SELF)
	    return TRUE;
	/* invisibility when already invisible */
	if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == DISAPPEAR)
	    return TRUE;
	/* peaceful monster won't cast invisibility if you can't see invisible,
	   same as when monsters drink potions of invisibility.  This doesn't
	   really make a lot of sense, but lets the player avoid hitting
	   peaceful monsters by mistake */
	if (mtmp->mpeaceful && !See_invisible(u.ux, u.uy) && spellnum == DISAPPEAR)
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && spellnum == CURE_SELF)
	    return TRUE;
	if (mtmp->mhp == mtmp->mhpmax && (spellnum == MASS_CURE_CLOSE || spellnum == MASS_CURE_FAR)){
		if(mtmp->mtame && (Upolyd ? (u.mh < u.mhmax) : (u.uhp < u.uhpmax)))
			return FALSE;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if((mtmp->mtame == tmpm->mtame || mtmp->mpeaceful == tmpm->mpeaceful)
			&& !mm_aggression(mtmp, tmpm)
			&& tmpm->mhp < tmpm->mhpmax
			) return FALSE;
		}
		return TRUE;
	}
	/* protection when no foes are nearby and not weakened */
	if (spellnum == MON_PROTECTION){
		if(mtmp->mhp < mtmp->mhpmax)
			return FALSE;
		if(!mtmp->mtame && !mtmp->mpeaceful)
			return FALSE;
		if(mtmp->mtame && (Upolyd ? (u.mh < u.mhmax) : (u.uhp < u.uhpmax)) && dist2(u.ux,u.uy,mtmp->mx,mtmp->my) <= 3*3+1)
			return FALSE;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(mtmp->mtame == tmpm->mtame || mtmp->mpeaceful == tmpm->mpeaceful){
				if( !mm_aggression(mtmp, tmpm)
				&& tmpm->mhp < tmpm->mhpmax
				&& dist2(u.ux,u.uy,mtmp->mx,mtmp->my) <= 3*3+1
				) return FALSE;
			} else if(mtmp->mtame != tmpm->mtame && mtmp->mpeaceful != tmpm->mpeaceful && distmin(mtmp->mx,mtmp->my,tmpm->mx,tmpm->my) <= 5)
				return FALSE;
		}
		return TRUE;
	}
	/* healing when stats are ok */
	if (spellnum == RECOVER && !(mtmp->mcan || mtmp->mcrazed || 
								!mtmp->mcansee || !mtmp->mcanmove || 
								!mtmp->msleeping || mtmp->mstun || 
								mtmp->mconf || mtmp->permspeed == MSLOW))
	    return TRUE;
       /* don't summon anything if it doesn't think you're around
          or the caster is peaceful */
       if ((!mcouldseeu || mtmp->mpeaceful) &&
             (spellnum == SUMMON_MONS || spellnum == INSECTS ||
               spellnum == SUMMON_SPHERE || spellnum == RAISE_DEAD ||
               spellnum == CLONE_WIZ || spellnum == SUMMON_ANGEL || 
			   spellnum == SUMMON_ALIEN || spellnum == SUMMON_DEVIL))
	    return TRUE;
       /* angels can't be summoned in Gehennom */
       if (spellnum == SUMMON_ANGEL && (In_hell(&u.uz)))
	    return TRUE;
       /* make visible spell by spellcaster with see invisible. */
       /* also won't cast it if your invisibility isn't intrinsic. */
	if ((!(HInvis & INTRINSIC) || perceives(mtmp->data))
		   && spellnum == MAKE_VISIBLE)
		return TRUE;
	/* blindness spell on blinded player */
	if (Blinded && spellnum == BLIND_YOU)
	    return TRUE;
    return FALSE;
}
/* return values:
 * 2: target died
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castmm(mtmp, mdef, mattk)
	register struct monst *mtmp;
	register struct monst *mdef;
	register struct attack *mattk;
{
	int	dmg, ml = mtmp->m_lev;
	int ret;
	int spellnum = 0, chance;
	int dmd, dmn;
	struct obj *mirror;

	if(mtmp->data->maligntyp < 0 && Is_illregrd(&u.uz)) return 0;
	
	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
	    int cnt = 40;

	    do {
               spellnum = choose_magic_special(mtmp, mattk->adtyp);
				if(!spellnum) return 0; //The monster's spellcasting code aborted the cast.
		/* not trying to attack?  don't allow directed spells */
	    } while(--cnt > 0 &&
		    mspell_would_be_useless(mtmp, mdef, spellnum));
	    if (cnt == 0) return 0;
	}

	/* monster unable to cast spells? */
	if(mtmp->mcan || (mtmp->mspec_used && !nospellcooldowns(mtmp->data)) || !ml) {
	    if (canseemon(mtmp) && couldsee(mtmp->mx, mtmp->my))
	    {
                char buf[BUFSZ];
		Sprintf1(buf, Monnam(mtmp));

		if (is_undirected_spell(spellnum))
	            pline("%s points all around, then curses.", buf);
		else
	            pline("%s points at %s, then curses.",
		          buf, mon_nam(mdef));

	    } else if ((!(moves % 4) || !rn2(4))) {
	        if (flags.soundok) Norep("You hear a mumbled curse.");
	    }
	    return(0);
	}

	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && !nospellcooldowns(mtmp->data)) {
	    mtmp->mspec_used = 10 - mtmp->m_lev;
	    if (mtmp->mspec_used < 2) mtmp->mspec_used = 2;
	}
	
	if(is_kamerel(mtmp->data)){
		/* find mirror */
		for (mirror = mtmp->minvent; mirror; mirror = mirror->nobj)
			if (mirror->otyp == MIRROR && !mirror->cursed)
				break;
	}
	
	if(!is_kamerel(mtmp->data) || !mirror){
		chance = 2;
		if(mtmp->mconf) chance += 8;
		if(u.uz.dnum == neutral_dnum && u.uz.dlevel <= sum_of_all_level.dlevel){
			if(u.uz.dlevel == sum_of_all_level.dlevel) chance -= 1;
			else if(u.uz.dlevel == spire_level.dlevel-1) chance += 10;
			else if(u.uz.dlevel == spire_level.dlevel-2) chance += 8;
			else if(u.uz.dlevel == spire_level.dlevel-3) chance += 6;
			else if(u.uz.dlevel == spire_level.dlevel-4) chance += 4;
			else if(u.uz.dlevel == spire_level.dlevel-5) chance += 2;
		}
		
		if((u.uz.dnum == neutral_dnum && u.uz.dlevel == spire_level.dlevel) || rn2(ml*2) < chance) {	/* fumbled attack */
			if (canseemon(mtmp) && flags.soundok)
			pline_The("air crackles around %s.", mon_nam(mtmp));
			return(0);
		}
	}
	if (cansee(mtmp->mx, mtmp->my) ||
	    canseemon(mtmp) ||
	    (!is_undirected_spell(spellnum) &&
	     (cansee(mdef->mx, mdef->my) || canseemon(mdef)))) {
            char buf[BUFSZ];
	    Sprintf(buf, " at ");
	    Strcat(buf, mon_nam(mdef));
	    pline("%s casts a spell%s!",
		  canspotmon(mtmp) ? Monnam(mtmp) : "Something",
		  is_undirected_spell(spellnum) ? "" : buf);
	}

	{
		dmd = 6;
		dmn = min(MAX_BONUS_DICE, ml/3+1);
		
		if(dmn > 15) dmn = 15;
		
		if (mattk->damd) dmd = (int)(mattk->damd);
		
		if (mattk->damn) dmn+= (int)(mattk->damn);

		if(dmn < 1) dmn = 1;
		
	    dmg = d(dmn, dmd);
	}
	
	ret = 1;

	switch (mattk->adtyp) {
		case AD_OONA:
			switch(u.oonaenergy){
				case AD_ELEC: goto elec_mm;
				case AD_FIRE: goto fire_mm;
				case AD_COLD: goto cold_mm;
				default: pline("Bad Oona spell type?");
			}
		break;
		case AD_RBRE:
			switch(rnd(3)){
				case 1: goto elec_mm;
				case 2: goto fire_mm;
				case 3: goto cold_mm;
			}
		break;
		case AD_ELEC:
elec_mm:
	        if (canspotmon(mdef))
		    pline("Lightning crackles around %s.", Monnam(mdef));
		if(resists_elec(mdef)) {
			shieldeff(mdef->mx, mdef->my);
	        if (canspotmon(mdef))
			   pline("But %s resists the effects.",
			        mhe(mdef));
			dmg = 0;
		}
		break;
	    case AD_FIRE:
fire_mm:
	        if (canspotmon(mdef))
		    pline("%s is enveloped in flames.", Monnam(mdef));
		if(resists_fire(mdef)) {
			shieldeff(mdef->mx, mdef->my);
	                if (canspotmon(mdef))
			    pline("But %s resists the effects.",
			        mhe(mdef));
			dmg = 0;
		}
		break;
	    case AD_COLD:
cold_mm:
	        if (canspotmon(mdef))
		    pline("%s is covered in frost.", Monnam(mdef));
		if(resists_fire(mdef)) {
			shieldeff(mdef->mx, mdef->my);
	                if (canspotmon(mdef))
			    pline("But %s resists the effects.",
			        mhe(mdef));
			dmg = 0;
		}
		break;
	    case AD_MAGM:
	        if (canspotmon(mdef))
		    pline("%s is hit by a shower of missiles!", Monnam(mdef));
		if(resists_magm(mdef)) {
			shieldeff(mdef->mx, mdef->my);
	                if (canspotmon(mdef))
			    pline_The("missiles bounce off!");
			dmg = 0;
		} //else dmg = d((int)mtmp->m_lev/2 + 1,6);
		break;
	    case AD_STAR:
		if (canspotmon(mdef))
		pline("%s is hit by a shower of silver stars!", Monnam(mdef));
		dmg /= 2;
		mdef->mspec_used += dmg;
		if(hates_silver(mdef->data)) dmg += d(dmn,20);
		if(resists_magm(mdef)) dmg /= 2;
		break;
	    default:
		{
	    /*aggravation is a special case;*/
		/*it's undirected but should still target the*/
		/*victim so as to aggravate you*/
		if (is_undirected_spell(spellnum)
			&& (spellnum != AGGRAVATION &&
		      spellnum != SUMMON_MONS)
		)
		   cast_spell(mtmp, dmg, spellnum);
		else
		    ucast_spell(mtmp, mdef, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}
	if (dmg > 0 && mdef->mhp > 0)
	{
	    mdef->mhp -= dmg;
	    if (mdef->mhp < 1) monkilled(mdef, "", mattk->adtyp);
	}
	if (mdef && mdef->mhp < 1) return 2;
	return(ret);
}

STATIC_DCL
boolean
mspell_would_be_useless(mtmp, mdef, spellnum)
struct monst *mtmp;
struct monst *mdef;
int spellnum;
{
	int wardAt = ward_at(mdef->mx, mdef->my);
	struct monst *tmpm;
	
	/*Don't cast at warded spaces*/
	if(onscary(mdef->mx, mdef->my, mtmp) && !is_undirected_spell(spellnum))
		return TRUE;
	
	if(spellnum == DEATH_TOUCH && (wardAt == CIRCLE_OF_ACHERON || wardAt == HEPTAGRAM || wardAt == HEXAGRAM))
		return TRUE;
	
	if(is_drow(mdef->data)){
		if(!is_drow(mtmp->data)){
			if(sengr_at("Elbereth", mdef->mux, mdef->muy)) return TRUE;
		} else {
			if(sengr_at("Lolth", mdef->mux, mdef->muy) && (mdef->m_lev < mtmp->m_lev)) return TRUE;
		}
	}
	
	if ((!mtmp->iswiz || flags.no_of_wizards > 1) && spellnum == CLONE_WIZ) 
		return TRUE;
 	/* haste self when already fast */
	if (mtmp->permspeed == MFAST && spellnum == HASTE_SELF)
	    return TRUE;
	/* invisibility when already invisible */
	if ((mtmp->minvis || mtmp->invis_blkd) && spellnum == DISAPPEAR)
	    return TRUE;
	/* healing when already healed */
	if (mtmp->mhp == mtmp->mhpmax && spellnum == CURE_SELF)
	    return TRUE;
	if (mtmp->mhp == mtmp->mhpmax && (spellnum == MASS_CURE_CLOSE || spellnum == MASS_CURE_FAR)){
		if(mtmp->mtame && (Upolyd ? (u.mh < u.mhmax) : (u.uhp < u.uhpmax)))
			return FALSE;
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if((mtmp->mtame == tmpm->mtame || mtmp->mpeaceful == tmpm->mpeaceful)
			&& !mm_aggression(mtmp, tmpm)
			&& tmpm->mhp < tmpm->mhpmax
			) return FALSE;
		}
		return TRUE;
	}
	/* healing when stats are ok */
	if (spellnum == RECOVER && !(mtmp->mcan || mtmp->mcrazed || 
								!mtmp->mcansee || !mtmp->mcanmove || 
								!mtmp->msleeping || mtmp->mstun || 
								mtmp->mconf || mtmp->permspeed == MSLOW))
			return TRUE;
	/* don't summon monsters if it doesn't think you're around */
#ifndef TAME_SUMMONING
	if(mtmp->mtame){
        if (spellnum == SUMMON_MONS)
	    return TRUE;
        if (spellnum == SUMMON_ANGEL)
	    return TRUE;
        if (spellnum == SUMMON_ALIEN)
	    return TRUE;
        if (spellnum == SUMMON_DEVIL)
	    return TRUE;
        if (spellnum == INSECTS)
	    return TRUE;
	}
#endif
	/* blindness spell on blinded player */
	if ((!haseyes(mdef->data) || mdef->mblinded) && spellnum == BLIND_YOU)
	    return TRUE;
    return FALSE;
}


STATIC_DCL
boolean
uspell_would_be_useless(mdef, spellnum)
struct monst *mdef;
int spellnum;
{
	struct monst *tmpm;
	/* do not check for wards on target if given no target */
	if (mdef)
	{
		int wardAt = ward_at(mdef->mx, mdef->my);

		/*Don't cast at warded spaces*/
		if (onscary(mdef->mx, mdef->my, &youmonst) && !is_undirected_spell(spellnum))
			return TRUE;

		if (spellnum == DEATH_TOUCH && (wardAt == CIRCLE_OF_ACHERON || wardAt == HEPTAGRAM || wardAt == HEXAGRAM))
			return TRUE;
	}
	
	/* PC drow can't be warded off this way */
	
	/* aggravate monsters, etc. won't be cast by peaceful monsters */
	if (spellnum == CLONE_WIZ)
	    return TRUE;
	/* haste self when already fast */
	if (Fast && spellnum == HASTE_SELF)
	    return TRUE;
	/* invisibility when already invisible */
	if ((HInvis & INTRINSIC) && spellnum == DISAPPEAR)
	    return TRUE;
	/* healing when already healed */
	if (u.mh == u.mhmax && spellnum == CURE_SELF)
	    return TRUE;
	if (u.mh == u.mhmax && (spellnum == MASS_CURE_CLOSE || spellnum == MASS_CURE_FAR)){
		for(tmpm = fmon; tmpm; tmpm = tmpm->nmon){
			if(tmpm->mtame
			&& tmpm->mhp < tmpm->mhpmax
			) return FALSE;
		}
		return TRUE;
	}
#ifndef TAME_SUMMONING
	if (spellnum == SUMMON_MONS)
	    return TRUE;
#endif
    return FALSE;
}
#endif /* OVLB */
#ifdef OVL0

/* convert 1..10 to 0..9; add 10 for second group (spell casting) */
#define ad_to_typ(k) (10 + (int)k - 1)

int
buzzmu(mtmp, mattk, ml)		/* monster uses spell (ranged) */
	register struct monst *mtmp;
	register struct attack  *mattk;
	int ml;
{
	int dmn = (int)(min(MAX_BONUS_DICE, ml/3+1));
	int type = mattk->adtyp;
	
	if(mattk->damn) dmn += mattk->damn;
	
	if(dmn < 1) dmn = 1;
	
	if(type == AD_RBRE){
		switch(rnd(3)){
			case 1:
				type = AD_ELEC;
			break;
			case 2:
				type = AD_COLD;
			break;
			case 3:
				type = AD_FIRE;
			break;
		}
	} else if(type == AD_OONA){
		type = u.oonaenergy;
	}

	
	/* don't print constant stream of curse messages for 'normal'
	   spellcasting monsters at range */
	if (type > AD_SPC2)
	    return(0);
	
	if (mtmp->mcan) {
	    cursetxt(mtmp, FALSE);
	    return(0);
	}
	if(lined_up(mtmp) && rn2(3)) {
	    nomul(0, NULL);
	    if(type && (type < 11)) { /* no cf unsigned >0 */
		if(canseemon(mtmp))
		    pline("%s zaps you with a %s!", Monnam(mtmp),
			  flash_type(type, SPBOOK_CLASS));
		buzz(type, SPBOOK_CLASS, FALSE, dmn,
		     mtmp->mx, mtmp->my, sgn(tbx), sgn(tby),0,0);
	    } else impossible("Monster spell %d cast", type-1);
	}
	return(1);
}

int
buzzmm(magr, mdef, mattk, ml)		/* monster uses spell (ranged) */
	struct monst *magr;
	struct monst *mdef;
	struct attack  *mattk;
	int ml;
{
	int dmn = (int)(min(MAX_BONUS_DICE, ml/3+1));
	int type = mattk->adtyp;
    char buf[BUFSZ];
	
	if(mattk->damn) dmn += mattk->damn;
	
	if(dmn < 1) dmn = 1;
	
	if(type == AD_RBRE){
		switch(rnd(3)){
			case 1:
				type = AD_ELEC;
			break;
			case 2:
				type = AD_COLD;
			break;
			case 3:
				type = AD_FIRE;
			break;
		}
	} else if(type == AD_OONA){
		type = u.oonaenergy;
	}

	
	/* don't print constant stream of curse messages for 'normal'
	   spellcasting monsters at range */
	if (type > AD_SPC2)
	    return(0);
	
	if (magr->mcan) {
	    cursetxt(magr, FALSE);
	    return(0);
	}
	if(mlined_up(magr, mdef, TRUE) && rn2(3)) {
	    // nomul(0, NULL);
	    if(type && (type < 11)) { /* no cf unsigned >0 */
		Sprintf(buf,"%s zaps ",Monnam(magr));
		Sprintf(buf,"%s%s with a ",buf,mon_nam(mdef));
		Sprintf(buf,"%s%s!",buf,flash_type(type, SPBOOK_CLASS));
		if(canseemon(magr)) pline1(buf);
		buzz(type, SPBOOK_CLASS, FALSE, dmn,
		     magr->mx, magr->my, sgn(mdef->mx - magr->mx), sgn(mdef->my - magr->my),0,0);
	    } else impossible("Monster spell %d cast", type-1);
	}
	return(1);
}

/* return values:
 * 2: target died
 * 1: successful spell
 * 0: unsuccessful spell
 */
int
castum(mtmp, mattk)
        register struct monst *mtmp; 
	register struct attack *mattk;
{
	int dmg, ml = mons[u.umonnum].mlevel;
	int ret;
	int spellnum = 0;
	boolean directed = FALSE;
	int dmd, dmn;

	/* Three cases:
	 * -- monster is attacking you.  Search for a useful spell.
	 * -- monster thinks it's attacking you.  Search for a useful spell,
	 *    without checking for undirected.  If the spell found is directed,
	 *    it fails with cursetxt() and loss of mspec_used.
	 * -- monster isn't trying to attack.  Select a spell once.  Don't keep
	 *    searching; if that spell is not useful (or if it's directed),
	 *    return and do something else. 
	 * Since most spells are directed, this means that a monster that isn't
	 * attacking casts spells only a small portion of the time that an
	 * attacking monster does.
	 */
	if ((mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) && ml) {
	    int cnt = 40;
		
	    if (!spellnum) do {
	        spellnum = choose_magic_special(&youmonst, mattk->adtyp);
             if(!spellnum) return 0; //The monster's spellcasting code aborted the cast.
//		/* not trying to attack?  don't allow directed spells */
//		if (!mtmp || mtmp->mhp < 1) {
//		    if (is_undirected_spell(spellnum) && 
//			!uspell_would_be_useless(mtmp, spellnum)) {
//		        break;
//		    }
//		}
	    } while(--cnt > 0 &&
	            ((!mtmp && !is_undirected_spell(spellnum))
		    || uspell_would_be_useless(mtmp, spellnum)));
	    if (cnt == 0) {
	        You("have no spells to cast right now!");
		return 0;
	    }
	}
	else if (!mtmp) {
		You("have no spells to cast right now!");
		return 0;
	}

	if (spellnum == AGGRAVATION && !mtmp)
	{
	    /* choose a random monster on the level */
	    int j = 0, k = 0;
	    for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	        if (!mtmp->mtame && !mtmp->mpeaceful) j++;
	    if (j > 0)
	    {
	        k = rn2(j); 
	        for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
	            if (!mtmp->mtame && !mtmp->mpeaceful)
		        if (--k < 0) break;
	    }
	}

	directed = mtmp && !is_undirected_spell(spellnum);

	/* unable to cast spells? */
	if(u.uen < ml) {
	    if (directed)
	        You("point at %s, then curse.", mon_nam(mtmp));
	    else
	        You("point all around, then curse.");
	    return(0);
	}

	if (mattk->adtyp == AD_SPEL || mattk->adtyp == AD_CLRC) {
	    u.uen -= ml;
	}

	if(rn2(ml*20) < (Confusion ? 100 : 20)) {	/* fumbled attack */
	    pline_The("air crackles around you.");
	    return(0);
	}

        You("cast a spell%s%s!",
	      directed ? " at " : "",
	      directed ? mon_nam(mtmp) : "");

/*
 *	As these are spells, the damage is related to the level
 *	of the monster casting the spell.
 */
	{
		dmd = 6;
		dmn = min(MAX_BONUS_DICE, ml/3+1);
		
		if(dmn > 15) dmn = 15;
		
		if (mattk->damd) dmd = (int)(mattk->damd);
		
		if (mattk->damn) dmn+= (int)(mattk->damn);
		
		if(dmn < 1) dmn = 1;
		
	    dmg = d(dmn, dmd);
	}

	ret = 1;

	switch (mattk->adtyp) {
		case AD_OONA:
			switch(u.oonaenergy){
				case AD_ELEC: goto elec_um;
				case AD_FIRE: goto fire_um;
				case AD_COLD: goto cold_um;
				default: pline("Bad Oona spell type?");
			}
		break;
		case AD_RBRE:
			switch(rnd(3)){
				case 1: goto elec_um;
				case 2: goto fire_um;
				case 3: goto cold_um;
			}
		break;
		case AD_ELEC:
elec_um:
	        if (canspotmon(mtmp))
		    pline("Lightning crackles around %s.", Monnam(mtmp));
		if(resists_elec(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
	        if (canspotmon(mtmp))
			   pline("But %s resists the effects.",
			        mhe(mtmp));
			dmg = 0;
		}
	    case AD_FIRE:
fire_um:
		pline("%s is enveloped in flames.", Monnam(mtmp));
		if(resists_fire(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
			pline("But %s resists the effects.",
			    mhe(mtmp));
			dmg = 0;
		}
		break;
	    case AD_COLD:
cold_um:
		pline("%s is covered in frost.", Monnam(mtmp));
		if(resists_cold(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
			pline("But %s resists the effects.",
			    mhe(mtmp));
			dmg = 0;
		}
		break;
	    case AD_MAGM:
		pline("%s is hit by a shower of missiles!", Monnam(mtmp));
		if(resists_magm(mtmp)) {
			shieldeff(mtmp->mx, mtmp->my);
			pline_The("missiles bounce off!");
			dmg = 0;
		}
		break;
	    case AD_STAR:
		pline("%s is hit by a shower of silver stars!", Monnam(mtmp));
		dmg /= 2;
		mtmp->mspec_used += dmg;
		if(hates_silver(mtmp->data)) dmg += d(dmn,20);
		if(resists_magm(mtmp)) dmg /= 2;
		break;
	    default:
	    {
		ucast_spell(&youmonst, mtmp, dmg, spellnum);
		dmg = 0; /* done by the spell casting functions */
		break;
	    }
	}

	if (mtmp && dmg > 0 && mtmp->mhp > 0)
	{
	    mtmp->mhp -= dmg;
	    if (mtmp->mhp < 1) killed(mtmp);
	}
	if (mtmp && mtmp->mhp < 1) return 2;

	return(ret);
}

extern NEARDATA const int nasties[];

/* monster wizard and cleric spellcasting functions */
/*
   If dmg is zero, then the monster is not casting at you.
   If the monster is intentionally not casting at you, we have previously
   called spell_would_be_useless() and spellnum should always be a valid
   undirected spell.
   If you modify either of these, be sure to change is_undirected_spell()
   and spell_would_be_useless().
 */
STATIC_OVL
void
ucast_spell(mattk, mtmp, dmg, spellnum)
struct monst *mattk;
struct monst *mtmp;
int dmg;
int spellnum;
{
    boolean resisted = FALSE;
    boolean yours = (mattk == &youmonst);

    if (dmg == 0 && !is_undirected_spell(spellnum)) {
	impossible("cast directed wizard spell (%d) with dmg=0?", spellnum);
	return;
    }

    if (mtmp && mtmp->mhp < 1)
    {
        impossible("monster already dead?");
	return;
    }
	if(spellnum == RAISE_DEAD) spellnum = CURE_SELF;
	else if(spellnum == SUMMON_ANGEL) spellnum = CURE_SELF;
	else if(spellnum == SUMMON_ALIEN) spellnum = CURE_SELF;
	else if(spellnum == SUMMON_DEVIL) spellnum = CURE_SELF;
	else if(spellnum == TIME_DUPLICATE) spellnum = MON_TIME_STOP;
	else if(spellnum == INSECTS) spellnum = CURE_SELF;
    switch (spellnum) {
    case DEATH_TOUCH:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("touch of death with no mtmp");
	    return;
	}
	if (yours){
		if(u.sealsActive&SEAL_BUER) goto uspsibolt; //not your fault.
	    pline("You're using the touch of death!");
	} else if (canseemon(mattk))
	{
	    char buf[BUFSZ];
	    Sprintf(buf, "%s%s", mtmp->mtame ? "Oh no, " : "",
	                         mhe(mattk));
	    if (!mtmp->mtame)
	        *buf = highc(*buf);

	    pline("%s's using the touch of death!", buf);
	}

	if (nonliving_mon(mtmp) || is_demon(mtmp->data)) {
	    if (yours || canseemon(mtmp))
	        pline("%s seems no deader than before.", Monnam(mtmp));
	} else if (!(resisted = (resists_magm(mtmp) || resists_death(mtmp) || resist(mtmp, 0, 0, FALSE))) ||
	           rn2(mons[u.umonnum].mlevel) > 12) {
            mtmp->mhp = -1;
	    if (yours) killed(mtmp);
	    else monkilled(mtmp, "", AD_SPEL);
	    return;
	} else {
	    if (resisted) shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp))
	    {
	        if (mtmp->mtame)
		    pline("Lucky for %s, it didn't work!", mon_nam(mtmp));
		else
	            pline("That didn't work...");
            }
	}
	dmg = 0;
	break;
    case CLONE_WIZ:
		goto uspsibolt; //probably never happens, but...
	break;
    case FILTH:
    {
       struct monst *mtmp2;
	   struct obj *otmp;
       long old;
	   if(!mtmp) break;
       pline("A cascade of filth pours onto %s!", mon_nam(mtmp));
       if ((otmp = MON_WEP(mtmp))) {
			if(!rn2(20)){
				if(canseemon(mtmp)) pline("%s %s is coated in gunk!", s_suffix(Monnam(mtmp)), xname(otmp));
				if(is_poisonable(otmp)){
					otmp->opoisoned = OPOISON_FILTH;
					if(otmp->otyp == VIPERWHIP) otmp->opoisonchrgs = 0;
				}
				otmp->greased = TRUE;
			}
			if (!otmp->cursed) {
				obj_extract_self(otmp);
				possibly_unwield(mtmp, FALSE);
				setmnotwielded(mtmp,otmp);
				obj_no_longer_held(otmp);
				place_object(otmp, mtmp->mx, mtmp->my);
				stackobj(otmp);
			}
       }
       if(haseyes(mtmp->data) && mattk && mattk->data != &mons[PM_DEMOGORGON] && rn2(3)) {
			mtmp->mcansee = 0;
			mtmp->mblinded = max(mtmp->mblinded, rn1(20, 9));
       }
       if (!resists_sickness(mtmp)) {
		  if(!rn2(10)){
			if (yours) killed(mtmp);
			else monkilled(mtmp, "", AD_SPEL);
			dmg = 0;
		  } else {
			dmg = rnd(10);
		  }
       } else dmg = 0;
	break;
	}
    case STRANGLE:
		goto uspsibolt; //monsters don't have breath stat
	break;
     case TURN_TO_STONE:
		if(!mtmp) break;
        if (!(poly_when_stoned(mtmp->data) && newcham(mtmp, &mons[PM_STONE_GOLEM], FALSE, TRUE))) {
           if(!resists_ston(mtmp) && !rn2(10) ){
			if (!munstone(mtmp, yours))
				minstapetrify(mtmp, yours);
		   }
		   else
			pline("%s stiffens momentarily.", Monnam(mtmp));
        }
		dmg = 0;
	 stop_occupation();
     break;
     case MAKE_VISIBLE:
		if(!mtmp) break;
		if(mtmp->minvis){
			mtmp->perminvis = 0;
			struct obj* otmp;
			for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj)
				if (otmp->owornmask && objects[otmp->otyp].oc_oprop == INVIS)
					break;
			if(!otmp) mtmp->minvis = 0;
			newsym(mtmp->mx, mtmp->my);
		}
       dmg = 0;
     break;
     case PLAGUE:
	   if(!mtmp) break;
       if (!resists_sickness(mtmp)) {
          	if(canseemon(mtmp)) pline("%s is afflicted with disease!", Monnam(mtmp));
		  if(!rn2(10)){
			if (yours) killed(mtmp);
			else monkilled(mtmp, "", AD_SPEL);
			dmg = 0;
		  } else {
			dmg = rnd(12);
		  }
       } else dmg = 0;
    break;
    case PUNISH:
		goto uspsibolt; //Punishment not implemented for monsters
	break;
    case EARTHQUAKE:
		pline_The("entire %s is shaking around you!",
               In_endgame(&u.uz) ? "plane" : "dungeon");
		if (mtmp) {
			if(yours) do_earthquake(mtmp->mx, mtmp->my, min(((int)u.ulevel - 1) / 3 + 1,24), min(((int)u.ulevel - 1) / 6 + 1, 8), FALSE, mattk);
			else do_earthquake(mtmp->mx, mtmp->my, min(((int)mattk->m_lev - 1) / 3 + 1,24), min(((int)mattk->m_lev - 1) / 6 + 1, 8), FALSE, mattk);
		} else {
			if(yours) do_earthquake(u.ux, u.uy, min(((int)u.ulevel - 1) / 3 + 1,24), min(((int)u.ulevel - 1) / 6 + 1, 8), FALSE, mattk);
			else do_earthquake(mattk->mx, mattk->my, min(((int)mattk->m_lev - 1) / 3 + 1,24), min(((int)mattk->m_lev - 1) / 6 + 1, 8), FALSE, mattk);
		}
		aggravate(); /* wake up without scaring */
		dmg = 0;
		stop_occupation();
		doredraw();
	break;
    case SUMMON_SPHERE:
    {
       /* For a change, let's not assume the spheres are together. : ) */
       int sphere = (!rn2(3) ? PM_FLAMING_SPHERE : (!rn2(2) ?
                             PM_FREEZING_SPHERE : PM_SHOCKING_SPHERE));
       boolean created = FALSE;
       struct monst *mpet;
	   if(!yours && !(mattk && mtmp && mattk->mtame != mtmp->mtame)) goto uspsibolt;
	   if(yours || mattk->mtame){
		   if (!(mvitals[sphere].mvflags & G_GONE && !In_quest(&u.uz)) &&
			(mpet = makemon(&mons[sphere],
				mtmp->mx, mtmp->my, MM_ADJACENTOK|MM_EDOG|NO_MINVENT)) != 0){
					if (canspotmon(mpet)) created++;
					initedog(mpet);
			}
		} else {
		   if (!(mvitals[sphere].mvflags & G_GONE && !In_quest(&u.uz)) &&
			(mpet = makemon(&mons[sphere],
				mtmp->mx, mtmp->my, MM_ADJACENTOK|NO_MINVENT)) != 0){
					if (canspotmon(mpet)) created++;
			}
		}
		if (created)
           pline("%s is created!",
                      Hallucination ? rndmonnam() : Amonnam(mpet));
       dmg = 0;
       break;
    }
    case SUMMON_MONS:
    {
	int count = 0;
        register struct monst *mpet;
		
		if(Role_if(PM_ANACHRONONAUT) && In_quest(&u.uz))
			break;
		
        if (!rn2(10) && Inhell) {
			if (yours) demonpet();
			else msummon(mattk);
		} else {
			register int i, j;
				int makeindex, tmp = 1;
			coord bypos;
			
			if(yours){
				tmp = (u.ulevel > 3) ? u.ulevel / 3 : 1;
				// if(!Upolyd) tmp = (u.ulevel > 3) ? u.ulevel / 3 : 1;
				// else
			} else {
				tmp = (mattk->m_lev > 3) ? mattk->m_lev / 3 : 1;
			}
			
			if (mtmp)
				bypos.x = mtmp->mx, bypos.y = mtmp->my;
			else if (yours)
				bypos.x = u.ux, bypos.y = u.uy;
			else
				bypos.x = mattk->mx, bypos.y = mattk->my;

			for (i = rnd(tmp); i > 0; --i){
				for(j=0; j<20; j++) {

					do {
						makeindex = pick_nasty();
					} while (attacktype(&mons[makeindex], AT_MAGC) &&
							 monstr[makeindex] >= monstr[u.umonnum]);
						if (yours &&
					!enexto(&bypos, u.ux, u.uy, &mons[makeindex]))
					continue;
						if (!yours &&
					!enexto(&bypos, mattk->mx, mattk->my, &mons[makeindex]))
					continue;
					if(mpet && (u.ualign.type == 0 ||
						mons[makeindex].maligntyp == 0 || 
						sgn(mons[makeindex].maligntyp) == sgn(u.ualign.type)) 
					) {
						if ((mpet = makemon(&mons[makeindex], 
									  bypos.x, bypos.y, 
						  (yours || mattk->mtame) ? MM_EDOG :
													MM_NOCOUNTBIRTH|NO_MINVENT)) != 0
						);
						else /* GENOD? */
							mpet = makemon((struct permonst *)0,
														bypos.x, bypos.y, (yours || mattk->mtame) ? MM_EDOG :
													MM_NOCOUNTBIRTH|NO_MINVENT);
						mpet->msleeping = 0;
						if (yours || mattk->mtame) initedog(mpet);
						else if (mattk->mpeaceful) mpet->mpeaceful = 1;
						else mpet->mpeaceful = mpet->mtame = 0;
						mpet->mvanishes = yours ? (u.ulevel+rnd(u.ulevel)) : mtmp ? (mtmp->m_lev+rnd(mtmp->m_lev)) : d(2,20);
						set_malign(mpet);
						count++;
						break;
					}
				}
			}

			const char *mappear =
				(count == 1) ? "A monster appears" : "Monsters appear";

			if (yours || canseemon(mtmp))
				pline("%s from nowhere!", mappear);
		}

	dmg = 0;
	break;
    }
    case AGGRAVATION:
	if (!mtmp || mtmp->mhp < 1) {
	    You_feel("lonely.");
	    return;
	}
	you_aggravate(mtmp);
	dmg = 0;
	break;
    case CURSE_ITEMS:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("curse spell with no mtmp");
	    return;
	}
	if (yours || canseemon(mtmp))
	    You_feel("as though %s needs some help.", mon_nam(mtmp));
	mrndcurse(mtmp);
	dmg = 0;
	break;
    case DESTRY_ARMR:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("destroy spell with no mtmp");
	    return;
	}
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp))
	        pline("A field of force surrounds %s!",
	               mon_nam(mtmp));
	} else {
            register struct obj *otmp = some_armor(mtmp), *otmp2;

#define oresist_disintegration(obj) \
		(objects[obj->otyp].oc_oprop == DISINT_RES || \
		 obj_resists(obj, 0, 90) || is_quest_artifact(obj))

	    if (otmp &&
	        !oresist_disintegration(otmp))
	    {
			long unwornmask = otmp->owornmask;
			if (yours || canseemon(mtmp)) pline("%s %s %s!",
				s_suffix(Monnam(mtmp)),
				xname(otmp),
				is_cloak(otmp)  ? "crumbles and turns to dust" :
				is_shirt(otmp)  ? "crumbles into tiny threads" :
				is_helmet(otmp) ? "turns to dust and is blown away" :
				is_gloves(otmp) ? "vanish" :
				is_boots(otmp)  ? "disintegrate" :
				is_shield(otmp) ? "crumbles away" :
								"turns to dust"
				);
			obj_extract_self(otmp);
			mtmp->misc_worn_check &= ~unwornmask;
			otmp->owornmask = 0L;
			update_mon_intrinsics(mtmp, otmp, FALSE, FALSE);
			obfree(otmp, (struct obj *)0);
 	    }
	    else if (yours || canseemon(mtmp))
	        pline("%s looks itchy.", Monnam(mtmp)); 
	}
	dmg = 0;
	break;
    case WEAKEN_YOU:		/* drain strength */
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("weaken spell with no mtmp");
	    return;
	}
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) {
	    shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp)) pline("%s looks momentarily weakened.", Monnam(mtmp));
	} else {
		dmg = rnd(4)*5;
	    if (mtmp->mhp < 1)
	    {
	        impossible("trying to drain monster that's already dead");
		return;
	    }
	    if (yours || canseemon(mtmp))
	        pline("%s suddenly seems weaker!", Monnam(mtmp));
            /* monsters don't have strength, so drain max hp instead */
	    mtmp->mhpmax -= dmg;
	    if ((mtmp->mhp -= dmg) <= 0) {
	        if (yours) killed(mtmp);
			else monkilled(mtmp, "", AD_SPEL);
		}
	}
	dmg = 0;
	break;
    case DISAPPEAR:		/* makes self invisible */
        if (!yours) {
	    impossible("ucast disappear but not yours?");
	    return;
	}
	if (!(HInvis & INTRINSIC)) {
	    HInvis |= FROMOUTSIDE;
	    if (!Blind && !BInvis) self_invis_message();
	    dmg = 0;
	} else
	    impossible("no reason for player to cast disappear spell?");
	break;
    case STUN_YOU:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("stun spell with no mtmp");
	    return;
	}
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp))
	        pline("%s seems momentarily disoriented.", Monnam(mtmp));
	} else {
	    
	    if (yours || canseemon(mtmp)) {
	        if (mtmp->mstun)
	            pline("%s struggles to keep %s balance.",
	 	          Monnam(mtmp), mhis(mtmp));
                else
	            pline("%s reels...", Monnam(mtmp));
	    }
	    mtmp->mstun = 1;
	}
	dmg = 0;
	break;
    case HASTE_SELF:
    if (!yours) {
	    impossible("ucast haste but not yours?");
	    return;
	}
        if (!(HFast & INTRINSIC))
	    You("are suddenly moving faster.");
	HFast |= INTRINSIC;
	dmg = 0;
	break;
    case CURE_SELF:
        if (!yours) impossible("ucast healing but not yours?");
		else if (u.mh < u.mhmax) {
			You("feel better.");
			if ((u.mh += d(3,6)) > u.mhmax)
			u.mh = u.mhmax;
			flags.botl = 1;
		}
		dmg = 0;
	break;
	case MASS_HASTE:{
		int x, y, n;
		struct monst *cmon;
		if(yours){
			x = (int) u.ux;
			y = (int) u.uy;
			n = min(MAX_BONUS_DICE, u.ulevel/3+1);
		} else {
			x = (int) mattk->mx;
			y = (int) mattk->my;
			n = min(MAX_BONUS_DICE, mattk->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( !DEADMONSTER(cmon) &&
				((mtmp && cmon->mpeaceful == mtmp->mpeaceful) ||
				 (!mtmp && !cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				mon_adjust_speed(cmon, 1, (struct obj *)0);
			}
		}
		if((yours || mattk->mtame) && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			if(!Very_fast)
				You("are suddenly moving %sfaster.",
					Fast ? "" : "much ");
			else {
				Your("%s get new energy.",
					makeplural(body_part(LEG)));
			}
			exercise(A_DEX, TRUE);
			incr_itimeout(&HFast, rn1(10, 100));
		}
	    dmg = 0;
	}break;
	case MASS_CURE_CLOSE:{
		int x, y, n;
		struct monst *cmon;
		if(yours){
			x = (int) u.ux;
			y = (int) u.uy;
			n = min(MAX_BONUS_DICE, u.ulevel/3+1);
		} else {
			x = (int) mattk->mx;
			y = (int) mattk->my;
			n = min(MAX_BONUS_DICE, mattk->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon->mhp < cmon->mhpmax && 
				!DEADMONSTER(cmon) &&
				((yours && cmon->mpeaceful) ||
				 (!yours && mattk && cmon->mpeaceful == mattk->mpeaceful) ||
				 (!mattk && cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				if((cmon->mhp += d(n,8)) > cmon->mhpmax)
					cmon->mhp = cmon->mhpmax;
				if (canseemon(cmon))
					pline("%s looks better.", Monnam(mtmp));
			}
		}
		if((yours || mattk->mtame) && ((Upolyd && u.mh<u.mhmax) || (!Upolyd && u.uhp < u.uhpmax)) && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			healup(d(n,8), 0, FALSE, FALSE);
			You("feel better.");
		}
	    dmg = 0;
	}break;
	case MASS_CURE_FAR:{
		int x, y, n;
		struct monst *cmon;
		if(yours){
			x = (int) u.ux;
			y = (int) u.uy;
			n = min(MAX_BONUS_DICE, u.ulevel/3+1);
		} else {
			if(mattk->mux == 0 && mattk->muy == 0){
				x = (int) mattk->mx;
				y = (int) mattk->my;
			} else {
				x = (int) mattk->mux;
				y = (int) mattk->muy;
			}
			n = min(MAX_BONUS_DICE, mattk->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon != mattk &&
				cmon->mhp<cmon->mhpmax && 
				!DEADMONSTER(cmon) &&
				((yours && cmon->mpeaceful) ||
				 (!yours && mattk && cmon->mpeaceful == mattk->mpeaceful) ||
				 (!mattk && cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				if((cmon->mhp += d(n,8)) > cmon->mhpmax)
					cmon->mhp = cmon->mhpmax;
				if (canseemon(cmon))
					pline("%s looks better.", Monnam(mtmp));
			}
		}
		if(!yours && mattk->mtame && ((Upolyd && u.mh<u.mhmax) || (!Upolyd && u.uhp < u.uhpmax)) && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			healup(d(n,8), 0, FALSE, FALSE);
			You("feel better.");
		}
	    dmg = 0;
	}break;
	case MON_PROTECTION:{
		int x, y, n, prot;
		struct monst *cmon;
		if(yours){
			x = (int) u.ux;
			y = (int) u.uy;
			n = min(MAX_BONUS_DICE, u.ulevel/3+1);
		} else {
			if(mattk->mux == 0 && mattk->muy == 0){
				x = (int) mattk->mx;
				y = (int) mattk->my;
			} else {
				x = (int) mattk->mux;
				y = (int) mattk->muy;
			}
			n = min(MAX_BONUS_DICE, mattk->m_lev/3+1);
		}
		for(cmon = fmon; cmon; cmon = cmon->nmon){
			if( cmon != mattk &&
				!DEADMONSTER(cmon) &&
				((yours && cmon->mpeaceful) ||
				 (!yours && mattk && cmon->mpeaceful == mattk->mpeaceful) ||
				 (!mattk && cmon->mpeaceful)) &&
				dist2(x,y,cmon->mx,cmon->my) <= 3*3+1
			){
				prot = -1*rnd(n);
				cmon->mstdy = max(prot, cmon->mstdy+prot);
				if (canseemon(cmon))
					pline("A shimmering shield surrounds %s!", mon_nam(cmon));
			}
		}
		if(!yours && mattk->mtame && dist2(x,y,u.ux,u.uy) <= 3*3+1){
			const char *hgolden = hcolor(NH_GOLDEN);
			if (u.uspellprot)
				pline_The("%s haze around you becomes more dense.",
					  hgolden);
			else
				pline_The("%s around you begins to shimmer with %s haze.",
					  (Underwater || Is_waterlevel(&u.uz)) ? "water" :
					   u.uswallow ? mbodypart(u.ustuck, STOMACH) :
					  IS_STWALL(levl[u.ux][u.uy].typ) ? "stone" : "air",
					  an(hgolden));
			u.uspellprot = max(rnd(n),u.uspellprot);
			u.uspmtime = max(1,u.uspmtime);
			if (!u.usptime)
				u.usptime = u.uspmtime;
			find_ac();
		}
	    dmg = 0;
	}break;
    case PSI_BOLT:
	default:
uspsibolt:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("psibolt spell with no mtmp");
	    return;
	}
	
	if(dmg > 50) dmg = 50;
	
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    dmg = (dmg + 1) / 2;
	}
	if (canseemon(mtmp))
	    pline("%s winces%s", Monnam(mtmp), (dmg <= 5) ? "." : "!");
	break;
    case GEYSER:{
		struct obj* boots;
		if (!mtmp || mtmp->mhp < 1) {
			impossible("geyser spell with no mtmp");
			return;
		}
		dmg = 0;
		boots = which_armor(mtmp, W_ARMF);
		static int mboots2 = 0;
		if (!mboots2) mboots2 = find_mboots();
		if(boots && boots->otyp == WATER_WALKING_BOOTS){
			if (yours || canseemon(mtmp)){
				pline("A sudden geyser erupts under %s's feet!", mon_nam(mtmp));
				if(mtmp->data->mmove >= 14) pline("%s puts the added monmentum to good use!", Monnam(mtmp));
				else if(mtmp->data->mmove <=10) pline("%s is knocked around by the geyser's force!", Monnam(mtmp));
				if(canseemon(mtmp)) makeknown(boots->otyp);
			}
			if(mtmp->data->mmove >= 25) mtmp->movement += 12;
			else if(mtmp->data->mmove >= 18) mtmp->movement += 8;
			else if(mtmp->data->mmove >= 14) mtmp->movement += 6;
			else if(mtmp->data->mmove <= 3) dmg = d(8, 6);
			else if(mtmp->data->mmove <= 6) dmg = d(4, 6);
			else if(mtmp->data->mmove <=10) dmg = rnd(6);
		} else {
			if (yours || canseemon(mtmp))
				pline("A sudden geyser slams into %s from nowhere!", mon_nam(mtmp));
			/* this is physical damage, not magical damage */
			dmg = d(8, 6);
			if(boots && boots->otyp != mboots2) water_damage(mtmp->minvent, FALSE, FALSE, FALSE, mtmp);
		}
	}break;
    case FIRE_PILLAR:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("firepillar spell with no mtmp");
	    return;
	}
	if (yours || canseemon(mtmp))
	    pline("A pillar of fire strikes all around %s!", mon_nam(mtmp));
	if (resists_fire(mtmp)) {
	    shieldeff(mtmp->mx, mtmp->my);
	    dmg = 0;
	} else
	    dmg = d(8, 6);
	(void) burnarmor(mtmp);
	destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
	destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
	destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
	(void) burn_floor_paper(mtmp->mx, mtmp->my, TRUE, FALSE);
	break;
    case LIGHTNING:
    {
	boolean reflects;
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("lightning spell with no mtmp");
	    return;
	}

	if (yours || canseemon(mtmp))
	    pline("A bolt of lightning strikes down at %s from above!",
	          mon_nam(mtmp));
	reflects = mon_reflects(mtmp, "It bounces off %s %s.");
	if (reflects || resists_elec(mtmp)) {
	    shieldeff(u.ux, u.uy);
	    dmg = 0;
	    if (reflects)
		break;
	} else
	    dmg = d(8, 6);
	destroy_mitem(mtmp, WAND_CLASS, AD_ELEC);
	destroy_mitem(mtmp, RING_CLASS, AD_ELEC);
	break;
    }
	case SILVER_RAYS:{
		int n = 0;
		char * rays;
		dmg = 0;
		if (!mtmp || mtmp->mhp < 1) {
			impossible("silver rays spell with no mtmp");
			return;
		}
		if(zap_hit(mtmp, 0, TRUE))
			n++;
		if(zap_hit(mtmp, 0, TRUE))
			n++;
		if(!n){
			if (yours || canseemon(mtmp))
				pline("Silver rays whiz past %s!",
					  mon_nam(mtmp));
			break;
		}
		if (n == 1)
			rays = "a ray";
		if (n >= 2)
			rays = "rays";
		if(hates_silver(mtmp->data)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n*2,20);
		} else if(!resists_fire(mtmp) && species_resists_cold(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = (d(n,20)*3+1)/2;
			destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
		} else if(!resists_cold(mtmp) && species_resists_fire(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is frozen by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = (d(n,20)*3+1)/2;
			destroy_mitem(mtmp, POTION_CLASS, AD_COLD);
		} else if(hates_unholy(mtmp->data)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by %s of unholy light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20) + d(n,9);
		} else if(hates_holy_mon(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by %s of holy light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20) + d(n,7);
		} else if(!resists_fire(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20);
			destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
		} else if(!resists_elec(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is shocked by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20);
			destroy_mitem(mtmp, WAND_CLASS, AD_ELEC);
			destroy_mitem(mtmp, RING_CLASS, AD_ELEC);
		} else if(!resists_cold(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is frozen by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20);
			destroy_mitem(mtmp, POTION_CLASS, AD_COLD);
		} else if(!resists_acid(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n,20);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
		} else {
			if (yours || canseemon(mtmp))
				pline("%s is pierced by %s of silver light!",
					  Monnam(mtmp), rays);
			dmg = d(n, 20);
		}
	}break;
	case GOLDEN_WAVE:
		dmg = 0;
		if (!mtmp || mtmp->mhp < 1) {
			impossible("golden wave spell with no mtmp");
			return;
		}
		if(!resists_fire(mtmp) && species_resists_cold(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by golden light!",
					  Monnam(mtmp));
			dmg = (d(2,12)*3+1)/2;
			destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
		} else if(!resists_cold(mtmp) && species_resists_fire(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is frozen by golden light!",
					  Monnam(mtmp));
			dmg = (d(2,12)*3+1)/2;
			destroy_mitem(mtmp, POTION_CLASS, AD_COLD);
		} else if(hates_silver(mtmp->data)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by golden light!",
					  Monnam(mtmp));
			dmg = d(2,12) + d(1,20);
		} else if(hates_unholy(mtmp->data)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by unholy light!",
					  Monnam(mtmp));
			dmg = d(2,12) + d(1,9);
		} else if(hates_holy(mtmp->data)){
			if (yours || canseemon(mtmp))
				pline("%s is seared by holy light!",
					  Monnam(mtmp));
			dmg = d(2,12) + d(1,7);
		} else if(!resists_fire(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by golden light!",
					  Monnam(mtmp));
			dmg = d(2,12);
			destroy_mitem(mtmp, SCROLL_CLASS, AD_FIRE);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
			destroy_mitem(mtmp, SPBOOK_CLASS, AD_FIRE);
		} else if(!resists_elec(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is shocked by golden light!",
					  Monnam(mtmp));
			dmg = d(2,12);
			destroy_mitem(mtmp, WAND_CLASS, AD_ELEC);
			destroy_mitem(mtmp, RING_CLASS, AD_ELEC);
		} else if(!resists_cold(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is frozen by golden light!",
					  Monnam(mtmp));
			dmg = d(2,12);
			destroy_mitem(mtmp, POTION_CLASS, AD_COLD);
		} else if(!resists_acid(mtmp)){
			if (yours || canseemon(mtmp))
				pline("%s is burned by golden light!",
					  Monnam(mtmp));
			dmg = d(2,12);
			destroy_mitem(mtmp, POTION_CLASS, AD_FIRE);
		} else {
			if (yours || canseemon(mtmp))
				pline("%s is slashed by golden light!",
					  Monnam(mtmp));
			dmg = d(2, 12);
		}
	break;
	case PRISMATIC_SPRAY:{
		int dx = 0, dy = 0;
		if (!mtmp || mtmp->mhp < 1) {
			impossible("prismatic spray spell with no mtmp");
			return;
		}
		dmg /= 10;
		if(dmg > 7) dmg = 7;
		for(; dmg; dmg--){
			int adtyp = 0;
			int color = 0;
			switch (rn2(7))
			{
				case 0: adtyp = AD_PHYS; color = EXPL_RED; break;
				case 1: adtyp = AD_FIRE; color = EXPL_FIERY; break;
				case 2: adtyp = AD_DRST; color = EXPL_YELLOW; break;
				case 3: adtyp = AD_ACID; color = EXPL_LIME; break;
				case 4: adtyp = AD_COLD; color = EXPL_BBLUE; break;
				case 5: adtyp = AD_ELEC; color = EXPL_MAGENTA; break;
				case 6: adtyp = AD_DISN; color = EXPL_MAGICAL; break;
			}
			explode(mtmp->mx + dx, mtmp->my + dy, adtyp, MON_EXPLODE, d(6, 6), color, 2);
				dx = rnd(3) - 2;
				dy = rnd(3) - 2;
				if (!isok(mtmp->mx + dx, mtmp->my + dy) ||
					IS_STWALL(levl[mtmp->mx + dx][mtmp->my + dy].typ)
				) {
					/* Spell is reflected back to center */
					dx = 0;
					dy = 0;
				}
		}
		dmg = 0;
	}break;
	case ACID_BLAST:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("acid blast spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx, mtmp->my, AD_ACID, MON_EXPLODE, dmg, EXPL_NOXIOUS, 1);
		dmg = 0;
	break;
	case MON_FIRA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("fira spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx, mtmp->my, AD_FIRE, MON_EXPLODE, dmg, EXPL_FIERY, 1);
		dmg = 0;
	break;
	case MON_FIRAGA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("firaga spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_FIRE, MON_EXPLODE, dmg/2, EXPL_FIERY, 1);
		dmg = 0;
	break;
	case MON_BLIZZARA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("blizzara spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx, mtmp->my, AD_COLD, MON_EXPLODE, dmg, EXPL_FROSTY, 1);
		dmg = 0;
	break;
	case MON_BLIZZAGA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("blizzaga spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2,EXPL_FROSTY, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2,EXPL_FROSTY, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_COLD, MON_EXPLODE, dmg/2,EXPL_FROSTY, 1);
		dmg = 0;
	break;
	case MON_THUNDARA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("thundara spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx, mtmp->my, AD_ELEC, MON_EXPLODE, dmg, EXPL_MAGICAL, 1);
		dmg = 0;
	break;
	case MON_THUNDAGA:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("thundaga spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_ELEC, MON_EXPLODE, dmg/2, EXPL_MAGICAL, 1);
		dmg = 0;
	break;
	case MON_FLARE:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("flare spell with no mtmp");
			return;
		}
		if(dmg > 60) dmg = 60;
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_FROSTY, 1);
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_FIERY, 1); 
		explode(mtmp->mx+rn2(3)-1, mtmp->my+rn2(3)-1, AD_PHYS, MON_EXPLODE, dmg/3, EXPL_MUDDY, 1); 
		explode(mtmp->mx, mtmp->my, AD_PHYS, MON_EXPLODE, dmg, EXPL_FROSTY, 2);
		dmg = 0;
	break;
	case MON_WARP:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("spacewarp spell with no mtmp");
			return;
		}
		if(dmg > 100) dmg = 100;
			if (yours || canseemon(mtmp))
				pline("Space around %s warps into deadly blades!", mon_nam(mtmp));
	break;
	case MON_POISON_GAS:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("poison gas spell with no mtmp");
			return;
		}
		if(!yours){
			flags.cth_attk=TRUE;//state machine stuff.
			create_gas_cloud(mtmp->mx, mtmp->my, rnd(3), rnd(3)+1);
			flags.cth_attk=FALSE;
		} else {
			create_gas_cloud(mtmp->mx, mtmp->my, rnd(3), rnd(3)+1);
		}
		dmg = 0;
	break;
	case SOLID_FOG:
		if (!mtmp || mtmp->mhp < 1) {
			impossible("solid fog spell with no mtmp");
			return;
		}
		if(!yours){
			flags.cth_attk=TRUE;//state machine stuff.
			create_fog_cloud(mtmp->mx, mtmp->my, 3, 8);
			flags.cth_attk=FALSE;
			if(mattk && mattk->data == &mons[PM_PLUMACH_RILMANI]) mattk->mcan = 1;
		} else {
			create_fog_cloud(mtmp->mx, mtmp->my, 3, 8);
		}
		dmg = 0;
		stop_occupation();
	break;
    case INSECTS:
	if (!mtmp || mtmp->mhp < 1) {
	    impossible("insect spell with no target");
	    return;
	}
      {
	/* Try for insects, and if there are none
	   left, go for (sticks to) snakes.  -3. */
	struct permonst *pm;
	struct monst *mtmp2 = (struct monst *)0;
	char let;
	boolean success;
	int i,j;
	coord bypos;
	int quan;
    
	if((yours && Race_if(PM_DROW)) || 
		(!yours && is_drow((mattk->data)) )
	){ /*summoning is determined by your actual race*/\
		j = 0;
		do pm = mkclass(S_SPIDER, G_NOHELL|G_HELL);
		while(!is_spider(pm) && j++ < 30);
		let = (pm ? S_SPIDER : S_SNAKE);
	} else {
		pm = mkclass(S_ANT,G_NOHELL|G_HELL);
		let = (pm ? S_ANT : S_SNAKE);
	}
	
	if(yours) quan = (mons[u.umonnum].mlevel < 2) ? 1 : 
	       rnd(mons[u.umonnum].mlevel / 2);
	else quan = (mattk->m_lev < 2) ? 1 : rnd((int)mattk->m_lev / 2);
	if (quan < 3) quan = 3;
	success = pm ? TRUE : FALSE;
	for (i = 0; i <= quan; i++) {
	    if (!enexto(&bypos, mtmp->mx, mtmp->my, mtmp->data)) break;
		if((yours && Race_if(PM_DROW)) || 
			(!yours && is_drow((mattk->data)) )
		){ /*summoning is determined by your actual race*/\
			j = 0;
			do pm = mkclass(S_SPIDER, G_NOHELL|G_HELL);
			while(!is_spider(pm) && j++ < 30);
			let = (pm ? S_SPIDER : S_SNAKE);
		} else {
			pm = mkclass(S_ANT,G_NOHELL|G_HELL);
			let = (pm ? S_ANT : S_SNAKE);
		}
	    if (pm && (mtmp2 = makemon(pm, bypos.x, bypos.y, NO_MM_FLAGS)) != 0) {
		success = TRUE;
		mtmp2->msleeping = 0;
		if (yours || mattk->mtame)
		    (void) tamedog(mtmp2, (struct obj *)0);
		else if (mattk->mpeaceful)
		    mattk->mpeaceful = 1;
		else mattk->mpeaceful = 0;

		set_malign(mtmp2);
	    }
	}

        if (yours)
	{
	    if (!success)
	        You("cast at a clump of sticks, but nothing happens.");
	    else if (let == S_SNAKE)
	        You("transforms a clump of sticks into snakes!");
	    else
	        You("summon %s!",let == S_SPIDER ? "arachnids" : "insects");
        } else if (canseemon(mtmp)) {
	    if (!success)
	        pline("%s casts at a clump of sticks, but nothing happens.",
		      Monnam(mattk));
	    else if (let == S_SNAKE)
	        pline("%s transforms a clump of sticks into snakes!",
		      Monnam(mattk));
	    else
	        pline("%s summons %s!", Monnam(mattk),let == S_SPIDER ? "arachnids" : "insects");
	}
	dmg = 0;
	break;
      }
    case BLIND_YOU:
        if (!mtmp || mtmp->mhp < 1) {
	    impossible("blindness spell with no mtmp");
	    return;
	}
	/* note: resists_blnd() doesn't apply here */
	if (!mtmp->mblinded &&
	    haseyes(mtmp->data)) {
	    if (!resists_blnd(mtmp)) {
	        int num_eyes = eyecount(mtmp->data);
	        pline("Scales cover %s %s!",
	          s_suffix(mon_nam(mtmp)),
		  (num_eyes == 1) ? "eye" : "eyes");

		  mtmp->mblinded = 127;
	    }
	    dmg = 0;

	} else
	    impossible("no reason for monster to cast blindness spell?");
	break;
    case PARALYZE:
        if (!mtmp || mtmp->mhp < 1) {
	    impossible("paralysis spell with no mtmp");
	    return;
	}
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp))
	        pline("%s stiffens briefly.", Monnam(mtmp));
	} else {
	    if (yours || canseemon(mtmp))
	        pline("%s is frozen in place!", Monnam(mtmp));
	    dmg = 4 + mons[u.umonnum].mlevel;
	    mtmp->mcanmove = 0;
	    mtmp->mfrozen = dmg;
	}
	dmg = 0;
	break;
    case CONFUSE_YOU:
        if (!mtmp || mtmp->mhp < 1) {
	    impossible("confusion spell with no mtmp");
	    return;
	}
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    if (yours || canseemon(mtmp))
	        pline("%s seems momentarily dizzy.", Monnam(mtmp));
	} else {
	    if (yours || canseemon(mtmp))
	        pline("%s seems %sconfused!", Monnam(mtmp),
	              mtmp->mconf ? "more " : "");
	    mtmp->mconf = 1;
	}
	dmg = 0;
	break;
    case OPEN_WOUNDS:
        if (!mtmp || mtmp->mhp < 1) {
	    impossible("wound spell with no mtmp");
	    return;
	}
	if( dmg > 60) dmg = 60;
	if (resists_magm(mtmp) || resist(mtmp, 0, 0, FALSE)) { 
	    shieldeff(mtmp->mx, mtmp->my);
	    dmg = (dmg + 1) / 2;
	}
	/* not canseemon; if you can't see it you don't know it was wounded */
	if (yours)
	{
	    if (dmg <= 5)
	        pline("%s looks itchy!", Monnam(mtmp));
	    else if (dmg <= 10)
	        pline("Wounds appear on %s!", mon_nam(mtmp));
	    else if (dmg <= 20)
	        pline("Severe wounds appear on %s!", mon_nam(mtmp));
	    else
	        pline("%s is covered in wounds!", Monnam(mtmp));
	}
	break;
    }
	
    if (dmg > 0 && mtmp->mhp > 0)
    {
        mtmp->mhp -= dmg;
        if (mtmp->mhp < 1) {
	    if (yours) killed(mtmp);
	    else monkilled(mtmp, "", AD_CLRC);
	}
    }
}

#endif /* OVL0 */

/*mcastu.c*/
