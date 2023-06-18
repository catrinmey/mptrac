// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// The ODE Function of Chemical Model File
// 
// Generated by KPP-3.0.0 symbolic chemistry Kinetics PreProcessor
//       (https:/github.com/KineticPreProcessor/KPP
// KPP is distributed under GPL, the general public licence
//       (http://www.gnu.org/copyleft/gpl.html)
// (C) 1995-1997, V. Damian & A. Sandu, CGRER, Univ. Iowa
// (C) 1997-2022, A. Sandu, Michigan Tech, Virginia Tech
//     With important contributions from:
//        M. Damian,   Villanova University, Philadelphia, PA, USA
//        R. Sander,   Max-Planck Institute for Chemistry, Mainz, Germany
//        M. Long,     Renaissance Fiber, LLC, North Carolina, USA
//        H. Lin,      Harvard University, Cambridge, MA, USA
//        R. Yantosca, Harvard University, Cambridge, MA, USA
// 
// File                 : chem_Function.c
// Equation file        : chem.kpp
// Output root filename : chem
// 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "chem_Parameters.h"
#include "chem_Global.h"
#include "chem_Sparse.h"


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// Fun - time derivatives of variables - Aggregate form
//   Arguments :
//      V         - Concentrations of variable species (local)
//      F         - Concentrations of fixed species (local)
//      RCT       - Rate constants (local)
//      Vdot      - Time derivative of variable species concentrations
//      Aout      - Optional argument to return equation rate constants
// 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Fun( 
  double V[NVAR],                        /* Concentrations of variable species (local) */
  double F[NFIX],                        /* Concentrations of fixed species (local) */
  double RCT[NREACT],                    /* Rate constants (local) */
  double Vdot[NVAR],                     /* Time derivative of variable species concentrations */
  double Aout[NREACT]                    /* Optional argument to return equation rate constants */
)
{

// Local variables
double A[NREACT];                        /* Rate for each equation */

// Computation of equation rates
  A[0] = RCT[0]*V[1]*V[2];
  A[1] = RCT[1]*V[2]*F[0];
  A[2] = RCT[2]*F[1]*F[1];


// Aggregate function
  Vdot[0] = A[0]+A[1];
  Vdot[1] = -A[0]+A[2];
  Vdot[2] = -A[0]-A[1];
}

// End of Fun function
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// Fun_SPLIT - time derivatives of variables - Split form
//   Arguments :
//      V         - Concentrations of variable species (local)
//      F         - Concentrations of fixed species (local)
//      RCT       - Rate constants (local)
//      Vdot      - Time derivative of variable species concentrations
//      P_VAR     - Production term
//      D_VAR     - Destruction term
//      Aout      - Optional argument to return equation rate constants
// 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void Fun_SPLIT( 
  double V[NVAR],                        /* Concentrations of variable species (local) */
  double F[NFIX],                        /* Concentrations of fixed species (local) */
  double RCT[NREACT],                    /* Rate constants (local) */
  double Vdot[NVAR],                     /* Time derivative of variable species concentrations */
  double P_VAR[NVAR],                    /* Production term */
  double D_VAR[NVAR],                    /* Destruction term */
  double Aout[NREACT]                    /* Optional argument to return equation rate constants */
)
{

// Local variables
double A[NREACT];                        /* Rate for each equation */

// Computation of equation rates
  A[0] = RCT[0]*V[1]*V[2];
  A[1] = RCT[1]*V[2]*F[0];
  A[2] = RCT[2]*F[1]*F[1];


// Production function
  P_VAR[0] = A[0]+A[1];
  P_VAR[1] = A[2];
  P_VAR[2] = 0;

// Destruction function
  D_VAR[0] = 0;
  D_VAR[1] = RCT[0]*V[2];
  D_VAR[2] = RCT[0]*V[1]+RCT[1]*F[0];

  for( int n=0; n<NVAR; n++ ) {
    Vdot[n] = P_VAR[n] - D_VAR[n]*V[n];
  }
}

// End of Fun_SPLIT function
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// CalcStoichNum - calculate stoichiometric numbers
//   Arguments :
//      StoichNum - Stoichiometric numbers
// 
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void CalcStoichNum( 
  double StoichNum[NVAR][NREACT]         /* Stoichiometric numbers */
)
{
  StoichNum[0][0] = 1;
  StoichNum[0][1] = 1;
  StoichNum[1][0] = -1;
  StoichNum[1][2] = 1;
  StoichNum[2][0] = -1;
  StoichNum[2][1] = -1;
}

// End of CalcStoichNum function
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


