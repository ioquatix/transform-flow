//
//  FeatureAlgorithm.h
//  Transform Flow
//
//  Created by Samuel Williams on 22/08/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef __Transform_Flow__FeatureAlgorithm__
#define __Transform_Flow__FeatureAlgorithm__

#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include <Dream/Class.h>
#include "VideoStream.h"

namespace TransformFlow {
	class MatchingAlgorithm : public Object {
	protected:
		class FrameAnalysis {
			cv::Mat descriptors;

			Mat44 homography;
		};

		Ref<ObjectCache<FeaturePoints>> _frame_cache;

		std::string _name;
		Shared<cv::FeatureDetector> _detector;
		Shared<cv::DescriptorExtractor> _extractor;
		Shared<cv::DescriptorMatcher> _matcher;

		void detect_features(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints);
		void extract_features(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints, cv::Mat & descriptors) const;
		void match_features(const cv::Mat & query, const cv::Mat & train, std::vector<cv::DMatch> & matches) const;

	public:
		MatchingAlgorithm(std::string name, Shared<cv::FeatureDetector> detector, Shared<cv::DescriptorExtractor> extractor, Shared<cv::DescriptorMatcher> matcher);

		virtual Vec2 calculate_local_translation(const ImageUpdate & initial, const ImageUpdate & next);
		virtual Mat44 calculate_local_transform(const ImageUpdate & initial, const ImageUpdate & next);
	};

	Ref<MatchingAlgorithm> matchingAlgorithmUsingORB();
}

#endif /* defined(__Transform_Flow__FeatureAlgorithm__) */
