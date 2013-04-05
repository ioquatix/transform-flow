//
//  VideoStreamRenderer.cpp
//  Transform Flow
//
//  Created by Samuel Williams on 9/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#include "VideoStreamRenderer.h"

#include <Dream/Geometry/Plane.h>
#include <Dream/Numerics/Interpolate.h>

#include "SurfaceFeatures.h"

namespace TransformFlow {
	
	bool MarkerParticles::update_particle(Particle & particle, TimeT last_time, TimeT current_time, TimeT dt) {
		particle.update_time(dt);
		
		//RealT alpha = physics.calculate_alpha(0.7);
		particle.update_vertex_color(particle.color << 1.0);
		
		// Markers never die:
		return true;
	}
	
	void MarkerParticles::add(Vec3 center, Vec3 up, Vec3 forward, Vec2u mapping, Vec3 color) {
		Particle particle;
				
		particle.set_position(center, up, forward, 0.0);
		particle.color = color;
		particle.set_mapping(Vec2u(2, 2), mapping);
		
		_particles.push_back(particle);
	}
	
	MarkerRenderer::MarkerRenderer(Ptr<RendererState> renderer_state, bool billboard) : _renderer_state(renderer_state) {
		// Load marker texture:
		_marker_texture = _renderer_state->load_texture(TextureParameters::FILTERED, "Textures/Markers");
		
		// Load particle program:
		if (billboard) {
			_particle_program = _renderer_state->load_program("Shaders/billboard-particle");
		} else {
			_particle_program = _renderer_state->load_program("Shaders/particle");
		}
		
		_particle_program->set_attribute_location("position", MarkerParticles::POSITION);
		_particle_program->set_attribute_location("offset", MarkerParticles::OFFSET);
		_particle_program->set_attribute_location("mapping", MarkerParticles::MAPPING);
		_particle_program->set_attribute_location("color", MarkerParticles::COLOR);
		_particle_program->link();
		
		{
			auto binding = _particle_program->binding();
			binding.set_texture_unit("diffuse_texture", 0);
		}
	}
	
	MarkerRenderer::~MarkerRenderer() {
		
	}
	
