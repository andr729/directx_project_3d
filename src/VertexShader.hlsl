cbuffer vs_const_buffer_t {
	float4x4 matWorldViewProj;
	float4x4 matWorldView;
	float4x4 matView;
	float4 colMaterial;
	float4 colLight;
	float4 dirLight;
	float4 padding;
};

struct vs_output_t {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
};

vs_output_t main(
 		float3 pos : POSITION, 
 		float3 norm : NORMAL, 
 		float4 col : COLOR,
		float2 tex : TEXCOORD,
		row_major float4x4 mat_w : WORLD,
  		uint instance_id: SV_InstanceID) {
	vs_output_t result;
	
	float4 NW = mul(float4(norm, 0.0f), matWorldView);
	float4 LW = mul(dirLight, matView);
	
	result.position = mul(mul(float4(pos, 1.0f), mat_w), matWorldViewProj);
	// result.position = mul(float4(pos, 1.0f), matWorldViewProj);
	result.color = col; //mul(
 			// max(-dot(normalize(LW), normalize(NW)), 0.2f), 
 			// colLight * col);

	result.tex = tex;

	return result;
}
