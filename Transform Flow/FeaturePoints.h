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
	using namespace Dream::Imaging;
	
	struct FeaturePoint {
		Vec2u offset;
		
		// The colour identity along the gradient:
		Vec3 a, b;

		// The gradient of the feature point:
		Vec3 gradient;
		
		float similarity(const FeaturePoint & other);
	};
	
	class FeaturePoints : public Object {
	protected:
		std::vector<Vec2> _offsets;
		Ref<Image> _source;
		
		static void features_along_line(Ptr<Image> image, Vec2i start, Vec2i end, std::vector<Vec2> & features);
		
	public:		
		FeaturePoints();
		virtual ~FeaturePoints();

		void scan(Ptr<Image> image);
		
		const std::vector<Vec2> & offsets() const { return _offsets; }
	};
}

#endif
