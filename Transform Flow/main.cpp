//
//  main.cpp
//  Demo
//
//  Created by Samuel Williams on 16/09/11.
//  Copyright 2011 Orion Transfer Ltd. All rights reserved.
//

#include <Dream/Client/Client.h>
#include <Dream/Client/Display/Application.h>

#include <Dream/Client/Graphics/ShaderManager.h>
#include <Dream/Client/Graphics/TextureManager.h>

#include <Dream/Renderer/Viewport.h>
#include <Dream/Renderer/BirdsEyeCamera.h>
#include <Dream/Renderer/PointCamera.h>
#include <Dream/Renderer/Projection.h>

#include <Dream/Events/Logger.h>

#include <Dream/Text/Font.h>
#include <Dream/Text/TextBuffer.h>

#include <Dream/Client/Graphics/MeshBuffer.h>
#include <Dream/Client/Graphics/ImageRenderer.h>
#include <Dream/Client/Graphics/WireframeRenderer.h>

#include <Euclid/Geometry/Generate/Planar.h>

#include "VideoStream.h"
#include "VideoStreamRenderer.h"

namespace TransformFlow {
	using namespace Dream;
	using namespace Dream::Events;
	using namespace Dream::Resources;
	using namespace Dream::Renderer;
	using namespace Dream::Client::Display;
	using namespace Dream::Client::Graphics;
	using namespace Euclid;
		
	class ImageSequenceScene : public Scene
	{
	protected:
		// These are the locations of default attributes in the shader programs used:
		enum ProgramAttributes {
			POSITION = 0,
			NORMAL = 1,
			COLOR = 2,
			MAPPING = 3
		};
		
		Ref<BirdsEyeCamera> _camera;
		Ref<IProjection> _projection;
		Ref<Viewport> _viewport;
				
		Ref<ShaderManager> _shader_manager;
		Ref<TextureManager> _texture_manager;
		Ref<ImageRenderer> _pixel_buffer_renderer;
		
		Ref<WireframeRenderer> _wireframe_renderer;
		Ref<Program> _wireframe_program;
		
		typedef Geometry::Mesh<> MeshT;
		Ref<MeshBuffer<MeshT>> _grid_mesh_buffer;
		
		Ref<VideoStream> _video_stream;
		Ref<VideoStreamRenderer> _video_stream_renderer;

		// Text rendering
		Ref<Text::Font> _font;
		Ref<Text::TextBuffer> _text_buffer;
		Ref<ImageRenderer> _text_renderer;
		Ref<Program> _text_program;

	public:
		virtual ~ImageSequenceScene ();
		
		void set_video_stream(Ptr<VideoStream> video_stream) { _video_stream = video_stream; }
		
		virtual void will_become_current (ISceneManager *);
		virtual void will_revoke_current (ISceneManager *);
		
		virtual bool event (const EventInput & input);
		virtual bool button (const ButtonInput & input);
		virtual bool motion (const MotionInput & input);
		virtual bool resize (const ResizeInput & input);
				
		GLuint compile_shader_of_type (GLenum type, StringT path);
		Ref<Program> load_program(StringT name);
		
		virtual void render_frame_for_time (TimeT time);
	};
	
	ImageSequenceScene::~ImageSequenceScene ()
	{
	}
	
	GLuint ImageSequenceScene::compile_shader_of_type (GLenum type, StringT name)
	{
		Ref<IData> data = resource_loader()->data_for_resource(name);
		
		if (data) {
			return _shader_manager->compile(type, data->buffer().get());
		} else {
			return 0;
		}
	}
	
	Ref<Program> ImageSequenceScene::load_program(StringT name) {
		GLuint vertex_shader = compile_shader_of_type(GL_VERTEX_SHADER, name + ".vertex-shader");
		GLuint geometry_shader = compile_shader_of_type(GL_GEOMETRY_SHADER, name + ".geometry-shader");
		GLuint fragment_shader = compile_shader_of_type(GL_FRAGMENT_SHADER, name + ".fragment-shader");
		
		Ref<Program> program = new Program;
		
		logger()->log(LOG_INFO, LogBuffer() << "Loading shader: " << name);
		
		DREAM_ASSERT(vertex_shader || geometry_shader || fragment_shader);
		
		if (vertex_shader)
			program->attach(vertex_shader);
		
		if (geometry_shader)
			program->attach(geometry_shader);
		
		if (fragment_shader)
			program->attach(fragment_shader);
		
		program->link();
		program->bind_fragment_location("fragment_color");
				
		return program;
	}
	
