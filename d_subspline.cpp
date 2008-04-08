/*
 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1992 by James Tough
 * Parts Copyright (c) 1989-2002 by Brian V. Smith
 * Parts Copyright (c) 1995 by C. Blanc and C. Schlick
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

extern "C"{
#include "fig.h"
#include "resources.h"
#include "object.h"
#include "d_subspline.h"
#include "u_create.h"
#include "u_draw.h"
#include "u_list.h"

#include "u_free.h"
}

/*************************** local declarations *********************/

#define MIN_NUMPOINTS_FOR_QUICK_REDRAW     2

static INLINE Boolean  add_subspline_point(F_spline *spline, F_point **last_point, F_sfactor **last_sfactor, F_point *point);
static F_spline       *extract_subspline(F_spline *spline, F_point *point);



F_spline     *
create_subspline(int *num_pts, F_spline *spline, F_point *point, F_sfactor **sfactor, F_sfactor **sub_sfactor)
{
  F_spline *sub_spline;
  *sub_sfactor = NULL;

  *num_pts = num_points(spline->points);
  *sfactor = search_sfactor(spline, point);
  if (*num_pts > MIN_NUMPOINTS_FOR_QUICK_REDRAW)
    {
      sub_spline = extract_subspline(spline, point);
      if (sub_spline != NULL) 
	*sub_sfactor = search_sfactor(sub_spline, 
			  search_spline_point(sub_spline, point->x, point->y));
    }
    else
      {
	sub_spline = spline;
	*sub_sfactor = *sfactor;
      }
  return sub_spline;
}


void free_subspline(int num_pts, F_spline **spline)
{
  if (num_pts > MIN_NUMPOINTS_FOR_QUICK_REDRAW)
      free_spline(spline);
}

void
draw_subspline(int num_pts, F_spline *spline, int op)
{
  if (num_pts > MIN_NUMPOINTS_FOR_QUICK_REDRAW)
    quick_draw_spline(spline, op);
  else
    draw_spline(spline, op);
}


static INLINE Boolean
add_subspline_point(F_spline *spline, F_point **last_point, F_sfactor **last_sfactor, F_point *point)
{
  Boolean point_ok, sfactor_ok;

  point_ok = insert_point(point->x, point->y, *last_point);
  sfactor_ok = append_sfactor(search_sfactor(spline, point)->s,
				    *last_sfactor);
  if (!(point_ok && sfactor_ok))
    return False;

  *last_point = (*last_point)->next;
  *last_sfactor = (*last_sfactor)->next;
  return True;
}


static F_spline     *
extract_subspline(F_spline *spline, F_point *point)
{
  F_spline  *subspline;
  F_point   *cursor, *last_point;
  F_point   *prev1, *prev2;
  F_sfactor *sfactor_cursor, *last_sfactor;
  int       i = 0;


  if ((subspline = create_spline()) == NULL)
    return NULL;
  
  *subspline            = *spline;
  subspline->next       = NULL;
  subspline->points     = NULL;
  subspline->sfactors   = NULL;
  subspline->for_arrow  = NULL;
  subspline->back_arrow = NULL;
  subspline->comments   = NULL;

  prev1 = prev2 = spline->points;
  for (cursor=spline->points ; cursor!=point ; cursor = cursor->next)
    {
      prev2 = prev1;
      prev1 = cursor;
    }

  if ((closed_spline(spline)) && (prev2==prev1))
                                           /* use points list as circular */
    {
      for (cursor = point ; cursor->next->next != NULL ; cursor=cursor->next)
	;
      if (prev1 == point)       /* edition of the first point of the spline */
	{
	  prev2 = cursor;
	  prev1 = cursor->next;
	}
      else if (prev2 == prev1)                              /* second point */
	{
	  prev2 = cursor->next;
	}
      cursor = point;                      /* reset cursor as we found it */
    }

  sfactor_cursor = search_sfactor(spline, prev2);

  if (!first_spline_point(prev2->x, prev2->y, sfactor_cursor->s, subspline))
    {
      free_spline(&subspline);
      return NULL;
    }
  
  last_point = subspline->points;
  last_sfactor = subspline->sfactors;

  if (!(add_subspline_point(spline, &last_point, &last_sfactor, prev1)
	&&  add_subspline_point(spline, &last_point, &last_sfactor, cursor)))
    {
      free_spline(&subspline);
      return NULL;
    }

  for (i=1; i < 3; i++)
    {
      if (cursor->next != NULL)
	cursor = cursor->next;
      else if (closed_spline(spline))
	cursor = spline->points;
      
      if (!add_subspline_point(spline, &last_point, &last_sfactor, cursor))
	{
	  free_spline(&subspline);
	  subspline = NULL;
	  break;
	}
    }

  return (subspline);
}
