/*
 * Copyright (c) 1986, 1990 by The Trustees of Columbia University in
 * the City of New York.  Permission is granted to any individual or
 * institution to use, copy, or redistribute this software so long as it
 * is not sold for profit, provided this copyright notice is retained.
 */

#include "mm.h"
#include "rd.h"

main()
{
    int i;
    extern folder_switch msg_ops[];
    extern int num_msg_ops;

    for(i = 0; i < num_msg_ops; i++)
	printf("int type_%s = %d;\n", msg_ops[i].typename, i);
}
