/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2010, Linköpings University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF THIS OSMC PUBLIC
 * LICENSE (OSMC-PL). ANY USE, REPRODUCTION OR DISTRIBUTION OF
 * THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE OF THE OSMC
 * PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköpings University, either from the above address,
 * from the URL: http://www.ida.liu.se/projects/OpenModelica
 * and in the OpenModelica distribution.
 *
 * This program is distributed  WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

/*! \file initialization.c
 */

#include "initialization.h"
#include "simulation_data.h"
#include "omc_error.h"
#include "openmodelica.h"
#include "openmodelica_func.h"
#include "model_help.h"
#include "read_matlab4.h"
#include "events.h"

#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <kinsol/kinsol.h>
#include <kinsol/kinsol_dense.h>
#include <nvector/nvector_serial.h>
#include <sundials/sundials_types.h>
#include <sundials/sundials_math.h>

DATA *globalData = NULL;
double* globalInitialResiduals = NULL;

typedef struct INIT_DATA
{
  long nz;
  long nStates;
  long nParameters;
  double* z;
  double* start;
  double* min;
  double* max;
  double* nominal;
  char** name;
}INIT_DATA;

typedef struct KINSOL_DATA
{
  INIT_DATA* initData;
  DATA* data;
}KINSOL_DATA;

#ifndef NEWUOA
#define NEWUOA newuoa_
#endif

#ifndef NELMEAD
#define NELMEAD nelmead_
#endif

void NEWUOA(long *nz,
  long *NPT,
  double *z,
  double *RHOBEG,
  double *RHOEND,
  long *IPRINT,
  long *MAXFUN,
  double *W,
  void (*leastSquare) (long *nz, double *z, double *funcValue));

void NELMEAD(double *z,
  double *STEP,
  long *nz,
  double *funcValue,
  long *MAXF,
  long *IPRINT,
  double *STOPCR,
  long *NLOOP,
  long *IQUAD,
  double *SIMP,
  double *VAR,
  void (*leastSquare) (long *nz, double *z, double *funcValue),
  long *IFAULT);

enum INIT_INIT_METHOD
{
  IIM_UNKNOWN = 0,
  IIM_NONE,
  IIM_STATE,
  IIM_MAX
};

const char *initMethodStr[IIM_MAX] = {"unknown", "none", "state"};
const char *initMethodDescStr[IIM_MAX] = {"unknown", "no initialization method", "default initialization method"};

enum INIT_OPTI_METHOD
{
  IOM_UNKNOWN = 0,
  IOM_NELDER_MEAD_EX,
  IOM_NELDER_MEAD_EX2,
  IOM_SIMPLEX,
  IOM_NEWUOA,
  IOM_KINSOL,
  IOM_MAX
};

const char *optiMethodStr[IOM_MAX] = {"unknown", "nelder_mead_ex", "nelder_mead_ex2", "simplex", "newuoa", "kinsol"};
const char *optiMethodDescStr[IOM_MAX] = {"unknown", "with global homotopy", "without global homotopy", "", "brent's method", "sundials/kinsol"};

/*! \fn leastSquareWithLambda
 *
 *  This function calculates the residual value
 *  as the sum of squared residual equations.
 *
 *  \param [ref] [data]
 *  \param [in]  [nz] number of unfixed states and unfixed parameters
 *  \param [in]  [z] vector of unfixed states and unfixed parameters
 *  \param [in]  [zNominal] vector of nominal-values for z or NULL
 *  \param [in]  [initialResidualScalingCoefficients] vector of scaling-coefficients for initial_residuals or NULL
 *  \param [in]  [lambda] E [0; 1]
 *  \param [out] [initialResiduals]
 */
static double leastSquareWithLambda(DATA* data, long nz, double* z, double* zNominal, double* initialResidualScalingCoefficients, double lambda, double* initialResiduals)
{
  int indz = 0;
  fortran_integer i = 0;
  long j = 0;
  double funcValue = 0.0;
  double scalingCoefficient;

  for(i=0; i<data->modelData.nStates; ++i)
  {
    if(data->modelData.realVarsData[i].attribute.fixed==0)
    {
      data->localData[0]->realVars[i] = z[indz] * (zNominal ? zNominal[indz] : 1.0);
      indz++;
    }
  }

  /* for real parameters */
  for(i=0; i<data->modelData.nParametersReal; ++i)
  {
    if(data->modelData.realParameterData[i].attribute.fixed == 0)
    {
      data->simulationInfo.realParameter[i] = z[indz] * (zNominal ? zNominal[indz] : 1.0);
      indz++;
    }
  }

  updateBoundParameters(data);
  functionODE(data);
  functionAlgebraics(data);
  initial_residual(data, lambda, initialResiduals);

  if (initialResidualScalingCoefficients)
  {
    /* use scaling coefficients */
    for(j=0; j<data->modelData.nResiduals; ++j)
    {
      scalingCoefficient = initialResidualScalingCoefficients[j];
      if(scalingCoefficient > 0.0)
        funcValue += (initialResiduals[j] / scalingCoefficient) * (initialResiduals[j] / scalingCoefficient);
    }
  }
  else
  {
    /* no scaling coefficients given */
    for(j=0; j<data->modelData.nResiduals; ++j)
    {
      funcValue += initialResiduals[j] * initialResiduals[j];
    }
  }

  return funcValue;
}

/*! \fn void leastSquare(long nz, double *z, double *funcValue)
*
*  This function calculates the residual value
*  as the sum of squared residual equations.
*
*  \param nz [in] number of variables
*  \param z [in] vector of variables
*  \param funcValue [out] result
*/
void leastSquare(long *nz, double *z, double *funcValue)
{
  *funcValue = leastSquareWithLambda(globalData, *nz, z, NULL, NULL, 1.0, globalInitialResiduals);

  DEBUG_INFO1(LOG_INIT, "leastSquare | leastSquare-Value: %g", *funcValue);
}

/*! \fn kinsol_residuals
 *
 *  \param [in]  [z]
 *  \param [out] [f]
 *  \param [ref] [user_data]
 *
 *  \author lochel
 */
static int kinsol_residuals(N_Vector z, N_Vector f, void* user_data)
{
  double* zdata = NV_DATA_S(z);
  double* fdata = NV_DATA_S(f);

  KINSOL_DATA* kdata = (KINSOL_DATA*) user_data;
  DATA* data = kdata->data;
  INIT_DATA* initData = kdata->initData;

  double* lb = initData->min;
  double* ub = initData->max;
  long i;

  leastSquareWithLambda(data, initData->nz, zdata, NULL, NULL, 1.0, globalInitialResiduals);

  for(i=0; i<initData->nz; ++i)
  {
    fdata[i] = globalInitialResiduals[i];
    fdata[initData->nz+2*i+0] = zdata[initData->nz+2*i+0] - zdata[i] + lb[i];
    fdata[initData->nz+2*i+1] = zdata[initData->nz+2*i+1] - zdata[i] + ub[i];
  }

  return 0;
}

