/*	SCCS Id: @(#)do_name.c	3.4	2003/01/14	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#ifdef OVLB
#include "artilist.h"
#else
STATIC_DCL struct artifact artilist[];
#endif

#ifdef OVLB

//STATIC_DCL void FDECL(do_oname, (struct obj *));//moved to extern.h
static void FDECL(getpos_help, (BOOLEAN_P,const char *));

extern const char what_is_an_unknown_object[];		/* from pager.c */

/* the response for '?' help request in getpos() */
static void
getpos_help(force, goal)
boolean force;
const char *goal;
{
    char sbuf[BUFSZ];
    boolean doing_what_is;
    winid tmpwin = create_nhwindow(NHW_MENU);

    Sprintf(sbuf, "Use [%s] to move the cursor to %s.",
	    iflags.num_pad ? "2468" : "hjkl", goal);
    putstr(tmpwin, 0, sbuf);
    putstr(tmpwin, 0, "Use [HJKL] to move the cursor 8 units at a time.");
    putstr(tmpwin, 0, "Or enter a background symbol (ex. <).");
    /* disgusting hack; the alternate selection characters work for any
       getpos call, but they only matter for dowhatis (and doquickwhatis) */
    putstr(tmpwin, 0, "Use m and M to select a monster.");
    putstr(tmpwin, 0, "Use @ to select yourself.");
    doing_what_is = (goal == what_is_an_unknown_object);
    Sprintf(sbuf, "Type a .%s when you are at the right place.",
            doing_what_is ? " or , or ; or :" : "");
    putstr(tmpwin, 0, sbuf);
    if (!force)
	putstr(tmpwin, 0, "Type Space or Escape when you're done.");
    putstr(tmpwin, 0, "");
    display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
}

struct _getpos_monarr {
    coord pos;
    long du;
};
static int getpos_monarr_len = 0;
static int getpos_monarr_idx = 0;
static struct _getpos_monarr *getpos_monarr_pos = NULL;

void
getpos_freemons()
{
    if (getpos_monarr_pos) free(getpos_monarr_pos);
    getpos_monarr_pos = NULL;
    getpos_monarr_len = 0;
}

static int
getpos_monarr_cmp(a, b)
     const void *a;
     const void *b;
{
    const struct _getpos_monarr *m1 = (const struct _getpos_monarr *)a;
    const struct _getpos_monarr *m2 = (const struct _getpos_monarr *)b;
    return (m1->du - m2->du);
}

void
getpos_initmons()
{
    struct monst *mtmp = fmon;
    if (getpos_monarr_pos) getpos_freemons();
    while (mtmp) {
	if (!DEADMONSTER(mtmp) && canspotmon(mtmp)) getpos_monarr_len++;
	mtmp = mtmp->nmon;
    }
    if (getpos_monarr_len) {
	int idx = 0;
	getpos_monarr_pos = (struct _getpos_monarr *)malloc(sizeof(struct _getpos_monarr) * getpos_monarr_len);
	mtmp = fmon;
	while (mtmp) {
	    if (!DEADMONSTER(mtmp) && canspotmon(mtmp)) {
		getpos_monarr_pos[idx].pos.x = mtmp->mx;
		getpos_monarr_pos[idx].pos.y = mtmp->my;
		getpos_monarr_pos[idx].du = distu(mtmp->mx, mtmp->my);
		idx++;
	    }
	    mtmp = mtmp->nmon;
	}
	qsort(getpos_monarr_pos, getpos_monarr_len, sizeof(struct _getpos_monarr), getpos_monarr_cmp);
    }
}

struct monst *
getpos_nextmon()
{
    if (!getpos_monarr_pos) {
	getpos_initmons();
	if (getpos_monarr_len < 1) return NULL;
	getpos_monarr_idx = -1;
    }
    if (getpos_monarr_idx >= -1 && getpos_monarr_idx < getpos_monarr_len) {
	struct monst *mon;
	getpos_monarr_idx = (getpos_monarr_idx + 1) % getpos_monarr_len;
	mon = m_at(getpos_monarr_pos[getpos_monarr_idx].pos.x,
		   getpos_monarr_pos[getpos_monarr_idx].pos.y);
	return mon;
    }
    return NULL;
}

struct monst *
getpos_prevmon()
{
    if (!getpos_monarr_pos) {
	getpos_initmons();
	if (getpos_monarr_len < 1) return NULL;
	getpos_monarr_idx = getpos_monarr_len;
    }
    if (getpos_monarr_idx >= 0 && getpos_monarr_idx <= getpos_monarr_len) {
	struct monst *mon;
	getpos_monarr_idx = (getpos_monarr_idx - 1);
	if (getpos_monarr_idx < 0) getpos_monarr_idx = getpos_monarr_len - 1;
	mon = m_at(getpos_monarr_pos[getpos_monarr_idx].pos.x,
		   getpos_monarr_pos[getpos_monarr_idx].pos.y);
	return mon;
    }
    return NULL;
}


int
getpos(cc, force, goal)
coord *cc;
boolean force;
const char *goal;
{
    int result = 0;
    int cx, cy, i, c;
    int sidx, tx, ty;
    boolean msg_given = TRUE;	/* clear message window by default */
    static const char pick_chars[] = ".,;:";
    const char *cp;
    const char *sdp;
    if(iflags.num_pad) sdp = ndir; else sdp = sdir;	/* DICE workaround */

    if (flags.verbose) {
	pline("(For instructions type a ?)");
	msg_given = TRUE;
    }
    cx = cc->x;
    cy = cc->y;
#ifdef CLIPPING
    cliparound(cx, cy);
#endif
    curs(WIN_MAP, cx,cy);
    flush_screen(0);
#ifdef MAC
    lock_mouse_cursor(TRUE);
#endif
    for (;;) {
	c = nh_poskey(&tx, &ty, &sidx);
	if (c == '\033') {
	    cx = cy = -10;
	    msg_given = TRUE;	/* force clear */
	    result = -1;
	    break;
	}
	if(c == 0) {
	    if (!isok(tx, ty)) continue;
	    /* a mouse click event, just assign and return */
	    cx = tx;
	    cy = ty;
	    break;
	}
	if ((cp = index(pick_chars, c)) != 0) {
	    /* '.' => 0, ',' => 1, ';' => 2, ':' => 3 */
	    result = cp - pick_chars;
	    break;
	}
	for (i = 0; i < 8; i++) {
	    int dx, dy;

	    if (sdp[i] == c) {
		/* a normal movement letter or digit */
		dx = xdir[i];
		dy = ydir[i];
	    } else if (sdir[i] == lowc((char)c)) {
		/* a shifted movement letter */
		dx = 8 * xdir[i];
		dy = 8 * ydir[i];
	    } else
		continue;

	    /* truncate at map edge; diagonal moves complicate this... */
	    if (cx + dx < 1) {
		dy -= sgn(dy) * (1 - (cx + dx));
		dx = 1 - cx;		/* so that (cx+dx == 1) */
	    } else if (cx + dx > COLNO-1) {
		dy += sgn(dy) * ((COLNO-1) - (cx + dx));
		dx = (COLNO-1) - cx;
	    }
	    if (cy + dy < 0) {
		dx -= sgn(dx) * (0 - (cy + dy));
		dy = 0 - cy;		/* so that (cy+dy == 0) */
	    } else if (cy + dy > ROWNO-1) {
		dx += sgn(dx) * ((ROWNO-1) - (cy + dy));
		dy = (ROWNO-1) - cy;
	    }
	    cx += dx;
	    cy += dy;
	    goto nxtc;
	}

	if(c == '?'){
	    getpos_help(force, goal);
	} else if (c == 'm' || c == 'M') {
	    struct monst *tmpmon = (c == 'm') ? getpos_nextmon() : getpos_prevmon();
	    if (tmpmon) {
		cx = tmpmon->mx;
		cy = tmpmon->my;
		goto nxtc;
	    }
	} else if (c == '@') {
	    cx = u.ux;
	    cy = u.uy;
	    goto nxtc;
	} else {
	    if (!index(quitchars, c)) {
		char matching[MAXPCHARS];
		int pass, lo_x, lo_y, hi_x, hi_y, k = 0;
		(void)memset((genericptr_t)matching, 0, sizeof matching);
		for (sidx = 1; sidx < MAXPCHARS; sidx++)
		    if (c == defsyms[sidx].sym || c == (int)showsyms[sidx])
			matching[sidx] = (char) ++k;
		if (k) {
		    for (pass = 0; pass <= 1; pass++) {
			/* pass 0: just past current pos to lower right;
			   pass 1: upper left corner to current pos */
			lo_y = (pass == 0) ? cy : 0;
			hi_y = (pass == 0) ? ROWNO - 1 : cy;
			for (ty = lo_y; ty <= hi_y; ty++) {
			    lo_x = (pass == 0 && ty == lo_y) ? cx + 1 : 1;
			    hi_x = (pass == 1 && ty == hi_y) ? cx : COLNO - 1;
			    for (tx = lo_x; tx <= hi_x; tx++) {
				k = levl[tx][ty].glyph;
				if (glyph_is_cmap(k) &&
					matching[glyph_to_cmap(k)]) {
				    cx = tx,  cy = ty;
				    if (msg_given) {
					clear_nhwindow(WIN_MESSAGE);
					msg_given = FALSE;
				    }
				    goto nxtc;
				}
			    }	/* column */
			}	/* row */
		    }		/* pass */
		    pline("Can't find dungeon feature '%c'.", c);
		    msg_given = TRUE;
		    goto nxtc;
		} else {
		    pline("Unknown direction: '%s' (%s).",
			  visctrl((char)c),
			  !force ? "aborted" :
			  iflags.num_pad ? "use 2468 or ." : "use hjkl or .");
		    msg_given = TRUE;
		} /* k => matching */
	    } /* !quitchars */
	    if (force) goto nxtc;
	    pline("Done.");
	    msg_given = FALSE;	/* suppress clear */
	    cx = -1;
	    cy = 0;
	    result = 0;	/* not -1 */
	    break;
	}
    nxtc:	;
#ifdef CLIPPING
	cliparound(cx, cy);
#endif
	curs(WIN_MAP,cx,cy);
	flush_screen(0);
    }
#ifdef MAC
    lock_mouse_cursor(FALSE);
#endif
    if (msg_given) clear_nhwindow(WIN_MESSAGE);
    cc->x = cx;
    cc->y = cy;
    getpos_freemons();
    return result;
}

