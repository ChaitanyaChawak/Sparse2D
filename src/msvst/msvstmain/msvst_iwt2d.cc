/**************************************************
 * PROJECT : PoisMSVST - version Ib (Separated detection and estiamtion)
 * CEA
 * Filename : main1.cc
 * This is the main file of PoisMSVST
 **************************************************/
#include <string>
#include <iostream>
#include <iomanip>
#include <time.h>
#include "cdflib.h"
#include "GlobalInc.h"
#include "IM_IO.h"
#include "Array.h"
#include "PoisMSVSTException.h"
#include "Atrous.h"
#include "B3VSTAtrous.h"
#include "Wavelet.h"
#include "ImLib.h"
#include "Border.h"

using namespace std;

extern int  OptInd;
extern char *OptArg;
extern int  GetOpt(int argc, char **argv, char *opts);

char   Name_Imag_In[256];   // input image file
char   Name_Imag_Out[256];  // output image file
const int METHOD_LEN = 2;   // total number of methods available
enum   MODE1 { PV = 0, FDR = 1 };  // denoising modes
enum   MODE2 { DIRECT = 0, L1REG = 1 }; // iteration modes
MODE1  PROGMODE = PV;     // denoising mode
MODE2  ITERMODE = L1REG;    // iteration mode
int    NITER = 10;          // max. number of iterations
double PROBA[METHOD_LEN] = {0.000465, 0.1}; // default cut p-value at 3.5*sigma and FDR = 0.1
int    NSCALE = 5;          // max. number of scales
bool   VERBOSE = false;     // verbose mode
bool   DEC[3] = {false, false, false}; // undecimated
bool   FDRINDEP = false;    // dependence parameter in FDR
int    FSCALE = 1;          // First detection scale
bool   KILLLAST = false;    // ignore the last approximation scale
bool   DETPOS   = false;    // detect only positive coefficients
double GLEVEL = 3.5;        // Gauss-detection level (k-sigma)
bool   TRANSF2 = false;     // ver2 in the transform
type_border TBORDER = I_MIRROR;   // border type

// NOTICE : FDR are all band-by-band tested

//***************************************
static void usage(char *argv[])
{
  cout << "MS-VST : Separated detection and estiamtion" << endl;
  cout << "Usage: " << argv[0] << " OPTIONS input_image_name output_image_name" << endl;
  cout << "OPTIONS are" << endl;  
  cout << "    [-T] use g=Id-h*h as iteration band-pass filter" << endl;
  cout << "    [-M value]" << endl;
  cout << "         M = 0 : p-value threshold (default)" << endl;
  cout << "         M = 1 : FDR threshold" << endl;
  cout << "    [-E value] two sided Pr or FDR (default: Pr = 3.5*sigma or FDR = 0.1)" << endl;
  cout << "    [-s value] Equivalent Gauss-detection level (default = 3.5*sigma)" << endl;
  cout << "    [-c value] manual FDR setting, 0/1 = independence/dependence of coef. (default = 1)" << endl;
  cout << "    [-n value] number of scales (default = 5)" << endl;
  cout << "    [-F value] first detection scale (default = 1)" << endl;
  cout << "    [-K] ignore the last approximation band (used with iteration)" << endl;
  cout << "    [-p] detect only the positive coefficients (used with iteration)" << endl;
  cout << "    [-I value] iteration modes for M = 0,1" << endl;
  cout << "                      I = 0 : Direct iterative mode" << endl;
  cout << "                      I = 1 : L1-regularized iterative mode (default)" << endl;
  cout << "    [-i value] number of iteration (default = 10)" << endl; 
  cout << "    [-B value] border modes" << endl;
  cout << "                      B = 0 : cont" << endl;
  cout << "                      B = 1 : mirror (default)" << endl;
  cout << "                      B = 2 : period" << endl;
  cout << "                      B = 3 : zero" << endl;
  cout << "    [-v] verbose mode" << endl;
  cout << endl;
}
 
//*********************************************************************

