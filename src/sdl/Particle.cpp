#include "sdl/Particle.h"
#include <cmath>
#include <cstdlib>

static SDL_Renderer* s_r   = nullptr;
static SDL_Texture*  s_dot = nullptr;

struct Particle {
    float  x, y, vx, vy, grav;
    float  life, maxLife;
    float  size0, size1;
    Uint8  cr, cg, cb;
    bool   additive;
    bool   alive;
};

static const int MAXP = 700;
static Particle   s_pool[MAXP];
static int        s_next = 0;
static bool       s_enabled = true;

static float frand(float a, float b) { return a + (b - a) * (rand() / (float)RAND_MAX); }

static Particle* Alloc() {
    for (int i = 0; i < MAXP; i++) {
        int j = (s_next + i) % MAXP;
        if (!s_pool[j].alive) { s_next = (j + 1) % MAXP; return &s_pool[j]; }
    }
    Particle* p = &s_pool[s_next];
    s_next = (s_next + 1) % MAXP;
    return p;
}

void Particle_Init(SDL_Renderer* renderer) {
    s_r = renderer;
    for (auto& p : s_pool) p.alive = false;

    const int D = 32;
    SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, D, D, 32, SDL_PIXELFORMAT_RGBA32);
    if (surf) {
        Uint32* px = (Uint32*)surf->pixels;
        for (int y = 0; y < D; y++)
            for (int x = 0; x < D; x++) {
                float dx = (x - D / 2 + 0.5f) / (D / 2.0f);
                float dy = (y - D / 2 + 0.5f) / (D / 2.0f);
                float d  = std::sqrt(dx * dx + dy * dy);
                float a  = (d >= 1.0f) ? 0.0f : (1.0f - d);
                a = a * a;
                px[y * D + x] = SDL_MapRGBA(surf->format, 255, 255, 255, (Uint8)(a * 255));
            }
        s_dot = SDL_CreateTextureFromSurface(renderer, surf);
        SDL_FreeSurface(surf);
        if (s_dot) SDL_SetTextureBlendMode(s_dot, SDL_BLENDMODE_BLEND);
    }
}

void Particle_Shutdown() {
    if (s_dot) { SDL_DestroyTexture(s_dot); s_dot = nullptr; }
    s_r = nullptr;
}

void Particle_Clear() { for (auto& p : s_pool) p.alive = false; }

void Particle_SetEnabled(bool on) {
    s_enabled = on;
    if (!s_enabled) Particle_Clear();
}

bool Particle_IsEnabled() { return s_enabled; }

void Particle_Update(float dt) {
    if (dt > 0.05f) dt = 0.05f;
    for (auto& p : s_pool) {
        if (!p.alive) continue;
        p.vy += p.grav * dt;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.life -= dt;
        if (p.life <= 0.0f) p.alive = false;
    }
}

void Particle_Render(SDL_Renderer* renderer) {
    if (!s_dot) return;
    for (auto& p : s_pool) {
        if (!p.alive) continue;
        float f  = p.life / p.maxLife;
        float sz = p.size1 + (p.size0 - p.size1) * f;
        Uint8 a  = (Uint8)(225.0f * f);
        SDL_SetTextureBlendMode(s_dot, p.additive ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
        SDL_SetTextureColorMod(s_dot, p.cr, p.cg, p.cb);
        SDL_SetTextureAlphaMod(s_dot, a);
        SDL_Rect d = { (int)(p.x - sz / 2), (int)(p.y - sz / 2), (int)sz, (int)sz };
        SDL_RenderCopy(renderer, s_dot, nullptr, &d);
    }

    SDL_SetTextureColorMod(s_dot, 255, 255, 255);
    SDL_SetTextureAlphaMod(s_dot, 255);
    SDL_SetTextureBlendMode(s_dot, SDL_BLENDMODE_BLEND);
}

void Particle_BurstDirt(float x, float y) {
    if (!s_enabled) return;
    int n = 14 + rand() % 8;
    for (int i = 0; i < n; i++) {
        Particle* p = Alloc(); p->alive = true;
        float ang = frand(3.14159f, 6.28318f);
        float spd = frand(50.0f, 170.0f);
        p->x = x + frand(-6, 6);  p->y = y + frand(-2, 8);
        p->vx = std::cos(ang) * spd;
        p->vy = std::sin(ang) * spd - frand(30, 90);
        p->grav   = frand(420, 600);
        p->maxLife = p->life = frand(0.40f, 0.68f);
        p->size0 = frand(12, 22); p->size1 = frand(4, 8);
        int t = rand() % 3;
        if      (t == 0) { p->cr = 184; p->cg = 150; p->cb = 100; }
        else if (t == 1) { p->cr = 158; p->cg = 122; p->cb = 78;  }
        else             { p->cr = 206; p->cg = 176; p->cb = 126; }
        p->additive = false;
    }
}

void Particle_ShimmerAt(float x, float y, int faction) {
    if (!s_enabled) return;
    Particle* p = Alloc(); p->alive = true;
    p->x = x + frand(-14, 14); p->y = y + frand(-8, 8);
    p->vx = frand(-12, 12); p->vy = frand(-34, -16);
    p->grav   = frand(-8, 12);
    p->maxLife = p->life = frand(0.5f, 0.9f);
    p->size0 = frand(7, 13); p->size1 = frand(1, 3);
    if (faction == 1) { p->cr = 180; p->cg = 214; p->cb = 255; }
    else              { p->cr = 255; p->cg = 236; p->cb = 172; }
    p->additive = true;
}

void Particle_BurstHarvest(float x, float y, int faction) {
    if (!s_enabled) return;
    int nl = 5 + rand() % 3;
    for (int i = 0; i < nl; i++) {
        Particle* p = Alloc(); p->alive = true;
        p->x = x + frand(-12, 12); p->y = y + frand(-12, 8);
        p->vx = frand(-30, 30); p->vy = frand(-60, -20);
        p->grav   = frand(120, 200);
        p->maxLife = p->life = frand(0.9f, 1.5f);
        p->size0 = frand(8, 14); p->size1 = frand(6, 10);
        if (rand() % 2) { p->cr = 96;  p->cg = 150; p->cb = 64; }
        else            { p->cr = 124; p->cg = 176; p->cb = 82; }
        p->additive = false;
    }
    int ns = 14 + rand() % 6;
    Uint8 sr = (faction == 1) ? 180 : 255;
    Uint8 sg = (faction == 1) ? 214 : 236;
    Uint8 sb = (faction == 1) ? 255 : 172;
    for (int i = 0; i < ns; i++) {
        Particle* p = Alloc(); p->alive = true;
        p->x = x + frand(-16, 16); p->y = y + frand(-16, 16);
        p->vx = frand(-60, 60); p->vy = frand(-120, -40);
        p->grav   = frand(-10, 60);
        p->maxLife = p->life = frand(0.55f, 1.05f);
        p->size0 = frand(10, 20); p->size1 = frand(2, 5);
        p->cr = sr; p->cg = sg; p->cb = sb;
        p->additive = true;
    }
}
