#version 150

#define MAX_LINES 128

uniform mat4  inverse_mvp;
uniform vec4  viewport;
uniform vec4  lines[MAX_LINES];
uniform float strengths[MAX_LINES];
uniform vec2  vinf;
uniform int   n_lines;

in mat4 frag_mvp;

in vec4  gl_FragCoord;
out vec4 frag_color;

// 2D vector field visualization by Morgan McGuire, @morgan3d,
// http://casual-effects.com
const float PI = 3.14159265358979;

const int ARROW_V_STYLE = 1;
const int ARROW_LINE_STYLE = 2;

// Choose your arrow head style
const int   ARROW_STYLE = ARROW_LINE_STYLE;
const float ARROW_TILE_SIZE = 32.0;

// How sharp should the arrow head be? Used
const float ARROW_HEAD_ANGLE = 45.0 * PI / 180.0;

// Used for ARROW_LINE_STYLE
const float ARROW_HEAD_LENGTH = ARROW_TILE_SIZE / 5.0;
const float ARROW_SHAFT_THICKNESS = 3.0;

const float TAU = 2.0 * PI;
const int   INTEGRATOR_STEPS = 30;

// Computes the center pixel of the tile containing pixel pos
vec4
arrowTileCenterCoord(vec4 pos)
{
    return vec4((floor(pos.xy / ARROW_TILE_SIZE) + 0.5) * ARROW_TILE_SIZE,
                pos.zw);
}

// v = field sampled at tileCenterCoord(p), scaled by the length
// desired in pixels for arrows
// Returns 1.0 where there is an arrow pixel.
float
arrow(vec2 p, vec2 v)
{
    // Make everything relative to the center, which may be fractional
    p -= arrowTileCenterCoord(vec4(p, 0.0, 1.0)).xy;

    float mag_v = length(v), mag_p = length(p);

    if (mag_v > 0.0) {
        // Non-zero velocity case
        vec2 dir_p = p / mag_p, dir_v = v / mag_v;

        // We can't draw arrows larger than the tile radius, so clamp magnitude.
        // Enforce a minimum length to help see direction
        mag_v = clamp(mag_v, 5.0, ARROW_TILE_SIZE / 2.0);

        // Arrow tip location
        v = dir_v * mag_v;

        // Define a 2D implicit surface so that the arrow is antialiased.
        // In each line, the left expression defines a shape and the right
        // controls how quickly it fades in or out.

        float dist;
        if (ARROW_STYLE == ARROW_LINE_STYLE) {
            // Signed distance from a line segment based on
            // https://www.shadertoy.com/view/ls2GWG by Matthias Reitinger,
            // @mreitinger

            // Line arrow style
            dist = max(
                // Shaft
                ARROW_SHAFT_THICKNESS / 4.0 -
                    max(abs(dot(p, vec2(dir_v.y, -dir_v.x))), // Width
                        abs(dot(p, dir_v)) - mag_v +
                            ARROW_HEAD_LENGTH / 2.0), // Length

                // Arrow head
                min(0.0,
                    dot(v - p, dir_v) -
                        cos(ARROW_HEAD_ANGLE / 2.0) * length(v - p)) *
                        2.0 + // Front sides
                    min(0.0,
                        dot(p, dir_v) + ARROW_HEAD_LENGTH - mag_v)); // Back
        } else {
            // V arrow style
            dist = min(0.0, mag_v - mag_p) * 2.0 + // length
                   min(0.0,
                       dot(normalize(v - p), dir_v) -
                           cos(ARROW_HEAD_ANGLE / 2.0)) *
                       2.0 * length(v - p) + // head sides
                   min(0.0, dot(p, dir_v) + 1.0) + // head back
                   min(0.0,
                       cos(ARROW_HEAD_ANGLE / 2.0) -
                           dot(normalize(v * 0.33 - p), dir_v)) *
                       mag_v * 0.8; // cutout
        }

        return clamp(1.0 + dist, 0.0, 1.0);
    }

    // Center of the pixel is always on the arrow
    return max(0.0, 1.2 - mag_p);
}

/////////////////////////////////////////////////////////////////////

vec2
integrate(vec2 here, vec2 p1, vec2 p2)
{
    vec2 diff = p2 - p1;
    vec2 delta = diff / INTEGRATOR_STEPS;

    vec2 ans = vec2(0.0, 0.0);
    for (int i = 0; i < INTEGRATOR_STEPS; ++i) {
        vec2 current_pos = p1 + i * delta;

        vec2  r = here - current_pos;
        float denom = dot(r, r);

        vec2 grad = vec2(current_pos.y - here.y, here.x - current_pos.x);
        grad /= denom;

        ans += grad;
    }

    return ans / INTEGRATOR_STEPS;
}

// The vector field; use your own function or texture
vec2
field(vec4 pos)
{
    vec4 ndcPos;
    ndcPos.xy = (2.0 * pos.xy - 2.0 * viewport.xy) / viewport.zw - 1;
    ndcPos.z = (2.0 * pos.z - gl_DepthRange.near - gl_DepthRange.far) /
               (gl_DepthRange.far - gl_DepthRange.near);
    ndcPos.w = 1.0;

    vec4 clipPos = ndcPos / pos.w;
    vec4 worldPos = inverse_mvp * clipPos;

    vec2 total_flow = vec2(0.0, 0.0);

    for (int i = 0; i < n_lines; ++i) {
        vec4 p1 = frag_mvp * vec4(lines[i].xy, 0.0, 1.0);
        vec4 p2 = frag_mvp * vec4(lines[i].zw, 0.0, 1.0);
        total_flow += strengths[i] * integrate(worldPos.xy, p1.xy, p2.xy);
    }

    return total_flow / TAU + vinf;
}

void
main()
{
    float arrow_k = (1.0 - arrow(gl_FragCoord.xy,
                                 field(arrowTileCenterCoord(gl_FragCoord)) *
                                     ARROW_TILE_SIZE * 0.4));
    frag_color =
        arrow_k * vec4(normalize(field(gl_FragCoord)) * 0.5 + 0.5, 0.5, 1.0);
}
