/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *
 *      Description:
 *              SImulation MAnager implementation (ARTÌS)
 *
 *      Authors:
 *              Michele Bracuto <bracuto@cs.unibo.it>
 *              Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <LPInfo.h>
#include <RTIComm.h>


int main(int argc, char *argv[])
{
    SIMA_Initialize(5000, atoi(argv[1]), "channels.txt");

    /* ... */

    SIMA_Finalize( );


    return(0);
}

/*---------------------------------------------------------------------------*/
