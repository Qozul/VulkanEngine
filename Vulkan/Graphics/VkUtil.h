#pragma once
#include "../../Shared/Utility.h"
#include "OptionalExtensions.h"
#include "../SystemMasters.h"

namespace QZL
{
	namespace Graphics {
#define CHECK_VKRESULT(result) if (result != VK_SUCCESS) { auto str = "Check VK error code " + std::to_string(static_cast<int>(result)) \
+ " in file " + __FILE__ + " at line " + std::to_string(__LINE__); DEBUG_LOG(str); throw std::runtime_error(str); };
#define NOTHROW_CHECK_VKRESULT(result) if (result != VK_SUCCESS) std::cout << "Check VK error code " << std::to_string(static_cast<int>(result)) \
	<< " in file" << __FILE__ << " at line " << __LINE__ << std::endl;

		using MeshLoadFunc = void(*)(uint32_t& count, std::vector<char>& indices, std::vector<char>& vertices);

		/// Utility function for getting data from Vulkan where it needs to call a function to get
		/// a count of the data and then call it again to get the data.
		/// Can be called succinctly with the following basic line:
		/// auto x = obtainVkData<R>(func, arg0, arg1, ..., argN);
		template<class R, typename FP, typename... Args>
		inline std::vector<R> obtainVkData(FP funcPtr, Args&& ... args)
		{
			uint32_t count = 0;
			funcPtr(std::forward<Args>(args)..., &count, nullptr);
			if (count == 0) {
				DEBUG_LOG(("Obtain vkdata found no data for type " + std::string(typeid(R).name())));
			}
			std::vector<R> result(count);
			funcPtr(std::forward<Args>(args)..., &count, result.data());
			return result;
		}
	}
}