/*! \fn computeInitialResidualScalingCoefficients
 *
 *  This function calculates coefficients for every initial_residual.
 *  They describe the order of magnitude.
 *
 *  \param [ref] [data]
 *  \param [in]  [nz] number of unfixed states and unfixed parameters
 *  \param [in]  [z] vector of unfixed states and unfixed parameters
 *  \param [in]  [zNominal] vector of nominal-values for z or NULL
 *  \param [out] [initialResidualScalingCoefficients] vector of scaling-coefficients for initial_residuals
 *
 *  \author lochel
 */
static void computeInitialResidualScalingCoefficients(DATA *data, double nz, double *z, double *zNominal, double *initialResidualScalingCoefficients)
{
  int i, j;

  double *tmpInitialResidual1 = (double*)malloc(data->modelData.nResiduals * sizeof(double));
  double *tmpInitialResidual2 = (double*)malloc(data->modelData.nResiduals * sizeof(double));
  double *states = (double*)malloc(nz * sizeof(double));

  const double h = 1e-6;

  for(j=0; j<data->modelData.nResiduals; ++j)
    initialResidualScalingCoefficients[j] = 0.0;

  if (zNominal)
  {
    for(i=0; i<nz; ++i)
      states[i] = z[i] * zNominal[i];
  }
  else
  {
    for(i=0; i<nz; ++i)
      states[i] = z[i];
  }

  /* lambda = 1.0 */
  leastSquareWithLambda(data, nz, states, NULL, NULL, 1.0, tmpInitialResidual1);

  for(i=0; i<nz; ++i)
  {
    states[i] += h;
    leastSquareWithLambda(data, nz, states, NULL, NULL, 1.0, tmpInitialResidual2);

    for(j=0; j<data->modelData.nResiduals; ++j)
    {
      double f = fabs(zNominal[i] * (tmpInitialResidual2[j] - tmpInitialResidual1[j]) / h /* / tmpInitialResidual2[j] */ );
      if(f > initialResidualScalingCoefficients[j])
        initialResidualScalingCoefficients[j] = f;
    }
    states[i] -= h;
  }

  /* lambda = 0.0 */
  leastSquareWithLambda(data, nz, states, NULL, NULL, 0.0, tmpInitialResidual1);

  for(i=0; i<nz; ++i)
  {
    states[i] += h;
    leastSquareWithLambda(data, nz, states, NULL, NULL, 0.0, tmpInitialResidual2);

    for(j=0; j<data->modelData.nResiduals; ++j)
    {
      double f = fabs(zNominal[i] * (tmpInitialResidual2[j] - tmpInitialResidual1[j]) / h /* / tmpInitialResidual2[j] */ );
      if(f > initialResidualScalingCoefficients[j])
        initialResidualScalingCoefficients[j] = f;
    }
    states[i] -= h;
  }

  DEBUG_INFO(LOG_INIT, "initial residuals scaling coefficients");
  for(j=0; j<data->modelData.nResiduals; ++j)
  {
    if(initialResidualScalingCoefficients[j] < 1e-42)
    {
      initialResidualScalingCoefficients[j] = 0.0;
      DEBUG_INFO_AL2(LOG_INIT, "   initial residual no. %d: %g [ineffective]", j, initialResidualScalingCoefficients[j]);
    }
    else
    {
      DEBUG_INFO_AL2(LOG_INIT, "   initial residual no. %d: %g", j, initialResidualScalingCoefficients[j]);
    }
  }

  free(tmpInitialResidual1);
  free(tmpInitialResidual2);
  free(states);
}

/*! \fn NelderMeadOptimization
 *
 *  This function performs a Nelder-Mead-Optimization with some
 *  special changes for initialization.
 *
 *  \param [in]  [N] number of unfixed states and unfixed parameters
 *  \param [in]  [var] vector of unfixed states and unfixed parameters
 *  \param [in]  [scale] vector of nominal-values for var or NULL
 *  \param [in]  [initialResidualScalingCoefficients] vector of scaling-coefficients for initial_residuals or NULL
 *  \param [in]  [lambda_step]
 *  \param [in]  [acc]
 *  \param [in]  [maxIt]
 *  \param [in]  [dump]
 *  \param [in]  [pLambda]
 *  \param [in]  [pIteration]
 *  \param [in]  [leastSquare]
 *  \param [ref] [data]
 *  \param [in]  [initialResiduals]
 *
 *  \author lochel
 */
