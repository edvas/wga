struct uniforms
{
    projection_matrix: mat4x4f,
    view_matrix: mat4x4f,
    model_matrix: mat4x4f,
    color: vec4f,
    time: f32,
};

@group(0) @binding(0) var<uniform> us: uniforms;

struct vertex_input
{
    @location(0) position: vec3f,
    @location(1) color: vec3f,
};

struct vertex_output
{
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
};

@vertex
fn vs_main(in: vertex_input) -> vertex_output
{
    let M = us.projection_matrix * us.view_matrix * us.model_matrix;
    var out: vertex_output;
    out.position = us.projection_matrix * us.view_matrix * us.model_matrix * vec4f(in.position, 1.0);
    out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f
{
    let color = in.color * us.color.rgb;
    let linear_color = pow(color, vec3f(2.2));
    return vec4f(linear_color, us.color.a);
}
