//
//  FeaturePoints.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 1/03/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "FeaturePoints.h"
#include <Dream/Events/Logger.h>

namespace TransformFlow {
	
	using namespace Dream::Events::Logging;

	// Bresenham's Line Drawing Algorithm
	void FeaturePoints::features_along_line(Ptr<Image> image, Vec2i start, Vec2i end, std::vector<Vec2> & features) {
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

		for (int x = start[X]; x <= end[X]; x += 1) {
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
				
				if (distance > 10000) {
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
		}
	}
	
	FeaturePoints::FeaturePoints() {
		
	}
	
	FeaturePoints::~FeaturePoints() {
		
	}
	
	void FeaturePoints::scan(Ptr<Image> source) {
		if (_offsets.size()) return;
		
		_source = source;
		
		Vec3u size = source->size();
		
		std::size_t dy = std::max<std::size_t>(size[Y] / 40, 2);
		std::size_t dx = std::max<std::size_t>(size[X] / 40, 2);

		for (std::size_t y = dy; y < size[Y]; y += dy) {
			features_along_line(source, Vec2i(dx, y), Vec2i(size[X] - dx, y), _offsets);
		}
		
		//for (std::size_t x = dx; x < size[X]; x += dx) {
		//	features_along_line(source_image, Vec2i(x, dy), Vec2i(x, size[Y] - dy), _offsets);
		//}

		_table = new FeatureTable(size.length() / 10, _source->size());
		_table->update(this);

		logger()->log(LOG_INFO, LogBuffer() << "Found " << _offsets.size() << " feature points");
	}
}
