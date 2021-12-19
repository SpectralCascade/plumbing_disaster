#include <iostream>
#include <string.h>
#include <sstream>
#include <time.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

using namespace std;

int winW = 700;
int winH = 480;

const int BUTTON_W = 120;
const int BUTTON_H = 80;

const int BANNER_W = 300;
const int BANNER_H = 240;

SDL_Rect gWindowRect = {0, 0, 700, 480};

bool workingTime = true;

bool muteClicked = false;
bool muted = false;

SDL_Window* gWindow = NULL;
SDL_Renderer* gRenderer = NULL;
SDL_Surface* gWindowSurface = NULL;

// texture for pipes to point to and use
SDL_Texture* pipe_texture = NULL;

SDL_Texture* button_skip = NULL;
SDL_Texture* button_again = NULL;
SDL_Texture* button_pump = NULL;
SDL_Texture* button_mute = NULL;

SDL_Texture* banner_texture = NULL;

TTF_Font* gFont = NULL;
SDL_Texture* gTimeTexture = NULL;

Mix_Chunk *gTurnPipe = NULL;
Mix_Chunk *gButtonClick = NULL;
Mix_Chunk *gPumpClick = NULL;
Mix_Chunk *gWaterFlow = NULL;

enum PipeType
{
    PIPE_STRAIGHT = 0,
    PIPE_BEND,
    PIPE_START,
    PIPE_END,
    TOTAL_TYPES
};

enum PerpDirections
{
    UP = 0,
    RIGHT = 90,
    DOWN = 180,
    LEFT = 270
};

enum ButtonSelect
{
    BUTTON_E_SKIP = 0,
    BUTTON_E_AGAIN,
    BUTTON_E_PUMP,
    BUTTON_E_MUTE,
    TOTAL_BUTTONS
};

const int TOTAL_ROWS = 10;
const int TOTAL_COLUMNS = 10;

// pipe dimensions
int pipeDim = 48;
const int PIPE_CLIP_DIM = 48;

class TextureText
{
public:
    TextureText();
    ~TextureText();

    bool loadFromText(string text, SDL_Color textColor);

    void render(int x, int y);

    void freeTexture();

    int getWidth();
    int getHeight();

private:
    SDL_Texture* texture;

    int width;
    int height;
};

class Timer
{
public:
    Timer();
    ~Timer();

    void start();

    void stop();

    void pause();

    void resume();

    Uint32 getTicks();

    bool getPausedState();
    bool getStartedState();

private:
    Uint32 startTicks;
    Uint32 pausedTicks;

    bool paused;
    bool started;

};

class Button
{
public:
    Button(int ax, int ay, int w, int h, SDL_Texture* setTexture);
    ~Button();

    bool handleEvent(SDL_Event* e);

    void render();

private:
    SDL_Texture* texture;
    SDL_Rect clip;
    int x;
    int y;
    int width;
    int height;
};

class Banner
{
public:
    Banner();
    ~Banner();

    void setType(bool win);

    void render();

private:
    SDL_Texture* texture;
    SDL_Rect clip;
};

class Pipe
{
public:
    Pipe(int ax, int ay, int row, int column);
    ~Pipe();

    void setType(int set_type);

    void render();

    int getDirection();

    void setDirection(int newDirection);

    int getType();

    int getRow();

    int getColumn();

    void handleEvent(SDL_Event* e);

    void toggleFlow();

private:
    // points to the pipes texture
    SDL_Texture* texture;
    // from enum PipeTypes
    int type;
    // sets the sprite according to pipe type
    SDL_Rect clip;

    int direction;
    int x;
    int y;
    int mRow;
    int mColumn;

    bool clicked;
    bool flow;
};

class Pump
{
public:
    Pump();
    ~Pump();

    // Sets the current pipe to next pipe or returns false if there is no accessible pipe
    bool checkNextPipe();

    void setCurrentPipe(Pipe* setCurrent);

    void setEndPipe(Pipe* setEnd);

    void setPrevious(Pipe* setPrevious);

    bool reachedEnd();

    int getCurrentRow();
    int getCurrentColumn();
    int getPreviousRow();
    int getPreviousColumn();

private:
    Pipe* endPipe;
    Pipe* currentPipe;
    Pipe* nextPipe;

