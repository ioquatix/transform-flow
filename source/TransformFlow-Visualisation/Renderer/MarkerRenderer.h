//
//  MarkerRenderer.h
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 24/7/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#ifndef TRANSFORMFLOW_RENDERER_MARKERRENDERER_H
#define TRANSFORMFLOW_RENDERER_MARKERRENDERER_H

#include <Euclid/Numerics/Matrix.h>

#include <Dream/Graphics/Renderer.h>
#include <Dream/Graphics/ParticleRenderer.h>

namespace TransformFlow
{
	namespace Renderer
	{
		using namespace Dream;
		using namespace Dream::Graphics;
		using namespace Euclid::Numerics;

		// The actual particle simulation/drawing code:
		class MarkerParticles : public ParticleRenderer<MarkerParticles>
		{
		public:
			bool update_particle(Particle & particle, TimeT last_time, TimeT current_time, TimeT dt);
			void add(Vec3 center, Vec3 up, Vec3 forward, Vec2u mapping, Vec3 color = IDENTITY);
		};
		
		// The high level renderer that manages associated textures/shaders:
		class MarkerRenderer : public Object
		{
		protected:
			Ref<Program> _particle_program;
			Ref<Texture> _marker_texture;
			
			Ref<RendererState> _renderer_state;
			
		public:
			MarkerRenderer(Ptr<RendererState> renderer_state, bool billboard = false);
			virtual ~MarkerRenderer();
			
			void render(Ptr<MarkerParticles> markers, Mat44 global_transform);
		};
	}
}

#endif