struct monst *
christen_monst(mtmp, name)
struct monst *mtmp;
const char *name;
{
	int lth;
	struct monst *mtmp2;
	char buf[PL_PSIZ];

	/* dogname & catname are PL_PSIZ arrays; object names have same limit */
	lth = *name ? (int)(strlen(name) + 1) : 0;
	if(lth > PL_PSIZ){
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	if (lth == mtmp->mnamelth) {
		/* don't need to allocate a new monst struct */
		if (lth) Strcpy(NAME(mtmp), name);
		return mtmp;
	}
	mtmp2 = newmonst(mtmp->mxlth + lth);
	*mtmp2 = *mtmp;
	(void) memcpy((genericptr_t)mtmp2->mextra,
		      (genericptr_t)mtmp->mextra, mtmp->mxlth);
	mtmp2->mnamelth = lth;
	if (lth) Strcpy(NAME(mtmp2), name);
	replmon(mtmp,mtmp2);
	return(mtmp2);
}

int
do_mname()
{
	char buf[BUFSZ];
	coord cc;
	register int cx,cy;
	register struct monst *mtmp;
	char qbuf[QBUFSZ];

	if (Hallucination) {
		You("would never recognize it anyway.");
		return 0;
	}
	cc.x = u.ux;
	cc.y = u.uy;
	if (getpos(&cc, FALSE, "the monster you want to name") < 0 ||
			(cx = cc.x) < 0)
		return 0;
	cy = cc.y;

	if (cx == u.ux && cy == u.uy) {
#ifdef STEED
	    if (u.usteed && canspotmon(u.usteed))
		mtmp = u.usteed;
	    else {
#endif
		pline("This %s creature is called %s and cannot be renamed.",
		ACURR(A_CHA) > 14 ?
		(flags.female ? "beautiful" : "handsome") :
		"ugly",
		plname);
		return(0);
#ifdef STEED
	    }
#endif
	} else
	    mtmp = m_at(cx, cy);

	if (!mtmp || (!sensemon(mtmp) &&
			(!(cansee(cx,cy) || see_with_infrared(mtmp) || see_with_bloodsense(mtmp) || see_with_lifesense(mtmp) || see_with_senseall(mtmp) || see_with_earthsense(mtmp))
			|| mtmp->mundetected
			|| mtmp->m_ap_type == M_AP_FURNITURE
			|| mtmp->m_ap_type == M_AP_OBJECT
			|| (mtmp->minvis && !See_invisible(mtmp->mx, mtmp->my))))) {
		pline("I see no monster there.");
		return(0);
	}
	/* special case similar to the one in lookat() */
	(void) distant_monnam(mtmp, ARTICLE_THE, buf);
	Sprintf(qbuf, "What do you want to call %s?", buf);
	getlin(qbuf,buf);
	if(!*buf || *buf == '\033') return(0);
	/* strip leading and trailing spaces; unnames monster if all spaces */
	(void)mungspaces(buf);

	if (mtmp->data->geno & G_UNIQ)
	    pline("%s doesn't like being called names!", Monnam(mtmp));
	else
	    (void) christen_monst(mtmp, buf);
	return(0);
}

/*
 * This routine changes the address of obj. Be careful not to call it
 * when there might be pointers around in unknown places. For now: only
 * when obj is in the inventory.
 */
void
do_oname(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[BUFSZ+BUFSZ];
	const char *aname;
	short objtyp;

	Sprintf(qbuf, "What do you want to name %s %s?",
		is_plural(obj) ? "these" : "this", xname(obj));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')	return;
	/* strip leading and trailing spaces; unnames item if all spaces */
	(void)mungspaces(buf);

	/* relax restrictions over proper capitalization for artifacts */
	if ((aname = artifact_name(buf, &objtyp)) != 0 && objtyp == obj->otyp)
		Strcpy(buf, aname);

	if (obj->oartifact) {
		pline_The("artifact seems to resist the attempt.");
		return;
	} else if (restrict_name(obj, buf) || exist_artifact(obj->otyp, buf)) {
		int n = rn2((int)strlen(buf));
		register char c1, c2;

		c1 = lowc(buf[n]);
		do c2 = 'a' + rn2('z'-'a'); while (c1 == c2);
		buf[n] = (buf[n] == c1) ? c2 : highc(c2);  /* keep same case */
//		pline("While engraving your %s slips.", body_part(HAND));
		display_nhwindow(WIN_MESSAGE, FALSE);
		You("christen it \"%s\".",buf);
	}
	obj = oname(obj, buf);
}

/*
 * Allocate a new and possibly larger storage space for an obj.
 */
struct obj *
realloc_obj(obj, oextra_size, oextra_src, oname_size, name)
struct obj *obj;
int oextra_size;		/* storage to allocate for oextra            */
genericptr_t oextra_src;
int oname_size;			/* size of name string + 1 (null terminator) */
const char *name;
{
	struct obj *otmp;

	otmp = newobj(oextra_size + oname_size);
	*otmp = *obj;	/* the cobj pointer is copied to otmp */
	if (oextra_size) {
	    if (oextra_src)
		(void) memcpy((genericptr_t)otmp->oextra, oextra_src,
							oextra_size);
	} else {
	    otmp->oattached = OATTACHED_NOTHING;
	}
	otmp->oxlth = oextra_size;

	otmp->onamelth = oname_size;
	otmp->timed = 0;	/* not timed, yet */
	otmp->lamplit = 0;	/* ditto */
	/* __GNUC__ note:  if the assignment of otmp->onamelth immediately
	   precedes this `if' statement, a gcc bug will miscompile the
	   test on vax (`insv' instruction used to store bitfield does
	   not set condition codes, but optimizer behaves as if it did).
	   gcc-2.7.2.1 finally fixed this. */
	if (oname_size) {
	    if (name)
		Strcpy(ONAME(otmp), name);
	}

	if (obj->owornmask) {
		boolean save_twoweap = u.twoweap;
		/* unwearing the old instance will clear dual-wield mode
		   if this object is either of the two weapons */
		setworn((struct obj *)0, obj->owornmask);
		setworn(otmp, otmp->owornmask);
		u.twoweap = save_twoweap;
	}

	/* replace obj with otmp */
	replace_object(obj, otmp);

	/* fix ocontainer pointers */
	if (Has_contents(obj)) {
		struct obj *inside;

		for(inside = obj->cobj; inside; inside = inside->nobj)
			inside->ocontainer = otmp;
	}

	/* move timers and light sources from obj to otmp */
	if (obj->timed) obj_move_timers(obj, otmp);
	if (obj->lamplit) obj_move_light_source(obj, otmp);

	/* objects possibly being manipulated by multi-turn occupations
	   which have been interrupted but might be subsequently resumed */
	if (obj->oclass == FOOD_CLASS)
	    food_substitution(obj, otmp);	/* eat food or open tin */
	else if (obj->oclass == SPBOOK_CLASS)
	    book_substitution(obj, otmp);	/* read spellbook */

	/* obfree(obj, otmp);	now unnecessary: no pointers on bill */
	dealloc_obj(obj);	/* let us hope nobody else saved a pointer */
	return otmp;
}

struct obj *
oname(obj, name)
struct obj *obj;
const char *name;
{
	int lth;
	char buf[PL_PSIZ];

	lth = *name ? (int)(strlen(name) + 1) : 0;
	if (lth > PL_PSIZ) {
		lth = PL_PSIZ;
		name = strncpy(buf, name, PL_PSIZ - 1);
		buf[PL_PSIZ - 1] = '\0';
	}
	/* If named artifact exists in the game, do not create another.
	 * Also trying to create an artifact shouldn't de-artifact
	 * it (e.g. Excalibur from prayer). In this case the object
	 * will retain its current name. */
	if (obj->oartifact || (lth && exist_artifact(obj->otyp, name)))
		return obj;
	
    if(!strcmp((&artilist[ART_SCALPEL_OF_LIFE_AND_DEATH])->name,name) &&
       obj && obj->otyp == SCALPEL){
      obj->ovar1 = COMMAND_DEATH;
    }
    if(((!strcmp((&artilist[ART_FIGURINE_OF_GALATEA])->name,name)) || (!strcmp((&artilist[ART_FIGURINE_OF_PYGMALION])->name,name))) &&
       obj && obj->otyp == FIGURINE){
      // obj->corpsenm = u.umonster*S*D*D*D*S*;
    }
	if(!strcmp((&artilist[ART_MANTLE_OF_HEAVEN])->name,name) &&
	   obj && obj->otyp == LEATHER_CLOAK){
		if(!Race_if(PM_VAMPIRE)) obj = poly_obj(obj,find_cope());
		else{
			obj = poly_obj(obj,find_opera_cloak());
			name = "The Vestment of Hell";
		}
	}
	else if(!strcmp((&artilist[ART_VESTMENT_OF_HELL])->name,name) &&
	   obj && obj->otyp == LEATHER_CLOAK){
		if(Race_if(PM_VAMPIRE)) obj = poly_obj(obj,find_opera_cloak());
		else{
			obj = poly_obj(obj,find_cope());
			name = "The Mantle of Heaven";
		}
	}
	else if(!strcmp((&artilist[ART_SILVER_KEY])->name,name) &&
	   obj && obj->otyp == UNIVERSAL_KEY && Role_if(PM_EXILE)){
		obj = poly_obj(obj,MIRROR);
		name = "The Hand-Mirror of Cthylla";
		lth = *name ? (int)(strlen(name) + 1) : 0;
		if (lth > PL_PSIZ) {
			lth = PL_PSIZ;
			name = strncpy(buf, name, PL_PSIZ - 1);
			buf[PL_PSIZ - 1] = '\0';
		}
	}
	
	if (lth == obj->onamelth) {
		/* no need to replace entire object */
		if (lth) Strcpy(ONAME(obj), name);
	} else {
		obj = realloc_obj(obj, obj->oxlth,
			      (genericptr_t)obj->oextra, lth, name);
	}
	
	if (lth) artifact_exists(obj, name, TRUE);
	
	if (obj->oartifact) {
		
		/*artifacts are unique, except the fluorite octet which is a set of 8*/
		if(obj->oartifact == ART_FLUORITE_OCTAHEDRON) obj->quan = 8L;
		else obj->quan = 1L;
		obj->owt = weight(obj);
		
		if(obj->oartifact == ART_WAR_MASK_OF_DURIN) obj->corpsenm = PM_DWARF;
		
		if(obj->oartifact == ART_GREEN_DRAGON_CRESCENT_BLAD) obj->owt = 150;
		

		if (obj->oartifact && artilist[obj->oartifact].size)
			obj->objsize = artilist[obj->oartifact].size;
		else if(is_nameable_artifact((&artilist[obj->oartifact])) || obj->oartifact == ART_EXCALIBUR)
			;//keep current/default size
		else
			obj->objsize = MZ_MEDIUM;
		
		if (obj->oartifact && artilist[obj->oartifact].material)  obj->obj_material = artilist[obj->oartifact].material;
		else if (obj->otyp == SABER)                              obj->obj_material = SILVER;
		else if (obj->otyp == KHAKKHARA)                          obj->obj_material = SILVER;
		else if (obj->otyp == CHAKRAM)                            obj->obj_material = SILVER;
		else if (obj->otyp == GLOVES)                             obj->obj_material = LEATHER;
		else if (obj->otyp == BAR)                                obj->obj_material = IRON;
		else if (obj->otyp == VIPERWHIP)                          obj->obj_material = SILVER;
		else if (obj->otyp == find_gcirclet())                    obj->obj_material = GOLD;
		else if (obj->otyp == ARMORED_BOOTS)                      obj->obj_material = COPPER;
		else if (obj->otyp == ROUNDSHIELD)                        obj->obj_material = COPPER;
		else if (obj->otyp == LIGHTSABER)                         obj->obj_material = SILVER;
		else if (obj->otyp == BEAMSWORD)                          obj->obj_material = GOLD;
		else if (obj->otyp == KAMEREL_VAJRA)                      obj->obj_material = GOLD;
		else if (obj->otyp == DOUBLE_LIGHTSABER)                  obj->obj_material = PLATINUM;
		else if (is_nameable_artifact((&artilist[obj->oartifact])) || obj->oartifact == ART_EXCALIBUR);//keep current/default material
		else
			obj->obj_material = objects[obj->otyp].oc_material;
		
		if(is_nameable_artifact((&artilist[obj->oartifact])) || obj->oartifact == ART_EXCALIBUR); //keep current/default body type
		else obj->bodytypeflag = MB_HUMANOID;
		
		if(obj->oartifact == ART_SCOURGE_OF_LOLTH) obj->ovar1 = 8;
		
		fix_object(obj);
		
	    /* can't dual-wield with artifact as secondary weapon */
	    if (obj == uswapwep) untwoweapon();
	    /* activate warning if you've just named your weapon "Sting" */
	    if (obj == uwep) set_artifact_intrinsic(obj, TRUE, W_WEP);
		if (carried(obj)) {
			/*
			 * We may need to do extra adjustments for the hero if we're
			 * messing with the hero's inventory.  The following calls are
			 * equivalent to calling freeinv on obj and addinv on obj,
			 * while doing an in-place swap of the actual objects.
			 */
			freeinv_core(obj);
			addinv_core1(obj);
			addinv_core2(obj);
		}
	}
	if (carried(obj)) update_inventory();
	return obj;
}

static NEARDATA const char callable[] = {
	SCROLL_CLASS, POTION_CLASS, WAND_CLASS, RING_CLASS, AMULET_CLASS,
	GEM_CLASS, SPBOOK_CLASS, ARMOR_CLASS, TOOL_CLASS, 0 };

int
ddocall()
{
	register struct obj *obj;
#ifdef REDO
	char	ch;
#endif
	char allowall[2];

	switch(
#ifdef REDO
		ch =
#endif
		ynq("Name an individual object?")) {
	case 'q':
		break;
	case 'y':
#ifdef REDO
		savech(ch);
#endif
		allowall[0] = ALL_CLASSES; allowall[1] = '\0';
		obj = getobj(allowall, "name");
		if(obj) do_oname(obj);
		break;
	default :
#ifdef REDO
		savech(ch);
#endif
		obj = getobj(callable, "call");
		if (obj) {
			/* behave as if examining it in inventory;
			   this might set dknown if it was picked up
			   while blind and the hero can now see */
			(void) xname(obj);

			if (!obj->dknown) {
				You("would never recognize another one.");
				return 0;
			}
			docall(obj);
		}
		break;
	}
	return 0;
}

void
docall(obj)
register struct obj *obj;
{
	char buf[BUFSZ], qbuf[QBUFSZ];
	struct obj otemp;
	register char **str1;

	if (!obj->dknown) return; /* probably blind */
	otemp = *obj;
	otemp.quan = 1L;
	otemp.onamelth = 0;
	otemp.oxlth = 0;
	if (objects[otemp.otyp].oc_class == POTION_CLASS && otemp.fromsink)
	    /* kludge, meaning it's sink water */
	    Sprintf(qbuf,"Call a stream of %s fluid:",
		    OBJ_DESCR(objects[otemp.otyp]));
	else
	    Sprintf(qbuf, "Call %s:", an(xname(&otemp)));
	getlin(qbuf, buf);
	if(!*buf || *buf == '\033')
		return;

	/* clear old name */
	str1 = &(objects[obj->otyp].oc_uname);
	if(*str1) free((genericptr_t)*str1);

	/* strip leading and trailing spaces; uncalls item if all spaces */
	(void)mungspaces(buf);
	if (!*buf) {
	    if (*str1) {	/* had name, so possibly remove from disco[] */
		/* strip name first, for the update_inventory() call
		   from undiscover_object() */
		*str1 = (char *)0;
		undiscover_object(obj->otyp);
	    }
	} else {
	    *str1 = strcpy((char *) alloc((unsigned)strlen(buf)+1), buf);
	    discover_object(obj->otyp, FALSE, TRUE); /* possibly add to disco[] */
	}
}

#endif /*OVLB*/
#ifdef OVL0

static const char * const ghostnames[] = {
	/* these names should have length < PL_NSIZ */
	/* Capitalize the names for aesthetics -dgk */
	"Adri", "Andries", "Andreas", "Bert", "David", "Dirk", "Emile",
	"Frans", "Fred", "Greg", "Hether", "Jay", "John", "Jon", "Karnov",
	"Kay", "Kenny", "Kevin", "Maud", "Michiel", "Mike", "Peter", "Robert",
	"Ron", "Tom", "Wilmar", "Nick Danger", "Phoenix", "Jiro", "Mizue",
	"Stephan", "Lance Braccus", "Shadowhawk"
};

/* ghost names formerly set by x_monnam(), now by makemon() instead */
const char *
rndghostname()
{
	return rn2(7) ? ghostnames[rn2(SIZE(ghostnames))] : (const char *)plname;
}

/* Monster naming functions:
 * x_monnam is the generic monster-naming function.
 *		  		seen		unseen			detected		  named
 * mon_nam:		the newt	it				the invisible orc	Fido
 * noit_mon_nam:the newt 	(as if detected)the invisible orc	Fido
 * l_monnam:	newt		it				invisible orc		dog called fido
 * Monnam:		The newt	It				The invisible orc	Fido
 * noit_Monnam: The newt 	(as if detected)The invisible orc	Fido
 * Adjmonnam:	The poor newt	It			The poor invisible orc	The poor Fido
 * Amonnam:		A newt		It				An invisible orc	Fido
 * a_monnam:	a newt		it				an invisible orc	Fido
 * m_monnam:	newt		xan				orc					Fido
 * y_monnam:	your newt   your xan		your invisible orc	Fido
 */

/* Bug: if the monster is a priest or shopkeeper, not every one of these
 * options works, since those are special cases.
 */
char *
x_monnam(mtmp, article, adjective, suppress, called)
register struct monst *mtmp;
int article;
/* ARTICLE_NONE, ARTICLE_THE, ARTICLE_A: obvious
 * ARTICLE_YOUR: "your" on pets, "the" on everything else
 *
 * If the monster would be referred to as "it" or if the monster has a name
 * _and_ there is no adjective, "invisible", "saddled", etc., override this
 * and always use no article.
 */
const char *adjective;
int suppress;
/* SUPPRESS_IT, SUPPRESS_INVISIBLE, SUPPRESS_HALLUCINATION, SUPPRESS_SADDLE.
 * EXACT_NAME: combination of all the above
 */
boolean called;
{
#ifdef LINT	/* static char buf[BUFSZ]; */
	char buf[BUFSZ];
#else
	static char buf[BUFSZ];
#endif
	struct permonst *mdat = mtmp->data;
	boolean do_hallu, do_invis, do_it, do_saddle;
	boolean name_at_start, has_adjectives;
	char *bp;

	if (program_state.gameover)
	    suppress |= SUPPRESS_HALLUCINATION;
	if (article == ARTICLE_YOUR && !mtmp->mtame)
	    article = ARTICLE_THE;

	do_hallu = Hallucination && !(suppress & SUPPRESS_HALLUCINATION);
	do_invis = mtmp->minvis && !(suppress & SUPPRESS_INVISIBLE);
	do_it = !canspotmon(mtmp) && 
	    article != ARTICLE_YOUR &&
	    !program_state.gameover &&
#ifdef STEED
	    mtmp != u.usteed &&
#endif
	    !(u.uswallow && mtmp == u.ustuck) &&
	    !(suppress & SUPPRESS_IT);
	do_saddle = !(suppress & SUPPRESS_SADDLE);

	buf[0] = 0;

	/* unseen monsters, etc.  Use "it" */
	if (do_it) {
	    Strcpy(buf, "it");
	    return buf;
	}

	/* priests and minions: don't even use this function */
	if (mtmp->ispriest || mtmp->isminion) {
	    char priestnambuf[BUFSZ];
	    char *name;
	    long save_prop = EHalluc_resistance;
	    unsigned save_invis = mtmp->minvis;

	    /* when true name is wanted, explicitly block Hallucination */
	    if (!do_hallu) EHalluc_resistance = 1L;
	    if (!do_invis) mtmp->minvis = 0;
	    name = priestname(mtmp, priestnambuf);
	    EHalluc_resistance = save_prop;
	    mtmp->minvis = save_invis;
	    if (article == ARTICLE_NONE && !strncmp(name, "the ", 4))
		name += 4;
	    return strcpy(buf, name);
	}

	/* Shopkeepers: use shopkeeper name.  For normal shopkeepers, just
	 * "Asidonhopo"; for unusual ones, "Asidonhopo the invisible
	 * shopkeeper" or "Asidonhopo the blue dragon".  If hallucinating,
	 * none of this applies.
	 */
	if (mtmp->isshk && !do_hallu) {
	    if (adjective && article == ARTICLE_THE) {
		/* pathological case: "the angry Asidonhopo the blue dragon"
		   sounds silly */
		Strcpy(buf, "the ");
		Strcat(strcat(buf, adjective), " ");
		Strcat(buf, shkname(mtmp));
		return buf;
	    }
	    Strcat(buf, shkname(mtmp));
	    if (mdat == &mons[PM_SHOPKEEPER] && !do_invis)
		return buf;
	    Strcat(buf, " the ");
	    if (do_invis) Strcat(buf, "invisible ");
		if (mtmp->mflee && mtmp->data == &mons[PM_BANDERSNATCH]){
			Strcat(buf, "frumious ");
			name_at_start = FALSE;
		}
		if (
			((u.sealsActive&SEAL_MOTHER && !is_undead_mon(mtmp)) || (Role_if(PM_HEALER) && (!nonliving_mon(mtmp) || has_blood_mon(mtmp)))) 
			&& !DEADMONSTER(mtmp)
		){
			if(mtmp->mhp == mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "uninjured ") : Strcat(buf, "undamaged ");
			else if(mtmp->mhp >= .9*mtmp->mhpmax) Strcat(buf, "scuffed ");
			else if(mtmp->mhp >= .5*mtmp->mhpmax) (has_blood_mon(mtmp)) ?  Strcat(buf, "bruised ") : Strcat(buf, "dented ");
			else if(mtmp->mhp >= .25*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "bloodied ") : Strcat(buf, "damaged ");
			else if(mtmp->mhp >= .1*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "badly bloodied ") : Strcat(buf, "badly damaged ");
			else if(mtmp->mhp > 0) (has_blood_mon(mtmp)) ? Strcat(buf, "mortally injured ") : Strcat(buf, "critically damaged ");
			name_at_start = FALSE;
		}
		if(is_drow(mdat)){
			struct obj *otmp;
			for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
				if ((otmp->otyp == DROVEN_PLATE_MAIL || otmp->otyp == DROVEN_CHAIN_MAIL || otmp->otyp == CONSORT_S_SUIT) 
					&& otmp->owornmask & mtmp->misc_worn_check){
						Sprintf(eos(buf), "%s ", getDrowHouse(otmp->oward));
						name_at_start = FALSE;
					}
			}
		}
	    Strcat(buf, mdat->mname);
		if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, "'s zombie");
		else if(mtmp->mfaction == SKELIFIED) Strcat(buf, "'s skeleton");
		else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, "'s vitrean");
		else if(mtmp->mfaction == FRACTURED) Strcat(buf, ", Witness of the Fracture");
	    return buf;
	}

	/* Put the adjectives in the buffer */
	if (adjective)
	    Strcat(strcat(buf, adjective), " ");
	if (do_invis)
	    Strcat(buf, "invisible ");
