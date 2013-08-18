//
//  AxisRenderer.cpp
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 10/7/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#include "AxisRenderer.h"

#include <Euclid/Numerics/Vector.IO.h>

namespace TransformFlow
{
	AxisRenderer::AxisRenderer(Ptr<RendererState> renderer_state) : _renderer_state(renderer_state)
	{
		// Load marker texture:
		_texture = _renderer_state->load_texture(TextureParameters::FILTERED, "Textures/arrow");
		
		// Load particle program:
		_program = _renderer_state->load_program("Shaders/axis");
		
		_program->set_attribute_location("position", POSITION);
		_program->set_attribute_location("offset", OFFSET);
		_program->set_attribute_location("axis", AXIS);
		_program->set_attribute_location("mapping", MAPPING);
		_program->set_attribute_location("color", COLOR);
		_program->link();
		
		{
			auto binding = _program->binding();
			binding.set_texture_unit("diffuse_texture", 0);
		}

		setup_axes();
	}

	AxisRenderer::~AxisRenderer()
	{
	}	

	void AxisRenderer::setup_axes()
	{
		// Setup vertex array:
		{
			auto binding = _vertex_array.binding();

			// Attach vertices buffer:
			auto attributes = binding.attach(_vertex_buffer);
			attributes[POSITION] = &Vertex::position;
			attributes[OFFSET] = &Vertex::offset;
			attributes[AXIS] = &Vertex::axis;
			attributes[MAPPING] = &Vertex::mapping;
			attributes[COLOR] = &Vertex::color;

			// Attach indices buffer:
			binding.attach(_indices_buffer);
		}

		// Generate vertices:
		{
			auto binding = _vertex_buffer.binding();

			// Resize to contain 3 quads of 4 vertices each.
			binding.resize(4 * 3);

			auto buffer = binding.array();

			Vec2 mappings[4] = {
				{0, 0}, {0, 1}, {1, 1}, {1, 0}
			};

			Vec3 axes[3] = {
				{1, 0, 0}, {0, 1, 0}, {0, 0, 1}
			};

			const RealT length = 5.0;
			const RealT width = 1.0;

			// Generate the 3 quads:
			for (std::size_t i = 0; i < 3; i += 1) {
				Vertex vertices[4];

				// Calculate absolute offsets:
				vertices[0].offset = {-width / 2.0, length / 2.0};
				vertices[1].offset = {-width / 2.0, -length / 2.0};
				vertices[2].offset = {width / 2.0, -length / 2.0};
				vertices[3].offset = {width / 2.0, length / 2.0};

				for (std::size_t v = 0; v < 4; v += 1) {
					// The center is at the middle of the axis marker:
					vertices[v].position = axes[i] * (length / 2.0);
					vertices[v].mapping = mappings[v];

					vertices[v].axis = axes[i];
					vertices[v].color = axes[i] << 1.0;
				}

				std::copy_n(vertices, 4, &buffer[i*4]);
			}

			binding.unmap();
		}

		// Generate indices:
		{
			std::vector<GLushort> indices;

			// Setup indices for drawing quadrilaterals as triangles:
			std::size_t additions = setup_triangle_indicies(3, indices);

			if (additions) {
				auto binding = _indices_buffer.binding();
				binding.set_data(indices);
			}
		}
	}

	void AxisRenderer::render(Mat44 global_transform)
	{
		glDepthMask(GL_FALSE);

		glEnable(GL_BLEND);

		// Additive blending with source alpha:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		
		{
			auto binding = _program->binding();
			binding.set_uniform("display_transform", _renderer_state->viewport->display_matrix() * global_transform);
			
			_renderer_state->texture_manager->bind(0, _texture);

			// Draw vertex array
			{
				auto binding = _vertex_array.binding();
				binding.draw_elements(GL_TRIANGLES, (GLsizei)(3 * 6), GLTypeTraits<GLushort>::TYPE);
			}
		}
		
		glDisable(GL_BLEND);
		glDepthMask(GL_TRUE);
	}
}
