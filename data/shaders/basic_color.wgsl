struct uniforms
{
    color: vec4f,
    time: f32,
};

@group(0) @binding(0) var<uniform> us: uniforms;

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
    var offset = vec2f(-0.6875, -0.463);
    offset += 0.3 * vec2f(cos(us.time), sin(us.time));

    var out: vertex_output;
    out.position = vec4f(in.position.x + offset.x, (in.position.y + offset.y) * ratio, 0.0, 1.0);
    out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f
{
    let color = in.color * us.color.rgb;
    let linear_color = pow(color, vec3f(2.2));
    return vec4f(linear_color, 1.0);
}
