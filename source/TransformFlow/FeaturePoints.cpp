//
//  FeaturePoints.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 1/03/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "FeaturePoints.h"
#include <Dream/Events/Logger.h>
#include <Euclid/Geometry/AlignedBox.h>

namespace TransformFlow {
	
	using namespace Dream::Events::Logging;
	using namespace Euclid::Geometry;

	// Bresenham's Line Drawing Algorithm
	void FeaturePoints::features_along_line(Ptr<Image> image, Vec2i start, Vec2i end, std::vector<Vec2> & features) {
		// We want the algorithm to work with the origin in the bottom left, not the top left.
		start[Y] = image->size()[HEIGHT] - start[Y];
		end[Y] = image->size()[HEIGHT] - end[Y];

		bool steep = abs(end[Y] - start[Y]) > abs(end[X] - start[X]);
		
		if (steep) {
			std::swap(start[X], start[Y]);
			std::swap(end[X], end[Y]);
		}
		
		if (start[X] > end[X]) {
			std::swap(start[X], end[X]);
			std::swap(start[Y], end[Y]);
		}
		
		int dx = end[X] - start[X];
		int dy = abs(end[Y] - start[Y]);
		int error = dx / 2;
		
		int ystep = -1;
		int y = start[Y];
		
		if (start[Y] < end[Y])
			ystep = 1;
		
		Vec2i offset[2];
		Vector<3, unsigned char> pixel[2];
		unsigned count = 0, skip = 0;
		//std::size_t maximum_error = (255 * 255) * 3;
		
		unsigned step = std::max<unsigned>((end - start).length() / 100, 1);

		auto image_reader = reader(*image);
		//auto image_writer = writer(*image);

		for (int x = start[X]; x < end[X]; x += 1) {
			if (steep) {
				offset[0] = vector(y, x);
			} else {
				offset[0] = vector(x, y);
			}

			pixel[0] = image_reader[offset[0]];
			//image->read_pixel(offset[0] << 0, pixel[0]);
			if (count > 0 && skip == 0) {
				// Calculate the distance between the two pixels:
				RealT distance = 0;
				
				for (std::size_t i = 0; i < 3; i += 1) {
					int d = (int)pixel[1][i] - (int)pixel[0][i];
					distance += (d*d);
				}
				
				//logger()->log(LOG_DEBUG, LogBuffer() << "Pixel[0] = " << pixel[0] << " Pixel[1] = " << pixel[1] << " Distance = " << distance);
				
				if (distance > 5000) {
					Vec2 middle = offset[0] + offset[1];
					middle /= 2.0;
					//Vec2 middle = offset[1];
					
					middle[Y] = image->size()[HEIGHT] - (middle[Y] + 1);
					middle += 0.5;
					features.push_back(middle);

					skip = step;
				}
			}
			
			if (skip)
				skip -= 1;
			
			// Calculate the next step
			error = error - dy;
			
			if (error < 0) {
				y = y + ystep;
				error = error + dx;
			}
			
			count += 1;
			offset[1] = offset[0];
			pixel[1] = pixel[0];

			//image_writer.set(offset[0], Vector<3, ByteT>{0, 255, 0});
		}
	}
	
	FeaturePoints::FeaturePoints() {
		
	}
	
	FeaturePoints::~FeaturePoints() {
		
	}
	
	void FeaturePoints::scan(Ptr<Image> source) {
		if (_offsets.size()) return;
		
		_source = source;
		
		Vec2u size = source->size();
		
		std::size_t dy = std::max<std::size_t>(size[Y] / 40, 2);
		std::size_t dx = std::max<std::size_t>(size[X] / 40, 2);

		//for (std::size_t y = dy; y < size[Y]; y += dy) {
		//	features_along_line(source, Vec2i(dx, y), Vec2i(size[X] - dx, y), _offsets);
		//}
		
		for (std::size_t x = dx; x < size[X]; x += dx) {
			features_along_line(source, Vec2i(x, dy), Vec2i(x, size[Y] - dy), _offsets);
		}

		_table = new FeatureTable(size.length() / 3, AlignedBox2::from_origin_and_size(0, size), R90);
		_table->update(_offsets);

		// logger()->log(LOG_INFO, LogBuffer() << "Found " << _offsets.size() << " feature points");
	}

	void FeaturePoints::scan(Ptr<Image> source, const Radians<> & tilt)
	{
		if (_offsets.size()) return;
		
		_source = source;
		AlignedBox2 image_box(ZERO, _source->size());
		image_box.set_center_and_size(image_box.center(), image_box.size() * 0.9);

		{
			AlignedBox2 bounds = ZERO;

			// Forward rotation, create a bounding box where -y is "down".
			Mat22 rotation = rotate<Z>(tilt);

			Vec2 size = _source->size();

			bounds.union_with_point(rotation * size);
			bounds.union_with_point(rotation * Vec2(size[X], 0));
			bounds.union_with_point(rotation * Vec2(0, size[Y]));

			_bounding_box = bounds;
		}

		{
			// Now we need to enumerate lines in the "rotated" space, and translate them back to image space:
			Mat22 rotation = rotate<Z>(-tilt);

			auto size = _bounding_box.size();

			auto dy = std::max<std::size_t>(size.length() / 40, 2);
			auto dx = std::max<std::size_t>(size.length() / 40, 2);

			for (auto y = _bounding_box.min()[Y] + dy; (y + dy) < _bounding_box.max()[Y]; y += dy) {
				Vec2 min(_bounding_box.min()[X], y), max(_bounding_box.max()[X], y);

				// This segment is now in image space, perpendicular to gravity.
				LineSegment2 segment(rotation * min, rotation * max), clipped_segment;
				//_segments.push_back(segment);

				//log_debug("Features along line segment:", segment.start(), segment.end(), segment.direction());

				if (segment.clip(image_box, clipped_segment)) {
					//log_debug("Clipped line segment:", segment.start(), segment.end(), segment.direction());
					
					//DREAM_ASSERT(clipped_segment.direction().equivalent(segment.direction()));

					_segments.push_back(clipped_segment);

					features_along_line(source, clipped_segment.start(), clipped_segment.end(), _offsets);
				}
			}
		}

		_table = new FeatureTable(_bounding_box.size().length() / 3, image_box, tilt);
		_table->update(_offsets);

		log_debug("Found", _offsets.size(), "feature points.");
	}
}
