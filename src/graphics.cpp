#include <glm/ext/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "dream.h"
#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <fstream>
#include "stb_image.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLFWwindow* window;
unsigned short screenWidth =800;
unsigned short screenHeight =600;
unsigned int vao;
unsigned int quadVbo;



void PhysicsMomentumComponent::tick(float delta)
{
    this->gameobject->position += this->momentum * delta;
}

void PhysicsMomentumComponent::ApplyForce(glm::vec2 force, float delta)
{
    momentum += force * delta;
}


unsigned short GetScreenWidth()
{
    return screenWidth;
}

unsigned short GetScreenHeight()
{
    return screenHeight;
}
void GuiText(std::string text)
{
    ImGui::Text(text.c_str());
}

void BeginWindow(std::string name)
{
    ImGui::Begin(name.c_str());
}


void EndWindow()
{
    ImGui::End();
}

float delta = 0.0f;
void GLAPIENTRY
MessageCallback( GLenum source,
                 GLenum type,
                 GLuint id,
                 GLenum severity,
                 GLsizei length,
                 const GLchar* message,
                 const void* userParam )
{
  fprintf( stderr, "GL CALLBACK: %s \n\ttype = 0x%x, \n\tseverity = 0x%x, \n\tmessage = %s\n",
           ( type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : "" ),
            type, severity, message );
}

std::map<std::string, Shader*> shaderCache;
std::map<std::string, Texture*> textureCache;


enum class DreamKeyState {
  UP,
  PRESSED,
  DOWN,
  RELEASED
};

std::map<int, DreamKeyState> inputMap;

float quadVerts[] = {
    -0.5f,-0.5f,-0.25f,        0.0f,0.0f,
    0.5f,-0.5f,-0.25f,         1.0f,0.0f,
    0.5f,0.5f,-0.25f,          1.0f,1.0f,

    -0.5f,0.5f,-0.25f,         0.0f,1.0f,
    -0.5f,-0.5f,-0.25f,        0.0f,0.0f,
    0.5f,0.5f,-0.25f,          1.0f,1.0f
};

std::vector<DrawCall*> renderQueue;

GameObject* sceneRoot = nullptr;

float start = 0.0f;

void (*curInitFunc)() = nullptr;
void (*curLoopFunc)(float) = nullptr;

Texture::Texture(std::string path)
{
    int width,height,nChannels;
    unsigned char* data = stbi_load(path.c_str(),&width,&height,&nChannels,0);
    if(data)
    {
        GLuint texture;
        glGenTextures(1,&texture);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vao);

        glBindTexture(GL_TEXTURE_2D, texture);
        glActiveTexture(GL_TEXTURE0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,data);
        glGenerateMipmap(GL_TEXTURE_2D);
        this->textureHandle = texture;
        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load texture: " << path << "\n";
    }
}

GameObject* DreamAddGameObject(GameObject* object)
{
    sceneRoot->AddChild(object);
    return object;

}


bool DreamKeyDown(int key) {

  try {
    if (inputMap.at(key) == DreamKeyState::DOWN ||
        inputMap.at(key) == DreamKeyState::PRESSED) {
      return true;
    } else {
      return false;
    }
  } catch (std::exception e) {
    return false;
  }
}

bool DreamKeyPressed(int key) {
  try {
    if (inputMap.at(key) == DreamKeyState::PRESSED) {
      return true;
    } else {
      return false;
    }
  } catch (std::exception e) {
    return false;
  }
}

bool DreamKeyReleased(int key) {
  try {
    if (inputMap.at(key) == DreamKeyState::RELEASED) {
      return true;
    } else {
      return false;
    }
  } catch (std::exception e) {
    return false;
  }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  if (action == GLFW_PRESS)
  {
      inputMap[key] = DreamKeyState::PRESSED;
  }
  else if (action == GLFW_RELEASE)
  {
      inputMap[key] = DreamKeyState::RELEASED;
  }
}

Texture* GetTexture(std::string path)
{
    if(textureCache.find(path) != textureCache.end())
    {
        return textureCache.at(path);
    }
    else
    {
        Texture* texture = new Texture(path);
        textureCache.emplace(path,texture);
        return texture;
    }
}

Shader* GetShader(std::string path)
{
    if(shaderCache.find(path) != shaderCache.end())
    {
        return shaderCache.at(path);
    }
    else
    {
        Shader* shader = new Shader(path);
        shaderCache.emplace(path, shader);
        return shader;
    }
}


