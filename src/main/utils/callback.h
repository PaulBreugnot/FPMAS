#ifndef CALLBACK_H
#define CALLBACK_H

#include "api/utils/callback.h"

namespace FPMAS::utils {
	template<typename... Args>
	class VoidCallback : public api::utils::Callback<Args...> {
		public:
			void call(Args...) override {}
	};
}
#endif
