uniform bool betaLodEnabled;
uniform sampler2D betaLodCoverage;
uniform vec2 betaLodOrigin;
uniform vec2 betaLodTransition;

vec2 lodCoverage(vec2 position) {
 if(!betaLodEnabled) return vec2(0.0);
 ivec2 cell = ivec2(floor((position - betaLodOrigin) / 16.0));
 if(any(lessThan(cell, ivec2(0))) || any(greaterThanEqual(cell, textureSize(betaLodCoverage, 0))))
  return vec2(0.0);
 return texelFetch(betaLodCoverage, cell, 0).rg;
}

float lodBlend(vec2 position) {
 return smoothstep(betaLodTransition.x, betaLodTransition.y, length(position));
}

float lodDither() {
 ivec2 p = ivec2(gl_FragCoord.xy) & 7;
 ivec2 q = p ^ ivec2(p.y);
 int rank = ((q.x & 1) * 2 + (p.y & 1)) * 16
          + (((q.x >> 1) & 1) * 2 + ((p.y >> 1) & 1)) * 4
          + (((q.x >> 2) & 1) * 2 + ((p.y >> 2) & 1));
 return (float(rank) + 0.5) / 64.0;
}
