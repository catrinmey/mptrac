/*
  This file is part of MPTRAC.
  
  MPTRAC is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  MPTRAC is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with MPTRAC. If not, see <http://www.gnu.org/licenses/>.
  
  Copyright (C) 2013-2023 Forschungszentrum Juelich GmbH
*/

/*! 
  \file
  MPTRAC library definitions.
*/

#include "libtrac.h"

/*****************************************************************************/

double buoyancy_frequency(
  double p0,
  double t0,
  double p1,
  double t1) {

  double theta0 = THETA(p0, t0);
  double theta1 = THETA(p1, t1);
  double dz = RI / MA / G0 * 0.5 * (t0 + t1) * (log(p0) - log(p1));

  return sqrt(2. * G0 / (theta0 + theta1) * (theta1 - theta0) / dz);
}

/*****************************************************************************/

void cart2geo(
  double *x,
  double *z,
  double *lon,
  double *lat) {

  double radius = NORM(x);
  *lat = asin(x[2] / radius) * 180. / M_PI;
  *lon = atan2(x[1], x[0]) * 180. / M_PI;
  *z = radius - RE;
}

/*****************************************************************************/

double clim_hno3(
  clim_t * clim,
  double t,
  double lat,
  double p) {

  /* Get seconds since begin of year... */
  double sec = FMOD(t, 365.25 * 86400.);
  while (sec < 0)
    sec += 365.25 * 86400.;

  /* Check pressure... */
  if (p < clim->hno3_p[0])
    p = clim->hno3_p[0];
  else if (p > clim->hno3_p[clim->hno3_np - 1])
    p = clim->hno3_p[clim->hno3_np - 1];

  /* Check latitude... */
  if (lat < clim->hno3_lat[0])
    lat = clim->hno3_lat[0];
  else if (lat > clim->hno3_lat[clim->hno3_nlat - 1])
    lat = clim->hno3_lat[clim->hno3_nlat - 1];

  /* Get indices... */
  int isec = locate_irr(clim->hno3_time, clim->hno3_ntime, sec);
  int ilat = locate_reg(clim->hno3_lat, clim->hno3_nlat, lat);
  int ip = locate_irr(clim->hno3_p, clim->hno3_np, p);

  /* Interpolate HNO3 climatology (Froidevaux et al., 2015)... */
  double aux00 = LIN(clim->hno3_p[ip],
		     clim->hno3[isec][ilat][ip],
		     clim->hno3_p[ip + 1],
		     clim->hno3[isec][ilat][ip + 1], p);
  double aux01 = LIN(clim->hno3_p[ip],
		     clim->hno3[isec][ilat + 1][ip],
		     clim->hno3_p[ip + 1],
		     clim->hno3[isec][ilat + 1][ip + 1], p);
  double aux10 = LIN(clim->hno3_p[ip],
		     clim->hno3[isec + 1][ilat][ip],
		     clim->hno3_p[ip + 1],
		     clim->hno3[isec + 1][ilat][ip + 1], p);
  double aux11 = LIN(clim->hno3_p[ip],
		     clim->hno3[isec + 1][ilat + 1][ip],
		     clim->hno3_p[ip + 1],
		     clim->hno3[isec + 1][ilat + 1][ip + 1], p);
  aux00 = LIN(clim->hno3_lat[ilat], aux00,
	      clim->hno3_lat[ilat + 1], aux01, lat);
  aux11 = LIN(clim->hno3_lat[ilat], aux10,
	      clim->hno3_lat[ilat + 1], aux11, lat);
  aux00 = LIN(clim->hno3_time[isec], aux00,
	      clim->hno3_time[isec + 1], aux11, sec);

  /* Convert from ppb to ppv... */
  return GSL_MAX(1e-9 * aux00, 0.0);
}

/*****************************************************************************/

void clim_hno3_init(
  clim_t * clim) {

  /* Write info... */
  LOG(1, "Initialize HNO3 data...");

  clim->hno3_ntime = 12;
  double hno3_time[12] = {
    1209600.00, 3888000.00, 6393600.00,
    9072000.00, 11664000.00, 14342400.00,
    16934400.00, 19612800.00, 22291200.00,
    24883200.00, 27561600.00, 30153600.00
  };
  memcpy(clim->hno3_time, hno3_time, sizeof(clim->hno3_time));

  clim->hno3_nlat = 18;
  double hno3_lat[18] = {
    -85, -75, -65, -55, -45, -35, -25, -15, -5,
    5, 15, 25, 35, 45, 55, 65, 75, 85
  };
  memcpy(clim->hno3_lat, hno3_lat, sizeof(clim->hno3_lat));

  clim->hno3_np = 10;
  double hno3_p[10] = {
    4.64159, 6.81292, 10, 14.678, 21.5443,
    31.6228, 46.4159, 68.1292, 100, 146.78
  };
  memcpy(clim->hno3_p, hno3_p, sizeof(clim->hno3_p));

  double hno3[12][18][10] = {
    {{0.782, 1.65, 2.9, 4.59, 6.71, 8.25, 7.16, 5.75, 2.9, 1.74},
     {0.529, 1.64, 2.76, 4.55, 6.58, 8, 6.99, 5.55, 2.68, 1.57},
     {0.723, 1.55, 2.73, 4.48, 6.32, 7.58, 7.05, 5.16, 2.49, 1.54},
     {0.801, 1.56, 2.74, 4.52, 6.23, 7.35, 6.68, 4.4, 1.97, 1.23},
     {0.818, 1.62, 2.77, 4.38, 5.98, 6.84, 5.83, 3.05, 1.15, 0.709},
     {0.901, 1.73, 2.78, 4.21, 5.63, 6.16, 4.68, 1.87, 0.617, 0.37},
     {0.997, 1.8, 2.79, 4.09, 4.88, 4.96, 3.12, 1.22, 0.311, 0.244},
     {1, 1.71, 2.51, 3.4, 3.74, 3.39, 2.25, 0.845, 0.204, 0.222},
     {0.997, 1.7, 2.36, 2.88, 3.01, 2.25, 1.77, 0.608, 0.163, 0.181},
     {0.991, 1.79, 2.57, 3.06, 3.08, 2.15, 1.81, 0.59, 0.168, 0.104},
     {0.974, 1.86, 2.84, 3.8, 3.93, 3.79, 2.91, 1.02, 0.152, 0.0985},
     {0.85, 1.86, 3.3, 5.24, 6.55, 6.86, 5.12, 1.93, 0.378, 0.185},
     {0.783, 1.89, 3.85, 6.6, 8.56, 8.66, 6.95, 3.95, 1.47, 0.745},
     {0.883, 2.05, 4.34, 7.54, 9.68, 9.77, 8.19, 5.72, 3.15, 1.77},
     {1.4, 2.44, 4.72, 8.07, 10.5, 10.9, 9.28, 6.95, 4.47, 2.49},
     {1.7, 2.43, 4.24, 7.43, 10.4, 11.2, 9.72, 8.15, 5.7, 2.97},
     {2.06, 2.27, 3.68, 6.77, 10.3, 10.3, 9.05, 9.1, 6.73, 3.14},
     {2.33, 2.39, 3.51, 6.45, 10.3, 9.88, 8.57, 9.42, 7.22, 3.19}},
    {{0.947, 2.21, 3.81, 5.69, 7.55, 8.63, 7.53, 5.98, 3.03, 1.64},
     {0.642, 2, 3.4, 5.49, 7.5, 8.52, 7.53, 5.83, 2.74, 1.42},
     {0.756, 1.83, 3.18, 5.11, 7.24, 8.63, 7.66, 5.5, 2.45, 1.33},
     {0.837, 1.75, 3.06, 5, 6.79, 8.08, 7.05, 4.42, 1.81, 1.05},
     {0.86, 1.73, 2.96, 4.68, 6.38, 7.38, 6.09, 2.92, 1.06, 0.661},
     {0.926, 1.78, 2.89, 4.37, 5.74, 6.14, 4.59, 1.78, 0.561, 0.332},
     {0.988, 1.78, 2.75, 3.95, 4.64, 4.49, 2.85, 1.13, 0.271, 0.184},
     {0.999, 1.7, 2.44, 3.27, 3.57, 3.03, 2.06, 0.736, 0.181, 0.189},
     {0.971, 1.67, 2.23, 2.63, 2.83, 2.15, 1.74, 0.554, 0.157, 0.167},
     {0.985, 1.72, 2.34, 2.69, 2.81, 2.11, 1.78, 0.592, 0.152, 0.101},
     {0.95, 1.72, 2.57, 3.44, 3.84, 3.89, 2.91, 0.976, 0.135, 0.114},
     {0.819, 1.64, 2.93, 4.75, 6.02, 6.93, 5.2, 1.83, 0.347, 0.191},
     {0.731, 1.58, 3.3, 5.95, 7.81, 8.32, 6.93, 3.83, 1.47, 0.875},
     {0.77, 1.75, 3.74, 6.67, 8.76, 9.41, 8.19, 5.78, 3.32, 2.11},
     {1.08, 2.17, 4.24, 7.13, 9.2, 10.3, 9.03, 6.87, 4.65, 3.01},
     {1.43, 2.49, 4.31, 7, 9.14, 10.6, 9.34, 7.6, 5.86, 3.64},
     {1.5, 2.68, 4.32, 6.75, 8.78, 10.6, 9.05, 7.65, 6.27, 4.07},
     {1.73, 2.91, 4.33, 6.67, 8.73, 10.6, 8.5, 7.54, 6.63, 4.17}},
    {{1.43, 3.07, 5.22, 7.54, 9.78, 10.4, 10.1, 7.26, 3.61, 1.69},
     {0.989, 2.69, 4.76, 7.19, 9.44, 9.94, 9.5, 6.74, 3.24, 1.52},
     {0.908, 2.23, 4.11, 6.48, 8.74, 9.41, 8.58, 5.8, 2.66, 1.3},
     {0.923, 1.99, 3.61, 5.83, 7.84, 8.6, 7.55, 4.57, 1.87, 0.98},
     {0.933, 1.9, 3.31, 5.28, 7.1, 7.84, 6.44, 3.18, 1.1, 0.642},
     {0.982, 1.88, 3.1, 4.76, 6.16, 6.57, 5.16, 2.04, 0.598, 0.33},
     {1.02, 1.82, 2.88, 4.12, 4.71, 4.54, 3.03, 1.22, 0.268, 0.174},
     {0.992, 1.7, 2.51, 3.33, 3.62, 2.87, 2.05, 0.705, 0.161, 0.169},
     {0.969, 1.69, 2.2, 2.62, 2.84, 2.13, 1.78, 0.529, 0.146, 0.186},
     {0.945, 1.69, 2.27, 2.64, 2.83, 2.2, 1.83, 0.561, 0.139, 0.121},
     {0.922, 1.65, 2.48, 3.33, 3.83, 4.09, 2.92, 0.973, 0.117, 0.135},
     {0.886, 1.59, 2.66, 4.26, 5.51, 6.57, 5.09, 1.79, 0.342, 0.194},
     {0.786, 1.5, 2.78, 5.01, 6.8, 7.83, 6.65, 3.62, 1.45, 1},
     {0.745, 1.55, 3.05, 5.49, 7.44, 8.6, 7.8, 5.28, 2.95, 2.12},
     {0.938, 1.76, 3.4, 5.82, 7.8, 9.04, 8.43, 6.15, 3.85, 2.82},
     {0.999, 2, 3.66, 5.95, 7.94, 9.27, 8.8, 6.93, 4.87, 3.54},
     {1.13, 2.23, 3.86, 5.82, 7.65, 9, 8.82, 7.17, 5.72, 4.08},
     {1.23, 2.33, 3.94, 5.74, 7.48, 8.9, 8.84, 7.35, 6.3, 4.42}},
    {{1.55, 3.2, 6.25, 10, 12.9, 12.9, 11.9, 7.96, 3.96, 1.75},
     {1.32, 3.27, 6.32, 9.99, 12.7, 12.4, 11.3, 7.51, 3.66, 1.58},
     {1.25, 3.08, 5.77, 8.71, 11.2, 11.2, 9.84, 6.52, 3.23, 1.5},
     {1.18, 2.59, 4.76, 7.46, 9.61, 9.66, 8.42, 5.06, 2.25, 1.09},
     {1.09, 2.24, 3.99, 6.4, 8.33, 8.54, 7.08, 3.69, 1.36, 0.727},
     {1.06, 2.07, 3.52, 5.52, 7.06, 7.26, 5.83, 2.46, 0.732, 0.409},
     {1.07, 1.91, 3.09, 4.63, 5.21, 4.9, 3.68, 1.43, 0.326, 0.198},
     {1.03, 1.74, 2.63, 3.54, 3.78, 2.89, 2.09, 0.743, 0.175, 0.12},
     {0.959, 1.71, 2.32, 2.77, 2.99, 2.24, 1.76, 0.519, 0.149, 0.172},
     {0.931, 1.68, 2.32, 2.74, 2.99, 2.46, 1.88, 0.578, 0.156, 0.157},
     {0.933, 1.66, 2.49, 3.42, 3.99, 4.12, 2.93, 1.02, 0.181, 0.138},
     {0.952, 1.64, 2.6, 4, 5.15, 6.07, 4.84, 1.78, 0.407, 0.286},
     {0.84, 1.54, 2.68, 4.47, 5.97, 7.13, 6.23, 3.25, 1.38, 1.02},
     {0.714, 1.44, 2.73, 4.68, 6.28, 7.68, 7.21, 4.82, 2.55, 1.96},
     {0.838, 1.57, 2.96, 4.93, 6.55, 8.08, 7.74, 5.77, 3.32, 2.52},
     {0.823, 1.65, 3.11, 5.09, 6.89, 8.36, 8.31, 6.59, 4.1, 3.04},
     {0.886, 1.83, 3.42, 5.33, 6.92, 8.36, 8.63, 7.21, 4.82, 3.46},
     {1.07, 2.12, 3.74, 5.54, 6.98, 8.41, 8.75, 7.41, 5.16, 3.62}},
    {{1.13, 2.59, 7.49, 13.5, 15.4, 12.9, 11.3, 8.62, 4.18, 1.63},
     {0.973, 2.79, 7.23, 12.8, 15.2, 13.3, 11.6, 8.42, 4.06, 1.57},
     {1.46, 3.44, 6.78, 10.4, 12.7, 12.1, 10.5, 7.04, 3.59, 1.63},
     {1.52, 3.38, 6.04, 9.08, 11, 10.3, 8.9, 5.7, 2.77, 1.37},
     {1.32, 2.65, 4.75, 7.49, 9.32, 8.89, 7.42, 4.27, 1.7, 0.88},
     {1.19, 2.2, 3.88, 6.36, 8.03, 7.81, 6.19, 2.94, 0.948, 0.527},
     {1.14, 1.96, 3.28, 5.26, 6.12, 5.8, 4.47, 1.66, 0.388, 0.229},
     {1.07, 1.82, 2.82, 3.92, 4.03, 3.15, 2.31, 0.871, 0.183, 0.0972},
     {0.978, 1.77, 2.53, 3.04, 3.1, 2.36, 1.76, 0.575, 0.16, 0.126},
     {0.962, 1.72, 2.49, 3.01, 3.22, 2.72, 2, 0.716, 0.162, 0.183},
     {0.968, 1.7, 2.6, 3.57, 4.28, 4.35, 3.09, 1.2, 0.262, 0.18},
     {0.977, 1.68, 2.71, 4.03, 5.17, 6.01, 4.81, 1.81, 0.473, 0.343},
     {0.819, 1.58, 2.75, 4.37, 5.8, 6.9, 5.96, 2.95, 1.19, 0.964},
     {0.672, 1.44, 2.69, 4.42, 5.92, 7.26, 6.79, 4.32, 2.22, 1.83},
     {0.783, 1.42, 2.65, 4.45, 6.04, 7.57, 7.39, 5.4, 2.94, 2.25},
     {0.757, 1.43, 2.7, 4.54, 6.14, 7.65, 7.51, 5.95, 3.42, 2.39},
     {0.758, 1.57, 3.04, 4.88, 6.24, 7.85, 7.58, 6.35, 3.81, 2.52},
     {0.835, 1.72, 3.35, 5.24, 6.5, 8.1, 7.67, 6.51, 4, 2.6}},
    {{1.5, 2.12, 7.64, 10.5, 5.59, 2.14, 2.2, 3.5, 4.71, 3.26},
     {1.32, 2.14, 7.23, 12, 9.3, 5.3, 5.11, 5.37, 5.12, 3.05},
     {1.53, 2.92, 6.9, 11.9, 13.5, 11.3, 9.91, 7.18, 4.75, 2.65},
     {1.66, 3.48, 6.25, 9.53, 11.3, 10.3, 9.01, 5.76, 2.99, 1.67},
     {1.54, 3.03, 5.21, 8.03, 9.66, 8.98, 7.5, 4.64, 2.11, 1.13},
     {1.32, 2.39, 4.03, 6.74, 8.52, 8.05, 6.4, 3.48, 1.2, 0.639},
     {1.17, 2.08, 3.35, 5.52, 6.86, 6.54, 5.08, 1.97, 0.462, 0.217},
     {1.07, 1.92, 3.01, 4.24, 4.47, 3.77, 2.77, 1.07, 0.213, 0.0694},
     {0.992, 1.88, 2.76, 3.39, 3.32, 2.52, 1.8, 0.713, 0.192, 0.136},
     {0.992, 1.8, 2.63, 3.34, 3.46, 2.95, 2.09, 0.9, 0.242, 0.194},
     {0.987, 1.77, 2.67, 3.64, 4.37, 4.36, 3, 1.27, 0.354, 0.229},
     {0.979, 1.74, 2.77, 3.99, 5.12, 5.75, 4.53, 1.75, 0.555, 0.302},
     {0.832, 1.6, 2.78, 4.32, 5.53, 6.67, 5.69, 2.59, 0.982, 0.66},
     {0.696, 1.41, 2.64, 4.31, 5.65, 7.14, 6.56, 3.8, 1.75, 1.41},
     {0.788, 1.36, 2.59, 4.3, 5.73, 7.35, 7.04, 4.82, 2.41, 1.8},
     {0.761, 1.43, 2.61, 4.28, 5.64, 7.37, 7.11, 5.37, 2.68, 1.9},
     {0.701, 1.44, 2.82, 4.64, 5.76, 7.63, 7.07, 5.74, 2.98, 1.88},
     {0.763, 1.5, 2.95, 4.97, 6.08, 7.88, 7.12, 5.98, 3.21, 1.91}},
    {{3.58, 2.59, 6.49, 5.84, 1.63, 0.282, 0.647, 0.371, 1.36, 2.33},
     {3.09, 2.38, 6.37, 7.66, 4.06, 1.23, 1.8, 1.65, 2.32, 2.78},
     {2.31, 2.84, 5.58, 9.63, 11, 9.02, 8.2, 6.23, 4.17, 3.08},
     {1.61, 3.16, 5.72, 9.13, 11.4, 10.4, 9.15, 6.18, 3.52, 2.3},
     {1.32, 2.8, 4.79, 7.44, 9.43, 8.83, 7.41, 4.9, 2.38, 1.38},
     {1.14, 2.36, 3.94, 6.41, 8.38, 8.17, 6.53, 3.76, 1.31, 0.656},
     {1.05, 2.1, 3.36, 5.45, 7.07, 6.98, 5.44, 2.22, 0.52, 0.176},
     {1.02, 2, 3.05, 4.33, 4.74, 4.21, 3.2, 1.26, 0.277, 0.0705},
     {1.01, 1.96, 2.9, 3.53, 3.46, 2.69, 1.89, 0.859, 0.254, 0.12},
     {1.01, 1.86, 2.7, 3.46, 3.59, 3.03, 2.14, 1, 0.34, 0.199},
     {1.02, 1.81, 2.67, 3.68, 4.39, 4.3, 2.93, 1.35, 0.477, 0.25},
     {0.991, 1.79, 2.82, 4.05, 5.08, 5.5, 4.21, 1.74, 0.605, 0.259},
     {0.844, 1.73, 2.87, 4.38, 5.49, 6.47, 5.5, 2.44, 0.85, 0.422},
     {0.729, 1.57, 2.76, 4.43, 5.73, 7.13, 6.43, 3.52, 1.38, 0.913},
     {0.819, 1.46, 2.69, 4.45, 5.92, 7.47, 7.05, 4.52, 2, 1.4},
     {0.783, 1.47, 2.71, 4.48, 5.92, 7.46, 7.16, 5.08, 2.35, 1.56},
     {0.735, 1.51, 2.96, 4.84, 5.92, 7.77, 7.2, 5.54, 2.56, 1.61},
     {0.8, 1.61, 3.14, 5.2, 6.26, 8.08, 7.27, 5.72, 2.75, 1.62}},
    {{5, 4.43, 5.53, 5.35, 2.33, 0.384, 0.663, 0.164, 0.692, 1.4},
     {3.62, 3.79, 4.77, 5.94, 4.12, 1.36, 1.3, 0.973, 1.37, 1.73},
     {2.11, 2.7, 4.12, 7.14, 9.03, 7.74, 7.12, 5.44, 3.73, 2.6},
     {1.13, 2.32, 4.12, 6.97, 9.86, 9.69, 8.85, 6.22, 3.59, 2.14},
     {0.957, 2.28, 4.11, 6.47, 8.66, 8.78, 7.33, 4.94, 2.44, 1.38},
     {0.881, 2.1, 3.65, 5.94, 7.98, 8.29, 6.69, 3.95, 1.36, 0.672},
     {0.867, 1.96, 3.26, 5.23, 6.94, 7.2, 5.63, 2.41, 0.578, 0.19},
     {0.953, 1.94, 2.98, 4.23, 4.83, 4.52, 3.38, 1.34, 0.293, 0.181},
     {1.01, 1.91, 2.77, 3.35, 3.3, 2.62, 1.99, 0.905, 0.245, 0.107},
     {1.03, 1.81, 2.57, 3.29, 3.43, 2.87, 2.13, 0.988, 0.306, 0.185},
     {1.02, 1.78, 2.58, 3.59, 4.19, 4, 2.72, 1.29, 0.389, 0.224},
     {1.01, 1.84, 2.84, 4.06, 4.9, 5.08, 3.71, 1.64, 0.529, 0.232},
     {0.902, 1.84, 2.98, 4.43, 5.5, 6.28, 5.18, 2.35, 0.734, 0.341},
     {0.785, 1.68, 2.93, 4.67, 5.95, 7.3, 6.52, 3.48, 1.24, 0.754},
     {0.847, 1.62, 2.94, 4.86, 6.38, 7.99, 7.5, 4.64, 1.93, 1.23},
     {0.8, 1.6, 2.94, 4.95, 6.62, 8.16, 7.91, 5.43, 2.43, 1.45},
     {0.82, 1.76, 3.37, 5.47, 6.82, 8.24, 7.73, 5.79, 2.69, 1.5},
     {0.988, 2.05, 3.87, 6.01, 7.18, 8.41, 7.7, 5.93, 2.89, 1.55}},
    {{1.52, 2.7, 3.79, 4.95, 3.8, 1.51, 1.11, 0.784, 1.1, 1.56},
     {1.19, 2.16, 3.34, 4.76, 4.61, 2.93, 2.07, 1.65, 1.63, 1.74},
     {0.804, 1.65, 2.79, 4.63, 6.64, 6.95, 6.68, 5.11, 3.3, 2.09},
     {0.86, 1.8, 3.25, 5.3, 7.91, 8.76, 8.28, 6.01, 3.39, 1.83},
     {0.859, 1.95, 3.54, 5.64, 7.88, 8.55, 7.3, 4.88, 2.3, 1.22},
     {0.809, 1.88, 3.38, 5.45, 7.47, 8.02, 6.69, 3.98, 1.35, 0.646},
     {0.822, 1.81, 3.11, 4.9, 6.62, 6.96, 5.63, 2.47, 0.614, 0.169},
     {0.92, 1.83, 2.8, 3.93, 4.56, 4.4, 3.25, 1.31, 0.295, 0.0587},
     {0.986, 1.83, 2.6, 3.13, 3.08, 2.53, 1.94, 0.886, 0.244, 0.0815},
     {0.997, 1.74, 2.5, 3.16, 3.24, 2.67, 2.05, 0.939, 0.281, 0.147},
     {1.01, 1.75, 2.57, 3.55, 4.1, 3.81, 2.53, 1.21, 0.354, 0.197},
     {1.04, 1.88, 2.9, 4.16, 4.95, 4.96, 3.48, 1.63, 0.502, 0.163},
     {0.967, 1.95, 3.17, 4.72, 5.85, 6.5, 5.34, 2.53, 0.748, 0.303},
     {0.846, 1.83, 3.23, 5.15, 6.62, 7.82, 6.85, 3.79, 1.36, 0.714},
     {0.91, 1.81, 3.35, 5.55, 7.32, 8.55, 7.88, 5.03, 2.13, 1.1},
     {0.87, 1.94, 3.6, 5.97, 7.98, 9.14, 8.71, 6.04, 2.73, 1.41},
     {1.04, 2.36, 4.22, 6.57, 8.5, 9.53, 9.22, 6.71, 3.2, 1.56},
     {1.36, 2.84, 4.72, 6.94, 8.81, 9.87, 9.59, 7.1, 3.43, 1.65}},
    {{0.704, 1.4, 2.03, 3.08, 4.64, 4.24, 2.55, 1.57, 1.99, 1.91},
     {0.484, 1.38, 2.08, 3.54, 5.11, 4.98, 3.73, 2.57, 2.29, 1.84},
     {0.749, 1.57, 2.63, 4.17, 6.15, 6.97, 6.64, 5.11, 3.35, 1.97},
     {0.864, 1.69, 3.16, 4.87, 7.13, 8.33, 7.87, 5.9, 3.17, 1.56},
     {0.861, 1.79, 3.28, 5.2, 7.29, 8.32, 7.38, 4.9, 2.23, 1.11},
     {0.835, 1.79, 3.19, 4.99, 6.72, 7.58, 6.45, 3.68, 1.25, 0.616},
     {0.847, 1.8, 3.07, 4.66, 6.12, 6.6, 5.21, 2.18, 0.554, 0.21},
     {0.941, 1.78, 2.68, 3.68, 4.28, 4.18, 2.97, 1.15, 0.238, 0.0968},
     {0.98, 1.78, 2.48, 2.99, 2.96, 2.35, 1.88, 0.747, 0.207, 0.105},
     {0.978, 1.74, 2.51, 3.07, 3.12, 2.36, 1.95, 0.777, 0.216, 0.146},
     {1.01, 1.79, 2.63, 3.53, 3.95, 3.47, 2.38, 1.08, 0.265, 0.178},
     {1.06, 1.94, 3.02, 4.43, 5.19, 5.01, 3.68, 1.71, 0.429, 0.14},
     {0.99, 2.02, 3.38, 5.22, 6.56, 6.91, 5.56, 2.75, 0.816, 0.353},
     {0.923, 2.05, 3.66, 5.98, 7.78, 8.5, 7.23, 4.26, 1.67, 0.802},
     {1.08, 2.27, 4.17, 6.8, 8.89, 9.55, 8.59, 5.64, 2.58, 1.2},
     {1.12, 2.5, 4.52, 7.22, 9.76, 10.3, 9.72, 6.79, 3.32, 1.52},
     {1.2, 2.64, 4.81, 7.64, 10.5, 11.4, 10.6, 7.65, 3.87, 1.73},
     {1.4, 2.91, 5.01, 7.75, 10.7, 11.6, 11.1, 8.02, 4.04, 1.8}},
    {{0.75, 1.49, 2.39, 3.39, 4.93, 5.94, 5.03, 2.75, 2.27, 1.78},
     {0.508, 1.52, 2.38, 3.82, 5.34, 6.13, 5.6, 3.31, 2.42, 1.73},
     {0.715, 1.56, 2.7, 4.39, 6.18, 6.96, 7.1, 5.04, 3.01, 1.75},
     {0.813, 1.62, 2.94, 4.65, 6.53, 7.65, 7.52, 5.49, 2.75, 1.41},
     {0.802, 1.68, 2.97, 4.64, 6.37, 7.53, 7.01, 4.56, 1.9, 0.955},
     {0.816, 1.75, 3.01, 4.59, 6.15, 7.06, 6.15, 3.38, 1.11, 0.61},
     {0.867, 1.78, 2.92, 4.35, 5.69, 6.05, 4.73, 1.91, 0.519, 0.269},
     {0.932, 1.7, 2.55, 3.44, 4.03, 3.98, 2.74, 1.08, 0.247, 0.132},
     {0.937, 1.74, 2.51, 3.09, 3.11, 2.34, 1.84, 0.67, 0.189, 0.121},
     {0.942, 1.75, 2.63, 3.3, 3.27, 2.21, 1.87, 0.663, 0.171, 0.147},
     {0.959, 1.8, 2.82, 3.78, 4.03, 3.37, 2.53, 1.04, 0.199, 0.146},
     {1.01, 1.9, 3.13, 4.76, 5.63, 5.6, 4.31, 1.83, 0.367, 0.172},
     {0.989, 2.04, 3.64, 6, 7.62, 7.6, 6, 3.35, 1.05, 0.448},
     {1.02, 2.28, 4.32, 7.19, 9.21, 9.16, 7.64, 4.97, 2.2, 0.948},
     {1.26, 2.77, 5.2, 8.31, 10.5, 10.4, 9.01, 6.37, 3.46, 1.56},
     {1.31, 2.76, 5.23, 8.49, 11.2, 11.3, 10.1, 7.27, 3.98, 1.76},
     {1.26, 2.5, 5.14, 8.85, 12.3, 12.3, 11.2, 8.13, 4.45, 1.97},
     {1.35, 2.49, 5.26, 9.16, 13, 12.8, 11.8, 8.57, 4.72, 2.05}},
    {{0.759, 1.54, 2.54, 4.22, 6.26, 7.44, 7.14, 4.99, 2.84, 1.89},
     {0.508, 1.55, 2.5, 4.29, 6.29, 7.29, 7.07, 5.03, 2.77, 1.74},
     {0.699, 1.56, 2.62, 4.17, 6.08, 7.38, 7.04, 5.17, 2.81, 1.65},
     {0.778, 1.5, 2.65, 4.35, 6.07, 7.28, 6.84, 4.8, 2.28, 1.28},
     {0.772, 1.55, 2.71, 4.3, 5.76, 6.91, 6.2, 3.69, 1.45, 0.837},
     {0.836, 1.67, 2.78, 4.21, 5.56, 6.41, 5.33, 2.47, 0.807, 0.488},
     {0.937, 1.79, 2.78, 4.12, 5.17, 5.38, 3.89, 1.47, 0.392, 0.256},
     {0.97, 1.75, 2.52, 3.39, 3.83, 3.63, 2.48, 0.968, 0.212, 0.198},
     {0.968, 1.74, 2.5, 3.11, 3.2, 2.34, 1.79, 0.629, 0.169, 0.173},
     {0.98, 1.8, 2.69, 3.42, 3.4, 2.18, 1.81, 0.606, 0.164, 0.138},
     {0.975, 1.84, 2.96, 4.08, 4.12, 3.5, 2.79, 1.02, 0.145, 0.133},
     {0.96, 1.94, 3.27, 5.17, 6.26, 6.35, 4.88, 1.91, 0.329, 0.189},
     {0.954, 2.06, 3.8, 6.53, 8.46, 8.32, 6.53, 3.83, 1.32, 0.6},
     {1, 2.34, 4.58, 7.71, 9.68, 9.75, 7.96, 5.45, 2.84, 1.39},
     {1.24, 2.65, 5.14, 8.51, 10.7, 10.6, 8.96, 6.51, 3.83, 1.85},
     {1.34, 2.44, 4.99, 8.63, 11.6, 11.4, 10.1, 7.84, 4.77, 2.24},
     {1.33, 2.1, 4.76, 8.78, 12.2, 11.7, 10.8, 8.68, 5.15, 2.35},
     {1.42, 2.04, 4.68, 8.92, 12.7, 12, 11.2, 8.99, 5.32, 2.33}}
  };
  memcpy(clim->hno3, hno3, sizeof(clim->hno3));

  /* Get range... */
  double hno3min = 1e99, hno3max = -1e99;
  for (int it = 0; it < clim->hno3_ntime; it++)
    for (int iz = 0; iz < clim->hno3_np; iz++)
      for (int iy = 0; iy < clim->hno3_nlat; iy++) {
	hno3min = GSL_MIN(hno3min, clim->hno3[it][iy][iz]);
	hno3max = GSL_MAX(hno3max, clim->hno3[it][iy][iz]);
      }

  /* Write info... */
  LOG(2, "Number of time steps: %d", clim->hno3_ntime);
  LOG(2, "Time steps: %.2f, %.2f ... %.2f s",
      clim->hno3_time[0], clim->hno3_time[1],
      clim->hno3_time[clim->hno3_ntime - 1]);
  LOG(2, "Number of pressure levels: %d", clim->hno3_np);
  LOG(2, "Altitude levels: %g, %g ... %g km",
      Z(clim->hno3_p[0]), Z(clim->hno3_p[1]),
      Z(clim->hno3_p[clim->hno3_np - 1]));
  LOG(2, "Pressure levels: %g, %g ... %g hPa", clim->hno3_p[0],
      clim->hno3_p[1], clim->hno3_p[clim->hno3_np - 1]);
  LOG(2, "Number of latitudes: %d", clim->hno3_nlat);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      clim->hno3_lat[0], clim->hno3_lat[1],
      clim->hno3_lat[clim->hno3_nlat - 1]);
  LOG(2, "HNO3 concentration range: %g ... %g ppv", 1e-9 * hno3min,
      1e-9 * hno3max);
}

/*****************************************************************************/

double clim_oh(
  clim_t * clim,
  double t,
  double lat,
  double p) {

  /* Get seconds since begin of year... */
  double sec = FMOD(t, 365.25 * 86400.);
  while (sec < 0)
    sec += 365.25 * 86400.;

  /* Check pressure... */
  if (p < clim->oh_p[clim->oh_np - 1])
    p = clim->oh_p[clim->oh_np - 1];
  else if (p > clim->oh_p[0])
    p = clim->oh_p[0];

  /* Check latitude... */
  if (lat < clim->oh_lat[0])
    lat = clim->oh_lat[0];
  else if (lat > clim->oh_lat[clim->oh_nlat - 1])
    lat = clim->oh_lat[clim->oh_nlat - 1];

  /* Get indices... */
  int isec = locate_irr(clim->oh_time, clim->oh_ntime, sec);
  int ilat = locate_reg(clim->oh_lat, clim->oh_nlat, lat);
  int ip = locate_irr(clim->oh_p, clim->oh_np, p);

  /* Interpolate OH climatology... */
  double aux00 = LIN(clim->oh_p[ip],
		     clim->oh[isec][ip][ilat],
		     clim->oh_p[ip + 1],
		     clim->oh[isec][ip + 1][ilat], p);
  double aux01 = LIN(clim->oh_p[ip],
		     clim->oh[isec][ip][ilat + 1],
		     clim->oh_p[ip + 1],
		     clim->oh[isec][ip + 1][ilat + 1], p);
  double aux10 = LIN(clim->oh_p[ip],
		     clim->oh[isec + 1][ip][ilat],
		     clim->oh_p[ip + 1],
		     clim->oh[isec + 1][ip + 1][ilat], p);
  double aux11 = LIN(clim->oh_p[ip],
		     clim->oh[isec + 1][ip][ilat + 1],
		     clim->oh_p[ip + 1],
		     clim->oh[isec + 1][ip + 1][ilat + 1], p);
  aux00 = LIN(clim->oh_lat[ilat], aux00, clim->oh_lat[ilat + 1], aux01, lat);
  aux11 = LIN(clim->oh_lat[ilat], aux10, clim->oh_lat[ilat + 1], aux11, lat);
  aux00 =
    LIN(clim->oh_time[isec], aux00, clim->oh_time[isec + 1], aux11, sec);

  return GSL_MAX(aux00, 0.0);
}

/*****************************************************************************/

double clim_oh_diurnal(
  ctl_t * ctl,
  clim_t * clim,
  double t,
  double p,
  double lon,
  double lat) {

  double oh = clim_oh(clim, t, lat, p), sza2 = sza(t, lon, lat);

  if (sza2 <= M_PI / 2. * 89. / 90.)
    return oh * exp(-ctl->oh_chem_beta / cos(sza2));
  else
    return oh * exp(-ctl->oh_chem_beta / cos(M_PI / 2. * 89. / 90.));
}