static void NelderMeadOptimization(long N,
  double* var,
  double* scale,
  double* initialResidualScalingCoefficients,
  double lambda_step,
  double acc,
  long maxIt,
  long dump,
  double* pLambda,
  long* pIteration,
  double (*leastSquare)(DATA*, long, double*, double*, double*, double, double*),
  DATA* data,
  double* initialResiduals)
{
  const double alpha    = 1.0;        /* 0 < alpha */
  const double beta     = 2;          /* 1 < beta */
  const double gamma    = 0.5;        /* 0 < gamma < 1 */

  double* simplex = (double*)malloc((N+1)*N*sizeof(double));
  double* fvalues = (double*)malloc((N+1)*sizeof(double));

  double* xr = (double*)malloc(N * sizeof(double));
  double* xe = (double*)malloc(N * sizeof(double));
  double* xk = (double*)malloc(N * sizeof(double));
  double* xbar = (double*)malloc(N * sizeof(double));

  double fxr;
  double fxe;
  double fxk;

  long xb = 0;        /* best vertex */
  long xs = 0;        /* worst vertex */
  long xz = 0;        /* second-worst vertex */

  long x = 0;
  long i = 0;

  double lambda = *pLambda;
  long iteration = 0;

  /* check Memory */
  ASSERT(simplex, "out of memory");
  ASSERT(fvalues, "out of memory");
  ASSERT(xr, "out of memory");
  ASSERT(xe, "out of memory");
  ASSERT(xk, "out of memory");
  ASSERT(xbar, "out of memory");

  /* initialize simplex */
  for(x=0; x<N+1; x++)
  {
    for(i=0; i<N; i++)
    {
      /* vertex x / var i */
      simplex[x*N + i] = var[i];
    }
  }
  for(i=0; i<N; i++)
  {
    simplex[i*N + i] += 1.0;    /* canonical simplex */
  }

  computeInitialResidualScalingCoefficients(data, N, simplex, scale, initialResidualScalingCoefficients);

  do
  {
    /* lambda-control */
    double sigma = 0.0;
    double average = 0.0;
    double g = 1e-8;

    iteration++;

    /* func-values for the simplex */
    for(x=0; x<N+1; x++)
      fvalues[x] = leastSquare(data, N, &simplex[x*N], scale, initialResidualScalingCoefficients, lambda, initialResiduals);

    /* calculate xb, xs, xz */
    xb = 0;
    for(x=1; x<N+1; x++)
    {
      if(fvalues[x] < fvalues[xb])
        xb = x;
    }

    if(lambda >= 1.0 && fvalues[xb] < acc)
      break;

    if(maxIt < iteration)
      break;

    xs = xb;
    xz = xb;
    for(x=0; x<N+1; x++)
    {
      if(fvalues[x] > fvalues[xs])
      {
        xz = xs;
        xs = x;
      }

      if(fvalues[x] > fvalues[xz] && (x != xs))
        xz = x;
    }

    for(x=0; x<N+1; x++)
      average += fvalues[x];
    average /= (N+1);

    for(x=0; x<N+1; x++)
      sigma += (fvalues[x] - average) * (fvalues[x] - average);
    sigma /= N;

    /* dump every dump-th step */
    if(dump && !(iteration % dump))
      INFO4("NelderMeadOptimization | lambda=%g / step=%d / f=%g [%g]", lambda, (int)iteration, fvalues[xb], fvalues[xs]);

    if(sigma < g)
    {
      if(lambda < 1.0)
      {
        lambda += lambda_step;
        if(lambda >= 1.0)
          break;

        DEBUG_INFO3(LOG_INIT, "NelderMeadOptimization | increasing lambda to %g in step %d at f=%g", lambda, (int)iteration, fvalues[xb]);
        continue;
      }
    }

    /* calculate central point for the n best vertices */
    for(i=0; i<N; i++)
      xbar[i] = 0;

    for(x=0; x<N+1; x++)
    {
      if(x != xs)            /* leaving worst vertex */
      {
        for(i=0; i<N; i++)
          xbar[i] += simplex[x*N+i];
      }
    }

    for(i=0; i<N; i++)
      xbar[i] /= N;

    /* reflect worst vertex at xbar */
    for(i=0; i<N; i++)
      xr[i] = xbar[i] + alpha*(xbar[i] - simplex[xs*N + i]);
    fxr = leastSquare(data, N, xr, scale, initialResidualScalingCoefficients, lambda, initialResiduals);

    if(fvalues[xb] <= fxr && fxr <= fvalues[xz])
    {
      /* replace xs by xr */
      for(i=0; i<N; i++)
        simplex[xs*N+i] = xr[i];
    }
    else if(fxr <= fvalues[xb])
    {
      for(i=0; i<N; i++)
        xe[i] = xbar[i] + beta*(xr[i] - xbar[i]);
      fxe = leastSquare(data, N, xe, scale, initialResidualScalingCoefficients, lambda, initialResiduals);

      if(fxe < fxr)    /* if(fxe < fvalues[xb]) */
      {
        /* replace xs by xe */
        for(i=0; i<N; i++)
          simplex[xs*N+i] = xe[i];
      }
      else
      {
        /* replace xs by xr */
        for(i=0; i<N; i++)
          simplex[xs*N+i] = xr[i];
      }
    }
    else if(fvalues[xz] <= fxr)
    {
      if(fxr >= fvalues[xs])
      {
        for(i=0; i<N; i++)
          xk[i] = xbar[i] + gamma*(simplex[xs*N+i] - xbar[i]);
        fxk = leastSquare(data, N, xk, scale, initialResidualScalingCoefficients, lambda, initialResiduals);
      }
      else
      {
        for(i=0; i<N; i++)
          xk[i] = xbar[i] + gamma*(xr[i] - xbar[i]);
        fxk = leastSquare(data, N, xk, scale, initialResidualScalingCoefficients, lambda, initialResiduals);
      }

      if(fxk < fvalues[xs])
      {
        /* replace xs by xk */
        for(i=0; i<N; i++)
          simplex[xs*N+i] = xk[i];
      }
      else
      {
        /* constrict simplex around xb */
        for(x=0; x<N+1; x++)
        {
          for(i=0; i<N; i++)
          {
            simplex[x*N+i] = (simplex[x*N+i] + simplex[xb*N+i]) / 2.0;
          }
        }
      }
    }
    else
    {
      /* not possible to be here */
      WARNING1("fxr = %g", fxr);
      WARNING1("fxk = %g", fxk);

      THROW("undefined error in NelderMeadOptimization");
    }
  }while(1.0);

  /* copying solution */
  for(i=0; i<N; i++)
    var[i] = simplex[xb*N+i];

  if(pLambda)
    *pLambda = lambda;

  if(pIteration)
    *pIteration = iteration;

  free(xe);
  free(xr);
  free(xk);
  free(xbar);
  free(fvalues);
  free(simplex);
}

/*! \fn reportResidualValue
 *
 *  Returns 1 if residual is non-zero and prints appropriate error message.
 *
 *  \param [ref] [data]
 *  \param [in]  [funcValue] leastSquare-Value
 *  \param [in]  [initialResiduals]
 */
static int reportResidualValue(DATA* data, double funcValue, double* initialResiduals)
{
  long i = 0;
  if(funcValue > 1e-5)
  {
    WARNING("reportResidualValue | error in initialization. System of initial equations are not consistent");
    WARNING1("reportResidualValue | (Least Square function value is %g)", funcValue);

    for(i=0; i<data->modelData.nResiduals; i++)
    {
      if(fabs(initialResiduals[i]) > 1e-6)
      {
        INFO2("reportResidualValue | residual[%d] = %g", (int) i, initialResiduals[i]);
      }
    }
    return 1;
  }
  return 0;
}

void kinsol_errorHandler(int error_code, const char* module, const char* function, char* msg, void* user_data)
{
  WARNING3("[module] %s | [function] %s | [error_code] %d", module, function, error_code);
  WARNING_AL1("%s", msg);
  THROW("see last warning");
}

/*! \fn kinsol_initialization
 *
 *  \param [ref] [data]
 *  \param [in]  [initData]
 *  \param [ref] [initialResiduals]
 *
 *  \author lochel
 */
