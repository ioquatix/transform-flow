//
//  VideoStreamRenderer.cpp
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 9/02/12.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#include "VideoStreamRenderer.h"

#include "MarkerRenderer.h"

#include <TransformFlow/OpticalFlowMotionModel.h>

#include <Euclid/Geometry/Plane.h>
#include <Euclid/Numerics/Matrix.IO.h>

namespace TransformFlow
{
	namespace Renderer
	{
		VideoStreamRenderer::FrameCache::FrameCache() : selected_feature_index((std::size_t)-1)
		{
			marker_particles = new MarkerParticles;
			debug_particles = new MarkerParticles;
			feature_particles = new MarkerParticles;
		}
		
		VideoStreamRenderer::FrameCache::~FrameCache()
		{
		}
		
		Plane<3> VideoStreamRenderer::FrameCache::frame_plane() const
		{
			Vec3 center = global_transform * (image_box.center() << 0.0);
			Vec4 normal = global_transform * Vec4(0, 0, -1, 0);
			
			// Not sure why this has to be negative:
			return Plane<3>(-center, normal.reduce());
		}
		
		Vec3 VideoStreamRenderer::FrameCache::global_coordinate_of_pixel_coordinate(Vec2 point)
		{
			// We need to calculate the global positioning of the pixel:
			Vec2 normalized_point = point / image_buffer()->size();
			Vec2 center = image_box.absolute_position_of(normalized_point);
			
			// Calculate the 3d position of the feature by applying the frame's global transform to the image coordinate:
			Vec3 global_coordinate = global_transform * (center << 0.0);
			
			return global_coordinate;
		}
		
		Vec2 VideoStreamRenderer::FrameCache::pixel_coordinate_of_global_coordinate(Vec3 point)
		{
			Vec3 local_coordinate = inverse(global_transform) * point;
			
			Vec2 normalized_point = image_box.relative_offset_of(local_coordinate.reduce());
			Vec2 pixel_coordinate = normalized_point * image_buffer()->size();
			
			return pixel_coordinate;
		}
				
		void VideoStreamRenderer::FrameCache::select(std::size_t index)
		{
			if (selected_feature_index != (std::size_t)-1) {
				marker_particles->particles()[selected_feature_index].color = Vec3(1.0, 1.0, 1.0);
			}
			
			selected_feature_index = index;
			
			if (selected_feature_index != (std::size_t)-1) {
				marker_particles->particles()[selected_feature_index].color = Vec3(0.0, 1.0, 0.0);
			}
		}

		// Deprecated...
		void VideoStreamRenderer::FrameCache::find_vertical_edges()
		{
			if (selected_feature_index != (std::size_t)-1) {
				// We just use 2-space gravity as we are interested in the delta as applied to the captured image. If the camera is pointing up or down, the gravity vector can't be easily applied to the image.
				Vec2 gravity = (global_transform * video_frame.gravity).reduce();
				
				if (gravity.length_squared() < 0.01) {
					return;
				}
				
				gravity.normalize();
				
				Vec2i point = feature_points()->offsets()[selected_feature_index];
				
				for (std::size_t i = 0; i < 10; i += 1) {
					Vec2i offset = point + (gravity * i);

					writer(*image_buffer()).set(offset, Vector<3, ByteT>(255, 0, 0));
				}
			}
		}
		
		VideoStreamRenderer::VideoStreamRenderer(Ptr<RendererState> renderer_state) : _renderer_state(renderer_state)
		{
			_marker_renderer = new MarkerRenderer(renderer_state);
			_billboard_marker_renderer = new MarkerRenderer(renderer_state, true);
			
			_pixel_buffer_renderer = new ImageRenderer(_renderer_state->texture_manager);
			
			// Generate mip-maps and use default filters.
			_pixel_buffer_renderer->texture_parameters().generate_mip_maps = true;
			_pixel_buffer_renderer->texture_parameters().min_filter = 0;
			//_pixel_buffer_renderer->texture_parameters().mag_filter = 0;

			_wireframe_renderer = new WireframeRenderer;
			
			{
				_frame_program = _renderer_state->load_program("Shaders/frame");
				
				_frame_program->set_attribute_location("position", 0);
				_frame_program->set_attribute_location("mapping", 1);
				_frame_program->link();
				
				auto binding = _frame_program->binding();
				binding.set_texture_unit("diffuse_texture", 0);			
			}
			
			{
				_wireframe_renderer = new WireframeRenderer;
				
				_wireframe_program = _renderer_state->load_program("Shaders/wireframe");
				_wireframe_program->set_attribute_location("position", WireframeRenderer::POSITION);
				_wireframe_program->link();
				
				auto binding = _wireframe_program->binding();
				binding.set_uniform("major_color", Vec4(1.0, 1.0, 1.0, 1.0));
			}
			
			_start = 0;
			_count = 1;
			
			_frame_index = 0;
			_feature_index = 1;

			_scale = 25.0;

			_axis_renderer = new AxisRenderer(renderer_state);
		}
		
		VideoStreamRenderer::~VideoStreamRenderer()
		{
		}