/*****************************************************************************/

void clim_oh_init(
  ctl_t * ctl,
  clim_t * clim) {

  int nt, ncid, varid;

  double *help, ohmin = 1e99, ohmax = -1e99;

  /* Write info... */
  LOG(1, "Read OH data: %s", ctl->clim_oh_filename);

  /* Open netCDF file... */
  if (nc_open(ctl->clim_oh_filename, NC_NOWRITE, &ncid) != NC_NOERR) {
    WARN("OH climatology data are missing!");
    return;
  }

  /* Read pressure data... */
  NC_INQ_DIM("press", &clim->oh_np, 2, CP);
  NC_GET_DOUBLE("press", clim->oh_p, 1);

  /* Check ordering of pressure data... */
  if (clim->oh_p[0] < clim->oh_p[1])
    ERRMSG("Pressure data are not descending!");

  /* Read latitudes... */
  NC_INQ_DIM("lat", &clim->oh_nlat, 2, CY);
  NC_GET_DOUBLE("lat", clim->oh_lat, 1);

  /* Check ordering of latitudes... */
  if (clim->oh_lat[0] > clim->oh_lat[1])
    ERRMSG("Latitude data are not ascending!");

  /* Set time data for monthly means... */
  clim->oh_ntime = 12;
  clim->oh_time[0] = 1209600.00;
  clim->oh_time[1] = 3888000.00;
  clim->oh_time[2] = 6393600.00;
  clim->oh_time[3] = 9072000.00;
  clim->oh_time[4] = 11664000.00;
  clim->oh_time[5] = 14342400.00;
  clim->oh_time[6] = 16934400.00;
  clim->oh_time[7] = 19612800.00;
  clim->oh_time[8] = 22291200.00;
  clim->oh_time[9] = 24883200.00;
  clim->oh_time[10] = 27561600.00;
  clim->oh_time[11] = 30153600.00;

  /* Check number of timesteps... */
  NC_INQ_DIM("time", &nt, 12, 12);

  /* Read OH data... */
  ALLOC(help, double,
	clim->oh_nlat * clim->oh_np * clim->oh_ntime);
  NC_GET_DOUBLE("OH", help, 1);
  for (int it = 0; it < clim->oh_ntime; it++)
    for (int iz = 0; iz < clim->oh_np; iz++)
      for (int iy = 0; iy < clim->oh_nlat; iy++) {
	clim->oh[it][iz][iy] =
	  help[ARRAY_3D(it, iz, clim->oh_np, iy, clim->oh_nlat)]
	  / clim_oh_init_help(ctl->oh_chem_beta, clim->oh_time[it],
			      clim->oh_lat[iy]);
	ohmin = GSL_MIN(ohmin, clim->oh[it][iz][iy]);
	ohmax = GSL_MAX(ohmax, clim->oh[it][iz][iy]);
      }
  free(help);

  /* Close netCDF file... */
  NC(nc_close(ncid));

  /* Write info... */
  LOG(2, "Number of time steps: %d", clim->oh_ntime);
  LOG(2, "Time steps: %.2f, %.2f ... %.2f s",
      clim->oh_time[0], clim->oh_time[1], clim->oh_time[clim->oh_ntime - 1]);
  LOG(2, "Number of pressure levels: %d", clim->oh_np);
  LOG(2, "Altitude levels: %g, %g ... %g km",
      Z(clim->oh_p[0]), Z(clim->oh_p[1]), Z(clim->oh_p[clim->oh_np - 1]));
  LOG(2, "Pressure levels: %g, %g ... %g hPa",
      clim->oh_p[0], clim->oh_p[1], clim->oh_p[clim->oh_np - 1]);
  LOG(2, "Number of latitudes: %d", clim->oh_nlat);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      clim->oh_lat[0], clim->oh_lat[1], clim->oh_lat[clim->oh_nlat - 1]);
  LOG(2, "OH concentration range: %g ... %g molec/cm^3", ohmin, ohmax);
}

/*****************************************************************************/

double clim_oh_init_help(
  double beta,
  double time,
  double lat) {

  double aux, lon, sum = 0;

  int n = 0;

  /* Integrate day/night correction factor over longitude... */
  for (lon = -180; lon < 180; lon += 1) {
    aux = sza(time, lon, lat);
    if (aux <= M_PI / 2. * 85. / 90.)
      sum += exp(-beta / cos(aux));
    else
      sum += exp(-beta / cos(M_PI / 2. * 85. / 90.));
    n++;
  }
  return sum / (double) n;
}

/*****************************************************************************/

double clim_h2o2(
  clim_t * clim,
  double t,
  double lat,
  double p) {

  /* Get seconds since begin of year... */
  double sec = FMOD(t, 365.25 * 86400.);
  while (sec < 0)
    sec += 365.25 * 86400.;

  /* Check pressure... */
  if (p < clim->h2o2_p[clim->h2o2_np - 1])
    p = clim->h2o2_p[clim->h2o2_np - 1];
  else if (p > clim->h2o2_p[0])
    p = clim->h2o2_p[0];

  /* Check latitude... */
  if (lat < clim->h2o2_lat[0])
    lat = clim->h2o2_lat[0];
  else if (lat > clim->h2o2_lat[clim->h2o2_nlat - 1])
    lat = clim->h2o2_lat[clim->h2o2_nlat - 1];

  /* Get indices... */
  int isec = locate_irr(clim->h2o2_time, clim->h2o2_ntime, sec);
  int ilat = locate_reg(clim->h2o2_lat, clim->h2o2_nlat, lat);
  int ip = locate_irr(clim->h2o2_p, clim->h2o2_np, p);

  /* Interpolate H2O2 climatology... */
  double aux00 = LIN(clim->h2o2_p[ip],
		     clim->h2o2[isec][ip][ilat],
		     clim->h2o2_p[ip + 1],
		     clim->h2o2[isec][ip + 1][ilat], p);
  double aux01 = LIN(clim->h2o2_p[ip],
		     clim->h2o2[isec][ip][ilat + 1],
		     clim->h2o2_p[ip + 1],
		     clim->h2o2[isec][ip + 1][ilat + 1], p);
  double aux10 = LIN(clim->h2o2_p[ip],
		     clim->h2o2[isec + 1][ip][ilat],
		     clim->h2o2_p[ip + 1],
		     clim->h2o2[isec + 1][ip + 1][ilat], p);
  double aux11 = LIN(clim->h2o2_p[ip],
		     clim->h2o2[isec + 1][ip][ilat + 1],
		     clim->h2o2_p[ip + 1],
		     clim->h2o2[isec + 1][ip + 1][ilat + 1], p);
  aux00 =
    LIN(clim->h2o2_lat[ilat], aux00, clim->h2o2_lat[ilat + 1], aux01, lat);
  aux11 =
    LIN(clim->h2o2_lat[ilat], aux10, clim->h2o2_lat[ilat + 1], aux11, lat);
  aux00 =
    LIN(clim->h2o2_time[isec], aux00, clim->h2o2_time[isec + 1], aux11, sec);

  return GSL_MAX(aux00, 0.0);
}

/*****************************************************************************/

void clim_h2o2_init(
  ctl_t * ctl,
  clim_t * clim) {

  int ncid, varid, it, iy, iz, nt;

  double *help, h2o2min = 1e99, h2o2max = -1e99;

  /* Write info... */
  LOG(1, "Read H2O2 data: %s", ctl->clim_h2o2_filename);

  /* Open netCDF file... */
  if (nc_open(ctl->clim_h2o2_filename, NC_NOWRITE, &ncid) != NC_NOERR) {
    WARN("H2O2 climatology data are missing!");
    return;
  }

  /* Read pressure data... */
  NC_INQ_DIM("press", &clim->h2o2_np, 2, CP);
  NC_GET_DOUBLE("press", clim->h2o2_p, 1);

  /* Check ordering of pressure data... */
  if (clim->h2o2_p[0] < clim->h2o2_p[1])
    ERRMSG("Pressure data are not descending!");

  /* Read latitudes... */
  NC_INQ_DIM("lat", &clim->h2o2_nlat, 2, CY);
  NC_GET_DOUBLE("lat", clim->h2o2_lat, 1);

  /* Check ordering of latitude data... */
  if (clim->h2o2_lat[0] > clim->h2o2_lat[1])
    ERRMSG("Latitude data are not ascending!");

  /* Set time data (for monthly means)... */
  clim->h2o2_ntime = 12;
  clim->h2o2_time[0] = 1209600.00;
  clim->h2o2_time[1] = 3888000.00;
  clim->h2o2_time[2] = 6393600.00;
  clim->h2o2_time[3] = 9072000.00;
  clim->h2o2_time[4] = 11664000.00;
  clim->h2o2_time[5] = 14342400.00;
  clim->h2o2_time[6] = 16934400.00;
  clim->h2o2_time[7] = 19612800.00;
  clim->h2o2_time[8] = 22291200.00;
  clim->h2o2_time[9] = 24883200.00;
  clim->h2o2_time[10] = 27561600.00;
  clim->h2o2_time[11] = 30153600.00;

  /* Check number of timesteps... */
  NC_INQ_DIM("time", &nt, 12, 12);

  /* Read data... */
  ALLOC(help, double,
	clim->h2o2_nlat * clim->h2o2_np * clim->h2o2_ntime);
  NC_GET_DOUBLE("h2o2", help, 1);
  for (it = 0; it < clim->h2o2_ntime; it++)
    for (iz = 0; iz < clim->h2o2_np; iz++)
      for (iy = 0; iy < clim->h2o2_nlat; iy++) {
	clim->h2o2[it][iz][iy] =
	  help[ARRAY_3D(it, iz, clim->h2o2_np, iy, clim->h2o2_nlat)];
	h2o2min = GSL_MIN(h2o2min, clim->h2o2[it][iz][iy]);
	h2o2max = GSL_MAX(h2o2max, clim->h2o2[it][iz][iy]);
      }
  free(help);

  /* Close netCDF file... */
  NC(nc_close(ncid));

  /* Write info... */
  LOG(2, "Number of time steps: %d", clim->h2o2_ntime);
  LOG(2, "Time steps: %.2f, %.2f ... %.2f s",
      clim->h2o2_time[0], clim->h2o2_time[1],
      clim->h2o2_time[clim->h2o2_ntime - 1]);
  LOG(2, "Number of pressure levels: %d", clim->h2o2_np);
  LOG(2, "Altitude levels: %g, %g ... %g km",
      Z(clim->h2o2_p[0]), Z(clim->h2o2_p[1]),
      Z(clim->h2o2_p[clim->h2o2_np - 1]));
  LOG(2, "Pressure levels: %g, %g ... %g hPa", clim->h2o2_p[0],
      clim->h2o2_p[1], clim->h2o2_p[clim->h2o2_np - 1]);
  LOG(2, "Number of latitudes: %d", clim->h2o2_nlat);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      clim->h2o2_lat[0], clim->h2o2_lat[1],
      clim->h2o2_lat[clim->h2o2_nlat - 1]);
  LOG(2, "H2O2 concentration range: %g ... %g molec/cm^3", h2o2min, h2o2max);
}

/*****************************************************************************/

double clim_ho2(
  clim_t * clim,
  double t,
  double lat,
  double p) {

  /* Get seconds since begin of year... */
  double sec = FMOD(t, 365.25 * 86400.);
  while (sec < 0)
    sec += 365.25 * 86400.;

  /* Check pressure... */
  if (p < clim->ho2_p[clim->ho2_np - 1])
    p = clim->ho2_p[clim->ho2_np - 1];
  else if (p > clim->ho2_p[0])
    p = clim->ho2_p[0];

  /* Check latitude... */
  if (lat < clim->ho2_lat[0])
    lat = clim->ho2_lat[0];
  else if (lat > clim->ho2_lat[clim->ho2_nlat - 1])
    lat = clim->ho2_lat[clim->ho2_nlat - 1];

  /* Get indices... */
  int isec = locate_irr(clim->ho2_time, clim->ho2_ntime, sec);
  int ilat = locate_reg(clim->ho2_lat, clim->ho2_nlat, lat);
  int ip = locate_irr(clim->ho2_p, clim->ho2_np, p);

  /* Interpolate ho2 climatology... */
  double aux00 = LIN(clim->ho2_p[ip],
		     clim->ho2[isec][ip][ilat],
		     clim->ho2_p[ip + 1],
		     clim->ho2[isec][ip + 1][ilat], p);
  double aux01 = LIN(clim->ho2_p[ip],
		     clim->ho2[isec][ip][ilat + 1],
		     clim->ho2_p[ip + 1],
		     clim->ho2[isec][ip + 1][ilat + 1], p);
  double aux10 = LIN(clim->ho2_p[ip],
		     clim->ho2[isec + 1][ip][ilat],
		     clim->ho2_p[ip + 1],
		     clim->ho2[isec + 1][ip + 1][ilat], p);
  double aux11 = LIN(clim->ho2_p[ip],
		     clim->ho2[isec + 1][ip][ilat + 1],
		     clim->ho2_p[ip + 1],
		     clim->ho2[isec + 1][ip + 1][ilat + 1], p);
  aux00 =
    LIN(clim->ho2_lat[ilat], aux00, clim->ho2_lat[ilat + 1], aux01, lat);
  aux11 =
    LIN(clim->ho2_lat[ilat], aux10, clim->ho2_lat[ilat + 1], aux11, lat);
  aux00 =
    LIN(clim->ho2_time[isec], aux00, clim->ho2_time[isec + 1], aux11, sec);

  return GSL_MAX(aux00, 0.0);
}

/*****************************************************************************/

void clim_ho2_init(
  ctl_t * ctl,
  clim_t * clim) {

  int ncid, varid, it, iy, iz, nt;

  double *help, ho2min = 1e99, ho2max = -1e99;

  /* Write info... */
  LOG(1, "Read HO2 data: %s", ctl->clim_ho2_filename);

  /* Open netCDF file... */
  if (nc_open(ctl->clim_ho2_filename, NC_NOWRITE, &ncid) != NC_NOERR) {
    WARN("HO2 climatology data are missing!");
    return;
  }

  /* Read pressure data... */
  NC_INQ_DIM("press", &clim->ho2_np, 2, CP);
  NC_GET_DOUBLE("press", clim->ho2_p, 1);

  /* Check ordering of pressure data... */
  if (clim->ho2_p[0] < clim->ho2_p[1])
    ERRMSG("Pressure data are not descending!");

  /* Read latitudes... */
  NC_INQ_DIM("lat", &clim->ho2_nlat, 2, CY);
  NC_GET_DOUBLE("lat", clim->ho2_lat, 1);

  /* Check ordering of latitude data... */
  if (clim->ho2_lat[0] > clim->ho2_lat[1])
    ERRMSG("Latitude data are not ascending!");

  /* Set time data (for monthly means)... */
  clim->ho2_ntime = 12;
  clim->ho2_time[0] = 1209600.00;
  clim->ho2_time[1] = 3888000.00;
  clim->ho2_time[2] = 6393600.00;
  clim->ho2_time[3] = 9072000.00;
  clim->ho2_time[4] = 11664000.00;
  clim->ho2_time[5] = 14342400.00;
  clim->ho2_time[6] = 16934400.00;
  clim->ho2_time[7] = 19612800.00;
  clim->ho2_time[8] = 22291200.00;
  clim->ho2_time[9] = 24883200.00;
  clim->ho2_time[10] = 27561600.00;
  clim->ho2_time[11] = 30153600.00;

  /* Check number of timesteps... */
  NC_INQ_DIM("time", &nt, 12, 12);

  /* Read data... */
  ALLOC(help, double,
	clim->ho2_nlat * clim->ho2_np * clim->ho2_ntime);
  NC_GET_DOUBLE("HO2", help, 1);
  for (it = 0; it < clim->ho2_ntime; it++)
    for (iz = 0; iz < clim->ho2_np; iz++)
      for (iy = 0; iy < clim->ho2_nlat; iy++) {
	clim->ho2[it][iz][iy] =
	  help[ARRAY_3D(it, iz, clim->ho2_np, iy, clim->ho2_nlat)];
	ho2min = GSL_MIN(ho2min, clim->ho2[it][iz][iy]);
	ho2max = GSL_MAX(ho2max, clim->ho2[it][iz][iy]);
      }
  free(help);

  /* Close netCDF file... */
  NC(nc_close(ncid));

  /* Write info... */
  LOG(2, "Number of time steps: %d", clim->ho2_ntime);
  LOG(2, "Time steps: %.2f, %.2f ... %.2f s",
      clim->ho2_time[0], clim->ho2_time[1],
      clim->ho2_time[clim->ho2_ntime - 1]);
  LOG(2, "Number of pressure levels: %d", clim->ho2_np);
  LOG(2, "Altitude levels: %g, %g ... %g km",
      Z(clim->ho2_p[0]), Z(clim->ho2_p[1]), Z(clim->ho2_p[clim->ho2_np - 1]));
  LOG(2, "Pressure levels: %g, %g ... %g hPa", clim->ho2_p[0],
      clim->ho2_p[1], clim->ho2_p[clim->ho2_np - 1]);
  LOG(2, "Number of latitudes: %d", clim->ho2_nlat);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      clim->ho2_lat[0], clim->ho2_lat[1], clim->ho2_lat[clim->ho2_nlat - 1]);
  LOG(2, "HO2 concentration range: %g ... %g molec/cm^3", ho2min, ho2max);
}

/*****************************************************************************/

double clim_tropo(
  clim_t * clim,
  double t,
  double lat) {

  /* Get seconds since begin of year... */
  double sec = FMOD(t, 365.25 * 86400.);
  while (sec < 0)
    sec += 365.25 * 86400.;

  /* Get indices... */
  int isec = locate_irr(clim->tropo_time, clim->tropo_ntime, sec);
  int ilat = locate_reg(clim->tropo_lat, clim->tropo_nlat, lat);

  /* Interpolate tropopause data (NCEP/NCAR Reanalysis 1)... */
  double p0 = LIN(clim->tropo_lat[ilat],
		  clim->tropo[isec][ilat],
		  clim->tropo_lat[ilat + 1],
		  clim->tropo[isec][ilat + 1], lat);
  double p1 = LIN(clim->tropo_lat[ilat],
		  clim->tropo[isec + 1][ilat],
		  clim->tropo_lat[ilat + 1],
		  clim->tropo[isec + 1][ilat + 1], lat);
  return LIN(clim->tropo_time[isec], p0, clim->tropo_time[isec + 1], p1, sec);
}

/*****************************************************************************/

void clim_tropo_init(
  clim_t * clim) {

  /* Write info... */
  LOG(1, "Initialize tropopause data...");

  clim->tropo_ntime = 12;
  double tropo_time[12] = {
    1209600.00, 3888000.00, 6393600.00,
    9072000.00, 11664000.00, 14342400.00,
    16934400.00, 19612800.00, 22291200.00,
    24883200.00, 27561600.00, 30153600.00
  };
  memcpy(clim->tropo_time, tropo_time, sizeof(clim->tropo_time));

  clim->tropo_nlat = 73;
  double tropo_lat[73] = {
    -90, -87.5, -85, -82.5, -80, -77.5, -75, -72.5, -70, -67.5,
    -65, -62.5, -60, -57.5, -55, -52.5, -50, -47.5, -45, -42.5,
    -40, -37.5, -35, -32.5, -30, -27.5, -25, -22.5, -20, -17.5,
    -15, -12.5, -10, -7.5, -5, -2.5, 0, 2.5, 5, 7.5, 10, 12.5,
    15, 17.5, 20, 22.5, 25, 27.5, 30, 32.5, 35, 37.5, 40, 42.5,
    45, 47.5, 50, 52.5, 55, 57.5, 60, 62.5, 65, 67.5, 70, 72.5,
    75, 77.5, 80, 82.5, 85, 87.5, 90
  };
  memcpy(clim->tropo_lat, tropo_lat, sizeof(clim->tropo_lat));

  double tropo[12][73] = {
    {324.1, 325.6, 325, 324.3, 322.5, 319.7, 314, 307.2, 301.8, 299.6,
     297.1, 292.2, 285.6, 276.1, 264, 248.9, 231.9, 213.5, 194.4,
     175.3, 157, 140.4, 126.7, 116.3, 109.5, 105.4, 103, 101.4, 100.4,
     99.69, 99.19, 98.84, 98.56, 98.39, 98.39, 98.42, 98.44, 98.54,
     98.68, 98.81, 98.89, 98.96, 99.12, 99.65, 101.4, 105.4, 113.5, 128,
     152.1, 184.7, 214, 234.1, 247.3, 255.8, 262.6, 267.7, 271.7, 275,
     277.2, 279, 280.1, 280.4, 280.6, 280.1, 279.3, 278.3, 276.8, 275.8,
     275.3, 275.6, 275.4, 274.1, 273.5},
    {337.3, 338.7, 337.8, 336.4, 333, 328.8, 321.1, 312.6, 306.6, 303.7,
     300.2, 293.8, 285.4, 273.8, 259.6, 242.7, 224.4, 205.2, 186, 167.5,
     150.3, 135, 122.8, 113.9, 108.2, 104.7, 102.5, 101.1, 100.2, 99.42,
     98.88, 98.52, 98.25, 98.09, 98.07, 98.1, 98.12, 98.2, 98.25, 98.27,
     98.26, 98.27, 98.36, 98.79, 100.2, 104.2, 113.7, 131.2, 159.5, 193,
     220.4, 238.1, 250.2, 258.1, 264.7, 269.7, 273.7, 277.3, 280.2, 282.8,
     284.9, 286.5, 288.1, 288.8, 289, 288.5, 287.2, 286.3, 286.1, 287.2,
     287.5, 286.2, 285.8},
    {335, 336, 335.7, 335.1, 332.3, 328.1, 320.6, 311.8, 305.1, 301.9,
     297.6, 290, 280.4, 268.3, 254.6, 239.6, 223.9, 207.9, 192.2, 176.9,
     161.7, 146.4, 132.2, 120.6, 112.3, 107.2, 104.3, 102.4, 101.3,
     100.4, 99.86, 99.47, 99.16, 98.97, 98.94, 98.97, 99, 99.09, 99.2,
     99.31, 99.35, 99.41, 99.51, 99.86, 101.1, 104.9, 114.3, 131, 156.8,
     186.3, 209.3, 224.6, 236.8, 246.3, 254.9, 262.3, 268.8, 274.8,
     279.9, 284.6, 288.6, 291.6, 294.9, 297.5, 299.8, 301.8, 303.1,
     304.3, 304.9, 306, 306.6, 306.2, 306},
    {306.2, 306.7, 305.7, 307.1, 307.3, 306.4, 301.8, 296.2, 292.4,
     290.3, 287.1, 280.9, 273.4, 264.3, 254.1, 242.8, 231, 219, 207.2,
     195.5, 183.3, 169.7, 154.7, 138.7, 124.1, 113.6, 107.8, 104.7,
     102.8, 101.7, 100.9, 100.4, 100, 99.79, 99.7, 99.66, 99.68, 99.79,
     99.94, 100.2, 100.5, 100.9, 101.4, 102.1, 103.4, 107, 115.2, 129.1,
     148.7, 171, 190.8, 205.6, 218.4, 229.4, 239.6, 248.6, 256.5,
     263.7, 270.3, 276.6, 282.6, 288.1, 294.5, 300.4, 306.3, 311.4,
     315.1, 318.3, 320.3, 322.2, 322.8, 321.5, 321.1},
    {266.5, 264.9, 260.8, 261, 262, 263, 261.3, 259.7, 259.2, 259.8,
     260.1, 258.6, 256.7, 253.6, 249.5, 243.9, 237.4, 230, 222.1, 213.9,
     205, 194.4, 180.4, 161.8, 140.7, 122.9, 112.1, 106.7, 104.1, 102.7,
     101.8, 101.4, 101.1, 101, 101, 101, 101.1, 101.2, 101.5, 101.9,
     102.4, 103, 103.8, 104.9, 106.8, 110.1, 115.6, 124, 135.2, 148.9,
     165.2, 181.3, 198, 211.8, 223.5, 233.8, 242.9, 251.5, 259, 266.2,
     273.1, 279.2, 286.2, 292.8, 299.6, 306, 311.1, 315.5, 318.8, 322.6,
     325.3, 325.8, 325.8},
    {220.1, 218.1, 210.8, 207.2, 207.6, 210.5, 211.4, 213.5, 217.3,
     222.4, 227.9, 232.8, 237.4, 240.8, 242.8, 243, 241.5, 238.6, 234.2,
     228.5, 221, 210.7, 195.1, 172.9, 147.8, 127.6, 115.6, 109.9, 107.1,
     105.7, 105, 104.8, 104.8, 104.9, 105, 105.1, 105.3, 105.5, 105.8,
     106.4, 107, 107.6, 108.1, 108.8, 110, 111.8, 114.2, 117.4, 121.6,
     127.9, 137.3, 151.2, 169.5, 189, 205.8, 218.9, 229.1, 237.8, 245,
     251.5, 257.1, 262.3, 268.2, 274, 280.4, 286.7, 292.4, 297.9, 302.9,
     308.5, 312.2, 313.1, 313.3},
    {187.4, 184.5, 173.3, 166.1, 165.4, 167.8, 169.6, 173.6, 179.6,
     187.9, 198.9, 210, 220.5, 229.2, 235.7, 239.9, 241.8, 241.6, 239.6,
     235.8, 229.4, 218.6, 200.9, 175.9, 149.4, 129.4, 118.3, 113.1,
     110.8, 109.7, 109.3, 109.4, 109.7, 110, 110.2, 110.4, 110.5, 110.7,
     111, 111.4, 111.8, 112.1, 112.3, 112.7, 113.2, 113.9, 115, 116.4,
     117.9, 120.4, 124.1, 130.9, 142.2, 159.6, 179.6, 198.5, 212.9,
     224.2, 232.7, 239.1, 243.8, 247.7, 252.4, 257.3, 263.2, 269.5,
     275.4, 281.1, 286.3, 292, 296.3, 298.2, 298.8},
    {166, 166.4, 155.7, 148.3, 147.1, 149, 152.1, 157, 163.6, 172.4,
     185.3, 199.2, 212.6, 224, 233.2, 239.6, 243.3, 244.6, 243.6, 240.3,
     233.9, 222.6, 203.7, 177, 149.5, 129.7, 119, 114, 111.7, 110.7,
     110.3, 110.3, 110.6, 110.9, 111.1, 111.3, 111.5, 111.6, 111.9,
     112.2, 112.5, 112.6, 112.8, 113, 113.4, 114, 115.1, 116.5, 118.3,
     120.9, 124.4, 130.2, 139.4, 154.6, 173.8, 193.1, 208.1, 220.4,
     230.1, 238.2, 244.7, 249.5, 254.5, 259.3, 264.5, 269.4, 273.7,
     278.2, 282.6, 287.4, 290.9, 292.5, 293},
    {171.9, 172.8, 166.2, 162.3, 161.4, 162.5, 165.2, 169.6, 175.3,
     183.1, 193.8, 205.9, 218.3, 229.6, 238.5, 244.3, 246.9, 246.7,
     243.8, 238.4, 230.2, 217.9, 199.6, 174.9, 148.9, 129.8, 119.5,
     114.8, 112.3, 110.9, 110.3, 110.1, 110.2, 110.3, 110.4, 110.5,
     110.6, 110.8, 111, 111.4, 111.8, 112, 112.2, 112.4, 112.9, 113.6,
     114.7, 116.3, 118.4, 121.9, 127.1, 136.1, 149.8, 168.4, 186.9,
     203.3, 217, 229.1, 238.7, 247, 254, 259.3, 264.3, 268.3, 272.5,
     276.6, 280.4, 284.4, 288.4, 293.3, 297.2, 298.7, 299.1},
    {191.6, 192.2, 189, 188.1, 190.2, 193.7, 197.8, 202.9, 208.5,
     215.6, 224.2, 233.1, 241.2, 247.3, 250.8, 251.3, 248.9, 244.2,
     237.3, 228.4, 217.2, 202.9, 184.5, 162.5, 140.7, 124.8, 116.2,
     111.8, 109.4, 107.9, 107, 106.7, 106.6, 106.6, 106.7, 106.7,
     106.8, 107, 107.4, 108, 108.7, 109.3, 109.8, 110.4, 111.2,
     112.4, 114.2, 116.9, 121.1, 127.9, 139.3, 155.2, 173.6, 190.7,
     206.1, 220.1, 232.3, 243, 251.8, 259.2, 265.7, 270.6, 275.3,
     279.3, 283.3, 286.9, 289.7, 292.8, 296.1, 300.5, 303.9, 304.8,
     305.1},
    {241.5, 239.6, 236.8, 237.4, 239.4, 242.3, 244.2, 246.4, 249.2,
     253.6, 258.6, 262.7, 264.8, 264.2, 260.6, 254.1, 245.5, 235.3,
     223.9, 211.7, 198.3, 183.1, 165.6, 147.1, 130.5, 118.7, 111.9,
     108.1, 105.8, 104.3, 103.4, 102.8, 102.5, 102.4, 102.5, 102.5,
     102.5, 102.7, 103.1, 103.8, 104.6, 105.4, 106.1, 107, 108.2,
     109.9, 112.8, 117.5, 126, 140.4, 161, 181.9, 201.2, 216.8, 230.4,
     241.8, 251.4, 259.9, 266.9, 272.8, 277.4, 280.4, 282.9, 284.6,
     286.1, 287.4, 288.3, 289.5, 290.9, 294.2, 296.9, 297.5, 297.6},
    {301.2, 300.3, 296.6, 295.4, 295, 294.3, 291.2, 287.4, 284.9, 284.7,
     284.1, 281.5, 277.1, 270.4, 261.7, 250.6, 237.6, 223.1, 207.9, 192,
     175.8, 158.8, 142.1, 127.6, 116.8, 109.9, 106, 103.6, 102.1, 101.1,
     100.4, 99.96, 99.6, 99.37, 99.32, 99.32, 99.31, 99.46, 99.77, 100.2,
     100.7, 101.3, 101.8, 102.7, 104.1, 106.8, 111.9, 121, 136.7, 160,
     186.9, 209.9, 228.1, 241.2, 251.5, 259.5, 265.7, 270.9, 274.8, 278,
     280.3, 281.8, 283, 283.3, 283.7, 283.8, 283, 282.2, 281.2, 281.4,
     281.7, 281.1, 281.2}
  };
  memcpy(clim->tropo, tropo, sizeof(clim->tropo));

  /* Get range... */
  double tropomin = 1e99, tropomax = -1e99;
  for (int it = 0; it < clim->tropo_ntime; it++)
    for (int iy = 0; iy < clim->tropo_nlat; iy++) {
      tropomin = GSL_MIN(tropomin, clim->tropo[it][iy]);
      tropomax = GSL_MAX(tropomax, clim->tropo[it][iy]);
    }

  /* Write info... */
  LOG(2, "Number of time steps: %d", clim->tropo_ntime);
  LOG(2, "Time steps: %.2f, %.2f ... %.2f s",
      clim->tropo_time[0], clim->tropo_time[1],
      clim->tropo_time[clim->tropo_ntime - 1]);
  LOG(2, "Number of latitudes: %d", clim->tropo_nlat);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      clim->tropo_lat[0], clim->tropo_lat[1],
      clim->tropo_lat[clim->tropo_nlat - 1]);
  LOG(2, "Tropopause altitude range: %g ... %g hPa", Z(tropomax),
      Z(tropomin));
  LOG(2, "Tropopause pressure range: %g ... %g hPa", tropomin, tropomax);
}

/*****************************************************************************/

void compress_pack(
  char *varname,
  float *array,
  size_t nxy,
  size_t nz,
  int decompress,
  FILE * inout) {

  double min[EP], max[EP], off[EP], scl[EP];

  unsigned short *sarray;

  /* Allocate... */
  ALLOC(sarray, unsigned short,
	nxy * nz);

  /* Read compressed stream and decompress array... */
  if (decompress) {

    /* Write info... */
    LOG(2, "Read 3-D variable: %s (pack, RATIO= %g %%)",
	varname, 100. * sizeof(unsigned short) / sizeof(float));

    /* Read data... */
    FREAD(&scl, double,
	  nz,
	  inout);
    FREAD(&off, double,
	  nz,
	  inout);
    FREAD(sarray, unsigned short,
	  nxy * nz,
	  inout);

    /* Convert to float... */
#pragma omp parallel for default(shared)
    for (size_t ixy = 0; ixy < nxy; ixy++)
      for (size_t iz = 0; iz < nz; iz++)
	array[ixy * nz + iz]
	  = (float) (sarray[ixy * nz + iz] * scl[iz] + off[iz]);
  }

  /* Compress array and output compressed stream... */
  else {

    /* Write info... */
    LOG(2, "Write 3-D variable: %s (pack, RATIO= %g %%)",
	varname, 100. * sizeof(unsigned short) / sizeof(float));

    /* Get range... */
    for (size_t iz = 0; iz < nz; iz++) {
      min[iz] = array[iz];
      max[iz] = array[iz];
    }
    for (size_t ixy = 1; ixy < nxy; ixy++)
      for (size_t iz = 0; iz < nz; iz++) {
	if (array[ixy * nz + iz] < min[iz])
	  min[iz] = array[ixy * nz + iz];
	if (array[ixy * nz + iz] > max[iz])
	  max[iz] = array[ixy * nz + iz];
      }

    /* Get offset and scaling factor... */
    for (size_t iz = 0; iz < nz; iz++) {
      scl[iz] = (max[iz] - min[iz]) / 65533.;
      off[iz] = min[iz];
    }

    /* Convert to short... */
#pragma omp parallel for default(shared)
    for (size_t ixy = 0; ixy < nxy; ixy++)
      for (size_t iz = 0; iz < nz; iz++)
	if (scl[iz] != 0)
	  sarray[ixy * nz + iz] = (unsigned short)
	    ((array[ixy * nz + iz] - off[iz]) / scl[iz] + .5);
	else
	  sarray[ixy * nz + iz] = 0;

    /* Write data... */
    FWRITE(&scl, double,
	   nz,
	   inout);
    FWRITE(&off, double,
	   nz,
	   inout);
    FWRITE(sarray, unsigned short,
	   nxy * nz,
	   inout);
  }

  /* Free... */
  free(sarray);
}

/*****************************************************************************/

#ifdef ZFP
void compress_zfp(
  char *varname,
  float *array,
  int nx,
  int ny,
  int nz,
  int precision,
  double tolerance,
  int decompress,
  FILE * inout) {

  zfp_type type;		/* array scalar type */
  zfp_field *field;		/* array meta data */
  zfp_stream *zfp;		/* compressed stream */
  void *buffer;			/* storage for compressed stream */
  size_t bufsize;		/* byte size of compressed buffer */
  bitstream *stream;		/* bit stream to write to or read from */
  size_t zfpsize;		/* byte size of compressed stream */

  /* Allocate meta data for the 3D array a[nz][ny][nx]... */
  type = zfp_type_float;
  field = zfp_field_3d(array, type, (uint) nx, (uint) ny, (uint) nz);

  /* Allocate meta data for a compressed stream... */
  zfp = zfp_stream_open(NULL);

  /* Set compression mode... */
  int actual_prec = 0;
  double actual_tol = 0;
  if (precision > 0)
    actual_prec = (int) zfp_stream_set_precision(zfp, (uint) precision);
  else if (tolerance > 0)
    actual_tol = zfp_stream_set_accuracy(zfp, tolerance);
  else
    ERRMSG("Set precision or tolerance!");

  /* Allocate buffer for compressed data... */
  bufsize = zfp_stream_maximum_size(zfp, field);
  buffer = malloc(bufsize);

  /* Associate bit stream with allocated buffer... */
  stream = stream_open(buffer, bufsize);
  zfp_stream_set_bit_stream(zfp, stream);
  zfp_stream_rewind(zfp);

  /* Read compressed stream and decompress array... */
  if (decompress) {
    FREAD(&zfpsize, size_t,
	  1,
	  inout);
    if (fread(buffer, 1, zfpsize, inout) != zfpsize)
      ERRMSG("Error while reading zfp data!");
    if (!zfp_decompress(zfp, field)) {
      ERRMSG("Decompression failed!");
    }
    LOG(2, "Read 3-D variable: %s "
	"(zfp, PREC= %d, TOL= %g, RATIO= %g %%)",
	varname, actual_prec, actual_tol,
	(100. * (double) zfpsize) / (double) (nx * ny * nz));
  }

  /* Compress array and output compressed stream... */
  else {
    zfpsize = zfp_compress(zfp, field);
    if (!zfpsize) {
      ERRMSG("Compression failed!");
    } else {
      FWRITE(&zfpsize, size_t,
	     1,
	     inout);
      if (fwrite(buffer, 1, zfpsize, inout) != zfpsize)
	ERRMSG("Error while writing zfp data!");
    }
    LOG(2, "Write 3-D variable: %s "
	"(zfp, PREC= %d, TOL= %g, RATIO= %g %%)",
	varname, actual_prec, actual_tol,
	(100. * (double) zfpsize) / (double) (nx * ny * nz));
  }

  /* Free... */
  zfp_field_free(field);
  zfp_stream_close(zfp);
  stream_close(stream);
  free(buffer);
}
#endif

