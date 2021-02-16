#include "core.hlsl"

#ifdef SV_VERTEX_SHADER

struct Input {
    float3 position : Position;
    float3 normal : Normal;
    float3 tangent : Tangent;
	float2 texcoord : Texcoord;
};

struct Output {
    float3 frag_position : FragPosition;
    float3x3 TBN : TBN;
	float2 texcoord : FragTexcoord;
    float4 position : SV_Position;
};

SV_CONSTANT_BUFFER(instance_buffer, b0) {
    matrix mvm;
	matrix imvm;
};

SV_CONSTANT_BUFFER(camera_buffer, b1) {
    Camera camera;
};

Output main(Input input) 
{
    Output output;

    float4 pos = mul(mvm, float4(input.position, 1.f));
    output.frag_position = pos.xyz;
    output.position = mul(camera.pm, pos);

	float3 bitangent = cross(input.normal, input.tangent);
	output.TBN = float3x3(input.tangent, bitangent, input.normal);
	output.TBN = mul((float3x3)imvm, output.TBN);

	output.texcoord = input.texcoord;

    return output;
}

#endif

#ifdef SV_PIXEL_SHADER

struct Input {
    float3 position : FragPosition;
    float3x3 TBN : TBN;
	float2 texcoord : FragTexcoord;
};

struct Output {
    float4 color : SV_Target0;
};

struct Material {
	float3	diffuse_color;
	u32		flags;
	float3	specular_color;
	f32		shininess;
};

#define MAT_FLAG_NORMAL_MAPPING SV_BIT(0u)

#define LIGHT_TYPE_POINT 1u
#define LIGHT_TYPE_DIRECTION 2u

struct Light {
	float3 position;
	u32 type;
	f32	range;
	f32 intensity;
	f32 smoothness;
	f32 _padding0;
	float3 color;
	f32 _padding1;
};

#define LIGHT_COUNT 10u

SV_CONSTANT_BUFFER(material_buffer, b0) {
	Material material;
};

SV_CONSTANT_BUFFER(light_instances_buffer, b1) {
	Light lights[LIGHT_COUNT];
};

SV_TEXTURE(diffuse_map, t0);
SV_TEXTURE(normal_map, t1);

SV_SAMPLER(sam, s0);

Output main(Input input) 
{
    Output output;

	float3 normal;

	if (material.flags & MAT_FLAG_NORMAL_MAPPING) 
		normal = normalize(mul(input.TBN, (normal_map.Sample(sam, input.texcoord).xyz * 2.f - 1.f)));
	else
		normal = normalize(float3(input.TBN._13, input.TBN._23, input.TBN._33));

	float3 light_accumulation = float3(0.f, 0.f, 0.f);

	[unroll]
	foreach(i, LIGHT_COUNT) {

		Light light = lights[i];

		switch (light.type) {

		case LIGHT_TYPE_POINT:
		{
			float3 to_light = light.position - input.position;
			f32 distance = length(to_light);
			to_light = normalize(to_light);

			// Diffuse
			f32 diffuse = max(dot(normal, to_light), 0.1f);

			// Specular
			float specular = pow(max(dot(normalize(-input.position), reflect(-to_light, normal)), 0.f), material.shininess);

			// TODO attenuation

			light_accumulation += light.color * ((diffuse * material.diffuse_color) + (specular * material.specular_color)) * light.intensity;
		}
		break;

		case LIGHT_TYPE_DIRECTION:
		{
			// Diffuse
			f32 diffuse = max(dot(normal, light.position), 0.f);

			// Specular
			float specular = pow(max(dot(normalize(-input.position), reflect(-light.position, normal)), 0.f), material.shininess);

			light_accumulation += light.color * ((diffuse * material.diffuse_color) + (specular * material.specular_color)) * light.intensity;
		}
		break;

		}
	}

	float4 diffuse = diffuse_map.Sample(sam, input.texcoord);
    
    output.color = diffuse * float4(light_accumulation, 1.f);

    return output;
}

#endif