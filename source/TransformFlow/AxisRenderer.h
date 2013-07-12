//
//  AxisRenderer.h
//  File file is part of the "Transform Flow" project and released under the MIT License.
//
//  Created by Samuel Williams on 10/7/2013.
//  Copyright, 2013, by Samuel Williams. All rights reserved.
//

#ifndef TRANSFORMFLOW_AXISRENDERER_H
#define TRANSFORMFLOW_AXISRENDERER_H

#include <Dream/Client/Graphics/Renderer.h>
#include <Dream/Client/Graphics/ParticleRenderer.h>

namespace TransformFlow
{
	using namespace Dream;
	using namespace Euclid::Numerics;
	using namespace Dream::Client::Graphics;

	class AxisRenderer : public Object
	{
	public:
		AxisRenderer(Ptr<RendererState> renderer_state);
		virtual ~AxisRenderer();

		void render(Mat44 global_transform);

		struct Vertex
		{
			Vec3 position;
			Vec2 offset;
			Vec3 axis;
			Vec4 color;
			Vec2 mapping;
		};

		enum {
			POSITION = 0,
			OFFSET = 1,
			AXIS = 2,
			COLOR = 3,
			MAPPING = 4
		};

	private:
		Ref<Program> _program;
		Ref<Texture> _texture;

		VertexArray _vertex_array;
		IndexBuffer<GLushort> _indices_buffer;
		VertexBuffer<Vertex> _vertex_buffer;

		Ref<RendererState> _renderer_state;

		void setup_axes();
	};
}

#endif
