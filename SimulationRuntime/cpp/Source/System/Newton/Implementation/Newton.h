
#pragma once

#include "System/Interfaces/IAlgLoop.h"				// Interface to AlgLoop
#include "System/Interfaces/IAlgLoopSolver.h"		// Export function from dll

#include "System/Newton/Interfaces/INewtonSettings.h"



/*****************************************************************************/
/**

Damped Newton-Raphson Method

The purpose of Newton is to find a zero of a system F of n nonlinear functions in n 
variables y of the form

F(t,y_1,...,y_n) = 0,				(1)

or

f_1(t,y_1,...,y_n) = 0
...				   ...
f_n(t,y_1,...,y_n) = 0

by the use of an iterative Newton method. The solution of the linear system is done 
by Lapack/DGESV, which computes the solution to a real system of linear equations

A * y = B,							(2)

where A is an n-by-n matrix and y and B are n-by-n(right hand side) matrices. 

\date     2008, September, 16th
\author   

*/
/*****************************************************************************
OSMS(c) 2008
*****************************************************************************/
class Newton : public IAlgLoopSolver
{
public:

	Newton(IAlgLoop* algLoop,INewtonSettings* settings);

	virtual ~Newton();

	/// (Re-) initialize the solver
	virtual void init();

	/// Solution of a (non-)linear system of equations
	virtual void solve(const IContinous::UPDATE command = IContinous::UNDEF_UPDATE);

	/// Returns the status of iteration
	virtual ITERATIONSTATUS getIterationStatus();
	

private:
	/// Encapsulation of determination of residuals to given unknowns
	void calcFunction(const double* y, double* residual);

	/// Encapsulation of determination of Jacobian
	void calcJacobian(); 


	// Member variables
	//---------------------------------------------------------------
	INewtonSettings
		*_newtonSettings;			///< Settings for the solver

	IAlgLoop
		*_algLoop;					///< Algebraic loop to be solved

	ITERATIONSTATUS		
		_iterationStatus;			///< Output		- Denotes the status of iteration

	long int			
		_dimSys;					///< Temp		- Number of unknowns (=dimension of system of equations)

	bool
		_firstCall;					///< Temp		- Denotes the first call to the solver, init() is called

	double
		*_y,						///< Temp		- Unknowns
		*_f,						///< Temp		- Residuals
		*_yHelp,					///< Temp		- Auxillary variables
		*_fHelp,					///< Temp		- Auxillary variables
		*_jac;						///< Temp		- Jacobian
};
