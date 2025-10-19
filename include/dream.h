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
    GameObject* AddComponent(Component* c);
    GameObject* AddChild(GameObject* object);

    glm::vec2 position = glm::vec2(20.0f,20.0f);
    glm::vec2 size = glm::vec2(128.0f,128.0f);

protected:
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
    GameObject* gameobject;
};

class DrawCall
{
public:
    virtual void draw() =0;
    virtual ~DrawCall();

};

class SpriteRenderComponent : public Component
{
private:
    glm::vec2 offset;
    glm::vec2 size;
    Texture* cachedSprite;
public:
    virtual void render() override;

    std::string spriteName;

    SpriteRenderComponent(std::string sprite,glm::vec2 size,glm::vec2 offset);
    SpriteRenderComponent(std::string sprite, glm::vec2 offset);
};

class ConstantTravelComponent : public Component
{
  private:
    glm::vec2 direction;
    float speed;
  public:
    ConstantTravelComponent(glm::vec2 direction, float speed);
    virtual void tick(float delta) override;
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

void BeginWindow(std::string name);
void EndWindow();
void GuiText(std::string text);




unsigned short GetScreenWidth();
unsigned short GetScreenHeight();

bool DreamKeyDown(int key);
bool DreamKeyPressed(int key);
bool DreamKeyReleased(int key);

void DreamProcessRenderQueue();
GameObject* DreamAddGameObject(GameObject* object);


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
