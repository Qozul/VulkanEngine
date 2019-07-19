/// Author: Ralph Ridley
/// Date: 13/01/19
#include "PerfMeasurer.h"

using namespace QZL;
using namespace QZL::Shared;

void PerfMeasurer::startTime()
{
	currentStartTime_ = PerfMeasurer::Timer::now();
}

void PerfMeasurer::endTime()
{
	totalTime_ += PerfMeasurer::Timer::now() - currentStartTime_;
	++numTimes_;
}

PerfMeasurer::Resolution PerfMeasurer::getAverageTime()
{
	return totalTime_ / numTimes_;
}
