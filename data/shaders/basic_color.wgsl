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
    @location(1) normal: vec3f,
    @location(2) color: vec3f,
};

struct vertex_output
{
    @builtin(position) position: vec4f,
    @location(0) color: vec3f,
    @location(1) normal: vec3f,
};

@vertex
fn vs_main(in: vertex_input) -> vertex_output
{
    let M = us.projection_matrix * us.view_matrix * us.model_matrix;
    var out: vertex_output;
    out.position = us.projection_matrix * us.view_matrix * us.model_matrix * vec4f(in.position, 1.0);
    out.normal = (us.model_matrix * vec4f(in.normal, 0.0)).xyz;
    out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f
{
    let normal = normalize(in.normal);

    let light_source_1 = vec3f(0.5, -0.9, 0.1);
    let light_color_1 = vec3f(1.0, 0.9, 0.6);
    let shading_1 = max(0.0, dot(light_source_1, normal)) * light_color_1;

    let light_source_2 = vec3f(0.2, 0.4, 0.3);
    let light_color_2 = vec3f(0.6, 0.9, 1.0);
    let shading_2 = max(0.0, dot(light_source_2, normal)) * light_color_2;

    //let color = in.color * us.color.rgb;
    let shading = shading_1 + shading_2;

    let color = in.color * shading;
    let linear_color = pow(color, vec3f(2.2));
    return vec4f(linear_color, us.color.a);
}
