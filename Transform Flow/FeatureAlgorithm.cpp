//
//  FeatureAlgorithm.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 22/08/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "FeatureAlgorithm.h"

#include <Dream/Events/Logger.h>
#include <Euclid/Numerics/Matrix.IO.h>

namespace TransformFlow {
	using namespace Dream::Events::Logging;

	MatchingAlgorithm::MatchingAlgorithm(std::string name, Shared<cv::FeatureDetector> detector, Shared<cv::DescriptorExtractor> extractor, Shared<cv::DescriptorMatcher> matcher) : _name(name), _detector(detector), _extractor(extractor), _matcher(matcher) {

	}

	void MatchingAlgorithm::detect_features(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints) {
		_detector->detect(image, keypoints);
	}

	void MatchingAlgorithm::extract_features(const cv::Mat & image, std::vector<cv::KeyPoint> & keypoints, cv::Mat & descriptors) const {
		_extractor->compute(image, keypoints, descriptors);
	}

	void MatchingAlgorithm::match_features(const cv::Mat & query, const cv::Mat & train, std::vector<cv::DMatch> & matches) const {
		_matcher->match(query, train, matches);
	}

	static void convert_to_greyscale(Ref<Image> pixel_buffer, cv::Mat & output) {
		Vec3u size = pixel_buffer->size();
		
		cv::Mat color_frame(size[Y], size[X], CV_8UC4, (void*)pixel_buffer->data());

		output = cv::Mat(size[Y], size[X], CV_8UC1);
		cv::cvtColor(color_frame, output, CV_RGB2GRAY);
	}

	Vec2 MatchingAlgorithm::calculate_local_translation(ImageUpdate & initial, ImageUpdate & next) {
		Vec3u size = initial.image_buffer->size();
		
		cv::Mat initial_frame(size[Y], size[X], CV_8UC4, (void*)initial.image_buffer->data());
		cv::Mat next_frame(size[Y], size[X], CV_8UC4, (void*)next.image_buffer->data());

		std::clock_t start_time = std::clock();
		std::vector<cv::KeyPoint> initial_keypoints;
		detect_features(initial_frame, initial_keypoints);
		std::clock_t features_time = std::clock();

		std::vector<cv::Point2f> initial_points, next_points;
		cv::KeyPoint::convert(initial_keypoints, initial_points);

		std::vector<uint8_t> status;
		std::vector<float> error;

		cv::calcOpticalFlowPyrLK(initial_frame, next_frame, initial_points, next_points, status, error);
		std::clock_t flow_time = std::clock();

		double feature_duration = double(features_time - start_time) / CLOCKS_PER_SEC;
		double optical_flow_duration = double(flow_time - features_time) / CLOCKS_PER_SEC;
		logger()->log(LOG_DEBUG, LogBuffer() << "Feature duration = " << feature_duration << " Optical flow duration = " << optical_flow_duration);

		Vec2 total_translation(ZERO);
		std::size_t samples = 0;

		for (std::size_t i = 0; i < status.size(); i += 1) {
			if (status[i]) {
				auto a = initial_points[i], b = next_points[i];

				total_translation += Vec2(b.x - a.x, b.y - a.y);
				samples += 1;
			}
		}

		return total_translation / samples;
	}

	Mat44 MatchingAlgorithm::calculate_local_transform(ImageUpdate & initial, ImageUpdate & next) {
		cv::Mat initial_frame, next_frame;

		convert_to_greyscale(initial.image_buffer, initial_frame);
		convert_to_greyscale(next.image_buffer, next_frame);

		std::vector<cv::KeyPoint> initial_keypoints, next_keypoints;
		detect_features(initial_frame, initial_keypoints);
		detect_features(next_frame, next_keypoints);

		logger()->log(LOG_DEBUG, LogBuffer() << "Found keypoints initial = " << initial_keypoints.size() << ", next = " << next_keypoints.size());

		cv::Mat initial_descriptors, next_descriptors;
		extract_features(initial_frame, initial_keypoints, initial_descriptors);
		extract_features(next_frame, next_keypoints, next_descriptors);

		std::vector<cv::DMatch> matches;
		match_features(initial_descriptors, next_descriptors, matches);

		logger()->log(LOG_DEBUG, LogBuffer() << "Found matches = " << matches.size());

		std::vector<cv::Point2f> initial_points, next_points;
		for (cv::DMatch & match : matches) {
			initial_points.push_back(initial_keypoints[match.queryIdx].pt);
			next_points.push_back(next_keypoints[match.trainIdx].pt);
		}

		cv::KeyPoint::convert(initial_keypoints, initial_points);
		cv::KeyPoint::convert(next_keypoints, next_points);

		cv::Mat fundamental_matrix = cv::findFundamentalMat(initial_points, next_points);

		//cv::Mat homography = cv::findHomography(initial_points, next_points, CV_RANSAC);
		//logger()->log(LOG_DEBUG, LogBuffer() << "Homography: " << std::endl << homography);

		// Decompose the projection matrix into:
		//cv::Mat camera_matrix(3,3, cv::DataType<float>::type); // intrinsic parameter matrix
		//cv::Mat rotation_matrix(3,3, cv::DataType<float>::type); // rotation matrix
		//cv::Mat translation_matrix(4,1, cv::DataType<float>::type); // translation vector
		//cv::decomposeProjectionMatrix(homography, camera_matrix, rotation_matrix, translation_matrix);

		//Mat33 rotation(homography.ptr<float>());
		Mat33 rotation = *fundamental_matrix.ptr<float>();

		Mat44 transform = rotation;

		logger()->log(LOG_DEBUG, LogBuffer() << "Transform: " << std::endl << transform);

		return transform;
	}

	Ref<MatchingAlgorithm> matchingAlgorithmUsingORB() {
		Shared<cv::FeatureDetector> detector = new cv::OrbFeatureDetector;
		Shared<cv::DescriptorExtractor> extractor = new cv::OrbDescriptorExtractor;
		Shared<cv::DescriptorMatcher> matcher = new cv::BFMatcher(cv::NORM_HAMMING, true);

		return new MatchingAlgorithm("ORB/ORB/BF", detector, extractor, matcher);
	}
}