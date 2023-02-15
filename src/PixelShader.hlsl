struct ps_input_t {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
	float dist : DIST;
};

Texture2D texture_ps;
SamplerState sampler_ps;

float4 main(ps_input_t input) : SV_TARGET {	
	float fogy = clamp(1 - input.dist, 0, 1);
	float4 fog_color = float4(0.2f, 0.5f, 0.5f, 1.0f);

	return input.color
	       * texture_ps.Sample(sampler_ps, input.tex)
		   * fogy + (1 - fogy) * fog_color;
}