// GET COMMAND LINE ARGUMENTS
static void filtinit(int argc, char *argv[])
{
    int c;  

    // get options 
    while ((c = GetOpt(argc,argv,"M:E:s:c:n:F:I:i:B:TKpv")) != -1) 
    {
       switch (c) 
	   {	
    case 'M':
	  int pmd;
	  if (sscanf(OptArg, "%d", &pmd) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  if (pmd == 0)
	    PROGMODE = PV;
	  else 
        PROGMODE = FDR;
	  break;
	
	case 'E':
	  double proba;
	  if (sscanf(OptArg, "%lf", &proba) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  for (int i=0; i<METHOD_LEN; i++) PROBA[i] = proba;
	  break;
	  
	case 's':
	  if (sscanf(OptArg, "%lf", &GLEVEL) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
      for (int i=0; i<METHOD_LEN; i++) PROBA[i] = 2. * (1 - Utils<double>::cumNormal(GLEVEL));
	  break;

	case 'c':
	  int dep;
	  if (sscanf(OptArg, "%d", &dep) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  if (dep != 0) FDRINDEP = false;
	  else FDRINDEP = true;
	  break;
	  
	case 'n':
	  if (sscanf(OptArg, "%d", &NSCALE) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  break;
	
	case 'F':
	  if (sscanf(OptArg, "%d", &FSCALE) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  break;

	case 'I':
	  int imd;
	  if (sscanf(OptArg, "%d", &imd) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  if (imd == 0)
	    ITERMODE = DIRECT;
	  else 
	    ITERMODE = L1REG;
	  break;
	
	case 'i':
	  if (sscanf(OptArg, "%d", &NITER) != 1)
	    {
	      cerr << "Bad or missing parameter " << OptArg << endl;
	      exit (-1);
	    }
	  break;

	case 'B':
	  int border;
	  if (sscanf(OptArg, "%d", &border) != 1)
	    {
		  cerr << "Bad or missing parameter " << OptArg << endl;
		  exit(-1);
	    }
	   if (border == 0)
		 TBORDER = I_CONT;
	   else if (border == 1)
		 TBORDER = I_MIRROR;
	   else if (border == 2)
		 TBORDER = I_PERIOD;
	   else
		 TBORDER = I_ZERO;
	   break;

	case 'K': 
	  KILLLAST = true;
	  break;

	case 'T': 
	  TRANSF2 = true;
	  break;

	case 'p': 
	  DETPOS = true;
	  break;

	case 'v': 
	  VERBOSE = true;
	  break;
	
	case '?':
	default: 
	  usage(argv); 
	  exit(-1);
	}
  } 
       
    // get optional input file names from trailing parameters and open files 
    if (OptInd < argc) strcpy(Name_Imag_In, argv[OptInd++]);
    else { cerr << "Error : Input image required." << endl; usage(argv); exit(-1); }

    if (OptInd < argc) strcpy(Name_Imag_Out, argv[OptInd++]);
    else { cerr << "Error : Output image required." << endl; usage(argv); exit(-1); }
    
    // make sure there are not too many parameters 
    if (OptInd < argc)
    {
    	fprintf(OUTMAN, "Too many parameters: %s ...\n", argv[OptInd]);
	    usage(argv);
    	exit(-1);
    }
}

// max. scale of the data
int scaleOfData (int dim, int nx, int ny, int nz)
{
	int s = 0;
	
	if (dim == 1)
	{
		s = iilog2(nx);
	}
	else if (dim == 2)
	{
		s = MIN(iilog2(nx), iilog2(ny));
	}
	else // dim == 3
	{
		s = MIN(iilog2(nx), iilog2(ny));
		s = MIN(s, iilog2(nz));
	}
	return s;
}

// display the content of a data array (useful in debug mode)
template <typename DATATYPE>
void display (convert_to_array<DATATYPE, true> &data)
{
	int dim = data.naxis();
	if (VERBOSE)
	  {
	    if (dim == 1)
	      {
		for (int x=0; x<data.nx(); x++)
		  cout << "(" << x << ")=" << data(x) << " ";
		cout << endl << endl;
	      }
	    else if (dim == 2)
	      {
		for (int y=0; y<data.ny(); y++)
		  {
		    for (int x=0; x<data.nx(); x++)
		      cout << "(" << x << "," << y << ")=" << data(x, y) << " ";
		    cout << endl << endl;
		  }
	      }
	    else
	      {
		for (int z=0; z<data.nz(); z++)
		  {
		    for (int y=0; y<data.ny(); y++)
		      {
			for (int x=0; x<data.nx(); x++)
			  cout << "(" << x << "," << y << "," << z << ")=" << data(x, y, z) << " ";
			cout << endl << endl;
		      }
		    cout << endl << endl;
		  }
	      }
	  }
	cout << " data information : " << endl;
	data.info();
	cout << endl << endl;
}

template <typename DATATYPE>
void setConst (convert_to_array<DATATYPE, true> &dest, convert_to_array<DATATYPE, true> &ref, double cst)
{
	int nx = ref.nx(), ny = ref.ny(), nz = ref.nz();
	int len = ref.n_elem();

	dest.resize(nx, ny, nz);
    for (int i=0; i<len; i++)
		dest(i) = (DATATYPE)cst;		
}

// Wavelet denoising - general process
template <typename SUPTYPE>
void b3SplineDenoise (fltarray &data, convert_to_array<SUPTYPE, true> *multiSup)
{
	int dim = data.naxis();
	double pr, fdrp; // threshold p-value and FDR-threshold p-value
	int N = data.n_elem();
    bool ms = (multiSup != NULL);
    
	// wavelet filter configuration
	B3VSTAtrous atrous(TBORDER);
	WaveletShrinkage<float, SUPTYPE> ws;

	fltarray *ch = new fltarray;
	fltarray *cg = new fltarray;
    
	for (int s=1; s<=NSCALE; s++)
	{
		atrous.transform(data, *ch, *cg, s);
		data = *ch;
		
        if (s < FSCALE)
        {
            if (ms)
            {
                multiSup[s-1].resize(cg->nx(), cg->ny(), cg->nz());
                for (int i=0; i<N; i++) (multiSup[s-1])(i) = -1;
            }
        }		
		else if (PROGMODE == PV)
		{
          // sigma = l2 norm of Id-B3 filter
		  if (VERBOSE)
              cout << "Var = " << Utils<double>::b3VSTSepCoefVar (dim, s) << endl;
          pr = ws.gaussHardThreshold(*cg, PROBA[PROGMODE], 
                                     sqrt(Utils<double>::b3VSTSepCoefVar (dim, s)), 
                                     ms ? &multiSup[s-1] : NULL);
		  if (VERBOSE)
		    cout << "scale = " << s << " cut-off p-value = " << setprecision(6) << pr << endl;                 
        }
		else if (PROGMODE == FDR)
		{
		  if (VERBOSE)
              cout << "Var = " << Utils<double>::b3VSTSepCoefVar (dim, s) << endl;
          fdrp = ws.gaussFDRThreshold(*cg, sqrt(Utils<double>::b3VSTSepCoefVar (dim, s)), 
                                      PROBA[PROGMODE], FDRINDEP, ms ? &multiSup[s-1] : NULL);
		  if (VERBOSE)
		    cout << "scale = " << s << " cut-off p-value = " << setprecision(6) << fdrp << endl;                 
        }
	}
    
	delete ch; delete cg; ch = NULL; cg = NULL; 
}

// for different modes of iteration
template <typename SUPTYPE>
void procArr (fltarray &origdata, fltarray &data, convert_to_array<SUPTYPE, true> &coef, double lambda, int itr)
{
  int n = data.n_elem();

  for (int x=0; x<n; x++)
  {
      if (itr == 0)
      {
        data(x) = (coef(x) < 0) ? 0. : data(x);
        if (DETPOS)
            data(x) = (origdata(x) < 0) ? 0. : data(x);
      }
      if (DETPOS)
          data(x) = ((coef(x) >= 0) && (origdata(x) >= 0)) ? origdata(x) : data(x);
      else
          data(x) = (coef(x) >= 0) ? origdata(x) : data(x);
      if (ITERMODE == L1REG)
           data(x) = soft_threshold(data(x), lambda);
  }
}

// put to zeros the negative values
void posProject (fltarray &data)
{	
	float value;
	int len = data.n_elem();
	
	for (int i=0; i<len; i++)
	{
		value = data(i);
		data(i) = MAX(value, 0);
	}
}

// solve with the iterative algo. in using the multiresolution support
void multiSupIter (fltarray &origdata, fltarray &solution, fltarray *multiSup, int niter)
{
  int dim = origdata.naxis();
  SplineAtrous atrous = SplineAtrous(3, 1., TBORDER, TRANSF2);
  double lambda = 0, delta = 0;
  
  if (ITERMODE == L1REG)
  {
      if (niter <= 1) return;
      lambda = 1.;
      delta = lambda / (niter-1);
  }
    
  fltarray *chs = new fltarray;
  fltarray *cgs = new fltarray[NSCALE];
  fltarray *tempd = new fltarray;
  fltarray *ch = new fltarray;
  fltarray *cg = new fltarray;

  for (int i=0; i<niter; i++)
  {
      if (VERBOSE) cout << "Iteration = " << (i+1) << " ... " << endl;
      *tempd = origdata;
      
	  for (int s=1; s<=NSCALE; s++)
	  {
	    atrous.transform(*tempd, *ch, *cg, s);
		atrous.transform(solution, *chs, cgs[s-1], s);
		procArr<float>(*cg, cgs[s-1], multiSup[s-1], lambda, i);
		solution = *chs;
		*tempd = *ch;
	  }
	  if ((KILLLAST) && (i==niter-1))
        setConst(*chs, *ch, 0.);    
	  else
        *chs = *ch; // same approximation band

	  for (int s=NSCALE; s>=1; s--)
	  {
	    atrous.recons(*chs, cgs[s-1], solution, s);
	    *chs = solution;
      }
      
      posProject(solution);
      lambda -= delta;
  }
  
  delete chs; delete [] cgs; chs = NULL; cgs = NULL;
  delete tempd; tempd = NULL;
  delete ch; delete cg; ch = NULL; cg = NULL;
}

void denoise (fltarray &data)
{	
    int dim = data.naxis();
	// backup the original data
	fltarray *origData = new fltarray;
	*origData = data;
    
    fltarray *multiSup = NULL;
    if (NITER > 1)
    {
 	    multiSup = new fltarray[NSCALE];
    }

	if (VERBOSE)
	  cout << "Initial denoising ... " << endl;
	  
    b3SplineDenoise<float>(data, multiSup);
    data = *origData;
    	
    if (NITER > 1)
    {
    	if (VERBOSE)
	       cout << "Entering into the iterative denoising ..." << endl;
        multiSupIter (*origData, data, multiSup, NITER);
	    if (VERBOSE)
	       cout << "Iteration complete." << endl;
    }
    
	delete origData; origData = NULL; 
	if (multiSup != NULL)
	{
        delete [] multiSup; multiSup = NULL;
    }
}

//*********************************************************************
int main(int argc, char *argv[])
{
   char cmd[512];
   cmd[0] = '\0';
   for (int k=0; k<argc; k++) 
   	sprintf(cmd, "%s %s", cmd, argv[k]);

   fitsstruct header, mheader;
   fltarray *data = new fltarray;
   fltarray *model = NULL;
       
   // Get command line arguments, open input file(s) if necessary
   filtinit(argc, argv);
   if (VERBOSE)
   {
       	cout << "Input image : " << Name_Imag_In << endl;
       	cout << "Output image : " << Name_Imag_Out << endl;
   }
   fits_read_fltarr(Name_Imag_In, *data, &header);

   header.bitpix = BP_FLOAT;
   header.origin = cmd;

   int dim = data->naxis();
   int nx = data->nx(), ny = data->ny(), nz = data->nz();
   NSCALE = MIN(NSCALE, scaleOfData(dim, nx, ny, nz));
   if ((NITER > 0) && (NITER <= 9))
        NITER = 10;
   if ((KILLLAST) && (NITER <= 9))
        NITER = 10;
   if ((DETPOS) && (NITER <= 9))
        NITER = 10;

   if (VERBOSE)
   {
        cout << "Iteration Filter g = " << ((TRANSF2) ? "Id-h*h" : "Id-h") << endl;
        cout << "Mode : ";
        if (PROGMODE == PV)
    		cout << "Individual tests" << endl;
        else if (PROGMODE == FDR)
    		cout << "FDR multiple tests" << endl;
            
        if (PROGMODE == FDR)
        {
    	    if (FDRINDEP) cout << "independence mode" << endl;
       		else cout << "dependence mode" << endl;
        }
    
    	if (ITERMODE == DIRECT)
    		cout << "Iterative Mode : Direct" << endl;
    	else
    		cout << "Iterative Mode : L1 - Regularized" << endl;
    	cout << "Max. Iteration : " << NITER << endl;
    
		cout << "Border mode : " << TBORDER << endl;
        cout << "Max scale(s) : " << NSCALE << endl;
	    cout << "First detection scale : " << FSCALE << endl;
	    cout << "Ignore the last approx. band : " << (KILLLAST ? "true" : "false") << endl;
	    cout << "Detect only positive coefficients : " << (DETPOS ? "true" : "false") << endl;
   }
   
   // Denoising
   try {
	   denoise(*data);

       if (VERBOSE)
    	   cout << "Writing denoising result file ... " << endl;
       fits_write_fltarr(Name_Imag_Out, *data, &header);
       if (VERBOSE)
    	 cout << "Writing complete." << endl;

       delete data; data = NULL; 
   }
   catch (PoisMSVSTException msvstExcept)
   {
   	cerr << "Exception : " << endl;
   	cerr << msvstExcept.getReason() << endl;
   	exit (-1);
   }

   return 0;
}
