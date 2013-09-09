//
//  VideoStreamRenderer.h
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 9/02/12.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#ifndef TRANSFORMFLOW_RENDERER_VIDEOSTREAMRENDERER_H
#define TRANSFORMFLOW_RENDERER_VIDEOSTREAMRENDERER_H

#include <TransformFlow/VideoStream.h>

#include <Dream/Renderer/Projection.h>
#include <Dream/Renderer/Viewport.h>

#include <Dream/Graphics/ShaderManager.h>
#include <Dream/Graphics/TextureManager.h>
#include <Dream/Graphics/ImageRenderer.h>
#include <Dream/Graphics/WireframeRenderer.h>
#include <Dream/Graphics/Renderer.h>

#include <TransformFlow/FeatureAlgorithm.h>
#include "AxisRenderer.h"

namespace TransformFlow
{
	namespace Renderer
	{
		using namespace Dream::Renderer;
		using namespace Dream::Graphics;

		class MarkerRenderer;
		class MarkerParticles;
			
		class VideoStreamRenderer : public Object {
		protected:
			Ref<RendererState> _renderer_state;

			Ref<MarkerRenderer> _marker_renderer, _billboard_marker_renderer;
			Ref<WireframeRenderer> _wireframe_renderer;
			Ref<ImageRenderer> _pixel_buffer_renderer;
			Ref<Program> _frame_program, _wireframe_program;

			Ref<AxisRenderer> _axis_renderer;

			RealT _scale;
						
			std::size_t _start, _count;
			
			std::size_t _frame_index, _feature_index;
			
			struct FrameCache
			{
				FrameCache();
				virtual ~FrameCache();
				
				VideoStream::VideoFrame video_frame;

				inline Ref<Image> image_buffer() { return video_frame.image_update->image_buffer; }
				inline Ref<FeaturePoints> feature_points() { return video_frame.feature_points; }
				
				// A transform to align based on sensor data:
				Mat44 local_transform;
				
				// A cache of the global transform, calculated per frame:
				Mat44 global_transform;
				
				AlignedBox2 image_box;
				
				std::size_t selected_feature_index;
				
				Plane<3> frame_plane() const;
				
				Ref<MarkerParticles> marker_particles;
				Ref<MarkerParticles> feature_particles;
				Ref<MarkerParticles> debug_particles;
				
				Vec2 pixel_coordinate_of_global_coordinate(Vec3 point);
				Vec3 global_coordinate_of_pixel_coordinate(Vec2 point);
				
				void calculate_feature_transform(Shared<FrameCache> previous);
				void select(std::size_t index);
				
				void find_vertical_edges();
			};
			
			std::vector<Shared<FrameCache>> _frame_cache;
			
			void update_cache(Ptr<VideoStream> video_stream);

		public:
			VideoStreamRenderer(Ptr<RendererState> renderer_state);
			virtual ~VideoStreamRenderer();

			void render_frame_for_time(TimeT time, Ptr<VideoStream> video_stream);

			const std::vector<Shared<FrameCache>> & frame_cache() { return _frame_cache; }

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

			bool apply_feature_algorithm();
			void find_vertical_edges();
		};
	}
}

#endif