int kinsol_initialization(DATA* data, INIT_DATA* initData, double* initialResiduals)
{
  long i, indz;
  KINSOL_DATA* kdata = NULL;
  double fnormtol  = 1.e-9;     /* function tolerance */
  double scsteptol = 1.e-9;     /* step tolerance */

  long int nni, nfe, nje, nfeD;

  N_Vector z = NULL;
  N_Vector s = NULL;
  N_Vector c = NULL;

  int glstr = KIN_LINESEARCH;   /* KIN_LINESEARCH */
  int mset = 1;                 /* 0 */
  void *kmem = NULL;

  ASSERT(data->modelData.nInitEquations == initData->nz, "The number of initial equations are not consistent with the number of unfixed variables. Select a different initialization.");

  kdata = (KINSOL_DATA*)malloc(sizeof(KINSOL_DATA));
  ASSERT(kdata, "out of memory");

  kdata->initData = initData;
  kdata->data = data;

  z = N_VNew_Serial(3*initData->nz);
  ASSERT(z, "out of memory");

  s = N_VNew_Serial(3*initData->nz);
  ASSERT(s, "out of memory");

  c = N_VNew_Serial(3*initData->nz);
  ASSERT(c, "out of memory");

  /* initial guess */
  for(i=0; i<initData->nz; ++i)
  {
    NV_Ith_S(z, i) = initData->start[i];
    NV_Ith_S(z, data->modelData.nInitEquations+2*i+0) = NV_Ith_S(z, i) - initData->min[i];
    NV_Ith_S(z, data->modelData.nInitEquations+2*i+1) = NV_Ith_S(z, i) - initData->max[i];
  }

  N_VConst_Serial(1.0, s);        /* no scaling */

  for(i=0; i<initData->nz; ++i)
  {
    NV_Ith_S(c, i) =  0.0;        /* no constraint on z[i] */
    NV_Ith_S(c, data->modelData.nInitEquations+2*i+0) = 1.0;
    NV_Ith_S(c, data->modelData.nInitEquations+2*i+1) = -1.0;
  }

  kmem = KINCreate();
  ASSERT(kmem, "out of memory");

  KINSetErrHandlerFn(kmem, kinsol_errorHandler, NULL);
  KINSetUserData(kmem, kdata);
  KINSetConstraints(kmem, c);
  KINSetFuncNormTol(kmem, fnormtol);
  KINSetScaledStepTol(kmem, scsteptol);
  KINInit(kmem, kinsol_residuals, z);

  /* Call KINDense to specify the linear solver */
  KINDense(kmem, 3*initData->nz);

  if(mset == 1 && glstr == KIN_NONE)
    DEBUG_INFO(LOG_INIT, "using exact Newton");
  else if(mset == 1)
    DEBUG_INFO(LOG_INIT, "using exact Newton with line search");
  else if(mset == 0 && glstr == KIN_NONE)
    DEBUG_INFO(LOG_INIT, "using modified Newton");
  else if(mset == 0)
    DEBUG_INFO(LOG_INIT, "using modified Newton with line search");

  DEBUG_INFO(LOG_INIT, "tolerance parameters:");
  DEBUG_INFO_AL1(LOG_INIT, "function tolerance = %10.6g", fnormtol);
  DEBUG_INFO_AL1(LOG_INIT, "step tolerance     = %10.6g", scsteptol);

  KINSetMaxSetupCalls(kmem, mset);

  globalInitialResiduals = initialResiduals;

  KINSol(kmem,           /* KINSol memory block */
         z,              /* initial guess on input; solution vector */
         glstr,          /* global stragegy choice */
         s,              /* scaling vector, for the variable cc */
         s);             /* scaling vector for function values fval */

  globalInitialResiduals = NULL;

  KINGetNumNonlinSolvIters(kmem, &nni);
  KINGetNumFuncEvals(kmem, &nfe);
  KINDlsGetNumJacEvals(kmem, &nje);
  KINDlsGetNumFuncEvals(kmem, &nfeD);

  DEBUG_INFO(LOG_INIT, "final kinsol Statistics:");
  DEBUG_INFO_AL1(LOG_INIT, "  KINGetNumNonlinSolvIters = %5ld", nni);
  DEBUG_INFO_AL1(LOG_INIT, "  KINGetNumFuncEvals       = %5ld", nfe);
  DEBUG_INFO_AL1(LOG_INIT, "  KINDlsGetNumJacEvals     = %5ld", nje);
  DEBUG_INFO_AL1(LOG_INIT, "  KINDlsGetNumFuncEvals    = %5ld", nfeD);

  /* Free memory */
  N_VDestroy_Serial(z);
  N_VDestroy_Serial(s);
  N_VDestroy_Serial(c);
  KINFree(&kmem);
  free(kdata);

  /* debug output */
  indz = 0;
  for(i=0; i<data->modelData.nStates; ++i)
    if(data->modelData.realVarsData[i].attribute.fixed==0)
      DEBUG_INFO_AL2(LOG_INIT, "   %s = %g", initData->name[indz++], data->localData[0]->realVars[i]);

  for(i=0; i<data->modelData.nParametersReal; ++i)
    if(data->modelData.realParameterData[i].attribute.fixed == 0)
      DEBUG_INFO_AL2(LOG_INIT, "   %s = %g", initData->name[indz++], data->simulationInfo.realParameter[i]);

  return 0;
}

/*! \fn int newuoa_initialization(long nz, double *z)
 *
 *  This function performs initialization using the newuoa function, which is
 *  a trust region method that forms quadratic models by interpolation.
 */
int newuoa_initialization(DATA* data, long nz, double *z, char** zName, double *zNominal, double* initialResiduals)
{
  long IPRINT = DEBUG_FLAG(LOG_INIT) ? 1000 : 0;
  long MAXFUN = 50000;
  double RHOEND = 1.0e-6;
  double RHOBEG = 10;     /* This should be about one tenth of the greatest
                             expected value of a variable. Perhaps the nominal
                             value can be used for this. */
  long NPT = 2*nz+1;
  double funcValue = 0;

  double *W = (double*)calloc((NPT+13)*(NPT+nz)+3*nz*(nz+3)/2, sizeof(double));

  globalData = data;
  globalInitialResiduals = initialResiduals;


  ASSERT(W, "out of memory");
  NEWUOA(&nz, &NPT, z, &RHOBEG, &RHOEND, &IPRINT, &MAXFUN, W, leastSquare);
  free(W);

  globalData = NULL;
  globalInitialResiduals = NULL;

  /* Calculate the residual to verify that equations are consistent. */
  funcValue = leastSquareWithLambda(data, nz, z, NULL, NULL, 1.0, initialResiduals);
  return reportResidualValue(data, funcValue, initialResiduals);
}

/*! \fn int simplex_initialization(long nz,double *z)
 *
 *  This function performs initialization by using the simplex algorithm.
 *  This does not require a jacobian for the residuals.
 */
int simplex_initialization(DATA* data, long nz, double *z, char** zName, double *zNominal, double* initialResiduals)
{
  int ind = 0;
  double funcValue = 0;
  double *STEP = (double*)malloc(nz * sizeof(double));
  double *VAR = (double*)malloc(nz * sizeof(double));
  double STOPCR = 0, SIMP = 0;
  long IPRINT = 0, NLOOP = 0, IQUAD = 0, IFAULT = 0, MAXF = 0;
  ASSERT(STEP, "out of memory");
  ASSERT(VAR, "out of memory");

  /* Start with stepping .5 in each direction. */
  for(ind = 0; ind < nz; ind++)
  {
    /* some kind of scaling */
    STEP[ind] = (z[ind]!=0.0 ? fabs(z[ind])/1000.0 : 1);    /* 1.0 */
    VAR[ind]  = 0.0;
  }

  /* Set max. no. of function evaluations = 5000, print every 100. */

  MAXF = 5000 * nz;
  IPRINT = DEBUG_FLAG(LOG_INIT) ? 1000 : -1;

  /* Set value for stopping criterion.   Stopping occurs when the
  * standard deviation of the values of the objective function at
  * the points of the current simplex < stopcr. */

  STOPCR = 1.e-12;
  NLOOP = nz;

  /* Fit a quadratic surface to be sure a minimum has been found. */

  IQUAD = 0;

  /* As function value is being evaluated in DOUBLE PRECISION, it
  * should be accurate to about 15 decimals.   If we set simp = 1.d-6,
  * we should get about 9 dec. digits accuracy in fitting the surface. */

  SIMP = 1.e-12;

  /* Now call NELMEAD to do the work. */

  funcValue = leastSquareWithLambda(data, nz, z, NULL, NULL, 1.0, initialResiduals);

  if(fabs(funcValue) != 0)
  {
    globalData = data;
    globalInitialResiduals = initialResiduals;
    NELMEAD(z,STEP, &nz, &funcValue, &MAXF, &IPRINT, &STOPCR, &NLOOP,&IQUAD,&SIMP,VAR,leastSquare,&IFAULT);
    globalData = NULL;
    globalInitialResiduals = NULL;
  }
  else
  {
    DEBUG_INFO1(LOG_INIT, "simplex_initialization | Result of leastSquare method = %g. The initial guess fits to the system", funcValue);
  }

  funcValue = leastSquareWithLambda(data, nz, z, NULL, NULL, 1.0, initialResiduals);

  DEBUG_INFO1(LOG_INIT, "leastSquare=%g", funcValue);

  if(IFAULT == 1)
  {
    if(funcValue > SIMP) {
      WARNING1("Error in initialization. Solver iterated %d times without finding a solution", (int)MAXF);
      return -1;
    }
  }else if(IFAULT == 2 ) {
    WARNING("Error in initialization. Inconsistent initial conditions.");
    return -1;
  }else if(IFAULT == 3) {
    WARNING("Error in initialization. Number of initial values to calculate < 1");
    return -1;
  }else if(IFAULT == 4) {
    WARNING("Error in initialization. Internal error, NLOOP < 1.");
    return -1;
  }
  return reportResidualValue(data, funcValue, initialResiduals);
}

