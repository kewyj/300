#version 450
#extension GL_ARB_gpu_shader_int64 : enable


// -- Workgroup Size --
layout(local_size_x = 32, local_size_y = 32, local_size_z = 1) in;

// -- Uniform Variables --
uniform int uEmitterCount;
uniform float uSeed;
uniform vec3 uCamPos;

float PHI = 1.61803398874989484820459;  // golden ratio

struct ParticleEmitter
{
    vec4 mStartColor;
	vec4 mEndColor;
	vec4 mSizeLifetimeCount;	// X: start size | Y: end size | Z: Lifetime | W: Count
	vec4 mPositionSpeed;		// XYZ: position | W: Speed
	vec4 mOffset;				// XYZ: offset position
	vec4 mRotationGravity;		// X: Rotation Delta | Y: Gravity
	uint64_t mTexture;
};

struct Particle
{
	vec4 mStartColor;
	vec4 mEndColor;
	vec4 mCurrColor;
	vec4 mVelocity;
	vec4 mSizeLife;			// X: Start size | Y: End size | Z: Life Time left | W: Max Life
	vec4 mPositionSpeed;	// XYZ: position | W: Speed
	vec4 mRotationGravity;	// X: Current Rotation | Y: Rotation Delta | Z: Gravity
	uint64_t mTexture;
	mat4 mLtwMatrix;		// Local-to-world transformation matrix
};

layout (std430, binding = 5) buffer emittersBuffer
{
    ParticleEmitter emitters[];
};

layout (std430, binding = 6) buffer particleBuffer
{
    Particle particles[];
};

layout (std430, binding = 7) buffer indexBuffer
{
    int count;
	int indices[];
}freelist;

// -- Helper Functions --
void MakeParticle(out Particle p, ParticleEmitter e);
void InitVectors(vec3 position);
float Rand(in vec2 xy);		// Returns value between [-0.5, 0.5]
float Rand2(in vec2 xy, float seed);	// Returns value between [0.0, 1.0]

// -- Global Variables -- 
vec3 rightVector;
vec3 upVector;
vec3 forwardVector;

void main()
{
	// Each thread group will process one emitter
	int emitterIndex = int(gl_WorkGroupID.y * gl_NumWorkGroups.x + gl_WorkGroupID.x);

	if (emitterIndex >= uEmitterCount)	// Boundary Check
		return;
	
	// Retrieve Emitter from buffer
	ParticleEmitter emitter = emitters[emitterIndex];
	int particleCount = int(emitter.mSizeLifetimeCount.w);
	
	InitVectors(emitter.mPositionSpeed.xyz);

	int threadIndex = int(gl_LocalInvocationID.y * gl_WorkGroupSize.x + gl_LocalInvocationID.x);
	if (threadIndex < particleCount)
	{
		// undo decrement and return if nothing in freelist
		int index = atomicAdd(freelist.count, -1) - 1;
		if (index < 0)
		{
			atomicAdd(freelist.count, 1);
			return;
		}
		int particleIndex = freelist.indices[index];
		MakeParticle(particles[particleIndex], emitter);
	}
}

void MakeParticle(out Particle p, ParticleEmitter e)
{
	// Position, Speed
	p.mPositionSpeed = e.mPositionSpeed;
	vec3 offset = Rand(vec2(gl_GlobalInvocationID.xy)) * e.mOffset.xyz;
	p.mPositionSpeed.xyz += vec3(offset.x * rightVector + offset.y * upVector + offset.z * forwardVector);
	// clamp speed
	float scale = fract(uSeed);
	p.mPositionSpeed.w *= clamp(Rand2(vec2(gl_GlobalInvocationID.xy), uSeed + 0.1) * p.mPositionSpeed.w, 0.1 * p.mPositionSpeed.w, p.mPositionSpeed.w);

	// Color
	p.mStartColor = e.mStartColor;
	p.mEndColor = e.mEndColor;
	
	// Size, Life time left
	p.mSizeLife.x = e.mSizeLifetimeCount.x;			// start size
	p.mSizeLife.y = e.mSizeLifetimeCount.y;			// end size
	p.mSizeLife.z = e.mSizeLifetimeCount.z;			// life left
	p.mSizeLife.w = e.mSizeLifetimeCount.z;			// life time
	p.mRotationGravity.y = e.mRotationGravity.x;	// Rotation
	p.mRotationGravity.z = e.mRotationGravity.y;	// Gravity

	// Random velocity with pseudorandom noise function, with thread index as seed
	p.mVelocity.xyz = Rand(vec2(gl_GlobalInvocationID.xz)) * rightVector + Rand(vec2(gl_GlobalInvocationID.yz)) * upVector;	// Make velocity be outwards perpendicular to camera's view
	p.mVelocity.xyz += upVector * p.mRotationGravity.z;		// Factor in gravity
	p.mVelocity.xyz = normalize(p.mVelocity.xyz);
	// Texture handle
	p.mTexture = e.mTexture;
}

void InitVectors(vec3 position)
{
	// Compute the rotation vectors
	forwardVector	= normalize(uCamPos - position);
	rightVector		= normalize(cross(forwardVector, vec3(0, 1, 0)));
	upVector		= normalize(cross(rightVector, forwardVector));
}

float hash(float n)
{
	return fract(sin(n) * 43758.5453);
}

//float Rand(vec3 x)
//{
//	// The noise function returns a value in the range -1.0f -> 1.0f
//
//	vec3 p = floor(x);
//	vec3 f = fract(x);
//
//	f = f * f * (3.0 - 2.0 * f);
//	float n = p.x + p.y * 57.0 + 113.0 * p.z;
//
//	float num =  mix(mix(mix(hash(n + 0.0), hash(n + 1.0), f.x),
//		mix(hash(n + 57.0), hash(n + 58.0), f.x), f.y),
//		mix(mix(hash(n + 113.0), hash(n + 114.0), f.x),
//			mix(hash(n + 170.0), hash(n + 171.0), f.x), f.y), f.z);
//
//	return num - 0.5;
//}

float Rand(in vec2 xy)
{
       float num = fract(tan(distance(xy*PHI, xy)*uSeed)*xy.x);

	   return num - 0.5;
}

float Rand2(in vec2 xy, float seed)
{
       return fract(tan(distance(xy*PHI, xy)* seed)*xy.x);
}