/******************************************************************************
 * @author Adrian Pop [adrpo@ida.liu.se, http://www.ida.liu.se/~adrpo]
 * Copyright (c) 2002-2006, Adrian Pop [adrpo@ida.liu.se],
 * Programming Environments Laboratory (PELAB),
 * Department of Computer and Information Science (IDA), 
 * Link pings University (LiU). 
 * All rights reserved.
 *
 * http://www.ida.liu.se/~adrpo/license/
 *
 * NON-COMMERCIAL terms and conditions [NON-COMMERCIAL setting]:
 * Permission to use, copy, modify, and distribute this software and
 * its documentation in source or binary form (including products 
 * developed or generated using this software) for NON-COMMERCIAL 
 * purposes and without fee is hereby granted, provided that this 
 * copyright notice appear in all copies and that both the copyright 
 * notice and this permission notice and warranty disclaimer appear 
 * in supporting documentation, and that the name of The Author is not 
 * to be used in advertising or publicity pertaining to distribution 
 * of the software without specific, prior written permission.
 * 
 * COMMERCIAL terms and conditions [COMMERCIAL setting]:
 * COMMERCIAL use, copy, modification and distribution in source 
 * or binary form (including products developed or generated using
 * this software) is NOT permitted without prior written agreement 
 * from Adrian Pop [adrpo@ida.liu.se].
 * 
 * THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *****************************************************************************/
/* misc_print.c */
#include <stdio.h>
#include "rml.h"

RML_BEGIN_LABEL(RML__print)
{
    void *str = rmlA0;
    fwrite(RML_STRINGDATA(str), RML_HDRSTRLEN(RML_GETHDR(str)), 1, stdout);
	fflush(stdout);
    RML_TAILCALLK(rmlSC);
}
RML_END_LABEL



void rmldb_var_print(void *p)
{
	/* rmldb_sprintf("[%p]", p); */
	if (!p) { rmldb_sprintf ("NIL"); fflush(stdout); return; }
	if( RML_ISIMM(p) ) 
	{
		rmldb_sprintf ("%d", RML_UNTAGFIXNUM(p));    
	} 
	else 
	{
		rml_uint_t phdr = RML_GETHDR(p);            
		if( phdr == RML_REALHDR ) 
		{
			rmldb_sprintf ("%f", rml_prim_get_real(p));
			fflush(stdout);
		} else 
			if( RML_HDRISSTRING(phdr) ) 
			{
				rmldb_sprintf ("\"%s\"", RML_STRINGDATA(p));
				fflush(stdout);
				/* use if neccesarry RML_HDRSTRLEN(phdr) */
			} else 
				if( RML_HDRISSTRUCT(phdr) ) 
				{
					rml_uint_t slots = RML_HDRSLOTS(phdr);
					rml_uint_t constr = RML_HDRCTOR(phdr);
					void **pp = NULL;
					if (slots == 0)
					{
						rmldb_sprintf ("{S(%d)[%d]=NIL}", constr, slots);
						fflush(stdout);
						return;
					}
					
					rmldb_sprintf ("S(%d)[%d](", constr, slots);

					pp = RML_STRUCTDATA(p);
					fflush(stdout);
					// function definition
					if ((constr == 64 || constr==13) &&
						slots > 1000000) return;
					if( slots != 0 )
					{
						// rmldb_sprintf ("\n\t"); 
						while( --slots > 0 )
						{
							rmldb_var_print(*pp++);
							rmldb_sprintf (",");
							fflush(stdout);
						}
						p = *pp; 
						rmldb_var_print(*pp); rmldb_sprintf (")"); fflush(stdout);
						// goto tail_recur_debug;  
					}					    
				} 
				else 
				{
					rmldb_sprintf ("UNKNOWN"); fflush(stdout);
				}
	}
}


RML_BEGIN_LABEL(RML__debug_5fprint)
{
	void *str = rmlA0;
	/*if (execution_type == RMLDB_STEP || rmldb_show == RMLDB_SHOW)*/
	{
		rmldb_sprintf ("%s=[", RML_STRINGDATA(str));
		rmldb_var_print(rmlA1);
		rmldb_sprintf ("]\n");
		fflush(stdout);
	}
	RML_TAILCALLK(rmlSC);
}
RML_END_LABEL
