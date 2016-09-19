/*
Made By Talha Ali
Using C++, OpenGL and GLSL.
Built upon open source library, Cinder.
*/



#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/Rand.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;


#if defined( CINDER_GL_ES ) 
const int MAXPARTICLES = 600e2;
const float SPACING=0.03;
#else
const int MAXPARTICLES = 600e3;
const float SPACING = 0.003;
#endif

class ParticleSystemApp : public App {
  public:
	void setup() override;
	void update() override;
	void draw() override;

	private:
	gl::GlslProgRef mRenderProg;
	gl::GlslProgRef mUpdateProg;
	gl::VaoRef		mAttributes[2];
	gl::VboRef		pbuffer[2];
	std::uint32_t	initialIndex = 0;
	std::uint32_t	FinalIndex = 1;
	vec3			mMousePos = vec3(-1000, 0, 0);//3 field vector representing the mouse position
	float Disturbance = 300.0f;
};


//This is a struct holding all the data required in order to create the particles
struct ParticleData
{
	vec3	position; //3 field vector representing position of particles during "rest" state.
	vec3	particleposition;//3 Field Vector represnting position of particles during "disturbance"
	vec3	home;
	ColorA  color;//Color of particles
	float	damping;//The damping effect
};




void ParticleSystemApp::setup()
{
	vector<ParticleData> particles; //Creates a vector of that hold ParticleData Struct
	particles.assign(MAXPARTICLES, ParticleData());//Assigns a size of Num_Particles, and assigns it the struct particle

	for (int i = 0; i < particles.size(); ++i) //For loop going through entire vector
	{	
		float x = SPACING*i; //Defines X value for particles
		float y = Rand::randFloat(0, 1.0f)*getWindowHeight();//Defines y value for particles, randomly generated.
		float z = Rand::randFloat(0, 1.0f)*180.0f;		//Defines z axis, just like y value, it is randomly generated.
		
		auto &p = particles.at(i);//Address of particles vector.
		//Assigns data to each variable in the ParticlesData Struct
		p.position = vec3(x, y, z);
		p.home = p.position;
		p.particleposition = p.home;
		p.damping = 0.9f;
		p.color = Color(CM_HSV, lmap<float>(i, 0.0f, particles.size(), 0.0f, 1.0f), 1.0f, 1.0f);
	}

	// Create particle buffers on GPU and copy data into the first buffer.
	pbuffer[initialIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, particles.size() * sizeof(ParticleData), particles.data(), GL_STATIC_DRAW);
	pbuffer[FinalIndex] = gl::Vbo::create(GL_ARRAY_BUFFER, particles.size() * sizeof(ParticleData), nullptr, GL_STATIC_DRAW);

	mRenderProg = gl::getStockShader(gl::ShaderDef().color());

	for (int i = 0; i < 2; ++i)
	{	// Describe the particle layout for OpenGL.
		mAttributes[i] = gl::Vao::create();
		gl::ScopedVao vao(mAttributes[i]);

		// Define attributes as offsets into the bound particle buffer
		gl::ScopedBuffer buffer(pbuffer[i]);
		gl::enableVertexAttribArray(0);
		gl::enableVertexAttribArray(1);
		gl::enableVertexAttribArray(2);
		gl::enableVertexAttribArray(3);
		gl::enableVertexAttribArray(4);
		gl::vertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleData), (const GLvoid*)offsetof(ParticleData, position));
		gl::vertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ParticleData), (const GLvoid*)offsetof(ParticleData, color));
		gl::vertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleData), (const GLvoid*)offsetof(ParticleData, particleposition));
		gl::vertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(ParticleData), (const GLvoid*)offsetof(ParticleData, home));
		gl::vertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(ParticleData), (const GLvoid*)offsetof(ParticleData, damping));
	}



	//Loading our Vertex Shaders
#if defined( CINDER_GL_ES_3 )
	mUpdateProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("particleUpdate_es3.vs"))
		.fragment(loadAsset("no_op_es3.fs"))
#else
	mUpdateProg = gl::GlslProg::create(gl::GlslProg::Format().vertex(loadAsset("particleUpdate.vs"))
#endif
		.feedbackFormat(GL_INTERLEAVED_ATTRIBS)
		.feedbackVaryings({ "position", "pposition", "home", "color", "damping" })
		.attribLocation("iPosition", 0)
		.attribLocation("iColor", 1)
		.attribLocation("iPPosition", 2)
		.attribLocation("iHome", 3)
		.attribLocation("iDamping", 4)
		);



	//Mouse related input goes here
	getWindow()->getSignalMouseDown().connect([this](MouseEvent event)
	{
		Disturbance = 800.0f;
		mMousePos = vec3(event.getX(), event.getY(), 0.0f);
	});
	getWindow()->getSignalMouseMove().connect([this](MouseEvent event)
	{
		mMousePos = vec3(event.getX(), event.getY(), 0.0f);
	});
	

	getWindow()->getSignalMouseDrag().connect([this](MouseEvent event)
	{
		mMousePos = vec3(event.getX(), event.getY(), 0.0f);
	});

	getWindow()->getSignalMouseUp().connect([this](MouseEvent event)
	{
		Disturbance = 300.0f;
	});


}



void ParticleSystemApp::update()
{
	//Update Vertex shaders with new uniform data
	gl::ScopedGlslProg prog(mUpdateProg);
	gl::ScopedState rasterizer(GL_RASTERIZER_DISCARD, true);	
	mUpdateProg->uniform("disturbance", Disturbance);//Set disturbance variable within the vertex shader
	mUpdateProg->uniform("MousePosition", mMousePos);//Set mouseposition data within the vertex shader.

	gl::ScopedVao source(mAttributes[initialIndex]);
	gl::bindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, pbuffer[FinalIndex]);
	gl::beginTransformFeedback(GL_POINTS);
	gl::drawArrays(GL_POINTS, 0, MAXPARTICLES);

	gl::endTransformFeedback();
	std::swap(initialIndex, FinalIndex);

}

void ParticleSystemApp::draw()
{
	//Create Background Color
	gl::clear(Color(0.2,0.2, 0.2));
	gl::setMatricesWindowPersp(getWindowSize());
	//Enable depth
	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::ScopedGlslProg render(mRenderProg);
	gl::ScopedVao vao(mAttributes[initialIndex]);
	gl::context()->setDefaultShaderVars();
	gl::drawArrays(GL_POINTS, 0, MAXPARTICLES);
}


CINDER_APP(ParticleSystemApp, RendererGl, [](App::Settings *settings) {
	settings->setWindowSize(1280, 720);
	settings->setMultiTouchEnabled(false);
})
