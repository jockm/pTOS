/*
 * iumem.c - internal user memory management routines
 *
 * Copyright (c) 2001 Lineo, Inc.
 *
 * Authors:
 *  xxx <xxx@xxx>
 *
 * This file is distributed under the GPL, version 2 or at your
 * option any later version.  See doc/license.txt for details.
 */



#define DBG_IUMEM  0



#include "portab.h"
#include "fs.h"
#include "bios.h"
#include "mem.h"
#include "gemerror.h"
#include "../bios/kprint.h"


/*
 *  STATIUMEM - cond comp; set to true to count calls to these routines
 */

#define STATIUMEM	FALSE


#if	STATIUMEM

long	ccffit;
long	ccfreeit;

ccffit=0;
ccfreeit=0;

#endif



/**
 *  ffit - find first fit for requested memory in ospool
 */

MD	*ffit(long amount, MPB *mp)
{
    MD *p,*q,*p1;	       /* free list is composed of MD's */
    BOOLEAN maxflg;
    long maxval;

#if	DBG_IUMEM
    kprint("BDOS: ffit - amount = ");
    kputp(amount);
    kprint("\n");
    kprint("BDOS: ffit - mp = ");
    kputp((void*)mp);
    kprint("\n");
#endif

#if	STATIUMEM
    ++ccffit ;
#endif

    if( (q = mp->mp_rover) == 0  )	/*  get rotating pointer	*/
    {
#if	DBG_IUMEM
	kprint("ffit: null rover\n") ;
#endif
	return( 0 ) ;
    }

    maxval = 0;
    maxflg = (amount == -1 ? TRUE : FALSE) ;

    p = q->m_link;			/*  start with next MD		*/

    do /* search the list for an MD with enough space */
    {

	if( p == 0 )
	{
	    /*	at end of list, wrap back to start  */
	    q = (MD *) &mp->mp_mfl ;	/*  q => mfl field  */
	    p = q->m_link ;		/*  p => 1st MD     */
	}

	if ((!maxflg) && (p->m_length >= amount))
	{
	    /*	big enough	*/
	    if (p->m_length == amount)
		/* take the whole thing */
		q->m_link = p->m_link;
	    else
	    {
		/* break it up - 1st allocate a new
		   MD to describe the remainder */

		/*********** TBD **********
		 *  Nicer Handling of This *
		 *	  Situation	  *
		 **************************/
		if( (p1=MGET(MD)) == 0 )
		{
#if	DBG_IUMEM
		    kprint("ffit: Null Mget\n") ;
#endif
		    return(0);
		}

		/*  init new MD  */

		p1->m_length = p->m_length - amount;
		p1->m_start = p->m_start + amount;
		p1->m_link = p->m_link;

		/*  adjust allocated block  */

		p->m_length = amount;
		q->m_link = p1;
	    }

	    /*	link allocate block into allocated list,
	     mark owner of block, & adjust rover  */

	    p->m_link = mp->mp_mal;
	    mp->mp_mal = p;

	    p->m_own = run;

	    mp->mp_rover =
		(q == (MD *) &mp->mp_mfl ? q->m_link : q);

	    return(p);	/* got some */
	}
	else if (p->m_length > maxval)
	    maxval = p->m_length;

	p = ( q=p )->m_link;

    } while (q != mp->mp_rover);

    /*	return either the max, or 0 (error)  */

#if	DBG_IUMEM
    if( !maxflg )
	kprint("ffit: Not Enough Contiguous Memory\n") ;
#endif
    return( maxflg ? (MD *) maxval : 0 ) ;
}


/*
 *  freeit - Free up a memory descriptor
 */

freeit(MD *m, MPB *mp)
{
    MD *p, *q;

#if	STATIUMEM
    ++ccfreeit ;
#endif

    q = 0;

    for (p = mp->mp_mfl; p ; p = (q=p) -> m_link)
	if (m->m_start <= p->m_start)
	    break;

    m->m_link = p;

    if (q)
	q->m_link = m;
    else
	mp->mp_mfl = m;

    if (!mp->mp_rover)
	mp->mp_rover = m;

    if (p)
	if (m->m_start + m->m_length == p->m_start)
	{ /* join to higher neighbor */
	    m->m_length += p->m_length;
	    m->m_link = p->m_link;

	    if (p == mp->mp_rover)
		mp->mp_rover = m;

	    xmfreblk(p);
	}

    if (q)
	if (q->m_start + q->m_length == m->m_start)
	{ /* join to lower neighbor */
	    q->m_length += m->m_length;
	    q->m_link = m->m_link;

	    if (m == mp->mp_rover)
		mp->mp_rover = q;

	    xmfreblk(m);
	}
}

