/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985-1988 by Supoj Sutanthavibul
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1991 by Paul King
 *
 * Any party obtaining a copy of these files is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and documentation
 * files (the "Software"), including without limitation the rights to use,
 * copy, modify, merge, publish distribute, sublicense and/or sell copies of
 * the Software, and to permit persons who receive copies from any such
 * party to do so, with the only requirement being that the above copyright
 * and this permission notice remain intact.
 *
 */

#include "fig.h"
#include "resources.h"
#include "object.h"


void translate_lines (F_line *lines, int dx, int dy);
void translate_splines (F_spline *splines, int dx, int dy);
void translate_ellipses (F_ellipse *ellipses, int dx, int dy);
void translate_arcs (F_arc *arcs, int dx, int dy);
void translate_texts (F_text *texts, int dx, int dy);
void translate_compounds (F_compound *compounds, int dx, int dy);

void translate_ellipse(F_ellipse *ellipse, int dx, int dy)
{
    ellipse->center.x += dx;
    ellipse->center.y += dy;
    ellipse->start.x += dx;
    ellipse->start.y += dy;
    ellipse->end.x += dx;
    ellipse->end.y += dy;
}

void translate_arc(F_arc *arc, int dx, int dy)
{
    arc->center.x += (float) dx;
    arc->center.y += (float) dy;
    arc->point[0].x += dx;
    arc->point[0].y += dy;
    arc->point[1].x += dx;
    arc->point[1].y += dy;
    arc->point[2].x += dx;
    arc->point[2].y += dy;
}

void translate_line(F_line *line, int dx, int dy)
{
    F_point	   *point;

    for (point = line->points; point != NULL; point = point->next) {
	point->x += dx;
	point->y += dy;
    }
}

void translate_text(F_text *text, int dx, int dy)
{
    text->base_x += dx;
    text->base_y += dy;
}

void translate_spline(F_spline *spline, int dx, int dy)
{
    F_point	   *point;

    for (point = spline->points; point != NULL; point = point->next) {
	point->x += dx;
	point->y += dy;
    }

}

void translate_compound(F_compound *compound, int dx, int dy)
{
    compound->nwcorner.x += dx;
    compound->nwcorner.y += dy;
    compound->secorner.x += dx;
    compound->secorner.y += dy;

    translate_lines(compound->lines, dx, dy);
    translate_splines(compound->splines, dx, dy);
    translate_ellipses(compound->ellipses, dx, dy);
    translate_arcs(compound->arcs, dx, dy);
    translate_texts(compound->texts, dx, dy);
    translate_compounds(compound->compounds, dx, dy);
}

void translate_arcs(F_arc *arcs, int dx, int dy)
{
    F_arc	   *a;

    for (a = arcs; a != NULL; a = a->next)
	translate_arc(a, dx, dy);
}

void translate_compounds(F_compound *compounds, int dx, int dy)
{
    F_compound	   *c;

    for (c = compounds; c != NULL; c = c->next)
	translate_compound(c, dx, dy);
}

void translate_ellipses(F_ellipse *ellipses, int dx, int dy)
{
    F_ellipse	   *e;

    for (e = ellipses; e != NULL; e = e->next)
	translate_ellipse(e, dx, dy);
}

void translate_lines(F_line *lines, int dx, int dy)
{
    F_line	   *l;

    for (l = lines; l != NULL; l = l->next)
	translate_line(l, dx, dy);
}

void translate_splines(F_spline *splines, int dx, int dy)
{
    F_spline	   *s;

    for (s = splines; s != NULL; s = s->next)
	translate_spline(s, dx, dy);
}

void translate_texts(F_text *texts, int dx, int dy)
{
    F_text	   *t;

    for (t = texts; t != NULL; t = t->next)
	translate_text(t, dx, dy);
}