		void VideoStreamRenderer::update_cache(Ptr<VideoStream> video_stream)
		{
			if (_frame_cache.size() != 0) return;
			
			// Faux rotation
			//Mat44 rotation(IDENTITY);
			Shared<FrameCache> previous;
			
			for (auto & frame : video_stream->images())
			{
				// Calculate the image box:
				Vec2 box_size = Vec2(frame.image_update->image_buffer->size()) / _scale;
				AlignedBox2 image_box = AlignedBox2::from_center_and_size(ZERO, box_size);

				// Transform the video from camera space to device space:
				auto distance = frame.image_update->distance_from_origin(box_size[WIDTH]);
				Mat44 renderer_transform = translate(Vec3{0, 0, -distance});

				// Prepare the frame cache:
				Shared<FrameCache> cache = new FrameCache;
				cache->image_box = image_box;
				cache->video_frame = frame;

				// We need to rotate the image such that the gravity vector is pointing directly down.
				{
					// Global down vector:
					Vec3 down(0, -1, 0);

					cache->global_transform = IDENTITY;

					auto angle = down.angle_between(frame.gravity);
					if (!number(angle.value).equivalent(0))
					{
						auto s = cross_product(frame.gravity, down).normalize();

						Mat44 gravity_rotation = rotate(angle, s);

						cache->global_transform = gravity_rotation;
					}
				}

				// The device transform should shift the image frames into the same frame of reference as the sensor data.
				// The iPhone camera is rotated -90_deg around the Z axis:
				Mat44 device_transform = rotate<Z>(-90_deg);

				cache->global_transform = rotate<Y>(-frame.bearing) << cache->global_transform << renderer_transform << device_transform;

				// Calculate the local transform, if any:
				if (previous) {
					cache->local_transform = inverse(inverse(cache->global_transform) * previous->global_transform);
				} else {
					cache->local_transform = cache->global_transform;
				}
				
				_frame_cache.push_back(cache);
				previous = cache;

				// There is a slight problem, since this visualisation doesn't do spherical projections, the actual FOV will be a bit inaccurate at the edges. Perhaps updating the renderer to do spherical projections could be a good idea?
				if (0) {
					// Check frame FOV:
					auto a = cache->global_transform * (image_box.corner({false, false}) << 0.0);
					auto b = cache->global_transform * (image_box.corner({true, false}) << 0.0);
					auto c = cache->global_transform * (image_box.corner({true, true}) << 0.0);

					log_debug("fov:", R2D * a.angle_between(b), R2D * b.angle_between(c));
				}
				
				// Setup particles
				for (Vec2 offset : cache->feature_points()->offsets()) {
					Vec2 center = (offset / _scale) + cache->image_box.min();
					
					cache->marker_particles->add(center << 0, Vec3(0.5, 0.5, 0), Vec3(0, 0, 1), Vec2u(1, 1));
				}

				for (Vec2 offset : find_key_points(cache->image_buffer())) {
					Vec2 center = (offset / _scale) + cache->image_box.min();

					cache->feature_particles->add(center << 0, Vec3(0.5, 0.5, 0), Vec3(0, 0, 1), Vec2u(1, 1), Vec3(0.5, 0.5, 0.2));
				}
			}
		}
		
		void VideoStreamRenderer::render_frame_for_time(TimeT time, Ptr<VideoStream> video_stream)
		{
			{
				std::size_t start = _start, count = _count, offset = 0;
				
				glEnable(GL_DEPTH_TEST);

				auto binding = _frame_program->binding();
				binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix());
							
				// Update the frame cache if required.
				update_cache(video_stream);
								
				for (auto & frame : _frame_cache) {
					binding.set_uniform("transform_matrix", frame->global_transform);
					
					if (offset >= start)
						_pixel_buffer_renderer->render(frame->image_box, frame->image_buffer());
									
					if (offset >= start && --count == 0)
						break;
					
					offset += 1;
				}

				glDisable(GL_DEPTH_TEST);
			}
			
			{
				std::size_t start = _start, count = _count, offset = 0;

				glDisable(GL_CULL_FACE);
				glDisable(GL_DEPTH_TEST);
				
				for (auto & frame : _frame_cache) {
					frame->marker_particles->update(time);
					frame->debug_particles->update(time);
					frame->feature_particles->update(time);

					if (offset == _start) {
						_marker_renderer->render(frame->marker_particles, frame->global_transform);
						_billboard_marker_renderer->render(frame->debug_particles, IDENTITY);
						_marker_renderer->render(frame->feature_particles, frame->global_transform);
					}
					
					if (offset >= start && --count == 0)
						break;
					
					offset += 1;
				}

				glEnable(GL_DEPTH_TEST);

				_axis_renderer->render(IDENTITY);

				glEnable(GL_CULL_FACE);
			}

