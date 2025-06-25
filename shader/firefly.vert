#version 330 core

uniform float time;
uniform vec3 cameraWorldPos;
uniform mat4 matProj;
uniform mat4 view;

out float flicker;
out float distToCamera;

const float CHUNK_SIZE = 16.0;
const float MAP_Y = 8.0;
const float HEIGHT_VARIATION = 6.0;
const int TILE_RANGE = 5;  // ±5 chunks in each direction
const int FIREFLIES_PER_CHUNK = 500;

float hash1(float n) {
    return fract(sin(n) * 43758.5453);
}

vec2 hash2(float n) {
    return vec2(hash1(n), hash1(n + 1.0));
}

vec2 movementOffset(float id, float t) {
    float speed = 0.5;  // slow speed
    float x = sin(t * speed + id * 12.9898) * 0.5;
    float y = cos(t * speed + id * 78.233) * 0.5;
    return vec2(x, y);
}

void main() {
    int tileIndex = gl_VertexID / FIREFLIES_PER_CHUNK;  // 0..120
    int fireflyId = gl_VertexID % FIREFLIES_PER_CHUNK;  // 0..499

    int offsetX = (tileIndex % (2 * TILE_RANGE + 1)) - TILE_RANGE;
    int offsetZ = (tileIndex / (2 * TILE_RANGE + 1)) - TILE_RANGE;

    vec2 rand2D = hash2(float(fireflyId));
    vec2 basePos = rand2D * CHUNK_SIZE;

    float height = MAP_Y + (hash1(float(fireflyId + 3)) - 0.5) * HEIGHT_VARIATION * 2.0;

    vec2 moveOffset = movementOffset(float(fireflyId), time);

    vec2 camChunk = floor(cameraWorldPos.xz / CHUNK_SIZE);
    vec2 repeatedChunkOrigin = (camChunk + vec2(offsetX, offsetZ)) * CHUNK_SIZE;

    vec2 posXZ = basePos + repeatedChunkOrigin + moveOffset;

    vec3 worldPos = vec3(posXZ.x, height, posXZ.y);

    distToCamera = distance(worldPos, cameraWorldPos);

    gl_Position = matProj * view * vec4(worldPos, 1.0);
    gl_PointSize = 8.0;

    flicker = 0.5 + 0.5 * sin(time * 4.0 + float(fireflyId) * 17.0);
}
