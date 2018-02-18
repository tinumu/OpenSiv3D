﻿//-----------------------------------------------
//
//	This file is part of the Siv3D Engine.
//
//	Copyright (c) 2008-2018 Ryo Suzuki
//	Copyright (c) 2016-2018 OpenSiv3D Project
//
//	Licensed under the MIT License.
//
//-----------------------------------------------

# include <opencv2/imgproc.hpp>
# include <Siv3D/Image.hpp>
# include <Siv3D/PointVector.hpp>
# include <Siv3D/Rectangle.hpp>
# include <Siv3D/Circle.hpp>
# include <Siv3D/Polygon.hpp>
# include "PaintShape.hpp"

namespace s3d
{
	namespace detail
	{
		void WritePaintBufferReference(
			Color* dst,
			const unsigned* offsets,
			size_t offsetCount,
			const Color& color
		)
		{
			const uint32 srcBlend = color.a;
			const uint32 dstBlend = 255 - srcBlend;
			const uint32 premulSrcR = srcBlend * color.r;
			const uint32 premulSrcG = srcBlend * color.g;
			const uint32 premulSrcB = srcBlend * color.b;
			
			for (size_t i = 0; i < offsetCount; ++i)
			{
				Color* pDst = dst + offsets[i];
				pDst->r = (pDst->r * dstBlend + premulSrcR) / 255;
				pDst->g = (pDst->g * dstBlend + premulSrcG) / 255;
				pDst->b = (pDst->b * dstBlend + premulSrcB) / 255;
			}
		}
	}

	const Point& Point::paint(Image& dst, const Color& color) const
	{
		if (x < 0 || dst.width() <= x || y < 0 || dst.height() <= y)
		{
			return *this;
		}

		const uint32 srcBlend = color.a;
		const uint32 dstBlend = 255 - srcBlend;

		Color* const pDst = dst.data() + y * dst.width() + x;
		pDst->r = ((pDst->r * dstBlend) + (color.r * srcBlend)) / 255;
		pDst->g = ((pDst->g * dstBlend) + (color.g * srcBlend)) / 255;
		pDst->b = ((pDst->b * dstBlend) + (color.b * srcBlend)) / 255;

		return *this;
	}

	const Point& Point::overwrite(Image& dst, const Color& color) const
	{
		if (x < 0 || dst.width() <= x || y < 0 || dst.height() <= y)
		{
			return *this;
		}

		Color* const pDst = dst.data() + y * dst.width() + x;
		*pDst = color;

		return *this;
	}

	const Line& Line::paint(Image& dst, const Color& color) const
	{
		return paint(dst, 1, color);
	}

	const Line& Line::paint(Image& dst, int32 thickness, const Color& color) const
	{
		if (!dst || thickness < 1)
		{
			return *this;
		}

		Array<uint32> paintBuffer;

		PaintShape::PaintLine(paintBuffer, *this, dst.width(), dst.height(), thickness);

		if (paintBuffer)
		{
			detail::WritePaintBufferReference(dst[0], paintBuffer.data(), paintBuffer.size(), color);
		}

		return *this;
	}

	const Line& Line::overwrite(Image& dst, const Color& color) const
	{
		return overwrite(dst, 1, color);
	}

	const Line& Line::overwrite(Image& dst, int32 thickness, const Color& color) const
	{
		if (!dst || thickness < 1)
		{
			return *this;
		}

		cv::Mat_<cv::Vec4b> mat(dst.height(), dst.width(), static_cast<cv::Vec4b*>(static_cast<void*>(dst.data())), dst.stride());

		cv::line(mat,
			{ static_cast<int32>(begin.x), static_cast<int32>(begin.y) },
			{ static_cast<int32>(end.x), static_cast<int32>(end.y) },
			cv::Scalar(color.r, color.g, color.b, color.a), thickness);

		return *this;
	}

