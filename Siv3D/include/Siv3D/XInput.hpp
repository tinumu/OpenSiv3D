﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2017 Ryo Suzuki
//	Copyright (c) 2016-2017 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# pragma once
# include "Fwd.hpp"

namespace s3d
{
	struct DeadZone
	{

	};

	class XInput
	{
	private:

		uint32 m_userIndex = 0;

	public:

		constexpr XInput(uint32 userIndex = 0) noexcept
			: m_userIndex(userIndex) {}
	};
}