#ifdef STEED
	if (do_saddle && (mtmp->misc_worn_check & W_SADDLE) &&
	    !Blind && !Hallucination)
	    Strcat(buf, "saddled ");
#endif
	if (buf[0] != 0)
	    has_adjectives = TRUE;
	else
	    has_adjectives = FALSE;

	/* Put the actual monster name or type into the buffer now */
	/* Be sure to remember whether the buffer starts with a name */
	if (do_hallu) {
	    Strcat(buf, rndmonnam());
	    name_at_start = FALSE;
	} else if (mtmp->mnamelth) {
	    char *name = NAME(mtmp);

	    if (mdat == &mons[PM_GHOST]) {
			Sprintf(eos(buf), "%s ghost", s_suffix(name));
			name_at_start = TRUE;
	    } else if (mdat == &mons[PM_SHADE]) {
			Sprintf(eos(buf), "%s shade", s_suffix(name));
			name_at_start = TRUE;
	    } else if (mdat == &mons[PM_BROKEN_SHADOW]) {
			Sprintf(eos(buf), "%s broken shadow", s_suffix(name));
			name_at_start = TRUE;
	    } else if (called) {
			if (mtmp->mflee && mtmp->data == &mons[PM_BANDERSNATCH]){
				Sprintf(eos(buf), "frumious ");
				name_at_start = FALSE;
			}
			if ((u.sealsActive&SEAL_MOTHER && !is_undead_mon(mtmp)) || (Role_if(PM_HEALER) && (!nonliving_mon(mtmp) || has_blood_mon(mtmp))) ){
				if(mtmp->mhp == mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "uninjured ") : Strcat(buf, "undamaged ");
				else if(mtmp->mhp >= .9*mtmp->mhpmax) Strcat(buf, "scuffed ");
				else if(mtmp->mhp >= .5*mtmp->mhpmax) (has_blood_mon(mtmp)) ?  Strcat(buf, "bruised ") : Strcat(buf, "dented ");
				else if(mtmp->mhp >= .25*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "bloodied ") : Strcat(buf, "damaged ");
				else if(mtmp->mhp >= .1*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "badly bloodied ") : Strcat(buf, "badly damaged ");
				else if(mtmp->mhp > 0) (has_blood_mon(mtmp)) ? Strcat(buf, "mortally injured ") : Strcat(buf, "critically damaged ");
				name_at_start = FALSE;
			}
			if(is_drow(mdat)){
				struct obj *otmp;
				for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
					if ((otmp->otyp == DROVEN_PLATE_MAIL || otmp->otyp == DROVEN_CHAIN_MAIL || otmp->otyp == CONSORT_S_SUIT) 
						&& otmp->owornmask & mtmp->misc_worn_check){
							Sprintf(eos(buf), "%s ", getDrowHouse(otmp->oward));
							name_at_start = FALSE;
						}
				}
			}
			
			Sprintf(eos(buf), "%s", mdat->mname);
			if(type_is_pname(mdat)){
				if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, "'s zombie");
				else if(mtmp->mfaction == SKELIFIED) Strcat(buf, "'s skeleton");
				else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, "'s vitrean");
				else if(mtmp->mfaction == FRACTURED) Strcat(buf, ", Witness of the Fracture");
			} else {
				if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, " zombie");
				else if(mtmp->mfaction == SKELIFIED) Strcat(buf, " skeleton");
				else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, " vitrean");
				else if(mtmp->mfaction == FRACTURED) Strcat(buf, " witness");
			}
			Sprintf(eos(buf), " called %s", name);
			
			name_at_start = (boolean)type_is_pname(mdat);
	    } else if (is_mplayer(mdat) && (bp = strstri(name, " the ")) != 0) {
			/* <name> the <adjective> <invisible> <saddled> <rank> */
			char pbuf[BUFSZ];

			Strcpy(pbuf, name);
			pbuf[bp - name + 5] = '\0'; /* adjectives right after " the " */
			if (has_adjectives)
				Strcat(pbuf, buf);
			Strcat(pbuf, bp + 5);	/* append the rest of the name */
			Strcpy(buf, pbuf);
			article = ARTICLE_NONE;
			name_at_start = TRUE;
	    } else {
			Strcat(buf, name);
			name_at_start = TRUE;
	    }
	} else if (is_mplayer(mdat) && !In_endgame(&u.uz)) {
	    char pbuf[BUFSZ];
	    Strcpy(pbuf, rank_of((int)mtmp->m_lev,
				 monsndx(mdat),
				 (boolean)mtmp->female));
	    Strcat(buf, lcase(pbuf));
		if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, " zombie");
		else if(mtmp->mfaction == SKELIFIED) Strcat(buf, " skeleton");
		else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, " vitrean");
		else if(mtmp->mfaction == FRACTURED) Strcat(buf, " witness");
	    name_at_start = FALSE;
	} else {
	    name_at_start = (boolean)type_is_pname(mdat);
		if (mtmp->mflee && mtmp->data == &mons[PM_BANDERSNATCH]){
			Strcat(buf, "frumious ");
			name_at_start = FALSE;
		}
		if ((u.sealsActive&SEAL_MOTHER && !is_undead_mon(mtmp)) || (Role_if(PM_HEALER) && (!nonliving_mon(mtmp) || has_blood_mon(mtmp))) ){
			if(mtmp->mhp == mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "uninjured ") : Strcat(buf, "undamaged ");
			else if(mtmp->mhp >= .9*mtmp->mhpmax) Strcat(buf, "scuffed ");
			else if(mtmp->mhp >= .5*mtmp->mhpmax) (has_blood_mon(mtmp)) ?  Strcat(buf, "bruised ") : Strcat(buf, "dented ");
			else if(mtmp->mhp >= .25*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "bloodied ") : Strcat(buf, "damaged ");
			else if(mtmp->mhp >= .1*mtmp->mhpmax) (has_blood_mon(mtmp)) ? Strcat(buf, "badly bloodied ") : Strcat(buf, "badly damaged ");
			else if(mtmp->mhp > 0) (has_blood_mon(mtmp)) ? Strcat(buf, "mortally injured ") : Strcat(buf, "critically damaged ");
			name_at_start = FALSE;
		}
		if(is_drow(mdat)){
			struct obj *otmp;
			for (otmp = mtmp->minvent; otmp; otmp = otmp->nobj) {
				if ((otmp->otyp == DROVEN_PLATE_MAIL || otmp->otyp == DROVEN_CHAIN_MAIL || otmp->otyp == CONSORT_S_SUIT) 
					&& otmp->owornmask & mtmp->misc_worn_check){
						Sprintf(eos(buf), "%s ", getDrowHouse(otmp->oward));
						name_at_start = FALSE;
					}
			}
		}
	    Strcat(buf, mdat->mname);
		if(type_is_pname(mdat)){
			if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, "'s zombie");
			else if(mtmp->mfaction == SKELIFIED) Strcat(buf, "'s skeleton");
			else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, "'s vitrean");
			else if(mtmp->mfaction == FRACTURED) Strcat(buf, ", Witness of the Fracture");
		} else {
			if(mtmp->mfaction == ZOMBIFIED) Strcat(buf, " zombie");
			else if(mtmp->mfaction == SKELIFIED) Strcat(buf, " skeleton");
			else if(mtmp->mfaction == CRYSTALFIED) Strcat(buf, " vitrean");
			else if(mtmp->mfaction == FRACTURED) Strcat(buf, " witness");
		}
	}

	if (name_at_start && (article == ARTICLE_YOUR || !has_adjectives)) {
	    if (mdat == &mons[PM_WIZARD_OF_YENDOR])
		article = ARTICLE_THE;
	    else
		article = ARTICLE_NONE;
	} else if ((mdat->geno & G_UNIQ) && article == ARTICLE_A) {
	    article = ARTICLE_THE;
	}

	{
	    char buf2[BUFSZ];

	    switch(article) {
		case ARTICLE_YOUR:
		    Strcpy(buf2, "your ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_THE:
		    Strcpy(buf2, "the ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_A:
		    return(an(buf));
		case ARTICLE_NONE:
		default:
		    return buf;
	    }
	}
}

char *
x_ptrnam(ptr, article, adjective, called)
register struct permonst *ptr;
int article;
/* ARTICLE_NONE, ARTICLE_THE, ARTICLE_A: obvious
 * ARTICLE_YOUR: "your" on pets, "the" on everything else
 *
 * If the monster would be referred to as "it" or if the monster has a name
 * _and_ there is no adjective, "invisible", "saddled", etc., override this
 * and always use no article.
 */
const char *adjective;
boolean called;
{
#ifdef LINT	/* static char buf[BUFSZ]; */
	char buf[BUFSZ];
#else
	static char buf[BUFSZ];
#endif
	struct permonst *mdat = ptr;
	boolean name_at_start, has_adjectives;
	char *bp;

	buf[0] = 0;

	/* Put the adjectives in the buffer */
	if (adjective)
	    Strcat(strcat(buf, adjective), " ");
	if (buf[0] != 0)
	    has_adjectives = TRUE;
	else
	    has_adjectives = FALSE;

	/* Put the actual monster name or type into the buffer now */
	/* Be sure to remember whether the buffer starts with a name */
	{
	    Strcat(buf, mdat->mname);
	    name_at_start = (boolean)type_is_pname(mdat);
	}

	if (name_at_start && (article == ARTICLE_YOUR || !has_adjectives)) {
	    if (mdat == &mons[PM_WIZARD_OF_YENDOR])
		article = ARTICLE_THE;
	    else
		article = ARTICLE_NONE;
	} else if ((mdat->geno & G_UNIQ) && article == ARTICLE_A) {
	    article = ARTICLE_THE;
	}

	{
	    char buf2[BUFSZ];

	    switch(article) {
		case ARTICLE_YOUR:
		    Strcpy(buf2, "your ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_THE:
		    Strcpy(buf2, "the ");
		    Strcat(buf2, buf);
		    Strcpy(buf, buf2);
		    return buf;
		case ARTICLE_A:
		    return(an(buf));
		case ARTICLE_NONE:
		default:
		    return buf;
	    }
	}
}

#endif /* OVL0 */
#ifdef OVLB

char *
l_monnam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_NONE, (char *)0, 
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, TRUE));
}