	const Circle& Circle::paint(Image& dst, const Color& color, bool antialiased) const
	{
		const int32 yBegin = std::max(static_cast<int32>(y - r - 1), 0);
		const int32 yEnd = std::min(static_cast<int32>(y + r + 1), dst.height());
		const int32 xBegin = std::max(static_cast<int32>(x - r - 1), 0);
		const int32 xEnd = std::min(static_cast<int32>(x + r + 1), dst.width());
		const int32 fillWidth = xEnd - xBegin;
		const int32 fillHeight = yEnd - yBegin;

		if (fillWidth <= 0 || fillHeight <= 0)
		{
			return *this;
		}

		Color* pDst = dst.data() + yBegin * dst.width() + xBegin;
		const int32 stepOffset = dst.width() - fillWidth;
		const double lengthSq = (r + 0.5) * (r + 0.5);

		const uint32 srcBlend = color.a;

		if (antialiased)
		{
			const Vec2 center2(center.movedBy(-0.5, -0.5));
			const double length = std::sqrt(lengthSq);

			if (srcBlend == 255)
			{
				for (int32 _y = yBegin; _y < yEnd; ++_y)
				{
					for (int32 _x = xBegin; _x < xEnd; ++_x)
					{
						const double d = length - center2.distanceFrom(Vec2(_x, _y));

						if (d > 1.0)
						{
							const uint8 a = pDst->a;
							*pDst = color;
							pDst->a = a;
						}
						else if(d > 0.0)
						{			
							const uint32 srcBlend2 = static_cast<uint32>(255 * d);
							const uint32 premulSrcR = srcBlend2 * color.r;
							const uint32 premulSrcG = srcBlend2 * color.g;
							const uint32 premulSrcB = srcBlend2 * color.b;
							const uint32 dstBlend = 255 - srcBlend2;

							pDst->r = (pDst->r * dstBlend + premulSrcR) / 255;
							pDst->g = (pDst->g * dstBlend + premulSrcG) / 255;
							pDst->b = (pDst->b * dstBlend + premulSrcB) / 255;
						}

						++pDst;
					}

					pDst += stepOffset;
				}
			}
			else
			{
				const uint32 premulSrcR = srcBlend * color.r;
				const uint32 premulSrcG = srcBlend * color.g;
				const uint32 premulSrcB = srcBlend * color.b;
				const uint32 dstBlend = 255 - srcBlend;

				for (int32 _y = yBegin; _y < yEnd; ++_y)
				{
					for (int32 _x = xBegin; _x < xEnd; ++_x)
					{
						const double d = length - center2.distanceFrom(Vec2(_x, _y));

						if (d > 1.0)
						{
							pDst->r = (pDst->r * dstBlend + premulSrcR) / 255;
							pDst->g = (pDst->g * dstBlend + premulSrcG) / 255;
							pDst->b = (pDst->b * dstBlend + premulSrcB) / 255;
						}
						else if (d > 0.0)
						{
							const uint32 srcBlend2 = static_cast<uint32>(srcBlend * d);
							const uint32 premulSrcR2 = srcBlend2 * color.r;
							const uint32 premulSrcG2 = srcBlend2 * color.g;
							const uint32 premulSrcB2 = srcBlend2 * color.b;
							const uint32 dstBlend2 = 255 - srcBlend2;

							pDst->r = (pDst->r * dstBlend2 + premulSrcR2) / 255;
							pDst->g = (pDst->g * dstBlend2 + premulSrcG2) / 255;
							pDst->b = (pDst->b * dstBlend2 + premulSrcB2) / 255;
						}

						++pDst;
					}

					pDst += stepOffset;
				}
			}
		}
		else
		{
			if (srcBlend == 255)
			{
				for (int32 _y = yBegin; _y < yEnd; ++_y)
				{
					for (int32 _x = xBegin; _x < xEnd; ++_x)
					{
						if (center.distanceFromSq(Vec2(_x, _y)) <= lengthSq)
						{
							const uint8 a = pDst->a;
							*pDst = color;
							pDst->a = a;
						}

						++pDst;
					}

					pDst += stepOffset;
				}
			}
			else
			{
				const uint32 premulSrcR = srcBlend * color.r;
				const uint32 premulSrcG = srcBlend * color.g;
				const uint32 premulSrcB = srcBlend * color.b;
				const uint32 dstBlend = 255 - srcBlend;

				for (int32 _y = yBegin; _y < yEnd; ++_y)
				{
					for (int32 _x = xBegin; _x < xEnd; ++_x)
					{
						if (center.distanceFromSq(Vec2(_x, _y)) <= lengthSq)
						{
							pDst->r = (pDst->r * dstBlend + premulSrcR) / 255;
							pDst->g = (pDst->g * dstBlend + premulSrcG) / 255;
							pDst->b = (pDst->b * dstBlend + premulSrcB) / 255;
						}

						++pDst;
					}

					pDst += stepOffset;
				}
			}
		}

		return *this;
	}

