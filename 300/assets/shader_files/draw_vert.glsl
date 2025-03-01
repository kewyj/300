#version 450

//#extension GL_ARB_separate_shader_objects  : enable
//#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inQPos;		        // INPUT_POSITION

layout (location = 1) in vec4 inVertexColor;        // input color
layout (location = 2) in vec2 inQUV;			    // Texture Coordinates
layout (location = 5) in vec3 inTangent;			// Per vertex Tangent
layout (location = 6) in vec3 inNormal;			    // Per vertex Normal
layout (location = 7) in mat4 inLTW;			    // local to world

uniform mat4 uMatrixVP;

out vec4 VertexColor;
out vec2 TexCoords;

void main() 
{
    // Decompress the binormal & transform everything to world space
    mat3 Rot                    = mat3(inLTW);

	 // Position
    vec4 Pos = vec4(inQPos.xyz, 1.0f);

    // Set all the output vars
    VertexColor                 = inVertexColor;
    gl_Position                 = uMatrixVP * inLTW * Pos;
    TexCoords                   = inQUV;
}