void Shader::bind()
{
    glUseProgram(this->shaderHandle);
}


Shader::Shader(std::string path)
{
    std::string vertexSource;
    std::string fragmentSource;

    std::ifstream vertFile("shaders/" + path + ".vsl");
    if(!vertFile.is_open())
    {
        std::cout << "Failed to find file: " + path + ".vsl\n";
    }

    std::string line;

    while(std::getline(vertFile,line))
    {
        vertexSource += line;
        vertexSource += "\n";
    }

    std::ifstream fragFile("shaders/" + path + ".fsl");

    if(!fragFile.is_open())
    {
        std::cout << "Failed to find file: " + path + ".fsl\n";
    }

    while(std::getline(fragFile,line))
    {
        fragmentSource += line;
        fragmentSource += "\n";
    }

    const char* vertSrc = vertexSource.c_str();
    const char* fragSrc = fragmentSource.c_str();


    int success;

    unsigned int vertexShader;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertSrc,NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
        std::cout << "ERROR: Vertex Shader Compilation Failed\n\tShader:\t" << path << "\n\t"<<infoLog<<"\n";
    }


    unsigned int fragmentShader;
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragSrc,NULL);
    glCompileShader(fragmentShader);
    if(!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
        std::cout << "ERROR: Fragment Shader Compilation Failed\n\tShader:\t" << path << "\n\t"<<infoLog<<"\n";
    }
    shaderHandle = glCreateProgram();

    glAttachShader(shaderHandle,vertexShader);
    glAttachShader(shaderHandle, fragmentShader);

    glLinkProgram(shaderHandle);
}



DrawCall::~DrawCall()
{

}



void DreamInitFunc(void (*initFunc)())
{
    curInitFunc = initFunc;
}

void DreamMainLoopFunc(void (*loopFunc)(float))
{
    curLoopFunc = loopFunc;
}

void DreamProcessRenderQueue()
{
    for(DrawCall* d : renderQueue)
    {
        d->draw();
        delete d;
    }
    renderQueue.clear();
}


void ProcessInput()
{
  for (std::pair<int,DreamKeyState> key : inputMap)
  {
    if (key.second == DreamKeyState::PRESSED)
    {
        inputMap[key.first] = DreamKeyState::DOWN;
    } else if (key.second == DreamKeyState::RELEASED) {
        inputMap[key.first] = DreamKeyState::UP;
    }
  }
}


void InputBindComponent::tick(float delta) {
  if (DreamKeyDown(this->key)) {
      inputBinding(delta,this);
  }
}

InputBindComponent::InputBindComponent(int key, void (*binding)(float,Component*)) {
  inputBinding = binding;
  this->key = key;
}

void DreamStart()
{
    DreamInit();
    //enable debugging
    glEnable              ( GL_DEBUG_OUTPUT );
    glDebugMessageCallback( MessageCallback, 0 );

    curInitFunc();
    while(!glfwWindowShouldClose((window)))
    {
        start = glfwGetTime();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwPollEvents();
        ProcessInput();
        curLoopFunc(delta);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if(sceneRoot != nullptr)
        {
            sceneRoot->processTick(delta);
            sceneRoot->processRender();
        }
        DreamProcessRenderQueue();


        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        delta = glfwGetTime() - start;
    }
    DreamTerminate();
}

void window_size_callback(GLFWwindow *window, int width, int height) {
  screenWidth = width;
  screenHeight = height;
  glViewport(0,0,screenWidth,screenHeight);
}

void DreamInit()
{
    if(!glfwInit())
    {
        std::cout << "glfw initialization failed. try again jackass.\n";
    }
    else
    {
        window = glfwCreateWindow(screenWidth,screenHeight,"Dream - v0.0.2",NULL,NULL);
        glfwMakeContextCurrent(window);
        glewInit();
        glfwSwapInterval(0);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();


        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.ScaleAllSizes(1.0);
        style.FontScaleDpi = 1.0;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 330 core");

        glfwSetKeyCallback(window, key_callback);
        glfwSetWindowSizeCallback(window,window_size_callback);
        glGenVertexArrays(1,&vao);
        glBindVertexArray(vao);

        quadVbo = GrabVbo();
        glBindBuffer(GL_ARRAY_BUFFER,quadVbo);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
        glBufferData(GL_ARRAY_BUFFER,sizeof(quadVerts),quadVerts,GL_STATIC_DRAW);
        sceneRoot = new GameObject();


        glEnable(GL_BLEND);
    }
}