#endif /* OVLB */
#ifdef OVL0

const char *
sheheit(mtmp)
register struct monst *mtmp;
{
	return (is_neuter(mtmp->data)) ? "it" : mtmp->female ? "she" : "he";
}

const char *
SheHeIt(mtmp)
register struct monst *mtmp;
{
	return (is_neuter(mtmp->data)) ? "It" : mtmp->female ? "She" : "He";
}

const char *
hisherits(mtmp)
register struct monst *mtmp;
{
	return (!canspotmon(mtmp) || is_neuter(mtmp->data)) ? "its" : mtmp->female ? "her" : "his";
}

const char *
himherit(mtmp)
register struct monst *mtmp;
{
	return (!canspotmon(mtmp) || is_neuter(mtmp->data)) ? "it" : mtmp->female ? "her" : "him";
}

const char *
HisHerIts(mtmp)
register struct monst *mtmp;
{
	return (!canspotmon(mtmp) || is_neuter(mtmp->data)) ? "Its" : mtmp->female ? "Her" : "His";
}

char *
mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE));
}

/* print the name as if mon_nam() was called, but assume that the player
 * can always see the monster--used for probing and for monsters aggravating
 * the player with a cursed potion of invisibility
 */
char *
noit_mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? (SUPPRESS_SADDLE|SUPPRESS_IT) :
		    SUPPRESS_IT, FALSE));
}