/*! \fn nelderMeadEx_initialization
 *
 *  This function performs initialization by using an extend version of the
 *  nelderMead algorithm.
 *  This does not require a jacobian for the residuals.
 *
 *  \param [ref] [data]
 *  \param [in]  [nz] number of unfixed states and unfixed parameters
 *  \param [in]  [z] vector of unfixed states and unfixed parameters
 *  \param [in]  [zName] variable names
 *  \param [in]  [zNominal] vector of nominal-values for z
 *  \param [in]  [initialResiduals]
 *
 *  \author lochel
 */
static int nelderMeadEx_initialization(DATA *data, long nz, double *z, char** zName, double *zNominal, double* initialResiduals, double lambdaStart)
{
  double STOPCR = 1.e-12;
  double lambda_step = 0.2;
  long NLOOP = 1000 * nz;

  double funcValue;

  double lambda = lambdaStart;
  long iteration = 0;

  long l=0, i=0;

  double* initialResidualScalingCoefficients = (double*)malloc(data->modelData.nResiduals * sizeof(double));
  double* bestZ = (double*)malloc(nz * sizeof(double));
  double bestFuncValue;

  /* down-scale */
  for(i=0; i<nz; i++)
    z[i] /= zNominal[i];

  funcValue = leastSquareWithLambda(data, nz, z, zNominal, NULL, 1.0, initialResiduals);

  bestFuncValue = funcValue;
  for(i=0; i<nz; i++)
    bestZ[i] = z[i];

  DEBUG_INFO(LOG_INIT, "starting with...");
  for(i=0; i<nz; i++)
    DEBUG_INFO_AL4(LOG_INIT, "   z[%ld]: %s(nominal=%g) = %g", i, zName[i], zNominal[i], z[i]);
  DEBUG_INFO_AL1(LOG_INIT, "   funcValue = %g", funcValue);

  for(l=0; l<100 && funcValue > STOPCR; l++)
  {
    DEBUG_INFO1(LOG_INIT, "initialization-nr. %ld", l);

    NelderMeadOptimization(nz, z, zNominal, initialResidualScalingCoefficients, lambda_step, STOPCR, NLOOP, DEBUG_FLAG(LOG_INIT) ? 10000 : 0, &lambda, &iteration, leastSquareWithLambda, data, initialResiduals);

    storePreValues(data);                       /* save pre-values */
    overwriteOldSimulationData(data);           /* if there are non-linear equations */
    update_DAEsystem(data);                     /* evaluate discrete variables */

    /* valid system for the first time! */
    SaveZeroCrossings(data);
    storePreValues(data);
    overwriteOldSimulationData(data);

    funcValue = leastSquareWithLambda(data, nz, z, zNominal, initialResidualScalingCoefficients, 1.0, initialResiduals);

    DEBUG_INFO1(LOG_INIT, "ending with funcValue = %g", funcValue);
    DEBUG_INFO_AL1(LOG_INIT, "   iterations: %ld", iteration);
    DEBUG_INFO_AL1(LOG_INIT, "   lambda: %g", lambda);
    for(i=0; i<nz; i++)
      DEBUG_INFO_AL3(LOG_INIT, "   z[%ld]: %s = %g", i, zName[i], z[i] * (zNominal ? zNominal[i] : 1.0));
    for(i=0; i<data->modelData.nResiduals; i++)
      if(fabs(initialResiduals[i]) > 1e-3)
        DEBUG_INFO_AL2(LOG_INIT, "   residual[%ld] = %g", i, initialResiduals[i]);

    if(funcValue < bestFuncValue)
    {
      bestFuncValue = funcValue;
      for(i=0; i<nz; i++)
        bestZ[i] = z[i];
    }
    else if(funcValue == bestFuncValue)
    {
      WARNING("local minimum");
      break;
    }
  }
  free(initialResidualScalingCoefficients);
  free(bestZ);

  DEBUG_INFO_AL1(LOG_INIT, "   optimization-calls: %ld", l);

  /* up-scale */
  for(i=0; i<nz; i++)
    z[i] *= zNominal[i];

  if(lambda < 1.0 && funcValue > STOPCR)
    return -1;

  return reportResidualValue(data, funcValue, initialResiduals);
}

/*! \fn freeInitData
 *
 *  \param [ref] [initData]
 *
 *  \author lochel
 */
static void freeInitData(INIT_DATA *initData)
{
  free(initData->z);
  free(initData->start);
  free(initData->min);
  free(initData->max);
  free(initData->nominal);
  free(initData->name);
  free(initData);
}

/*! \fn initializeInitData
 *
 *  \param [in]  [data]
 *
 *  \author lochel
 */
