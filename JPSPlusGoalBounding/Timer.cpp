/*
 * $Id: timer.cpp,v 1.6 2006/11/30 20:33:25 nathanst Exp $
 *
 * timer.cpp
 * HOG file
 * 
 * Written by Renee Jansen on 08/28/06
 *
 * This file is part of HOG.
 *
 * HOG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * HOG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with HOG; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//#include "UnitSimulation.h"
#include "stdafx.h"
#include "Timer.h"
#include <stdint.h>
#include <cstring>
#include <windows.h>

Timer::Timer()
{
	QueryPerformanceFrequency(&ticksPerSecond);
	startTimeInUS = 0;
	elapsedTime = 0;
}

void Timer::StartTimer()
{
	startTimeInUS = GetHighestResolutionTime();
}

double Timer::EndTimer()
{
	double currentTimeInUS = GetHighestResolutionTime();
	double elapsedTimeInUS = currentTimeInUS - startTimeInUS;
	elapsedTime = elapsedTimeInUS;

	return elapsedTime;

}
