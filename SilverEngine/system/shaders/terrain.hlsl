#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
       float  height : Position;
       float3 normal : Normal;
       float2 texcoord : Texcoord;
       float4 color : Color;
};

struct Output {
       float3 frag_position : FragPosition;
       float3 normal : FragNormal;
       float2 texcoord : FragTexcoord;
       float4 color : FragColor;
       float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(instance_buffer, b1) {
	matrix mvm;
	matrix imvm;
	u32 resolution_width;
	u32 resolution_height;
};

float3 compute_terrain_position(f32 h, u32 index, u32 width, u32 height)
{
	float3 p;
	p.y = h;
	
	p.x = f32(index % width) / f32(width) - 0.5f;
	p.z = -(f32(index / width) / f32(height) - 0.5f);

	return p;
}

Output main(Input input, u32 index : SV_VertexID)
{
    Output output;

    float3 terrain_position = compute_terrain_position(input.height, index, resolution_width, resolution_height);
    float4 pos = mul(float4(terrain_position, 1.f), mvm);
    
    output.frag_position = pos.xyz;
    output.position = mul(pos, camera.pm);
    output.texcoord = input.texcoord;
    output.normal = mul((float3x3)imvm, input.normal);

    output.color = input.color;

    return output;
}

#endif

#ifdef SV_PIXEL_SHADER

#include "shared_headers/lighting.h"

SV_TEXTURE(albedo_map, t0);

struct Input {
       float3 position : FragPosition;
       float3 normal : FragNormal;
       float2 texcoord : Texcoord;
       float4 color : FragColor;
};

struct Output {
       float4 color : SV_Target0;
       float4 normal : SV_Target1;
       float4 emission : SV_Target2;
};

Output main(Input input)
{
    Output output;

    float3 normal = normalize(input.normal);

    float3 lighting = compute_light(input.position, normal, 1.f, 1.f, float3(1.f, 1.f, 1.f));

    output.color = albedo_map.Sample(sam, input.texcoord * 50.f) * float4(lighting, 1.f);
    output.normal = float4(normal, 1.f);
    output.emission = float4(0.f, 0.f, 0.f, 1.f);
    return output;
}

#endif