static INIT_DATA *initializeInitData(DATA *data)
{
  long i;
  long iz;
  INIT_DATA* initData = NULL;

  initData = (INIT_DATA*)malloc(sizeof(INIT_DATA));
  ASSERT(initData, "out of memory");

  initData->nz = 0;
  initData->nStates = 0;
  initData->nParameters = 0;
  initData->z = NULL;
  initData->start = NULL;
  initData->min = NULL;
  initData->max = NULL;
  initData->nominal = NULL;
  initData->name = NULL;

  /* count unfixed states */
  for(i=0; i<data->modelData.nStates; ++i)
    if(data->modelData.realVarsData[i].attribute.fixed == 0)
      ++initData->nz;
  initData->nStates = initData->nz;

  /* plus unfixed real-parameters */
  for(i=0; i<data->modelData.nParametersReal; ++i)
    if(data->modelData.realParameterData[i].attribute.fixed == 0)
      ++initData->nz;
  initData->nParameters = initData->nz - initData->nStates;

  if(initData->nz == 0)
  {
    freeInitData(initData);
    return NULL;
  }

  initData->z = (double*)calloc(initData->nz, sizeof(double));
  ASSERT(initData->z, "out of memory");

  initData->start = (double*)calloc(initData->nz, sizeof(double));
  ASSERT(initData->start, "out of memory");

  initData->min = (double*)calloc(initData->nz, sizeof(double));
  ASSERT(initData->min, "out of memory");

  initData->max = (double*)calloc(initData->nz, sizeof(double));
  ASSERT(initData->max, "out of memory");

  initData->nominal = (double*)calloc(initData->nz, sizeof(double));
  ASSERT(initData->nominal, "out of memory");

  initData->name = (char**)calloc(initData->nz, sizeof(char*));
  ASSERT(initData->name, "out of memory");

  /* setup initData */
  for(i=0, iz=0; i<data->modelData.nStates; ++i)
  {
    if(data->modelData.realVarsData[i].attribute.fixed == 0)
    {
      initData->name[iz] = (char*)data->modelData.realVarsData[i].info.name;
      initData->nominal[iz] = fabs(data->modelData.realVarsData[i].attribute.nominal);
      if(initData->nominal[iz] == 0.0)
      {
        /* adrpo 2012-05-08 disable the warning for now until the whole infrastructure is in place
         *                  because this breaks the FMI tests with these kind of messages:
         *                  warning | (null)(nominal=0)
         *                          | nominal value is set to 1.0
         * put it back when everything works fine.
        WARNING2("%s(nominal=%g)", initData->name[iz], initData->nominal[iz]);
        WARNING_AL("nominal value is set to 1.0");
        */
        initData->nominal[iz] = 1.0;
      }

      initData->z[iz] = data->modelData.realVarsData[i].attribute.start;
      initData->start[iz] = data->modelData.realVarsData[i].attribute.start;
      initData->min[iz] = data->modelData.realVarsData[i].attribute.min;
      initData->max[iz] = data->modelData.realVarsData[i].attribute.max;

      iz++;
    }
  }

  for(i=0; i<data->modelData.nParametersReal; ++i)
  {
    if(data->modelData.realParameterData[i].attribute.fixed == 0)
    {
      initData->name[iz] = (char*)data->modelData.realParameterData[i].info.name;
      initData->nominal[iz] = fabs(data->modelData.realParameterData[i].attribute.nominal);
      if(initData->nominal[iz] == 0.0)
      {
        /* adrpo 2012-05-08 disable the warning for now until the whole infrastructure is in place
         *                  because this breaks the FMI tests with these kind of messages:
         *                  warning | (null)(nominal=0)
         *                          | nominal value is set to 1.0
         * put it back when everything works fine.
        WARNING2("%s(nominal=%g)", initData->name[iz], initData->nominal[iz]);
        WARNING_AL("nominal value is set to 1.0");
        */
        initData->nominal[iz] = 1.0;
      }

      initData->z[iz] = data->modelData.realParameterData[i].attribute.start;
      initData->start[iz] = data->modelData.realParameterData[i].attribute.start;
      initData->min[iz] = data->modelData.realParameterData[i].attribute.min;
      initData->max[iz] = data->modelData.realParameterData[i].attribute.max;

      iz++;
    }
  }

  return initData;
}

/*! \fn initialize
 *
 *  \param [ref] [data]
 *  \param [in]  [optiMethod] specified optimization method
 *
 *  \author lochel
 */
static int initialize(DATA *data, int optiMethod)
{
  int retVal = -1;
  const double h = 1e-6;
  double *initialResiduals = NULL;
  long i, j, k;
  double f;
  double *z_f = NULL;

  INIT_DATA *initData = initializeInitData(data);

  /* no initial values to calculate. */
  if(initData == NULL)
  {
    DEBUG_INFO(LOG_INIT, "no variables to initialize");
    return 0;
  }

  if(data->modelData.nInitEquations == 0)
  {
    DEBUG_INFO(LOG_INIT, "no initial equations");
    return 0;
  }

  initialResiduals = (double*) calloc(data->modelData.nResiduals, sizeof(double));
  ASSERT(initialResiduals, "out of memory");

  DEBUG_INFO(LOG_INIT, "initial problem:");
  DEBUG_INFO_AL1(LOG_INIT, "   number of unfixed variables: %ld", initData->nz);
  DEBUG_INFO_AL1(LOG_INIT, "   number of initial equations: %ld", data->modelData.nInitEquations);

  if(data->modelData.nInitEquations < initData->nz)
  {
    DEBUG_INFO_AL(LOG_INIT, "   [under-determined]");

    z_f = (double*)malloc(initData->nz * sizeof(double));
    f = leastSquareWithLambda(data, initData->nz, initData->z, NULL, NULL, 1.0, initialResiduals);
    for(i=0; i<initData->nz; ++i)
    {
      initData->z[i] += h;
      z_f[i] = fabs(leastSquareWithLambda(data, initData->nz, initData->z, NULL, NULL, 1.0, initialResiduals) - f) / h;
      initData->z[i] -= h;
    }

    for(j=0; j < data->modelData.nInitEquations; ++j)
    {
      k = 0;
      for(i=1; i<initData->nz; ++i)
        if(z_f[i] > z_f[k])
          k = i;
      z_f[k] = -1.0;
    }

    k = 0;
    DEBUG_INFO(LOG_INIT, "setting fixed=true for:");
    for(i=0; i<data->modelData.nStates; ++i)
    {
      if(data->modelData.realVarsData[i].attribute.fixed == 0)
      {
        if(z_f[k] >= 0.0)
        {
          data->modelData.realVarsData[i].attribute.fixed = 1;
          DEBUG_INFO2(LOG_INIT, "   %s(fixed=true) = %g", initData->name[k], initData->z[k]);
        }
        k++;
      }
    }
    for(i=0; i<data->modelData.nParametersReal; ++i)
    {
      if(data->modelData.realParameterData[i].attribute.fixed == 0)
      {
        if(z_f[k] >= 0.0)
        {
          data->modelData.realParameterData[i].attribute.fixed = 1;
          DEBUG_INFO2(LOG_INIT, "   %s(fixed=true) = %g", initData->name[k], initData->z[k]);
        }
        k++;
      }
    }

    free(z_f);

    freeInitData(initData);
    initData = initializeInitData(data);
    /* no initial values to calculate. (not possible to be here)*/
    if(initData == NULL)
    {
      DEBUG_INFO(LOG_INIT, "no initial values to calculate");
      return 0;
    }
  }
  else if(data->modelData.nInitEquations > initData->nz)
    DEBUG_INFO_AL(LOG_INIT, "   [over-determined]");

  DEBUG_INFO1(LOG_INIT, "%ld unfixed states:", initData->nStates);
  for(i=0; i<initData->nStates; ++i)
    DEBUG_INFO_AL2(LOG_INIT, "   [%ld] %s", i, initData->name[i]);
  DEBUG_INFO1(LOG_INIT, "%ld unfixed parameters:", initData->nParameters);
  for(; i<initData->nz; ++i)
    DEBUG_INFO_AL2(LOG_INIT, "   [%ld] %s", i, initData->name[i]);

  if(optiMethod == IOM_NELDER_MEAD_EX)
    retVal = nelderMeadEx_initialization(data, initData->nz, initData->z, initData->name, initData->nominal, initialResiduals, 0.0);
  else if(optiMethod == IOM_NELDER_MEAD_EX2)
    retVal = nelderMeadEx_initialization(data, initData->nz, initData->z, initData->name, initData->nominal, initialResiduals, 1.0);
  else if(optiMethod == IOM_SIMPLEX)
    retVal = simplex_initialization(data, initData->nz, initData->z, initData->name, initData->nominal, initialResiduals);
  else if(optiMethod == IOM_NEWUOA)
      retVal = newuoa_initialization(data, initData->nz, initData->z, initData->name, initData->nominal, initialResiduals);
  else if(optiMethod == IOM_KINSOL)
      retVal = kinsol_initialization(data, initData, initialResiduals);
  else
    THROW("unsupported option -iom");

  free(initialResiduals);
  freeInitData(initData);
  return retVal;
}

