f16vec4 tex_sample_rgba = f16vec4(texture(gtexture, v.coord));
			if (tex_sample_rgba.a < float16_t(0.1)) { discard; }
			f16vec3 color = f16vec3(tex_sample_rgba.rgb);