			{
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				std::size_t start = _start, count = _count, offset = 0;
				auto binding = _wireframe_program->binding();

				glDepthMask(GL_FALSE);
				
				for (auto & frame : _frame_cache) {
					binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix() * frame->global_transform);
					binding.set_uniform("major_color", Vec4(0.4, 0.4, 0.4, 0.4));

					_wireframe_renderer->render(frame->image_box);

					Ref<FeaturePoints> feature_points = frame->feature_points();

					if (offset == _start) {
						Ref<FeatureTable> table = feature_points->table();
						binding.set_uniform("major_color", Vec4(1.0, 0.4, 0.4, 0.89));

						for (auto chain : table->chains()) {
							std::vector<Vec3> points;

							while (chain != nullptr) {
								Vec2 normalized_point = chain->offset / frame->image_buffer()->size();
								Vec2 center = frame->image_box.absolute_position_of(normalized_point);

								points.push_back(center << 0.01);

								chain = chain->next;
							}

							_wireframe_renderer->render(points, LINE_STRIP);
						}
						
						binding.set_uniform("major_color", Vec4(0.1, 0.2, 1.0, 0.75));

						// *** Render horizontal scan lines
						for (auto & segment : feature_points->segments()) {
							std::vector<Vec3> points;

							{
								Vec2 normalized_point = segment.start() / frame->image_buffer()->size();
								Vec2 center = frame->image_box.absolute_position_of(normalized_point);

								points.push_back(center << 0.01);
							}

							{
								Vec2 normalized_point = segment.end() / frame->image_buffer()->size();
								Vec2 center = frame->image_box.absolute_position_of(normalized_point);

								points.push_back(center << 0.01);
							}

							_wireframe_renderer->render(points, LINE_STRIP);
						}
					}

					offset += 1;
				}
				
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
			}
		}

		bool VideoStreamRenderer::select_feature_point(Vec2 screen_coordinate) {
			auto object_coordinate = _renderer_state->viewport->convert_to_object_space(screen_coordinate);
			
			Ptr<FeaturePoints> closest_feature_points = nullptr;
			std::size_t closest_offset_index = 0, frame_offset_index;
			RealT closest_distance = 0;
			
			log_debug("Searching for features at", screen_coordinate);
			
			std::size_t start = _start, count = _count, offset = 0;
			for (auto & frame : _frame_cache) {
				Plane3 frame_plane = frame->frame_plane();

				Vec3 at;
				if (offset >= start && frame_plane.intersects_with(object_coordinate.forward, at)) {
					//frame->debug_particles->add(at, Vec3(0.5, 0.5, 0.0), Vec3(0.0, 0.0, -1.0), Vec2u(0, 0), Vec3(0.2, 0.8, 0.3));
					
					std::size_t index = 0;
					for (auto point : frame->feature_points()->offsets()) {
						Vec2 center = Vec2(point) / _scale;
						center += frame->image_box.min();
						Vec3 feature_center = frame->global_transform * (center << 0.0);
						
						RealT distance = (feature_center - at).length();
						if (!closest_feature_points || closest_distance > distance) {
							frame_offset_index = offset;
							closest_feature_points = frame->feature_points();
							closest_offset_index = index;
							closest_distance = distance;
						}
						
						index += 1;
					}	
				}
				
				if (offset >= start && --count == 0)
					break;
				
				offset += 1;
			}
			
			if (closest_feature_points) {
				log_debug("Found feature point at", closest_feature_points->offsets()[closest_offset_index]);
				
				auto & cache = _frame_cache[frame_offset_index];
				
				cache->select(closest_offset_index);
				
				// We keep track of this for visualisation purposes:
				_frame_index = frame_offset_index;
				_feature_index = closest_offset_index;
				
				return true;
			}
			
			return false;
		}
		
		void VideoStreamRenderer::find_vertical_edges() {
			Shared<FrameCache> frame = _frame_cache[_frame_index];
			
			frame->find_vertical_edges();
			
			_pixel_buffer_renderer->invalidate(frame->image_buffer());
		}

		bool VideoStreamRenderer::apply_feature_algorithm() {
			Shared<FrameCache> frame = _frame_cache[_frame_index];

			if (_frame_index+1 < _frame_cache.size() && frame->selected_feature_index != (std::size_t)-1) {
				Shared<FrameCache> next = _frame_cache[_frame_index + 1];
				auto image_box = frame->image_box;

				Ref<MatchingAlgorithm> matching_algorithm = matchingAlgorithmUsingORB();

				Mat44 homography = matching_algorithm->calculate_local_transform(*frame->video_frame.image_update, *next->video_frame.image_update);
				
				log_debug("Homography: ", homography);
				
				next->local_transform = homography;
				next->global_transform = homography * frame->global_transform;
				next->global_transform = frame->global_transform * homography;

				std::clock_t start = std::clock();
				Vec2 translation = matching_algorithm->calculate_local_translation(*frame->video_frame.image_update, *next->video_frame.image_update);
				Vec2 offset = -translation / image_box.size();
				std::clock_t end = std::clock();
				double duration = double(end - start) / CLOCKS_PER_SEC;

				log_debug("Offset (Optical Flow)", offset, "Time =", duration);
				
				//next->feature_transform = translate(offset);

				return true;
			} else {
				return false;
			}
		}
	}
}