/*! \fn none_initialization
 *
 *  \param [ref] [data]
 *
 *  \author lochel
 */
static int none_initialization(DATA *data, int updateStartValues)
{
  long i;
  INIT_DATA *initData = initializeInitData(data);

  /* set up all variables and parameters with their start-values */
  setAllVarsToStart(data);
  setAllParamsToStart(data);
  if(updateStartValues)
  {
    updateBoundParameters(data);
    updateBoundStartValues(data);
  }

  /* initial sample and delay before initial the system */
  initSample(data, data->simulationInfo.startTime, data->simulationInfo.stopTime);
  initDelay(data, data->simulationInfo.startTime);

  /* initialize all relations that are ZeroCrossings */
  storePreValues(data);
  overwriteOldSimulationData(data);
  update_DAEsystem(data);

  /* and restore start values and helpvars */
  restoreExtrapolationDataOld(data);
  resetAllHelpVars(data);
  storePreValues(data);

  DEBUG_INFO1(LOG_INIT, "%ld unfixed states:", initData->nStates);
  for(i=0; i<initData->nStates; ++i)
    DEBUG_INFO2(LOG_INIT, "   %s = %g", initData->name[i], initData->z[i]);

  DEBUG_INFO1(LOG_INIT, "%ld unfixed parameters:", initData->nParameters);
  for(; i<initData->nStates+initData->nParameters; ++i)
    DEBUG_INFO2(LOG_INIT, "   %s = %g", initData->name[i], initData->z[i]);

  storeInitialValues(data);
  storeInitialValuesParam(data);
  storePreValues(data);             /* save pre-values */
  overwriteOldSimulationData(data); /* if there are non-linear equations */
  update_DAEsystem(data);           /* evaluate discrete variables */

  /* valid system for the first time! */
  SaveZeroCrossings(data);
  storeInitialValues(data);
  storeInitialValuesParam(data);
  storePreValues(data);             /* save pre-values */
  overwriteOldSimulationData(data); /* if there are non-linear equations */

  return 0;
}

/*! \fn state_initialization
 *
 *  \param [ref] [data]
 *  \param [in]  [optiMethod] specified optimization method
 *  \param [in]  [updateStartValues]
 *
 *  \author lochel
 */
static int state_initialization(DATA *data, int optiMethod, int updateStartValues)
{
  int retVal = 0;
  int i;

  /* set up all variables and parameters with their start-values */
  setAllVarsToStart(data);
  setAllParamsToStart(data);
  if(updateStartValues)
  {
    updateBoundParameters(data);
    updateBoundStartValues(data);
  }

  /* initial sample and delay before initial the system */
  initSample(data, data->simulationInfo.startTime, data->simulationInfo.stopTime);
  initDelay(data, data->simulationInfo.startTime);

  /* initialize all relations that are ZeroCrossings */
  storePreValues(data);
  overwriteOldSimulationData(data);
  update_DAEsystem(data);

  /* and restore start values and helpvars */
  restoreExtrapolationDataOld(data);
  resetAllHelpVars(data);
  storePreValues(data);

  /* debug print */
  if(DEBUG_FLAG(LOG_DEBUG))
    for(i=0; i<3;i++)
      printAllVars(data, i);

  retVal = initialize(data, optiMethod);

  /* debug print */
  if(DEBUG_FLAG(LOG_DEBUG))
    for(i=0; i<3;i++)
      printAllVars(data, i);

  storeInitialValues(data);
  storeInitialValuesParam(data);
  storePreValues(data);             /* save pre-values */
  overwriteOldSimulationData(data); /* if there are non-linear equations */
  update_DAEsystem(data);           /* evaluate discrete variables */

  /* valid system for the first time! */
  SaveZeroCrossings(data);
  storeInitialValues(data);
  storeInitialValuesParam(data);
  storePreValues(data);             /* save pre-values */
  overwriteOldSimulationData(data); /* if there are non-linear equations */

  return retVal;
}

/*! \fn mapToDymolaVars
 *
 *  \param [in]  [varname]
 *
 *  converts a given variable name into dymola style
 *  ** der(foo.foo2) -> foo.der(foo2)
 *  ** foo.foo2[1,2,3] -> foo.foo2[1, 2, 3]
 *
 *  \author lochel
 */
char* mapToDymolaVars(const char* varname)
{
  unsigned int varnameSize = strlen(varname);
  unsigned int level = 0;
  unsigned int i=0, j=0, pos=0;
  char* newVarname = NULL;
  unsigned int newVarnameSize = 0;

  newVarnameSize = varnameSize;
  for(i=0; i<varnameSize; i++)
  {
    if(varname[i] == '[')
      level++;
    else if(varname[i] == ']')
      level--;

    if(level > 0 && varname[i] == ',' && varname[i+1] != ' ')
      newVarnameSize++;
  }

  newVarname = (char*)malloc((newVarnameSize+1) * sizeof(char));
  for(i=0,j=0; i<newVarnameSize; i++,j++)
  {
    if(varname[j] == '[')
      level++;
    else if(varname[j] == ']')
      level--;

    newVarname[i] = varname[j];
    if(level > 0 && varname[j] == ',' && varname[j+1] != ' ')
    {
      i++;
      newVarname[i] = ' ';
    }
  }
  newVarname[newVarnameSize] = '\0';

  while(!memcmp((const void*)newVarname, (const void*)"der(", 4*sizeof(char)))
  {
    for(pos=newVarnameSize; pos>=4; pos--)
      if(newVarname[pos] == '.')
        break;

    memcpy((void*)newVarname, (const void*)(newVarname+4), (pos-3)*sizeof(char));
    memcpy((void*)(newVarname+pos-3), (const void*)"der(", 4*sizeof(char));
  }

  return newVarname;
}

/*! \fn importStartValues
 *
 *  \param [ref] [data]
 *  \param [in]  [pInitFile]
 *  \param [in]  [initTime]
 *
 *  \author lochel
 */
