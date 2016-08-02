#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "stdint.h"
#include "assert.h"

#include "tiny_jpeg.h"

#define V3OP(op)\
    vec3 operator op (const vec3& a, const vec3& b){\
        return vec3(\
            a.x op b.x,\
            a.y op b.y,\
            a.z op b.z\
            );\
    }\
    vec3 operator op (const vec3& a, float b){\
        return vec3(\
            a.x op b,\
            a.y op b,\
            a.z op b\
            );\
    }\
    vec3 operator op (float a, const vec3& b){\
        return vec3(\
            a op b.x,\
            a op b.y,\
            a op b.z\
            );\
    }


struct vec3{
    float x, y, z;
    vec3() : x(0.0f), y(0.0f), z(0.0f){};
    vec3(float a, float b, float c) : x(a), y(b), z(c){};
    vec3(float* p) : x(p[0]), y(p[1]), z(p[2]){};
    vec3(float f) : x(f), y(f), z(f){};
    vec3(const vec3& o) : x(o.x), y(o.y), z(o.z){};
    void print(){
        printf("%.4f %.4f %.4f\n", x, y, z);
    }
};

V3OP(+)
V3OP(-)
V3OP(*)
V3OP(/)

float dot(const vec3& a, const vec3& b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
vec3 cross(const vec3& a, const vec3& b){
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}
vec3 reflect(const vec3& d, const vec3& n){
	return vec3(d - 2.0f * dot(d, n) * n);
}
#define MAX(a, b) ( (a > b) ? a : b)
#define MIN(a, b) ( (a < b) ? a : b)
#define CLAMP(x, lo, hi) (MAX(MIN(x, hi), lo))
#define LERP(a, b, alpha) ((1.0f - alpha) * a + alpha * b)
vec3 lerp(const vec3& a, const vec3& b, float alpha){
    return vec3(
        LERP(a.x, b.x, alpha),
        LERP(a.y, b.y, alpha),
        LERP(a.z, b.z, alpha)
    );
}
float length(const vec3& a){
    return sqrt(dot(a, a));
}
float distance(const vec3& a, const vec3& b){
    return length(a - b);
}
vec3 normalize(const vec3& a){
    return vec3(a) / length(a);
}
float sphere(const vec3& p, float r){
    return length(p) - r;
}

float map(const vec3& point){
    return sphere(point - vec3(0.0f, 0.0f, -1.0f), 0.75f);
}

vec3 map_normal(const vec3& point){
    constexpr float e = 0.0001f;
    const vec3 x(e, 0.0f, 0.0f);
    const vec3 y(0.0f, e, 0.0f);
    const vec3 z(0.0f, 0.0f, e);
    return normalize(vec3(
        map(point + x) - map(point - x),
        map(point + y) - map(point - y),
        map(point + z) - map(point - z)
        ));
}

void do_ray(uint8_t* rgb, const vec3& eye, const vec3& light, float x, float y){
    constexpr float e = 0.0001f;
    bool hit = false, visible = true;
    vec3 origin = vec3(x, y, 0.0f);
    vec3 dir = normalize(origin - eye);
    rgb[0] = 0; rgb[1] = 0; rgb[2] = 0;
    
    vec3 point = origin;
    for(char i = 0; i < 60; i++){
        float dist = fabsf(map(point));
        if(dist < e){
            hit = true;
            break;
        }
        point = point + dir * dist;
    }

    if(!hit)return;
    
    vec3 L = normalize(light - point);
    {
        vec3 p2 = point + L * 100.0f * e;
        for(char i = 0; i < 60; i++){
            float dist = fabsf(map(p2));
            if(dist < e){
                visible = false;
                break;
            }
            p2 = p2 + L * dist;
        }
    }
    
    if(!visible){
        memset(rgb, 5, sizeof(char) * 3);
        return;
    }
    
    const vec3 N = map_normal(point);
    const float D = MAX(0.0f, dot(N, L));
    const vec3 H = normalize(L - dir);
    const float S = MAX(0.0f, powf(dot(N, H), 60.0f));
    float d2 = dot(light - point, light - point);
    float lum = (D + S) / d2;
    
    lum = CLAMP(lum, 0.0f, 1.0f);
    uint8_t lum8 = 5 + lum * 250;
    memset(rgb, lum8, sizeof(uint8_t) * 3);
}

int main(){
    const int32_t width = 2048, height = 2048;
    const float invw2 = 2.0f / width;
    const float invh2 = 2.0f / height;
    uint8_t* data = new uint8_t[width * height * 3];
    vec3 eye(0.0f, 0.0f, 1.0f);
    vec3 light(1.0f, -1.0f, 0.0f);
    
    size_t i = 0;
    for(int32_t y = 0; y < height; y++){
        for(int32_t x = 0; x < width; x++){
            const float xf = x * invw2 - 1.0f;
            const float yf = y * invh2 - 1.0f;
            do_ray(&data[i], eye, light, xf, yf);
            i += 3;
        }
    }
    
    if(!tje_encode_to_file("out.jpg", width, height, 3, data)){
        puts("encoding failed");
        return 1;
    }
    
    delete[] data;
    
    return 0;
}