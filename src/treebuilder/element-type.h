/*
 * This file is part of Hubbub.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2008 John-Mark Bell <jmb@netsurf-browser.org>
 */

#ifndef hubbub_treebuilder_element_type_h_
#define hubbub_treebuilder_element_type_h_

#include "treebuilder/treebuilder.h"

typedef enum
{
/* Special */
	ADDRESS, AREA, ARTICLE, ASIDE, BASE, BASEFONT, BGSOUND, BLOCKQUOTE,
	BODY, BR, CENTER, COL, COLGROUP, COMMAND, DATAGRID, DD, DETAILS,
	DIALOG, DIR, DIV, DL, DT, EMBED, FIELDSET, FIGCAPTION, FIGURE, FOOTER,
	FORM, FRAME, FRAMESET, H1, H2, H3, H4, H5, H6, HEAD, HEADER, HR, IFRAME,
	IMAGE, IMG, INPUT, ISINDEX, LI, LINK, LISTING, MAIN, MENU, META, NAV,
	NOEMBED, NOFRAMES, NOSCRIPT, OL, OPTGROUP, OPTION, P, PARAM, PLAINTEXT,
	PRE, SCRIPT, SECTION, SELECT, SPACER, STYLE, SUMMARY, TBODY, TEXTAREA,
	TFOOT, THEAD, TITLE, TR, UL, WBR,
/* Scoping */
	APPLET, BUTTON, CAPTION, HTML, MARQUEE, OBJECT, TABLE, TD, TH,
/* Formatting */
	A, B, BIG, CODE, EM, FONT, I, NOBR, S, SMALL, STRIKE, STRONG, TT, U,
/* Phrasing */
	/**< \todo Enumerate phrasing elements */
	LABEL, OUTPUT, RP, RT, RUBY, SPAN, SUB, SUP, VAR, XMP,
/* MathML */
	MATH, MGLYPH, MALIGNMARK, MI, MO, MN, MS, MTEXT, ANNOTATION_XML,
/* SVG */
	SVG, FOREIGNOBJECT, /* foreignobject is scoping, but only in SVG ns */
	DESC,
	UNKNOWN
} element_type;

struct element_type_map {
	const char *name;
	element_type type;
};

const struct element_type_map *hubbub_element_type_lookup(
		register const char *str,
		register size_t len);

#endif

