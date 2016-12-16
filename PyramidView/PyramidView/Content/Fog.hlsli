#define fog_ro 1.03f
#define fog_r0 exp(-5.0f)//#define fog_r0 exp(-6.0f)
#define fog_mu exp(0.0f)//#define fog_mu exp(80.0f)

float HFog(float h1, float h2, float S, float h0, float mu0, float ho)
{
	// Ray to plane angle
	float sina = abs(h2 - h1) / S;

	// Characteristic pass length of ray
	float S_ = h0 * abs(exp(-(h1 - ho) / h0) - exp(-(h2 - ho) / h0)) / sina;

	// Transmittance (Lambert law)
	float T = exp(- mu0 * S_);

	return T;
}

float SFog(float3 P, float3 E, float3 eye, float r0, float mu0, float ro)
{
	float s = length(eye - P);
	float3 S = normalize(eye - P);

	float PO = dot(S, E - P);
	float3 O = P + PO * S;

	float r1 = length(P - E);
	float r  = length(O - E);
	float r2 = length(eye - E);
	float x1 = length(P - O);
	float x2 = length(eye - O);

	if (PO > 0.0f)
		return HFog(r1, r, x1, r0, mu0, ro) * HFog(r, r2, x2, r0, mu0, ro);
	else
		return HFog(r1, r2, s, r0, mu0, ro);
}