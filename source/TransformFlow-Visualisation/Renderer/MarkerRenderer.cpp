//
//  MarkerRenderer.cpp
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 24/7/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#include "MarkerRenderer.h"

namespace TransformFlow
{
	namespace Renderer
	{
		bool MarkerParticles::update_particle(Particle & particle, TimeT last_time, TimeT current_time, TimeT dt)
		{
			particle.update_time(dt);
			
			particle.update_vertex_color(particle.color << 1.0);
			
			// Markers never die:
			return true;
		}
		
		void MarkerParticles::add(Vec3 center, Vec3 up, Vec3 forward, Vec2u mapping, Vec3 color)
		{
			Particle particle;
					
			particle.set_position(center, up, forward, R0);
			particle.color = color;
			particle.set_mapping(Vec2u(2, 2), mapping);
			
			_particles.push_back(particle);
		}
		
		MarkerRenderer::MarkerRenderer(Ptr<RendererState> renderer_state, bool billboard) : _renderer_state(renderer_state)
		{
			// Load marker texture:
			_marker_texture = _renderer_state->load_texture(TextureParameters::FILTERED, "Textures/markers");
			
			// Load particle program:
			if (billboard) {
				ShaderParser::DefinesMapT defines = {{"BILLBOARD", ""}};
				_particle_program = _renderer_state->load_program("Shaders/particle", &defines);
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
		
		MarkerRenderer::~MarkerRenderer()
		{
		}
		
		void MarkerRenderer::render(Ptr<MarkerParticles> markers, Mat44 global_transform)
		{
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
	}
}