/* print the name as if mon_nam() was called, but assume that the player
 * can always see the monster and is not hallucinating--used for 
 * printing monster name to livelog in dNAO
 */
char *
noit_nohalu_mon_nam(mtmp)
register struct monst *mtmp;
{
	return(x_monnam(mtmp, ARTICLE_THE, (char *)0,
		mtmp->mnamelth ? (SUPPRESS_SADDLE|SUPPRESS_IT|SUPPRESS_HALLUCINATION) :
		    SUPPRESS_IT|SUPPRESS_HALLUCINATION, FALSE));
}

char *
Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

char *
noit_Monnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = noit_mon_nam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* monster's own name */
char *
m_monnam(mtmp)
struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_NONE, (char *)0, EXACT_NAME, FALSE);
}

/* pet name: "your little dog" */
char *
y_monnam(mtmp)
struct monst *mtmp;
{
	int prefix, suppression_flag;

	prefix = mtmp->mtame ? ARTICLE_YOUR : ARTICLE_THE;
	suppression_flag = (mtmp->mnamelth
#ifdef STEED
			    /* "saddled" is redundant when mounted */
			    || mtmp == u.usteed
#endif
			    ) ? SUPPRESS_SADDLE : 0;

	return x_monnam(mtmp, prefix, (char *)0, suppression_flag, FALSE);
}

