// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// Sparse Hessian Data Structures File
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
// File                 : chem_HessianSP.c
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



// Hessian Sparse Data
// 
 /* Index i of Hessian element d^2 f_i/dv_j.dv_k */

  int  IHESS_I[] = {
       0,  1,  2 }; 

 /* Index j of Hessian element d^2 f_i/dv_j.dv_k */

  int  IHESS_J[] = {
       1,  1,  1 }; 

 /* Index k of Hessian element d^2 f_i/dv_j.dv_k */

  int  IHESS_K[] = {
       2,  2,  2 }; 