	void ImageSequenceScene::will_become_current(ISceneManager * manager) {
		Scene::will_become_current(manager);
		
		glEnable(GL_DEPTH_TEST);
		
		check_graphics_error();
		
		_camera = new BirdsEyeCamera;
		_camera->set_distance(10);
		_camera->set_multiplier(Vec3(0.1, 0.1, 0.01));
		
		_projection = new PerspectiveProjection(R90 * 0.7, 1, 1024 * 12);
		_viewport = new Viewport(_camera, _projection);
		_viewport->set_bounds(AlignedBox<2>(ZERO, manager->display_context()->size()));
		
		_shader_manager = new ShaderManager;
		_texture_manager = new TextureManager;
		_pixel_buffer_renderer = new ImageRenderer(_texture_manager);
		
		TextureParameters parameters;
		parameters.generate_mip_maps = true;
		parameters.min_filter = GL_LINEAR_MIPMAP_LINEAR;
		parameters.mag_filter = GL_LINEAR;
		parameters.target = GL_TEXTURE_2D;
		
		{
			_wireframe_renderer = new WireframeRenderer;
			
			_wireframe_program = load_program("Shaders/wireframe");
			_wireframe_program->set_attribute_location("position", WireframeRenderer::POSITION);
			_wireframe_program->link();
			
			auto binding = _wireframe_program->binding();
			binding.set_uniform("major_color", Vec4(1.0, 1.0, 1.0, 1.0));
		}
		
		{
			using namespace Geometry;
			
			Shared<MeshT> mesh = new MeshT;
			mesh->layout = LINES;
			Generate::grid(*mesh, 32, 2.0);
			
			_grid_mesh_buffer = new MeshBuffer<MeshT>();
			_grid_mesh_buffer->set_mesh(mesh);
			
			{
				auto binding = _grid_mesh_buffer->vertex_array().binding();
				auto attributes = binding.attach(_grid_mesh_buffer->vertex_buffer());
				attributes[POSITION] = &MeshT::VertexT::position;
			}
		}
		
		
		Ref<RendererState> renderer_state = new RendererState;
		renderer_state->viewport = _viewport;
		renderer_state->texture_manager = _texture_manager;
		renderer_state->shader_manager = _shader_manager;
		renderer_state->resource_loader = manager->resource_loader();
		_video_stream_renderer = new VideoStreamRenderer(renderer_state);
		
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		
		{
			using namespace Text;
			
			_font = resource_loader()->load<Font>("Fonts/Monaco");
			_font->set_pixel_size(18);

			_text_buffer = new TextBuffer(_font);
			_text_renderer = new ImageRenderer(renderer_state->texture_manager);
			auto & texture_parameters = _text_renderer->texture_parameters();
			texture_parameters = TextureParameters::NEAREST;
			texture_parameters.internal_format = GL_RG;

			_text_program = renderer_state->load_program("Shaders/text");

			_text_program->set_attribute_location("position", 0);
			_text_program->set_attribute_location("mapping", 1);
			_text_program->link();

			{
				auto binding = _text_program->binding();
				binding.set_texture_unit("diffuse_texture", 0);
				binding.set_uniform("highlight_color", Vec4(IDENTITY));
			}

		}
	}
	
	void ImageSequenceScene::will_revoke_current (ISceneManager * manager)
	{
		Scene::will_revoke_current(manager);
				
		_shader_manager = NULL;
		_texture_manager = NULL;
	}
	
	bool ImageSequenceScene::event(const EventInput &input) {
		// Grab the cursor
		Ref<IContext> context = manager()->display_context();
		
		if (input.event() == EventInput::RESUME) {
			//context->set_cursor_mode(CURSOR_GRAB);
			//logger()->log(LOG_INFO, LogBuffer() << "Cursor Mode: " << context->cursor_mode());
			
			return true;
		} else {
			//context->set_cursor_mode(CURSOR_NORMAL);
			//logger()->log(LOG_INFO, LogBuffer() << "Cursor Mode: " << context->cursor_mode());
			
			return true;
		}
		
		return false;
	}
	
	bool ImageSequenceScene::resize (const ResizeInput & input)
	{
		logger()->log(LOG_INFO, LogBuffer() << "Resizing to " << input.new_size());
		
		_viewport->set_bounds(AlignedBox<2>(ZERO, input.new_size()));
		glViewport(0, 0, input.new_size()[WIDTH], input.new_size()[HEIGHT]);

		{
			auto binding = _text_program->binding();

			auto projection = orthographic_projection_matrix(AlignedBox3(ZERO, input.new_size() << 1.0));

			binding.set_uniform("display_transform", projection);
		}

		check_graphics_error();
		
		return Scene::resize(input);
	}
	
	bool ImageSequenceScene::motion (const MotionInput & input) {
		if (input.button_pressed(MouseLeftButton)) {
			if (_video_stream_renderer->select_feature_point(input.current_position().reduce())) {
				return true;
			}
		}
		
		return _camera->process(input);
	}
	
