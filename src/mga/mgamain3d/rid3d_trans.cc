/******************************************************************************
**                   Copyright (C) 2000 by CEA
*******************************************************************************
**
**    UNIT
**
**    Version: 1.0
**
**    Author: Jean-Luc Starck
**
**    Date:  03/02/00
**    
**    File:  ridgelet.cc
**
*******************************************************************************
**
**    DESCRIPTION  ridgelet transform program
**    ----------- 
**                 
******************************************************************************/

#include "IM_Obj.h"
#include "IM_IO.h"
#include "Ridgelet3D.h"
 
char Name_Dat_In[256];  // input file image  
char Name_Dat_Out[256]; //  output file name  
 
extern int  OptInd;
extern char *OptArg;

extern int  GetOpt(int argc, char **argv, char *opts);

Bool Verbose=False; 
Bool Reverse=False; // Reverse transform
int Nbr_Plan = DEF_RID3D_NBR_SCALE;
int BlockSize=0;
Bool BlockOverlap = False;
type_ridgelet3d_WTtrans RidTrans = DEF_RID3D_TRANS;
Bool ExtractBand = False;
 
/***************************************/

static void usage(char *argv[])
{
    // int i;
    fprintf(OUTMAN, "Usage: %s options in_image result\n\n", argv[0]);
    fprintf(OUTMAN, "   where options =  \n");

    fprintf(OUTMAN, "         [-t type_of_ridgelet]\n");
    for (int i = 0; i < NBR_RID3D_TRANS; i++)
    fprintf(OUTMAN, "              %d: %s \n",i+1, StringRid3DTransform((type_ridgelet3d_WTtrans) (i+1)));
    fprintf(OUTMAN, "              Default is %s.\n",  StringRid3DTransform(RidTrans));
    manline();

    fprintf(OUTMAN, "         [-n number_of_scales]\n");
    fprintf(OUTMAN, "             Number of scales used in the ridgelet transform.\n");
    fprintf(OUTMAN, "             Default is automatically calculated. \n");    
    manline();

    fprintf(OUTMAN, "         [-b BlockSize]\n");
    fprintf(OUTMAN, "             Block Size. Default is image size. \n");    
    manline();

//     fprintf(OUTMAN, "         [-i]\n");
//     fprintf(OUTMAN, "             Print statistical information about.\n");
//     fprintf(OUTMAN, "             each band. Default is no. \n");    
//     manline();

    fprintf(OUTMAN, "         [-O]\n");
    fprintf(OUTMAN, "             Block overlapping. Default is no. \n");    
    manline();

    fprintf(OUTMAN, "         [-r]\n");
    fprintf(OUTMAN, "             Inverse RIDGELET transform.\n");  
    manline();  

    write_scales_x_band_usage();
    manline();

    vm_usage();
    manline();    
    verbose_usage();
    manline();         
    manline();
    exit(-1);
}
  
/*********************************************************************/

/* GET COMMAND LINE ARGUMENTS */
static void filtinit(int argc, char *argv[])
{
    int c;
#ifdef LARGE_BUFF
    int VMSSize=-1;
    Bool OptZ = False;
    char VMSName[1024] = "";
#endif     

    /* get options */
    while ((c = GetOpt(argc,argv,"Cxt:Ob:n:rvzZ")) != -1) 
    {
	switch (c) 
        {
           case 'x': ExtractBand = True; break;
           case 't': 
               if (sscanf(OptArg,"%d",&c ) != 1) 
                {
                    fprintf(OUTMAN, "Error: bad type of ridgelet transform: %s\n", OptArg);
                    exit(-1);
                }
                if ((c > 0) && (c <= NBR_RID3D_TRANS)) RidTrans = (type_ridgelet3d_WTtrans) (c);
                else  
                {
                   fprintf(OUTMAN, "Error: bad type of ridgelet transform: %s\n", OptArg);
                   exit(-1);
                }
                break;
           case 'O': BlockOverlap= (BlockOverlap==True) ? False: True;break;
           case 'r': Reverse=True;break;
           case 'v': Verbose = True;break;
	   case 'b':
                if (sscanf(OptArg,"%d",&BlockSize) != 1) 
                {
                    fprintf(OUTMAN, "bad block size: %s\n", OptArg);
                    exit(-1);
                }
                break;
	   case 'n':
                /* -n <Nbr_Plan> */
                if (sscanf(OptArg,"%d",&Nbr_Plan) != 1) 
                {
                    fprintf(OUTMAN, "bad number of scales: %s\n", OptArg);
                    exit(-1);
                }
                break;	   
#ifdef LARGE_BUFF
	    case 'z':
	        if (OptZ == True)
		{
                   fprintf(OUTMAN, "Error: Z option already set...\n");
                   exit(-1);
                }
	        OptZ = True;
	        break;
            case 'Z':
	        if (sscanf(OptArg,"%d:%s",&VMSSize, VMSName) < 1)
		{
		   fprintf(OUTMAN, "Error: syntaxe is Size:Directory ... \n");
		   exit(-1);
		}
	        if (OptZ == True)
		{
                   fprintf(OUTMAN, "Error: z option already set...\n");
                   exit(-1);
                }
		OptZ = True;
                break;
#endif
            case '?': usage(argv); break;
	    default: usage(argv); break;
 		}
	} 
      
       if (Reverse == True)
       {
           
           if (ExtractBand == True) 
           {
              fprintf(OUTMAN, "Error: x and r option are not compatible...\n");
              exit(-1);
           }
       }

       /* get optional input file names from trailing 
          parameters and open files */
       if (OptInd < argc) strcpy(Name_Dat_In, argv[OptInd++]);
         else usage(argv);

	if (OptInd < argc) strcpy(Name_Dat_Out, argv[OptInd++]);
         else usage(argv);

	/* make sure there are not too many parameters */
	if (OptInd < argc)
        {
		fprintf(OUTMAN, "Too many parameters: %s ...\n", argv[OptInd]);
		exit(-1);
	}

       	   
#ifdef LARGE_BUFF
    if (OptZ == True) vms_init(VMSSize, VMSName, Verbose);
#endif  
}

