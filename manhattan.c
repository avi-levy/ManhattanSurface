/* (c) 2015 Avi Levy
 *
 * This shader renders a Manhattan surface,
 *      aka the 3d Koch Cube
 *      aka the 3d quadratic Koch surface (Type 1)
 *
 * Motivation:
 *      The Manhattan surface is homeomorphic to
 *      a 2-sphere, yet has a fractal dimension of
 *      log(13)/log(3) = 2.33.
 *
 *      This surface is constructed by gluing together
 *      many small quadrilaterals. More generally, one
 *      may glue together quadrilaterals in a random
 *      manner to produce a random surface. Like the
 *      Manhattan surface, random surfaces are also
 *      homeomorphic to the 2-sphere yet are higher
 *      dimensional: they have dimension 4 almost surely.
 *
 *      Random surfaces constructed in such a manner
 *      have been studied in relation with quantum
 *      gravity. This topic is being studied in:
 *      
 *      Special Topics Course MATH 583 E
 *      University of Washington
 *      Instructors: Steffen Rohde and Brent Werness
 *      Course Webpage:
 *      http://www.math.washington.edu/~bwerness/teaching.html
 *
 * Changelog:
 *      ============ May 9
 *      Fixes due to Inigo Quilez:
 *          * missing a return value in the
 *              intersect() function under
 *              a certain codepath.
 *          * faster sort() function
 *      ============ May 6
 *      Complete rewrite of the mapping function.
 *      Performance boost from 3fps to 60fps on
 *      the iteration 2 of the fractal.
 *      Speedup acheived using tesselation and
 *      cubic symmetry.
 *      ============ May 5
 *      Incorporated Brent's observation that this
 *      was not a Manhattan surface due to not
 *      having enough cubies.
 *
 * Shader based on code by inigo quilez - iq/2013
 */

float map(in vec3 p);
vec4 colorize(float f);

// raymarching algorithm
float intersect(in vec3 ro, in vec3 rd) {
    float t = 0.0;
    for(int i = 0; i < 1000; i++) {
        if(t >= 10.0) {
            return -1.;
        }
        float h = map(ro + rd*t);
        if(h < 0.01) {
            return t;
        }
        t += h;
    }
    return -1.;
}

vec3 light = normalize(vec3(1., .9, .3));

float blend(float t, float value) {
    return max(0., t + (1. - t) * value);
}

float softshadow(in vec3 ro, in vec3 rd, float mint, float k) {
    float res = 1.0;
    float t = mint;
    float h = 1.0;
    for(int i = 0; i < 32; i++) {
        h = map(ro + rd*t);
        res = min(res, k*h/t);
        t += clamp(h, 0.005, 0.1);
    }
    res = clamp(res, 0., 1.);
    return blend(.1, res);
}

vec3 calcNormal(in vec3 pos) {
    vec3  eps = vec3(.001,0.0,0.0);
    vec3 nor;
    nor.x = map(pos+eps.xyy) - map(pos-eps.xyy);
    nor.y = map(pos+eps.yxy) - map(pos-eps.yxy);
    nor.z = map(pos+eps.yyx) - map(pos-eps.yyx);
    return normalize(nor);
}

vec3 render(in vec3 ro, in vec3 rd) {
    // background color
    vec3 color = mix(vec3(.3, .2, .1) * .5, vec3(.7, .9, 1.), blend(.5, rd.y));
    
    vec4 tmat = colorize(intersect(ro,rd));
    if(tmat.x > 0.) {
        vec3  pos = ro + tmat.x*rd;
        vec3  nor = calcNormal(pos);
        
        float occlusion = tmat.y;
        float incident = dot(nor, light);

        vec3 lin  = vec3(0.0);
        lin += 1.00 * blend(.1, incident) * softshadow(pos, light, 0.01, 64.0) * vec3(1.1, .85, .6);
        lin += 0.50 * blend(.5, nor.y) * occlusion * vec3(.1, .2, .4);
        lin += 0.50 * blend(.4, -incident) * blend(.5, occlusion) * vec3(1);
        lin += 0.25 * occlusion * vec3(.15, .17, .2);

        vec3 matcol = vec3(
            0.5+0.5*cos(0.0+2.0*tmat.z),
            0.5+0.5*cos(1.0+2.0*tmat.z),
            0.5+0.5*cos(2.0+2.0*tmat.z)
        );
        color = matcol * lin;
    }

    return pow(color, vec3(0.4545));
}

vec3 rotate(float t) {
    vec3 p = vec3(0., 1., 0.);
    vec3 q = vec3(2.5, 1.0, 2.5);
    vec3 r = vec3(0.25, 0.13, 0.25);
    return p + vec3(
        q.x * sin(r.x * t),
        q.y * cos(r.y * t),
        q.z * cos(r.z * t)
    );
}

vec4 colorize(float f) {
    if(f == -1.) {
        return vec4(-1., -1., -1., 1.);
    } else {
        // length, 0, material, 0
        return vec4(abs(f), 0., .25, 0.);
    }
}

void mainImage(out vec4 color, in vec2 position) {
    vec2 p = -1.0 + 2.0 * position.xy / iResolution.xy;
    p.x *= 1.33;

    // camera
    vec3 ro = 1.1 * rotate(iGlobalTime * 1.);
    vec3 ww = normalize(vec3(0.0) - ro);
    vec3 uu = normalize(cross( vec3(0.0,1.0,0.0), ww ));
    vec3 vv = normalize(cross(ww,uu));
    vec3 rd = normalize(p.x*uu + p.y*vv + 2.5*ww);
    
    color = vec4(render(ro, rd), 1.);
}

float max(in vec3 p) {
    return max(p.x,max(p.y,p.z));
}

float rect(vec3 p, vec3 center, vec3 radius) {
    return length(max(abs(p - center) - radius, 0.));
}

vec3 tesselate(vec3 p, vec3 tile) {
    return mod(p - .5 * tile, tile) - .5 * tile;
}

vec3 sort(in vec3 p) {
    float mi = min(min(p.x, p.y), p.z);
    float ma = max(max(p.x, p.y), p.z);
    return vec3(mi, p.x + p.y + p.z - mi - ma, ma);
}

float scale = .7;

float map(in vec3 p) {
    p = abs(p)/scale;
    p = sort(p);
    
    float s;

    float r = rect(p, vec3(0), vec3(1));

    s = rect(p, vec3(0.,0.,4./3.), vec3(1./3.));
    r = min(r,s);

    vec3 t;
    t = tesselate(p, vec3(2./3.,2./3.,0.));
    s = rect(t, vec3(0.,0.,10./9.), vec3(1./9.));
    s = max(s, rect(p, vec3(0), vec3(1.,1.,2.)));
    r = min(r,s);

    s = rect(p, vec3(0.,4./9.,4./3.), vec3(1./9.));
    r = min(r,s);

    s = rect(p, vec3(0.,0.,16./9.), vec3(1./9.));
    r = min(r,s);
    return scale * r;
}