    int currentRow;
    int currentColumn;
    int previousRow;
    int previousColumn;

    bool finished;
};

Button* buttons[TOTAL_BUTTONS];

Banner* banner;

Pipe* pipes[TOTAL_ROWS][TOTAL_COLUMNS];
Pipe* pipeStart;
Pipe* pipeEnd;

Pump* pump;

int pipesClicked = 0;

TextureText gTimerText;

Timer gTimer;


TextureText::TextureText()
{
    texture = NULL;
    width = 0;
    height = 0;
}

bool TextureText::loadFromText(string text, SDL_Color textColor)
{
    freeTexture();
    SDL_Color trans = {0, 0, 0, 0};
    SDL_Surface* textSurface = TTF_RenderText(gFont, text.c_str(), textColor, trans);
    if (textSurface == NULL)
    {
        cout << "Error! Could not render text. TTF_Error: " << TTF_GetError() << endl;
    }
    else
    {
        texture = SDL_CreateTextureFromSurface(gRenderer, textSurface);
        if (texture == NULL)
        {
            cout << "Error! Unable to create texture from surface. SDL_Error: " << SDL_GetError() << endl;
        }
        else
        {
            width = textSurface->w;
            height = textSurface->h;
        }
        SDL_FreeSurface(textSurface);
    }
    return texture != NULL;
}

void TextureText::freeTexture()
{
    SDL_DestroyTexture(texture);
    texture = NULL;
}

void TextureText::render(int x, int y)
{
    SDL_Rect renderQuad = {x, y, width, height};
    SDL_Rect normalRect = {0, 0, width, height};
    SDL_RenderCopyEx(gRenderer, texture, &normalRect, &renderQuad, 0.0, NULL, SDL_FLIP_NONE);
}

int TextureText::getWidth()
{
    return width;
}

int TextureText::getHeight()
{
    return height;
}

TextureText::~TextureText()
{
    freeTexture();
    width = 0;
    height = 0;
}



Timer::Timer()
{
    startTicks = 0;
    pausedTicks = 0;
    paused = false;
    started = false;
}

void Timer::start()
{
    startTicks = SDL_GetTicks();
    started = true;
    paused = false;
    pausedTicks = 0;
}

void Timer::stop()
{
    started = false;
    paused = false;
    pausedTicks = 0;
    startTicks = 0;
}

void Timer::pause()
{
    if (started && !paused)
    {
        paused = true;
        pausedTicks = SDL_GetTicks() - startTicks;
        startTicks = 0;
    }
}

void Timer::resume()
{
    if (paused && started)
    {
        paused = false;
        startTicks = SDL_GetTicks() - pausedTicks;
        pausedTicks = 0;
    }
}

Uint32 Timer::getTicks()
{
    Uint32 time = 0;
    if (started)
    {
        if (paused)
        {
            time = pausedTicks;
        }
        else
        {
            time = SDL_GetTicks() - startTicks;
        }
    }
    return time;
}

bool Timer::getStartedState()
{
    return started;
}

bool Timer::getPausedState()
{
    return paused;
}

Timer::~Timer()
{
    started = false;
    paused = false;
    startTicks = 0;
    pausedTicks = 0;
}

Button::Button(int ax, int ay, int w, int h, SDL_Texture* setTexture)
{
    texture = setTexture;
    clip = {0, 0, w, h};
    x = ax;
    y = ay;
    width = w;
    height = h;
}

void Button::render()
{
    SDL_Rect renderQuad = {x, y, clip.w, clip.h};
    SDL_RenderCopyEx(gRenderer, texture, &clip, &renderQuad, 0.0, NULL, SDL_FLIP_NONE);
}

