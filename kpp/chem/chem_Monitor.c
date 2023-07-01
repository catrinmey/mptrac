// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// 
// Utility Data Initialization
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
// File                 : chem_Monitor.c
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



 /* Names of chemical species */

  char *  SPC_NAMES[] = {
     "n2o","ccl3f","ccl2f2","cclf3","o1d","o3","o3p","h2o2",
     "oh","ho2","o2","n2","h2o" }; 

 /* Indexes of species to look at */

  int  LOOKAT[] = {
       5 }; 

 /* Indexes of species to monitor */

  int  MONITOR[] = {
       5 }; 

 /* Names of atoms for mass balance */

  char *  SMASS[] = {
     0 }; 

 /* Equation names */

  char *  EQN_NAMES[] = {
     "    o3p + o2 --> o3                    ",
     "    o3 + o3p --> 2 o2                  ",
     "    o1d + o2 --> o3p + o2              ",
     "    o1d + o3 --> o3p + 1.5 o2          ",
     "   o1d + h2o --> 2 oh                  ",
     "    o1d + n2 --> o3p + n2              ",
     "    o1d + n2 --> n2o                   ",
     "   n2o + o1d -->                       ",
     " ccl3f + o1d --> 0.1 ccl3f + 0.1 o3p   ",
     "ccl2f2 + o1d --> 0.14 ccl2f2 + 0.14 o3p",
     " cclf3 + o1d --> 0.18 cclf3 + 0.18 o3p ",
     "   o3p + ho2 --> oh + o2               ",
     "  o3p + h2o2 --> oh + ho2              ",
     "     o3 + oh --> ho2 + o2              ",
     "        2 oh --> o3p + h2o             ",
     "        2 oh --> h2o2                  ",
     "    oh + ho2 --> o2 + h2o              ",
     "   h2o2 + oh --> ho2 + h2o             ",
     "    o3 + ho2 --> oh + 2 o2             ",
     "       2 ho2 --> h2o2 + o2             ",
     "          o2 --> 2 o3p                 ",
     "          o3 --> o1d                   ",
     "          o3 --> o3p + o2              ",
     "         ho2 --> o3p + oh              ",
     "        h2o2 --> o1d + oh              ",
     "         n2o --> o1d                   ",
     "       ccl3f -->                       ",
     "      ccl2f2 -->                       "
      }; 

 /* Names of chemical familes */

  char *  FAM_NAMES[] = {
     0 }; 


// INLINED global variables

// End INLINED global variables