/*****************************************************************************/

#ifdef ZSTD
void compress_zstd(
  char *varname,
  float *array,
  size_t n,
  int decompress,
  FILE * inout) {

  /* Get buffer sizes... */
  size_t uncomprLen = n * sizeof(float);
  size_t comprLen = ZSTD_compressBound(uncomprLen);
  size_t compsize;

  /* Allocate... */
  char *compr = (char *) calloc((uint) comprLen, 1);
  char *uncompr = (char *) array;

  /* Read compressed stream and decompress array... */
  if (decompress) {
    FREAD(&comprLen, size_t,
	  1,
	  inout);
    if (fread(compr, 1, comprLen, inout) != comprLen)
      ERRMSG("Error while reading zstd data!");
    compsize = ZSTD_decompress(uncompr, uncomprLen, compr, comprLen);
    if (ZSTD_isError(compsize)) {
      ERRMSG("Decompression failed!");
    }
    LOG(2, "Read 3-D variable: %s (zstd, RATIO= %g %%)",
	varname, (100. * (double) comprLen) / (double) uncomprLen);
  }

  /* Compress array and output compressed stream... */
  else {
    compsize = ZSTD_compress(compr, comprLen, uncompr, uncomprLen, 0);
    if (ZSTD_isError(compsize)) {
      ERRMSG("Compression failed!");
    } else {
      FWRITE(&compsize, size_t,
	     1,
	     inout);
      if (fwrite(compr, 1, compsize, inout) != compsize)
	ERRMSG("Error while writing zstd data!");
    }
    LOG(2, "Write 3-D variable: %s (zstd, RATIO= %g %%)",
	varname, (100. * (double) compsize) / (double) uncomprLen);
  }

  /* Free... */
  free(compr);
}
#endif

/*****************************************************************************/

void day2doy(
  int year,
  int mon,
  int day,
  int *doy) {

  const int
    d0[12] = { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
    d0l[12] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336 };

  /* Get day of year... */
  if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0))
    *doy = d0l[mon - 1] + day - 1;
  else
    *doy = d0[mon - 1] + day - 1;
}

/*****************************************************************************/

void doy2day(
  int year,
  int doy,
  int *mon,
  int *day) {

  const int
    d0[12] = { 1, 32, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 },
    d0l[12] = { 1, 32, 61, 92, 122, 153, 183, 214, 245, 275, 306, 336 };

  int i;

  /* Get month and day... */
  if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) {
    for (i = 11; i > 0; i--)
      if (d0l[i] <= doy)
	break;
    *mon = i + 1;
    *day = doy - d0l[i] + 1;
  } else {
    for (i = 11; i > 0; i--)
      if (d0[i] <= doy)
	break;
    *mon = i + 1;
    *day = doy - d0[i] + 1;
  }
}

/*****************************************************************************/

void geo2cart(
  double z,
  double lon,
  double lat,
  double *x) {

  double radius = z + RE;
  x[0] = radius * cos(lat / 180. * M_PI) * cos(lon / 180. * M_PI);
  x[1] = radius * cos(lat / 180. * M_PI) * sin(lon / 180. * M_PI);
  x[2] = radius * sin(lat / 180. * M_PI);
}

/*****************************************************************************/

void get_met(
  ctl_t * ctl,
  clim_t * clim,
  double t,
  met_t ** met0,
  met_t ** met1) {

  static int init;

  met_t *mets;

  char cachefile[LEN], cmd[2 * LEN], filename[LEN];

  /* Set timer... */
  SELECT_TIMER("GET_MET", "INPUT", NVTX_READ);

  /* Init... */
  if (t == ctl->t_start || !init) {
    init = 1;

    /* Read meteo data... */
    get_met_help(ctl, t + (ctl->direction == -1 ? -1 : 0), -1,
		 ctl->metbase, ctl->dt_met, filename);
    if (!read_met(filename, ctl, clim, *met0))
      ERRMSG("Cannot open file!");

    get_met_help(ctl, t + (ctl->direction == 1 ? 1 : 0), 1,
		 ctl->metbase, ctl->dt_met, filename);
    if (!read_met(filename, ctl, clim, *met1))
      ERRMSG("Cannot open file!");

    /* Update GPU... */
#ifdef _OPENACC
    met_t *met0up = *met0;
    met_t *met1up = *met1;
#ifdef ASYNCIO
#pragma acc update device(met0up[:1],met1up[:1]) async(5)
#else
#pragma acc update device(met0up[:1],met1up[:1])
#endif
#endif

    /* Caching... */
    if (ctl->met_cache && t != ctl->t_stop) {
      get_met_help(ctl, t + 1.1 * ctl->dt_met * ctl->direction,
		   ctl->direction, ctl->metbase, ctl->dt_met, cachefile);
      sprintf(cmd, "cat %s > /dev/null &", cachefile);
      LOG(1, "Caching: %s", cachefile);
      if (system(cmd) != 0)
	WARN("Caching command failed!");
    }
  }

  /* Read new data for forward trajectories... */
  if (t > (*met1)->time) {

    /* Pointer swap... */
    mets = *met1;
    *met1 = *met0;
    *met0 = mets;

    /* Read new meteo data... */
    get_met_help(ctl, t, 1, ctl->metbase, ctl->dt_met, filename);
    if (!read_met(filename, ctl, clim, *met1))
      ERRMSG("Cannot open file!");

    /* Update GPU... */
#ifdef _OPENACC
    met_t *met1up = *met1;
#ifdef ASYNCIO
#pragma acc update device(met1up[:1]) async(5)
#else
#pragma acc update device(met1up[:1])
#endif
#endif

    /* Caching... */
    if (ctl->met_cache && t != ctl->t_stop) {
      get_met_help(ctl, t + ctl->dt_met, 1, ctl->metbase, ctl->dt_met,
		   cachefile);
      sprintf(cmd, "cat %s > /dev/null &", cachefile);
      LOG(1, "Caching: %s", cachefile);
      if (system(cmd) != 0)
	WARN("Caching command failed!");
    }
  }
  /* Read new data for backward trajectories... */
  if (t < (*met0)->time) {

    /* Pointer swap... */
    mets = *met1;
    *met1 = *met0;
    *met0 = mets;

    /* Read new meteo data... */
    get_met_help(ctl, t, -1, ctl->metbase, ctl->dt_met, filename);
    if (!read_met(filename, ctl, clim, *met0))
      ERRMSG("Cannot open file!");

    /* Update GPU... */
#ifdef _OPENACC
    met_t *met0up = *met0;
#ifdef ASYNCIO
#pragma acc update device(met0up[:1]) async(5)
#else
#pragma acc update device(met0up[:1])
#endif
#endif

    /* Caching... */
    if (ctl->met_cache && t != ctl->t_stop) {
      get_met_help(ctl, t - ctl->dt_met, -1, ctl->metbase, ctl->dt_met,
		   cachefile);
      sprintf(cmd, "cat %s > /dev/null &", cachefile);
      LOG(1, "Caching: %s", cachefile);
      if (system(cmd) != 0)
	WARN("Caching command failed!");
    }
  }
  /* Check that grids are consistent... */
  if ((*met0)->nx != 0 && (*met1)->nx != 0) {
    if ((*met0)->nx != (*met1)->nx
	|| (*met0)->ny != (*met1)->ny || (*met0)->np != (*met1)->np)
      ERRMSG("Meteo grid dimensions do not match!");
    for (int ix = 0; ix < (*met0)->nx; ix++)
      if (fabs((*met0)->lon[ix] - (*met1)->lon[ix]) > 0.001)
	ERRMSG("Meteo grid longitudes do not match!");
    for (int iy = 0; iy < (*met0)->ny; iy++)
      if (fabs((*met0)->lat[iy] - (*met1)->lat[iy]) > 0.001)
	ERRMSG("Meteo grid latitudes do not match!");
    for (int ip = 0; ip < (*met0)->np; ip++)
      if (fabs((*met0)->p[ip] - (*met1)->p[ip]) > 0.001)
	ERRMSG("Meteo grid pressure levels do not match!");
  }
}

/*****************************************************************************/

void get_met_help(
  ctl_t * ctl,
  double t,
  int direct,
  char *metbase,
  double dt_met,
  char *filename) {

  char repl[LEN];

  double t6, r;

  int year, mon, day, hour, min, sec;

  /* Round time to fixed intervals... */
  if (direct == -1)
    t6 = floor(t / dt_met) * dt_met;
  else
    t6 = ceil(t / dt_met) * dt_met;

  /* Decode time... */
  jsec2time(t6, &year, &mon, &day, &hour, &min, &sec, &r);

  /* Set filename of MPTRAC meteo files... */
  if (ctl->clams_met_data == 0) {
    if (ctl->met_type == 0)
      sprintf(filename, "%s_YYYY_MM_DD_HH.nc", metbase);
    else if (ctl->met_type == 1)
      sprintf(filename, "%s_YYYY_MM_DD_HH.bin", metbase);
    else if (ctl->met_type == 2)
      sprintf(filename, "%s_YYYY_MM_DD_HH.pck", metbase);
    else if (ctl->met_type == 3)
      sprintf(filename, "%s_YYYY_MM_DD_HH.zfp", metbase);
    else if (ctl->met_type == 4)
      sprintf(filename, "%s_YYYY_MM_DD_HH.zstd", metbase);
    sprintf(repl, "%d", year);
    get_met_replace(filename, "YYYY", repl);
    sprintf(repl, "%02d", mon);
    get_met_replace(filename, "MM", repl);
    sprintf(repl, "%02d", day);
    get_met_replace(filename, "DD", repl);
    sprintf(repl, "%02d", hour);
    get_met_replace(filename, "HH", repl);
  }

  /* Set filename of CLaMS meteo files... */
  else {
    sprintf(filename, "%s_YYMMDDHH.nc", metbase);
    sprintf(repl, "%d", year);
    get_met_replace(filename, "YYYY", repl);
    sprintf(repl, "%d", year % 100);
    get_met_replace(filename, "YY", repl);
    sprintf(repl, "%02d", mon);
    get_met_replace(filename, "MM", repl);
    sprintf(repl, "%02d", day);
    get_met_replace(filename, "DD", repl);
    sprintf(repl, "%02d", hour);
    get_met_replace(filename, "HH", repl);
  }
}

/*****************************************************************************/

void get_met_replace(
  char *orig,
  char *search,
  char *repl) {

  char buffer[LEN];

  /* Iterate... */
  for (int i = 0; i < 3; i++) {

    /* Replace sub-string... */
    char *ch;
    if (!(ch = strstr(orig, search)))
      return;
    strncpy(buffer, orig, (size_t) (ch - orig));
    buffer[ch - orig] = 0;
    sprintf(buffer + (ch - orig), "%s%s", repl, ch + strlen(search));
    orig[0] = 0;
    strcpy(orig, buffer);
  }
}

/*****************************************************************************/

void intpol_met_space_3d(
  met_t * met,
  float array[EX][EY][EP],
  double p,
  double lon,
  double lat,
  double *var,
  int *ci,
  double *cw,
  int init) {

  /* Initialize interpolation... */
  if (init) {

    /* Check longitude... */
    if (met->lon[met->nx - 1] > 180 && lon < 0)
      lon += 360;

    /* Get interpolation indices... */
    ci[0] = locate_irr(met->p, met->np, p);
    ci[1] = locate_reg(met->lon, met->nx, lon);
    ci[2] = locate_reg(met->lat, met->ny, lat);

    /* Get interpolation weights... */
    cw[0] = (met->p[ci[0] + 1] - p)
      / (met->p[ci[0] + 1] - met->p[ci[0]]);
    cw[1] = (met->lon[ci[1] + 1] - lon)
      / (met->lon[ci[1] + 1] - met->lon[ci[1]]);
    cw[2] = (met->lat[ci[2] + 1] - lat)
      / (met->lat[ci[2] + 1] - met->lat[ci[2]]);
  }

  /* Interpolate vertically... */
  double aux00 =
    cw[0] * (array[ci[1]][ci[2]][ci[0]] - array[ci[1]][ci[2]][ci[0] + 1])
    + array[ci[1]][ci[2]][ci[0] + 1];
  double aux01 =
    cw[0] * (array[ci[1]][ci[2] + 1][ci[0]] -
	     array[ci[1]][ci[2] + 1][ci[0] + 1])
    + array[ci[1]][ci[2] + 1][ci[0] + 1];
  double aux10 =
    cw[0] * (array[ci[1] + 1][ci[2]][ci[0]] -
	     array[ci[1] + 1][ci[2]][ci[0] + 1])
    + array[ci[1] + 1][ci[2]][ci[0] + 1];
  double aux11 =
    cw[0] * (array[ci[1] + 1][ci[2] + 1][ci[0]] -
	     array[ci[1] + 1][ci[2] + 1][ci[0] + 1])
    + array[ci[1] + 1][ci[2] + 1][ci[0] + 1];

  /* Interpolate horizontally... */
  aux00 = cw[2] * (aux00 - aux01) + aux01;
  aux11 = cw[2] * (aux10 - aux11) + aux11;
  *var = cw[1] * (aux00 - aux11) + aux11;
}

/*****************************************************************************/

void intpol_met_space_2d(
  met_t * met,
  float array[EX][EY],
  double lon,
  double lat,
  double *var,
  int *ci,
  double *cw,
  int init) {

  /* Initialize interpolation... */
  if (init) {

    /* Check longitude... */
    if (met->lon[met->nx - 1] > 180 && lon < 0)
      lon += 360;

    /* Get interpolation indices... */
    ci[1] = locate_reg(met->lon, met->nx, lon);
    ci[2] = locate_reg(met->lat, met->ny, lat);

    /* Get interpolation weights... */
    cw[1] = (met->lon[ci[1] + 1] - lon)
      / (met->lon[ci[1] + 1] - met->lon[ci[1]]);
    cw[2] = (met->lat[ci[2] + 1] - lat)
      / (met->lat[ci[2] + 1] - met->lat[ci[2]]);
  }

  /* Set variables... */
  double aux00 = array[ci[1]][ci[2]];
  double aux01 = array[ci[1]][ci[2] + 1];
  double aux10 = array[ci[1] + 1][ci[2]];
  double aux11 = array[ci[1] + 1][ci[2] + 1];

  /* Interpolate horizontally... */
  if (isfinite(aux00) && isfinite(aux01)
      && isfinite(aux10) && isfinite(aux11)) {
    aux00 = cw[2] * (aux00 - aux01) + aux01;
    aux11 = cw[2] * (aux10 - aux11) + aux11;
    *var = cw[1] * (aux00 - aux11) + aux11;
  } else {
    if (cw[2] < 0.5) {
      if (cw[1] < 0.5)
	*var = aux11;
      else
	*var = aux01;
    } else {
      if (cw[1] < 0.5)
	*var = aux10;
      else
	*var = aux00;
    }
  }
}

/*****************************************************************************/

#ifdef UVW
void intpol_met_space_uvw(
  met_t * met,
  double p,
  double lon,
  double lat,
  double *u,
  double *v,
  double *w,
  int *ci,
  double *cw,
  int init) {

  /* Initialize interpolation... */
  if (init) {

    /* Check longitude... */
    if (met->lon[met->nx - 1] > 180 && lon < 0)
      lon += 360;

    /* Get interpolation indices... */
    ci[0] = locate_irr(met->p, met->np, p);
    ci[1] = locate_reg(met->lon, met->nx, lon);
    ci[2] = locate_reg(met->lat, met->ny, lat);

    /* Get interpolation weights... */
    cw[0] = (met->p[ci[0] + 1] - p)
      / (met->p[ci[0] + 1] - met->p[ci[0]]);
    cw[1] = (met->lon[ci[1] + 1] - lon)
      / (met->lon[ci[1] + 1] - met->lon[ci[1]]);
    cw[2] = (met->lat[ci[2] + 1] - lat)
      / (met->lat[ci[2] + 1] - met->lat[ci[2]]);
  }

  /* Interpolate vertically... */
  double u00 =
    cw[0] * (met->uvw[ci[1]][ci[2]][ci[0]][0] -
	     met->uvw[ci[1]][ci[2]][ci[0] + 1][0])
    + met->uvw[ci[1]][ci[2]][ci[0] + 1][0];
  double u01 =
    cw[0] * (met->uvw[ci[1]][ci[2] + 1][ci[0]][0] -
	     met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][0])
    + met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][0];
  double u10 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2]][ci[0]][0] -
	     met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][0])
    + met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][0];
  double u11 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2] + 1][ci[0]][0] -
	     met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][0])
    + met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][0];

  double v00 =
    cw[0] * (met->uvw[ci[1]][ci[2]][ci[0]][1] -
	     met->uvw[ci[1]][ci[2]][ci[0] + 1][1])
    + met->uvw[ci[1]][ci[2]][ci[0] + 1][1];
  double v01 =
    cw[0] * (met->uvw[ci[1]][ci[2] + 1][ci[0]][1] -
	     met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][1])
    + met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][1];
  double v10 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2]][ci[0]][1] -
	     met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][1])
    + met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][1];
  double v11 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2] + 1][ci[0]][1] -
	     met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][1])
    + met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][1];

  double w00 =
    cw[0] * (met->uvw[ci[1]][ci[2]][ci[0]][2] -
	     met->uvw[ci[1]][ci[2]][ci[0] + 1][2])
    + met->uvw[ci[1]][ci[2]][ci[0] + 1][2];
  double w01 =
    cw[0] * (met->uvw[ci[1]][ci[2] + 1][ci[0]][2] -
	     met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][2])
    + met->uvw[ci[1]][ci[2] + 1][ci[0] + 1][2];
  double w10 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2]][ci[0]][2] -
	     met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][2])
    + met->uvw[ci[1] + 1][ci[2]][ci[0] + 1][2];
  double w11 =
    cw[0] * (met->uvw[ci[1] + 1][ci[2] + 1][ci[0]][2] -
	     met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][2])
    + met->uvw[ci[1] + 1][ci[2] + 1][ci[0] + 1][2];

  /* Interpolate horizontally... */
  u00 = cw[2] * (u00 - u01) + u01;
  u11 = cw[2] * (u10 - u11) + u11;
  *u = cw[1] * (u00 - u11) + u11;

  v00 = cw[2] * (v00 - v01) + v01;
  v11 = cw[2] * (v10 - v11) + v11;
  *v = cw[1] * (v00 - v11) + v11;

  w00 = cw[2] * (w00 - w01) + w01;
  w11 = cw[2] * (w10 - w11) + w11;
  *w = cw[1] * (w00 - w11) + w11;
}
#endif

/*****************************************************************************/

void intpol_met_time_3d(
  met_t * met0,
  float array0[EX][EY][EP],
  met_t * met1,
  float array1[EX][EY][EP],
  double ts,
  double p,
  double lon,
  double lat,
  double *var,
  int *ci,
  double *cw,
  int init) {

  double var0, var1, wt;

  /* Spatial interpolation... */
  intpol_met_space_3d(met0, array0, p, lon, lat, &var0, ci, cw, init);
  intpol_met_space_3d(met1, array1, p, lon, lat, &var1, ci, cw, 0);

  /* Get weighting factor... */
  wt = (met1->time - ts) / (met1->time - met0->time);

  /* Interpolate... */
  *var = wt * (var0 - var1) + var1;
}

/*****************************************************************************/

void intpol_met_time_2d(
  met_t * met0,
  float array0[EX][EY],
  met_t * met1,
  float array1[EX][EY],
  double ts,
  double lon,
  double lat,
  double *var,
  int *ci,
  double *cw,
  int init) {

  double var0, var1, wt;

  /* Spatial interpolation... */
  intpol_met_space_2d(met0, array0, lon, lat, &var0, ci, cw, init);
  intpol_met_space_2d(met1, array1, lon, lat, &var1, ci, cw, 0);

  /* Get weighting factor... */
  wt = (met1->time - ts) / (met1->time - met0->time);

  /* Interpolate... */
  if (isfinite(var0) && isfinite(var1))
    *var = wt * (var0 - var1) + var1;
  else if (wt < 0.5)
    *var = var1;
  else
    *var = var0;
}

/*****************************************************************************/

#ifdef UVW
void intpol_met_time_uvw(
  met_t * met0,
  met_t * met1,
  double ts,
  double p,
  double lon,
  double lat,
  double *u,
  double *v,
  double *w) {

  double u0, u1, v0, v1, w0, w1, wt;

  /* Spatial interpolation... */
  INTPOL_INIT;
  intpol_met_space_uvw(met0, p, lon, lat, &u0, &v0, &w0, ci, cw, 1);
  intpol_met_space_uvw(met1, p, lon, lat, &u1, &v1, &w1, ci, cw, 0);

  /* Get weighting factor... */
  wt = (met1->time - ts) / (met1->time - met0->time);

  /* Interpolate... */
  *u = wt * (u0 - u1) + u1;
  *v = wt * (v0 - v1) + v1;
  *w = wt * (w0 - w1) + w1;
}
#endif

/*****************************************************************************/

void jsec2time(
  double jsec,
  int *year,
  int *mon,
  int *day,
  int *hour,
  int *min,
  int *sec,
  double *remain) {

  struct tm t0, *t1;

  t0.tm_year = 100;
  t0.tm_mon = 0;
  t0.tm_mday = 1;
  t0.tm_hour = 0;
  t0.tm_min = 0;
  t0.tm_sec = 0;

  time_t jsec0 = (time_t) jsec + timegm(&t0);
  t1 = gmtime(&jsec0);

  *year = t1->tm_year + 1900;
  *mon = t1->tm_mon + 1;
  *day = t1->tm_mday;
  *hour = t1->tm_hour;
  *min = t1->tm_min;
  *sec = t1->tm_sec;
  *remain = jsec - floor(jsec);
}

/*****************************************************************************/

double kernel_weight(
  double kz[EP],
  double kw[EP],
  int nk,
  double p) {

  /* Check number of data points... */
  if (nk < 2)
    return 1.0;

  /* Get altitude... */
  double z = Z(p);

  /* Get weighting factor... */
  if (z < kz[0])
    return kw[0];
  else if (z > kz[nk - 1])
    return kw[nk - 1];
  else {
    int idx = locate_irr(kz, nk, z);
    return LIN(kz[idx], kw[idx], kz[idx + 1], kw[idx + 1], z);
  }
}

/*****************************************************************************/

double lapse_rate(
  double t,
  double h2o) {

  /*
     Calculate moist adiabatic lapse rate [K/km] from temperature [K]
     and water vapor volume mixing ratio [1].

     Reference: https://en.wikipedia.org/wiki/Lapse_rate
   */

  const double a = RA * SQR(t), r = SH(h2o) / (1. - SH(h2o));

  return 1e3 * G0 * (a + LV * r * t) / (CPD * a + SQR(LV) * r * EPS);
}

/*****************************************************************************/

int locate_irr(
  double *xx,
  int n,
  double x) {

  int ilo = 0;
  int ihi = n - 1;
  int i = (ihi + ilo) >> 1;

  if (xx[i] < xx[i + 1])
    while (ihi > ilo + 1) {
      i = (ihi + ilo) >> 1;
      if (xx[i] > x)
	ihi = i;
      else
	ilo = i;
  } else
    while (ihi > ilo + 1) {
      i = (ihi + ilo) >> 1;
      if (xx[i] <= x)
	ihi = i;
      else
	ilo = i;
    }

  return ilo;
}

/*****************************************************************************/

int locate_reg(
  double *xx,
  int n,
  double x) {

  /* Calculate index... */
  int i = (int) ((x - xx[0]) / (xx[1] - xx[0]));

  /* Check range... */
  if (i < 0)
    return 0;
  else if (i > n - 2)
    return n - 2;
  else
    return i;
}

/*****************************************************************************/

double nat_temperature(
  double p,
  double h2o,
  double hno3) {

  /* Check water vapor vmr... */
  h2o = GSL_MAX(h2o, 0.1e-6);

  /* Calculate T_NAT... */
  double p_hno3 = hno3 * p / 1.333224;
  double p_h2o = h2o * p / 1.333224;
  double a = 0.009179 - 0.00088 * log10(p_h2o);
  double b = (38.9855 - log10(p_hno3) - 2.7836 * log10(p_h2o)) / a;
  double c = -11397.0 / a;
  double tnat = (-b + sqrt(b * b - 4. * c)) / 2.;
  double x2 = (-b - sqrt(b * b - 4. * c)) / 2.;
  if (x2 > 0)
    tnat = x2;

  return tnat;
}

/*****************************************************************************/

void quicksort(
  double arr[],
  int brr[],
  int low,
  int high) {

  if (low < high) {
    int pi = quicksort_partition(arr, brr, low, high);

#pragma omp task firstprivate(arr,brr,low,pi)
    {
      quicksort(arr, brr, low, pi - 1);
    }

    // #pragma omp task firstprivate(arr,brr,high,pi)
    {
      quicksort(arr, brr, pi + 1, high);
    }
  }
}

/*****************************************************************************/

int quicksort_partition(
  double arr[],
  int brr[],
  int low,
  int high) {

  double pivot = arr[high];
  int i = (low - 1);

  for (int j = low; j <= high - 1; j++)
    if (arr[j] <= pivot) {
      i++;
      SWAP(arr[i], arr[j], double);
      SWAP(brr[i], brr[j], int);
    }
  SWAP(arr[high], arr[i + 1], double);
  SWAP(brr[high], brr[i + 1], int);

  return (i + 1);
}

/*****************************************************************************/

int read_atm(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  int result;

  /* Set timer... */
  SELECT_TIMER("READ_ATM", "INPUT", NVTX_READ);

  /* Init... */
  atm->np = 0;

  /* Write info... */
  LOG(1, "Read atmospheric data: %s", filename);

  /* Read ASCII data... */
  if (ctl->atm_type == 0)
    result = read_atm_asc(filename, ctl, atm);

  /* Read binary data... */
  else if (ctl->atm_type == 1)
    result = read_atm_bin(filename, ctl, atm);

  /* Read netCDF data... */
  else if (ctl->atm_type == 2)
    result = read_atm_nc(filename, ctl, atm);

  /* Read CLaMS data... */
  else if (ctl->atm_type == 3)
    result = read_atm_clams(filename, ctl, atm);

  /* Error... */
  else
    ERRMSG("Atmospheric data type not supported!");

  /* Check result... */
  if (result != 1)
    return 0;

  /* Check number of air parcels... */
  if (atm->np < 1)
    ERRMSG("Can not read any data!");

  /* Write info... */
  double mini, maxi;
  LOG(2, "Number of particles: %d", atm->np);
  gsl_stats_minmax(&mini, &maxi, atm->time, 1, (size_t) atm->np);
  LOG(2, "Time range: %.2f ... %.2f s", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, atm->p, 1, (size_t) atm->np);
  LOG(2, "Altitude range: %g ... %g km", Z(maxi), Z(mini));
  LOG(2, "Pressure range: %g ... %g hPa", maxi, mini);
  gsl_stats_minmax(&mini, &maxi, atm->lon, 1, (size_t) atm->np);
  LOG(2, "Longitude range: %g ... %g deg", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, atm->lat, 1, (size_t) atm->np);
  LOG(2, "Latitude range: %g ... %g deg", mini, maxi);
  for (int iq = 0; iq < ctl->nq; iq++) {
    char msg[LEN];
    sprintf(msg, "Quantity %s range: %s ... %s %s",
	    ctl->qnt_name[iq], ctl->qnt_format[iq],
	    ctl->qnt_format[iq], ctl->qnt_unit[iq]);
    gsl_stats_minmax(&mini, &maxi, atm->q[iq], 1, (size_t) atm->np);
    LOG(2, msg, mini, maxi);
  }

  /* Return success... */
  return 1;
}

/*****************************************************************************/

int read_atm_asc(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  FILE *in;

  /* Open file... */
  if (!(in = fopen(filename, "r"))) {
    WARN("Cannot open file!");
    return 0;
  }

  /* Read line... */
  char line[LEN];
  while (fgets(line, LEN, in)) {

    /* Read data... */
    char *tok;
    TOK(line, tok, "%lg", atm->time[atm->np]);
    TOK(NULL, tok, "%lg", atm->p[atm->np]);
    TOK(NULL, tok, "%lg", atm->lon[atm->np]);
    TOK(NULL, tok, "%lg", atm->lat[atm->np]);
    for (int iq = 0; iq < ctl->nq; iq++)
      TOK(NULL, tok, "%lg", atm->q[iq][atm->np]);

    /* Convert altitude to pressure... */
    atm->p[atm->np] = P(atm->p[atm->np]);

    /* Increment data point counter... */
    if ((++atm->np) > NP)
      ERRMSG("Too many data points!");
  }

  /* Close file... */
  fclose(in);

  /* Return success... */
  return 1;
}

/*****************************************************************************/

int read_atm_bin(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  FILE *in;

  /* Open file... */
  if (!(in = fopen(filename, "r")))
    return 0;

  /* Check version of binary data... */
  int version;
  FREAD(&version, int,
	1,
	in);
  if (version != 100)
    ERRMSG("Wrong version of binary data!");

  /* Read data... */
  FREAD(&atm->np, int,
	1,
	in);
  FREAD(atm->time, double,
	  (size_t) atm->np,
	in);
  FREAD(atm->p, double,
	  (size_t) atm->np,
	in);
  FREAD(atm->lon, double,
	  (size_t) atm->np,
	in);
  FREAD(atm->lat, double,
	  (size_t) atm->np,
	in);
  for (int iq = 0; iq < ctl->nq; iq++)
    FREAD(atm->q[iq], double,
	    (size_t) atm->np,
	  in);

  /* Read final flag... */
  int final;
  FREAD(&final, int,
	1,
	in);
  if (final != 999)
    ERRMSG("Error while reading binary data!");

  /* Close file... */
  fclose(in);

  /* Return success... */
  return 1;
}

/*****************************************************************************/

int read_atm_clams(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  int ncid, varid;

  /* Open file... */
  if (nc_open(filename, NC_NOWRITE, &ncid) != NC_NOERR)
    return 0;

  /* Get dimensions... */
  NC_INQ_DIM("NPARTS", &atm->np, 1, NP);

  /* Get time... */
  if (nc_inq_varid(ncid, "TIME_INIT", &varid) == NC_NOERR) {
    NC(nc_get_var_double(ncid, varid, atm->time));
  } else {
    WARN("TIME_INIT not found use time instead!");
    double time_init;
    NC_GET_DOUBLE("time", &time_init, 1);
    for (int ip = 0; ip < atm->np; ip++) {
      atm->time[ip] = time_init;
    }
  }

  /* Read zeta coordinate, pressure is optional... */
  if (ctl->vert_coord_ap == 1) {
    NC_GET_DOUBLE("ZETA", atm->zeta, 1);
    NC_GET_DOUBLE("PRESS", atm->p, 0);
  }

  /* Read pressure, zeta coordinate is optional... */
  else {
    NC_GET_DOUBLE("PRESS", atm->p, 1);
    NC_GET_DOUBLE("ZETA", atm->zeta, 0);
  }

  /* Read longitude and latitude... */
  NC_GET_DOUBLE("LON", atm->lon, 1);
  NC_GET_DOUBLE("LAT", atm->lat, 1);

  /* Close file... */
  NC(nc_close(ncid));

  /* Return success... */
  return 1;
}

/*****************************************************************************/

int read_atm_nc(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  int ncid, varid;

  /* Open file... */
  if (nc_open(filename, NC_NOWRITE, &ncid) != NC_NOERR)
    return 0;

  /* Get dimensions... */
  NC_INQ_DIM("obs", &atm->np, 1, NP);

  /* Read geolocations... */
  NC_GET_DOUBLE("time", atm->time, 1);
  NC_GET_DOUBLE("press", atm->p, 1);
  NC_GET_DOUBLE("lon", atm->lon, 1);
  NC_GET_DOUBLE("lat", atm->lat, 1);

  /* Read variables... */
  for (int iq = 0; iq < ctl->nq; iq++)
    NC_GET_DOUBLE(ctl->qnt_name[iq], atm->q[iq], 0);

  /* Close file... */
  NC(nc_close(ncid));

  /* Return success... */
  return 1;
}

/*****************************************************************************/

void read_clim(
  ctl_t * ctl,
  clim_t * clim) {

  /* Set timer... */
  SELECT_TIMER("READ_CLIM", "INPUT", NVTX_READ);

  /* Init tropopause climatology... */
  clim_tropo_init(clim);

  /* Init HNO3 climatology... */
  clim_hno3_init(clim);

  /* Read OH climatology... */
  if (ctl->clim_oh_filename[0] != '-')
    clim_oh_init(ctl, clim);

  /* Read H2O2 climatology... */
  if (ctl->clim_h2o2_filename[0] != '-')
    clim_h2o2_init(ctl, clim);

  if (ctl->clim_ho2_filename[0] != '-')
    clim_ho2_init(ctl, clim);
}

/*****************************************************************************/