	void MarkerRenderer::render(Ptr<MarkerParticles> markers, Mat44 global_transform) {
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		{
			auto binding = _particle_program->binding();
			binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix() * global_transform);
			
			_renderer_state->texture_manager->bind(0, _marker_texture);
			markers->draw();
		}
		
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);		
	}
	
	VideoStreamRenderer::FrameCache::FrameCache() : selected_feature_index((std::size_t)-1), corresponding_feature_coordinate(0, 0) {
		marker_particles = new MarkerParticles;
		debug_particles = new MarkerParticles;
		feature_particles = new MarkerParticles;
	}
	
	VideoStreamRenderer::FrameCache::~FrameCache() {
		
	}
	
	Plane<3> VideoStreamRenderer::FrameCache::frame_plane() const {
		Vec3 center = global_transform * (image_box.center() << 0.0);
		Vec4 normal = global_transform * Vec4(0, 0, -1, 0);
		
		// Not sure why this has to be negative:
		return Plane<3>(-center, normal.reduce());
	}
	
	Vec3 VideoStreamRenderer::FrameCache::global_coordinate_of_pixel_coordinate(Vec2 point) {
		// We need to calculate the global positioning of the pixel:
		Vec2 normalized_point = point / image_update.image_buffer->size().reduce();
		Vec2 center = image_box.absolute_position_of(normalized_point);
		
		// Calculate the 3d position of the feature by applying the frame's global transform to the image coordinate:
		Vec3 global_coordinate = global_transform * (center << 0.0);
		
		return global_coordinate;
	}
	
	Vec2 VideoStreamRenderer::FrameCache::pixel_coordinate_of_global_coordinate(Vec3 point) {
		Vec3 local_coordinate = global_transform.inverse_matrix() * point;
		
		Vec2 normalized_point = image_box.relative_offset_of(local_coordinate.reduce());
		Vec2 pixel_coordinate = normalized_point * image_update.image_buffer->size().reduce();
		
		return pixel_coordinate;
	}
	
	void VideoStreamRenderer::FrameCache::calculate_feature_transform(Shared<FrameCache> next) {
		// We want the local transform from the current frame to the next frame..
		Vec2 point = image_update.feature_points->offsets()[selected_feature_index];
		
		// Hard coded horizontal scanning - grab left and right pixel from source image:
		Vector<3, ByteT> source_pixels[2], next_pixels[2];
		
		image_update.image_buffer->read_pixel(point << 0, source_pixels[0]);
		image_update.image_buffer->read_pixel(point + Vec2(1, 0) << 0, source_pixels[1]);

		// Calculate the 3d position of the feature by applying the frame's global transform to the image coordinate:
		Vec3 global_coordinate = global_coordinate_of_pixel_coordinate(point);
		
		logger()->log(LOG_DEBUG, LogBuffer() << "Image box: " << image_box << " Offset: " << point << " Center: " << center);
		logger()->log(LOG_DEBUG, LogBuffer() << "Global Coordinate: " << global_coordinate);
		
		debug_particles->add(global_coordinate, Vec3(0.1, 0.1, 0.0), Vec3(0, 0, 1.0), Vec2u(0, 0), Vec3(0, 0, 1));
		
		// Calculate the 3d position of the feature in the next frame by appling the local transform:
		Vec3 forwards_transformed_coordinate = next->local_transform * global_coordinate;
		
		debug_particles->add(forwards_transformed_coordinate, Vec3(0.1, 0.1, 0.0), Vec3(0, 0, 1.0), Vec2u(0, 0), Vec3(0, 1, 0));
		
		// Calculate the pixel position of the feature in the next frame by applying the inverse global transform for the next frame:
		Vec2 next_pixel_coordinate = next->pixel_coordinate_of_global_coordinate(global_coordinate);
		
		// If camera moves to left, vertical edges move to the right:
		Vec2 delta = point - next_pixel_coordinate;
		
		// -5.62024 5.09367
		// -44.62024 5.09367
		
		RealT minimum = 255.0 * 2.0;
		std::size_t offset = 0, minimum_offset = 0;
		
		for (; offset < 60; offset += 1) {
			Vec2 scan_point = next_pixel_coordinate + Vec2(offset, 0);
			scan_point[Y] = next->image_update.image_buffer->size()[HEIGHT] - scan_point[Y];
			
			next_pixels[0] = next_pixels[1];
			next->image_update.image_buffer->read_pixel(scan_point << 0, next_pixels[1]);
			
			if (offset == 0)
				continue;
			
			Vec3 difference[2] = {
				Vec3(source_pixels[0]) - Vec3(next_pixels[0]),
				Vec3(source_pixels[1]) - Vec3(next_pixels[1])
			};
						
			RealT total = difference[0].length() + difference[1].length();
			
			if (total < minimum) {
				minimum = total;
				minimum_offset = offset;
			}
			
			logger()->log(LOG_INFO, LogBuffer() << "Index: " << offset << " difference: " << total << " minimum: " << minimum << " @ " << minimum_offset);
			
			Ptr<Image> image = next->image_update.image_buffer;
			image->write_pixel(scan_point << 0, Vector<3, ByteT>(IDENTITY, Math::max(255.0 - total, 0.0)));
		}
		
		//Vec2 corrected_delta(-50, 5.09367);
		Vec2 corrected_delta(delta[X] - minimum_offset, delta[Y]);
		Vec2 difference = corrected_delta - delta;
		
		logger()->log(LOG_INFO, LogBuffer() << "Delta: " << delta << " corrected: " << corrected_delta << " difference: " << difference);
		
		{
			Vec3 global_offset = next->global_coordinate_of_pixel_coordinate(next_pixel_coordinate + difference);
			
			debug_particles->add(global_offset, Vec3(0.1, 0.1, 0.0), Vec3(0, 0, 1.0), Vec2u(0, 0), Vec3(1, 0, 0));
			
			//RealT angle = global_coordinate.angle_between(global_offset);
			//next->feature_transform = Mat44::rotating_matrix(angle, Vec3(0, 1, 0));
			Vec2 offset = difference / image_box.size();
			logger()->log(LOG_DEBUG, LogBuffer() << "Offset (Transform Flow): " << offset);
			next->feature_transform = Mat44::translating_matrix(offset);
		}
	}
	
	void VideoStreamRenderer::FrameCache::select(std::size_t index) {
		if (selected_feature_index != (std::size_t)-1) {
			marker_particles->particles()[selected_feature_index].color = Vec3(1.0, 1.0, 1.0);
		}
		
		selected_feature_index = index;
		
		if (selected_feature_index != (std::size_t)-1) {
			marker_particles->particles()[selected_feature_index].color = Vec3(0.0, 1.0, 0.0);
		}
	}
	
	void VideoStreamRenderer::FrameCache::find_vertical_edges() {
		if (selected_feature_index != (std::size_t)-1) {
			// We just use 2-space gravity as we are interested in the delta as applied to the captured image. If the camera is pointing up or down, the gravity vector can't be easily applied to the image.
			Vec2 gravity = (global_transform * image_update.gravity).reduce();
			
			if (gravity.length2() < 0.01) {
				return;
			}
			
			gravity.normalize();
			
			Vec2i point = image_update.feature_points->offsets()[selected_feature_index];
			
			for (std::size_t i = 0; i < 10; i += 1) {
				Vec2i offset = point + (gravity * i);
				
				Ptr<Image> image = image_update.image_buffer;
				image->write_pixel(offset << 0, Vector<3, ByteT>(255, 0, 0));
			}
		}
	}
	
	VideoStreamRenderer::VideoStreamRenderer(Ptr<RendererState> renderer_state) : _renderer_state(renderer_state) {
		_marker_renderer = new MarkerRenderer(renderer_state);
		_billboard_marker_renderer = new MarkerRenderer(renderer_state, true);
		
		_pixel_buffer_renderer = new PixelBufferRenderer(_renderer_state->texture_manager);
		
		// Generate mip-maps and use default filters.
		_pixel_buffer_renderer->texture_parameters().generate_mip_maps = true;
		_pixel_buffer_renderer->texture_parameters().min_filter = 0;
		//_pixel_buffer_renderer->texture_parameters().mag_filter = 0;
		
		_alignment_mode = FEATURE_ALIGNMENT;
		
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
		
		_scale = 20.0;
	}
	
	VideoStreamRenderer::~VideoStreamRenderer() {
		
	}
	
	void VideoStreamRenderer::update_cache(Ptr<VideoStream> video_stream) {
		if (_frame_cache.size() != 0) return;
		
		Vec3 down(0, 0, 1);
		
		Mat44 transform = IDENTITY;
		// Images from the phone are not oriented according to the phones natural axes:
		transform = transform * Mat44::translating_matrix(Vec3(0, 0, 25));
		transform = transform * Mat44::rotating_matrix_around_z(R90);
		transform = transform * Mat44::rotating_matrix_around_x(R180);
		
		// Faux rotation
		Mat44 rotation(IDENTITY);
		Shared<FrameCache> previous;
		
		for (auto & frame : video_stream->images()) {
			if (frame.gravity.is_zero())
				continue;
			
			Mat44 global_transform = rotation * transform;
			
			RealT angle = down.angle_between(frame.gravity);
			if (angle > 0.001 && _alignment_mode > 0) {
				Vec3 s = down.cross(frame.gravity);
				
				Mat44 gravity_rotation = Mat44::rotating_matrix(angle, s);
				global_transform = gravity_rotation * global_transform;
			}
			
			Vec2 box_size = Vec2(frame.image_buffer->size().reduce()) / _scale;
			AlignedBox2 image_box = AlignedBox2::from_center_and_size(ZERO, box_size);

			Shared<FrameCache> cache = new FrameCache;
			cache->image_update = frame;
			cache->global_transform = global_transform;
			
			if (previous) {
				cache->local_transform = cache->global_transform * previous->global_transform.inverse_matrix();
			} else {
				cache->local_transform = cache->global_transform;
			}
			
			cache->image_box = image_box;
			
			// We calculate this on demand for the visualisation:
			cache->feature_transform = Mat44(IDENTITY);
			
			_frame_cache.push_back(cache);
			previous = cache;
			
			rotation = rotation * frame.rotation.rotating_matrix();
			
			// Setup particles
			for (Vec2 offset : cache->image_update.feature_points->offsets()) {
				Vec2 center = (offset / _scale) + cache->image_box.min();
				
				cache->marker_particles->add(center << 0, Vec3(0.5, 0.5, 0), Vec3(0, 0, 1), Vec2u(1, 1));
			}

			for (Vec2 offset : find_key_points(cache->image_update.image_buffer)) {
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
			//glEnable(GL_BLEND);
			// Nice for constant alpha
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			
			auto binding = _frame_program->binding();
			binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix());
						
			// Update the frame cache if required.
			update_cache(video_stream);
			
			Mat44 global_transform(IDENTITY);
			
			for (auto & frame : _frame_cache) {
				if (frame->image_update.gravity.is_zero())
					continue;
				
				global_transform = frame->local_transform * global_transform;
				
				if (_alignment_mode >= FEATURE_ALIGNMENT) {
					global_transform = global_transform * frame->feature_transform;
				}
				
				frame->global_transform = global_transform;
				binding.set_uniform("transform_matrix", frame->global_transform);
				
				if (offset >= start)
					_pixel_buffer_renderer->render(frame->image_box, frame->image_update.image_buffer);
								
				if (offset >= start && --count == 0)
					break;
				
				offset += 1;
			}

			//glDisable(GL_BLEND);
			glDisable(GL_DEPTH_TEST);
		}
		
		{
			std::size_t start = _start, count = _count, offset = 0;

			auto binding = _wireframe_program->binding();
			binding.set_uniform("major_color", Vec4(0.5, 0.5, 1.0, 1.0));

			for (auto & frame : _frame_cache) {
				binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix() * frame->global_transform);
				binding.set_uniform("major_color", Vec4(0.5, 0.5, 1.0, 1.0));
				
				//std::size_t index = 0;
				//for (auto point : frame->image_update.feature_points->offsets()) {
					//Vec2 center = Vec2(point) / _scale;
					//center += frame->image_box.min();
					
					//if (index == frame->selected_feature_index) {
					//	binding.set_uniform("major_color", Vec4(0.0, 1.0, 0.0, 1.0));
					//}
					
					//LineSegment3 marker(center << -1, center << 1);
					//_wireframe_renderer->render(marker);
					//_wireframe_renderer->render(frame->image_box);
					
					//_wireframe_renderer->render(LineSegment3(ZERO, Vec3(0, -20, 0)));
					
					//if (index == frame->selected_feature_index) {
					//	binding.set_uniform("major_color", Vec4(0.5, 0.5, 1.0, 1.0));
					//}
					
					//index += 1;
				//}
				
				{
					binding.set_uniform("major_color", Vec4(1.0, 0.95, 0.95, 1.0));
					LineSegment3 marker(frame->corresponding_feature_coordinate << -1, frame->corresponding_feature_coordinate << 1);
					_wireframe_renderer->render(marker);
				}
				
				if (offset >= start && --count == 0)
					break;
				
				offset += 1;
			}
			
			binding.set_uniform("major_color", Vec4(1.0, 0.0, 0.0, 1.0));
			
			binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix());
			_wireframe_renderer->render(AlignedBox3::from_center_and_size(_selection_marker, 2));
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
					//_marker_renderer->render(frame->feature_particles, frame->global_transform);
				}
				
				if (offset >= start && --count == 0)
					break;
				
				offset += 1;
			}
			
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);
		}

		{
			auto binding = _wireframe_program->binding();
			
			glDepthMask(GL_FALSE);
			for (auto & frame : _frame_cache) {
				binding.set_uniform("display_matrix", _renderer_state->viewport->display_matrix() * frame->global_transform);
				binding.set_uniform("major_color", Vec4(0.4, 0.4, 0.4, 0.4));

				_wireframe_renderer->render(frame->image_box);
			}
			glDepthMask(GL_TRUE);

		}
	}

	bool VideoStreamRenderer::select_feature_point(Vec2 screen_coordinate) {
		ViewportEyeSpace object_coordinate = _renderer_state->viewport->convert_to_object_space(screen_coordinate);
		
		Ptr<FeaturePoints> closest_feature_points = nullptr;
		std::size_t closest_offset_index = 0, frame_offset_index;
		RealT closest_distance = 0;
		
		logger()->log(LOG_DEBUG, LogBuffer() << "Searching for features at " << screen_coordinate);
		
		std::size_t start = _start, count = _count, offset = 0;
		for (auto & frame : _frame_cache) {
			Plane3 frame_plane = frame->frame_plane();
			
			_selection_marker = frame_plane.normal() * frame_plane.distance();
			
			Vec3 at;
			if (offset >= start && frame_plane.intersects_with(object_coordinate.forward, at)) {
				frame->debug_particles->add(at, Vec3(0.5, 0.5, 0.0), Vec3(0.0, 0.0, -1.0), Vec2u(0, 0), Vec3(0.2, 0.8, 0.3));
				
				std::size_t index = 0;
				for (auto point : frame->image_update.feature_points->offsets()) {
					Vec2 center = Vec2(point) / _scale;
					center += frame->image_box.min();
					Vec3 feature_center = frame->global_transform * (center << 0.0);
					
					RealT distance = (feature_center - at).length();
					if (!closest_feature_points || closest_distance > distance) {
						frame_offset_index = offset;
						closest_feature_points = frame->image_update.feature_points;
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
			logger()->log(LOG_DEBUG, LogBuffer() << "Found feature point at " << closest_feature_points->offsets()[closest_offset_index]);
			
			auto & cache = _frame_cache[frame_offset_index];
			
			cache->select(closest_offset_index);
			
			// We keep track of this for visualisation purposes:
			_frame_index = frame_offset_index;
			_feature_index = closest_offset_index;
			
			return true;
		}
		
		return false;
	}
	
	bool VideoStreamRenderer::update_feature_transform() {
		Shared<FrameCache> frame = _frame_cache[_frame_index];
		
		if (_frame_index+1 < _frame_cache.size() && frame->selected_feature_index != (std::size_t)-1) {
			Shared<FrameCache> next = _frame_cache[_frame_index + 1];

			std::clock_t start = std::clock();
			frame->calculate_feature_transform(next);
			std::clock_t end = std::clock();
			double duration = double(end - start) / CLOCKS_PER_SEC;
			logger()->log(LOG_DEBUG, LogBuffer() << "Time = " << duration);
			
			_pixel_buffer_renderer->invalidate(next->image_update.image_buffer);
		
			return true;
		} else {
			return false;
		}
	}
	
	void VideoStreamRenderer::find_vertical_edges() {
		Shared<FrameCache> frame = _frame_cache[_frame_index];
		
		frame->find_vertical_edges();
		
		_pixel_buffer_renderer->invalidate(frame->image_update.image_buffer);
	}

	bool VideoStreamRenderer::apply_feature_algorithm() {
		Shared<FrameCache> frame = _frame_cache[_frame_index];

		if (_frame_index+1 < _frame_cache.size() && frame->selected_feature_index != (std::size_t)-1) {
			Shared<FrameCache> next = _frame_cache[_frame_index + 1];
			auto image_box = frame->image_box;

			Ref<MatchingAlgorithm> matching_algorithm = matchingAlgorithmUsingORB();

			//Mat44 homography = matching_algorithm->calculate_local_transform(frame->image_update, next->image_update);
			//logger()->log(LOG_DEBUG, LogBuffer() << "Homography: " << std::endl << homography);
			//next->local_transform = homography;
			//next->global_transform = homography * frame->global_transform;
			//next->global_transform = frame->global_transform * homography;

			std::clock_t start = std::clock();
			Vec2 translation = matching_algorithm->calculate_local_translation(frame->image_update, next->image_update);
			Vec2 offset = -translation / image_box.size();
			std::clock_t end = std::clock();
			double duration = double(end - start) / CLOCKS_PER_SEC;

			logger()->log(LOG_DEBUG, LogBuffer() << "Offset (Optical Flow): " << offset);
			logger()->log(LOG_DEBUG, LogBuffer() << "Time = " << duration);
			next->feature_transform = Mat44::translating_matrix(offset);

			return true;
		} else {
			return false;
		}
	}
}
