#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "blur.hpp"
#include "imgui_internal.h"

#include <Geode/Geode.hpp>
#include <cocos2d.h>

#include <vector>
#include <cstdint>
#include <algorithm>

using namespace cocos2d;

namespace {

struct BlurRegion {
    float x0;
    float y0;
    float x1;
    float y1;
    float rounding;
    float radius;
    float alpha;
};

constexpr size_t kMaxRegions = 16;

std::vector<BlurRegion> g_regions;

GLuint g_texture = 0;
bool   g_failed  = false;

struct BlurProgram {
    CCGLProgram* program = nullptr;
    GLint locTex      = -1;
    GLint locHalfpixel = -1;
    GLint locAlpha    = -1;
    GLint locRound    = -1;
    GLint locSize     = -1;
    GLint locMask     = -1;
};

BlurProgram g_down;
BlurProgram g_up;

const char* kVertexShader =
    "attribute vec2 a_position;\n"
    "attribute vec2 a_texCoord;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
    "    gl_Position = vec4(a_position, 0.0, 1.0);\n"
    "    v_uv = a_texCoord;\n"
    "}\n";

const char* kDownsampleShader =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_halfpixel;\n"
    "void main() {\n"
    "    vec4 sum = texture2D(u_tex, v_uv) * 4.0;\n"
    "    sum += texture2D(u_tex, v_uv - u_halfpixel);\n"
    "    sum += texture2D(u_tex, v_uv + u_halfpixel);\n"
    "    sum += texture2D(u_tex, v_uv + vec2(u_halfpixel.x, -u_halfpixel.y));\n"
    "    sum += texture2D(u_tex, v_uv - vec2(u_halfpixel.x, -u_halfpixel.y));\n"
    "    gl_FragColor = sum / 8.0;\n"
    "}\n";

const char* kUpsampleShader =
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec2 v_uv;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 u_halfpixel;\n"
    "uniform float u_alpha;\n"
    "uniform float u_round;\n"
    "uniform vec2 u_size;\n"
    "uniform float u_mask;\n"
    "void main() {\n"
    "    vec4 sum = texture2D(u_tex, v_uv + vec2(-u_halfpixel.x * 2.0, 0.0));\n"
    "    sum += texture2D(u_tex, v_uv + vec2(-u_halfpixel.x, u_halfpixel.y)) * 2.0;\n"
    "    sum += texture2D(u_tex, v_uv + vec2(0.0, u_halfpixel.y * 2.0));\n"
    "    sum += texture2D(u_tex, v_uv + vec2(u_halfpixel.x, u_halfpixel.y)) * 2.0;\n"
    "    sum += texture2D(u_tex, v_uv + vec2(u_halfpixel.x * 2.0, 0.0));\n"
    "    sum += texture2D(u_tex, v_uv + vec2(u_halfpixel.x, -u_halfpixel.y)) * 2.0;\n"
    "    sum += texture2D(u_tex, v_uv + vec2(0.0, -u_halfpixel.y * 2.0));\n"
    "    sum += texture2D(u_tex, v_uv + vec2(-u_halfpixel.x, -u_halfpixel.y)) * 2.0;\n"
    "    vec3 col = (sum / 12.0).rgb;\n"
    "    float mask = 1.0;\n"
    "    if (u_mask > 0.5) {\n"
    "        vec2 halfSize = u_size * 0.5;\n"
    "        vec2 p = abs((v_uv - vec2(0.5)) * u_size) - (halfSize - vec2(u_round));\n"
    "        float d = length(max(p, vec2(0.0))) + min(max(p.x, p.y), 0.0) - u_round;\n"
    "        mask = 1.0 - smoothstep(-1.0, 1.0, d);\n"
    "    }\n"
    "    gl_FragColor = vec4(col, u_alpha * mask);\n"
    "}\n";

bool compileProgram(BlurProgram& out, const char* fragSrc, bool isUpsample) {
    CCGLProgram* program = new CCGLProgram();

    if (!program->initWithVertexShaderByteArray(kVertexShader, fragSrc)) {
        delete program;
        return false;
    }

    program->addAttribute("a_position", kCCVertexAttrib_Position);
    program->addAttribute("a_texCoord", kCCVertexAttrib_TexCoords);
    program->link();
    program->updateUniforms();

    const GLuint handle = program->getProgram();

    out.program      = program;
    out.locTex       = glGetUniformLocation(handle, "u_tex");
    out.locHalfpixel = glGetUniformLocation(handle, "u_halfpixel");

    if (isUpsample) {
        out.locAlpha = glGetUniformLocation(handle, "u_alpha");
        out.locRound = glGetUniformLocation(handle, "u_round");
        out.locSize  = glGetUniformLocation(handle, "u_size");
        out.locMask  = glGetUniformLocation(handle, "u_mask");
    }

    return true;
}

bool ensurePrograms() {
    if (g_failed)
        return false;

    if (g_down.program && g_up.program)
        return true;

    if (!compileProgram(g_down, kDownsampleShader, false) ||
        !compileProgram(g_up, kUpsampleShader, true)) {
        g_failed = true;
        geode::log::warn("blur: shader compilation failed, blur disabled");
        return false;
    }

    return true;
}

bool ensureTexture() {
    if (g_texture)
        return true;

    glGenTextures(1, &g_texture);
    if (!g_texture)
        return false;

    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

void renderRegion(const ImDrawList*, const ImDrawCmd* cmd) {
    if (!cmd)
        return;

    const size_t index = (size_t)(intptr_t)cmd->UserCallbackData;
    if (index >= g_regions.size())
        return;

    if (!ensurePrograms() || !ensureTexture())
        return;

    const BlurRegion region = g_regions[index];

    GLint viewport[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_VIEWPORT, viewport);

    const int fbW = viewport[2];
    const int fbH = viewport[3];
    if (fbW <= 0 || fbH <= 0)
        return;

    ImGuiIO& io = ImGui::GetIO();

    const float scaleX = io.DisplaySize.x > 0.f ? (float)fbW / io.DisplaySize.x : 1.f;
    const float scaleY = io.DisplaySize.y > 0.f ? (float)fbH / io.DisplaySize.y : 1.f;

    int px0 = (int)(region.x0 * scaleX);
    int py1 = (int)(region.y1 * scaleY);
    int pw  = (int)((region.x1 - region.x0) * scaleX);
    int ph  = (int)((region.y1 - region.y0) * scaleY);

    if (pw <= 2 || ph <= 2)
        return;

    int py0 = fbH - py1;

    if (px0 < 0) { pw += px0; px0 = 0; }
    if (py0 < 0) { ph += py0; py0 = 0; }
    if (px0 + pw > fbW) pw = fbW - px0;
    if (py0 + ph > fbH) ph = fbH - py0;

    if (pw <= 2 || ph <= 2)
        return;

    GLint previousTexture = 0;
    GLint previousProgram = 0;
    GLint previousArray   = 0;
    GLint previousVao     = 0;
    GLint previousBlendSrc = GL_SRC_ALPHA;
    GLint previousBlendDst = GL_ONE_MINUS_SRC_ALPHA;

    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &previousArray);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &previousVao);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &previousBlendSrc);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &previousBlendDst);

    const GLboolean previousScissor = glIsEnabled(GL_SCISSOR_TEST);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    ccGLEnableVertexAttribs(kCCVertexAttribFlag_Position | kCCVertexAttribFlag_TexCoords);

    const GLfloat coords[8] = {
        0.f, 0.f,
        1.f, 0.f,
        0.f, 1.f,
        1.f, 1.f
    };
    glVertexAttribPointer(kCCVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, 0, coords);

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);

    auto drawRect = [&](int w, int h) {
        const float ndcX0 = ((float)px0 / (float)fbW) * 2.f - 1.f;
        const float ndcX1 = ((float)(px0 + w) / (float)fbW) * 2.f - 1.f;
        const float ndcY0 = ((float)py0 / (float)fbH) * 2.f - 1.f;
        const float ndcY1 = ((float)(py0 + h) / (float)fbH) * 2.f - 1.f;

        const GLfloat vertices[8] = {
            ndcX0, ndcY0,
            ndcX1, ndcY0,
            ndcX0, ndcY1,
            ndcX1, ndcY1
        };

        glVertexAttribPointer(kCCVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    };

    glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, px0, py0, pw, ph, 0);

    int levels = 2 + (int)(region.radius * 0.3f);
    levels = ImClamp(levels, 2, 3);
    while (levels > 1 && (std::min(pw, ph) >> levels) < 8)
        levels--;

    int curW = pw;
    int curH = ph;

    glBlendFunc(GL_ONE, GL_ZERO);

    for (int i = 0; i < levels; i++) {
        const int nextW = std::max(curW / 2, 4);
        const int nextH = std::max(curH / 2, 4);

        g_down.program->use();
        if (g_down.locTex >= 0) glUniform1i(g_down.locTex, 0);
        if (g_down.locHalfpixel >= 0)
            glUniform2f(g_down.locHalfpixel, 0.5f / (float)curW, 0.5f / (float)curH);

        drawRect(nextW, nextH);
        glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, px0, py0, nextW, nextH, 0);

        curW = nextW;
        curH = nextH;
    }

    for (int i = 0; i < levels; i++) {
        const bool last = i == levels - 1;
        const int  nextW = last ? pw : std::min(curW * 2, pw);
        const int  nextH = last ? ph : std::min(curH * 2, ph);

        g_up.program->use();
        if (g_up.locTex >= 0) glUniform1i(g_up.locTex, 0);
        if (g_up.locHalfpixel >= 0)
            glUniform2f(g_up.locHalfpixel, 0.5f / (float)curW, 0.5f / (float)curH);
        if (g_up.locAlpha >= 0) glUniform1f(g_up.locAlpha, last ? region.alpha : 1.f);
        if (g_up.locRound >= 0) glUniform1f(g_up.locRound, region.rounding * scaleX);
        if (g_up.locSize  >= 0) glUniform2f(g_up.locSize, (float)pw, (float)ph);
        if (g_up.locMask  >= 0) glUniform1f(g_up.locMask, last ? 1.f : 0.f);

        glBlendFunc(last ? GL_SRC_ALPHA : GL_ONE, last ? GL_ONE_MINUS_SRC_ALPHA : GL_ZERO);

        drawRect(nextW, nextH);

        if (!last)
            glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, px0, py0, nextW, nextH, 0);

        curW = nextW;
        curH = nextH;
    }

    glBindTexture(GL_TEXTURE_2D, (GLuint)previousTexture);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)previousArray);
    glBindVertexArray((GLuint)previousVao);
    glUseProgram((GLuint)previousProgram);

    glBlendFunc((GLenum)previousBlendSrc, (GLenum)previousBlendDst);

    if (previousScissor)
        glEnable(GL_SCISSOR_TEST);
}

}

namespace nh::blur {

void newFrame() {
    g_regions.clear();
}

bool available() {
    return !g_failed;
}

void submit(ImDrawList* list, ImVec2 min, ImVec2 max, float rounding, float radius, float alpha) {
    if (!list || g_failed)
        return;

    if (alpha <= 0.01f)
        return;

    if (max.x - min.x <= 4.f || max.y - min.y <= 4.f)
        return;

    if (g_regions.size() >= kMaxRegions)
        return;

    g_regions.push_back(BlurRegion{ min.x, min.y, max.x, max.y, rounding, radius, alpha });

    list->AddCallback(renderRegion, (void*)(intptr_t)(g_regions.size() - 1));
}

}