void read_ctl(
  const char *filename,
  int argc,
  char *argv[],
  ctl_t * ctl) {

  /* Set timer... */
  SELECT_TIMER("READ_CTL", "INPUT", NVTX_READ);

  /* Write info... */
  LOG(1, "\nMassive-Parallel Trajectory Calculations (MPTRAC)\n"
      "(executable: %s | version: %s | compiled: %s, %s)\n",
      argv[0], VERSION, __DATE__, __TIME__);

  /* Initialize quantity indices... */
  ctl->qnt_idx = -1;
  ctl->qnt_ens = -1;
  ctl->qnt_stat = -1;
  ctl->qnt_m = -1;
  ctl->qnt_vmr = -1;
  ctl->qnt_rp = -1;
  ctl->qnt_rhop = -1;
  ctl->qnt_ps = -1;
  ctl->qnt_ts = -1;
  ctl->qnt_zs = -1;
  ctl->qnt_us = -1;
  ctl->qnt_vs = -1;
  ctl->qnt_lsm = -1;
  ctl->qnt_sst = -1;
  ctl->qnt_pbl = -1;
  ctl->qnt_pt = -1;
  ctl->qnt_tt = -1;
  ctl->qnt_zt = -1;
  ctl->qnt_h2ot = -1;
  ctl->qnt_z = -1;
  ctl->qnt_p = -1;
  ctl->qnt_t = -1;
  ctl->qnt_rho = -1;
  ctl->qnt_u = -1;
  ctl->qnt_v = -1;
  ctl->qnt_w = -1;
  ctl->qnt_h2o = -1;
  ctl->qnt_o3 = -1;
  ctl->qnt_lwc = -1;
  ctl->qnt_iwc = -1;
  ctl->qnt_cc = -1;
  ctl->qnt_pct = -1;
  ctl->qnt_pcb = -1;
  ctl->qnt_cl = -1;
  ctl->qnt_plcl = -1;
  ctl->qnt_plfc = -1;
  ctl->qnt_pel = -1;
  ctl->qnt_cape = -1;
  ctl->qnt_cin = -1;
  ctl->qnt_hno3 = -1;
  ctl->qnt_oh = -1;
  ctl->qnt_vmrimpl = -1;
  ctl->qnt_mloss_oh = -1;
  ctl->qnt_mloss_h2o2 = -1;
  ctl->qnt_mloss_wet = -1;
  ctl->qnt_mloss_dry = -1;
  ctl->qnt_mloss_decay = -1;
  ctl->qnt_psat = -1;
  ctl->qnt_psice = -1;
  ctl->qnt_pw = -1;
  ctl->qnt_sh = -1;
  ctl->qnt_rh = -1;
  ctl->qnt_rhice = -1;
  ctl->qnt_theta = -1;
  ctl->qnt_zeta = -1;
  ctl->qnt_tvirt = -1;
  ctl->qnt_lapse = -1;
  ctl->qnt_vh = -1;
  ctl->qnt_vz = -1;
  ctl->qnt_pv = -1;
  ctl->qnt_tdew = -1;
  ctl->qnt_tice = -1;
  ctl->qnt_tsts = -1;
  ctl->qnt_tnat = -1;
  ctl->qnt_Cso2 = -1;    /* TODO: change names from "qnt_Cso2" to "qnt_so2", etc... Think about the units (but don't try to unify them)... */
  ctl->qnt_Ch2o2 = -1;
  ctl->qnt_Cho2 = -1;

  /* Read quantities... */
  ctl->nq = (int) scan_ctl(filename, argc, argv, "NQ", -1, "0", NULL);
  if (ctl->nq > NQ)
    ERRMSG("Too many quantities!");
  for (int iq = 0; iq < ctl->nq; iq++) {

    /* Read quantity name and format... */
    scan_ctl(filename, argc, argv, "QNT_NAME", iq, "", ctl->qnt_name[iq]);
    scan_ctl(filename, argc, argv, "QNT_LONGNAME", iq, ctl->qnt_name[iq],
	     ctl->qnt_longname[iq]);
    scan_ctl(filename, argc, argv, "QNT_FORMAT", iq, "%g",
	     ctl->qnt_format[iq]);

    /* Try to identify quantity... */
    SET_QNT(qnt_idx, "idx", "particle index", "-")
      SET_QNT(qnt_ens, "ens", "ensemble index", "-")
      SET_QNT(qnt_stat, "stat", "station flag", "-")
      SET_QNT(qnt_m, "m", "mass", "kg")
      SET_QNT(qnt_vmr, "vmr", "volume mixing ratio", "ppv")
      SET_QNT(qnt_rp, "rp", "particle radius", "microns")
      SET_QNT(qnt_rhop, "rhop", "particle density", "kg/m^3")
      SET_QNT(qnt_ps, "ps", "surface pressure", "hPa")
      SET_QNT(qnt_ts, "ts", "surface temperature", "K")
      SET_QNT(qnt_zs, "zs", "surface height", "km")
      SET_QNT(qnt_us, "us", "surface zonal wind", "m/s")
      SET_QNT(qnt_vs, "vs", "surface meridional wind", "m/s")
      SET_QNT(qnt_lsm, "lsm", "land-sea mask", "1")
      SET_QNT(qnt_sst, "sst", "sea surface temperature", "K")
      SET_QNT(qnt_pbl, "pbl", "planetary boundary layer", "hPa")
      SET_QNT(qnt_pt, "pt", "tropopause pressure", "hPa")
      SET_QNT(qnt_tt, "tt", "tropopause temperature", "K")
      SET_QNT(qnt_zt, "zt", "tropopause geopotential height", "km")
      SET_QNT(qnt_h2ot, "h2ot", "tropopause water vapor", "ppv")
      SET_QNT(qnt_z, "z", "geopotential height", "km")
      SET_QNT(qnt_p, "p", "pressure", "hPa")
      SET_QNT(qnt_t, "t", "temperature", "K")
      SET_QNT(qnt_rho, "rho", "air density", "kg/m^3")
      SET_QNT(qnt_u, "u", "zonal wind", "m/s")
      SET_QNT(qnt_v, "v", "meridional wind", "m/s")
      SET_QNT(qnt_w, "w", "vertical velocity", "hPa/s")
      SET_QNT(qnt_h2o, "h2o", "water vapor", "ppv")
      SET_QNT(qnt_o3, "o3", "ozone", "ppv")
      SET_QNT(qnt_lwc, "lwc", "cloud ice water content", "kg/kg")
      SET_QNT(qnt_iwc, "iwc", "cloud liquid water content", "kg/kg")
      SET_QNT(qnt_cc, "cc", "cloud cover", "1")
      SET_QNT(qnt_pct, "pct", "cloud top pressure", "hPa")
      SET_QNT(qnt_pcb, "pcb", "cloud bottom pressure", "hPa")
      SET_QNT(qnt_cl, "cl", "total column cloud water", "kg/m^2")
      SET_QNT(qnt_plcl, "plcl", "lifted condensation level", "hPa")
      SET_QNT(qnt_plfc, "plfc", "level of free convection", "hPa")
      SET_QNT(qnt_pel, "pel", "equilibrium level", "hPa")
      SET_QNT(qnt_cape, "cape", "convective available potential energy",
	      "J/kg")
      SET_QNT(qnt_cin, "cin", "convective inhibition", "J/kg")
      SET_QNT(qnt_hno3, "hno3", "nitric acid", "ppv")
      SET_QNT(qnt_oh, "oh", "hydroxyl radical", "molec/cm^3")
      SET_QNT(qnt_vmrimpl, "vmrimpl", "volume mixing ratio (implicit)", "ppv")
      SET_QNT(qnt_mloss_oh, "mloss_oh", "mass loss due to OH chemistry", "kg")
      SET_QNT(qnt_mloss_h2o2, "mloss_h2o2", "mass loss due to H2O2 chemistry",
	      "kg")
      SET_QNT(qnt_mloss_wet, "mloss_wet", "mass loss due to wet deposition",
	      "kg")
      SET_QNT(qnt_mloss_dry, "mloss_dry", "mass loss due to dry deposition",
	      "kg")
      SET_QNT(qnt_mloss_decay, "mloss_decay",
	      "mass loss due to exponential decay", "kg")
      SET_QNT(qnt_psat, "psat", "saturation pressure over water", "hPa")
      SET_QNT(qnt_psice, "psice", "saturation pressure over ice", "hPa")
      SET_QNT(qnt_pw, "pw", "partial water vapor pressure", "hPa")
      SET_QNT(qnt_sh, "sh", "specific humidity", "kg/kg")
      SET_QNT(qnt_rh, "rh", "relative humidity", "%%")
      SET_QNT(qnt_rhice, "rhice", "relative humidity over ice", "%%")
      SET_QNT(qnt_theta, "theta", "potential temperature", "K")
      SET_QNT(qnt_zeta, "zeta", "zeta coordinate", "K")
      SET_QNT(qnt_tvirt, "tvirt", "virtual temperature", "K")
      SET_QNT(qnt_lapse, "lapse", "temperature lapse rate", "K/km")
      SET_QNT(qnt_vh, "vh", "horizontal velocity", "m/s")
      SET_QNT(qnt_vz, "vz", "vertical velocity", "m/s")
      SET_QNT(qnt_pv, "pv", "potential vorticity", "PVU")
      SET_QNT(qnt_tdew, "tdew", "dew point temperature", "K")
      SET_QNT(qnt_tice, "tice", "frost point temperature", "K")
      SET_QNT(qnt_tsts, "tsts", "STS existence temperature", "K")
      SET_QNT(qnt_tnat, "tnat", "NAT existence temperature", "K")
      SET_QNT(qnt_Cso2, "Cso2", "particle SO2 concentration", "mole/cm3")   /* TODO: change unit to "molec/cm3"  */
      SET_QNT(qnt_Ch2o2, "Ch2o2", "particle H2O2 concentration", "mole/cm3")
      SET_QNT(qnt_Ch2o2, "Cho2", "particle HO2 concentration", "mole/cm3")
      scan_ctl(filename, argc, argv, "QNT_UNIT", iq, "", ctl->qnt_unit[iq]);
  }

  /* netCDF I/O parameters... */
  ctl->chunkszhint =
    (size_t) scan_ctl(filename, argc, argv, "CHUNKSZHINT", -1, "163840000",
		      NULL);
  ctl->read_mode =
    (int) scan_ctl(filename, argc, argv, "READMODE", -1, "0", NULL);

  /* Vertical coordinates and velocities... */
  ctl->vert_coord_ap =
    (int) scan_ctl(filename, argc, argv, "VERT_COORD_AP", -1, "0", NULL);
  ctl->vert_coord_met =
    (int) scan_ctl(filename, argc, argv, "VERT_COORD_MET", -1, "0", NULL);
  ctl->vert_vel =
    (int) scan_ctl(filename, argc, argv, "VERT_VEL", -1, "0", NULL);
  ctl->clams_met_data =
    (int) scan_ctl(filename, argc, argv, "CLAMS_MET_DATA", -1, "0", NULL);

  /* Time steps of simulation... */
  ctl->direction =
    (int) scan_ctl(filename, argc, argv, "DIRECTION", -1, "1", NULL);
  if (ctl->direction != -1 && ctl->direction != 1)
    ERRMSG("Set DIRECTION to -1 or 1!");
  ctl->t_stop = scan_ctl(filename, argc, argv, "T_STOP", -1, "1e100", NULL);
  ctl->dt_mod = scan_ctl(filename, argc, argv, "DT_MOD", -1, "180", NULL);

  /* Meteo data... */
  scan_ctl(filename, argc, argv, "METBASE", -1, "-", ctl->metbase);
  ctl->dt_met = scan_ctl(filename, argc, argv, "DT_MET", -1, "3600", NULL);
  ctl->met_type =
    (int) scan_ctl(filename, argc, argv, "MET_TYPE", -1, "0", NULL);
  ctl->met_nc_scale =
    (int) scan_ctl(filename, argc, argv, "MET_NC_SCALE", -1, "1", NULL);
  ctl->met_dx = (int) scan_ctl(filename, argc, argv, "MET_DX", -1, "1", NULL);
  ctl->met_dy = (int) scan_ctl(filename, argc, argv, "MET_DY", -1, "1", NULL);
  ctl->met_dp = (int) scan_ctl(filename, argc, argv, "MET_DP", -1, "1", NULL);
  if (ctl->met_dx < 1 || ctl->met_dy < 1 || ctl->met_dp < 1)
    ERRMSG("MET_DX, MET_DY, and MET_DP need to be greater than zero!");
  ctl->met_sx = (int) scan_ctl(filename, argc, argv, "MET_SX", -1, "1", NULL);
  ctl->met_sy = (int) scan_ctl(filename, argc, argv, "MET_SY", -1, "1", NULL);
  ctl->met_sp = (int) scan_ctl(filename, argc, argv, "MET_SP", -1, "1", NULL);
  if (ctl->met_sx < 1 || ctl->met_sy < 1 || ctl->met_sp < 1)
    ERRMSG("MET_SX, MET_SY, and MET_SP need to be greater than zero!");
  ctl->met_detrend =
    scan_ctl(filename, argc, argv, "MET_DETREND", -1, "-999", NULL);
  ctl->met_np = (int) scan_ctl(filename, argc, argv, "MET_NP", -1, "0", NULL);
  if (ctl->met_np > EP)
    ERRMSG("Too many levels!");
  for (int ip = 0; ip < ctl->met_np; ip++)
    ctl->met_p[ip] = scan_ctl(filename, argc, argv, "MET_P", ip, "", NULL);
  ctl->met_geopot_sx
    = (int) scan_ctl(filename, argc, argv, "MET_GEOPOT_SX", -1, "-1", NULL);
  ctl->met_geopot_sy
    = (int) scan_ctl(filename, argc, argv, "MET_GEOPOT_SY", -1, "-1", NULL);
  ctl->met_relhum
    = (int) scan_ctl(filename, argc, argv, "MET_RELHUM", -1, "0", NULL);
  ctl->met_tropo =
    (int) scan_ctl(filename, argc, argv, "MET_TROPO", -1, "3", NULL);
  if (ctl->met_tropo < 0 || ctl->met_tropo > 5)
    ERRMSG("Set MET_TROPO = 0 ... 5!");
  ctl->met_tropo_lapse =
    scan_ctl(filename, argc, argv, "MET_TROPO_LAPSE", -1, "2.0", NULL);
  ctl->met_tropo_nlev =
    (int) scan_ctl(filename, argc, argv, "MET_TROPO_NLEV", -1, "20", NULL);
  ctl->met_tropo_lapse_sep =
    scan_ctl(filename, argc, argv, "MET_TROPO_LAPSE_SEP", -1, "3.0", NULL);
  ctl->met_tropo_nlev_sep =
    (int) scan_ctl(filename, argc, argv, "MET_TROPO_NLEV_SEP", -1, "10",
		   NULL);
  ctl->met_tropo_pv =
    scan_ctl(filename, argc, argv, "MET_TROPO_PV", -1, "3.5", NULL);
  ctl->met_tropo_theta =
    scan_ctl(filename, argc, argv, "MET_TROPO_THETA", -1, "380", NULL);
  ctl->met_tropo_spline =
    (int) scan_ctl(filename, argc, argv, "MET_TROPO_SPLINE", -1, "1", NULL);
  ctl->met_cloud =
    (int) scan_ctl(filename, argc, argv, "MET_CLOUD", -1, "1", NULL);
  if (ctl->met_cloud < 0 || ctl->met_cloud > 3)
    ERRMSG("Set MET_CLOUD = 0 ... 3!");
  ctl->met_cloud_min =
    scan_ctl(filename, argc, argv, "MET_CLOUD_MIN", -1, "0", NULL);
  ctl->met_dt_out =
    scan_ctl(filename, argc, argv, "MET_DT_OUT", -1, "0.1", NULL);
  ctl->met_cache =
    (int) scan_ctl(filename, argc, argv, "MET_CACHE", -1, "0", NULL);

  /* Sorting... */
  ctl->sort_dt = scan_ctl(filename, argc, argv, "SORT_DT", -1, "-999", NULL);

  /* Isosurface parameters... */
  ctl->isosurf =
    (int) scan_ctl(filename, argc, argv, "ISOSURF", -1, "0", NULL);
  scan_ctl(filename, argc, argv, "BALLOON", -1, "-", ctl->balloon);

  /* Advection parameters... */
  ctl->advect = (int) scan_ctl(filename, argc, argv, "ADVECT", -1, "2", NULL);
  if (!(ctl->advect == 1 || ctl->advect == 2 || ctl->advect == 4))
    ERRMSG("Set ADVECT to 1, 2, or 4!");
  ctl->reflect =
    (int) scan_ctl(filename, argc, argv, "REFLECT", -1, "0", NULL);

  /* Diffusion parameters... */
  ctl->turb_dx_trop =
    scan_ctl(filename, argc, argv, "TURB_DX_TROP", -1, "50", NULL);
  ctl->turb_dx_strat =
    scan_ctl(filename, argc, argv, "TURB_DX_STRAT", -1, "0", NULL);
  ctl->turb_dz_trop =
    scan_ctl(filename, argc, argv, "TURB_DZ_TROP", -1, "0", NULL);
  ctl->turb_dz_strat =
    scan_ctl(filename, argc, argv, "TURB_DZ_STRAT", -1, "0.1", NULL);
  ctl->turb_mesox =
    scan_ctl(filename, argc, argv, "TURB_MESOX", -1, "0.16", NULL);
  ctl->turb_mesoz =
    scan_ctl(filename, argc, argv, "TURB_MESOZ", -1, "0.16", NULL);

  /* Convection... */
  ctl->conv_cape
    = scan_ctl(filename, argc, argv, "CONV_CAPE", -1, "-999", NULL);
  ctl->conv_cin
    = scan_ctl(filename, argc, argv, "CONV_CIN", -1, "-999", NULL);
  ctl->conv_dt = scan_ctl(filename, argc, argv, "CONV_DT", -1, "-999", NULL);
  ctl->conv_mix
    = (int) scan_ctl(filename, argc, argv, "CONV_MIX", -1, "1", NULL);
  ctl->conv_mix_bot
    = (int) scan_ctl(filename, argc, argv, "CONV_MIX_BOT", -1, "1", NULL);
  ctl->conv_mix_top
    = (int) scan_ctl(filename, argc, argv, "CONV_MIX_TOP", -1, "1", NULL);

  /* Boundary conditions... */
  ctl->bound_mass =
    scan_ctl(filename, argc, argv, "BOUND_MASS", -1, "-999", NULL);
  ctl->bound_mass_trend =
    scan_ctl(filename, argc, argv, "BOUND_MASS_TREND", -1, "0", NULL);
  ctl->bound_vmr =
    scan_ctl(filename, argc, argv, "BOUND_VMR", -1, "-999", NULL);
  ctl->bound_vmr_trend =
    scan_ctl(filename, argc, argv, "BOUND_VMR_TREND", -1, "0", NULL);
  ctl->bound_lat0 =
    scan_ctl(filename, argc, argv, "BOUND_LAT0", -1, "-90", NULL);
  ctl->bound_lat1 =
    scan_ctl(filename, argc, argv, "BOUND_LAT1", -1, "90", NULL);
  ctl->bound_p0 =
    scan_ctl(filename, argc, argv, "BOUND_P0", -1, "1e10", NULL);
  ctl->bound_p1 =
    scan_ctl(filename, argc, argv, "BOUND_P1", -1, "-1e10", NULL);
  ctl->bound_dps =
    scan_ctl(filename, argc, argv, "BOUND_DPS", -1, "-999", NULL);
  ctl->bound_dzs =
    scan_ctl(filename, argc, argv, "BOUND_DZS", -1, "-999", NULL);
  ctl->bound_zetas =
    scan_ctl(filename, argc, argv, "BOUND_ZETAS", -1, "-999", NULL);
  ctl->bound_pbl =
    (int) scan_ctl(filename, argc, argv, "BOUND_PBL", -1, "0", NULL);

  /* Species parameters... */
  scan_ctl(filename, argc, argv, "SPECIES", -1, "-", ctl->species);
  if (strcasecmp(ctl->species, "CF2Cl2") == 0) {
    ctl->molmass = 120.907;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 3e-5;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 3500.0;
  } else if (strcasecmp(ctl->species, "CFCl3") == 0) {
    ctl->molmass = 137.359;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1.1e-4;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 3300.0;
  } else if (strcasecmp(ctl->species, "CH4") == 0) {
    ctl->molmass = 16.043;
    ctl->oh_chem_reaction = 2;
    ctl->oh_chem[0] = 2.45e-12;
    ctl->oh_chem[1] = 1775;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1.4e-5;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 1600.0;
  } else if (strcasecmp(ctl->species, "CO") == 0) {
    ctl->molmass = 28.01;
    ctl->oh_chem_reaction = 3;
    ctl->oh_chem[0] = 6.9e-33;
    ctl->oh_chem[1] = 2.1;
    ctl->oh_chem[2] = 1.1e-12;
    ctl->oh_chem[3] = -1.3;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 9.7e-6;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 1300.0;
  } else if (strcasecmp(ctl->species, "CO2") == 0) {
    ctl->molmass = 44.009;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 3.3e-4;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 2400.0;
  } else if (strcasecmp(ctl->species, "H2O") == 0) {
    ctl->molmass = 18.01528;
  } else if (strcasecmp(ctl->species, "N2O") == 0) {
    ctl->molmass = 44.013;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 2.4e-4;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 2600.;
  } else if (strcasecmp(ctl->species, "NH3") == 0) {
    ctl->molmass = 17.031;
    ctl->oh_chem_reaction = 2;
    ctl->oh_chem[0] = 1.7e-12;
    ctl->oh_chem[1] = 710;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 5.9e-1;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 4200.0;
  } else if (strcasecmp(ctl->species, "HNO3") == 0) {
    ctl->molmass = 63.012;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 2.1e3;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 8700.0;
  } else if (strcasecmp(ctl->species, "NO") == 0) {
    ctl->molmass = 30.006;
    ctl->oh_chem_reaction = 3;
    ctl->oh_chem[0] = 7.1e-31;
    ctl->oh_chem[1] = 2.6;
    ctl->oh_chem[2] = 3.6e-11;
    ctl->oh_chem[3] = 0.1;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1.9e-5;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 1600.0;
  } else if (strcasecmp(ctl->species, "NO2") == 0) {
    ctl->molmass = 46.005;
    ctl->oh_chem_reaction = 3;
    ctl->oh_chem[0] = 1.8e-30;
    ctl->oh_chem[1] = 3.0;
    ctl->oh_chem[2] = 2.8e-11;
    ctl->oh_chem[3] = 0.0;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1.2e-4;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 2400.0;
  } else if (strcasecmp(ctl->species, "O3") == 0) {
    ctl->molmass = 47.997;
    ctl->oh_chem_reaction = 2;
    ctl->oh_chem[0] = 1.7e-12;
    ctl->oh_chem[1] = 940;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1e-4;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 2800.0;
  } else if (strcasecmp(ctl->species, "SF6") == 0) {
    ctl->molmass = 146.048;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 2.4e-6;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 3100.0;
  } else if (strcasecmp(ctl->species, "SO2") == 0) {
    ctl->molmass = 64.066;
    ctl->oh_chem_reaction = 3;
    ctl->oh_chem[0] = 2.9e-31;
    ctl->oh_chem[1] = 4.1;
    ctl->oh_chem[2] = 1.7e-12;
    ctl->oh_chem[3] = -0.2;
    ctl->wet_depo_ic_h[0] = ctl->wet_depo_bc_h[0] = 1.3e-2;
    ctl->wet_depo_ic_h[1] = ctl->wet_depo_bc_h[1] = 2900.0;
  } else {
    ctl->molmass =
      scan_ctl(filename, argc, argv, "MOLMASS", -1, "-999", NULL);
    ctl->chemistry =
      (int) scan_ctl(filename, argc, argv, "CHEMISTRY", -1, "0", NULL);
    ctl->oh_chem_reaction =
      (int) scan_ctl(filename, argc, argv, "OH_CHEM_REACTION", -1, "0", NULL);
    ctl->h2o2_chem_reaction =
      (int) scan_ctl(filename, argc, argv, "H2O2_CHEM_REACTION", -1, "0",
		     NULL);
    for (int ip = 0; ip < 4; ip++)
      ctl->oh_chem[ip] =
	scan_ctl(filename, argc, argv, "OH_CHEM", ip, "0", NULL);
    ctl->dry_depo_vdep =
      scan_ctl(filename, argc, argv, "DRY_DEPO_VDEP", -1, "0", NULL);
    ctl->dry_depo_dp =
      scan_ctl(filename, argc, argv, "DRY_DEPO_DP", -1, "30", NULL);
    ctl->wet_depo_ic_a =
      scan_ctl(filename, argc, argv, "WET_DEPO_IC_A", -1, "0", NULL);
    ctl->wet_depo_ic_b =
      scan_ctl(filename, argc, argv, "WET_DEPO_IC_B", -1, "0", NULL);
    ctl->wet_depo_bc_a =
      scan_ctl(filename, argc, argv, "WET_DEPO_BC_A", -1, "0", NULL);
    ctl->wet_depo_bc_b =
      scan_ctl(filename, argc, argv, "WET_DEPO_BC_B", -1, "0", NULL);
    for (int ip = 0; ip < 3; ip++)
      ctl->wet_depo_ic_h[ip] =
	scan_ctl(filename, argc, argv, "WET_DEPO_IC_H", ip, "0", NULL);
    for (int ip = 0; ip < 1; ip++)
      ctl->wet_depo_bc_h[ip] =
	scan_ctl(filename, argc, argv, "WET_DEPO_BC_H", ip, "0", NULL);
  }

  /* Wet deposition... */
  ctl->wet_depo_pre[0] =
    scan_ctl(filename, argc, argv, "WET_DEPO_PRE", 0, "0.5", NULL);
  ctl->wet_depo_pre[1] =
    scan_ctl(filename, argc, argv, "WET_DEPO_PRE", 1, "0.36", NULL);
  ctl->wet_depo_ic_ret_ratio =
    scan_ctl(filename, argc, argv, "WET_DEPO_IC_RET_RATIO", -1, "1", NULL);
  ctl->wet_depo_bc_ret_ratio =
    scan_ctl(filename, argc, argv, "WET_DEPO_BC_RET_RATIO", -1, "1", NULL);

  /* OH chemistry... */
  ctl->oh_chem_beta =
    scan_ctl(filename, argc, argv, "OH_CHEM_BETA", -1, "0", NULL);
  scan_ctl(filename, argc, argv, "CLIM_OH_FILENAME", -1,
	   "../../data/clams_radical_species.nc", ctl->clim_oh_filename);

  /* H2O2 chemistry... */
  ctl->h2o2_chem_cc =
    scan_ctl(filename, argc, argv, "H2O2_CHEM_CC", -1, "1", NULL);
  scan_ctl(filename, argc, argv, "CLIM_H2O2_FILENAME", -1,
	   "../../data/cams_H2O2.nc", ctl->clim_h2o2_filename);

  scan_ctl(filename, argc, argv, "CLIM_HO2_FILENAME", -1,
	   "../../data/clams_radical_species.nc", ctl->clim_ho2_filename);

  /* Chemistry grid... */
  ctl->chemgrid_z0 =
    scan_ctl(filename, argc, argv, "CHEMGRID_Z0", -1, "0", NULL);
  ctl->chemgrid_z1 =
    scan_ctl(filename, argc, argv, "CHEMGRID_Z1", -1, "100", NULL);
  ctl->chemgrid_nz =
    (int) scan_ctl(filename, argc, argv, "CHEMGRID_NZ", -1, "1", NULL);
  ctl->chemgrid_lon0 =
    scan_ctl(filename, argc, argv, "CHEMGRID_LON0", -1, "-180", NULL);
  ctl->chemgrid_lon1 =
    scan_ctl(filename, argc, argv, "CHEMGRID_LON1", -1, "180", NULL);
  ctl->chemgrid_nx =
    (int) scan_ctl(filename, argc, argv, "CHEMGRID_NX", -1, "360", NULL);
  ctl->chemgrid_lat0 =
    scan_ctl(filename, argc, argv, "CHEMGRID_LAT0", -1, "-90", NULL);
  ctl->chemgrid_lat1 =
    scan_ctl(filename, argc, argv, "CHEMGRID_LAT1", -1, "90", NULL);
  ctl->chemgrid_ny =
    (int) scan_ctl(filename, argc, argv, "CHEMGRID_NY", -1, "180", NULL);
  ctl->interparc_trop =
    (double) scan_ctl(filename, argc, argv, "INTERPARC_TROP", -1, "1e-3",
		      NULL);
  ctl->interparc_strat =
    (double) scan_ctl(filename, argc, argv, "INTERPARC_STRAT", -1, "1e-6",
		      NULL);

  /* Exponential decay... */
  ctl->tdec_trop = scan_ctl(filename, argc, argv, "TDEC_TROP", -1, "0", NULL);
  ctl->tdec_strat =
    scan_ctl(filename, argc, argv, "TDEC_STRAT", -1, "0", NULL);

  /* PSC analysis... */
  ctl->psc_h2o = scan_ctl(filename, argc, argv, "PSC_H2O", -1, "4e-6", NULL);
  ctl->psc_hno3 =
    scan_ctl(filename, argc, argv, "PSC_HNO3", -1, "9e-9", NULL);

  /* Output of atmospheric data... */
  scan_ctl(filename, argc, argv, "ATM_BASENAME", -1, "-", ctl->atm_basename);
  scan_ctl(filename, argc, argv, "ATM_GPFILE", -1, "-", ctl->atm_gpfile);
  ctl->atm_dt_out =
    scan_ctl(filename, argc, argv, "ATM_DT_OUT", -1, "86400", NULL);
  ctl->atm_filter =
    (int) scan_ctl(filename, argc, argv, "ATM_FILTER", -1, "0", NULL);
  ctl->atm_stride =
    (int) scan_ctl(filename, argc, argv, "ATM_STRIDE", -1, "1", NULL);
  ctl->atm_type =
    (int) scan_ctl(filename, argc, argv, "ATM_TYPE", -1, "0", NULL);

  /* Output of CSI data... */
  scan_ctl(filename, argc, argv, "CSI_BASENAME", -1, "-", ctl->csi_basename);
  scan_ctl(filename, argc, argv, "CSI_KERNEL", -1, "-", ctl->csi_kernel);
  ctl->csi_dt_out =
    scan_ctl(filename, argc, argv, "CSI_DT_OUT", -1, "86400", NULL);
  scan_ctl(filename, argc, argv, "CSI_OBSFILE", -1, "-", ctl->csi_obsfile);
  ctl->csi_obsmin =
    scan_ctl(filename, argc, argv, "CSI_OBSMIN", -1, "0", NULL);
  ctl->csi_modmin =
    scan_ctl(filename, argc, argv, "CSI_MODMIN", -1, "0", NULL);
  ctl->csi_z0 = scan_ctl(filename, argc, argv, "CSI_Z0", -1, "0", NULL);
  ctl->csi_z1 = scan_ctl(filename, argc, argv, "CSI_Z1", -1, "100", NULL);
  ctl->csi_nz = (int) scan_ctl(filename, argc, argv, "CSI_NZ", -1, "1", NULL);
  ctl->csi_lon0 =
    scan_ctl(filename, argc, argv, "CSI_LON0", -1, "-180", NULL);
  ctl->csi_lon1 = scan_ctl(filename, argc, argv, "CSI_LON1", -1, "180", NULL);
  ctl->csi_nx =
    (int) scan_ctl(filename, argc, argv, "CSI_NX", -1, "360", NULL);
  ctl->csi_lat0 = scan_ctl(filename, argc, argv, "CSI_LAT0", -1, "-90", NULL);
  ctl->csi_lat1 = scan_ctl(filename, argc, argv, "CSI_LAT1", -1, "90", NULL);
  ctl->csi_ny =
    (int) scan_ctl(filename, argc, argv, "CSI_NY", -1, "180", NULL);

  /* Output of ensemble data... */
  scan_ctl(filename, argc, argv, "ENS_BASENAME", -1, "-", ctl->ens_basename);
  ctl->ens_dt_out =
    scan_ctl(filename, argc, argv, "ENS_DT_OUT", -1, "86400", NULL);

  /* Output of grid data... */
  scan_ctl(filename, argc, argv, "GRID_BASENAME", -1, "-",
	   ctl->grid_basename);
  scan_ctl(filename, argc, argv, "GRID_KERNEL", -1, "-", ctl->grid_kernel);
  scan_ctl(filename, argc, argv, "GRID_GPFILE", -1, "-", ctl->grid_gpfile);
  ctl->grid_dt_out =
    scan_ctl(filename, argc, argv, "GRID_DT_OUT", -1, "86400", NULL);
  ctl->grid_sparse =
    (int) scan_ctl(filename, argc, argv, "GRID_SPARSE", -1, "0", NULL);
  ctl->grid_z0 = scan_ctl(filename, argc, argv, "GRID_Z0", -1, "0", NULL);
  ctl->grid_z1 = scan_ctl(filename, argc, argv, "GRID_Z1", -1, "100", NULL);
  ctl->grid_nz =
    (int) scan_ctl(filename, argc, argv, "GRID_NZ", -1, "1", NULL);
  ctl->grid_lon0 =
    scan_ctl(filename, argc, argv, "GRID_LON0", -1, "-180", NULL);
  ctl->grid_lon1 =
    scan_ctl(filename, argc, argv, "GRID_LON1", -1, "180", NULL);
  ctl->grid_nx =
    (int) scan_ctl(filename, argc, argv, "GRID_NX", -1, "360", NULL);
  ctl->grid_lat0 =
    scan_ctl(filename, argc, argv, "GRID_LAT0", -1, "-90", NULL);
  ctl->grid_lat1 =
    scan_ctl(filename, argc, argv, "GRID_LAT1", -1, "90", NULL);
  ctl->grid_ny =
    (int) scan_ctl(filename, argc, argv, "GRID_NY", -1, "180", NULL);
  ctl->grid_type =
    (int) scan_ctl(filename, argc, argv, "GRID_TYPE", -1, "0", NULL);

  /* Output of profile data... */
  scan_ctl(filename, argc, argv, "PROF_BASENAME", -1, "-",
	   ctl->prof_basename);
  scan_ctl(filename, argc, argv, "PROF_OBSFILE", -1, "-", ctl->prof_obsfile);
  ctl->prof_z0 = scan_ctl(filename, argc, argv, "PROF_Z0", -1, "0", NULL);
  ctl->prof_z1 = scan_ctl(filename, argc, argv, "PROF_Z1", -1, "60", NULL);
  ctl->prof_nz =
    (int) scan_ctl(filename, argc, argv, "PROF_NZ", -1, "60", NULL);
  ctl->prof_lon0 =
    scan_ctl(filename, argc, argv, "PROF_LON0", -1, "-180", NULL);
  ctl->prof_lon1 =
    scan_ctl(filename, argc, argv, "PROF_LON1", -1, "180", NULL);
  ctl->prof_nx =
    (int) scan_ctl(filename, argc, argv, "PROF_NX", -1, "360", NULL);
  ctl->prof_lat0 =
    scan_ctl(filename, argc, argv, "PROF_LAT0", -1, "-90", NULL);
  ctl->prof_lat1 =
    scan_ctl(filename, argc, argv, "PROF_LAT1", -1, "90", NULL);
  ctl->prof_ny =
    (int) scan_ctl(filename, argc, argv, "PROF_NY", -1, "180", NULL);

  /* Output of sample data... */
  scan_ctl(filename, argc, argv, "SAMPLE_BASENAME", -1, "-",
	   ctl->sample_basename);
  scan_ctl(filename, argc, argv, "SAMPLE_KERNEL", -1, "-",
	   ctl->sample_kernel);
  scan_ctl(filename, argc, argv, "SAMPLE_OBSFILE", -1, "-",
	   ctl->sample_obsfile);
  ctl->sample_dx =
    scan_ctl(filename, argc, argv, "SAMPLE_DX", -1, "50", NULL);
  ctl->sample_dz =
    scan_ctl(filename, argc, argv, "SAMPLE_DZ", -1, "-999", NULL);

  /* Output of station data... */
  scan_ctl(filename, argc, argv, "STAT_BASENAME", -1, "-",
	   ctl->stat_basename);
  ctl->stat_lon = scan_ctl(filename, argc, argv, "STAT_LON", -1, "0", NULL);
  ctl->stat_lat = scan_ctl(filename, argc, argv, "STAT_LAT", -1, "0", NULL);
  ctl->stat_r = scan_ctl(filename, argc, argv, "STAT_R", -1, "50", NULL);
  ctl->stat_t0 =
    scan_ctl(filename, argc, argv, "STAT_T0", -1, "-1e100", NULL);
  ctl->stat_t1 = scan_ctl(filename, argc, argv, "STAT_T1", -1, "1e100", NULL);
}

/*****************************************************************************/

void read_kernel(
  char *filename,
  double kz[EP],
  double kw[EP],
  int *nk) {

  /* Write info... */
  LOG(1, "Read kernel function: %s", filename);

  /* Open file... */
  FILE *in;
  if (!(in = fopen(filename, "r")))
    ERRMSG("Cannot open file!");

  /* Read data... */
  char line[LEN];
  int n = 0;
  while (fgets(line, LEN, in))
    if (sscanf(line, "%lg %lg", &kz[n], &kw[n]) == 2) {
      if (n > 0 && kz[n] < kz[n - 1])
	ERRMSG("Height levels must be ascending!");
      if ((++n) >= EP)
	ERRMSG("Too many height levels!");
    }

  /* Close file... */
  fclose(in);

  /* Check number of data points... */
  *nk = n;
  if (n < 2)
    ERRMSG("Not enough height levels!");

  /* Normalize kernel function... */
  double kmax = gsl_stats_max(kw, 1, (size_t) n);
  for (int iz = 0; iz < n; iz++)
    kw[iz] /= kmax;
}

/*****************************************************************************/