bool Button::handleEvent(SDL_Event* e)
{
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    if (mouseX > x && mouseX < x + clip.w && mouseY > y && mouseY < y + clip.h)
    {
        switch (e->type)
        {
        case SDL_MOUSEBUTTONDOWN:
            if (clip.w == BUTTON_W)
            {
                if (clip.y != 2 * height)
                {
                    clip.y = 2 * height;
                }
            }
            else
            {
                muteClicked = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (clip.w == BUTTON_W)
            {
                if (clip.y == (2 * height))
                {
                    clip.y = 0;
                    if (!muted)
                    {
                        Mix_PlayChannel(-1, gButtonClick, 0);
                    }
                    return true;
                }
            }
            else
            {
                if (muteClicked)
                {
                    muteClicked = false;
                    if (clip.y == 0)
                    {
                        clip.y = height;
                        muted = !muted;
                    }
                    else
                    {
                        clip.y = 0;
                        muted = !muted;
                    }
                    return true;
                }
            }
            break;
        }
        if (clip.y != 2 * BUTTON_H && clip.w == BUTTON_W)
        {
            clip.y = BUTTON_H;
        }
    }
    else if (BUTTON_W == width)
    {
        clip.y = 0;
    }
    else
    {
        muteClicked = false;
    }
    return false;
}

Button::~Button()
{
    texture = NULL;
    clip = {0, 0, 0, 0};
    x = 0;
    y = 0;
    width = 0;
    height = 0;
}

Banner::Banner()
{
    texture = banner_texture;
    clip = {0, 0, BANNER_W, BANNER_H};
}

void Banner::setType(bool win)
{
    if (!win)
    {
        clip.y = 0;
    }
    else
    {
        clip.y = BANNER_H;
    }
}

void Banner::render()
{
    SDL_Rect renderQuad = {((winW / 2) - (BANNER_W / 2)), ((winH / 2) - (BANNER_H / 2)), clip.w, clip.h};
    SDL_RenderCopyEx(gRenderer, texture, &clip, &renderQuad, 0.0, NULL, SDL_FLIP_NONE);
}

Banner::~Banner()
{
    texture = NULL;
    clip = {0, 0, 0, 0};
}


Pipe::Pipe(int ax, int ay, int row, int column)
{
    x = ax;
    y = ay;
    mRow = row;
    mColumn = column;
    texture = NULL;
    type = PIPE_STRAIGHT;
    clip = {0, 0, 0, 0};
    direction = UP;
    clicked = false;
    flow = false;
}

int Pipe::getDirection()
{
    return direction;
}

void Pipe::setType(int set_type)
{
    type = set_type;
    clip.w = PIPE_CLIP_DIM;
    clip.h = PIPE_CLIP_DIM;
    switch (type)
    {
    case PIPE_STRAIGHT:
        clip.x = 0;
        clip.y = 0;
        break;
    case PIPE_BEND:
        clip.x = 0;
        clip.y = PIPE_CLIP_DIM;
        break;
    case PIPE_START:
        clip.x = PIPE_CLIP_DIM;
        clip.y = 0;
        break;
    case PIPE_END:
        clip.x = PIPE_CLIP_DIM;
        clip.y = PIPE_CLIP_DIM;
        break;
    default:
        cout << "Warning: Invalid pipe type!" << endl;
        break;
    }
}

void Pipe::render()
{
    SDL_Rect renderQuad;
    renderQuad.x = x;
    renderQuad.y = y;
    renderQuad.w = pipeDim;
    renderQuad.h = pipeDim;
    SDL_RenderCopyEx(gRenderer, pipe_texture, &clip, &renderQuad, direction, NULL, SDL_FLIP_NONE);
}

void Pipe::handleEvent(SDL_Event* e)
{
    int mouseX, mouseY;
    SDL_MouseButtonEvent mouseE;
    SDL_GetMouseState(&mouseX, &mouseY);
    // check that mouse is touching pipe
    if (mouseX > x && mouseX < x + pipeDim && mouseY < y + pipeDim && mouseY > y)
    {
        switch (e->type)
        {
        case SDL_MOUSEBUTTONDOWN:
            clicked = true;
            break;
        case SDL_MOUSEBUTTONUP:
            if (clicked)
            {
                mouseE.button = e->button.button;
                if (mouseE.button == SDL_BUTTON_LEFT)
                {
                    if (direction != 270)
                    {
                        direction += 90;
                    }
                    else
                    {
                        direction = 0;
                    }
                }
                else if (mouseE.button == SDL_BUTTON_RIGHT)
                {
                    if (direction != 0)
                    {
                        direction -= 90;
                    }
                    else
                    {
                        direction = 270;
                    }
                }
                pipesClicked++;
                clicked = false;
                if (!muted)
                {
                    Mix_PlayChannel(-1, gTurnPipe, 0);
                }
            }
            break;
        }
    }
}

int Pipe::getType()
{
    return type;
}

int Pipe::getRow()
{
    return mRow;
}

int Pipe::getColumn()
{
    return mColumn;
}

void Pipe::setDirection(int newDirection)
{
    switch (newDirection)
    {
    case 0:
        direction = 0;
        break;
    case 2:
        direction = 180;
        break;
    case 1:
        direction = 270;
        break;
    case 3:
        direction = 90;
        break;
    }
}

void Pipe::toggleFlow()
{
    if (type == PIPE_STRAIGHT || type == PIPE_BEND)
    {
        flow = !flow;
        if (flow)
        {
            clip.y = PIPE_CLIP_DIM * 2;
            if (type == PIPE_BEND)
            {
                clip.x = PIPE_CLIP_DIM;
            }
        }
        else
        {
            clip.x = 0;
            clip.y = 0;
            if (type == PIPE_BEND)
            {
                clip.y = PIPE_CLIP_DIM;
            }
        }
    }
}

Pipe::~Pipe()
{
    texture = NULL;
    clip = {0, 0, 0, 0};
    x = 0;
    y = 0;
    mRow = 0;
    mColumn = 0;
    direction = 0;
}


Pump::Pump()
{
    currentPipe = NULL;
    endPipe = NULL;
    nextPipe = NULL;
    currentRow = 0;
    currentColumn = 0;
    previousRow = 0;
    previousColumn = 0;
    finished = false;
}

void Pump::setCurrentPipe(Pipe* setCurrent)
{
    currentPipe = setCurrent;
}

void Pump::setEndPipe(Pipe* setEnd)
{
    endPipe = setEnd;
}

bool Pump::reachedEnd()
{
    return finished;
}

int Pump::getCurrentRow()
{
    return currentRow;
}
int Pump::getCurrentColumn()
{
    return currentColumn;
}
int Pump::getPreviousRow()
{
    return previousRow;
}
int Pump::getPreviousColumn()
{
    return previousColumn;
}


bool Pump::checkNextPipe()
{
    bool ok = true;
    int type = currentPipe->getType();
    int dir = currentPipe->getDirection();
    currentRow = currentPipe->getRow();
    currentColumn = currentPipe->getColumn();
    switch (type)
    {
    case PIPE_STRAIGHT:
        if (dir == UP || dir == DOWN)
        {
            if (previousRow > currentRow)
            {
                if ((currentRow - 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                else if ((currentRow - 1) >= 0)
                {
                    nextPipe = pipes[currentRow - 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // pipe does not join with current pipe
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != RIGHT)
                        {
                            // ditto - no join
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                if ((currentRow + 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                if ((currentRow + 1) < TOTAL_ROWS)
                {
                    nextPipe = pipes[currentRow + 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // pipe does not join with current pipe
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != DOWN && nextPipe->getDirection() != LEFT)
                        {
                            // ditto - no join
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
        }
        else
        {
            // if last pipe was on a column to the right
            if (previousColumn > currentColumn)
            {
                // check that the next left column exists
                if ((currentColumn - 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                else if ((currentColumn - 1) >= 0)
                {
                    // next pipe is on the column to the left
                    nextPipe = pipes[currentRow][currentColumn - 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // pipe does not join with current pipe
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != LEFT)
                        {
                            // ditto - no join
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                // last pipe is on column to the left. Does the column to the right exist? Or perhaps is it the end?
                if ((currentColumn + 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                else if ((currentColumn + 1) < TOTAL_COLUMNS)
                {
                    // Yay! The column exists - so the next pipe is in that column
                    nextPipe = pipes[currentRow][currentColumn + 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // pipe does not join with current pipe
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != DOWN)
                        {
                            // ditto - no join
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
        }
        break;
    case PIPE_BEND:
        switch (dir)
        {
        case UP:
            if (previousRow > currentRow)
            {
                // last pipe was below
                if ((currentColumn + 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                if (currentColumn + 1 < TOTAL_COLUMNS)
                {
                    nextPipe = pipes[currentRow][currentColumn + 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != DOWN)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                // last pipe was to the right
                if ((currentRow + 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                else if (currentRow + 1 < TOTAL_ROWS)
                {
                    nextPipe = pipes[currentRow + 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != LEFT && nextPipe->getDirection() != DOWN)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            break;
        case DOWN:
            if (previousRow < currentRow)
            {
                // last pipe was above
                if ((currentColumn - 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                if (currentColumn - 1 >= 0)
                {
                    nextPipe = pipes[currentRow][currentColumn - 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != LEFT)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                // last pipe was to the left
                if ((currentRow - 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                if (currentRow - 1 >= 0)
                {
                    nextPipe = pipes[currentRow - 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != RIGHT)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            break;
        case LEFT:
            if (previousRow < currentRow)
            {
                // last pipe was above
                if ((currentColumn + 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                if (currentColumn + 1 < TOTAL_COLUMNS)
                {
                    nextPipe = pipes[currentRow][currentColumn + 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != DOWN)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                // last pipe was to the right
                if ((currentRow - 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                if (currentRow - 1 >= 0)
                {
                    nextPipe = pipes[currentRow - 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != RIGHT)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            break;
        case RIGHT:
            if (previousRow > currentRow)
            {
                // last pipe was below
                if ((currentColumn - 1) == pipeEnd->getColumn() && (currentRow == pipeEnd->getRow()))
                {
                    finished = true;
                    return false;
                }
                if (currentColumn - 1 >= 0)
                {
                    nextPipe = pipes[currentRow][currentColumn - 1];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != RIGHT && nextPipe->getDirection() != LEFT)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != LEFT)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            else
            {
                // last pipe was to the left
                if ((currentRow + 1) == pipeEnd->getRow() && (currentColumn == pipeEnd->getColumn()))
                {
                    finished = true;
                    return false;
                }
                if (currentRow + 1 < TOTAL_ROWS)
                {
                    nextPipe = pipes[currentRow + 1][currentColumn];
                    switch (nextPipe->getType())
                    {
                    case PIPE_STRAIGHT:
                        if (nextPipe->getDirection() != UP && nextPipe->getDirection() != DOWN)
                        {
                            // Ono! The pipes don't join
                            ok = false;
                        }
                        break;
                    case PIPE_BEND:
                        if (nextPipe->getDirection() != LEFT && nextPipe->getDirection() != DOWN)
                        {
                            ok = false;
                        }
                        break;
                    default:
                        break;
                    }
                }
                else
                {
                    ok = false;
                }
            }
            break;
        }
        break;
    }
    if (ok)
    {
        currentPipe = nextPipe;
    }
    previousRow = currentRow;
    previousColumn = currentColumn;
    return ok;
}

void Pump::setPrevious(Pipe* setPrevious)
{
    previousRow = setPrevious->getRow();
    previousColumn = setPrevious->getColumn();
}

Pump::~Pump()
{
    currentRow = 0;
    currentColumn = 0;
    currentPipe = NULL;
    previousColumn = 0;
    previousRow = 0;
    endPipe = NULL;
    nextPipe = NULL;
}

bool init()
{
    bool success = true;
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Landscape");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
    {
        cout << "Error initialising! SDL_Error: " << SDL_GetError() << endl;
        success = false;
    }
    else
    {
#ifdef __ANDROID__
        SDL_Rect screenBounds;
        int gotBounds = SDL_GetDisplayUsableBounds(0, &screenBounds) == 0;
        if (gotBounds) {
            winW = screenBounds.h;
            winH = screenBounds.w;
            SDL_Log("Win w = %d, win h = %d", winW, winH);
        }
#endif
        gWindow = SDL_CreateWindow("Plumbing Disaster", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, winW, winH, SDL_WINDOW_SHOWN);
        if (gWindow == NULL)
        {
            cout << "Error creating window! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        else
        {
#ifdef __ANDROID__
            SDL_GetWindowSize(gWindow, &winH, &winW);
            pipeDim = winH / 10;
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
            SDL_SetWindowFullscreen(gWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
#endif
            gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            if (gRenderer == NULL)
            {
                cout << "Error creating renderer! SDL_Error: " << SDL_GetError() << endl;
                success = false;
            }
            else
            {
                gWindowSurface = SDL_GetWindowSurface(gWindow);
                if (TTF_Init() == -1)
                {
                    cout << "Could not initialise TTF! TTF_Error: " << TTF_GetError() << endl;
                    success = false;
                }
                if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
                {
                    cout << "Warning: Could not initialise SDL_Mixer! Mix_Error: " << Mix_GetError() << endl;
                }
            }
        }
    }
    return success;
}

bool loadMedia()
{
    bool success = true;
    SDL_Surface* loadedSurface = NULL;
    loadedSurface = SDL_LoadBMP("Textures/pipes.bmp");
    if (loadedSurface == NULL)
    {
        cout << "Error loading image pipes.bmp! SDL_Error: " << SDL_GetError() << endl;
        success = false;
    }
    else
    {
        SDL_Texture* newTexture = NULL;
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        pipe_texture = newTexture;
        if (pipe_texture == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        loadedSurface = SDL_LoadBMP("Textures/button_skip.bmp");
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        button_skip = newTexture;
        if (button_skip == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        loadedSurface = SDL_LoadBMP("Textures/button_again.bmp");
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        button_again = newTexture;
        if (button_again == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        loadedSurface = SDL_LoadBMP("Textures/button_pump.bmp");
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        button_pump = newTexture;
        if (button_pump == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        loadedSurface = SDL_LoadBMP("Textures/banner.bmp");
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        banner_texture = newTexture;
        if (banner_texture == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        loadedSurface = SDL_LoadBMP("Textures/mute.bmp");
        newTexture = SDL_CreateTextureFromSurface(gRenderer, loadedSurface);
        SDL_FreeSurface(loadedSurface);
        loadedSurface = NULL;
        button_mute = newTexture;
        if (button_mute == NULL)
        {
            cout << "Failed to create texture from surface! SDL_Error: " << SDL_GetError() << endl;
            success = false;
        }
        buttons[BUTTON_E_SKIP] = new Button(winW - (10 + BUTTON_W), 16, BUTTON_W, BUTTON_H, button_skip);
        buttons[BUTTON_E_PUMP] = new Button(winW - (10 + BUTTON_W), winH - (BUTTON_H + 16), BUTTON_W, BUTTON_H, button_pump);
        buttons[BUTTON_E_AGAIN] = new Button((winW / 2) - (BUTTON_W / 2), winH / 2, BUTTON_W, BUTTON_H, button_again);
        buttons[BUTTON_E_MUTE] = new Button(winW - (BUTTON_W / 2), (winH / 3), 32, 32, button_mute);
        banner = new Banner();
    }
    gFont = TTF_OpenFont("Fonts/Carlito-Regular.ttf", 28);
    if (gFont == NULL)
    {
        cout << "Error! Unable to open font. TTF_Error: " << TTF_GetError() << endl;
    }
    gWaterFlow = Mix_LoadWAV("Sounds/flow.wav");
    gTurnPipe = Mix_LoadWAV("Sounds/turn.wav");
    gButtonClick = Mix_LoadWAV("Sounds/click.wav");
    if (gWaterFlow == NULL || gTurnPipe == NULL || gButtonClick == NULL)
    {
        cout << "Error loading one or more WAV files! Mix_Error: " << Mix_GetError() << endl;
    }
    return success;
}

void close()
{
    for (int row = 0; row < TOTAL_ROWS; row++)
    {
        for (int column = 0; column < TOTAL_COLUMNS; column++)
        {
            delete pipes[row][column];
        }
    }
    delete pump;
    delete pipeStart;
    delete pipeEnd;
    for (int i = 0; i < TOTAL_BUTTONS; i++)
    {
        delete buttons[i];
    }

    SDL_DestroyTexture(button_again);
    SDL_DestroyTexture(button_pump);
    SDL_DestroyTexture(button_skip);
    SDL_DestroyTexture(pipe_texture);
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);

    Mix_FreeChunk(gButtonClick);
    Mix_FreeChunk(gPumpClick);
    Mix_FreeChunk(gWaterFlow);
    Mix_FreeChunk(gTurnPipe);

    button_again = NULL;
    button_pump = NULL;
    button_skip = NULL;
    button_mute = NULL;
    pipe_texture = NULL;
    gWindow = NULL;
    gRenderer = NULL;

    TTF_Quit();
    Mix_Quit();
    SDL_Quit();
}

void newGame()
{
    gTimer.stop();
    srand((unsigned int)time(NULL));
    pipesClicked = 0;
    delete pump;
    pump = new Pump();
    for (int row = 0; row < TOTAL_ROWS; row++)
    {
        for (int column = 0; column < TOTAL_COLUMNS; column++)
        {
            int randType = rand() % 2;
            delete pipes[row][column];
            pipes[row][column] = new Pipe((column + 1) * pipeDim, row * pipeDim, row, column);
            pipes[row][column]->setType(randType);
            int randDir = rand() % 4;
            pipes[row][column]->setDirection(randDir);
        }
    }
    int startRow = rand() % TOTAL_ROWS;
    int endRow = rand() % TOTAL_ROWS;
    delete pipeStart;
    delete pipeEnd;
    pipeStart = new Pipe(0, startRow * pipeDim, startRow, -1);
    pipeStart->setType(PIPE_START);
    pipeEnd = new Pipe((TOTAL_COLUMNS + 1) * pipeDim, endRow * pipeDim, endRow, TOTAL_COLUMNS);
    pipeEnd->setType(PIPE_END);
    pump->setPrevious(pipeStart);
    pump->setCurrentPipe(pipes[startRow][0]);
    pump->setEndPipe(pipeEnd);
    gTimer.start();
}

void renderAll()
{
    SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_FillRect(gWindowSurface, &gWindowRect, 0x00);
    SDL_RenderClear(gRenderer);
    for (int row = 0; row < TOTAL_ROWS; row++)
    {
        for (int column = 0; column < TOTAL_COLUMNS; column++)
        {
            pipes[row][column]->render();
        }
    }
    pipeEnd->render();
    pipeStart->render();
    buttons[BUTTON_E_SKIP]->render();
    buttons[BUTTON_E_PUMP]->render();
    buttons[BUTTON_E_MUTE]->render();
    stringstream timeText;
    timeText.str("");
    if (gTimer.getTicks() < 10000)
    {
        timeText << "000" << (gTimer.getTicks() / 1000);
    }
    else if (gTimer.getTicks() >= 10000 && gTimer.getTicks() < 100000)
    {
        timeText << "00" << (gTimer.getTicks() / 1000);
    }
    else if (gTimer.getTicks() >= 100000 && gTimer.getTicks() < 1000000)
    {
        timeText << "0" << (gTimer.getTicks() / 1000);
    }
    else
    {
        timeText << (gTimer.getTicks() / 1000);
    }
    SDL_Color textColor = {255, 0, 0, 255};
    if (workingTime)
    {
        if (!gTimerText.loadFromText(timeText.str().c_str(), textColor))
        {
            cout << "Error rendering time texture!" << endl;
            workingTime = false;
        }
    }
    gTimerText.render((winW - gTimerText.getWidth()) - (gTimerText.getWidth() / 2), winH / 2);
}


int main(int argc, char* argv[])
{
    bool quit = false;
    if (!init())
    {
        cout << "Error during initialisation." << endl;
        quit = true;
    }
    else
    {
        if (!loadMedia())
        {
            cout << "Error loading media." << endl;
            quit = true;
        }
        else
        {
            SDL_Event e;
            Uint32 startTime = 0;
            Uint32 delayTime = 140;
            newGame();
            while (!quit)
            {
                if (SDL_PollEvent(&e) != 0)
                {
                    for (int row = 0; row < TOTAL_ROWS; row++)
                    {
                        for (int column = 0; column < TOTAL_COLUMNS; column++)
                        {
                            pipes[row][column]->handleEvent(&e);
                        }
                    }
                    buttons[BUTTON_E_MUTE]->handleEvent(&e);
                    switch (e.type)
                    {
                    case SDL_QUIT:
                        quit = true;
                        break;
                    }
                    if (quit)
                    {
                        break;
                    }
                    switch (e.key.keysym.sym)
                    {
                    case SDLK_ESCAPE:
                        newGame();
                        break;
                    }
                    bool checkPump = buttons[BUTTON_E_PUMP]->handleEvent(&e);
                    if (checkPump)
                    {
                        while (pump->checkNextPipe() && !quit)
                        {
                            startTime = SDL_GetTicks();
                            pipes[pump->getCurrentRow()][pump->getCurrentColumn()]->toggleFlow();
                            while (SDL_GetTicks() - startTime < delayTime)
                            {
                                if (SDL_PollEvent(&e) != 0)
                                {
                                    buttons[BUTTON_E_MUTE]->handleEvent(&e);
                                    switch (e.type)
                                    {
                                    case SDL_QUIT:
                                        quit = true;
                                        break;
                                    }
                                }
                                renderAll();
                                SDL_RenderPresent(gRenderer);
                                if (quit)
                                {
                                    break;
                                }
                            }
                            pipes[pump->getCurrentRow()][pump->getCurrentColumn()]->toggleFlow();
                            if (quit)
                            {
                                break;
                            }
                            if (!muted)
                            {
                                Mix_PlayChannel(-1, gWaterFlow, 0);
                            }
                        }
                        if (quit)
                        {
                            break;
                        }
                        pipes[pump->getCurrentRow()][pump->getCurrentColumn()]->toggleFlow();
                        startTime = SDL_GetTicks();
                        while (SDL_GetTicks() - startTime < delayTime)
                        {
                            if (SDL_PollEvent(&e) != 0)
                            {
                                muted = buttons[BUTTON_E_MUTE]->handleEvent(&e);
                                switch (e.type)
                                {
                                case SDL_QUIT:
                                    quit = true;
                                    break;
                                }
                            }
                        }
                        pipes[pump->getCurrentRow()][pump->getCurrentColumn()]->toggleFlow();
                        if (quit)
                        {
                            break;
                        }
                        else if (pump->reachedEnd())
                        {
                            gTimer.pause();
                            // add in font texture showing how many pipes were used, how many clicks time taken etc.
                            banner->setType(true);
                            while (true)
                            {
                                renderAll();
                                banner->render();
                                buttons[BUTTON_E_AGAIN]->render();
                                SDL_RenderPresent(gRenderer);
                                if (SDL_PollEvent(&e) != 0)
                                {
                                    if (buttons[BUTTON_E_AGAIN]->handleEvent(&e))
                                    {
                                        break;
                                    }
                                    buttons[BUTTON_E_MUTE]->handleEvent(&e);
                                    switch (e.type)
                                    {
                                    case SDL_QUIT:
                                        quit = true;
                                        break;
                                    }
                                    if (quit)
                                    {
                                        break;
                                    }
                                }
                            }
                        }
                        else
                        {
                            gTimer.pause();
                            banner->setType(false);
                            while (true)
                            {
                                renderAll();
                                banner->render();
                                buttons[BUTTON_E_AGAIN]->render();
                                SDL_RenderPresent(gRenderer);
                                if (SDL_PollEvent(&e) != 0)
                                {
                                    if (buttons[BUTTON_E_AGAIN]->handleEvent(&e))
                                    {
                                        break;
                                    }
                                    buttons[BUTTON_E_MUTE]->handleEvent(&e);
                                    switch (e.type)
                                    {
                                    case SDL_QUIT:
                                        quit = true;
                                        break;
                                    }
                                    if (quit)
                                    {
                                        break;
                                    }
                                }
                            }
                            if (quit)
                            {
                                break;
                            }
                        }
                        newGame();
                    }
                    bool checkSkip = buttons[BUTTON_E_SKIP]->handleEvent(&e);
                    if (checkSkip)
                    {
                        newGame();
                    }
                }
                renderAll();
                SDL_RenderPresent(gRenderer);
            }
        }
    }
    close();
    return 0;
}