	const Circle& Circle::overwrite(Image& dst, const Color& color) const
	{
		const int32 yBegin	= std::max(static_cast<int32>(y - r), 0);
		const int32 yEnd	= std::min(static_cast<int32>(y + r + 1), dst.height());
		const int32 xBegin	= std::max(static_cast<int32>(x - r), 0);
		const int32 xEnd	= std::min(static_cast<int32>(x + r + 1), dst.width());
		const int32 fillWidth	= xEnd - xBegin;
		const int32 fillHeight	= yEnd - yBegin;

		if (fillWidth <= 0 || fillHeight <= 0)
		{
			return *this;
		}

		const int32 stepOffset = dst.width() - fillWidth;
		const double lengthSq = (r + 0.5) * (r + 0.5);

		Color* pDst = dst.data() + yBegin * dst.width() + xBegin;

		for (int32 _y = yBegin; _y < yEnd; ++_y)
		{
			for (int32 _x = xBegin; _x < xEnd; ++_x)
			{
				if (center.distanceFromSq(Vec2(_x, _y)) <= lengthSq)
				{
					*pDst = color;
				}

				++pDst;
			}

			pDst += stepOffset;
		}

		return *this;
	}

	const Polygon& Polygon::paint(Image& dst, const Color& color) const
	{
		if (!dst || isEmpty())
		{
			return *this;
		}

		//
		// outer
		//
		Array<Point> outerPts;

		for (const auto& pt : outer())
		{
			outerPts.push_back(pt.asPoint());
		}

		//
		//	holes
		//
		Array<Array<Point>> holePtsList(inners().size());

		uint32 holeIndex = 0;

		for (const auto& hole : inners())
		{
			for (const auto& pt : hole)
			{
				holePtsList[holeIndex].push_back(pt.asPoint());
			}

			++holeIndex;
		}

		cv::Mat_<cv::Vec4b> mat(dst.height(), dst.width(), static_cast<cv::Vec4b*>(static_cast<void*>(dst.data())), dst.stride());

		Array<const Point*> ppts;
		{
			ppts.push_back(outerPts.data());

			for (const auto& hole : holePtsList)
			{
				ppts.push_back(hole.data());
			}
		}

		Array<int32> npts;
		{
			npts.push_back(static_cast<int32>(outerPts.size()));

			for (const auto& hole : holePtsList)
			{
				npts.push_back(static_cast<int32>(hole.size()));
			}
		}

		Array<uint32> paintBuffer;

		PaintShape::PaintPolygon(paintBuffer, ppts, npts, dst.width(), dst.height());

		if (paintBuffer.empty())
		{
			return *this;
		}

		detail::WritePaintBufferReference(dst[0], paintBuffer.data(), paintBuffer.size(), color);

		return *this;
	}

	const Polygon& Polygon::overwrite(Image& dst, const Color& color) const
	{
		if (!dst || isEmpty())
		{
			return *this;
		}

		//
		// outer
		//
		Array<cv::Point> outerPts;

		for (const auto& pt : outer())
		{
			outerPts.emplace_back(static_cast<int32>(pt.x), static_cast<int32>(pt.y));
		}

		//
		//	holes
		//
		Array<Array<cv::Point>> holePtsList(inners().size());

		uint32 holeIndex = 0;

		for (const auto& hole : inners())
		{
			for (const auto& pt : hole)
			{
				holePtsList[holeIndex].emplace_back(static_cast<int32>(pt.x), static_cast<int32>(pt.y));
			}

			++holeIndex;
		}

		cv::Mat_<cv::Vec4b> mat(dst.height(), dst.width(), static_cast<cv::Vec4b*>(static_cast<void*>(dst.data())), dst.stride());

		Array<const cv::Point*> ppts;
		{
			ppts.push_back(outerPts.data());

			for (const auto& hole : holePtsList)
			{
				ppts.push_back(hole.data());
			}
		}

		Array<int32> npts;
		{
			npts.push_back(static_cast<int32>(outerPts.size()));

			for (const auto& hole : holePtsList)
			{
				npts.push_back(static_cast<int32>(hole.size()));
			}
		}

		cv::fillPoly(mat, ppts.data(), npts.data(), static_cast<int32>(ppts.size()), cv::Scalar(color.r, color.g, color.b, color.a));

		return *this;
	}
}