int read_met(
  char *filename,
  ctl_t * ctl,
  clim_t * clim,
  met_t * met) {

  /* Write info... */
  LOG(1, "Read meteo data: %s", filename);

  /* Read netCDF data... */
  if (ctl->met_type == 0) {

    int ncid;

    /* Open netCDF file... */
    if (nc__open(filename, ctl->read_mode, &ctl->chunkszhint, &ncid) !=
	NC_NOERR) {
      WARN("Cannot open file!");
      return 0;
    }

    /* Read coordinates of meteo data... */
    read_met_grid(filename, ncid, ctl, met);

    /* Read meteo data on vertical levels... */
    read_met_levels(ncid, ctl, met);

    /* Extrapolate data for lower boundary... */
    read_met_extrapolate(met);

    /* Read surface data... */
    read_met_surface(ncid, met, ctl);

    /* Create periodic boundary conditions... */
    read_met_periodic(met);

    /* Downsampling... */
    read_met_sample(ctl, met);

    /* Calculate geopotential heights... */
    read_met_geopot(ctl, met);

    /* Calculate potential vorticity... */
    read_met_pv(met);

    /* Calculate boundary layer data... */
    read_met_pbl(met);

    /* Calculate tropopause data... */
    read_met_tropo(ctl, clim, met);

    /* Calculate cloud properties... */
    read_met_cloud(ctl, met);

    /* Calculate convective available potential energy... */
    read_met_cape(clim, met);

    /* Detrending... */
    read_met_detrend(ctl, met);

    /* Close file... */
    NC(nc_close(ncid));
  }

  /* Read binary data... */
  else if (ctl->met_type >= 1 && ctl->met_type <= 4) {

    FILE *in;

    double r;

    int year, mon, day, hour, min, sec;

    /* Set timer... */
    SELECT_TIMER("READ_MET_BIN", "INPUT", NVTX_READ);

    /* Open file... */
    if (!(in = fopen(filename, "r"))) {
      WARN("Cannot open file!");
      return 0;
    }

    /* Check type of binary data... */
    int met_type;
    FREAD(&met_type, int,
	  1,
	  in);
    if (met_type != ctl->met_type)
      ERRMSG("Wrong MET_TYPE of binary data!");

    /* Check version of binary data... */
    int version;
    FREAD(&version, int,
	  1,
	  in);
    if (version != 100 && version != 101)
      ERRMSG("Wrong version of binary data!");

    /* Read time... */
    FREAD(&met->time, double,
	  1,
	  in);
    jsec2time(met->time, &year, &mon, &day, &hour, &min, &sec, &r);
    LOG(2, "Time: %.2f (%d-%02d-%02d, %02d:%02d UTC)",
	met->time, year, mon, day, hour, min);
    if (year < 1900 || year > 2100 || mon < 1 || mon > 12
	|| day < 1 || day > 31 || hour < 0 || hour > 23)
      ERRMSG("Error while reading time!");

    /* Read dimensions... */
    FREAD(&met->nx, int,
	  1,
	  in);
    LOG(2, "Number of longitudes: %d", met->nx);
    if (met->nx < 2 || met->nx > EX)
      ERRMSG("Number of longitudes out of range!");

    FREAD(&met->ny, int,
	  1,
	  in);
    LOG(2, "Number of latitudes: %d", met->ny);
    if (met->ny < 2 || met->ny > EY)
      ERRMSG("Number of latitudes out of range!");

    FREAD(&met->np, int,
	  1,
	  in);
    LOG(2, "Number of levels: %d", met->np);
    if (met->np < 2 || met->np > EP)
      ERRMSG("Number of levels out of range!");

    /* Read grid... */
    FREAD(met->lon, double,
	    (size_t) met->nx,
	  in);
    LOG(2, "Longitudes: %g, %g ... %g deg",
	met->lon[0], met->lon[1], met->lon[met->nx - 1]);

    FREAD(met->lat, double,
	    (size_t) met->ny,
	  in);
    LOG(2, "Latitudes: %g, %g ... %g deg",
	met->lat[0], met->lat[1], met->lat[met->ny - 1]);

    FREAD(met->p, double,
	    (size_t) met->np,
	  in);
    LOG(2, "Altitude levels: %g, %g ... %g km",
	Z(met->p[0]), Z(met->p[1]), Z(met->p[met->np - 1]));
    LOG(2, "Pressure levels: %g, %g ... %g hPa",
	met->p[0], met->p[1], met->p[met->np - 1]);

    /* Read surface data... */
    read_met_bin_2d(in, met, met->ps, "PS");
    read_met_bin_2d(in, met, met->ts, "TS");
    read_met_bin_2d(in, met, met->zs, "ZS");
    read_met_bin_2d(in, met, met->us, "US");
    read_met_bin_2d(in, met, met->vs, "VS");
    if (version == 101) {
      read_met_bin_2d(in, met, met->lsm, "LSM");
      read_met_bin_2d(in, met, met->sst, "SST");
    }
    read_met_bin_2d(in, met, met->pbl, "PBL");
    read_met_bin_2d(in, met, met->pt, "PT");
    read_met_bin_2d(in, met, met->tt, "TT");
    read_met_bin_2d(in, met, met->zt, "ZT");
    read_met_bin_2d(in, met, met->h2ot, "H2OT");
    read_met_bin_2d(in, met, met->pct, "PCT");
    read_met_bin_2d(in, met, met->pcb, "PCB");
    read_met_bin_2d(in, met, met->cl, "CL");
    read_met_bin_2d(in, met, met->plcl, "PLCL");
    read_met_bin_2d(in, met, met->plfc, "PLFC");
    read_met_bin_2d(in, met, met->pel, "PEL");
    read_met_bin_2d(in, met, met->cape, "CAPE");
    read_met_bin_2d(in, met, met->cin, "CIN");

    /* Read level data... */
    read_met_bin_3d(in, ctl, met, met->z, "Z", 0, 0.5);
    read_met_bin_3d(in, ctl, met, met->t, "T", 0, 5.0);
    read_met_bin_3d(in, ctl, met, met->u, "U", 8, 0);
    read_met_bin_3d(in, ctl, met, met->v, "V", 8, 0);
    read_met_bin_3d(in, ctl, met, met->w, "W", 8, 0);
    read_met_bin_3d(in, ctl, met, met->pv, "PV", 8, 0);
    read_met_bin_3d(in, ctl, met, met->h2o, "H2O", 8, 0);
    read_met_bin_3d(in, ctl, met, met->o3, "O3", 8, 0);
    read_met_bin_3d(in, ctl, met, met->lwc, "LWC", 8, 0);
    read_met_bin_3d(in, ctl, met, met->iwc, "IWC", 8, 0);
    if (version == 101)
      read_met_bin_3d(in, ctl, met, met->cc, "CC", 8, 0);

    /* Read final flag... */
    int final;
    FREAD(&final, int,
	  1,
	  in);
    if (final != 999)
      ERRMSG("Error while reading binary data!");

    /* Close file... */
    fclose(in);
  }

  /* Not implemented... */
  else
    ERRMSG("MET_TYPE not implemented!");

  /* Copy wind data to cache... */
#ifdef UVW
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      for (int ip = 0; ip < met->np; ip++) {
	met->uvw[ix][iy][ip][0] = met->u[ix][iy][ip];
	met->uvw[ix][iy][ip][1] = met->v[ix][iy][ip];
	met->uvw[ix][iy][ip][2] = met->w[ix][iy][ip];
      }
#endif

  /* Return success... */
  return 1;
}

/*****************************************************************************/

void read_met_bin_2d(
  FILE * in,
  met_t * met,
  float var[EX][EY],
  char *varname) {

  float *help;

  /* Allocate... */
  ALLOC(help, float,
	EX * EY);

  /* Read uncompressed... */
  LOG(2, "Read 2-D variable: %s (uncompressed)", varname);
  FREAD(help, float,
	  (size_t) (met->nx * met->ny),
	in);

  /* Copy data... */
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      var[ix][iy] = help[ARRAY_2D(ix, iy, met->ny)];

  /* Free... */
  free(help);
}

/*****************************************************************************/

void read_met_bin_3d(
  FILE * in,
  ctl_t * ctl,
  met_t * met,
  float var[EX][EY][EP],
  char *varname,
  int precision,
  double tolerance) {

  float *help;

  /* Allocate... */
  ALLOC(help, float,
	EX * EY * EP);

  /* Read uncompressed data... */
  if (ctl->met_type == 1) {
    LOG(2, "Read 3-D variable: %s (uncompressed)", varname);
    FREAD(help, float,
	    (size_t) (met->nx * met->ny * met->np),
	  in);
  }

  /* Read packed data... */
  else if (ctl->met_type == 2)
    compress_pack(varname, help, (size_t) (met->ny * met->nx),
		  (size_t) met->np, 1, in);

  /* Read zfp data... */
  else if (ctl->met_type == 3) {
#ifdef ZFP
    compress_zfp(varname, help, met->np, met->ny, met->nx, precision,
		 tolerance, 1, in);
#else
    ERRMSG("zfp compression not supported!");
    LOG(3, "%d %g", precision, tolerance);
#endif
  }

  /* Read zstd data... */
  else if (ctl->met_type == 4) {
#ifdef ZSTD
    compress_zstd(varname, help, (size_t) (met->np * met->ny * met->nx), 1,
		  in);
#else
    ERRMSG("zstd compression not supported!");
#endif
  }

  /* Copy data... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      for (int ip = 0; ip < met->np; ip++)
	var[ix][iy][ip] = help[ARRAY_3D(ix, iy, met->ny, ip, met->np)];

  /* Free... */
  free(help);
}

/*****************************************************************************/

void read_met_cape(
  clim_t * clim,
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_CAPE", "METPROC", NVTX_READ);
  LOG(2, "Calculate CAPE...");

  /* Vertical spacing (about 100 m)... */
  const double pfac = 1.01439, dz0 = RI / MA / G0 * log(pfac);

  /* Loop over columns... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Get potential temperature and water vapor vmr at lowest 50 hPa... */
      int n = 0;
      double h2o = 0, t, theta = 0;
      double pbot = GSL_MIN(met->ps[ix][iy], met->p[0]);
      double ptop = pbot - 50.;
      for (int ip = 0; ip < met->np; ip++) {
	if (met->p[ip] <= pbot) {
	  theta += THETA(met->p[ip], met->t[ix][iy][ip]);
	  h2o += met->h2o[ix][iy][ip];
	  n++;
	}
	if (met->p[ip] < ptop && n > 0)
	  break;
      }
      theta /= n;
      h2o /= n;

      /* Cannot compute anything if water vapor is missing... */
      met->plcl[ix][iy] = GSL_NAN;
      met->plfc[ix][iy] = GSL_NAN;
      met->pel[ix][iy] = GSL_NAN;
      met->cape[ix][iy] = GSL_NAN;
      met->cin[ix][iy] = GSL_NAN;
      if (h2o <= 0)
	continue;

      /* Find lifted condensation level (LCL)... */
      ptop = P(20.);
      pbot = met->ps[ix][iy];
      do {
	met->plcl[ix][iy] = (float) (0.5 * (pbot + ptop));
	t = theta / pow(1000. / met->plcl[ix][iy], 0.286);
	if (RH(met->plcl[ix][iy], t, h2o) > 100.)
	  ptop = met->plcl[ix][iy];
	else
	  pbot = met->plcl[ix][iy];
      } while (pbot - ptop > 0.1);

      /* Calculate CIN up to LCL... */
      INTPOL_INIT;
      double dcape, dz, h2o_env, t_env;
      double p = met->ps[ix][iy];
      met->cape[ix][iy] = met->cin[ix][iy] = 0;
      do {
	dz = dz0 * TVIRT(t, h2o);
	p /= pfac;
	t = theta / pow(1000. / p, 0.286);
	intpol_met_space_3d(met, met->t, p, met->lon[ix], met->lat[iy],
			    &t_env, ci, cw, 1);
	intpol_met_space_3d(met, met->h2o, p, met->lon[ix], met->lat[iy],
			    &h2o_env, ci, cw, 0);
	dcape = 1e3 * G0 * (TVIRT(t, h2o) - TVIRT(t_env, h2o_env)) /
	  TVIRT(t_env, h2o_env) * dz;
	if (dcape < 0)
	  met->cin[ix][iy] += fabsf((float) dcape);
      } while (p > met->plcl[ix][iy]);

      /* Calculate level of free convection (LFC), equilibrium level (EL),
         and convective available potential energy (CAPE)... */
      dcape = 0;
      p = met->plcl[ix][iy];
      t = theta / pow(1000. / p, 0.286);
      ptop = 0.75 * clim_tropo(clim, met->time, met->lat[iy]);
      do {
	dz = dz0 * TVIRT(t, h2o);
	p /= pfac;
	t -= lapse_rate(t, h2o) * dz;
	double psat = PSAT(t);
	h2o = psat / (p - (1. - EPS) * psat);
	intpol_met_space_3d(met, met->t, p, met->lon[ix], met->lat[iy],
			    &t_env, ci, cw, 1);
	intpol_met_space_3d(met, met->h2o, p, met->lon[ix], met->lat[iy],
			    &h2o_env, ci, cw, 0);
	double dcape_old = dcape;
	dcape = 1e3 * G0 * (TVIRT(t, h2o) - TVIRT(t_env, h2o_env)) /
	  TVIRT(t_env, h2o_env) * dz;
	if (dcape > 0) {
	  met->cape[ix][iy] += (float) dcape;
	  if (!isfinite(met->plfc[ix][iy]))
	    met->plfc[ix][iy] = (float) p;
	} else if (dcape_old > 0)
	  met->pel[ix][iy] = (float) p;
	if (dcape < 0 && !isfinite(met->plfc[ix][iy]))
	  met->cin[ix][iy] += fabsf((float) dcape);
      } while (p > ptop);

      /* Check results... */
      if (!isfinite(met->plfc[ix][iy]))
	met->cin[ix][iy] = GSL_NAN;
    }
}

/*****************************************************************************/

void read_met_cloud(
  ctl_t * ctl,
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_CLOUD", "METPROC", NVTX_READ);
  LOG(2, "Calculate cloud data...");

  /* Loop over columns... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Init... */
      met->pct[ix][iy] = GSL_NAN;
      met->pcb[ix][iy] = GSL_NAN;
      met->cl[ix][iy] = 0;

      /* Loop over pressure levels... */
      for (int ip = 0; ip < met->np - 1; ip++) {

	/* Check pressure... */
	if (met->p[ip] > met->ps[ix][iy] || met->p[ip] < P(20.))
	  continue;

	/* Check ice water and liquid water content... */
	if (met->iwc[ix][iy][ip] > ctl->met_cloud_min
	    || met->lwc[ix][iy][ip] > ctl->met_cloud_min) {

	  /* Get cloud top pressure ... */
	  met->pct[ix][iy]
	    = (float) (0.5 * (met->p[ip] + (float) met->p[ip + 1]));

	  /* Get cloud bottom pressure ... */
	  if (!isfinite(met->pcb[ix][iy]))
	    met->pcb[ix][iy]
	      = (float) (0.5 * (met->p[ip] + met->p[GSL_MAX(ip - 1, 0)]));
	}

	/* Get cloud water... */
	met->cl[ix][iy] += (float)
	  (0.5 * (met->iwc[ix][iy][ip] + met->iwc[ix][iy][ip + 1]
		  + met->lwc[ix][iy][ip] + met->lwc[ix][iy][ip + 1])
	   * 100. * (met->p[ip] - met->p[ip + 1]) / G0);
      }
    }
}

/*****************************************************************************/

void read_met_detrend(
  ctl_t * ctl,
  met_t * met) {

  met_t *help;

  /* Check parameters... */
  if (ctl->met_detrend <= 0)
    return;

  /* Set timer... */
  SELECT_TIMER("READ_MET_DETREND", "METPROC", NVTX_READ);
  LOG(2, "Detrend meteo data...");

  /* Allocate... */
  ALLOC(help, met_t, 1);

  /* Calculate standard deviation... */
  double sigma = ctl->met_detrend / 2.355;
  double tssq = 2. * SQR(sigma);

  /* Calculate box size in latitude... */
  int sy = (int) (3. * DY2DEG(sigma) / fabs(met->lat[1] - met->lat[0]));
  sy = GSL_MIN(GSL_MAX(1, sy), met->ny / 2);

  /* Calculate background... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++) {
    for (int iy = 0; iy < met->ny; iy++) {

      /* Calculate Cartesian coordinates... */
      double x0[3];
      geo2cart(0.0, met->lon[ix], met->lat[iy], x0);

      /* Calculate box size in longitude... */
      int sx =
	(int) (3. * DX2DEG(sigma, met->lat[iy]) /
	       fabs(met->lon[1] - met->lon[0]));
      sx = GSL_MIN(GSL_MAX(1, sx), met->nx / 2);

      /* Init... */
      float wsum = 0;
      for (int ip = 0; ip < met->np; ip++) {
	help->t[ix][iy][ip] = 0;
	help->u[ix][iy][ip] = 0;
	help->v[ix][iy][ip] = 0;
	help->w[ix][iy][ip] = 0;
      }

      /* Loop over neighboring grid points... */
      for (int ix2 = ix - sx; ix2 <= ix + sx; ix2++) {
	int ix3 = ix2;
	if (ix3 < 0)
	  ix3 += met->nx;
	else if (ix3 >= met->nx)
	  ix3 -= met->nx;
	for (int iy2 = GSL_MAX(iy - sy, 0);
	     iy2 <= GSL_MIN(iy + sy, met->ny - 1); iy2++) {

	  /* Calculate Cartesian coordinates... */
	  double x1[3];
	  geo2cart(0.0, met->lon[ix3], met->lat[iy2], x1);

	  /* Calculate weighting factor... */
	  float w = (float) exp(-DIST2(x0, x1) / tssq);

	  /* Add data... */
	  wsum += w;
	  for (int ip = 0; ip < met->np; ip++) {
	    help->t[ix][iy][ip] += w * met->t[ix3][iy2][ip];
	    help->u[ix][iy][ip] += w * met->u[ix3][iy2][ip];
	    help->v[ix][iy][ip] += w * met->v[ix3][iy2][ip];
	    help->w[ix][iy][ip] += w * met->w[ix3][iy2][ip];
	  }
	}
      }

      /* Normalize... */
      for (int ip = 0; ip < met->np; ip++) {
	help->t[ix][iy][ip] /= wsum;
	help->u[ix][iy][ip] /= wsum;
	help->v[ix][iy][ip] /= wsum;
	help->w[ix][iy][ip] /= wsum;
      }
    }
  }

  /* Subtract background... */
#pragma omp parallel for default(shared) collapse(3)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      for (int ip = 0; ip < met->np; ip++) {
	met->t[ix][iy][ip] -= help->t[ix][iy][ip];
	met->u[ix][iy][ip] -= help->u[ix][iy][ip];
	met->v[ix][iy][ip] -= help->v[ix][iy][ip];
	met->w[ix][iy][ip] -= help->w[ix][iy][ip];
      }

  /* Free... */
  free(help);
}

/*****************************************************************************/

void read_met_extrapolate(
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_EXTRAPOLATE", "METPROC", NVTX_READ);
  LOG(2, "Extrapolate meteo data...");

  /* Loop over columns... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Find lowest valid data point... */
      int ip0;
      for (ip0 = met->np - 1; ip0 >= 0; ip0--)
	if (!isfinite(met->t[ix][iy][ip0])
	    || !isfinite(met->u[ix][iy][ip0])
	    || !isfinite(met->v[ix][iy][ip0])
	    || !isfinite(met->w[ix][iy][ip0]))
	  break;

      /* Extrapolate... */
      for (int ip = ip0; ip >= 0; ip--) {
	met->t[ix][iy][ip] = met->t[ix][iy][ip + 1];
	met->u[ix][iy][ip] = met->u[ix][iy][ip + 1];
	met->v[ix][iy][ip] = met->v[ix][iy][ip + 1];
	met->w[ix][iy][ip] = met->w[ix][iy][ip + 1];
	met->h2o[ix][iy][ip] = met->h2o[ix][iy][ip + 1];
	met->o3[ix][iy][ip] = met->o3[ix][iy][ip + 1];
	met->lwc[ix][iy][ip] = met->lwc[ix][iy][ip + 1];
	met->iwc[ix][iy][ip] = met->iwc[ix][iy][ip + 1];
	met->cc[ix][iy][ip] = met->cc[ix][iy][ip + 1];
      }
    }
}

/*****************************************************************************/

void read_met_geopot(
  ctl_t * ctl,
  met_t * met) {

  static float help[EP][EX][EY];

  double logp[EP];

  int dx = ctl->met_geopot_sx, dy = ctl->met_geopot_sy;

  /* Set timer... */
  SELECT_TIMER("READ_MET_GEOPOT", "METPROC", NVTX_READ);
  LOG(2, "Calculate geopotential heights...");

  /* Calculate log pressure... */
#pragma omp parallel for default(shared)
  for (int ip = 0; ip < met->np; ip++)
    logp[ip] = log(met->p[ip]);

  /* Apply hydrostatic equation to calculate geopotential heights... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Get surface height and pressure... */
      double zs = met->zs[ix][iy];
      double lnps = log(met->ps[ix][iy]);

      /* Get temperature and water vapor vmr at the surface... */
      int ip0 = locate_irr(met->p, met->np, met->ps[ix][iy]);
      double ts = LIN(met->p[ip0], met->t[ix][iy][ip0], met->p[ip0 + 1],
		      met->t[ix][iy][ip0 + 1], met->ps[ix][iy]);
      double h2os = LIN(met->p[ip0], met->h2o[ix][iy][ip0], met->p[ip0 + 1],
			met->h2o[ix][iy][ip0 + 1], met->ps[ix][iy]);

      /* Upper part of profile... */
      met->z[ix][iy][ip0 + 1]
	= (float) (zs +
		   ZDIFF(lnps, ts, h2os, logp[ip0 + 1],
			 met->t[ix][iy][ip0 + 1], met->h2o[ix][iy][ip0 + 1]));
      for (int ip = ip0 + 2; ip < met->np; ip++)
	met->z[ix][iy][ip]
	  = (float) (met->z[ix][iy][ip - 1] +
		     ZDIFF(logp[ip - 1], met->t[ix][iy][ip - 1],
			   met->h2o[ix][iy][ip - 1], logp[ip],
			   met->t[ix][iy][ip], met->h2o[ix][iy][ip]));

      /* Lower part of profile... */
      met->z[ix][iy][ip0]
	= (float) (zs +
		   ZDIFF(lnps, ts, h2os, logp[ip0],
			 met->t[ix][iy][ip0], met->h2o[ix][iy][ip0]));
      for (int ip = ip0 - 1; ip >= 0; ip--)
	met->z[ix][iy][ip]
	  = (float) (met->z[ix][iy][ip + 1] +
		     ZDIFF(logp[ip + 1], met->t[ix][iy][ip + 1],
			   met->h2o[ix][iy][ip + 1], logp[ip],
			   met->t[ix][iy][ip], met->h2o[ix][iy][ip]));
    }

  /* Check control parameters... */
  if (dx == 0 || dy == 0)
    return;

  /* Default smoothing parameters... */
  if (dx < 0 || dy < 0) {
    if (fabs(met->lon[1] - met->lon[0]) < 0.5) {
      dx = 3;
      dy = 2;
    } else {
      dx = 6;
      dy = 4;
    }
  }

  /* Calculate weights for smoothing... */
  float ws[dx + 1][dy + 1];
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix <= dx; ix++)
    for (int iy = 0; iy < dy; iy++)
      ws[ix][iy] = (1.0f - (float) ix / (float) dx)
	* (1.0f - (float) iy / (float) dy);

  /* Copy data... */
#pragma omp parallel for default(shared) collapse(3)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      for (int ip = 0; ip < met->np; ip++)
	help[ip][ix][iy] = met->z[ix][iy][ip];

  /* Horizontal smoothing... */
#pragma omp parallel for default(shared) collapse(3)
  for (int ip = 0; ip < met->np; ip++)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {
	float res = 0, wsum = 0;
	int iy0 = GSL_MAX(iy - dy + 1, 0);
	int iy1 = GSL_MIN(iy + dy - 1, met->ny - 1);
	for (int ix2 = ix - dx + 1; ix2 <= ix + dx - 1; ++ix2) {
	  int ix3 = ix2;
	  if (ix3 < 0)
	    ix3 += met->nx;
	  else if (ix3 >= met->nx)
	    ix3 -= met->nx;
	  for (int iy2 = iy0; iy2 <= iy1; ++iy2)
	    if (isfinite(help[ip][ix3][iy2])) {
	      float w = ws[abs(ix - ix2)][abs(iy - iy2)];
	      res += w * help[ip][ix3][iy2];
	      wsum += w;
	    }
	}
	if (wsum > 0)
	  met->z[ix][iy][ip] = res / wsum;
	else
	  met->z[ix][iy][ip] = GSL_NAN;
      }
}

/*****************************************************************************/

void read_met_grid(
  char *filename,
  int ncid,
  ctl_t * ctl,
  met_t * met) {

  char levname[LEN], tstr[10];

  double rtime, r, r2;

  int varid, year2, mon2, day2, hour2, min2, sec2,
    year, mon, day, hour, min, sec;

  size_t np;

  /* Set timer... */
  SELECT_TIMER("READ_MET_GRID", "INPUT", NVTX_READ);
  LOG(2, "Read meteo grid information...");

  /* MPTRAC meteo files... */
  if (ctl->clams_met_data == 0) {

    /* Get time from filename... */
    met->time = time_from_filename(filename, 16);

    /* Check time information from data file... */
    jsec2time(met->time, &year, &mon, &day, &hour, &min, &sec, &r);
    if (nc_inq_varid(ncid, "time", &varid) == NC_NOERR) {
      NC(nc_get_var_double(ncid, varid, &rtime));
      if (fabs(year * 10000. + mon * 100. + day + hour / 24. - rtime) > 1.0)
	WARN("Time information in meteo file does not match filename!");
    } else
      WARN("Time information in meteo file is missing!");
  }

  /* CLaMS meteo files... */
  else {

    /* Read time from file... */
    NC_GET_DOUBLE("time", &rtime, 0);

    /* Get time from filename (considering the century)... */
    if (rtime < 0)
      sprintf(tstr, "19%.2s", &filename[strlen(filename) - 11]);
    else
      sprintf(tstr, "20%.2s", &filename[strlen(filename) - 11]);
    year = atoi(tstr);
    sprintf(tstr, "%.2s", &filename[strlen(filename) - 9]);
    mon = atoi(tstr);
    sprintf(tstr, "%.2s", &filename[strlen(filename) - 7]);
    day = atoi(tstr);
    sprintf(tstr, "%.2s", &filename[strlen(filename) - 5]);
    hour = atoi(tstr);
    time2jsec(year, mon, day, hour, 0, 0, 0, &met->time);
  }

  /* Check time... */
  if (year < 1900 || year > 2100 || mon < 1 || mon > 12
      || day < 1 || day > 31 || hour < 0 || hour > 23)
    ERRMSG("Cannot read time from filename!");
  jsec2time(met->time, &year2, &mon2, &day2, &hour2, &min2, &sec2, &r2);
  LOG(2, "Time: %.2f (%d-%02d-%02d, %02d:%02d UTC)",
      met->time, year2, mon2, day2, hour2, min2);

  /* Get grid dimensions... */
  NC_INQ_DIM("lon", &met->nx, 2, EX);
  LOG(2, "Number of longitudes: %d", met->nx);

  NC_INQ_DIM("lat", &met->ny, 2, EY);
  LOG(2, "Number of latitudes: %d", met->ny);

  if (ctl->vert_coord_met == 0) {
    int dimid;
    sprintf(levname, "lev");
    if (nc_inq_dimid(ncid, levname, &dimid) != NC_NOERR)
      sprintf(levname, "plev");
  } else
    sprintf(levname, "hybrid");
  NC_INQ_DIM(levname, &met->np, 1, EP);
  if (met->np == 1) {
    int dimid;
    sprintf(levname, "lev_2");
    if (nc_inq_dimid(ncid, levname, &dimid) != NC_NOERR) {
      sprintf(levname, "plev");
      NC(nc_inq_dimid(ncid, levname, &dimid));
    }
    NC(nc_inq_dimlen(ncid, dimid, &np));
    met->np = (int) np;
  }
  LOG(2, "Number of levels: %d", met->np);
  if (met->np < 2 || met->np > EP)
    ERRMSG("Number of levels out of range!");

  /* Read longitudes and latitudes... */
  NC_GET_DOUBLE("lon", met->lon, 1);
  LOG(2, "Longitudes: %g, %g ... %g deg",
      met->lon[0], met->lon[1], met->lon[met->nx - 1]);
  NC_GET_DOUBLE("lat", met->lat, 1);
  LOG(2, "Latitudes: %g, %g ... %g deg",
      met->lat[0], met->lat[1], met->lat[met->ny - 1]);

  /* Read pressure levels... */
  if (ctl->met_np <= 0) {
    NC_GET_DOUBLE(levname, met->p, 1);
    for (int ip = 0; ip < met->np; ip++)
      met->p[ip] /= 100.;
    LOG(2, "Altitude levels: %g, %g ... %g km",
	Z(met->p[0]), Z(met->p[1]), Z(met->p[met->np - 1]));
    LOG(2, "Pressure levels: %g, %g ... %g hPa",
	met->p[0], met->p[1], met->p[met->np - 1]);
  }
}

/*****************************************************************************/

void read_met_levels(
  int ncid,
  ctl_t * ctl,
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_LEVELS", "INPUT", NVTX_READ);
  LOG(2, "Read level data...");

  /* MPTRAC meteo data... */
  if (ctl->clams_met_data == 0) {

    /* Read meteo data... */
    if (!read_met_nc_3d(ncid, "t", "T", ctl, met, met->t, 1.0, 1))
      ERRMSG("Cannot read temperature!");
    if (!read_met_nc_3d(ncid, "u", "U", ctl, met, met->u, 1.0, 1))
      ERRMSG("Cannot read zonal wind!");
    if (!read_met_nc_3d(ncid, "v", "V", ctl, met, met->v, 1.0, 1))
      ERRMSG("Cannot read meridional wind!");
    if (!read_met_nc_3d(ncid, "w", "W", ctl, met, met->w, 0.01f, 1))
      WARN("Cannot read vertical velocity!");
    if (!read_met_nc_3d
	(ncid, "q", "Q", ctl, met, met->h2o, (float) (MA / MH2O), 1))
      WARN("Cannot read specific humidity!");
    if (ctl->met_relhum) {
      if (!read_met_nc_3d(ncid, "rh", "RH", ctl, met, met->h2o, 0.01f, 1))
	WARN("Cannot read relative humidity!");
#pragma omp parallel for default(shared) collapse(2)
      for (int ix = 0; ix < met->nx; ix++)
	for (int iy = 0; iy < met->ny; iy++)
	  for (int ip = 0; ip < met->np; ip++) {
	    double pw = met->h2o[ix][iy][ip] * PSAT(met->t[ix][iy][ip]);
	    met->h2o[ix][iy][ip] =
	      (float) (pw / (met->p[ip] - (1.0 - EPS) * pw));
	  }
    }
    if (!read_met_nc_3d
	(ncid, "o3", "O3", ctl, met, met->o3, (float) (MA / MO3), 1))
      WARN("Cannot read ozone data!");
    if (ctl->met_cloud == 1 || ctl->met_cloud == 3) {
      if (!read_met_nc_3d(ncid, "clwc", "CLWC", ctl, met, met->lwc, 1.0, 1))
	WARN("Cannot read cloud liquid water content!");
      if (!read_met_nc_3d(ncid, "ciwc", "CIWC", ctl, met, met->iwc, 1.0, 1))
	WARN("Cannot read cloud ice water content!");
    }
    if (ctl->met_cloud == 2 || ctl->met_cloud == 3) {
      if (!read_met_nc_3d
	  (ncid, "crwc", "CRWC", ctl, met, met->lwc, 1.0,
	   ctl->met_cloud == 2))
	WARN("Cannot read cloud rain water content!");
      if (!read_met_nc_3d
	  (ncid, "cswc", "CSWC", ctl, met, met->iwc, 1.0,
	   ctl->met_cloud == 2))
	WARN("Cannot read cloud snow water content!");
    }
    if (!read_met_nc_3d(ncid, "cc", "CC", ctl, met, met->cc, 1.0, 1))
      WARN("Cannot read cloud cover!");

    /* Transfer from model levels to pressure levels... */
    if (ctl->met_np > 0) {

      /* Read pressure on model levels... */
      if (!read_met_nc_3d(ncid, "pl", "PL", ctl, met, met->pl, 0.01f, 1))
	ERRMSG("Cannot read pressure on model levels!");

      /* Vertical interpolation from model to pressure levels... */
      read_met_ml2pl(ctl, met, met->t);
      read_met_ml2pl(ctl, met, met->u);
      read_met_ml2pl(ctl, met, met->v);
      read_met_ml2pl(ctl, met, met->w);
      read_met_ml2pl(ctl, met, met->h2o);
      read_met_ml2pl(ctl, met, met->o3);
      read_met_ml2pl(ctl, met, met->lwc);
      read_met_ml2pl(ctl, met, met->iwc);
      read_met_ml2pl(ctl, met, met->cc);

      /* Set new pressure levels... */
      met->np = ctl->met_np;
      for (int ip = 0; ip < met->np; ip++)
	met->p[ip] = ctl->met_p[ip];
    }

  }

  /* CLaMS meteo data... */
  else if (ctl->clams_met_data == 1) {

    /* Read meteorological data... */
    if (!read_met_nc_3d(ncid, "t", "TEMP", ctl, met, met->t, 1.0, 1))
      ERRMSG("Cannot read temperature!");
    if (!read_met_nc_3d(ncid, "u", "U", ctl, met, met->u, 1.0, 1))
      ERRMSG("Cannot read zonal wind!");
    if (!read_met_nc_3d(ncid, "v", "V", ctl, met, met->v, 1.0, 1))
      ERRMSG("Cannot read meridional wind!");
    if (!read_met_nc_3d(ncid, "W", "OMEGA", ctl, met, met->w, 0.01f, 1))
      WARN("Cannot read vertical velocity!");
    if (!read_met_nc_3d(ncid, "ZETA", "zeta", ctl, met, met->zeta, 1.0, 1))
      WARN("Cannot read ZETA in meteo data!");
    if (ctl->vert_vel == 1) {
      if (!read_met_nc_3d
	  (ncid, "ZETA_DOT_TOT", "zeta_dot_clr", ctl, met, met->zeta_dot,
	   0.00001157407f, 1)) {
	if (!read_met_nc_3d
	    (ncid, "ZETA_DOT_TOT", "ZETA_DOT_clr", ctl, met, met->zeta_dot,
	     0.00001157407f, 1)) {
	  WARN("Cannot read vertical velocity!");
	}
      }
    }
    if (!read_met_nc_3d
	(ncid, "sh", "SH", ctl, met, met->h2o, (float) (MA / MH2O), 1))
      WARN("Cannot read specific humidity!");
    if (!read_met_nc_3d
	(ncid, "o3", "O3", ctl, met, met->o3, (float) (MA / MO3), 1))
      WARN("Cannot read ozone data!");
    if (ctl->met_cloud == 1 || ctl->met_cloud == 3) {
      if (!read_met_nc_3d(ncid, "clwc", "CLWC", ctl, met, met->lwc, 1.0, 1))
	WARN("Cannot read cloud liquid water content!");
      if (!read_met_nc_3d(ncid, "ciwc", "CIWC", ctl, met, met->iwc, 1.0, 1))
	WARN("Cannot read cloud ice water content!");
    }
    if (ctl->met_cloud == 2 || ctl->met_cloud == 3) {
      if (!read_met_nc_3d
	  (ncid, "crwc", "CRWC", ctl, met, met->lwc, 1.0,
	   ctl->met_cloud == 2))
	WARN("Cannot read cloud rain water content!");
      if (!read_met_nc_3d
	  (ncid, "cswc", "CSWC", ctl, met, met->iwc, 1.0,
	   ctl->met_cloud == 2))
	WARN("Cannot read cloud snow water content!");
    }
    if (!read_met_nc_3d(ncid, "cc", "CC", ctl, met, met->cc, 1.0, 1))
      WARN("Cannot read cloud cover!");

    /* Transfer from model levels to pressure levels... */
    if (ctl->met_np > 0) {

      /* Read pressure on model levels... */
      if (!read_met_nc_3d(ncid, "pl", "PRESS", ctl, met, met->pl, 1.0, 1))
	ERRMSG("Cannot read pressure on model levels!");

      /* Vertical interpolation from model to pressure levels... */
      read_met_ml2pl(ctl, met, met->t);
      read_met_ml2pl(ctl, met, met->u);
      read_met_ml2pl(ctl, met, met->v);
      read_met_ml2pl(ctl, met, met->w);
      read_met_ml2pl(ctl, met, met->h2o);
      read_met_ml2pl(ctl, met, met->o3);
      read_met_ml2pl(ctl, met, met->lwc);
      read_met_ml2pl(ctl, met, met->iwc);
      read_met_ml2pl(ctl, met, met->cc);
      if (ctl->vert_vel == 1) {
	read_met_ml2pl(ctl, met, met->zeta);
	read_met_ml2pl(ctl, met, met->zeta_dot);
      }

      /* Set new pressure levels... */
      met->np = ctl->met_np;
      for (int ip = 0; ip < met->np; ip++)
	met->p[ip] = ctl->met_p[ip];

      /* Create a pressure field... */
      for (int i = 0; i < met->nx; i++)
	for (int j = 0; j < met->ny; j++)
	  for (int k = 0; k < met->np; k++) {
	    met->patp[i][j][k] = (float) met->p[k];
	  }
    }
  } else
    ERRMSG("Meteo data format unknown!");

  /* Check ordering of pressure levels... */
  for (int ip = 1; ip < met->np; ip++)
    if (met->p[ip - 1] < met->p[ip])
      ERRMSG("Pressure levels must be descending!");
}

/*****************************************************************************/

