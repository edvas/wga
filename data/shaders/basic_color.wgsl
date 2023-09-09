
struct vertex_input
{
    @location(0) position: vec2f,
    @location(1) color: vec3f,
};

struct vertex_output
{
    @builtin(position) position: vec4f,
    // required_limits.limits.maxInterStageShaderComponents >= 3
    @location(0) color: vec3f,
};

@vertex
fn vs_main(in: vertex_input) -> vertex_output
{
    let ratio = 640.0 / 480.0;
    let offset = vec2f(-0.6875, -0.463);

    var out: vertex_output;
    out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
    out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f
{
    let linear_color = pow(in.color, vec3f(2.2));
    return vec4f(linear_color, 1.0);
}
