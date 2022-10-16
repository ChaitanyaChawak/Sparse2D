
#include <time.h>
#include <map>

#include "Array.h"
#include "IM_IO.h"
#include "GetLongOptions.h"
#include "bfct.h"
#include "RCurvelet3D.h"
#include "BCurvelet3D.h"
#include "uowt.h"
#include "IM3D_DCT.h"
#include "MeyerWT1D.h"
#include "FCur_TFrame.h"
#include "FCur_SFrame.h"


char Name_Imag_In[256]={}; /* input file image */
char Name_Imag_Out[256]={}; /* output file name */
extern int  OptInd;
extern char *OptArg;
extern int  GetOpt(int argc, char **argv, char *opts);
extern char** short_argv;
extern int short_argc;

int Nx,Ny,Nz;

static void filtinit(int argc, char *argv[])
{
	int c;

	/* get options */
	while ((c = GetOpt(argc,argv,(char*)"x:y:z:")) != -1) 
	{
		switch (c) 
		{
			case 'x': 
				if (sscanf(OptArg,"%d",&Nx) != 1) 
				{fprintf(OUTMAN, "Error: bad size parameter: %s\n", OptArg);exit(-1); }
				break;
			case 'y': 
				if (sscanf(OptArg,"%d",&Ny) != 1) 
				{fprintf(OUTMAN, "Error: bad size parameter: %s\n", OptArg);exit(-1); }
				break;
			case 'z': 
				if (sscanf(OptArg,"%d",&Nz) != 1) 
				{fprintf(OUTMAN, "Error: bad size parameter: %s\n", OptArg);exit(-1); }
				break;
			default : 
				break;
		}
	} 

	/* get optional input file names from trailing 
	parameters and open files */
	if (OptInd < argc)
		strcpy(Name_Imag_In, argv[OptInd++]);
	else
	{
		cerr<<"plante : "<<argc<<"/"<<argc<<endl;
		exit(0);
	}

	if (OptInd < argc)
		strcpy(Name_Imag_Out, argv[OptInd++]);
	else
	{
		cerr<<"plante : "<<argc<<endl;
		exit(0);
	}

	/* make sure there are not too many parameters */
	if (OptInd < argc)
	{
		fprintf(OUTMAN, "Too many parameters: %s ...\n", argv[OptInd]);
		exit(-1);
	}
}



int main(int argc, char *argv[])
{

// Get short options
	filtinit(argc, argv);
	
	FILE * fid;
	fid=fopen(Name_Imag_In,"rb");
	long int n = Nx*Ny*Nz;
	float* toto;
	toto = (float*) malloc(n*sizeof(float));
	fread(toto,sizeof(float),n,fid);
	fltarray fofo;
	fofo.alloc(toto,Nx,Ny,Nz);
	writefltarr(Name_Imag_Out,fofo);
	//cerr<<"conversion done"<<endl;
}

