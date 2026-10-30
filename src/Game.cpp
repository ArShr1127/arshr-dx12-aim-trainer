#include "Game.h"
#include <SDL.h>
#include <iostream>

Game::Game(int width, int height)
    : window(nullptr), renderer(nullptr),
    running(false), modeMgr(width, height),
    score(0), hits(0), shots(0)
{
    // 初始化SDL2视频和音频子系统
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    window = SDL_CreateWindow("2D练枪游戏",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // 初始化SDL_mixer播放音效（如需）
    // Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);
}

Game::~Game() {
    // 释放资源
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Game::run() {
    running = true;
    timer = Timer();  // 重置计时
    while (running) {
        float dt = timer.deltaTime();  // 获取帧间隔（秒）
        // 处理输入事件
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            input.handleEvent(e);
        }
        if (input.isQuit()) { running = false; break; }

        // 更新当前模式逻辑
        modeMgr.update(dt);

        // 处理鼠标点击命中
        if (input.isMouseClicked()) {
            int mx, my;
            input.getMousePos(mx, my);
            shots++;
            if (modeMgr.handleClick(mx, my)) {
                hits++;
                score++;  // 每击中1分，可自定义计分
                // 播放击中音效示例: Mix_PlayChannel(-1, hitSound, 0);
            }
            input.reset();  // 重置点击状态
        }

        // 渲染当前帧
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        modeMgr.render(renderer);
        SDL_RenderPresent(renderer);

        // 限制帧率 (大约60 FPS)
        SDL_Delay(16);
    }
    std::cout << "游戏结束！得分：" << score
        << "，命中率：" << (shots > 0 ? hits * 100.0f / shots : 0.0f) << "%\n";
}
