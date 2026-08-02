
vec2 WeylNth(int n)
{
	return fract(vec2(n * 12664745, n*9560333) / exp2(24.0));
}

vec2 JitterSampleOffset(int frameOffset)
{
	return (WeylNth(int(mod(frameCounter + frameOffset, 16.0f))) * 2.0 - 1.0);
}

void TemporalJitterProjPos(inout vec4 pos)
{
	pos.xy += JitterSampleOffset(0) / vec2(viewWidth, viewHeight);
}

void TemporalJitterProjPos01(inout vec2 pos)
{
	pos.xy += (JitterSampleOffset(0) * 0.5) / vec2(viewWidth, viewHeight);
}

void TemporalJitterProjPosInv01(inout vec2 pos)
{
	pos.xy -= (JitterSampleOffset(0) * 0.5) / vec2(viewWidth, viewHeight);
}


void TemporalJitterProjPosPrevInv(inout vec4 pos)
{
	vec2 prevOffset = JitterSampleOffset(-1);
	vec2 thisOffset = JitterSampleOffset(0);
	pos.xy += (prevOffset - thisOffset) / vec2(viewWidth, viewHeight) * 0.5;
	// pos.xy += (JitterSampleOffset(-1)) / vec2(viewWidth, viewHeight);
}
