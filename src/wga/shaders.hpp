//
// Created by edvas on 9/9/23.
//

#ifndef WGA_SHADERS_HPP
#define WGA_SHADERS_HPP

namespace wga {
    [[maybe_unused]] static constexpr const char *triangle_shader_source = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4f
{
    var p = vec2f(0.0, 0.0);

    if (in_vertex_index == 0u)
    {
        p = vec2f(-0.5, -0.5);
    }
    else if (in_vertex_index == 1u)
    {
        p = vec2f(0.5, -0.5);
    }
    else
    {
        p = vec2f(0.0, 0.5);
    }

    return vec4f(p, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f
{
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

    static constexpr const char *basic_shader_source = R"(
@vertex
fn vs_main(@location(0) in_vertex_position: vec2f) -> @builtin(position) vec4f
{
	return vec4f(in_vertex_position, 0.0, 1.0);
}

@fragment
fn fs_main() -> @location(0) vec4f
{
    return vec4f(0.0, 0.4, 1.0, 1.0);
}
)";

    static constexpr const char *basic_color_shader_source = R"(
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
    var out: vertex_output;
    out.position = vec4f(in.position, 0.0, 1.0);
    out.color = in.color;
	return out;
}

@fragment
fn fs_main(in: vertex_output) -> @location(0) vec4f
{
    return vec4f(in.color, 1.0);
}
)";

}

#endif //WGA_SHADERS_HPP
