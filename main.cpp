
#include "stdio.h"
#include "math.h"
#include "stdint.h"
#include "time.h"
#include <random>
#include <thread>

#include "tiny_jpeg.h"

using namespace std;

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
#define MAX(a, b) ( ((a) > (b)) ? (a) : (b))
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b))
#define CLAMP(x, lo, hi) (MAX(MIN((x), (hi)), (lo)))
#define LERP(a, b, alpha) ((1.0f - (alpha)) * (a) + (alpha) * (b))
vec3 lerp(const vec3& a, const vec3& b, float alpha){
    return vec3(
        LERP(a.x, b.x, alpha),
        LERP(a.y, b.y, alpha),
        LERP(a.z, b.z, alpha)
    );
}
vec3 clamp(const vec3& x, float lo, float hi){
    return vec3(
        CLAMP(x.x, lo, hi),
        CLAMP(x.y, lo, hi),
        CLAMP(x.z, lo, hi)
    );
}
vec3 pow(const vec3& v, float e){
    return vec3(
        powf(v.x, e),
        powf(v.y, e),
        powf(v.z, e)
    );
}
vec3 pow(const vec3& v, const vec3& e){
    return vec3(
        powf(v.x, e.x),
        powf(v.y, e.y),
        powf(v.z, e.z)
    );
}
vec3 abs(const vec3& v){
    return vec3(
        fabs(v.x),
        fabs(v.y),
        fabs(v.z)
    );
}
float vmax(const vec3& v){
    return MAX(MAX(v.x, v.y), v.z);
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

struct Material{
    vec3 reflectance;
    vec3 emittance;
};

struct Ray{
    vec3 origin;
    vec3 direction;
};

struct MapSample{
    Material* material;
    float distance;
    bool operator < (const MapSample& other){
        return distance < other.distance;
    }
};

float operator - (const MapSample& a, const MapSample& b){
    return a.distance - b.distance;
}

static Material materials[] = {
    {vec3(0.5f, 0.5f, 0.5f), vec3(0.001f, 0.001f, 0.001f)}, // white
    {vec3(0.5f, 0.0f, 0.0f), vec3(0.001f, 0.0f, 0.0f)}, // red
    {vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 0.001f, 0.0f)}, // green
    {vec3(0.0f, 0.0f, 0.5f), vec3(0.0f, 0.0f, 0.001f)}, // blue
    {vec3(0.0f, 0.0f, 0.0f), vec3(0.5f, 0.5f, 0.5f)} // white light
};

MapSample sphere(const vec3& ray, const vec3& location, float radius, int mat){
    return {&materials[mat], distance(ray, location) - radius};
}

MapSample box(const vec3& ray, const vec3& location, const vec3& dimension, int mat){
    vec3 d = abs(ray - location) - dimension;
    return {&materials[mat], vmax(d)};
}

MapSample map(const vec3& ray){
    MapSample a = sphere(ray, vec3(0.5f, 0.0f, -1.0f), 0.5f, 1);
    a = MIN(a, sphere(ray, vec3(-0.5f, 0.0f, -1.0f), 0.5f, 2));
    a = MIN(a, box(ray, // light
        vec3(0.0f, 1.0f, 0.0f),
        vec3(0.5f, 0.01f, 0.5f), 
        4));
    a = MIN(a, box(ray, // left wall
        vec3(-2.0f, 0.0f, 0.0f),
        vec3(0.01f, 2.0f, 2.0f),
        0));
    a = MIN(a, box(ray, // right wall
        vec3(2.0f, 0.0f, 0.0f),
        vec3(0.01f, 2.0f, 2.0f),
        1));
    a = MIN(a, box(ray, // ceiling
        vec3(0.0f, 2.0f, 0.0f),
        vec3(2.0f, 0.01f, 2.0f),
        2));
    a = MIN(a, box(ray, // floor
        vec3(0.0f, -2.0f, 0.0f),
        vec3(2.0f, 0.01f, 2.0f),
        3));
    a = MIN(a, box(ray, // back
        vec3(0.0f, 0.0f, -2.0f),
        vec3(2.0f, 2.0f, 0.01f),
        0));
    a = MIN(a, box(ray, // front
        vec3(0.0f, 0.0f, 2.0f),
        vec3(2.0f, 2.0f, 0.01f),
        1));
    return a;
}