void read_met_ml2pl(
  ctl_t * ctl,
  met_t * met,
  float var[EX][EY][EP]) {

  double aux[EP], p[EP];

  /* Set timer... */
  SELECT_TIMER("READ_MET_ML2PL", "METPROC", NVTX_READ);
  LOG(2, "Interpolate meteo data to pressure levels...");

  /* Loop over columns... */
#pragma omp parallel for default(shared) private(aux,p) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Copy pressure profile... */
      for (int ip = 0; ip < met->np; ip++)
	p[ip] = met->pl[ix][iy][ip];

      /* Interpolate... */
      for (int ip = 0; ip < ctl->met_np; ip++) {
	double pt = ctl->met_p[ip];
	if ((pt > p[0] && p[0] > p[1]) || (pt < p[0] && p[0] < p[1]))
	  pt = p[0];
	else if ((pt > p[met->np - 1] && p[1] > p[0])
		 || (pt < p[met->np - 1] && p[1] < p[0]))
	  pt = p[met->np - 1];
	int ip2 = locate_irr(p, met->np, pt);
	aux[ip] = LIN(p[ip2], var[ix][iy][ip2],
		      p[ip2 + 1], var[ix][iy][ip2 + 1], pt);
      }

      /* Copy data... */
      for (int ip = 0; ip < ctl->met_np; ip++)
	var[ix][iy][ip] = (float) aux[ip];
    }
}

/*****************************************************************************/

int read_met_nc_2d(
  int ncid,
  char *varname,
  char *varname2,
  ctl_t * ctl,
  met_t * met,
  float dest[EX][EY],
  float scl,
  int init) {

  char varsel[LEN];

  float offset, scalfac;

  int varid;

  /* Check if variable exists... */
  if (nc_inq_varid(ncid, varname, &varid) != NC_NOERR)
    if (nc_inq_varid(ncid, varname2, &varid) != NC_NOERR) {
      WARN("Cannot read 2-D variable: %s or %s", varname, varname2);
      return 0;
    } else {
      sprintf(varsel, "%s", varname2);
  } else
    sprintf(varsel, "%s", varname);

  /* Read packed data... */
  if (ctl->met_nc_scale
      && nc_get_att_float(ncid, varid, "add_offset", &offset) == NC_NOERR
      && nc_get_att_float(ncid, varid, "scale_factor",
			  &scalfac) == NC_NOERR) {

    /* Allocate... */
    short *help;
    ALLOC(help, short,
	  EX * EY * EP);

    /* Read fill value and missing value... */
    short fillval, missval;
    if (nc_get_att_short(ncid, varid, "_FillValue", &fillval) != NC_NOERR)
      fillval = 0;
    if (nc_get_att_short(ncid, varid, "missing_value", &missval) != NC_NOERR)
      missval = 0;

    /* Write info... */
    LOG(2, "Read 2-D variable: %s"
	" (FILL = %d, MISS = %d, SCALE = %g, OFFSET = %g)",
	varsel, fillval, missval, scalfac, offset);

    /* Read data... */
    NC(nc_get_var_short(ncid, varid, help));

    /* Copy and check data... */
#pragma omp parallel for default(shared) num_threads(12)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {
	if (init)
	  dest[ix][iy] = 0;
	short aux = help[ARRAY_2D(iy, ix, met->nx)];
	if ((fillval == 0 || aux != fillval)
	    && (missval == 0 || aux != missval)
	    && fabsf(aux * scalfac + offset) < 1e14f)
	  dest[ix][iy] += scl * (aux * scalfac + offset);
	else
	  dest[ix][iy] = GSL_NAN;
      }

    /* Free... */
    free(help);
  }

  /* Unpacked data... */
  else {

    /* Allocate... */
    float *help;
    ALLOC(help, float,
	  EX * EY);

    /* Read fill value and missing value... */
    float fillval, missval;
    if (nc_get_att_float(ncid, varid, "_FillValue", &fillval) != NC_NOERR)
      fillval = 0;
    if (nc_get_att_float(ncid, varid, "missing_value", &missval) != NC_NOERR)
      missval = 0;

    /* Write info... */
    LOG(2, "Read 2-D variable: %s (FILL = %g, MISS = %g)",
	varsel, fillval, missval);

    /* Read data... */
    NC(nc_get_var_float(ncid, varid, help));

    /* Copy and check data... */
#pragma omp parallel for default(shared) num_threads(12)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {
	if (init)
	  dest[ix][iy] = 0;
	float aux = help[ARRAY_2D(iy, ix, met->nx)];
	if ((fillval == 0 || aux != fillval)
	    && (missval == 0 || aux != missval)
	    && fabsf(aux) < 1e14f)
	  dest[ix][iy] += scl * aux;
	else
	  dest[ix][iy] = GSL_NAN;
      }

    /* Free... */
    free(help);
  }

  /* Return... */
  return 1;
}

/*****************************************************************************/

int read_met_nc_3d(
  int ncid,
  char *varname,
  char *varname2,
  ctl_t * ctl,
  met_t * met,
  float dest[EX][EY][EP],
  float scl,
  int init) {

  char varsel[LEN];

  float offset, scalfac;

  int varid;

  /* Check if variable exists... */
  if (nc_inq_varid(ncid, varname, &varid) != NC_NOERR)
    if (nc_inq_varid(ncid, varname2, &varid) != NC_NOERR) {
      WARN("Cannot read 3-D variable: %s or %s", varname, varname2);
      return 0;
    } else {
      sprintf(varsel, "%s", varname2);
  } else
    sprintf(varsel, "%s", varname);

  /* Read packed data... */
  if (ctl->met_nc_scale
      && nc_get_att_float(ncid, varid, "add_offset", &offset) == NC_NOERR
      && nc_get_att_float(ncid, varid, "scale_factor",
			  &scalfac) == NC_NOERR) {

    /* Allocate... */
    short *help;
    ALLOC(help, short,
	  EX * EY * EP);

    /* Read fill value and missing value... */
    short fillval, missval;
    if (nc_get_att_short(ncid, varid, "_FillValue", &fillval) != NC_NOERR)
      fillval = 0;
    if (nc_get_att_short(ncid, varid, "missing_value", &missval) != NC_NOERR)
      missval = 0;

    /* Write info... */
    LOG(2, "Read 3-D variable: %s "
	"(FILL = %d, MISS = %d, SCALE = %g, OFFSET = %g)",
	varsel, fillval, missval, scalfac, offset);

    /* Read data... */
    NC(nc_get_var_short(ncid, varid, help));

    /* Copy and check data... */
#pragma omp parallel for default(shared) num_threads(12)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++)
	for (int ip = 0; ip < met->np; ip++) {
	  if (init)
	    dest[ix][iy][ip] = 0;
	  short aux = help[ARRAY_3D(ip, iy, met->ny, ix, met->nx)];
	  if ((fillval == 0 || aux != fillval)
	      && (missval == 0 || aux != missval)
	      && fabsf(aux * scalfac + offset) < 1e14f)
	    dest[ix][iy][ip] += scl * (aux * scalfac + offset);
	  else
	    dest[ix][iy][ip] = GSL_NAN;
	}

    /* Free... */
    free(help);
  }

  /* Unpacked data... */
  else {

    /* Allocate... */
    float *help;
    ALLOC(help, float,
	  EX * EY * EP);

    /* Read fill value and missing value... */
    float fillval, missval;
    if (nc_get_att_float(ncid, varid, "_FillValue", &fillval) != NC_NOERR)
      fillval = 0;
    if (nc_get_att_float(ncid, varid, "missing_value", &missval) != NC_NOERR)
      missval = 0;

    /* Write info... */
    LOG(2, "Read 3-D variable: %s (FILL = %g, MISS = %g)",
	varsel, fillval, missval);

    /* Read data... */
    NC(nc_get_var_float(ncid, varid, help));

    /* Copy and check data... */
#pragma omp parallel for default(shared) num_threads(12)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++)
	for (int ip = 0; ip < met->np; ip++) {
	  if (init)
	    dest[ix][iy][ip] = 0;
	  float aux = help[ARRAY_3D(ip, iy, met->ny, ix, met->nx)];
	  if ((fillval == 0 || aux != fillval)
	      && (missval == 0 || aux != missval)
	      && fabsf(aux) < 1e14f)
	    dest[ix][iy][ip] += scl * aux;
	  else
	    dest[ix][iy][ip] = GSL_NAN;
	}

    /* Free... */
    free(help);
  }

  /* Return... */
  return 1;
}

/*****************************************************************************/

void read_met_pbl(
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_PBL", "METPROC", NVTX_READ);
  LOG(2, "Calculate planetary boundary layer...");

  /* Parameters used to estimate the height of the PBL
     (e.g., Vogelezang and Holtslag, 1996; Seidel et al., 2012)... */
  const double rib_crit = 0.25, dz = 0.05, umin = 5.0;

  /* Loop over grid points... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {

      /* Set bottom level of PBL... */
      double pbl_bot = met->ps[ix][iy] + DZ2DP(dz, met->ps[ix][iy]);

      /* Find lowest level near the bottom... */
      int ip;
      for (ip = 1; ip < met->np; ip++)
	if (met->p[ip] < pbl_bot)
	  break;

      /* Get near surface data... */
      double zs = LIN(met->p[ip - 1], met->z[ix][iy][ip - 1],
		      met->p[ip], met->z[ix][iy][ip], pbl_bot);
      double ts = LIN(met->p[ip - 1], met->t[ix][iy][ip - 1],
		      met->p[ip], met->t[ix][iy][ip], pbl_bot);
      double us = LIN(met->p[ip - 1], met->u[ix][iy][ip - 1],
		      met->p[ip], met->u[ix][iy][ip], pbl_bot);
      double vs = LIN(met->p[ip - 1], met->v[ix][iy][ip - 1],
		      met->p[ip], met->v[ix][iy][ip], pbl_bot);
      double h2os = LIN(met->p[ip - 1], met->h2o[ix][iy][ip - 1],
			met->p[ip], met->h2o[ix][iy][ip], pbl_bot);
      double tvs = THETAVIRT(pbl_bot, ts, h2os);

      /* Init... */
      double rib_old = 0;

      /* Loop over levels... */
      for (; ip < met->np; ip++) {

	/* Get squared horizontal wind speed... */
	double vh2
	  = SQR(met->u[ix][iy][ip] - us) + SQR(met->v[ix][iy][ip] - vs);
	vh2 = GSL_MAX(vh2, SQR(umin));

	/* Calculate bulk Richardson number... */
	double rib = G0 * 1e3 * (met->z[ix][iy][ip] - zs) / tvs
	  * (THETAVIRT(met->p[ip], met->t[ix][iy][ip],
		       met->h2o[ix][iy][ip]) - tvs) / vh2;

	/* Check for critical value... */
	if (rib >= rib_crit) {
	  met->pbl[ix][iy] = (float) (LIN(rib_old, met->p[ip - 1],
					  rib, met->p[ip], rib_crit));
	  if (met->pbl[ix][iy] > pbl_bot)
	    met->pbl[ix][iy] = (float) pbl_bot;
	  break;
	}

	/* Save Richardson number... */
	rib_old = rib;
      }
    }
}

/*****************************************************************************/

void read_met_periodic(
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_PERIODIC", "METPROC", NVTX_READ);
  LOG(2, "Apply periodic boundary conditions...");

  /* Check longitudes... */
  if (!(fabs(met->lon[met->nx - 1] - met->lon[0]
	     + met->lon[1] - met->lon[0] - 360) < 0.01))
    return;

  /* Increase longitude counter... */
  if ((++met->nx) > EX)
    ERRMSG("Cannot create periodic boundary conditions!");

  /* Set longitude... */
  met->lon[met->nx - 1] = met->lon[met->nx - 2] + met->lon[1] - met->lon[0];

  /* Loop over latitudes and pressure levels... */
#pragma omp parallel for default(shared)
  for (int iy = 0; iy < met->ny; iy++) {
    met->ps[met->nx - 1][iy] = met->ps[0][iy];
    met->zs[met->nx - 1][iy] = met->zs[0][iy];
    met->ts[met->nx - 1][iy] = met->ts[0][iy];
    met->us[met->nx - 1][iy] = met->us[0][iy];
    met->vs[met->nx - 1][iy] = met->vs[0][iy];
    met->lsm[met->nx - 1][iy] = met->lsm[0][iy];
    met->sst[met->nx - 1][iy] = met->sst[0][iy];
    for (int ip = 0; ip < met->np; ip++) {
      met->t[met->nx - 1][iy][ip] = met->t[0][iy][ip];
      met->u[met->nx - 1][iy][ip] = met->u[0][iy][ip];
      met->v[met->nx - 1][iy][ip] = met->v[0][iy][ip];
      met->w[met->nx - 1][iy][ip] = met->w[0][iy][ip];
      met->h2o[met->nx - 1][iy][ip] = met->h2o[0][iy][ip];
      met->o3[met->nx - 1][iy][ip] = met->o3[0][iy][ip];
      met->lwc[met->nx - 1][iy][ip] = met->lwc[0][iy][ip];
      met->iwc[met->nx - 1][iy][ip] = met->iwc[0][iy][ip];
      met->cc[met->nx - 1][iy][ip] = met->cc[0][iy][ip];
    }
  }
}

/*****************************************************************************/

void read_met_pv(
  met_t * met) {

  double pows[EP];

  /* Set timer... */
  SELECT_TIMER("READ_MET_PV", "METPROC", NVTX_READ);
  LOG(2, "Calculate potential vorticity...");

  /* Set powers... */
#pragma omp parallel for default(shared)
  for (int ip = 0; ip < met->np; ip++)
    pows[ip] = pow(1000. / met->p[ip], 0.286);

  /* Loop over grid points... */
#pragma omp parallel for default(shared)
  for (int ix = 0; ix < met->nx; ix++) {

    /* Set indices... */
    int ix0 = GSL_MAX(ix - 1, 0);
    int ix1 = GSL_MIN(ix + 1, met->nx - 1);

    /* Loop over grid points... */
    for (int iy = 0; iy < met->ny; iy++) {

      /* Set indices... */
      int iy0 = GSL_MAX(iy - 1, 0);
      int iy1 = GSL_MIN(iy + 1, met->ny - 1);

      /* Set auxiliary variables... */
      double latr = 0.5 * (met->lat[iy1] + met->lat[iy0]);
      double dx = 1000. * DEG2DX(met->lon[ix1] - met->lon[ix0], latr);
      double dy = 1000. * DEG2DY(met->lat[iy1] - met->lat[iy0]);
      double c0 = cos(met->lat[iy0] / 180. * M_PI);
      double c1 = cos(met->lat[iy1] / 180. * M_PI);
      double cr = cos(latr / 180. * M_PI);
      double vort = 2 * 7.2921e-5 * sin(latr * M_PI / 180.);

      /* Loop over grid points... */
      for (int ip = 0; ip < met->np; ip++) {

	/* Get gradients in longitude... */
	double dtdx
	  = (met->t[ix1][iy][ip] - met->t[ix0][iy][ip]) * pows[ip] / dx;
	double dvdx = (met->v[ix1][iy][ip] - met->v[ix0][iy][ip]) / dx;

	/* Get gradients in latitude... */
	double dtdy
	  = (met->t[ix][iy1][ip] - met->t[ix][iy0][ip]) * pows[ip] / dy;
	double dudy
	  = (met->u[ix][iy1][ip] * c1 - met->u[ix][iy0][ip] * c0) / dy;

	/* Set indices... */
	int ip0 = GSL_MAX(ip - 1, 0);
	int ip1 = GSL_MIN(ip + 1, met->np - 1);

	/* Get gradients in pressure... */
	double dtdp, dudp, dvdp;
	double dp0 = 100. * (met->p[ip] - met->p[ip0]);
	double dp1 = 100. * (met->p[ip1] - met->p[ip]);
	if (ip != ip0 && ip != ip1) {
	  double denom = dp0 * dp1 * (dp0 + dp1);
	  dtdp = (dp0 * dp0 * met->t[ix][iy][ip1] * pows[ip1]
		  - dp1 * dp1 * met->t[ix][iy][ip0] * pows[ip0]
		  + (dp1 * dp1 - dp0 * dp0) * met->t[ix][iy][ip] * pows[ip])
	    / denom;
	  dudp = (dp0 * dp0 * met->u[ix][iy][ip1]
		  - dp1 * dp1 * met->u[ix][iy][ip0]
		  + (dp1 * dp1 - dp0 * dp0) * met->u[ix][iy][ip])
	    / denom;
	  dvdp = (dp0 * dp0 * met->v[ix][iy][ip1]
		  - dp1 * dp1 * met->v[ix][iy][ip0]
		  + (dp1 * dp1 - dp0 * dp0) * met->v[ix][iy][ip])
	    / denom;
	} else {
	  double denom = dp0 + dp1;
	  dtdp =
	    (met->t[ix][iy][ip1] * pows[ip1] -
	     met->t[ix][iy][ip0] * pows[ip0]) / denom;
	  dudp = (met->u[ix][iy][ip1] - met->u[ix][iy][ip0]) / denom;
	  dvdp = (met->v[ix][iy][ip1] - met->v[ix][iy][ip0]) / denom;
	}

	/* Calculate PV... */
	met->pv[ix][iy][ip] = (float)
	  (1e6 * G0 *
	   (-dtdp * (dvdx - dudy / cr + vort) + dvdp * dtdx - dudp * dtdy));
      }
    }
  }

  /* Fix for polar regions... */
#pragma omp parallel for default(shared)
  for (int ix = 0; ix < met->nx; ix++)
    for (int ip = 0; ip < met->np; ip++) {
      met->pv[ix][0][ip]
	= met->pv[ix][1][ip]
	= met->pv[ix][2][ip];
      met->pv[ix][met->ny - 1][ip]
	= met->pv[ix][met->ny - 2][ip]
	= met->pv[ix][met->ny - 3][ip];
    }
}

/*****************************************************************************/

void read_met_sample(
  ctl_t * ctl,
  met_t * met) {

  met_t *help;

  /* Check parameters... */
  if (ctl->met_dp <= 1 && ctl->met_dx <= 1 && ctl->met_dy <= 1
      && ctl->met_sp <= 1 && ctl->met_sx <= 1 && ctl->met_sy <= 1)
    return;

  /* Set timer... */
  SELECT_TIMER("READ_MET_SAMPLE", "METPROC", NVTX_READ);
  LOG(2, "Downsampling of meteo data...");

  /* Allocate... */
  ALLOC(help, met_t, 1);

  /* Copy data... */
  help->nx = met->nx;
  help->ny = met->ny;
  help->np = met->np;
  memcpy(help->lon, met->lon, sizeof(met->lon));
  memcpy(help->lat, met->lat, sizeof(met->lat));
  memcpy(help->p, met->p, sizeof(met->p));

  /* Smoothing... */
  for (int ix = 0; ix < met->nx; ix += ctl->met_dx) {
    for (int iy = 0; iy < met->ny; iy += ctl->met_dy) {
      for (int ip = 0; ip < met->np; ip += ctl->met_dp) {
	help->ps[ix][iy] = 0;
	help->zs[ix][iy] = 0;
	help->ts[ix][iy] = 0;
	help->us[ix][iy] = 0;
	help->vs[ix][iy] = 0;
	help->lsm[ix][iy] = 0;
	help->sst[ix][iy] = 0;
	help->t[ix][iy][ip] = 0;
	help->u[ix][iy][ip] = 0;
	help->v[ix][iy][ip] = 0;
	help->w[ix][iy][ip] = 0;
	help->h2o[ix][iy][ip] = 0;
	help->o3[ix][iy][ip] = 0;
	help->lwc[ix][iy][ip] = 0;
	help->iwc[ix][iy][ip] = 0;
	help->cc[ix][iy][ip] = 0;
	float wsum = 0;
	for (int ix2 = ix - ctl->met_sx + 1; ix2 <= ix + ctl->met_sx - 1;
	     ix2++) {
	  int ix3 = ix2;
	  if (ix3 < 0)
	    ix3 += met->nx;
	  else if (ix3 >= met->nx)
	    ix3 -= met->nx;

	  for (int iy2 = GSL_MAX(iy - ctl->met_sy + 1, 0);
	       iy2 <= GSL_MIN(iy + ctl->met_sy - 1, met->ny - 1); iy2++)
	    for (int ip2 = GSL_MAX(ip - ctl->met_sp + 1, 0);
		 ip2 <= GSL_MIN(ip + ctl->met_sp - 1, met->np - 1); ip2++) {
	      float w = (1.0f - (float) abs(ix - ix2) / (float) ctl->met_sx)
		* (1.0f - (float) abs(iy - iy2) / (float) ctl->met_sy)
		* (1.0f - (float) abs(ip - ip2) / (float) ctl->met_sp);
	      help->ps[ix][iy] += w * met->ps[ix3][iy2];
	      help->zs[ix][iy] += w * met->zs[ix3][iy2];
	      help->ts[ix][iy] += w * met->ts[ix3][iy2];
	      help->us[ix][iy] += w * met->us[ix3][iy2];
	      help->vs[ix][iy] += w * met->vs[ix3][iy2];
	      help->lsm[ix][iy] += w * met->lsm[ix3][iy2];
	      help->sst[ix][iy] += w * met->sst[ix3][iy2];
	      help->t[ix][iy][ip] += w * met->t[ix3][iy2][ip2];
	      help->u[ix][iy][ip] += w * met->u[ix3][iy2][ip2];
	      help->v[ix][iy][ip] += w * met->v[ix3][iy2][ip2];
	      help->w[ix][iy][ip] += w * met->w[ix3][iy2][ip2];
	      help->h2o[ix][iy][ip] += w * met->h2o[ix3][iy2][ip2];
	      help->o3[ix][iy][ip] += w * met->o3[ix3][iy2][ip2];
	      help->lwc[ix][iy][ip] += w * met->lwc[ix3][iy2][ip2];
	      help->iwc[ix][iy][ip] += w * met->iwc[ix3][iy2][ip2];
	      help->cc[ix][iy][ip] += w * met->cc[ix3][iy2][ip2];
	      wsum += w;
	    }
	}
	help->ps[ix][iy] /= wsum;
	help->zs[ix][iy] /= wsum;
	help->ts[ix][iy] /= wsum;
	help->us[ix][iy] /= wsum;
	help->vs[ix][iy] /= wsum;
	help->lsm[ix][iy] /= wsum;
	help->sst[ix][iy] /= wsum;
	help->t[ix][iy][ip] /= wsum;
	help->u[ix][iy][ip] /= wsum;
	help->v[ix][iy][ip] /= wsum;
	help->w[ix][iy][ip] /= wsum;
	help->h2o[ix][iy][ip] /= wsum;
	help->o3[ix][iy][ip] /= wsum;
	help->lwc[ix][iy][ip] /= wsum;
	help->iwc[ix][iy][ip] /= wsum;
	help->cc[ix][iy][ip] /= wsum;
      }
    }
  }

  /* Downsampling... */
  met->nx = 0;
  for (int ix = 0; ix < help->nx; ix += ctl->met_dx) {
    met->lon[met->nx] = help->lon[ix];
    met->ny = 0;
    for (int iy = 0; iy < help->ny; iy += ctl->met_dy) {
      met->lat[met->ny] = help->lat[iy];
      met->ps[met->nx][met->ny] = help->ps[ix][iy];
      met->zs[met->nx][met->ny] = help->zs[ix][iy];
      met->ts[met->nx][met->ny] = help->ts[ix][iy];
      met->us[met->nx][met->ny] = help->us[ix][iy];
      met->vs[met->nx][met->ny] = help->vs[ix][iy];
      met->lsm[met->nx][met->ny] = help->lsm[ix][iy];
      met->sst[met->nx][met->ny] = help->sst[ix][iy];
      met->np = 0;
      for (int ip = 0; ip < help->np; ip += ctl->met_dp) {
	met->p[met->np] = help->p[ip];
	met->t[met->nx][met->ny][met->np] = help->t[ix][iy][ip];
	met->u[met->nx][met->ny][met->np] = help->u[ix][iy][ip];
	met->v[met->nx][met->ny][met->np] = help->v[ix][iy][ip];
	met->w[met->nx][met->ny][met->np] = help->w[ix][iy][ip];
	met->h2o[met->nx][met->ny][met->np] = help->h2o[ix][iy][ip];
	met->o3[met->nx][met->ny][met->np] = help->o3[ix][iy][ip];
	met->lwc[met->nx][met->ny][met->np] = help->lwc[ix][iy][ip];
	met->iwc[met->nx][met->ny][met->np] = help->iwc[ix][iy][ip];
	met->cc[met->nx][met->ny][met->np] = help->cc[ix][iy][ip];
	met->np++;
      }
      met->ny++;
    }
    met->nx++;
  }

  /* Free... */
  free(help);
}

/*****************************************************************************/

void read_met_surface(
  int ncid,
  met_t * met,
  ctl_t * ctl) {

  /* Set timer... */
  SELECT_TIMER("READ_MET_SURFACE", "INPUT", NVTX_READ);
  LOG(2, "Read surface data...");

  /* MPTRAC meteo data... */
  if (ctl->clams_met_data == 0) {

    /* Read surface pressure... */
    if (!read_met_nc_2d(ncid, "lnsp", "LNSP", ctl, met, met->ps, 1.0f, 1)) {
      if (!read_met_nc_2d(ncid, "ps", "PS", ctl, met, met->ps, 0.01f, 1)) {
	if (!read_met_nc_2d(ncid, "sp", "SP", ctl, met, met->ps, 0.01f, 1)) {
	  WARN("Cannot not read surface pressure data (use lowest level)!");
	  for (int ix = 0; ix < met->nx; ix++)
	    for (int iy = 0; iy < met->ny; iy++)
	      met->ps[ix][iy] = (float) met->p[0];
	}
      }
    } else
      for (int ix = 0; ix < met->nx; ix++)
	for (int iy = 0; iy < met->ny; iy++)
	  met->ps[ix][iy] = (float) (exp(met->ps[ix][iy]) / 100.);

    /* Read geopotential height at the surface... */
    if (!read_met_nc_2d
	(ncid, "z", "Z", ctl, met, met->zs, (float) (1. / (1000. * G0)), 1))
      if (!read_met_nc_2d
	  (ncid, "zm", "ZM", ctl, met, met->zs, (float) (1. / 1000.), 1))
	WARN("Cannot read surface geopotential height!");

    /* Read temperature at the surface... */
    if (!read_met_nc_2d(ncid, "t2m", "T2M", ctl, met, met->ts, 1.0, 1))
      WARN("Cannot read surface temperature!");

    /* Read zonal wind at the surface... */
    if (!read_met_nc_2d(ncid, "u10m", "U10M", ctl, met, met->us, 1.0, 1))
      WARN("Cannot read surface zonal wind!");

    /* Read meridional wind at the surface... */
    if (!read_met_nc_2d(ncid, "v10m", "V10M", ctl, met, met->vs, 1.0, 1))
      WARN("Cannot read surface meridional wind!");

    /* Read land-sea mask... */
    if (!read_met_nc_2d(ncid, "lsm", "LSM", ctl, met, met->lsm, 1.0, 1))
      WARN("Cannot read land-sea mask!");

    /* Read sea surface temperature... */
    if (!read_met_nc_2d(ncid, "sstk", "SSTK", ctl, met, met->sst, 1.0, 1))
      WARN("Cannot read sea surface temperature!");
  }

  /* CLaMS meteo data... */
  else {

    /* Read surface pressure... */
    if (!read_met_nc_2d(ncid, "ps", "PS", ctl, met, met->ps, 0.01f, 1)) {
      WARN("Cannot not read surface pressure data (use lowest level)!");
      for (int ix = 0; ix < met->nx; ix++)
	for (int iy = 0; iy < met->ny; iy++)
	  met->ps[ix][iy] = (float) met->p[0];
    }

    /* Read geopotential height at the surface
       (use lowermost level of 3-D data field)... */
    float *help;
    ALLOC(help, float,
	  EX * EY * EP);
    memcpy(help, met->pl, sizeof(met->pl));
    if (!read_met_nc_3d
	(ncid, "gph", "GPH", ctl, met, met->pl, (float) (1e-3 / G0), 1)) {
      ERRMSG("Cannot read geopotential height!");
    } else
      for (int ix = 0; ix < met->nx; ix++)
	for (int iy = 0; iy < met->ny; iy++)
	  met->zs[ix][iy] = met->pl[ix][iy][0];
    memcpy(met->pl, help, sizeof(met->pl));
    free(help);

    /* Read temperature at the surface... */
    if (!read_met_nc_2d(ncid, "t2", "T2", ctl, met, met->ts, 1.0, 1))
      WARN("Cannot read surface temperature!");

    /* Read zonal wind at the surface... */
    if (!read_met_nc_2d(ncid, "u10", "U10", ctl, met, met->us, 1.0, 1))
      WARN("Cannot read surface zonal wind!");

    /* Read meridional wind at the surface... */
    if (!read_met_nc_2d(ncid, "v10", "V10", ctl, met, met->vs, 1.0, 1))
      WARN("Cannot read surface meridional wind!");

    /* Read land-sea mask... */
    if (!read_met_nc_2d(ncid, "lsm", "LSM", ctl, met, met->lsm, 1.0, 1))
      WARN("Cannot read land-sea mask!");

    /* Read sea surface temperature... */
    if (!read_met_nc_2d(ncid, "sstk", "SSTK", ctl, met, met->sst, 1.0, 1))
      WARN("Cannot read sea surface temperature!");
  }
}

/*****************************************************************************/

void read_met_tropo(
  ctl_t * ctl,
  clim_t * clim,
  met_t * met) {

  double p2[200], pv[EP], pv2[200], t[EP], t2[200], th[EP],
    th2[200], z[EP], z2[200];

  /* Set timer... */
  SELECT_TIMER("READ_MET_TROPO", "METPROC", NVTX_READ);
  LOG(2, "Calculate tropopause...");

  /* Get altitude and pressure profiles... */
#pragma omp parallel for default(shared)
  for (int iz = 0; iz < met->np; iz++)
    z[iz] = Z(met->p[iz]);
#pragma omp parallel for default(shared)
  for (int iz = 0; iz <= 190; iz++) {
    z2[iz] = 4.5 + 0.1 * iz;
    p2[iz] = P(z2[iz]);
  }

  /* Do not calculate tropopause... */
  if (ctl->met_tropo == 0)
#pragma omp parallel for default(shared) collapse(2)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++)
	met->pt[ix][iy] = GSL_NAN;

  /* Use tropopause climatology... */
  else if (ctl->met_tropo == 1) {
#pragma omp parallel for default(shared) collapse(2)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++)
	met->pt[ix][iy] = (float) clim_tropo(clim, met->time, met->lat[iy]);
  }

  /* Use cold point... */
  else if (ctl->met_tropo == 2) {

    /* Loop over grid points... */
#pragma omp parallel for default(shared) private(t,t2) collapse(2)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {

	/* Interpolate temperature profile... */
	for (int iz = 0; iz < met->np; iz++)
	  t[iz] = met->t[ix][iy][iz];
	spline(z, t, met->np, z2, t2, 171, ctl->met_tropo_spline);

	/* Find minimum... */
	int iz = (int) gsl_stats_min_index(t2, 1, 171);
	if (iz > 0 && iz < 170)
	  met->pt[ix][iy] = (float) p2[iz];
	else
	  met->pt[ix][iy] = GSL_NAN;
      }
  }

  /* Use WMO definition... */
  else if (ctl->met_tropo == 3 || ctl->met_tropo == 4) {

    /* Loop over grid points... */
#pragma omp parallel for default(shared) private(t,t2) collapse(2)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {

	/* Interpolate temperature profile... */
	int iz;
	for (iz = 0; iz < met->np; iz++)
	  t[iz] = met->t[ix][iy][iz];
	spline(z, t, met->np, z2, t2, 191, ctl->met_tropo_spline);

	/* Find 1st tropopause... */
	met->pt[ix][iy] = GSL_NAN;
	for (iz = 0; iz <= 170; iz++) {
	  int found = 1;
	  for (int iz2 = iz + 1; iz2 <= iz + ctl->met_tropo_nlev; iz2++)
	    if (LAPSE(p2[iz], t2[iz], p2[iz2], t2[iz2]) >
		ctl->met_tropo_lapse) {
	      found = 0;
	      break;
	    }
	  if (found) {
	    if (iz > 0 && iz < 170)
	      met->pt[ix][iy] = (float) p2[iz];
	    break;
	  }
	}

	/* Find 2nd tropopause... */
	if (ctl->met_tropo == 4) {
	  met->pt[ix][iy] = GSL_NAN;
	  for (; iz <= 170; iz++) {
	    int found = 1;
	    for (int iz2 = iz + 1; iz2 <= iz + ctl->met_tropo_nlev_sep; iz2++)
	      if (LAPSE(p2[iz], t2[iz], p2[iz2], t2[iz2]) <
		  ctl->met_tropo_lapse_sep) {
		found = 0;
		break;
	      }
	    if (found)
	      break;
	  }
	  for (; iz <= 170; iz++) {
	    int found = 1;
	    for (int iz2 = iz + 1; iz2 <= iz + ctl->met_tropo_nlev; iz2++)
	      if (LAPSE(p2[iz], t2[iz], p2[iz2], t2[iz2]) >
		  ctl->met_tropo_lapse) {
		found = 0;
		break;
	      }
	    if (found) {
	      if (iz > 0 && iz < 170)
		met->pt[ix][iy] = (float) p2[iz];
	      break;
	    }
	  }
	}
      }
  }

  /* Use dynamical tropopause... */
  else if (ctl->met_tropo == 5) {

    /* Loop over grid points... */
#pragma omp parallel for default(shared) private(pv,pv2,th,th2) collapse(2)
    for (int ix = 0; ix < met->nx; ix++)
      for (int iy = 0; iy < met->ny; iy++) {

	/* Interpolate potential vorticity profile... */
	for (int iz = 0; iz < met->np; iz++)
	  pv[iz] = met->pv[ix][iy][iz];
	spline(z, pv, met->np, z2, pv2, 171, ctl->met_tropo_spline);

	/* Interpolate potential temperature profile... */
	for (int iz = 0; iz < met->np; iz++)
	  th[iz] = THETA(met->p[iz], met->t[ix][iy][iz]);
	spline(z, th, met->np, z2, th2, 171, ctl->met_tropo_spline);

	/* Find dynamical tropopause... */
	met->pt[ix][iy] = GSL_NAN;
	for (int iz = 0; iz <= 170; iz++)
	  if (fabs(pv2[iz]) >= ctl->met_tropo_pv
	      || th2[iz] >= ctl->met_tropo_theta) {
	    if (iz > 0 && iz < 170)
	      met->pt[ix][iy] = (float) p2[iz];
	    break;
	  }
      }
  }

  else
    ERRMSG("Cannot calculate tropopause!");

  /* Interpolate temperature, geopotential height, and water vapor vmr... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++) {
      double h2ot, tt, zt;
      INTPOL_INIT;
      intpol_met_space_3d(met, met->t, met->pt[ix][iy], met->lon[ix],
			  met->lat[iy], &tt, ci, cw, 1);
      intpol_met_space_3d(met, met->z, met->pt[ix][iy], met->lon[ix],
			  met->lat[iy], &zt, ci, cw, 0);
      intpol_met_space_3d(met, met->h2o, met->pt[ix][iy], met->lon[ix],
			  met->lat[iy], &h2ot, ci, cw, 0);
      met->tt[ix][iy] = (float) tt;
      met->zt[ix][iy] = (float) zt;
      met->h2ot[ix][iy] = (float) h2ot;
    }
}

/*****************************************************************************/

void read_obs(
  char *filename,
  double *rt,
  double *rz,
  double *rlon,
  double *rlat,
  double *robs,
  int *nobs) {

  FILE *in;

  char line[LEN];

  /* Open observation data file... */
  LOG(1, "Read observation data: %s", filename);
  if (!(in = fopen(filename, "r")))
    ERRMSG("Cannot open file!");

  /* Read observations... */
  while (fgets(line, LEN, in))
    if (sscanf(line, "%lg %lg %lg %lg %lg", &rt[*nobs], &rz[*nobs],
	       &rlon[*nobs], &rlat[*nobs], &robs[*nobs]) == 5)
      if ((++(*nobs)) >= NOBS)
	ERRMSG("Too many observations!");

  /* Close observation data file... */
  fclose(in);

  /* Check time... */
  for (int i = 1; i < *nobs; i++)
    if (rt[i] < rt[i - 1])
      ERRMSG("Time must be ascending!");

  /* Write info... */
  int n = *nobs;
  double mini, maxi;
  LOG(2, "Number of observations: %d", *nobs);
  gsl_stats_minmax(&mini, &maxi, rt, 1, (size_t) n);
  LOG(2, "Time range: %.2f ... %.2f s", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, rz, 1, (size_t) n);
  LOG(2, "Altitude range: %g ... %g km", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, rlon, 1, (size_t) n);
  LOG(2, "Longitude range: %g ... %g deg", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, rlat, 1, (size_t) n);
  LOG(2, "Latitude range: %g ... %g deg", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, robs, 1, (size_t) n);
  LOG(2, "Observation range: %g ... %g", mini, maxi);
}

