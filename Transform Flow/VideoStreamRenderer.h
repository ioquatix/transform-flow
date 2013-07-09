//
//  VideoStreamRenderer.h
//  Transform Flow
//
//  Created by Samuel Williams on 9/02/12.
//  Copyright (c) 2012 Orion Transfer Ltd. All rights reserved.
//

#ifndef _TRANSFORM_FLOW_VIDEO_STREAM_RENDERER_H
#define _TRANSFORM_FLOW_VIDEO_STREAM_RENDERER_H

#include "VideoStream.h"

#include <Dream/Renderer/Projection.h>
#include <Dream/Renderer/Viewport.h>

#include <Dream/Client/Graphics/ShaderManager.h>
#include <Dream/Client/Graphics/TextureManager.h>
#include <Dream/Client/Graphics/ImageRenderer.h>
#include <Dream/Client/Graphics/WireframeRenderer.h>
#include <Dream/Client/Graphics/Renderer.h>

#include <Dream/Client/Graphics/ParticleRenderer.h>

#include "FeatureAlgorithm.h"
#include "AxisRenderer.h"

namespace TransformFlow {
	using namespace Dream::Renderer;
	using namespace Dream::Client::Graphics;
	
	enum AlignmentMode {
		NO_ALIGNMENT = 0,
		ORIENTATION_ALIGNMENT = 1,
		FEATURE_ALIGNMENT = 2,
		ALIGNMENT_MAX = 3
	};
	
	// The actual particle simulation/drawing code:
	class MarkerParticles : public ParticleRenderer<MarkerParticles> {
	public:
		bool update_particle(Particle & particle, TimeT last_time, TimeT current_time, TimeT dt);
		void add(Vec3 center, Vec3 up, Vec3 forward, Vec2u mapping, Vec3 color = IDENTITY);
	};
	
	// The high level renderer that manages associated textures/shaders:
	class MarkerRenderer : public Object {
	protected:
		Ref<Program> _particle_program;
		Ref<Texture> _marker_texture;
		
		Ref<RendererState> _renderer_state;
		
	public:
		MarkerRenderer(Ptr<RendererState> renderer_state, bool billboard = false);
		virtual ~MarkerRenderer();
		
		void render(Ptr<MarkerParticles> markers, Mat44 global_transform);
	};
	
	class VideoStreamRenderer : public Object {
	protected:
		Ref<RendererState> _renderer_state;

		Ref<MarkerRenderer> _marker_renderer, _billboard_marker_renderer;
		Ref<WireframeRenderer> _wireframe_renderer;
		Ref<ImageRenderer> _pixel_buffer_renderer;
		Ref<Program> _frame_program, _wireframe_program;

		Ref<AxisRenderer> _axis_renderer;

		AlignmentMode _alignment_mode;
		RealT _scale;
		
		Vec3 _selection_marker;
		
		std::size_t _start, _count;
		
		std::size_t _frame_index, _feature_index;
		
		struct FrameCache {
			FrameCache();
			virtual ~FrameCache();
			
			VideoStream::VideoFrame video_frame;

			inline Ref<Image> image_buffer() { return video_frame.image_update->image_buffer; }
			inline Ref<FeaturePoints> feature_points() { return video_frame.feature_points; }
			
			// A transform to align based on sensor data:
			Mat44 local_transform;
			
			// A tranform to align features:
			Mat44 feature_transform;
			
			// A cache of the global transform, calculated per frame:
			Mat44 global_transform;
			
			AlignedBox2 image_box;
			
			std::size_t selected_feature_index;
			Vec2 corresponding_feature_coordinate;
			
			Plane<3> frame_plane() const;
			
			Ref<MarkerParticles> marker_particles;
			Ref<MarkerParticles> feature_particles;
			Ref<MarkerParticles> debug_particles;
			
			Vec2 pixel_coordinate_of_global_coordinate(Vec3 point);
			Vec3 global_coordinate_of_pixel_coordinate(Vec2 point);
			
			void calculate_feature_transform(Shared<FrameCache> next);
			void select(std::size_t index);
			
			void find_vertical_edges();
		};
		
		std::vector<Shared<FrameCache>> _frame_cache;
		
		void update_cache(Ptr<VideoStream> video_stream);

	public:
		VideoStreamRenderer(Ptr<RendererState> renderer_state);
		virtual ~VideoStreamRenderer();
		
		void set_alignment_mode(AlignmentMode alignment_mode) { _alignment_mode = alignment_mode; }
		
		void render_frame_for_time(TimeT time, Ptr<VideoStream> video_stream);
		
		void set_range(Vec2u range) {
			_start = range[0];
			_count = range[1];
		}
		
		Vec2u range() const {
			return {_start, _count};
		}
		
		bool select_feature_point(Vec2 screen_coordinate);
		Vec2u selected_feature_point() const {
			return {_frame_index, _feature_index};
		}

		bool update_feature_transform();
		bool apply_feature_algorithm();
		
		void find_vertical_edges();
	};
}

#endif
