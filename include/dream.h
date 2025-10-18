#pragma once
#include <string>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

class GameObject;

class Texture
{
public:
    unsigned int textureHandle = 0;
    Texture(std::string path);
};

class Callbackable
{
public:
    virtual void start() = 0;
    virtual void tick(float delta) = 0;
    virtual void render() = 0;
    virtual void destroy() = 0;
};

class Component;

class GameObject : public Callbackable
{
public:
    virtual void start() override;
    virtual void tick(float delta) override;
    virtual void render() override;
    virtual void destroy() override;

    void postTick(float delta);
    void processTick(float delta);

    void postRender();
    void processRender();

    void AddChild(GameObject* object);
private:
    GameObject* parent;
    std::vector<GameObject*> children;
    std::vector<Component*> components;


};

class Component : public Callbackable
{
public:

    virtual void start() override;
    virtual void tick(float delta) override;
    virtual void render() override;
    virtual void destroy() override;
};

class DrawCall
{
public:
    virtual void draw() =0;
    virtual ~DrawCall();

};

class QuadDrawCall : public DrawCall
{
    glm::vec2 position;
    glm::vec2 size;
    std::string texture = "";

public:
    QuadDrawCall(glm::vec2 position, glm::vec2 size, std::string texture, glm::vec4 color);
    virtual void draw() override;
};

class Shader
{
private:
public:
    Shader(std::string path);
    void bind();

    unsigned int shaderHandle = 0;
};


bool DreamKeyDown(int key);
bool DreamKeyPressed(int key);
bool DreamKeyReleased(int key);

void DreamProcessRenderQueue();
void DreamAddGameObject(GameObject* object);


Shader* GetShader(std::string path);
Texture* GetTexture(std::string path);
void DreamInit();
void DreamTerminate();
void DreamInitFunc(void (*initFunc)());
void DreamMainLoopFunc(void (*loopFunc)(float));

void DreamStart();

void UseOrtho();

unsigned int GrabVbo();

void DreamDrawQuadSolid(float x, float y, float w, float h, glm::vec4 color);
void DreamDrawQuadTexture(float x, float y, float w, float h, glm::vec4 color, std::string texture);

void DrawSprite(int x, int y, std::string sprite);
void DrawSprite(int x, int y, int w, int h, std::string sprite);
