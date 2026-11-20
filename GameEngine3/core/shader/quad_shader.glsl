void vertex_main(out VertexData v) {
	v.uv = m_TexCoord;
	v.position = vec4(m_Position, 1.);
}

void fragment_main(out FragmentData f) {
	f.diffuse = texture(t_Diffuse, out_texCoord);
}