/*****************************************************************************/

double scan_ctl(
  const char *filename,
  int argc,
  char *argv[],
  const char *varname,
  int arridx,
  const char *defvalue,
  char *value) {

  FILE *in = NULL;

  char fullname1[LEN], fullname2[LEN], rval[LEN];

  int contain = 0, i;

  /* Open file... */
  if (filename[strlen(filename) - 1] != '-')
    if (!(in = fopen(filename, "r")))
      ERRMSG("Cannot open file!");

  /* Set full variable name... */
  if (arridx >= 0) {
    sprintf(fullname1, "%s[%d]", varname, arridx);
    sprintf(fullname2, "%s[*]", varname);
  } else {
    sprintf(fullname1, "%s", varname);
    sprintf(fullname2, "%s", varname);
  }

  /* Read data... */
  if (in != NULL) {
    char dummy[LEN], line[LEN], rvarname[LEN];
    while (fgets(line, LEN, in)) {
      if (sscanf(line, "%4999s %4999s %4999s", rvarname, dummy, rval) == 3)
	if (strcasecmp(rvarname, fullname1) == 0 ||
	    strcasecmp(rvarname, fullname2) == 0) {
	  contain = 1;
	  break;
	}
    }
  }
  for (i = 1; i < argc - 1; i++)
    if (strcasecmp(argv[i], fullname1) == 0 ||
	strcasecmp(argv[i], fullname2) == 0) {
      sprintf(rval, "%s", argv[i + 1]);
      contain = 1;
      break;
    }

  /* Close file... */
  if (in != NULL)
    fclose(in);

  /* Check for missing variables... */
  if (!contain) {
    if (strlen(defvalue) > 0)
      sprintf(rval, "%s", defvalue);
    else
      ERRMSG("Missing variable %s!\n", fullname1);
  }

  /* Write info... */
  LOG(1, "%s = %s", fullname1, rval);

  /* Return values... */
  if (value != NULL)
    sprintf(value, "%s", rval);
  return atof(rval);
}

/*****************************************************************************/

double sedi(
  double p,
  double T,
  double rp,
  double rhop) {

  /* Convert particle radius from microns to m... */
  rp *= 1e-6;

  /* Density of dry air [kg / m^3]... */
  double rho = RHO(p, T);

  /* Dynamic viscosity of air [kg / (m s)]... */
  double eta = 1.8325e-5 * (416.16 / (T + 120.)) * pow(T / 296.16, 1.5);

  /* Thermal velocity of an air molecule [m / s]... */
  double v = sqrt(8. * KB * T / (M_PI * 4.8096e-26));

  /* Mean free path of an air molecule [m]... */
  double lambda = 2. * eta / (rho * v);

  /* Knudsen number for air (dimensionless)... */
  double K = lambda / rp;

  /* Cunningham slip-flow correction (dimensionless)... */
  double G = 1. + K * (1.249 + 0.42 * exp(-0.87 / K));

  /* Sedimentation velocity [m / s]... */
  return 2. * SQR(rp) * (rhop - rho) * G0 / (9. * eta) * G;
}

/*****************************************************************************/

void spline(
  double *x,
  double *y,
  int n,
  double *x2,
  double *y2,
  int n2,
  int method) {

  /* Cubic spline interpolation... */
  if (method == 1) {

    /* Allocate... */
    gsl_interp_accel *acc;
    gsl_spline *s;
    acc = gsl_interp_accel_alloc();
    s = gsl_spline_alloc(gsl_interp_cspline, (size_t) n);

    /* Interpolate profile... */
    gsl_spline_init(s, x, y, (size_t) n);
    for (int i = 0; i < n2; i++)
      if (x2[i] <= x[0])
	y2[i] = y[0];
      else if (x2[i] >= x[n - 1])
	y2[i] = y[n - 1];
      else
	y2[i] = gsl_spline_eval(s, x2[i], acc);

    /* Free... */
    gsl_spline_free(s);
    gsl_interp_accel_free(acc);
  }

  /* Linear interpolation... */
  else {
    for (int i = 0; i < n2; i++)
      if (x2[i] <= x[0])
	y2[i] = y[0];
      else if (x2[i] >= x[n - 1])
	y2[i] = y[n - 1];
      else {
	int idx = locate_irr(x, n, x2[i]);
	y2[i] = LIN(x[idx], y[idx], x[idx + 1], y[idx + 1], x2[i]);
      }
  }
}

/*****************************************************************************/

float stddev(
  float *data,
  int n) {

  if (n <= 0)
    return 0;

  float mean = 0, var = 0;

  for (int i = 0; i < n; ++i) {
    mean += data[i];
    var += SQR(data[i]);
  }

  var = var / (float) n - SQR(mean / (float) n);

  return (var > 0 ? sqrtf(var) : 0);
}

/*****************************************************************************/

double sza(
  double sec,
  double lon,
  double lat) {

  double D, dec, e, g, GMST, h, L, LST, q, ra;

  /* Number of days and fraction with respect to 2000-01-01T12:00Z... */
  D = sec / 86400 - 0.5;

  /* Geocentric apparent ecliptic longitude [rad]... */
  g = (357.529 + 0.98560028 * D) * M_PI / 180;
  q = 280.459 + 0.98564736 * D;
  L = (q + 1.915 * sin(g) + 0.020 * sin(2 * g)) * M_PI / 180;

  /* Mean obliquity of the ecliptic [rad]... */
  e = (23.439 - 0.00000036 * D) * M_PI / 180;

  /* Declination [rad]... */
  dec = asin(sin(e) * sin(L));

  /* Right ascension [rad]... */
  ra = atan2(cos(e) * sin(L), cos(L));

  /* Greenwich Mean Sidereal Time [h]... */
  GMST = 18.697374558 + 24.06570982441908 * D;

  /* Local Sidereal Time [h]... */
  LST = GMST + lon / 15;

  /* Hour angle [rad]... */
  h = LST / 12 * M_PI - ra;

  /* Convert latitude... */
  lat *= M_PI / 180;

  /* Return solar zenith angle [rad]... */
  return acos(sin(lat) * sin(dec) + cos(lat) * cos(dec) * cos(h));
}

/*****************************************************************************/

void time2jsec(
  int year,
  int mon,
  int day,
  int hour,
  int min,
  int sec,
  double remain,
  double *jsec) {

  struct tm t0, t1;

  t0.tm_year = 100;
  t0.tm_mon = 0;
  t0.tm_mday = 1;
  t0.tm_hour = 0;
  t0.tm_min = 0;
  t0.tm_sec = 0;

  t1.tm_year = year - 1900;
  t1.tm_mon = mon - 1;
  t1.tm_mday = day;
  t1.tm_hour = hour;
  t1.tm_min = min;
  t1.tm_sec = sec;

  *jsec = (double) timegm(&t1) - (double) timegm(&t0) + remain;
}

/*****************************************************************************/

void timer(
  const char *name,
  const char *group,
  int output) {

  static char names[NTIMER][100], groups[NTIMER][100];

  static double rt_name[NTIMER], rt_group[NTIMER],
    rt_min[NTIMER], rt_max[NTIMER], dt, t0, t1;

  static int iname = -1, igroup = -1, nname, ngroup, ct_name[NTIMER];

  /* Get time... */
  t1 = omp_get_wtime();
  dt = t1 - t0;

  /* Add elapsed time to current timers... */
  if (iname >= 0) {
    rt_name[iname] += dt;
    rt_min[iname] = (ct_name[iname] <= 0 ? dt : GSL_MIN(rt_min[iname], dt));
    rt_max[iname] = (ct_name[iname] <= 0 ? dt : GSL_MAX(rt_max[iname], dt));
    ct_name[iname]++;
  }
  if (igroup >= 0)
    rt_group[igroup] += t1 - t0;

  /* Report timers... */
  if (output) {
    for (int i = 0; i < nname; i++)
      LOG(1, "TIMER_%s = %.3f s    (min= %g s, mean= %g s,"
	  " max= %g s, n= %d)", names[i], rt_name[i], rt_min[i],
	  rt_name[i] / ct_name[i], rt_max[i], ct_name[i]);
    for (int i = 0; i < ngroup; i++)
      LOG(1, "TIMER_GROUP_%s = %.3f s", groups[i], rt_group[i]);
    double total = 0.0;
    for (int i = 0; i < nname; i++)
      total += rt_name[i];
    LOG(1, "TIMER_TOTAL = %.3f s", total);
  }

  /* Identify IDs of next timer... */
  for (iname = 0; iname < nname; iname++)
    if (strcasecmp(name, names[iname]) == 0)
      break;
  for (igroup = 0; igroup < ngroup; igroup++)
    if (strcasecmp(group, groups[igroup]) == 0)
      break;

  /* Check whether this is a new timer... */
  if (iname >= nname) {
    sprintf(names[iname], "%s", name);
    if ((++nname) > NTIMER)
      ERRMSG("Too many timers!");
  }

  /* Check whether this is a new group... */
  if (igroup >= ngroup) {
    sprintf(groups[igroup], "%s", group);
    if ((++ngroup) > NTIMER)
      ERRMSG("Too many groups!");
  }

  /* Save starting time... */
  t0 = t1;
}

/*****************************************************************************/

double time_from_filename(
  const char *filename,
  int offset) {

  char tstr[10];

  double t;

  /* Get time from filename... */
  int len = (int) strlen(filename);
  sprintf(tstr, "%.4s", &filename[len - offset]);
  int year = atoi(tstr);
  sprintf(tstr, "%.2s", &filename[len - offset + 5]);
  int mon = atoi(tstr);
  sprintf(tstr, "%.2s", &filename[len - offset + 8]);
  int day = atoi(tstr);
  sprintf(tstr, "%.2s", &filename[len - offset + 11]);
  int hour = atoi(tstr);
  sprintf(tstr, "%.2s", &filename[len - offset + 14]);
  int min = atoi(tstr);

  /* Check time... */
  if (year < 1900 || year > 2100 || mon < 1 || mon > 12 || day < 1
      || day > 31 || hour < 0 || hour > 23 || min < 0 || min > 59)
    ERRMSG("Cannot read time from filename!");

  /* Convert time to Julian seconds... */
  time2jsec(year, mon, day, hour, min, 0, 0.0, &t);

  /* Return time... */
  return t;
}

/*****************************************************************************/

double tropo_weight(
  clim_t * clim,
  double t,
  double lat,
  double p) {

  /* Get tropopause pressure... */
  double pt = clim_tropo(clim, t, lat);

  /* Get pressure range... */
  double p1 = pt * 0.866877899;
  double p0 = pt / 0.866877899;

  /* Get weighting factor... */
  if (p > p0)
    return 1;
  else if (p < p1)
    return 0;
  else
    return LIN(p0, 1.0, p1, 0.0, p);
}

/*****************************************************************************/

void write_atm(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  /* Set timer... */
  SELECT_TIMER("WRITE_ATM", "OUTPUT", NVTX_WRITE);

  /* Write info... */
  LOG(1, "Write atmospheric data: %s", filename);

  /* Write ASCII data... */
  if (ctl->atm_type == 0)
    write_atm_asc(filename, ctl, atm, t);

  /* Write binary data... */
  else if (ctl->atm_type == 1)
    write_atm_bin(filename, ctl, atm);

  /* Write netCDF data... */
  else if (ctl->atm_type == 2)
    write_atm_nc(filename, ctl, atm);

  /* Write CLaMS data... */
  else if (ctl->atm_type == 3)
    write_atm_clams(ctl, atm, t);

  /* Error... */
  else
    ERRMSG("Atmospheric data type not supported!");

  /* Write info... */
  double mini, maxi;
  LOG(2, "Number of particles: %d", atm->np);
  gsl_stats_minmax(&mini, &maxi, atm->time, 1, (size_t) atm->np);
  LOG(2, "Time range: %.2f ... %.2f s", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, atm->p, 1, (size_t) atm->np);
  LOG(2, "Altitude range: %g ... %g km", Z(maxi), Z(mini));
  LOG(2, "Pressure range: %g ... %g hPa", maxi, mini);
  gsl_stats_minmax(&mini, &maxi, atm->lon, 1, (size_t) atm->np);
  LOG(2, "Longitude range: %g ... %g deg", mini, maxi);
  gsl_stats_minmax(&mini, &maxi, atm->lat, 1, (size_t) atm->np);
  LOG(2, "Latitude range: %g ... %g deg", mini, maxi);
  for (int iq = 0; iq < ctl->nq; iq++) {
    char msg[LEN];
    sprintf(msg, "Quantity %s range: %s ... %s %s",
	    ctl->qnt_name[iq], ctl->qnt_format[iq],
	    ctl->qnt_format[iq], ctl->qnt_unit[iq]);
    gsl_stats_minmax(&mini, &maxi, atm->q[iq], 1, (size_t) atm->np);
    LOG(2, msg, mini, maxi);
  }
}

/*****************************************************************************/

void write_atm_asc(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  FILE *out;

  /* Set time interval for output... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Check if gnuplot output is requested... */
  if (ctl->atm_gpfile[0] != '-') {

    /* Create gnuplot pipe... */
    if (!(out = popen("gnuplot", "w")))
      ERRMSG("Cannot create pipe to gnuplot!");

    /* Set plot filename... */
    fprintf(out, "set out \"%s.png\"\n", filename);

    /* Set time string... */
    double r;
    int year, mon, day, hour, min, sec;
    jsec2time(t, &year, &mon, &day, &hour, &min, &sec, &r);
    fprintf(out, "timestr=\"%d-%02d-%02d, %02d:%02d UTC\"\n",
	    year, mon, day, hour, min);

    /* Dump gnuplot file to pipe... */
    FILE *in;
    if (!(in = fopen(ctl->atm_gpfile, "r")))
      ERRMSG("Cannot open file!");
    char line[LEN];
    while (fgets(line, LEN, in))
      fprintf(out, "%s", line);
    fclose(in);
  }

  else {

    /* Create file... */
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");
  }

  /* Write header... */
  fprintf(out,
	  "# $1 = time [s]\n"
	  "# $2 = altitude [km]\n"
	  "# $3 = longitude [deg]\n" "# $4 = latitude [deg]\n");
  for (int iq = 0; iq < ctl->nq; iq++)
    fprintf(out, "# $%i = %s [%s]\n", iq + 5, ctl->qnt_name[iq],
	    ctl->qnt_unit[iq]);
  fprintf(out, "\n");

  /* Write data... */
  for (int ip = 0; ip < atm->np; ip += ctl->atm_stride) {

    /* Check time... */
    if (ctl->atm_filter == 2 && (atm->time[ip] < t0 || atm->time[ip] > t1))
      continue;

    /* Write output... */
    fprintf(out, "%.2f %g %g %g", atm->time[ip], Z(atm->p[ip]),
	    atm->lon[ip], atm->lat[ip]);
    for (int iq = 0; iq < ctl->nq; iq++) {
      fprintf(out, " ");
      if (ctl->atm_filter == 1 && (atm->time[ip] < t0 || atm->time[ip] > t1))
	fprintf(out, ctl->qnt_format[iq], GSL_NAN);
      else
	fprintf(out, ctl->qnt_format[iq], atm->q[iq][ip]);
    }
    fprintf(out, "\n");
  }

  /* Close file... */
  fclose(out);
}

/*****************************************************************************/

void write_atm_bin(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  FILE *out;

  /* Create file... */
  if (!(out = fopen(filename, "w")))
    ERRMSG("Cannot create file!");

  /* Write version of binary data... */
  int version = 100;
  FWRITE(&version, int,
	 1,
	 out);

  /* Write data... */
  FWRITE(&atm->np, int,
	 1,
	 out);
  FWRITE(atm->time, double,
	   (size_t) atm->np,
	 out);
  FWRITE(atm->p, double,
	   (size_t) atm->np,
	 out);
  FWRITE(atm->lon, double,
	   (size_t) atm->np,
	 out);
  FWRITE(atm->lat, double,
	   (size_t) atm->np,
	 out);
  for (int iq = 0; iq < ctl->nq; iq++)
    FWRITE(atm->q[iq], double,
	     (size_t) atm->np,
	   out);

  /* Write final flag... */
  int final = 999;
  FWRITE(&final, int,
	 1,
	 out);

  /* Close file... */
  fclose(out);
}

/*****************************************************************************/

void write_atm_clams(
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  /* Global Counter... */
  static size_t out_cnt = 0;

  char filename_out[2 * LEN] = "./traj_fix_3d_YYYYMMDDHH_YYYYMMDDHH.nc";

  double r, r_start, r_stop;

  int year, mon, day, hour, min, sec;
  int year_start, mon_start, day_start, hour_start, min_start, sec_start;
  int year_stop, mon_stop, day_stop, hour_stop, min_stop, sec_stop;
  int ncid, varid, tid, pid, cid, zid, dim_ids[2];

  /* time, nparc */
  size_t start[2], count[2];

  /* Determine start and stop times of calculation... */
  jsec2time(t, &year, &mon, &day, &hour, &min, &sec, &r);
  jsec2time(ctl->t_start, &year_start, &mon_start, &day_start, &hour_start,
	    &min_start, &sec_start, &r_start);
  jsec2time(ctl->t_stop, &year_stop, &mon_stop, &day_stop, &hour_stop,
	    &min_stop, &sec_stop, &r_stop);

  /* Set filename... */
  sprintf(filename_out,
	  "./traj_fix_3d_%02d%02d%02d%02d_%02d%02d%02d%02d.nc",
	  year_start % 100, mon_start, day_start, hour_start,
	  year_stop % 100, mon_stop, day_stop, hour_stop);
  printf("Write traj file: %s\n", filename_out);

  /* Define hyperslap for the traj_file... */
  start[0] = out_cnt;
  start[1] = 0;
  count[0] = 1;
  count[1] = (size_t) atm->np;

  /* Create the file at the first timestep... */
  if (out_cnt == 0) {

    /* Create file... */
    NC(nc_create(filename_out, NC_CLOBBER, &ncid));

    /* Define dimensions... */
    NC(nc_def_dim(ncid, "time", NC_UNLIMITED, &tid));
    NC(nc_def_dim(ncid, "NPARTS", (size_t) atm->np, &pid));
    NC(nc_def_dim(ncid, "TMDT", 7, &cid));
    dim_ids[0] = tid;
    dim_ids[1] = pid;

    /* Define variables and their attributes... */
    NC_DEF_VAR("time", NC_DOUBLE, 1, &tid, "Time",
	       "seconds since 2000-01-01 00:00:00 UTC");
    NC_DEF_VAR("LAT", NC_DOUBLE, 2, dim_ids, "Latitude", "deg");
    NC_DEF_VAR("LON", NC_DOUBLE, 2, dim_ids, "Longitude", "deg");
    NC_DEF_VAR("PRESS", NC_DOUBLE, 2, dim_ids, "Pressure", "hPa");
    NC_DEF_VAR("ZETA", NC_DOUBLE, 2, dim_ids, "Zeta", "K");
    for (int iq = 0; iq < ctl->nq; iq++)
      NC_DEF_VAR(ctl->qnt_name[iq], NC_DOUBLE, 2, dim_ids,
		 ctl->qnt_name[iq], ctl->qnt_unit[iq]);

    /* Define global attributes... */
    NC_PUT_ATT_GLOBAL("exp_VERTCOOR_name", "zeta");
    NC_PUT_ATT_GLOBAL("model", "MPTRAC");

    /* End definitions... */
    NC(nc_enddef(ncid));
    NC(nc_close(ncid));
  }

  /* Increment global counter to change hyperslap... */
  out_cnt++;

  /* Open file... */
  NC(nc_open(filename_out, NC_WRITE, &ncid));

  /* Write data... */
  NC_PUT_DOUBLE("time", atm->time, 1);
  NC_PUT_DOUBLE("LAT", atm->lat, 1);
  NC_PUT_DOUBLE("LON", atm->lon, 1);
  NC_PUT_DOUBLE("PRESS", atm->p, 1);
  if (ctl->vert_coord_ap == 1) {
    NC_PUT_DOUBLE("ZETA", atm->zeta, 1);
  } else if (ctl->qnt_zeta >= 0) {
    NC_PUT_DOUBLE("ZETA", atm->q[ctl->qnt_zeta], 1);
  }
  for (int iq = 0; iq < ctl->nq; iq++)
    NC_PUT_DOUBLE(ctl->qnt_name[iq], atm->q[iq], 1);

  /* Close file... */
  NC(nc_close(ncid));

  /* At the last time step create the init_fix_YYYYMMDDHH file... */
  if ((year == year_stop) && (mon == mon_stop)
      && (day == day_stop) && (hour == hour_stop)) {

    /* Set filename... */
    char filename_init[2 * LEN] = "./init_fix_YYYYMMDDHH.nc";
    sprintf(filename_init, "./init_fix_%02d%02d%02d%02d.nc",
	    year_stop % 100, mon_stop, day_stop, hour_stop);
    printf("Write init file: %s\n", filename_init);

    /* Create file... */
    NC(nc_create(filename_init, NC_CLOBBER, &ncid));

    /* Define dimensions... */
    NC(nc_def_dim(ncid, "time", 1, &tid));
    NC(nc_def_dim(ncid, "NPARTS", (size_t) atm->np, &pid));
    dim_ids[0] = tid;
    dim_ids[1] = pid;

    /* Define variables and their attributes... */
    NC_DEF_VAR("time", NC_DOUBLE, 1, &tid, "Time",
	       "seconds since 2000-01-01 00:00:00 UTC");
    NC_DEF_VAR("LAT", NC_DOUBLE, 1, &pid, "Latitude", "deg");
    NC_DEF_VAR("LON", NC_DOUBLE, 1, &pid, "Longitude", "deg");
    NC_DEF_VAR("PRESS", NC_DOUBLE, 1, &pid, "Pressure", "hPa");
    NC_DEF_VAR("ZETA", NC_DOUBLE, 1, &pid, "Zeta", "K");
    NC_DEF_VAR("ZETA_GRID", NC_DOUBLE, 1, &zid, "levels", "K");
    NC_DEF_VAR("ZETA_DELTA", NC_DOUBLE, 1, &zid, "Width of zeta levels", "K");
    for (int iq = 0; iq < ctl->nq; iq++)
      NC_DEF_VAR(ctl->qnt_name[iq], NC_DOUBLE, 2, dim_ids,
		 ctl->qnt_name[iq], ctl->qnt_unit[iq]);

    /* Define global attributes... */
    NC_PUT_ATT_GLOBAL("exp_VERTCOOR_name", "zeta");
    NC_PUT_ATT_GLOBAL("model", "MPTRAC");

    /* End definitions... */
    NC(nc_enddef(ncid));

    /* Write data... */
    NC_PUT_DOUBLE("time", atm->time, 0);
    NC_PUT_DOUBLE("LAT", atm->lat, 0);
    NC_PUT_DOUBLE("LON", atm->lon, 0);
    NC_PUT_DOUBLE("PRESS", atm->p, 0);
    NC_PUT_DOUBLE("ZETA", atm->q[ctl->qnt_zeta], 0);
    for (int iq = 0; iq < ctl->nq; iq++)
      NC_PUT_DOUBLE(ctl->qnt_name[iq], atm->q[iq], 0);

    /* Close file... */
    NC(nc_close(ncid));
  }
}

/*****************************************************************************/

void write_atm_nc(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm) {

  int ncid, obsid, varid;

  size_t start[2], count[2];

  /* Create file... */
  NC(nc_create(filename, NC_CLOBBER, &ncid));

  /* Define dimensions... */
  NC(nc_def_dim(ncid, "obs", (size_t) atm->np, &obsid));

  /* Define variables and their attributes... */
  NC_DEF_VAR("time", NC_DOUBLE, 1, &obsid, "time",
	     "seconds since 2000-01-01 00:00:00 UTC");
  NC_DEF_VAR("press", NC_DOUBLE, 1, &obsid, "pressure", "hPa");
  NC_DEF_VAR("lon", NC_DOUBLE, 1, &obsid, "longitude", "degrees_east");
  NC_DEF_VAR("lat", NC_DOUBLE, 1, &obsid, "latitude", "degrees_north");
  for (int iq = 0; iq < ctl->nq; iq++)
    NC_DEF_VAR(ctl->qnt_name[iq], NC_DOUBLE, 1, &obsid,
	       ctl->qnt_longname[iq], ctl->qnt_unit[iq]);

  /* Define global attributes... */
  NC_PUT_ATT_GLOBAL("featureType", "point");

  /* End definitions... */
  NC(nc_enddef(ncid));

  /* Write data... */
  NC_PUT_DOUBLE("time", atm->time, 0);
  NC_PUT_DOUBLE("press", atm->p, 0);
  NC_PUT_DOUBLE("lon", atm->lon, 0);
  NC_PUT_DOUBLE("lat", atm->lat, 0);
  for (int iq = 0; iq < ctl->nq; iq++)
    NC_PUT_DOUBLE(ctl->qnt_name[iq], atm->q[iq], 0);

  /* Close file... */
  NC(nc_close(ncid));
}

/*****************************************************************************/

void write_csi(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  static FILE *out;

  static double *modmean, *obsmean, *rt, *rz, *rlon, *rlat, *robs, *area,
    dlon, dlat, dz, x[NCSI], y[NCSI], kz[EP], kw[EP];

  static int *obscount, ct, cx, cy, cz, ip, ix, iy, iz, n, nobs, nk;

  /* Set timer... */
  SELECT_TIMER("WRITE_CSI", "OUTPUT", NVTX_WRITE);

  /* Init... */
  if (t == ctl->t_start) {

    /* Check quantity index for mass... */
    if (ctl->qnt_m < 0)
      ERRMSG("Need quantity mass!");

    /* Allocate... */
    ALLOC(area, double,
	  ctl->csi_ny);
    ALLOC(rt, double,
	  NOBS);
    ALLOC(rz, double,
	  NOBS);
    ALLOC(rlon, double,
	  NOBS);
    ALLOC(rlat, double,
	  NOBS);
    ALLOC(robs, double,
	  NOBS);

    /* Read observation data... */
    read_obs(ctl->csi_obsfile, rt, rz, rlon, rlat, robs, &nobs);

    /* Read kernel data... */
    if (ctl->csi_kernel[0] != '-')
      read_kernel(ctl->csi_kernel, kz, kw, &nk);

    /* Create new file... */
    LOG(1, "Write CSI data: %s", filename);
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");

    /* Write header... */
    fprintf(out,
	    "# $1 = time [s]\n"
	    "# $2 = number of hits (cx)\n"
	    "# $3 = number of misses (cy)\n"
	    "# $4 = number of false alarms (cz)\n"
	    "# $5 = number of observations (cx + cy)\n"
	    "# $6 = number of forecasts (cx + cz)\n"
	    "# $7 = bias (ratio of forecasts and observations) [%%]\n"
	    "# $8 = probability of detection (POD) [%%]\n"
	    "# $9 = false alarm rate (FAR) [%%]\n"
	    "# $10 = critical success index (CSI) [%%]\n");
    fprintf(out,
	    "# $11 = hits associated with random chance\n"
	    "# $12 = equitable threat score (ETS) [%%]\n"
	    "# $13 = Pearson linear correlation coefficient\n"
	    "# $14 = Spearman rank-order correlation coefficient\n"
	    "# $15 = column density mean error (F - O) [kg/m^2]\n"
	    "# $16 = column density root mean square error (RMSE) [kg/m^2]\n"
	    "# $17 = column density mean absolute error [kg/m^2]\n"
	    "# $18 = number of data points\n\n");

    /* Set grid box size... */
    dz = (ctl->csi_z1 - ctl->csi_z0) / ctl->csi_nz;
    dlon = (ctl->csi_lon1 - ctl->csi_lon0) / ctl->csi_nx;
    dlat = (ctl->csi_lat1 - ctl->csi_lat0) / ctl->csi_ny;

    /* Set horizontal coordinates... */
    for (iy = 0; iy < ctl->csi_ny; iy++) {
      double lat = ctl->csi_lat0 + dlat * (iy + 0.5);
      area[iy] = dlat * dlon * SQR(RE * M_PI / 180.) * cos(lat * M_PI / 180.);
    }
  }

  /* Set time interval... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Allocate... */
  ALLOC(modmean, double,
	ctl->csi_nx * ctl->csi_ny * ctl->csi_nz);
  ALLOC(obsmean, double,
	ctl->csi_nx * ctl->csi_ny * ctl->csi_nz);
  ALLOC(obscount, int,
	ctl->csi_nx * ctl->csi_ny * ctl->csi_nz);

  /* Loop over observations... */
  for (int i = 0; i < nobs; i++) {

    /* Check time... */
    if (rt[i] < t0)
      continue;
    else if (rt[i] >= t1)
      break;

    /* Check observation data... */
    if (!isfinite(robs[i]))
      continue;

    /* Calculate indices... */
    ix = (int) ((rlon[i] - ctl->csi_lon0) / dlon);
    iy = (int) ((rlat[i] - ctl->csi_lat0) / dlat);
    iz = (int) ((rz[i] - ctl->csi_z0) / dz);

    /* Check indices... */
    if (ix < 0 || ix >= ctl->csi_nx ||
	iy < 0 || iy >= ctl->csi_ny || iz < 0 || iz >= ctl->csi_nz)
      continue;

    /* Get mean observation index... */
    int idx = ARRAY_3D(ix, iy, ctl->csi_ny, iz, ctl->csi_nz);
    obsmean[idx] += robs[i];
    obscount[idx]++;
  }

  /* Analyze model data... */
  for (ip = 0; ip < atm->np; ip++) {

    /* Check time... */
    if (atm->time[ip] < t0 || atm->time[ip] > t1)
      continue;

    /* Get indices... */
    ix = (int) ((atm->lon[ip] - ctl->csi_lon0) / dlon);
    iy = (int) ((atm->lat[ip] - ctl->csi_lat0) / dlat);
    iz = (int) ((Z(atm->p[ip]) - ctl->csi_z0) / dz);

    /* Check indices... */
    if (ix < 0 || ix >= ctl->csi_nx ||
	iy < 0 || iy >= ctl->csi_ny || iz < 0 || iz >= ctl->csi_nz)
      continue;

    /* Get total mass in grid cell... */
    int idx = ARRAY_3D(ix, iy, ctl->csi_ny, iz, ctl->csi_nz);
    modmean[idx] += kernel_weight(kz, kw, nk, atm->p[ip])
      * atm->q[ctl->qnt_m][ip];
  }

  /* Analyze all grid cells... */
  for (ix = 0; ix < ctl->csi_nx; ix++)
    for (iy = 0; iy < ctl->csi_ny; iy++)
      for (iz = 0; iz < ctl->csi_nz; iz++) {

	/* Calculate mean observation index... */
	int idx = ARRAY_3D(ix, iy, ctl->csi_ny, iz, ctl->csi_nz);
	if (obscount[idx] > 0)
	  obsmean[idx] /= obscount[idx];

	/* Calculate column density... */
	if (modmean[idx] > 0)
	  modmean[idx] /= (1e6 * area[iy]);

	/* Calculate CSI... */
	if (obscount[idx] > 0) {
	  ct++;
	  if (obsmean[idx] >= ctl->csi_obsmin &&
	      modmean[idx] >= ctl->csi_modmin)
	    cx++;
	  else if (obsmean[idx] >= ctl->csi_obsmin &&
		   modmean[idx] < ctl->csi_modmin)
	    cy++;
	  else if (obsmean[idx] < ctl->csi_obsmin &&
		   modmean[idx] >= ctl->csi_modmin)
	    cz++;
	}

	/* Save data for other verification statistics... */
	if (obscount[idx] > 0
	    && (obsmean[idx] >= ctl->csi_obsmin
		|| modmean[idx] >= ctl->csi_modmin)) {
	  x[n] = modmean[idx];
	  y[n] = obsmean[idx];
	  if ((++n) > NCSI)
	    ERRMSG("Too many data points to calculate statistics!");
	}
      }

  /* Write output... */
  if (fmod(t, ctl->csi_dt_out) == 0) {

    /* Calculate verification statistics
       (https://www.cawcr.gov.au/projects/verification/) ... */
    static double work[2 * NCSI];
    int n_obs = cx + cy;
    int n_for = cx + cz;
    double bias = (n_obs > 0) ? 100. * n_for / n_obs : GSL_NAN;
    double pod = (n_obs > 0) ? (100. * cx) / n_obs : GSL_NAN;
    double far = (n_for > 0) ? (100. * cz) / n_for : GSL_NAN;
    double csi = (cx + cy + cz > 0) ? (100. * cx) / (cx + cy + cz) : GSL_NAN;
    double cx_rd = (ct > 0) ? (1. * n_obs * n_for) / ct : GSL_NAN;
    double ets = (cx + cy + cz - cx_rd > 0) ?
      (100. * (cx - cx_rd)) / (cx + cy + cz - cx_rd) : GSL_NAN;
    double rho_p =
      (n > 0) ? gsl_stats_correlation(x, 1, y, 1, (size_t) n) : GSL_NAN;
    double rho_s =
      (n > 0) ? gsl_stats_spearman(x, 1, y, 1, (size_t) n, work) : GSL_NAN;
    for (int i = 0; i < n; i++)
      work[i] = x[i] - y[i];
    double mean = (n > 0) ? gsl_stats_mean(work, 1, (size_t) n) : GSL_NAN;
    double rmse = (n > 0) ? gsl_stats_sd_with_fixed_mean(work, 1, (size_t) n,
							 0.0) : GSL_NAN;
    double absdev =
      (n > 0) ? gsl_stats_absdev_m(work, 1, (size_t) n, 0.0) : GSL_NAN;

    /* Write... */
    fprintf(out, "%.2f %d %d %d %d %d %g %g %g %g %g %g %g %g %g %g %g %d\n",
	    t, cx, cy, cz, n_obs, n_for, bias, pod, far, csi, cx_rd, ets,
	    rho_p, rho_s, mean, rmse, absdev, n);

    /* Set counters to zero... */
    n = ct = cx = cy = cz = 0;
  }

  /* Free... */
  free(modmean);
  free(obsmean);
  free(obscount);

  /* Finalize... */
  if (t == ctl->t_stop) {

    /* Close output file... */
    fclose(out);

    /* Free... */
    free(area);
    free(rt);
    free(rz);
    free(rlon);
    free(rlat);
    free(robs);
  }
}

/*****************************************************************************/

