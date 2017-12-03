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
	namespace System
	{
		bool LaunchBrowser(const FilePath& url);

		// [Siv3D ToDo]
		inline double DeltaTime()
		{
			return 0.016;
		}
	}
}