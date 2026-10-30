#ifndef GAME_H
#define GAME_H

#include <SDL.h>
#include "ModeManager.h"
#include "InputHandler.h"
#include "Timer.h"

class Game {
public:
    Game(int width = 1280, int height = 720);
    ~Game();
    void run();  // 游戏主循环

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    ModeManager modeMgr;
    InputHandler input;
    Timer timer;
    int score;      // 总得分
    int hits;       // 击中次数
    int shots;      // 总点击次数
};

#endif // GAME_H
