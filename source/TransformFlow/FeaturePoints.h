//
//  FeaturePoints.h
//  Transform Flow
//
//  Created by Samuel Williams on 1/03/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef _TRANSFORM_FLOW_FEATURE_POINTS_H
#define _TRANSFORM_FLOW_FEATURE_POINTS_H

#include <vector>
#include <Dream/Imaging/Image.h>
#include <Euclid/Numerics/Matrix.h>
#include <Euclid/Geometry/Line.h>

#include "FeatureTable.h"

// Dense optical flow vs Sparse optical flow
// Look at stereo image processing (correspondence between rectified pairs of images)
namespace TransformFlow {
	
	/*
		Given a set of feature points for one image, we need to find corresponding points in another image with an arbitrary transform. Typically, this can be considered a translation from one point to another, since the transform is normally small (due to frame rate). In this sense:
		
		Original image + feature points + translation + next image
				= per feature point pixel translation with confidence rating.
	 */
	
	using namespace Dream;
	using namespace Euclid::Numerics;
	using namespace Euclid::Geometry;
	using namespace Dream::Imaging;
	
	class FeaturePoints : public Object {
	protected:
		std::vector<Vec2> _offsets;
		Ref<FeatureTable> _table;
		
		Ref<Image> _source;
		
		static void features_along_line(Ptr<Image> image, Vec2i start, Vec2i end, const unsigned estimate, std::vector<Vec2> & features);

		std::vector<LineSegment2> _segments;
		AlignedBox2 _bounding_box;

	public:		
		FeaturePoints();
		virtual ~FeaturePoints();

		// dy is the distance between scanlines. estimate is the size of the expected motion blur.
		void scan(Ptr<Image> source, const Radians<> & gravity_rotation, RealT estimate = 1, std::size_t dy = 15);

		Ref<FeatureTable> table() { return _table; }

		Ref<Image> source() const { return _source; }
		const std::vector<Vec2> & offsets() const { return _offsets; }

		const std::vector<LineSegment2> & segments() const { return _segments; }

		const AlignedBox2 & bounding_box() const { return _bounding_box; }
	};
}

#endif