/***************************************************************************/
 
int main(int argc, char *argv[])
{
    int s,k;
    fitsstruct Header;
    char Cmd[512];
    Cmd[0] = '\0';
    for (k =0; k < argc; k++) sprintf(Cmd, "%s %s", Cmd, argv[k]);
     
    // Get command line arguments, open input file(s) if necessary
    lm_check(LIC_MR4);
    filtinit(argc, argv);
    
    //ExtractBand = True; 

    if (Verbose == True)
    { 
        cout << endl << endl << "PARAMETERS: " << endl << endl;
        cout << "File Name in = " << Name_Dat_In << endl;
        cout << "File Name Out = " << Name_Dat_Out << endl;  
        cout << "Ridgelet transform = " <<  StringRid3DTransform(RidTrans) << endl;  
        if (BlockOverlap == False) cout << "No overlapping " << endl;
        if (Reverse == True) cout << "Inverse Rigelet transform  " <<  endl;  
        if (BlockSize > 0)  cout << "BlockSize = " << BlockSize <<endl;
    }

//    if ((BlockSize > 0) && (is_power_of_2(BlockSize) == False))
//    {
//       cout << "Error: Block size must be a power of two ... " << endl;
//       exit(-1);
//    }
       
    FilterAnaSynt SelectFilter(F_MALLAT_7_9);
    SubBandFilter SB1D(SelectFilter, NORM_L2);
    Ridgelet3D RG(SB1D);
    RG.RidTrans = RidTrans;
    if (Nbr_Plan != DEF_RID3D_NBR_SCALE) 
    {
       RG.NbrScale = Nbr_Plan;
       RG.GetAutoNbScale = False;
       if (Nbr_Plan < 2)
       {
          RG.BlockOverlap = False;
       }
       else RG.BlockOverlap = BlockOverlap;
    }         
    else RG.BlockOverlap = BlockOverlap;
    RG.NbrScale = Nbr_Plan;
    RG.Verbose = Verbose;
    RG.StatInfo = False;    
    
    if (Reverse == False)
    {
       fltarray Data;
       Ifloat Result;
       io_3d_read_data(Name_Dat_In, Data);
       //int Nx = Data.nx();
       //int Ny = Data.ny(); 
       //int Nz = Data.nz();
       
       if (BlockSize > Data.nx())
       {
           cout << "Warning: Block size must lower than the cube size ... " << endl;
           cout << "         Block size is set to cube size " << endl;
           BlockSize = 0;
       }

        RG.transform(Data,Result,BlockSize);
        if (ExtractBand == True)
        {
           char Prefix[256];
           char Name_Imag[256];
           io_strcpy_prefix(Prefix,  Name_Dat_Out);
           Ifloat ImaScale(Result.nl(), Result.nc(), "scale");
           for (s = 0; s < RG.NbrScale; s++)
           {
              sprintf (Name_Imag, "%s_band_%d", Prefix, s+1);
              RG.get_scale(Result,ImaScale,s);
              io_write_ima_float(Name_Imag,ImaScale);
           }
        }
	RG.write(Name_Dat_Out, Result);
	// io_write_ima_float(Name_Dat_Out, Result);

    }
    else  
    {
        Ifloat Data;
        fltarray  Result; 
        // io_read_ima_float(Name_Dat_In, Data);
        RG.read(Name_Dat_In,Data);
        RG.recons(Data,Result,BlockSize);
	io_3d_write_data(Name_Dat_Out, Result);
    }
    exit(0);
}
