/*******************************************************************
 * File:            NumUtil.cpp
 * Description:      
 * Author:          Subhasis Ray
 * E-mail:          ray.subhasis@gmail.com
 * Created:         2007-11-02 11:46:40
 ********************************************************************/
/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment,
** also known as GENESIS 3 base code.
**           copyright (C) 2003-2005 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU General Public License version 2
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _NUMUTIL_CPP
#define _NUMUTIL_CPP
#include <cmath>
bool isEqual(float x, float y, float epsilon)
{
	if (fabs(x) > fabs(y)) 
	{
		return fabs(x - y) < fabs(epsilon*x);
	}
	else
	{
		return fabs(x-y) < fabs(epsilon*y);
	}
}
bool isEqual(double x, double y, double epsilon)
{
	if (fabs(x) > fabs(y)) 
	{
		return fabs(x - y) < fabs(epsilon*x);
	}
	else
	{
		return fabs(x-y) < fabs(epsilon*y);
	}
}
bool isEqual(long double x, long double y, long double epsilon)
{
	if (fabs(x) > fabs(y)) 
	{
		return fabs(x - y) < fabs(epsilon*x);
	}
	else
	{
		return fabs(x-y) < fabs(epsilon*y);
	}
}
#endif
