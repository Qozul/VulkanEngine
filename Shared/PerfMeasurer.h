/// Author: Ralph Ridley
/// Date: 13/01/19
/// Purpose: Provide functionality for measuring duration for performance testing
#pragma once
#include <chrono>
#include <vector>

namespace QZL
{
	namespace Shared
	{
		class PerfMeasurer {
		public:
			using Timer = std::chrono::high_resolution_clock;
			using Resolution = std::chrono::nanoseconds;

			PerfMeasurer() : currentStartTime_(Timer::now()), totalTime_(0), numTimes_(0) {};
			/// Measure the current time
			void startTime();
			/// Get the duration since startTime was last called, and add duration to toal time measured
			void endTime();
			/// Get the average time of each duration measured
			Resolution getAverageTime();

		private:
			Timer::time_point currentStartTime_;
			Resolution totalTime_;
			long long numTimes_;
		};
	}
}