#endif /* OVL0 */
#ifdef OVLB

char *
Adjmonnam(mtmp, adj)
register struct monst *mtmp;
register const char *adj;
{
	register char *bp = x_monnam(mtmp, ARTICLE_THE, adj,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);

	*bp = highc(*bp);
	return(bp);
}

char *
a_monnam(mtmp)
register struct monst *mtmp;
{
	return x_monnam(mtmp, ARTICLE_A, (char *)0,
		mtmp->mnamelth ? SUPPRESS_SADDLE : 0, FALSE);
}

char *
Amonnam(mtmp)
register struct monst *mtmp;
{
	register char *bp = a_monnam(mtmp);

	*bp = highc(*bp);
	return(bp);
}

/* used for monster ID by the '/', ';', and 'C' commands to block remote
   identification of the endgame altars via their attending priests */
char *
distant_monnam(mon, article, outbuf)
struct monst *mon;
int article;	/* only ARTICLE_NONE and ARTICLE_THE are handled here */
char *outbuf;
{
    /* high priest(ess)'s identity is concealed on the Astral Plane,
       unless you're adjacent (overridden for hallucination which does
       its own obfuscation) */
    if ( (mon->data == &mons[PM_HIGH_PRIEST] || mon->data == &mons[PM_ELDER_PRIEST]) && !Hallucination &&
	    Is_astralevel(&u.uz) && distu(mon->mx, mon->my) > 2) {
	Strcpy(outbuf, article == ARTICLE_THE ? "the " : "");
	Strcat(outbuf, mon->female ? "high priestess" : "high priest");
    } else {
	Strcpy(outbuf, x_monnam(mon, article, (char *)0, 0, TRUE));
    }
    return outbuf;
}

char *
getDrowHouse(hnum)
long hnum;
{
	switch(hnum){
		case BAENRE:
			return "House Baenre";
		case BARRISON_DEL_ARMGO:
			return "House Barrison Del'Armgo";
		case XORLARRIN:
			return "House Xorlarrin";
		case FAEN_TLABBAR:
			return "House Faen Tlabbar";
		case MIZZRYM:
			return "House Mizzrym";
		case FEY_BRANCHE:
			return "House Fey-Branche";
		case MELARN:
			return "House Melarn";
		case DUSKRYN:
			return "House Duskryn";
		/*Disbanded drow houses*/
		case BRONT_TEJ:
			return "House Bront'tej";
		case CELOFRAIE:
			return "House Celofraie";
		case DEVIR:
			return "House DeVir";
		case DO_URDEN:
			return "House Do'Urden";
		case ELEC_THIL:
			return "House Elec'thil";
		case H_KAR:
			return "House H'Kar";
		case HUN_ETT:
			return "House Hun'ett";
		case MASQ_IL_YR:
			return "House Masq'il'yr";
		case MLIN_THOBBYN:
			return "House Mlin'thobbyn";
		case OBLODRA:
			return "House Oblodra";
		case S_SRIL:
			return "House S'sril";
		case SYR_THAERL:
			return "House Syr'thaerl";
		case TEKEN_DUIS:
			return "House Teken'duis";
		case THAEYALLA:
			return "House Thaeyalla";
		case X_LARRAZ_ET_SOJ:
			return "House X'larraz'et'soj";
		/*towers*/
		case SORCERE:
			return "Tower Sorcere";
		case MAGTHERE:
			return "Tower Magthere";
		case XAXOX:
			return "Tower Xaxox";
		case LAST_BASTION_SYMBOL:
			return "Last Bastion";
		/*holy symbols*/
		case LOLTH_SYMBOL:
			return "silver star bearing";
		case KIARANSALEE_SYMBOL:
			return "silver hand bearing";
		case PEN_A_SYMBOL:
			return "silver matron bearing";
		case VER_TAS_SYMBOL:
			return "silver feather bearing";
		case EILISTRAEE_SYMBOL:
			return "silver dancer bearing";
		case GHAUNADAUR_SYMBOL:
			return "purple eye bearing";
		case LOST_HOUSE:
			return "silver-sign bearing";
		case EDDER_SYMBOL:
			return u.uevent.knoweddergud ? "Edderkirke" : "black-webbed";
	}
	return "";
}