void write_ens(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  static FILE *out;

  static double dummy, lat, lon, qm[NQ][NENS], qs[NQ][NENS], xm[NENS][3],
    x[3], zm[NENS];

  static int n[NENS];

  /* Set timer... */
  SELECT_TIMER("WRITE_ENS", "OUTPUT", NVTX_WRITE);

  /* Check quantities... */
  if (ctl->qnt_ens < 0)
    ERRMSG("Missing ensemble IDs!");

  /* Set time interval... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Init... */
  for (int i = 0; i < NENS; i++) {
    for (int iq = 0; iq < ctl->nq; iq++)
      qm[iq][i] = qs[iq][i] = 0;
    xm[i][0] = xm[i][1] = xm[i][2] = zm[i] = 0;
    n[i] = 0;
  }

  /* Loop over air parcels... */
  for (int ip = 0; ip < atm->np; ip++) {

    /* Check time... */
    if (atm->time[ip] < t0 || atm->time[ip] > t1)
      continue;

    /* Check ensemble ID... */
    if (atm->q[ctl->qnt_ens][ip] < 0 || atm->q[ctl->qnt_ens][ip] >= NENS)
      ERRMSG("Ensemble ID is out of range!");

    /* Get means... */
    geo2cart(0, atm->lon[ip], atm->lat[ip], x);
    for (int iq = 0; iq < ctl->nq; iq++) {
      qm[iq][ctl->qnt_ens] += atm->q[iq][ip];
      qs[iq][ctl->qnt_ens] += SQR(atm->q[iq][ip]);
    }
    xm[ctl->qnt_ens][0] += x[0];
    xm[ctl->qnt_ens][1] += x[1];
    xm[ctl->qnt_ens][2] += x[2];
    zm[ctl->qnt_ens] += Z(atm->p[ip]);
    n[ctl->qnt_ens]++;
  }

  /* Create file... */
  LOG(1, "Write ensemble data: %s", filename);
  if (!(out = fopen(filename, "w")))
    ERRMSG("Cannot create file!");

  /* Write header... */
  fprintf(out,
	  "# $1 = time [s]\n"
	  "# $2 = altitude [km]\n"
	  "# $3 = longitude [deg]\n" "# $4 = latitude [deg]\n");
  for (int iq = 0; iq < ctl->nq; iq++)
    fprintf(out, "# $%d = %s (mean) [%s]\n", 5 + iq,
	    ctl->qnt_name[iq], ctl->qnt_unit[iq]);
  for (int iq = 0; iq < ctl->nq; iq++)
    fprintf(out, "# $%d = %s (sigma) [%s]\n", 5 + ctl->nq + iq,
	    ctl->qnt_name[iq], ctl->qnt_unit[iq]);
  fprintf(out, "# $%d = number of members\n\n", 5 + 2 * ctl->nq);

  /* Write data... */
  for (int i = 0; i < NENS; i++)
    if (n[i] > 0) {
      cart2geo(xm[i], &dummy, &lon, &lat);
      fprintf(out, "%.2f %g %g %g", t, zm[i] / n[i], lon, lat);
      for (int iq = 0; iq < ctl->nq; iq++) {
	fprintf(out, " ");
	fprintf(out, ctl->qnt_format[iq], qm[iq][i] / n[i]);
      }
      for (int iq = 0; iq < ctl->nq; iq++) {
	fprintf(out, " ");
	double var = qs[iq][i] / n[i] - SQR(qm[iq][i] / n[i]);
	fprintf(out, ctl->qnt_format[iq], (var > 0 ? sqrt(var) : 0));
      }
      fprintf(out, " %d\n", n[i]);
    }

  /* Close file... */
  fclose(out);
}

/*****************************************************************************/

void write_grid(
  const char *filename,
  ctl_t * ctl,
  met_t * met0,
  met_t * met1,
  atm_t * atm,
  double t) {

  static double kz[EP], kw[EP];

  static int nk;

  double *cd, *mass, *vmr_expl, *vmr_impl, *z, *lon, *lat, *area, *press;

  int *ixs, *iys, *izs, *np;

  /* Set timer... */
  SELECT_TIMER("WRITE_GRID", "OUTPUT", NVTX_WRITE);

  /* Write info... */
  LOG(1, "Write grid data: %s", filename);

  /* Init... */
  if (t == ctl->t_start) {

    /* Read kernel data... */
    if (ctl->grid_kernel[0] != '-')
      read_kernel(ctl->grid_kernel, kz, kw, &nk);
  }

  /* Allocate... */
  ALLOC(cd, double,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(mass, double,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(vmr_expl, double,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(vmr_impl, double,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(z, double,
	ctl->grid_nz);
  ALLOC(lon, double,
	ctl->grid_nx);
  ALLOC(lat, double,
	ctl->grid_ny);
  ALLOC(area, double,
	ctl->grid_ny);
  ALLOC(press, double,
	ctl->grid_nz);
  ALLOC(np, int,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(ixs, int,
	atm->np);
  ALLOC(iys, int,
	atm->np);
  ALLOC(izs, int,
	atm->np);

  /* Set grid box size... */
  double dz = (ctl->grid_z1 - ctl->grid_z0) / ctl->grid_nz;
  double dlon = (ctl->grid_lon1 - ctl->grid_lon0) / ctl->grid_nx;
  double dlat = (ctl->grid_lat1 - ctl->grid_lat0) / ctl->grid_ny;

  /* Set vertical coordinates... */
#pragma omp parallel for default(shared)
  for (int iz = 0; iz < ctl->grid_nz; iz++) {
    z[iz] = ctl->grid_z0 + dz * (iz + 0.5);
    press[iz] = P(z[iz]);
  }

  /* Set horizontal coordinates... */
  for (int ix = 0; ix < ctl->grid_nx; ix++)
    lon[ix] = ctl->grid_lon0 + dlon * (ix + 0.5);
#pragma omp parallel for default(shared)
  for (int iy = 0; iy < ctl->grid_ny; iy++) {
    lat[iy] = ctl->grid_lat0 + dlat * (iy + 0.5);
    area[iy] = dlat * dlon * SQR(RE * M_PI / 180.)
      * cos(lat[iy] * M_PI / 180.);
  }

  /* Set time interval for output... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Get grid box indices... */
#pragma omp parallel for default(shared)
  for (int ip = 0; ip < atm->np; ip++) {
    ixs[ip] = (int) ((atm->lon[ip] - ctl->grid_lon0) / dlon);
    iys[ip] = (int) ((atm->lat[ip] - ctl->grid_lat0) / dlat);
    izs[ip] = (int) ((Z(atm->p[ip]) - ctl->grid_z0) / dz);
    if (atm->time[ip] < t0 || atm->time[ip] > t1
	|| ixs[ip] < 0 || ixs[ip] >= ctl->grid_nx
	|| iys[ip] < 0 || iys[ip] >= ctl->grid_ny
	|| izs[ip] < 0 || izs[ip] >= ctl->grid_nz)
      izs[ip] = -1;
  }

  /* Average data... */
  for (int ip = 0; ip < atm->np; ip++)
    if (izs[ip] >= 0) {
      int idx =
	ARRAY_3D(ixs[ip], iys[ip], ctl->grid_ny, izs[ip], ctl->grid_nz);
      np[idx]++;
      if (ctl->qnt_m >= 0)
	mass[idx] += kernel_weight(kz, kw, nk, atm->p[ip])
	  * atm->q[ctl->qnt_m][ip];
      if (ctl->qnt_vmr >= 0)
	vmr_expl[idx] += kernel_weight(kz, kw, nk, atm->p[ip])
	  * atm->q[ctl->qnt_vmr][ip];
    }

  /* Calculate column density and vmr... */
#pragma omp parallel for default(shared)
  for (int ix = 0; ix < ctl->grid_nx; ix++)
    for (int iy = 0; iy < ctl->grid_ny; iy++)
      for (int iz = 0; iz < ctl->grid_nz; iz++) {

	/* Get grid index... */
	int idx = ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz);

	/* Calculate column density... */
	cd[idx] = GSL_NAN;
	if (ctl->qnt_m >= 0)
	  cd[idx] = mass[idx] / (1e6 * area[iy]);

	/* Calculate volume mixing ratio (implicit)... */
	vmr_impl[idx] = GSL_NAN;
	if (ctl->qnt_m >= 0 && ctl->molmass > 0) {
	  vmr_impl[idx] = 0;
	  if (mass[idx] > 0) {

	    /* Get temperature... */
	    double temp;
	    INTPOL_INIT;
	    intpol_met_time_3d(met0, met0->t, met1, met1->t, t, press[iz],
			       lon[ix], lat[iy], &temp, ci, cw, 1);

	    /* Calculate volume mixing ratio... */
	    vmr_impl[idx] = MA / ctl->molmass * mass[idx]
	      / (RHO(press[iz], temp) * 1e6 * area[iy] * 1e3 * dz);
	  }
	}

	/* Calculate volume mixing ratio (explicit)... */
	if (ctl->qnt_vmr >= 0 && np[idx] > 0)
	  vmr_expl[idx] /= np[idx];
	else
	  vmr_expl[idx] = GSL_NAN;
      }

  /* Write ASCII data... */
  if (ctl->grid_type == 0)
    write_grid_asc(filename, ctl, cd, vmr_expl, vmr_impl,
		   t, z, lon, lat, area, dz, np);

  /* Write netCDF data... */
  else if (ctl->grid_type == 1)
    write_grid_nc(filename, ctl, cd, vmr_expl, vmr_impl,
		  t, z, lon, lat, area, dz, np);

  /* Error message... */
  else
    ERRMSG("Grid data format GRID_TYPE unknown!");

  /* Free... */
  free(cd);
  free(mass);
  free(vmr_expl);
  free(vmr_impl);
  free(z);
  free(lon);
  free(lat);
  free(area);
  free(press);
  free(np);
  free(ixs);
  free(iys);
  free(izs);
}

/*****************************************************************************/

void write_grid_asc(
  const char *filename,
  ctl_t * ctl,
  double *cd,
  double *vmr_expl,
  double *vmr_impl,
  double t,
  double *z,
  double *lon,
  double *lat,
  double *area,
  double dz,
  int *np) {

  FILE *in, *out;

  char line[LEN];

  /* Check if gnuplot output is requested... */
  if (ctl->grid_gpfile[0] != '-') {

    /* Create gnuplot pipe... */
    if (!(out = popen("gnuplot", "w")))
      ERRMSG("Cannot create pipe to gnuplot!");

    /* Set plot filename... */
    fprintf(out, "set out \"%s.png\"\n", filename);

    /* Set time string... */
    double r;
    int year, mon, day, hour, min, sec;
    jsec2time(t, &year, &mon, &day, &hour, &min, &sec, &r);
    fprintf(out, "timestr=\"%d-%02d-%02d, %02d:%02d UTC\"\n",
	    year, mon, day, hour, min);

    /* Dump gnuplot file to pipe... */
    if (!(in = fopen(ctl->grid_gpfile, "r")))
      ERRMSG("Cannot open file!");
    while (fgets(line, LEN, in))
      fprintf(out, "%s", line);
    fclose(in);
  }

  else {

    /* Create file... */
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");
  }

  /* Write header... */
  fprintf(out,
	  "# $1 = time [s]\n"
	  "# $2 = altitude [km]\n"
	  "# $3 = longitude [deg]\n"
	  "# $4 = latitude [deg]\n"
	  "# $5 = surface area [km^2]\n"
	  "# $6 = layer depth [km]\n"
	  "# $7 = number of particles [1]\n"
	  "# $8 = column density (implicit) [kg/m^2]\n"
	  "# $9 = volume mixing ratio (implicit) [ppv]\n"
	  "# $10 = volume mixing ratio (explicit) [ppv]\n\n");

  /* Write data... */
  for (int ix = 0; ix < ctl->grid_nx; ix++) {
    if (ix > 0 && ctl->grid_ny > 1 && !ctl->grid_sparse)
      fprintf(out, "\n");
    for (int iy = 0; iy < ctl->grid_ny; iy++) {
      if (iy > 0 && ctl->grid_nz > 1 && !ctl->grid_sparse)
	fprintf(out, "\n");
      for (int iz = 0; iz < ctl->grid_nz; iz++) {
	int idx = ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz);
	if (!ctl->grid_sparse || vmr_expl[idx] > 0 || vmr_impl[idx] > 0)
	  fprintf(out, "%.2f %g %g %g %g %g %d %g %g %g\n",
		  t, z[iz], lon[ix], lat[iy], area[iy], dz,
		  np[idx], cd[idx], vmr_impl[idx], vmr_expl[idx]);
      }
    }
  }

  /* Close file... */
  fclose(out);
}

/*****************************************************************************/

void write_grid_nc(
  const char *filename,
  ctl_t * ctl,
  double *cd,
  double *vmr_expl,
  double *vmr_impl,
  double t,
  double *z,
  double *lon,
  double *lat,
  double *area,
  double dz,
  int *np) {

  double *help;

  int *help2, ncid, dimid[10], varid;

  size_t start[2], count[2];

  /* Allocate... */
  ALLOC(help, double,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);
  ALLOC(help2, int,
	ctl->grid_nx * ctl->grid_ny * ctl->grid_nz);

  /* Create file... */
  NC(nc_create(filename, NC_CLOBBER, &ncid));

  /* Define dimensions... */
  NC(nc_def_dim(ncid, "time", 1, &dimid[0]));
  NC(nc_def_dim(ncid, "z", (size_t) ctl->grid_nz, &dimid[1]));
  NC(nc_def_dim(ncid, "lat", (size_t) ctl->grid_ny, &dimid[2]));
  NC(nc_def_dim(ncid, "lon", (size_t) ctl->grid_nx, &dimid[3]));
  NC(nc_def_dim(ncid, "dz", 1, &dimid[4]));

  /* Define variables and their attributes... */
  NC_DEF_VAR("time", NC_DOUBLE, 1, &dimid[0], "time",
	     "seconds since 2000-01-01 00:00:00 UTC");
  NC_DEF_VAR("z", NC_DOUBLE, 1, &dimid[1], "altitude", "km");
  NC_DEF_VAR("lat", NC_DOUBLE, 1, &dimid[2], "latitude", "degrees_north");
  NC_DEF_VAR("lon", NC_DOUBLE, 1, &dimid[3], "longitude", "degrees_east");
  NC_DEF_VAR("dz", NC_DOUBLE, 1, &dimid[1], "layer depth", "km");
  NC_DEF_VAR("area", NC_DOUBLE, 1, &dimid[2], "surface area", "km**2");
  NC_DEF_VAR("cd", NC_FLOAT, 4, dimid, "column density", "kg m**-2");
  NC_DEF_VAR("vmr_impl", NC_FLOAT, 4, dimid,
	     "volume mixing ratio (implicit)", "ppv");
  NC_DEF_VAR("vmr_expl", NC_FLOAT, 4, dimid,
	     "volume mixing ratio (explicit)", "ppv");
  NC_DEF_VAR("np", NC_INT, 4, dimid, "number of particles", "1");

  /* End definitions... */
  NC(nc_enddef(ncid));

  /* Write data... */
  NC_PUT_DOUBLE("time", &t, 0);
  NC_PUT_DOUBLE("lon", lon, 0);
  NC_PUT_DOUBLE("lat", lat, 0);
  NC_PUT_DOUBLE("z", z, 0);
  NC_PUT_DOUBLE("area", area, 0);
  NC_PUT_DOUBLE("dz", &dz, 0);

  for (int ix = 0; ix < ctl->grid_nx; ix++)
    for (int iy = 0; iy < ctl->grid_ny; iy++)
      for (int iz = 0; iz < ctl->grid_nz; iz++)
	help[ARRAY_3D(iz, iy, ctl->grid_ny, ix, ctl->grid_nx)] =
	  cd[ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz)];
  NC_PUT_DOUBLE("cd", help, 0);

  for (int ix = 0; ix < ctl->grid_nx; ix++)
    for (int iy = 0; iy < ctl->grid_ny; iy++)
      for (int iz = 0; iz < ctl->grid_nz; iz++)
	help[ARRAY_3D(iz, iy, ctl->grid_ny, ix, ctl->grid_nx)] =
	  vmr_impl[ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz)];
  NC_PUT_DOUBLE("vmr_impl", help, 0);

  for (int ix = 0; ix < ctl->grid_nx; ix++)
    for (int iy = 0; iy < ctl->grid_ny; iy++)
      for (int iz = 0; iz < ctl->grid_nz; iz++)
	help[ARRAY_3D(iz, iy, ctl->grid_ny, ix, ctl->grid_nx)] =
	  vmr_expl[ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz)];
  NC_PUT_DOUBLE("vmr_expl", help, 0);

  for (int ix = 0; ix < ctl->grid_nx; ix++)
    for (int iy = 0; iy < ctl->grid_ny; iy++)
      for (int iz = 0; iz < ctl->grid_nz; iz++)
	help2[ARRAY_3D(iz, iy, ctl->grid_ny, ix, ctl->grid_nx)] =
	  np[ARRAY_3D(ix, iy, ctl->grid_ny, iz, ctl->grid_nz)];
  NC_PUT_INT("np", help2, 0);

  /* Close file... */
  NC(nc_close(ncid));

  /* Free... */
  free(help);
  free(help2);
}

/*****************************************************************************/

int write_met(
  char *filename,
  ctl_t * ctl,
  met_t * met) {

  /* Set timer... */
  SELECT_TIMER("WRITE_MET", "OUTPUT", NVTX_WRITE);

  /* Write info... */
  LOG(1, "Write meteo data: %s", filename);

  /* Check compression flags... */
#ifndef ZFP
  if (ctl->met_type == 3)
    ERRMSG("zfp compression not supported!");
#endif
#ifndef ZSTD
  if (ctl->met_type == 4)
    ERRMSG("zstd compression not supported!");
#endif

  /* Write binary data... */
  if (ctl->met_type >= 1 && ctl->met_type <= 4) {

    /* Create file... */
    FILE *out;
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");

    /* Write type of binary data... */
    FWRITE(&ctl->met_type, int,
	   1,
	   out);

    /* Write version of binary data... */
    int version = 101;
    FWRITE(&version, int,
	   1,
	   out);

    /* Write grid data... */
    FWRITE(&met->time, double,
	   1,
	   out);
    FWRITE(&met->nx, int,
	   1,
	   out);
    FWRITE(&met->ny, int,
	   1,
	   out);
    FWRITE(&met->np, int,
	   1,
	   out);
    FWRITE(met->lon, double,
	     (size_t) met->nx,
	   out);
    FWRITE(met->lat, double,
	     (size_t) met->ny,
	   out);
    FWRITE(met->p, double,
	     (size_t) met->np,
	   out);

    /* Write surface data... */
    write_met_bin_2d(out, met, met->ps, "PS");
    write_met_bin_2d(out, met, met->ts, "TS");
    write_met_bin_2d(out, met, met->zs, "ZS");
    write_met_bin_2d(out, met, met->us, "US");
    write_met_bin_2d(out, met, met->vs, "VS");
    write_met_bin_2d(out, met, met->lsm, "LSM");
    write_met_bin_2d(out, met, met->sst, "SST");
    write_met_bin_2d(out, met, met->pbl, "PBL");
    write_met_bin_2d(out, met, met->pt, "PT");
    write_met_bin_2d(out, met, met->tt, "TT");
    write_met_bin_2d(out, met, met->zt, "ZT");
    write_met_bin_2d(out, met, met->h2ot, "H2OT");
    write_met_bin_2d(out, met, met->pct, "PCT");
    write_met_bin_2d(out, met, met->pcb, "PCB");
    write_met_bin_2d(out, met, met->cl, "CL");
    write_met_bin_2d(out, met, met->plcl, "PLCL");
    write_met_bin_2d(out, met, met->plfc, "PLFC");
    write_met_bin_2d(out, met, met->pel, "PEL");
    write_met_bin_2d(out, met, met->cape, "CAPE");
    write_met_bin_2d(out, met, met->cin, "CIN");

    /* Write level data... */
    write_met_bin_3d(out, ctl, met, met->z, "Z", 0, 0.5);
    write_met_bin_3d(out, ctl, met, met->t, "T", 0, 5.0);
    write_met_bin_3d(out, ctl, met, met->u, "U", 8, 0);
    write_met_bin_3d(out, ctl, met, met->v, "V", 8, 0);
    write_met_bin_3d(out, ctl, met, met->w, "W", 8, 0);
    write_met_bin_3d(out, ctl, met, met->pv, "PV", 8, 0);
    write_met_bin_3d(out, ctl, met, met->h2o, "H2O", 8, 0);
    write_met_bin_3d(out, ctl, met, met->o3, "O3", 8, 0);
    write_met_bin_3d(out, ctl, met, met->lwc, "LWC", 8, 0);
    write_met_bin_3d(out, ctl, met, met->iwc, "IWC", 8, 0);
    write_met_bin_3d(out, ctl, met, met->cc, "CC", 8, 0);

    /* Write final flag... */
    int final = 999;
    FWRITE(&final, int,
	   1,
	   out);

    /* Close file... */
    fclose(out);
  }

  return 0;
}

/*****************************************************************************/

void write_met_bin_2d(
  FILE * out,
  met_t * met,
  float var[EX][EY],
  char *varname) {

  float *help;

  /* Allocate... */
  ALLOC(help, float,
	EX * EY);

  /* Copy data... */
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      help[ARRAY_2D(ix, iy, met->ny)] = var[ix][iy];

  /* Write uncompressed data... */
  LOG(2, "Write 2-D variable: %s (uncompressed)", varname);
  FWRITE(help, float,
	   (size_t) (met->nx * met->ny),
	 out);

  /* Free... */
  free(help);
}

/*****************************************************************************/

void write_met_bin_3d(
  FILE * out,
  ctl_t * ctl,
  met_t * met,
  float var[EX][EY][EP],
  char *varname,
  int precision,
  double tolerance) {

  float *help;

  /* Allocate... */
  ALLOC(help, float,
	EX * EY * EP);

  /* Copy data... */
#pragma omp parallel for default(shared) collapse(2)
  for (int ix = 0; ix < met->nx; ix++)
    for (int iy = 0; iy < met->ny; iy++)
      for (int ip = 0; ip < met->np; ip++)
	help[ARRAY_3D(ix, iy, met->ny, ip, met->np)] = var[ix][iy][ip];

  /* Write uncompressed data... */
  if (ctl->met_type == 1) {
    LOG(2, "Write 3-D variable: %s (uncompressed)", varname);
    FWRITE(help, float,
	     (size_t) (met->nx * met->ny * met->np),
	   out);
  }

  /* Write packed data... */
  else if (ctl->met_type == 2)
    compress_pack(varname, help, (size_t) (met->ny * met->nx),
		  (size_t) met->np, 0, out);

  /* Write zfp data... */
#ifdef ZFP
  else if (ctl->met_type == 3)
    compress_zfp(varname, help, met->np, met->ny, met->nx, precision,
		 tolerance, 0, out);
#endif

  /* Write zstd data... */
#ifdef ZSTD
  else if (ctl->met_type == 4)
    compress_zstd(varname, help, (size_t) (met->np * met->ny * met->nx), 0,
		  out);
#endif

  /* Unknown method... */
  else {
    ERRMSG("MET_TYPE not supported!");
    LOG(3, "%d %g", precision, tolerance);
  }

  /* Free... */
  free(help);
}

/*****************************************************************************/

void write_prof(
  const char *filename,
  ctl_t * ctl,
  met_t * met0,
  met_t * met1,
  atm_t * atm,
  double t) {

  static FILE *out;

  static double *mass, *obsmean, *rt, *rz, *rlon, *rlat, *robs, *area,
    dz, dlon, dlat, *lon, *lat, *z, *press, temp, vmr, h2o, o3;

  static int nobs, *obscount, ip, okay;

  /* Set timer... */
  SELECT_TIMER("WRITE_PROF", "OUTPUT", NVTX_WRITE);

  /* Init... */
  if (t == ctl->t_start) {

    /* Check quantity index for mass... */
    if (ctl->qnt_m < 0)
      ERRMSG("Need quantity mass!");

    /* Check molar mass... */
    if (ctl->molmass <= 0)
      ERRMSG("Specify molar mass!");

    /* Allocate... */
    ALLOC(lon, double,
	  ctl->prof_nx);
    ALLOC(lat, double,
	  ctl->prof_ny);
    ALLOC(area, double,
	  ctl->prof_ny);
    ALLOC(z, double,
	  ctl->prof_nz);
    ALLOC(press, double,
	  ctl->prof_nz);
    ALLOC(rt, double,
	  NOBS);
    ALLOC(rz, double,
	  NOBS);
    ALLOC(rlon, double,
	  NOBS);
    ALLOC(rlat, double,
	  NOBS);
    ALLOC(robs, double,
	  NOBS);

    /* Read observation data... */
    read_obs(ctl->prof_obsfile, rt, rz, rlon, rlat, robs, &nobs);

    /* Create new output file... */
    LOG(1, "Write profile data: %s", filename);
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");

    /* Write header... */
    fprintf(out,
	    "# $1 = time [s]\n"
	    "# $2 = altitude [km]\n"
	    "# $3 = longitude [deg]\n"
	    "# $4 = latitude [deg]\n"
	    "# $5 = pressure [hPa]\n"
	    "# $6 = temperature [K]\n"
	    "# $7 = volume mixing ratio [ppv]\n"
	    "# $8 = H2O volume mixing ratio [ppv]\n"
	    "# $9 = O3 volume mixing ratio [ppv]\n"
	    "# $10 = observed BT index [K]\n"
	    "# $11 = number of observations\n");

    /* Set grid box size... */
    dz = (ctl->prof_z1 - ctl->prof_z0) / ctl->prof_nz;
    dlon = (ctl->prof_lon1 - ctl->prof_lon0) / ctl->prof_nx;
    dlat = (ctl->prof_lat1 - ctl->prof_lat0) / ctl->prof_ny;

    /* Set vertical coordinates... */
    for (int iz = 0; iz < ctl->prof_nz; iz++) {
      z[iz] = ctl->prof_z0 + dz * (iz + 0.5);
      press[iz] = P(z[iz]);
    }

    /* Set horizontal coordinates... */
    for (int ix = 0; ix < ctl->prof_nx; ix++)
      lon[ix] = ctl->prof_lon0 + dlon * (ix + 0.5);
    for (int iy = 0; iy < ctl->prof_ny; iy++) {
      lat[iy] = ctl->prof_lat0 + dlat * (iy + 0.5);
      area[iy] = dlat * dlon * SQR(RE * M_PI / 180.)
	* cos(lat[iy] * M_PI / 180.);
    }
  }

  /* Set time interval... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Allocate... */
  ALLOC(mass, double,
	ctl->prof_nx * ctl->prof_ny * ctl->prof_nz);
  ALLOC(obsmean, double,
	ctl->prof_nx * ctl->prof_ny);
  ALLOC(obscount, int,
	ctl->prof_nx * ctl->prof_ny);

  /* Loop over observations... */
  for (int i = 0; i < nobs; i++) {

    /* Check time... */
    if (rt[i] < t0)
      continue;
    else if (rt[i] >= t1)
      break;

    /* Check observation data... */
    if (!isfinite(robs[i]))
      continue;

    /* Calculate indices... */
    int ix = (int) ((rlon[i] - ctl->prof_lon0) / dlon);
    int iy = (int) ((rlat[i] - ctl->prof_lat0) / dlat);

    /* Check indices... */
    if (ix < 0 || ix >= ctl->prof_nx || iy < 0 || iy >= ctl->prof_ny)
      continue;

    /* Get mean observation index... */
    int idx = ARRAY_2D(ix, iy, ctl->prof_ny);
    obsmean[idx] += robs[i];
    obscount[idx]++;
  }

  /* Analyze model data... */
  for (ip = 0; ip < atm->np; ip++) {

    /* Check time... */
    if (atm->time[ip] < t0 || atm->time[ip] > t1)
      continue;

    /* Get indices... */
    int ix = (int) ((atm->lon[ip] - ctl->prof_lon0) / dlon);
    int iy = (int) ((atm->lat[ip] - ctl->prof_lat0) / dlat);
    int iz = (int) ((Z(atm->p[ip]) - ctl->prof_z0) / dz);

    /* Check indices... */
    if (ix < 0 || ix >= ctl->prof_nx ||
	iy < 0 || iy >= ctl->prof_ny || iz < 0 || iz >= ctl->prof_nz)
      continue;

    /* Get total mass in grid cell... */
    int idx = ARRAY_3D(ix, iy, ctl->prof_ny, iz, ctl->prof_nz);
    mass[idx] += atm->q[ctl->qnt_m][ip];
  }

  /* Extract profiles... */
  for (int ix = 0; ix < ctl->prof_nx; ix++)
    for (int iy = 0; iy < ctl->prof_ny; iy++) {
      int idx2 = ARRAY_2D(ix, iy, ctl->prof_ny);
      if (obscount[idx2] > 0) {

	/* Check profile... */
	okay = 0;
	for (int iz = 0; iz < ctl->prof_nz; iz++) {
	  int idx3 = ARRAY_3D(ix, iy, ctl->prof_ny, iz, ctl->prof_nz);
	  if (mass[idx3] > 0) {
	    okay = 1;
	    break;
	  }
	}
	if (!okay)
	  continue;

	/* Write output... */
	fprintf(out, "\n");

	/* Loop over altitudes... */
	for (int iz = 0; iz < ctl->prof_nz; iz++) {

	  /* Get temperature, water vapor, and ozone... */
	  INTPOL_INIT;
	  intpol_met_time_3d(met0, met0->t, met1, met1->t, t, press[iz],
			     lon[ix], lat[iy], &temp, ci, cw, 1);
	  intpol_met_time_3d(met0, met0->h2o, met1, met1->h2o, t, press[iz],
			     lon[ix], lat[iy], &h2o, ci, cw, 0);
	  intpol_met_time_3d(met0, met0->o3, met1, met1->o3, t, press[iz],
			     lon[ix], lat[iy], &o3, ci, cw, 0);

	  /* Calculate volume mixing ratio... */
	  int idx3 = ARRAY_3D(ix, iy, ctl->prof_ny, iz, ctl->prof_nz);
	  vmr = MA / ctl->molmass * mass[idx3]
	    / (RHO(press[iz], temp) * area[iy] * dz * 1e9);

	  /* Write output... */
	  fprintf(out, "%.2f %g %g %g %g %g %g %g %g %g %d\n",
		  t, z[iz], lon[ix], lat[iy], press[iz], temp, vmr, h2o, o3,
		  obsmean[idx2] / obscount[idx2], obscount[idx2]);
	}
      }
    }

  /* Free... */
  free(mass);
  free(obsmean);
  free(obscount);

  /* Finalize... */
  if (t == ctl->t_stop) {

    /* Close output file... */
    fclose(out);

    /* Free... */
    free(lon);
    free(lat);
    free(area);
    free(z);
    free(press);
    free(rt);
    free(rz);
    free(rlon);
    free(rlat);
    free(robs);
  }
}

/*****************************************************************************/

void write_sample(
  const char *filename,
  ctl_t * ctl,
  met_t * met0,
  met_t * met1,
  atm_t * atm,
  double t) {

  static FILE *out;

  static double area, dlat, rmax2, *rt, *rz, *rlon, *rlat, *robs, kz[EP],
    kw[EP];

  static int nobs, nk;

  /* Set timer... */
  SELECT_TIMER("WRITE_SAMPLE", "OUTPUT", NVTX_WRITE);

  /* Init... */
  if (t == ctl->t_start) {

    /* Allocate... */
    ALLOC(rt, double,
	  NOBS);
    ALLOC(rz, double,
	  NOBS);
    ALLOC(rlon, double,
	  NOBS);
    ALLOC(rlat, double,
	  NOBS);
    ALLOC(robs, double,
	  NOBS);

    /* Read observation data... */
    read_obs(ctl->sample_obsfile, rt, rz, rlon, rlat, robs, &nobs);

    /* Read kernel data... */
    if (ctl->sample_kernel[0] != '-')
      read_kernel(ctl->sample_kernel, kz, kw, &nk);

    /* Create output file... */
    LOG(1, "Write sample data: %s", filename);
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");

    /* Write header... */
    fprintf(out,
	    "# $1 = time [s]\n"
	    "# $2 = altitude [km]\n"
	    "# $3 = longitude [deg]\n"
	    "# $4 = latitude [deg]\n"
	    "# $5 = surface area [km^2]\n"
	    "# $6 = layer depth [km]\n"
	    "# $7 = number of particles [1]\n"
	    "# $8 = column density [kg/m^2]\n"
	    "# $9 = volume mixing ratio [ppv]\n"
	    "# $10 = observed BT index [K]\n\n");

    /* Set latitude range, squared radius, and area... */
    dlat = DY2DEG(ctl->sample_dx);
    rmax2 = SQR(ctl->sample_dx);
    area = M_PI * rmax2;
  }

  /* Set time interval for output... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Loop over observations... */
  for (int i = 0; i < nobs; i++) {

    /* Check time... */
    if (rt[i] < t0)
      continue;
    else if (rt[i] >= t1)
      break;

    /* Calculate Cartesian coordinates... */
    double x0[3];
    geo2cart(0, rlon[i], rlat[i], x0);

    /* Set pressure range... */
    double rp = P(rz[i]);
    double ptop = P(rz[i] + ctl->sample_dz);
    double pbot = P(rz[i] - ctl->sample_dz);

    /* Init... */
    double mass = 0;
    int np = 0;

    /* Loop over air parcels... */
#pragma omp parallel for default(shared) reduction(+:mass,np)
    for (int ip = 0; ip < atm->np; ip++) {

      /* Check time... */
      if (atm->time[ip] < t0 || atm->time[ip] > t1)
	continue;

      /* Check latitude... */
      if (fabs(rlat[i] - atm->lat[ip]) > dlat)
	continue;

      /* Check horizontal distance... */
      double x1[3];
      geo2cart(0, atm->lon[ip], atm->lat[ip], x1);
      if (DIST2(x0, x1) > rmax2)
	continue;

      /* Check pressure... */
      if (ctl->sample_dz > 0)
	if (atm->p[ip] > pbot || atm->p[ip] < ptop)
	  continue;

      /* Add mass... */
      if (ctl->qnt_m >= 0)
	mass +=
	  kernel_weight(kz, kw, nk, atm->p[ip]) * atm->q[ctl->qnt_m][ip];
      np++;
    }

    /* Calculate column density... */
    double cd = mass / (1e6 * area);

    /* Calculate volume mixing ratio... */
    double vmr = 0;
    if (ctl->molmass > 0 && ctl->sample_dz > 0) {
      if (mass > 0) {

	/* Get temperature... */
	double temp;
	INTPOL_INIT;
	intpol_met_time_3d(met0, met0->t, met1, met1->t, rt[i], rp,
			   rlon[i], rlat[i], &temp, ci, cw, 1);

	/* Calculate volume mixing ratio... */
	vmr = MA / ctl->molmass * mass
	  / (RHO(rp, temp) * 1e6 * area * 1e3 * ctl->sample_dz);
      }
    } else
      vmr = GSL_NAN;

    /* Write output... */
    fprintf(out, "%.2f %g %g %g %g %g %d %g %g %g\n", rt[i], rz[i],
	    rlon[i], rlat[i], area, ctl->sample_dz, np, cd, vmr, robs[i]);
  }

  /* Finalize...... */
  if (t == ctl->t_stop) {

    /* Close output file... */
    fclose(out);

    /* Free... */
    free(rt);
    free(rz);
    free(rlon);
    free(rlat);
    free(robs);
  }
}

/*****************************************************************************/

void write_station(
  const char *filename,
  ctl_t * ctl,
  atm_t * atm,
  double t) {

  static FILE *out;

  static double rmax2, x0[3], x1[3];

  /* Set timer... */
  SELECT_TIMER("WRITE_STATION", "OUTPUT", NVTX_WRITE);

  /* Init... */
  if (t == ctl->t_start) {

    /* Write info... */
    LOG(1, "Write station data: %s", filename);

    /* Create new file... */
    if (!(out = fopen(filename, "w")))
      ERRMSG("Cannot create file!");

    /* Write header... */
    fprintf(out,
	    "# $1 = time [s]\n"
	    "# $2 = altitude [km]\n"
	    "# $3 = longitude [deg]\n" "# $4 = latitude [deg]\n");
    for (int iq = 0; iq < ctl->nq; iq++)
      fprintf(out, "# $%i = %s [%s]\n", (iq + 5),
	      ctl->qnt_name[iq], ctl->qnt_unit[iq]);
    fprintf(out, "\n");

    /* Set geolocation and search radius... */
    geo2cart(0, ctl->stat_lon, ctl->stat_lat, x0);
    rmax2 = SQR(ctl->stat_r);
  }

  /* Set time interval for output... */
  double t0 = t - 0.5 * ctl->dt_mod;
  double t1 = t + 0.5 * ctl->dt_mod;

  /* Loop over air parcels... */
  for (int ip = 0; ip < atm->np; ip++) {

    /* Check time... */
    if (atm->time[ip] < t0 || atm->time[ip] > t1)
      continue;

    /* Check time range for station output... */
    if (atm->time[ip] < ctl->stat_t0 || atm->time[ip] > ctl->stat_t1)
      continue;

    /* Check station flag... */
    if (ctl->qnt_stat >= 0)
      if (atm->q[ctl->qnt_stat][ip])
	continue;

    /* Get Cartesian coordinates... */
    geo2cart(0, atm->lon[ip], atm->lat[ip], x1);

    /* Check horizontal distance... */
    if (DIST2(x0, x1) > rmax2)
      continue;

    /* Set station flag... */
    if (ctl->qnt_stat >= 0)
      atm->q[ctl->qnt_stat][ip] = 1;

    /* Write data... */
    fprintf(out, "%.2f %g %g %g",
	    atm->time[ip], Z(atm->p[ip]), atm->lon[ip], atm->lat[ip]);
    for (int iq = 0; iq < ctl->nq; iq++) {
      fprintf(out, " ");
      fprintf(out, ctl->qnt_format[iq], atm->q[iq][ip]);
    }
    fprintf(out, "\n");
  }

  /* Close file... */
  if (t == ctl->t_stop)
    fclose(out);
}