constexpr float norm_epsilon = 0.001f;
static const vec3 xe(norm_epsilon, 0.0f, 0.0f);
static const vec3 ye(0.0f, norm_epsilon, 0.0f);
static const vec3 ze(0.0f, 0.0f, norm_epsilon);

vec3 map_normal(const vec3& point){
    return normalize(
        vec3(
            map(point + xe) - map(point - xe),
            map(point + ye) - map(point - ye),
            map(point + ze) - map(point - ze)
        )
    );
}

static const float invRandMax = 2.0f / RAND_MAX;

vec3 randomDir(const vec3& N){
    vec3 dir;
    do{
        dir.x = rand() * invRandMax - 1.0f;
        dir.y = rand() * invRandMax - 1.0f;
        dir.z = rand() * invRandMax - 1.0f;
        dir = normalize(dir);
    }while(dot(dir, N) < 0.0f);
    return dir;
}

//https://en.wikipedia.org/wiki/Path_tracing
vec3 trace(const Ray& ray, int depth){
    if(depth >= 10)return vec3();
    constexpr float e = 0.001f;
    bool hit = false;
    vec3 point = ray.origin;
    MapSample sample;
    
    while(true){
        sample = map(point);
        if(fabs(sample.distance) < e){
            hit = true;
            break;
        }
        if(sample.distance > 30.0f)
            break;
        point = point + ray.direction * fabs(sample.distance);
    }
    
    if(!hit)return vec3();
    
    const vec3 N = map_normal(point);
    Ray r2 = {point + N * e, randomDir(N)};
    float cos_theta = dot(N, r2.direction);
    vec3 brdf = 2.0f * sample.material->reflectance * cos_theta;
    vec3 reflected = trace(r2, depth + 1);
    
    return sample.material->emittance + (brdf * reflected);
}

vec3 do_ray(const Ray& ray){
    vec3 out;
    
    for(size_t i = 0; i < 30; i++){
        out = (out + trace(ray, 0)) * 0.5f;
    }
    
    return clamp(pow(out, 1.0f / 2.2f), 0.0, 1.0f);
}

int main(){
    srand(time(NULL));
    const int32_t width = 1024, height = 1024;
    const float invw2 = 2.0f / width;
    const float invh2 = 2.0f / height;
    uint8_t* data = new uint8_t[width * height * 3];
    vec3* image = new vec3[width * height];
    vec3 eye(0.0f, 0.0f, 1.0f);
    
    auto work = [width, invw2, invh2, eye, image](int begin, int end){
        for(int i = begin; i < end; i++){
            int x = i % width;
            int y = i / width;
            const float xf = x * invw2 - 1.0f;
            const float yf = -1.0f * (y * invh2 - 1.0f);
            vec3 origin(xf, yf, 0.0f);
            image[i] = do_ray({origin, normalize(origin - eye)});
        }
    };
    
    vector<thread> threads;
    int num_threads = 8;
    int N = (width * height) / num_threads;
    for(int i = 0; i < num_threads; i++){
        int begin = i * N;
        int end = begin + N;
        threads.push_back(thread(work, begin, end));
    }
    
    for(auto& t : threads)
        t.join();
    
    for(size_t i = 0; i < width * height; i++){
        data[i * 3 + 0] = image[i].x * 255;
        data[i * 3 + 1] = image[i].y * 255;
        data[i * 3 + 2] = image[i].z * 255;
    }
    
    if(!tje_encode_to_file("out.jpg", width, height, 3, data)){
        puts("encoding failed");
        return 1;
    }
    
    delete[] data;
    delete[] image;
    
    return 0;
}