	bool ImageSequenceScene::button (const ButtonInput & input)
	{
		if (input.button_pressed('t')) {
			_video_stream_renderer->set_alignment_mode(ORIENTATION_ALIGNMENT);
			return true;
		} else if (input.button_pressed('r')) {
			_video_stream_renderer->set_alignment_mode(NO_ALIGNMENT);
			return true;
		} else if (input.button_pressed('e')) {
			_video_stream_renderer->set_alignment_mode(FEATURE_ALIGNMENT);
			return true;
		} else if (input.button_pressed('f')) {
			_video_stream_renderer->update_feature_transform();
			return true;
		} else if (input.button_pressed('v')) {
			_video_stream_renderer->find_vertical_edges();
		} else if (input.button_pressed('a')) {
			_video_stream_renderer->apply_feature_algorithm();
		} else if (input.button_pressed('o')) {
			Vec2u range = _video_stream_renderer->range();
			
			if (range[0] > 0)
				range[0] -= 1;
			
			_video_stream_renderer->set_range(range);
			return true;
		} else if (input.button_pressed('p')) {
			Vec2u range = _video_stream_renderer->range();
			
			if (range[0] < (_video_stream->images().size() - 1))
				range[0] += 1;
			
			_video_stream_renderer->set_range(range);
			return true;
		}
		
		return false;
	}
	
	void ImageSequenceScene::render_frame_for_time (TimeT time)
	{
		Scene::render_frame_for_time(time);
		
		/// Dequeue any button/motion/resize events and process them now (in this thread).
		manager()->process_pending_events(this);
		
		check_graphics_error();
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			auto binding = _wireframe_program->binding();
			binding.set_uniform("display_matrix", _viewport->display_matrix());
			
			AlignedBox3 sphere_box(-4, 4);
			_wireframe_renderer->render(sphere_box);
			_wireframe_renderer->render_axis();
		}

		_video_stream_renderer->render_frame_for_time(time, _video_stream);

		{
			auto binding = _text_program->binding();

			bool regenerated;

			StringStreamT buffer;

			{
				auto range = this->_video_stream_renderer->range();
				auto & first_image_update = this->_video_stream->images().at(range[0]);

				buffer << "Gravity: " << first_image_update.gravity << std::endl;
				buffer << "Time Offset: " << first_image_update.time_offset << std::endl;
				buffer << "Tilt: " << first_image_update.tilt() * R2D << std::endl;

				auto index = this->_video_stream_renderer->selected_feature_point();
				auto & image_update = this->_video_stream->images().at(index[0]);

				buffer << "Feature Index: " << index << std::endl;

				if (image_update.feature_points && image_update.feature_points->offsets().size() > index[1]) {
					auto feature = image_update.feature_points->offsets().at(index[1]);
					buffer << "Feature Offset: " << feature << std::endl;
				}

				if (image_update.image_buffer) {
					buffer << "Frame Size: " << image_update.image_buffer->size() << std::endl;
				}
				
				buffer << "Frame Index: " << range << std::endl;
			}

			_text_buffer->set_text(buffer.str());

			Ref<Image> text_image = _text_buffer->render_text(regenerated);
			
			if (regenerated) {
				_text_renderer->invalidate(text_image);
			}

			_text_renderer->render(AlignedBox2::from_origin_and_size(ZERO, text_image->size()), text_image, Vec2b(false, false), 0);
		}
		
		check_graphics_error();
	}
	
	class TransformFlowApplicationDelegate : public Object, implements IApplicationDelegate
	{
		protected:
			Ref<IContext> _context;
		
			virtual ~TransformFlowApplicationDelegate ();
			virtual void application_did_finish_launching (IApplication * application);
			
			virtual void application_will_enter_background (IApplication * application);
			virtual void application_did_enter_foreground (IApplication * application);
	};
	
	TransformFlowApplicationDelegate::~TransformFlowApplicationDelegate() {
	}
	
	void TransformFlowApplicationDelegate::application_did_finish_launching (IApplication * application)
	{		
		Ref<Dictionary> config = new Dictionary;
		_context = application->create_context(config);
		
		Ref<Thread> thread = new Events::Thread;
		Ref<ILoader> loader = SceneManager::default_resource_loader();
		
		Ref<SceneManager> scene_manager = new SceneManager(_context, thread->loop(), loader);

		Path root_data_path = "/Users/samuel/Documents/Programming/Graphics/transform-flow/Data/";

		Path data_path = root_data_path + "VideoStream-2013-05-22-18-13-23";
		
		Ref<VideoStream> video_stream = new VideoStream(data_path);
		
		Ref<ImageSequenceScene> image_sequence_scene = new ImageSequenceScene;
		image_sequence_scene->set_video_stream(video_stream);
		
		scene_manager->push_scene(image_sequence_scene);
		
		_context->start();
	}
		
	void TransformFlowApplicationDelegate::application_will_enter_background (IApplication * application)
	{
		logger()->log(LOG_INFO, "Entering background...");

		_context->stop();
	}
	
	void TransformFlowApplicationDelegate::application_did_enter_foreground (IApplication * application)
	{
		logger()->log(LOG_INFO, "Entering foreground...");
		
		_context->start();
	}
}

int main (int argc, const char * argv[])
{
	using namespace TransformFlow;
	using namespace Dream::Client::Display;
	
	Ref<TransformFlowApplicationDelegate> delegate = new TransformFlowApplicationDelegate;
	IApplication::start(delegate);
	
    return 0;
}