static const char * const bogusmons[] = {
	"jumbo shrimp", "giant pigmy", /* oxymorons */
	"gnu", "killer penguin", "Hexley", /* op-syses */
	"lemming with a suicide vest", /* They have to be more than just a lemming. Because jumping to your death is not enough.. */
	"giant cockroach", "giant slug", "maggot", 
	"pterodactyl", "tyrannosaurus rex", "basilisk",
	"efreeti", "dao",/* dnd genies */
	"rot grub", "bookworm", 
	"Euryale", "Stheno", /* Medusa's elder sisters. Oddly enough "The X" works for them,
							as their names can be interpreted as titles. */
	"demi lichen", "master lichen", "arch-lichen", /* lich lichens */
	"priest of a well-known god", /* Not very mysterious */
	"shallow one", /* Not very deep */
	"fungi from your-sock", /* Not from very far away */
	"mochtroid", "youtroid", /* fake metroids */
	"serge-ant", "lieuten-ant", "captant", /* soldier ant officers */
	"shadow", "hologram", "jester", "attorney", "sleazoid",
	"killer tomato", "shrinking violet",
	"amazon", "mermaid", "gynoid", "magical girl",
	"wild man", "merman", "android", "shonen hero", "delinquent",
	"robot", "battlemech",
	"rhinovirus",
	"harpy",
	"lion-dog", "rat-ant", 
	"bot for announcing commits", "bot for evaluating code", "message-sending bot", "bot for hugging people",
		"channel-occupying bot","verbally abusive bot (with great taste in endgame-content)", "discordian bot",
		"smartbot overlord",
	"Y2K bug", "foo",
						/* misc. */
	"neutral twin", "vampiric half-dragon half-troll lycanthropic fiendish snail", 
		"squid thingy", "allegedly-matriarchal drow", "ranch dressing semielemental", /* Order of the Stick */
	
	"grue", "Christmas-tree monster", "luck sucker", "paskald",
	"brogmoid", "dornbeast",		/* Quendor (Zork, &c.) */
	"Ancient Multi-Hued Dragon", "Evil Iggy",
						/* Moria */
	"rattlesnake", "ice monster", "phantom",
	"quagga", "aquator", "griffin",
	"emu", "kestrel", "xeroc", "venus flytrap",
						/* Rogue V5 http://rogue.rogueforge.net/vade-mecum/ */
	"creeping coins",			/* Wizardry */
	"hydra", "siren",			/* Greek legend */
	"jotun",					/* Norse legend */
	"killer bunny",				/* Monty Python */
	"P. o. t. U. S.",		/* President of the United States */
	"R. o. U. S.",		/* The Princess Bride */
	"bear named Smokey",	/* "Only you can prevent forest fires!" */
	"5.9176 pentranomeker worm", /* The Mimir, contributed by Joshua Jarvis */
	"andalite",					/* Animorphs */
	"yithian", "elder thing", "old one", "great old one", "outer god", "other god", "elder god", /* Lovecraft */
	"eyeless albino penguin", /* More Lovecraft */
	"shadowgirl",				/* Shadowgirls */
	"Luggage", "God of Stuff", "vampiric watermelon",	/* Discworld */
		"fearsome madeofplasticoid", /* The Unadulterated Cat */
	"Ent",					/* Lord of the Rings */
	"tangle tree", "nickelpede", "wiggle",	/* Xanth */
	"white rabbit", "snark",		/* Lewis Carroll */
	"pushmi-pullyu",			/* Dr. Doolittle */
	"smurf",				/* The Smurfs */
	"tribble", "Klingon", "Borg",		/* Star Trek */
	"Ewok",	"Wookiee", "Jedi", "Sith",	/* Star Wars */
	"incredible hulk",		/* Comics */
	"incredible bulk",		/* 8-bit theater */
	"hegemonic brute", "sovereign slayer", "draconian dignitary", "courtyard droll", /* Homestuck */
	"sprit named Totoro",	/* Tonari no Totoro */
	"ohmu",					/* Nausicaa */
	"youma",				/* Sailor Moon */
	"Lone Power", 			/* So you want to be a Wizard? */
	"electric boid", "feral waffle-iron", "Bowman's wolf", "worried machine", "omniquantalist", 
		"squid in a space-suit", /* Freefall */
	"carbosilicate amorph", "debatably-sapient kreely", "viral boy-band", 
		"self-replicating, self-aware war machine", "maximally effective mercenary", /* Schlock Mercenary */
	"friend demon", /* Drowtales */
	"unicorn jelly", "crystal basilisk", /*Unicorn Jelly*/
	"big dumb object", /* Scifi */
	"kaiju named Godzilla", "giant ape named King Kong",		/* monster movies */
	"earthquake beast",			/* old L of SH */
	"shinigami captain", "shinigami lieutenant", "arrancar", "hollow",	/* Bleach */
	"nyaasu", "missingno.", "pikachu",	/* Pokemon (Meowth) */
	"Invid", "Veritech",		/* Robotech */
	"berserk Eva",				/* Neon Genesis Evangelion */
	"boomer",				/* Bubblegum Crisis */
	"Dalek",				/* Dr. Who ("Exterminate!") */
	"microscopic space fleet", "Ravenous Bugblatter Beast of Traal", "dolphin",
						/* HGttG */
	"teenage mutant ninja turtle",		/* TMNT */
	"samurai rabbit",			/* Usagi Yojimbo */
	"aardvark",				/* Cerebus */
	"Audrey II",				/* Little Shop of Horrors */
	"witch doctor", "one-eyed one-horned flying purple people eater",
						/* 50's rock 'n' roll */
	"purple dinosaur",			/* Barney, saccharine kiddy TV */
	"sparkly vampire",			/* Twilight */
	"Morgoth",				/* Angband */
	"Terminator",				/* The Terminator */
	"Cylon",				/* Battlestar Galactica */
	"Vorlon",				/* Babylon 5 */
	"Mondoshawan",			/* Fifth Element */
	"Tralfamadorian",		/* Slaghterhouse 5 */
	"RIME", "Ch'Thone","palaeotech","wheeler","spindle king", "kung", "shand", /* Strata */
	"questing beast",		/* King Arthur */
	"Predator",				/* Movie */
	"mother-in-law", 		/* common pests */
	"visible stalker",		/* occupational hazard */
	"Minosaur", "vestigal virgin", "octagenarian", /*"saint called Mary of the Immaculate Contraption",*/ /*non campus mentus*/
	"spherical cow",		/* Joke about theoretical physicists */
	"arsassin",				/* Psych */
	"booksh elves",			/* Ciel, on freenode */
        "one-winged dewinged stab-bat",  /* KoL */
        "praying mantis",
        "arch-pedant",
        "beluga whale",
        "bluebird of happiness",
        "bouncing eye", "floating nose",
        "buffer overflow", "dangling pointer", "walking disk drive",
        "cacodemon", "scrag",
        "cardboard golem", "duct tape golem",
        "chess pawn",
        "chicken",
        "chocolate pudding",
        "coelacanth",
        "corpulent porpoise",
        "Crow T. Robot",
        "diagonally moving grid bug",
        "dropbear",
        "@ named Dudley",
        "El Pollo Diablo",
        "evil overlord",
        "existential angst",
        "figment of your imagination", "flash of insight",
        "flying pig",
        "gazebo",
        "gonzo journalist",
        "gray goo", "magnetic monopole",
	"ooblecks",
        "heisenbug",
        "lag monster",
        "loan shark",
        "Lord British",
        "newsgroup troll",
        "ninja pirate zombie robot",
        "octarine dragon",
        "particle man",
        "possessed waffle iron",
        "poultrygeist",
        "raging nerd",
        "roomba",
        "sea cucumber",
        "spelling bee",
        "Strong Bad",
        "stuffed raccoon puppet",
        "tapeworm",
        "liger",
        "velociraptor",
        "vermicious knid",
        "viking",
        "voluptuous ampersand",
        "wee green blobbie",
        "wereplatypus",
        "zergling",
	"hag of bolding",
	"blancmange",
	"killer beet",
	"land octopus",
	"frog prince",
	"slow loris",
	"dust speck",
	"kitten prospecting robot",
	"angry mariachi",
	"manbearpig",
	"star-nosed mole",
	"acid blog",
	"guillemet",
	"solidus",
	"obelus",
	"miniature blimp",
	"lungfish",

        "apostrophe golem", "angry flower named Bob",
        "bonsai-kitten", "Boxxy", "lonelygirl15",
        "tie-thulu", "Domo-kun",
        "looooooooooooong cat",                 /* internet memes */
        "bohrbug", "mandelbug", "schroedinbug", /* bugs */
        "clockwork bug",						/* often quite deadly */
        "rabbit named Gerbenok",                /* Monty Python killer rabbit */
        "doenertier",                           /* Erkan & Stefan */
        "Invisible Pink Unicorn",
        "Flying Spaghetti Monster",             /* deities */
        "Bluebear", "Professor Abdullah Nightingale",
        "Qwerty Uiop", "troglotroll",           /* Zamonien */
        "wolpertinger", "elwedritsche", "skvader",
        "Nessie", "tatzelwurm", "dahu",         /* european cryptids */
			"bigfoot", "tree octopus", "mothman",
        "three-headed monkey",                  /* Monkey Island */
        "little green man",                     /* modern folklore */
        "weighted Companion Cube",              /* Portal */
			"Oracle turret", "morality core",
        "/b/tard",                              /* /b/ */
        "manbearpig",                           /* South Park */
        "ceiling cat", "basement cat",
        "monorail cat",                         /* the Internet is made for cat pix */
        /*"rape golem",*/                           /* schnippi */
        "tridude",                              /* POWDER */
        "orcus cosmicus",                       /* Radomir Dopieralski */
        "yeek", "quylthulg",
        "Greater Hell Beast",                   /* Angband */
        "Vendor of Yizard",                     /* Souljazz */
        "Sigmund", "lernaean hydra", "Ijyb",
        "Gloorx Vloq", "Blork the orc",         /* Dungeon Crawl Stone Soup */
        "unicorn pegasus kitten",               /* Wil Wheaton, John Scalzi */

	"nyan cat",
	"grind bug",
	"enderman",
	"wight supremacist",
	"wizard named Magical Trevor",
	"first category perpetual motion device",

	"ghoti",
	"regex engine",
	"netsplit",
	"wiki",
	"peer",
	"pigasus",
	"Semigorgon",
	"meeple",
	"conventioneer",
	"terracotta warrior",
	"large microbat", "small megabat",
	"uberhulk",
	"hearse",
	"COBOL",
	"tofurkey",
	"hippocampus",
	"hippogriff",
	"kelpie",

	/* soundex and typos of monsters */
	"gloating eye",
	"flush golem",
	"martyr orc",
	"mortar orc",
	"acute blob",
	"aria elemental",
	"aliasing priest",
	"aligned parasite",
	"aligned parquet",
	"aligned proctor",
	"baby balky dragon",
	"baby blues dragon",
	"baby caricature",
	"baby crochet",
	"baby grainy dragon",
	"baby bong worm",
	"baby long word",
	"baby parable worm",
	"barfed devil",
	"beer wight",
	"boor wight",
	"brawny mold",
	"rave spider",
	"clue golem",
	"bust vortex",
	"errata elemental",
	"elastic eel",
	"electrocardiogram eel",
	"fir elemental",
	"tire elemental",
	"flamingo sphere",
	"fallacy golem",
	"frizzed centaur",
	"forest centerfold",
	"fierceness sphere",
	"frosted giant",
	"geriatric snake",
	"gnat ant",
	"giant bath",
	"grant beetle",
	"giant mango",
	"glossy golem",
	"gnome laureate",
	"gnome dummy",
	"gooier ooze",
	"green slide",
	"guardian nacho",
	"hell hound pun",
	"high purist",
	"hairnet devil",
	"ice trowel",
	"feather golem",
	"lounge worm",
	"mountain lymph",
	"pager golem",
	"pie fiend",
	"prophylactic worm",
	"sock mole",
	"rogue piercer",
	"seesawing sphere",
	"simile mimic",
	"moldier ant",
	"stain vortex",
	"scone giant",
	"umbrella hulk",
	"vampire mace",
	"verbal jabberwock",
	"water lemon",
	"water melon",
	"winged grizzly",
	"yellow wight", 

	/* bogus UnNetHack monsters */
	"weeping angle",
	"gelatinous sphere", "gelatinous pyramid",
	"gelatinous Klein bottle", "gelatinous Mandelbrot set",
	"array terminator"
};