static int importStartValues(DATA *data, const char* pInitFile, double initTime)
{
  ModelicaMatReader reader;
  ModelicaMatVariable_t *pVar = NULL;
  double value;
  const char *pError = NULL;
  char* newVarname = NULL;

  MODEL_DATA *mData = &(data->modelData);
  long i;

  DEBUG_INFO    (LOG_INIT, "import start values");
  DEBUG_INFO_AL1(LOG_INIT, "  file: %s", pInitFile);
  DEBUG_INFO_AL1(LOG_INIT, "  time: %g", initTime);

  pError = omc_new_matlab4_reader(pInitFile, &reader);
  if(pError)
  {
    THROW2("unable to read input-file <%s> [%s]", pInitFile, pError);
    return 1;
  }
  else
  {
    DEBUG_INFO(LOG_INIT, "import real variables");
    for(i=0; i<mData->nVariablesReal; ++i)
    {
      pVar = omc_matlab4_find_var(&reader, mData->realVarsData[i].info.name);

      if(!pVar)
      {
        newVarname = mapToDymolaVars(mData->realVarsData[i].info.name);
        pVar = omc_matlab4_find_var(&reader, newVarname);
        free(newVarname);
      }

      if(pVar)
      {
        omc_matlab4_val(&(mData->realVarsData[i].attribute.start), &reader, pVar, initTime);
        DEBUG_INFO_AL2(LOG_INIT, "  %s(start=%g)", mData->realVarsData[i].info.name, mData->realVarsData[i].attribute.start);
      }
      else
        THROW1("unable to import real variable %s from given file", mData->realVarsData[i].info.name);
    }

    DEBUG_INFO(LOG_INIT, "import real parameters");
    for(i=0; i<mData->nParametersReal; ++i)
    {
      pVar = omc_matlab4_find_var(&reader, mData->realParameterData[i].info.name);

      if(!pVar)
      {
        newVarname = mapToDymolaVars(mData->realParameterData[i].info.name);
        pVar = omc_matlab4_find_var(&reader, newVarname);
        free(newVarname);
      }

      if(pVar)
      {
        omc_matlab4_val(&(mData->realParameterData[i].attribute.start), &reader, pVar, initTime);
        DEBUG_INFO_AL2(LOG_INIT, "  %s(start=%g)", mData->realParameterData[i].info.name, mData->realParameterData[i].attribute.start);
      }
      else
        THROW1("unable to import real parameter %s from given file", mData->realParameterData[i].info.name);
    }

    DEBUG_INFO(LOG_INIT, "import integer parameters");
    for(i=0; i<mData->nParametersInteger; ++i)
    {
      pVar = omc_matlab4_find_var(&reader, mData->integerParameterData[i].info.name);

      if(!pVar)
      {
        newVarname = mapToDymolaVars(mData->integerParameterData[i].info.name);
        pVar = omc_matlab4_find_var(&reader, newVarname);
        free(newVarname);
      }

      if(pVar)
      {
        omc_matlab4_val(&value, &reader, pVar, initTime);
        mData->integerParameterData[i].attribute.start = (modelica_integer)value;
        DEBUG_INFO_AL2(LOG_INIT, "  %s(start=%ld)", mData->integerParameterData[i].info.name, mData->integerParameterData[i].attribute.start);
      }
      else
        THROW1("unable to import integer parameter %s from given file", mData->integerParameterData[i].info.name);
    }

    DEBUG_INFO(LOG_INIT, "import boolean parameters");
    for(i=0; i<mData->nParametersBoolean; ++i)
    {
      pVar = omc_matlab4_find_var(&reader, mData->booleanParameterData[i].info.name);

      if(!pVar)
      {
        newVarname = mapToDymolaVars(mData->booleanParameterData[i].info.name);
        pVar = omc_matlab4_find_var(&reader, newVarname);
        free(newVarname);
      }

      if(pVar)
      {
        omc_matlab4_val(&value, &reader, pVar, initTime);
        mData->booleanParameterData[i].attribute.start = (modelica_boolean)value;
        DEBUG_INFO_AL2(LOG_INIT, "  %s(start=%s)", mData->booleanParameterData[i].info.name, mData->booleanParameterData[i].attribute.start ? "true" : "false");
      }
      else
        THROW1("unable to import boolean parameter %s from given file", mData->booleanParameterData[i].info.name);
    }
    omc_free_matlab4_reader(&reader);
  }

  return 0;
}

/*! \fn initialization
 *
 *  \param [ref] [data]
 *  \param [in]  [pInitMethod] user defined initialization method
 *  \param [in]  [pOptiMethod] user defined optimization method
 *  \param [in]  [pInitFile] extra argument for initialization-method "file"
 *  \param [in]  [initTime] extra argument for initialization-method "file"
 *
 *  \author lochel
 */
int initialization(DATA *data, const char* pInitMethod, const char* pOptiMethod,
                   const char* pInitFile, double initTime)
{
  int initMethod = IIM_STATE;               /* default method */
  int optiMethod = IOM_NELDER_MEAD_EX;      /* default method */
  int retVal = -1;
  int updateStartValues = 1;
  int i;

  DEBUG_INFO(LOG_INIT, "### START INITIALIZATION ###");

  /* import start values from extern mat-file */
  if(pInitFile && strcmp(pInitFile, ""))
  {
    importStartValues(data, pInitFile, initTime);
    updateStartValues = 0;
  }

  /* if there are user-specified options, use them! */
  if(pInitMethod && strcmp(pInitMethod, ""))
  {
    initMethod = IIM_UNKNOWN;

    for(i=1; i<IIM_MAX; ++i)
    {
      if(!strcmp(pInitMethod, initMethodStr[i]))
        initMethod = i;
    }

    if(initMethod == IIM_UNKNOWN)
    {
      WARNING1("unrecognized option -iim %s", pInitMethod);
      WARNING_AL("current options are:");
      for(i=1; i<IIM_MAX; ++i)
        WARNING_AL2("  %-15s [%s]", initMethodStr[i], initMethodDescStr[i]);
      THROW("see last warning");
    }
  }

  if(pOptiMethod && strcmp(pOptiMethod, ""))
  {
    optiMethod = IOM_UNKNOWN;

    for(i=1; i<IOM_MAX; ++i)
    {
      if(!strcmp(pOptiMethod, optiMethodStr[i]))
        optiMethod = i;
    }

    if(optiMethod == IOM_UNKNOWN)
    {
      WARNING1("unrecognized option -iom %s", pOptiMethod);
      WARNING_AL("current options are:");
      for(i=1; i<IOM_MAX; ++i)
        WARNING_AL2("  %-15s [%s]", optiMethodStr[i], optiMethodDescStr[i]);
      THROW("see last warning");
    }
  }

  DEBUG_INFO2(LOG_INIT,    "initialization method: %-15s [%s]", initMethodStr[initMethod], initMethodDescStr[initMethod]);
  DEBUG_INFO_AL2(LOG_INIT, "optimization method:   %-15s [%s]", optiMethodStr[optiMethod], optiMethodDescStr[optiMethod]);
  DEBUG_INFO_AL1(LOG_INIT, "update start values:   %s", updateStartValues ? "true" : "false");

  /* start with the real initialization */
  data->simulationInfo.initial = 1;             /* to evaluate when-equations with initial()-conditions */
  /* select the right initialization-method */
  if(initMethod == IIM_NONE)
    retVal = none_initialization(data, updateStartValues);
  else if(initMethod == IIM_STATE)
    retVal = state_initialization(data, optiMethod, updateStartValues);
  else
    THROW("unsupported option -iim");
  data->simulationInfo.initial = 0;

  DEBUG_INFO(LOG_INIT, "### END INITIALIZATION ###");
  return retVal;
}