SpriteRenderComponent::SpriteRenderComponent(std::string sprite,glm::vec2 size, glm::vec2 offset)
{
    this->offset = offset;
    this->size = size;
    spriteName = sprite;
    cachedSprite = GetTexture(sprite);
}
SpriteRenderComponent::SpriteRenderComponent(std::string sprite,glm::vec2 size)
{
    this->size = size;
    this->offset = glm::vec2(0,0);
    spriteName = sprite;
    cachedSprite = GetTexture(sprite);
}

void SpriteRenderComponent::render()
{
    DreamDrawQuadTexture(this->gameobject->position.x + this->offset.x,this->gameobject->position.y + this->offset.y,this->gameobject->size.x,this->gameobject->size.y,this->gameobject->rotation,glm::vec4(1.0f,1.0f,1.0f,1.0f),this->spriteName);
}

void DreamTerminate()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

glm::mat4 Ortho() {
    return glm::ortho(0.0f,(float)screenWidth,(float)screenHeight,0.0f);
}

QuadDrawCall::QuadDrawCall(glm::vec2 position, glm::vec2 size, float rotation,
                           std::string texture, glm::vec4 color) {
    this->rotation = rotation;
    this->position = position;
    this->size = size;
    this->texture = texture;
}


void QuadDrawCall::draw() {

    GetShader("quad")->bind();
    glm::mat4 transform = glm::mat4(1.0f);

    transform = glm::translate(transform,glm::vec3(position.x, position.y, 0.0f));

    transform = glm::rotate(transform, this->rotation,glm::vec3(0,0,1));
    transform = glm::scale(transform, glm::vec3(size.x,size.y,1.0f));
    glm::mat4 ortho = Ortho();
    if(this->texture != "")
    {
        Texture *tex = GetTexture("test.png");
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex->textureHandle);
    }

    glUniformMatrix4fv(
        glGetUniformLocation(GetShader("quad")->shaderHandle, "ortho"), 1,GL_FALSE,glm::value_ptr(ortho));
    glUniformMatrix4fv(
        glGetUniformLocation(GetShader("quad")->shaderHandle, "transform"), 1,GL_FALSE,glm::value_ptr(transform));
    glUniform1i(glGetUniformLocation(GetShader("quad")->shaderHandle,"texture1"),0);

    GetShader("quad")->bind();
    glBindBuffer(GL_ARRAY_BUFFER,quadVbo);
    glDrawArrays(GL_TRIANGLES,0,6);
}

void DreamDrawQuadSolid(float x, float y, float w, float h,float rotation, glm::vec4 color)
{
    QuadDrawCall* call = new QuadDrawCall(glm::vec2(x,y),glm::vec2(w,h),rotation,"",color);
    renderQueue.push_back(call);
}

void DreamDrawQuadTexture(float x, float y, float w, float h,float rotation, glm::vec4 color, std::string texture)
{
    QuadDrawCall* call = new QuadDrawCall(glm::vec2(x,y), glm::vec2(w,h),rotation,texture,color);
    renderQueue.push_back(call);
}


void GameObject::processTick(float delta)
{
    for(Component* c : this->components)
    {
        c->tick(delta);
    }
    tick(delta);
    postTick(delta);



}

void GameObject::postTick(float delta)
{
    for(GameObject* child : children)
    {
        child->processTick(delta);
    }
}

void GameObject::processRender()
{
    for(Component* c : this->components)
    {
        c->render();
    }
    render();
    postRender();
}

void GameObject::postRender()
{
    for(GameObject* child : children)
    {
        child->processRender();
    }
}

GameObject* GameObject::AddComponent(Component* c)
{
    this->components.push_back(c);
    c->gameobject = this;
    return this;
}

GameObject* GameObject::AddChild(GameObject* object)
{
    children.push_back(object);
    return object;
}



ConstantTravelComponent::ConstantTravelComponent(glm::vec2 direction, float speed)
{
    this->direction = glm::normalize(direction);
    this->speed = speed;
}

void ConstantTravelComponent::tick(float delta)
{
    this->gameobject->position += direction * speed * delta;
}

void GameObject::start(){}
void GameObject::tick(float delta){}
void GameObject::render(){}
void GameObject::destroy(){}

void Component::start(){}
void Component::tick(float delta){}
void Component::render(){}
void Component::destroy(){}
//=======OPENGL UTILITIES======================
unsigned int GrabVbo()
{
    unsigned int vbo;
    glGenBuffers(1, &vbo);
    return vbo;
}