/* Return a random monster name, for hallucination.
 * KNOWN BUG: May be a proper name (Godzilla, Barney), may not
 * (the Terminator, a Dalek).  There's no elegant way to deal
 * with this without radically modifying the calling functions.
 */
const char *
rndmonnam()
{
	int name;

	do {
	    name = rn1(SPECIAL_PM + SIZE(bogusmons) - LOW_PM, LOW_PM);
	} while (name < SPECIAL_PM &&
	    (type_is_pname(&mons[name]) || (mons[name].geno & G_NOGEN)));

	if (name >= SPECIAL_PM) return bogusmons[name - SPECIAL_PM];
	return mons[name].mname;
}

#ifdef REINCARNATION
const char *
roguename() /* Name of a Rogue player */
{
	char *i, *opts;

	if ((opts = nh_getenv("ROGUEOPTS")) != 0) {
		for (i = opts; *i; i++)
			if (!strncmp("name=",i,5)) {
				char *j;
				if ((j = index(i+5,',')) != 0)
					*j = (char)0;
				return i+5;
			}
	}
	return rn2(3) ? (rn2(2) ? "Michael Toy" : "Kenneth Arnold")
		: "Glenn Wichman";
}
#endif /* REINCARNATION */
#endif /* OVLB */

#ifdef OVL2

static NEARDATA const char * const hcolors[] = {
	"ultraviolet", "infrared", "bluish-orange",
	"reddish-green", "dark white", "light black", "sky blue-pink",
	"salty", "sweet", "sour", "bitter",
	"striped", "spiral", "swirly", "plaid", "checkered", "argyle",
	"paisley", "blotchy", "guernsey-spotted", "polka-dotted",
	"square", "round", "triangular",
	"cabernet", "sangria", "fuchsia", "wisteria", "mauve", "puce",
	"lemon-lime", "strawberry-banana", "peppermint",
	"romantic", "incandescent"
};

const char *
hcolor(colorpref)
const char *colorpref;
{
	return (Hallucination || !colorpref) ?
		hcolors[rn2(SIZE(hcolors))] : colorpref;
}

/* return a random real color unless hallucinating */
const char *
rndcolor()
{
	int k = rn2(CLR_MAX);
	return Hallucination ? hcolor((char *)0) : (k == NO_COLOR) ?
		"colorless" : c_obj_colors[k];
}

/* Aliases for road-runner nemesis
 */
static const char * const coynames[] = {
	"Carnivorous Vulgaris","Road-Runnerus Digestus",
	"Eatibus Anythingus"  ,"Famishus-Famishus",
	"Eatibus Almost Anythingus","Eatius Birdius",
	"Famishius Fantasticus","Eternalii Famishiis",
	"Famishus Vulgarus","Famishius Vulgaris Ingeniusi",
	"Eatius-Slobbius","Hardheadipus Oedipus",
	"Carnivorous Slobbius","Hard-Headipus Ravenus",
	"Evereadii Eatibus","Apetitius Giganticus",
	"Hungrii Flea-Bagius","Overconfidentii Vulgaris",
	"Caninus Nervous Rex","Grotesques Appetitus",
	"Nemesis Riduclii","Canis latrans"
};
	
char *
coyotename(mtmp, buf)
struct monst *mtmp;
char *buf;
{
    if (mtmp && buf) {
	Sprintf(buf, "%s - %s",
	    x_monnam(mtmp, ARTICLE_NONE, (char *)0, 0, TRUE),
	    mtmp->mcan ? coynames[SIZE(coynames)-1] : coynames[rn2(SIZE(coynames)-1)]);
    }
    return buf;
}
#endif /* OVL2 */

/*do_name.c*/
