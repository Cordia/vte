/*
 * Copyright (C) 2003 Red Hat, Inc.
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef vte_vteregex_h_included
#define vte_vteregex_h_included

#ident "$Id: vteregex.h 1133 2005-02-15 10:56:16Z kmaraas $"

#include <glib.h>

G_BEGIN_DECLS

struct _vte_regex_match {
	int rm_so, rm_eo;
};
struct _vte_regex;

struct _vte_regex * _vte_regex_compile(const char *pattern);
void _vte_regex_free(struct _vte_regex *regex);
int _vte_regex_exec(struct _vte_regex *regex, const char *string,
		    gsize nmatch, struct _vte_regex_match *matches);
G_END_DECLS

#endif